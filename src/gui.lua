-- sfxr-dssi DSSI software synthesizer plugin
--
-- Copyright (C) 2011 Sean Bolton and others.
--
-- Portions of this file come from the SDL version of sfxr,
-- copyright (c) 2007 Tomas Pettersson.
--
-- Permission is hereby granted, free of charge, to any person
-- obtaining a copy of this software and associated documentation files
-- (the "Software"), to deal in the Software without restriction,
-- including without limitation the rights to use, copy, modify, merge,
-- publish, distribute, sublicense, and/or sell copies of the Software,
-- and to permit persons to whom the Software is furnished to do so,
-- subject to the following conditions:
--
-- The above copyright notice and this permission notice shall be
-- included in all copies or substantial portions of the Software.
--
-- THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
-- EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
-- MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
-- IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
-- CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
-- TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
-- SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

function GUIDB_MESSAGE(flag, format, ...)
  local msg = string.format(format, ...)
  _GUIDB_MESSAGE(flag, msg)  -- call the C version
end

package.path = ui_directory .. "/?.lua"
package.cpath = ""
if not pcall(require, "strict") then
  GUIDB_MESSAGE(DB_MAIN, " gui.lua: could not load strict.lua\n")
end

--require("ygtk")

-- declare globals
host_requested_quit = false
main_window = nil
pixbuf = {}
voice_adjustments = {}
voice_handler_ids = {}
voice_widget_duty = nil
voice_widget_duty_ramp = nil
action_adjustments = {}
about_dialog = nil

ACTION = { PICKUP_COIN = 1,
           LASER_SHOOT = 2,
           EXPLOSION   = 3,
           POWERUP     = 4,
           HIT_HURT    = 5,
           JUMP        = 6,
           BLIP_SELECT = 7,
           MUTATE      = 8,
           RANDOMIZE   = 9,
           PLAY_SOUND  = 10,
           -- LOAD_SOUND  = 11,
           -- SAVE_SOUND  = 12
           COUNT = 10
         }

------------------------------------------------------------------------------
--                              OSC callbacks                               --
------------------------------------------------------------------------------

local function wave_type_changed(value)
    voice_widget_duty:set("sensitive", value == 0)
    voice_widget_duty_ramp:set("sensitive", value == 0)
end

local function update_voice_widget(port, value, send_OSC)
    if port < PORT_SOUND_VOL or port >= PORTS_COUNT then
      return
    end

    GUIDB_MESSAGE(DB_GUI, " update_voice_widget: change of port %d to %f\n", port, value)

    local adjustment = voice_adjustments[port]
    if not adjustment then
      --print("update_voice_widget! no adjustment for port:", port)
      return
    end
    local handler_id = voice_handler_ids[port]
    if not handler_id then
      GUIDB_MESSAGE(DB_GUI, " update_voice_widget: no handler_id for port %d\n", port)
      return
    end

    -- temporarily block on_voice_control_change()
    adjustment:signal_handler_block(handler_id)
    adjustment:set_value(value)  -- will emit 'value-changed', causing widget to update
    adjustment:signal_handler_unblock(handler_id)

    if port == PORT_WAVE_TYPE then
        wave_type_changed(value)
    end

    -- if requested, send update to DSSI host
    if send_OSC then
      send_control(port, value)
    end
end

local function osc_control_handler(types, argv)
    if types ~= 'if' or #argv ~= 2 then return end
    update_voice_widget(argv[1], argv[2], false)
end

local osc_handlers = {
  [osc_instance_path .. "/control"] = osc_control_handler,
  [osc_instance_path .. "/hide"] = function() main_window:hide() end,
  [osc_instance_path .. "/quit"] = function() host_requested_quit = true gtk.main_quit() end,
  [osc_instance_path .. "/sample-rate"] = function() end,
  [osc_instance_path .. "/show"] = function() main_window:show() end, -- could do 'if (GTK_WIDGET_MAPPED(main_window)) gdk_window_raise(main_window->window); else ...'
}

