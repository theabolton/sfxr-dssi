/* synthtoys DSSI software synthesizer framework
 *
 * Copyright (C) 2011 Sean Bolton.
 *
 * Portions of this file may have come from Chris Cannam and Steve
 * Harris's public domain DSSI example code.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>

#include <whygui.h>

#include <ladspa.h>
#include <lo/lo.h>

#include "debug.h"
#include "ports.h"
#include "gui_main.h"

/* ==== global variables ==== */

lua_State* L;

/* ==== local variables ==== */

static char *     osc_host_url;
static char *     osc_self_url = NULL;
static lo_address osc_host_address;
static char *     osc_instance_path;
static char *     osc_control_path;
static char *     osc_exiting_path;
static char *     osc_midi_path;
static char *     osc_update_path;
static lo_server  osc_server = NULL;
static gint       osc_server_socket_tag;
static int        osc_receipt_callback_ref;

static int   gui_test_mode = 0;
static char *ui_directory;

/* ==== OSC handling ==== */

static char *
osc_build_path(char *base_path, char *method)
{
    char buffer[256];
    char *full_path;

    snprintf(buffer, 256, "%s%s", base_path, method);
    if (!(full_path = strdup(buffer))) {
        GUIDB_MESSAGE(DB_OSC, ": out of memory!\n");
        exit(1);
    }
    return full_path;
}

static void
osc_error(int num, const char *msg, const char *path)
{
    GUIDB_MESSAGE(DB_OSC, " error: liblo server error %d in path \"%s\": %s\n",
            num, (path ? path : "(null)"), msg);
}

int
osc_receipt_handler(const char *path, const char *types, lo_arg **argv,
                    int argc, lo_message msg, void *user_data)
{
    int i;

    /* GUIDB_MESSAGE(DB_OSC, " osc_receipt_handler: passing message to '%s' to Lua\n", path); */

    /* get the Lua callback function */
    lua_rawgeti(L, LUA_REGISTRYINDEX, osc_receipt_callback_ref);
    if (lua_isnil(L, -1)) {
        lua_pop(L, 1);
        return luaL_error(L, "callback handler not found");
    }

    /* push args, taking care to not overflow the stack */
    lua_pushstring(L, path);
    lua_pushstring(L, types);
    lua_checkstack(L, argc);
    for (i = 0; i < argc; ++i) {
        switch (types[i]) {
          case 'i': lua_pushinteger(L, argv[i]->i); break;
          case 'f': lua_pushnumber(L, argv[i]->f);  break;
          default:
            GUIDB_MESSAGE(DB_OSC, " osc_receipt_handler error: unhandled argument type '%c'\n", types[i]);
            lua_pushnil(L);
            break;
        }
    }

    /* call the callback */
    lua_call(L, 2 + argc, 0);

    return 0;
}

void
osc_data_on_socket_callback(gpointer data, gint source,
                            GdkInputCondition condition)
{
    lo_server server = (lo_server)data;

    lo_server_recv_noblock(server, 0);
}

/* ==== Lua support ==== */

static int
lua_debug_message(lua_State *L)
{
    /* note that this doesn't handle any args, i.e. it expects all formatting
     * to have already been done. */

    GUIDB_MESSAGE(luaL_checkinteger(L, 1), "%s", luaL_checkstring(L, 2));

    return 0;
}

static int
send_control(lua_State *L)
{
    int   index = luaL_checkinteger(L, 1);
    float value = luaL_checknumber(L, 2);

    lo_send(osc_host_address, osc_control_path, "if", index, value);

    return 0;
}

static int
send_exiting(lua_State *L)
{
    lo_send(osc_host_address, osc_exiting_path, "");

    return 0;
}

static int
send_midi(lua_State *L)
{
    unsigned char midi[4];

    midi[0] = 0;
    midi[1] = luaL_checkinteger(L, 1);
    midi[2] = luaL_checkinteger(L, 2);
    midi[3] = luaL_checkinteger(L, 3);

    lo_send(osc_host_address, osc_midi_path, "m", midi);

    return 0;
}

static int
send_update_request(lua_State *L)
{
    if (osc_self_url)
        lo_send(osc_host_address, osc_update_path, "s", osc_self_url);

    return 0;
}

