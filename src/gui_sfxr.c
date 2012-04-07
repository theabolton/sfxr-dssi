/* sfxr-dssi DSSI software synthesizer plugin
 *
 * Copyright (C) 2011 Sean Bolton.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stddef.h>

#include <why/lua.h>
#include <why/lauxlib.h>

#include "ports.h"
#include "gui_main.h"

const char *plugin_gui_name = "sfxr-dssi_gtk";

const luaL_Reg plugin_gui_lib[] = {
    // { "tempo_name",            tempo_name },
    { NULL, NULL }
};

const struct plugin_gui_enums_t plugin_gui_enums[] = {
    { "PORT_OUTPUT",        PORT_OUTPUT },
    { "PORT_SOUND_VOL",     PORT_SOUND_VOL },
    { "PORT_WAVE_TYPE",     PORT_WAVE_TYPE },
    { "PORT_ENV_ATTACK",    PORT_ENV_ATTACK },
    { "PORT_ENV_SUSTAIN",   PORT_ENV_SUSTAIN },
    { "PORT_ENV_PUNCH",     PORT_ENV_PUNCH },
    { "PORT_ENV_DECAY",     PORT_ENV_DECAY },
    { "PORT_BASE_FREQ",     PORT_BASE_FREQ },
    { "PORT_FREQ_LIMIT",    PORT_FREQ_LIMIT },
    { "PORT_FREQ_RAMP",     PORT_FREQ_RAMP },
    { "PORT_FREQ_DRAMP",    PORT_FREQ_DRAMP },
    { "PORT_VIB_STRENGTH",  PORT_VIB_STRENGTH },
    { "PORT_VIB_SPEED",     PORT_VIB_SPEED },
    { "PORT_ARP_MOD",       PORT_ARP_MOD },
    { "PORT_ARP_SPEED",     PORT_ARP_SPEED },
    { "PORT_DUTY",          PORT_DUTY },
    { "PORT_DUTY_RAMP",     PORT_DUTY_RAMP },
    { "PORT_REPEAT_SPEED",  PORT_REPEAT_SPEED },
    { "PORT_PHA_OFFSET",    PORT_PHA_OFFSET },
    { "PORT_PHA_RAMP",      PORT_PHA_RAMP },
    { "PORT_LPF_FREQ",      PORT_LPF_FREQ },
    { "PORT_LPF_RAMP",      PORT_LPF_RAMP },
    { "PORT_LPF_RESONANCE", PORT_LPF_RESONANCE },
    { "PORT_HPF_FREQ",      PORT_HPF_FREQ },
    { "PORT_HPF_RAMP",      PORT_HPF_RAMP },
    { "PORTS_COUNT",        PORTS_COUNT },
    { NULL, 0 }
};