local function osc_receipt_callback(path, types, ...)
    local argv = {...}
    GUIDB_MESSAGE(DB_OSC, " osc_receipt_callback: %d args (types '%s') to %s\n", #argv, types, path)
    if osc_handlers[path] then
      osc_handlers[path](types, argv)
    else
      GUIDB_MESSAGE(DB_OSC, " warning: unhandled OSC message to <%s>!\n", path)
    end
end

------------------------------------------------------------------------------
--                           port widget callbacks                          --
------------------------------------------------------------------------------

local test_note_noteon_key = 60
local test_note_noteoff_key = -1
local test_note_velocity = 96

local function release_test_note()
    if test_note_noteoff_key >= 0 then
      send_midi(0x80, test_note_noteoff_key, 0x40)
      test_note_noteoff_key = -1
    end
end

local function rnd(n)
    return math.random(0, n)
end

local function frnd(range)
    return math.random() * range
end

local params = {}

local function ResetParams()
    params[PORT_WAVE_TYPE]=0

    params[PORT_BASE_FREQ]=0.3
    params[PORT_FREQ_LIMIT]=0.0
    params[PORT_FREQ_RAMP]=0.0
    params[PORT_FREQ_DRAMP]=0.0
    params[PORT_DUTY]=0.0
    params[PORT_DUTY_RAMP]=0.0

    params[PORT_VIB_STRENGTH]=0.0
    params[PORT_VIB_SPEED]=0.0

    params[PORT_ENV_ATTACK]=0.0
    params[PORT_ENV_SUSTAIN]=0.3
    params[PORT_ENV_DECAY]=0.4
    params[PORT_ENV_PUNCH]=0.0

    params[PORT_LPF_RESONANCE]=0.0
    params[PORT_LPF_FREQ]=1.0
    params[PORT_LPF_RAMP]=0.0
    params[PORT_HPF_FREQ]=0.0
    params[PORT_HPF_RAMP]=0.0

    params[PORT_PHA_OFFSET]=0.0
    params[PORT_PHA_RAMP]=0.0

    params[PORT_REPEAT_SPEED]=0.0

    params[PORT_ARP_SPEED]=0.0
    params[PORT_ARP_MOD]=0.0
end

local function update_voice_widgets_from_params()
    for port = PORT_WAVE_TYPE, PORTS_COUNT - 1 do
      if voice_adjustments[port]:get_value() ~= params[port] then
        update_voice_widget(port, params[port], true)
        if port == PORT_WAVE_TYPE then
          wave_type_changed(params[port])
        end
      end
    end
end

local function do_generator(action, state)
    if not state then
      release_test_note()
    else
      ResetParams()

      if action == ACTION.PICKUP_COIN then

        params[PORT_BASE_FREQ]=0.4+frnd(0.5)
        params[PORT_ENV_ATTACK]=0.0
        params[PORT_ENV_SUSTAIN]=frnd(0.1)
        params[PORT_ENV_DECAY]=0.1+frnd(0.4)
        params[PORT_ENV_PUNCH]=0.3+frnd(0.3)
        if rnd(1) ~= 0 then
            params[PORT_ARP_SPEED]=0.5+frnd(0.2)
            params[PORT_ARP_MOD]=0.2+frnd(0.4)
        end

      elseif action == ACTION.LASER_SHOOT then

          params[PORT_WAVE_TYPE]=rnd(2)
          if params[PORT_WAVE_TYPE]==2 and rnd(1) ~= 0 then
                  params[PORT_WAVE_TYPE]=rnd(1)
          end
          params[PORT_BASE_FREQ]=0.5+frnd(0.5)
          params[PORT_FREQ_LIMIT]=params[PORT_BASE_FREQ]-0.2-frnd(0.6)
          if params[PORT_FREQ_LIMIT]<0.2 then params[PORT_FREQ_LIMIT]=0.2 end
          params[PORT_FREQ_RAMP]=-0.15-frnd(0.2)
          if rnd(2)==0 then
                  params[PORT_BASE_FREQ]=0.3+frnd(0.6)
                  params[PORT_FREQ_LIMIT]=frnd(0.1)
                  params[PORT_FREQ_RAMP]=-0.35-frnd(0.3)
          end
          if rnd(1) ~= 0 then
                  params[PORT_DUTY]=frnd(0.5)
                  params[PORT_DUTY_RAMP]=frnd(0.2)
          else
                  params[PORT_DUTY]=0.4+frnd(0.5)
                  params[PORT_DUTY_RAMP]=-frnd(0.7)
          end
          params[PORT_ENV_ATTACK]=0.0;
          params[PORT_ENV_SUSTAIN]=0.1+frnd(0.2)
          params[PORT_ENV_DECAY]=frnd(0.4)
          if rnd(1) ~= 0 then
                  params[PORT_ENV_PUNCH]=frnd(0.3)
          end
          if rnd(2)==0 then
                  params[PORT_PHA_OFFSET]=frnd(0.2)
                  params[PORT_PHA_RAMP]=-frnd(0.2)
          end
          if rnd(1) ~= 0 then
                  params[PORT_HPF_FREQ]=frnd(0.3)
          end

      elseif action == ACTION.EXPLOSION then

          params[PORT_WAVE_TYPE]=3
          if rnd(1) ~= 0 then
                  params[PORT_BASE_FREQ]=0.1+frnd(0.4)
                  params[PORT_FREQ_RAMP]=-0.1+frnd(0.4)
          else
                  params[PORT_BASE_FREQ]=0.2+frnd(0.7)
                  params[PORT_FREQ_RAMP]=-0.2-frnd(0.2)
          end
          params[PORT_BASE_FREQ] = params[PORT_BASE_FREQ] * params[PORT_BASE_FREQ]
          if rnd(4)==0 then
                  params[PORT_FREQ_RAMP]=0.0
          end
          if rnd(2)==0 then
                  params[PORT_REPEAT_SPEED]=0.3+frnd(0.5)
          end
          params[PORT_ENV_ATTACK]=0.0
          params[PORT_ENV_SUSTAIN]=0.1+frnd(0.3)
          params[PORT_ENV_DECAY]=frnd(0.5)
          if rnd(1)==0 then
                  params[PORT_PHA_OFFSET]=-0.3+frnd(0.9)
                  params[PORT_PHA_RAMP]=-frnd(0.3)
          end
          params[PORT_ENV_PUNCH]=0.2+frnd(0.6)
          if rnd(1) ~= 0 then
                  params[PORT_VIB_STRENGTH]=frnd(0.7)
                  params[PORT_VIB_SPEED]=frnd(0.6)
          end
          if rnd(2)==0 then
                  params[PORT_ARP_SPEED]=0.6+frnd(0.3)
                  params[PORT_ARP_MOD]=0.8-frnd(1.6)
          end

      elseif action == ACTION.POWERUP then

          if rnd(1) ~= 0 then
                  params[PORT_WAVE_TYPE]=1
          else
                  params[PORT_DUTY]=frnd(0.6)
          end
          if rnd(1) ~= 0 then
                  params[PORT_BASE_FREQ]=0.2+frnd(0.3)
                  params[PORT_FREQ_RAMP]=0.1+frnd(0.4)
                  params[PORT_REPEAT_SPEED]=0.4+frnd(0.4)
          else
                  params[PORT_BASE_FREQ]=0.2+frnd(0.3)
                  params[PORT_FREQ_RAMP]=0.05+frnd(0.2)
                  if rnd(1) ~= 0 then
                          params[PORT_VIB_STRENGTH]=frnd(0.7)
                          params[PORT_VIB_SPEED]=frnd(0.6)
                  end
          end
          params[PORT_ENV_ATTACK]=0.0
          params[PORT_ENV_SUSTAIN]=frnd(0.4)
          params[PORT_ENV_DECAY]=0.1+frnd(0.4)

      elseif action == ACTION.HIT_HURT then
        
          params[PORT_WAVE_TYPE]=rnd(2)
          if params[PORT_WAVE_TYPE]==2 then
                  params[PORT_WAVE_TYPE]=3
          end
          if params[PORT_WAVE_TYPE]==0 then
                  params[PORT_DUTY]=frnd(0.6)
          end
          params[PORT_BASE_FREQ]=0.2+frnd(0.6)
          params[PORT_FREQ_RAMP]=-0.3-frnd(0.4)
          params[PORT_ENV_ATTACK]=0.0
          params[PORT_ENV_SUSTAIN]=frnd(0.1)
          params[PORT_ENV_DECAY]=0.1+frnd(0.2)
          if rnd(1) ~= 0 then
                  params[PORT_HPF_FREQ]=frnd(0.3)
          end

      elseif action == ACTION.JUMP then

          params[PORT_WAVE_TYPE]=0
          params[PORT_DUTY]=frnd(0.6)
          params[PORT_BASE_FREQ]=0.3+frnd(0.3)
          params[PORT_FREQ_RAMP]=0.1+frnd(0.2)
          params[PORT_ENV_ATTACK]=0.0
          params[PORT_ENV_SUSTAIN]=0.1+frnd(0.3)
          params[PORT_ENV_DECAY]=0.1+frnd(0.2)
          if rnd(1) ~= 0 then
                  params[PORT_HPF_FREQ]=frnd(0.3)
          end
          if rnd(1) ~= 0 then
                  params[PORT_LPF_FREQ]=1.0-frnd(0.6)
          end

      elseif action == ACTION.BLIP_SELECT then

          params[PORT_WAVE_TYPE]=rnd(1)
          if params[PORT_WAVE_TYPE]==0 then
                  params[PORT_DUTY]=frnd(0.6)
          end
          params[PORT_BASE_FREQ]=0.2+frnd(0.4)
          params[PORT_ENV_ATTACK]=0.0
          params[PORT_ENV_SUSTAIN]=0.1+frnd(0.1)
          params[PORT_ENV_DECAY]=frnd(0.2)
          params[PORT_HPF_FREQ]=0.1

      elseif action == ACTION.MUTATE then

          for port = PORT_WAVE_TYPE, PORTS_COUNT - 1 do
                  params[port] = voice_adjustments[port]:get_value()
          end
          if rnd(1) ~= 0 then params[PORT_BASE_FREQ] = params[PORT_BASE_FREQ] + frnd(0.1)-0.05 end
          -- if rnd(1) ~= 0 then params[PORT_FREQ_LIMIT]+=frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_FREQ_RAMP] = params[PORT_FREQ_RAMP] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_FREQ_DRAMP] = params[PORT_FREQ_DRAMP] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_DUTY] = params[PORT_DUTY] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_DUTY_RAMP] = params[PORT_DUTY_RAMP] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_VIB_STRENGTH] = params[PORT_VIB_STRENGTH] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_VIB_SPEED] = params[PORT_VIB_SPEED] + frnd(0.1)-0.05 end
          -- if rnd(1) ~= 0 then p_vib_delay+=frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_ENV_ATTACK] = params[PORT_ENV_ATTACK] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_ENV_SUSTAIN] = params[PORT_ENV_SUSTAIN] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_ENV_DECAY] = params[PORT_ENV_DECAY] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_ENV_PUNCH] = params[PORT_ENV_PUNCH] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_LPF_RESONANCE] = params[PORT_LPF_RESONANCE] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_LPF_FREQ] = params[PORT_LPF_FREQ] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_LPF_RAMP] = params[PORT_LPF_RAMP] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_HPF_FREQ] = params[PORT_HPF_FREQ] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_HPF_RAMP] = params[PORT_HPF_RAMP] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_PHA_OFFSET] = params[PORT_PHA_OFFSET] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_PHA_RAMP] = params[PORT_PHA_RAMP] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_REPEAT_SPEED] = params[PORT_REPEAT_SPEED] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_ARP_SPEED] = params[PORT_ARP_SPEED] + frnd(0.1)-0.05 end
          if rnd(1) ~= 0 then params[PORT_ARP_MOD] = params[PORT_ARP_MOD] + frnd(0.1)-0.05 end

      elseif action == ACTION.RANDOMIZE then

          local pow = math.pow
          params[PORT_BASE_FREQ]=pow(frnd(2.0)-1.0, 2.0)
          if rnd(1) ~= 0 then
                  params[PORT_BASE_FREQ]=pow(frnd(2.0)-1.0, 3.0)+0.5
          end
          params[PORT_FREQ_LIMIT]=0.0
          params[PORT_FREQ_RAMP]=pow(frnd(2.0)-1.0, 5.0)
          if params[PORT_BASE_FREQ]>0.7 and params[PORT_FREQ_RAMP]>0.2 then
                  params[PORT_FREQ_RAMP]=-params[PORT_FREQ_RAMP]
          end
          if params[PORT_BASE_FREQ]<0.2 and params[PORT_FREQ_RAMP]<-0.05 then
                  params[PORT_FREQ_RAMP]=-params[PORT_FREQ_RAMP]
          end
          params[PORT_FREQ_DRAMP]=pow(frnd(2.0)-1.0, 3.0)
          params[PORT_DUTY]=frnd(2.0)-1.0
          params[PORT_DUTY_RAMP]=pow(frnd(2.0)-1.0, 3.0)
          params[PORT_VIB_STRENGTH]=pow(frnd(2.0)-1.0, 3.0)
          params[PORT_VIB_SPEED]=frnd(2.0)-1.0
          -- p_vib_delay=frnd(2.0)-1.0
          params[PORT_ENV_ATTACK]=pow(frnd(2.0)-1.0, 3.0)
          params[PORT_ENV_SUSTAIN]=pow(frnd(2.0)-1.0, 2.0)
          params[PORT_ENV_DECAY]=frnd(2.0)-1.0
          params[PORT_ENV_PUNCH]=pow(frnd(0.8), 2.0)
          if params[PORT_ENV_ATTACK]+params[PORT_ENV_SUSTAIN]+params[PORT_ENV_DECAY]<0.2 then
                  params[PORT_ENV_SUSTAIN] = params[PORT_ENV_SUSTAIN] + 0.2+frnd(0.3)
                  params[PORT_ENV_DECAY] = params[PORT_ENV_DECAY] + 0.2+frnd(0.3)
          end
          params[PORT_LPF_RESONANCE]=frnd(2.0)-1.0
          params[PORT_LPF_FREQ]=1.0-pow(frnd(1.0), 3.0)
          params[PORT_LPF_RAMP]=pow(frnd(2.0)-1.0, 3.0)
          if params[PORT_LPF_FREQ]<0.1 and params[PORT_LPF_RAMP]<-0.05 then
                  params[PORT_LPF_RAMP]=-params[PORT_LPF_RAMP]
          end
          params[PORT_HPF_FREQ]=pow(frnd(1.0), 5.0)
          params[PORT_HPF_RAMP]=pow(frnd(2.0)-1.0, 5.0)
          params[PORT_PHA_OFFSET]=pow(frnd(2.0)-1.0, 3.0)
          params[PORT_PHA_RAMP]=pow(frnd(2.0)-1.0, 3.0)
          params[PORT_REPEAT_SPEED]=frnd(2.0)-1.0
          params[PORT_ARP_SPEED]=frnd(2.0)-1.0
          params[PORT_ARP_MOD]=frnd(2.0)-1.0
      end

      for port = PORT_ENV_ATTACK, PORTS_COUNT - 1 do
        local ph = port_hint(port)
        if params[port] < ph.LowerBound then
          params[port] = ph.LowerBound
        elseif params[port] > ph.UpperBound then
          params[port] = ph.UpperBound
        end
      end
      update_voice_widgets_from_params()
      if test_note_noteoff_key < 0 then
        send_midi(0x90, test_note_noteon_key, test_note_velocity)
        test_note_noteoff_key = test_note_noteon_key
      end
    end