static int
osc_server_start(lua_State *L)
{
    char *url;

    /* save reference to the Lua callback function in the registry */
    luaL_checktype(L, 1, LUA_TFUNCTION);
    osc_receipt_callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

    /* create server */
    osc_server = lo_server_new(NULL, osc_error);
    lo_server_add_method(osc_server, NULL, NULL, osc_receipt_handler, NULL);

    /* get our URL */
    url = lo_server_get_url(osc_server);
    osc_self_url = osc_build_path(url, (strlen(osc_instance_path) > 1 ? osc_instance_path + 1 : osc_instance_path));
    free(url);
    GUIDB_MESSAGE(DB_OSC, ": listening on %s\n", osc_self_url);

    /* add OSC server socket to GTK+'s watched I/O */
    if (lo_server_get_socket_fd(osc_server) < 0) {
        return luaL_error(L, "%s fatal: OSC transport does not support exposing socket fd",
                          plugin_gui_name);
    }
    osc_server_socket_tag = gdk_input_add(lo_server_get_socket_fd(osc_server),
                                          GDK_INPUT_READ,
                                          osc_data_on_socket_callback,
                                          osc_server);

    return 0;
}

static int
osc_server_disconnect(lua_State *L)
{
    if (osc_server) gdk_input_remove(osc_server_socket_tag);

    return 0;
}

static int
port_hint(lua_State *L)
{
    int port = luaL_checkint(L, 1);

    if (port < 0 || port >= PORTS_COUNT) {
        lua_pushnil(L);
        return 1;
    }

    lua_newtable(L);
    lua_pushstring(L, "HintDescriptor");
    lua_pushinteger(L, port_hints[port].HintDescriptor);
    lua_settable(L, -3);
    lua_pushstring(L, "LowerBound");
    lua_pushnumber(L, port_hints[port].LowerBound);
    lua_settable(L, -3);
    lua_pushstring(L, "UpperBound");
    lua_pushnumber(L, port_hints[port].UpperBound);
    lua_settable(L, -3);

    return 1;
}

static const luaL_Reg main_lib[] = {
    { "_GUIDB_MESSAGE",        lua_debug_message },
    { "send_control",          send_control },
    { "send_exiting",          send_exiting },
    { "send_midi",             send_midi },
    { "send_update_request",   send_update_request },
    { "osc_server_start",      osc_server_start },
    { "osc_server_disconnect", osc_server_disconnect },
    { "port_hint",             port_hint },
    { NULL, NULL }
};

static const struct plugin_gui_enums_t main_enums[] = {
    { "LADSPA_HINT_INTEGER", LADSPA_HINT_INTEGER },
    { "DB_ERROR", (-1) }, 
    { "DB_DSSI",  DB_DSSI }, 
    { "DB_AUDIO", DB_AUDIO },
    { "DB_NOTE",  DB_NOTE }, 
    { "DB_DATA",  DB_DATA }, 
    { "DB_MAIN",  DB_MAIN }, 
    { "DB_OSC",   DB_OSC },  
    { "DB_IO",    DB_IO },   
    { "DB_GUI",   DB_GUI },  
    { NULL, 0 }
};

static int
run_lua_script(lua_State *L, const char *script)
{
    int rc = 1;
    char *path = g_build_filename(ui_directory, script, NULL);

    if (luaL_loadfile(L, path)) {
        const char *error = lua_tostring(L, -1);
#ifdef DSSP_DEBUG
        GUIDB_MESSAGE(-1, ": failed to load Lua script '%s': %s\n", path, error);
#else
        fprintf(stderr, "%s: failed to load Lua script '%s': %s\n", plugin_gui_name, path, error);
#endif
        rc = 0;

    } else if (lua_pcall(L, 0, LUA_MULTRET, 0)) {
        const char *error = lua_tostring(L, -1);
#ifdef DSSP_DEBUG
        GUIDB_MESSAGE(-1, ": error while running Lua script '%s': %s\n", path, error);
#else
        fprintf(stderr, "%s: error while running Lua script '%s': %s\n", plugin_gui_name, path, error);
#endif
        rc = 0;
    }

    g_free(path);
    return rc;
}

/* ==== main ==== */

