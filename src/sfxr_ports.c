/* sfxr-dssi DSSI software synthesizer plugin
 *
 * Copyright (C) 2010 Sean Bolton.
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

#include <ladspa.h>

#include "ports.h"
#include "sfxr.h"

const LADSPA_PortDescriptor port_descriptors[PORTS_COUNT] =
{
     /* PORT_OUTPUT        */  LADSPA_PORT_OUTPUT | LADSPA_PORT_AUDIO,
     /* PORT_SOUND_VOL     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_WAVE_TYPE     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_ENV_ATTACK    */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_ENV_SUSTAIN   */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_ENV_PUNCH     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_ENV_DECAY     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_BASE_FREQ     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_FREQ_LIMIT    */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_FREQ_RAMP     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_FREQ_DRAMP    */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_VIB_STRENGTH  */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_VIB_SPEED     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_ARP_MOD       */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_ARP_SPEED     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_DUTY          */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_DUTY_RAMP     */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_REPEAT_SPEED  */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_PHA_OFFSET    */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_PHA_RAMP      */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_LPF_FREQ      */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_LPF_RAMP      */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_LPF_RESONANCE */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_HPF_FREQ      */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
     /* PORT_HPF_RAMP      */  LADSPA_PORT_INPUT  | LADSPA_PORT_CONTROL,
};

const char * const port_names[PORTS_COUNT] =
{
     /* PORT_OUTPUT        */  "Output",
     /* PORT_SOUND_VOL     */  "Volume",
     /* PORT_WAVE_TYPE     */  "Wave Type",
     /* PORT_ENV_ATTACK    */  "Attack Time",
     /* PORT_ENV_SUSTAIN   */  "Sustain Time",
     /* PORT_ENV_PUNCH     */  "Sustain Punch",
     /* PORT_ENV_DECAY     */  "Decay Time",
     /* PORT_BASE_FREQ     */  "Start Frequency",
     /* PORT_FREQ_LIMIT    */  "Min Frequency",
     /* PORT_FREQ_RAMP     */  "Slide",
     /* PORT_FREQ_DRAMP    */  "Delta Slide",
     /* PORT_VIB_STRENGTH  */  "Vibrato Depth",
     /* PORT_VIB_SPEED     */  "Vibrato Speed",
     /* PORT_ARP_MOD       */  "Change Amount",
     /* PORT_ARP_SPEED     */  "Change Speed",
     /* PORT_DUTY          */  "Square Duty",
     /* PORT_DUTY_RAMP     */  "Duty Sweep",
     /* PORT_REPEAT_SPEED  */  "Repeat Speed",
     /* PORT_PHA_OFFSET    */  "Phaser Offset",
     /* PORT_PHA_RAMP      */  "Phaser Sweep",
     /* PORT_LPF_FREQ      */  "LP Filter Cutoff",
     /* PORT_LPF_RAMP      */  "LP Filter Cutoff Sweep",
     /* PORT_LPF_RESONANCE */  "LP Filter Resonance",
     /* PORT_HPF_FREQ      */  "HP Filter Cutoff",
     /* PORT_HPF_RAMP      */  "HP Filter Cutoff Sweep"
};

const LADSPA_PortRangeHint port_hints[PORTS_COUNT] =
{
#define HD_MIN     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MINIMUM)
#define HD_LOW     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_LOW)
#define HD_MID     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MIDDLE)
#define HD_MAX     (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_DEFAULT_MAXIMUM)
#define HD_INT_MIN  (LADSPA_HINT_BOUNDED_BELOW | LADSPA_HINT_BOUNDED_ABOVE | LADSPA_HINT_INTEGER | LADSPA_HINT_DEFAULT_MINIMUM)
     /* PORT_OUTPUT        */  { 0, 0, 0 },
     /* PORT_SOUND_VOL     */  { HD_MID,         0.0f, 1.0f },
     /* PORT_WAVE_TYPE     */  { HD_INT_MIN,     0.0f, 3.0f },
     /* PORT_ENV_ATTACK    */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_ENV_SUSTAIN   */  { HD_LOW,         0.0f, 1.0f },
     /* PORT_ENV_PUNCH     */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_ENV_DECAY     */  { HD_MID,         0.0f, 1.0f },
     /* PORT_BASE_FREQ     */  { HD_LOW,         0.0f, 1.0f },
     /* PORT_FREQ_LIMIT    */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_FREQ_RAMP     */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_FREQ_DRAMP    */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_VIB_STRENGTH  */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_VIB_SPEED     */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_ARP_MOD       */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_ARP_SPEED     */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_DUTY          */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_DUTY_RAMP     */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_REPEAT_SPEED  */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_PHA_OFFSET    */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_PHA_RAMP      */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_LPF_FREQ      */  { HD_MAX,         0.0f, 1.0f },
     /* PORT_LPF_RAMP      */  { HD_MID,        -1.0f, 1.0f },
     /* PORT_LPF_RESONANCE */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_HPF_FREQ      */  { HD_MIN,         0.0f, 1.0f },
     /* PORT_HPF_RAMP      */  { HD_MID,        -1.0f, 1.0f }
#undef HD_MIN
#undef HD_LOW
#undef HD_MID
#undef HD_MAX
#undef HD_INT_MIN
};