end

local function on_action_change (adjustment, action)
    local value = adjustment:get_value()

    GUIDB_MESSAGE(DB_GUI, " on_action_change: action %d changed to %d\n", action, value)

    if action <= ACTION.RANDOMIZE then
      do_generator(action, value > 0)
    elseif action == ACTION.PLAY_SOUND then
      if value > 0 then
        if test_note_noteoff_key < 0 then
          send_midi(0x90, test_note_noteon_key, test_note_velocity)
          test_note_noteoff_key = test_note_noteon_key
        end
      else
        release_test_note()
      end
    elseif action == ACTION.LOAD_SOUND then
      --if(Button(490, 290, false, "LOAD SOUND", 14))
      --{
      --        char filename[256];
      --        if(FileSelectorLoad(hWndMain, filename, 1)) // WIN32
      --        {
      --                ResetParams()
      --                LoadSettings(filename)
      --                PlaySample()
      --        }
      --}
    elseif action == ACTION.SAVE_SOUND then
      --if(Button(490, 320, false, "SAVE SOUND", 15))
      --{
      --        char filename[256];
      --        if(FileSelectorSave(hWndMain, filename, 1)) // WIN32
      --                SaveSettings(filename)
      --}
    end
end

local function on_voice_control_change (adjustment, port) -- was ( GtkWidget *widget, gpointer data )
    local value = adjustment:get_value()

    GUIDB_MESSAGE(DB_GUI, " on_voice_control_change: port %d changed to %f\n", port, value)

    if port == PORT_WAVE_TYPE then
      wave_type_changed(value)
    end

    send_control(port, value)