int
main (int argc, char *argv[])
{
    char *test_argv[5] = { NULL, NULL, "-", "-", "<test>" };
    int i;
    char *host, *port;

    DSSP_DEBUG_INIT(plugin_gui_name);

#ifdef DSSP_DEBUG
    GUIDB_MESSAGE(DB_MAIN, " starting (pid %d)...\n", getpid());
#else
    fprintf(stderr, "%s starting (pid %d)...\n", plugin_gui_name, getpid());
#endif
    /* { int i; fprintf(stderr, "args:\n"); for(i=0; i<argc; i++) printf("%d: %s\n", i, argv[i]); } */

    /* initialize GTK+ */
    gtk_init(&argc, &argv);

    if (argc > 1 && !strcmp(argv[1], "-test")) {
        gui_test_mode = 1;
        test_argv[0] = argv[0];
        test_argv[1] = "osc.udp://localhost:9/test/mode";
        if (argc >= 5)
            test_argv[4] = argv[4];
        argc = 5;
        argv = test_argv;
    } else if (argc != 5) {
        fprintf(stderr, "usage: %s <osc url> <plugin dllname> <plugin label> <user-friendly id>\n"
                        "   or: %s -test\n", argv[0], argv[0]);
        exit(1);
    }
    ui_directory = g_path_get_dirname(argv[0]);
    GUIDB_MESSAGE(DB_MAIN, ": ui_directory is '%s'\n", ui_directory);

    /* initialize Lua */
    L = luaL_newstate(); 
    luaL_openlibs(L);
    luaopen_ygtk(L);
    /* register C functions */
    lua_pushglobaltable(L);
    luaL_setfuncs(L, main_lib, 0);               /* -- _G */
    for (i = 0; main_enums[i].name != NULL; i++) {
	lua_pushinteger(L, main_enums[i].value);
	lua_setfield(L, -2, main_enums[i].name); /* -- _G */
    }
    luaL_setfuncs(L, plugin_gui_lib, 0);         /* -- _G */
    for (i = 0; plugin_gui_enums[i].name != NULL; i++) {
	lua_pushinteger(L, plugin_gui_enums[i].value);
	lua_setfield(L, -2, plugin_gui_enums[i].name); /* -- _G */
    }
    lua_pushstring(L, PACKAGE_VERSION);
    lua_setfield(L, -2, "plugin_version");       /* -- _G */
    lua_pushboolean(L, gui_test_mode);
    lua_setfield(L, -2, "gui_test_mode");        /* -- _G */
    lua_pushstring(L, ui_directory);
    lua_setfield(L, -2, "ui_directory");         /* -- _G */
    lua_pushstring(L, argv[2]);
    lua_setfield(L, -2, "plugin_so_name");       /* -- _G */
    lua_pushstring(L, argv[3]);
    lua_setfield(L, -2, "plugin_label");         /* -- _G */
    lua_pushstring(L, argv[4]);
    lua_setfield(L, -2, "user_friendly_id");     /* -- _G */

#if defined(__APPLE__)
    lua_pushliteral(L, "darwin");
#elif defined(__linux)
    lua_pushliteral(L, "linux");
#else
    lua_pushliteral(L, "unknown");
#endif
    lua_setfield(L, -2, "platform");             /* -- _G */

    /* set up OSC support */
    osc_host_url = argv[1];
    host = lo_url_get_hostname(osc_host_url);
    port = lo_url_get_port(osc_host_url);
    osc_instance_path = lo_url_get_path(osc_host_url);
    osc_host_address = lo_address_new(host, port);
    osc_control_path   = osc_build_path(osc_instance_path, "/control");
    osc_exiting_path   = osc_build_path(osc_instance_path, "/exiting");
    osc_midi_path      = osc_build_path(osc_instance_path, "/midi");
    osc_update_path    = osc_build_path(osc_instance_path, "/update");

    /* let Lua see OSC stuff */
    lua_pushstring(L, osc_instance_path);
    lua_setfield(L, -2, "osc_instance_path");    /* -- _G */
    lua_pop(L, 1);                               /* -- */

    /* let Lua gui.lua take it from here */
    run_lua_script(L, "gui.lua");

    /* clean up and exit */
    GUIDB_MESSAGE(DB_MAIN, ": yep, we got to the C cleanup!\n");

    /* cleanup Lua */
    lua_close(L); 

    /* clean up OSC support */
    if (osc_server) lo_server_free(osc_server);
    free(host);
    free(port);
    free(osc_instance_path);
    free(osc_control_path);
    free(osc_exiting_path);
    free(osc_midi_path);
    free(osc_update_path);
    if (osc_self_url) free(osc_self_url);
    free(ui_directory);

    return 0;
}