end

local function on_about_change(adjustment)
    if adjustment:get_value() == 0 then
      about_dialog:show()
    end
end

function on_about_delete_event(dialog, event, data)
    about_dialog:hide()
    return true -- stop
end

function on_about_response(dialog, id, data)
    about_dialog:hide()
end

------------------------------------------------------------------------------
--                              instantiation                               --
------------------------------------------------------------------------------

function load_pixbuf(file)
    local path = g.build_filename(ui_directory, file)
    local pixbuf, err = gdk.pixbuf.new_from_file(path)
    if not pixbuf then
      GUIDB_MESSAGE(-1, ": failed to load image file %s: %s", file, err)
      os.exit(1)
    end
    return pixbuf
end

local function place_control(port, pixbuf, cp, x, y)
    local adjustment = gtk.adjustment.new (0.0, port_hint(port).LowerBound, 1.0, 1.0 / 1000.0, 0.1, 0)
    local widget = gtk.control.new (adjustment, pixbuf,
                                    100, 8,  -- size
                                    99,      -- range
                                    99, 0,   -- origin_x, origin_y
                                    -1, 0,   -- stride_x, stride_y
                                    gtk.CONTROL_HORIZONTAL)
    widget:set_prelight_offsets(0, 8)
    widget:set("insensitive-y", 16)
    voice_adjustments[port] = adjustment
    cp:put(widget, x, y)
    voice_handler_ids[port] = adjustment:signal_connect('value-changed', on_voice_control_change, port)

    if port == PORT_DUTY      then voice_widget_duty      = widget end
    if port == PORT_DUTY_RAMP then voice_widget_duty_ramp = widget end
end

function create_main_window(tag)
    main_window = gtk.window.new()
    main_window:set("title", tag)
    main_window:signal_connect('destroy', gtk.main_quit)
    main_window:signal_connect("delete-event", function() gtk.main_quit() return true end)

    -- load our pixbufs
    for id, file in pairs({ background = "panel.png",
                            buttons = "buttons.png",
                            slider = "slider.png",
                            about = "about.png",
                            about_icon = "about_icon.png",
                          }) do
      pixbuf[id] = load_pixbuf(file)
    end

    -- create the GtkControlPanel
    local cp = gtk.control_panel.new(pixbuf.background, -1, -1)  -- use the pixbuf's size
    cp:set("width-request",  pixbuf.background:get("width"),
           "height-request", pixbuf.background:get("height"))
    main_window:add(cp)

    -- now, the GtkControls
    local adjustment, widget

    -- add buttons
    adjustment = gtk.adjustment.new (0.0, 0.0, 3.0, 1.0, 1.0, 0)
    voice_adjustments[PORT_WAVE_TYPE] = adjustment
    voice_handler_ids[PORT_WAVE_TYPE] = adjustment:signal_connect('value-changed',
                                            on_voice_control_change, PORT_WAVE_TYPE)
    for i = 0, 3 do
      widget = gtk.control.new(adjustment, pixbuf.buttons,
                               100, 17,    -- size
                               1,          -- range
                               0, i * 17,  -- origin_x, origin_y
                               200, 0,     -- stride_x, stride_y
                               gtk.CONTROL_RADIO)
      widget:set("radio-value", i)
      widget:set_prelight_offsets(100, 0)
      cp:put(widget, 130 + 120 * i, 30)
    end

    for i = 1, ACTION.COUNT do
      adjustment = gtk.adjustment.new (0.0, 0.0, 1.0, 1.0, 1.0, 0)
      action_adjustments[i] = adjustment
      adjustment:signal_connect('value-changed', on_action_change, i)
      widget = gtk.control.new (adjustment, pixbuf.buttons,
                                100, 17,          -- size
                                1,                -- range
                                0, (i + 3) * 17,  -- origin_x, origin_y
                                200, 0,           -- stride_x, stride_y
                                gtk.CONTROL_BUTTON)
      widget:set_prelight_offsets(100, 0)
      if i < ACTION.MUTATE then
        cp:put(widget, 5, 35 + (i - 1) * 30)
      elseif i == ACTION.MUTATE then
        cp:put(widget, 5, 382)
      elseif i == ACTION.RANDOMIZE then
        cp:put(widget, 5, 412)
      elseif i == ACTION.PLAY_SOUND then
        cp:put(widget, 490, 200)
      elseif i == ACTION.LOAD_SOUND then
        cp:put(widget, 490, 290)
      elseif i == ACTION.SAVE_SOUND then
        cp:put(widget, 490, 320)
      end
    end

    -- add sliders
    place_control(PORT_SOUND_VOL,     pixbuf.slider, cp, 490, 181)
    -- PORT_WAVE_TYPE above
    place_control(PORT_ENV_ATTACK,    pixbuf.slider, cp, 350,  73)
    place_control(PORT_ENV_SUSTAIN,   pixbuf.slider, cp, 350,  91)
    place_control(PORT_ENV_PUNCH,     pixbuf.slider, cp, 350, 109)
    place_control(PORT_ENV_DECAY,     pixbuf.slider, cp, 350, 127)
    place_control(PORT_BASE_FREQ,     pixbuf.slider, cp, 350, 145)
    place_control(PORT_FREQ_LIMIT,    pixbuf.slider, cp, 350, 163)
    place_control(PORT_FREQ_RAMP,     pixbuf.slider, cp, 350, 181)
    place_control(PORT_FREQ_DRAMP,    pixbuf.slider, cp, 350, 199)
    place_control(PORT_VIB_STRENGTH,  pixbuf.slider, cp, 350, 217)
    place_control(PORT_VIB_SPEED,     pixbuf.slider, cp, 350, 235)
    place_control(PORT_ARP_MOD,       pixbuf.slider, cp, 350, 253)
    place_control(PORT_ARP_SPEED,     pixbuf.slider, cp, 350, 271)
    place_control(PORT_DUTY,          pixbuf.slider, cp, 350, 289)
    place_control(PORT_DUTY_RAMP,     pixbuf.slider, cp, 350, 307)
    place_control(PORT_REPEAT_SPEED,  pixbuf.slider, cp, 350, 325)
    place_control(PORT_PHA_OFFSET,    pixbuf.slider, cp, 350, 343)
    place_control(PORT_PHA_RAMP,      pixbuf.slider, cp, 350, 361)
    place_control(PORT_LPF_FREQ,      pixbuf.slider, cp, 350, 379)
    place_control(PORT_LPF_RAMP,      pixbuf.slider, cp, 350, 397)
    place_control(PORT_LPF_RESONANCE, pixbuf.slider, cp, 350, 415)
    place_control(PORT_HPF_FREQ,      pixbuf.slider, cp, 350, 433)
    place_control(PORT_HPF_RAMP,      pixbuf.slider, cp, 350, 451)

    -- add about 'button'
    adjustment = gtk.adjustment.new (0.0, 0.0, 1.0, 1.0, 1.0, 0)
    adjustment:signal_connect('value-changed', on_about_change)
    widget = gtk.control.new (adjustment, pixbuf.about,
                              129, 49, -- size
                              1,       -- range
                              0, 0,    -- origin
                              0, 98,   -- stride
                              gtk.CONTROL_BUTTON)
    widget:set_prelight_offsets(0, 49)
    cp:put(widget, 489, 408)

    cp:show_all()
end

function create_about_dialog(tag)
    about_dialog = gtk.about_dialog.new()
    about_dialog:set("title", "About sfxr-dssi")
    about_dialog:set("logo", pixbuf.about_icon)
    about_dialog:set("program-name", "sfxr-dssi")
    about_dialog:set("version", plugin_version)
--n     about_dialog:set("authors", { "Sean Bolton" }) -- !FIX!
--n     about_dialog:set("artists", { "Thanks to the following ....",
--n                                   "for their Creative Commons licensed work:" })
--n     about_dialog:set("comments", "Comments!")
--n     about_dialog:set("copyright", "Copyright!")
--n     about_dialog:set("license", "Nigiri's source code is copyright (C) 2011 by Sean Bolton. !FIX!")
--n     about_dialog:set("website", "http://smbolton.com")
--n     -- "documenters"              GStrv*                : Read / Write
--n     -- "translator-credits"       gchar*                : Read / Write
--n     -- "version"                  gchar*                : Read / Write
--n     -- "website-label"            gchar*                : Read / Write
--n     -- "wrap-license"             gboolean              : Read / Write

    about_dialog:signal_connect("delete-event", on_about_delete_event)
    about_dialog:signal_connect("response", on_about_response)
end

function create_windows(instance_tag)
    -- build a nice identifier string for the window titles
    local tag
    if instance_tag == "" then
      tag = "sfxr-dssi"
    elseif string.find(instance_tag, "sfxr-DSSI", 1, true) or
           string.find(instance_tag, "sfxr-dssi", 1, true) then
      if string.len(instance_tag) > 50 then
        tag = "..." .. string.sub(instance_tag, -47) -- hope the unique info is at the end
      else
        tag = instance_tag
      end
    else
      if string.len(instance_tag) > 40 then
        tag = "sfxr-dssi ..." .. string.sub(instance_tag, -37) -- hope the unique info is at the end
      else
        tag = "sfxr-dssi " .. instance_tag
      end
    end

    create_main_window(tag)
    create_about_dialog(tag)
end

------------------------------------------------------------------------------
--                                  main                                    --
------------------------------------------------------------------------------

GUIDB_MESSAGE(DB_MAIN, ": Lua side starting, OSC instance path is %s\n", osc_instance_path)

create_windows(user_friendly_id)

osc_server_start(osc_receipt_callback)

-- schedule our update request
g.timeout_add(50,
    function()
      if not gui_test_mode then
        -- send our update request
        GUIDB_MESSAGE(DB_OSC, " update request timeout callback: sending update request\n")
        --lo_send(osc_host_address, osc_update_path, "s", osc_self_url)
        send_update_request()
      else
        main_window:show()
      end
      return false  -- don't need to do this again
    end, nil
)

gtk.main()

GUIDB_MESSAGE(DB_MAIN, ": yep, we got to the Lua cleanup!\n")

-- turn off test note if it is still on
release_test_note()

-- say bye-bye
if not host_requested_quit then
  send_exiting()
end

-- disable incoming OSC message callback
osc_server_disconnect()

