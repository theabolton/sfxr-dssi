/* sfxr-dssi DSSI software synthesizer plugin
 *
 * Copyright (C) 2011 Sean Bolton and others.
 *
 * Portions of this file come from the SDL version of sfxr,
 * copyright (c) 2007 Tomas Pettersson.
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

#ifndef _PORTS_H
#define _PORTS_H

#include <ladspa.h>
#include <dssi.h>

#define PORT_OUTPUT         0
#define PORT_SOUND_VOL      1
#define PORT_WAVE_TYPE      2
#define PORT_ENV_ATTACK     3
#define PORT_ENV_SUSTAIN    4
#define PORT_ENV_PUNCH      5
#define PORT_ENV_DECAY      6
#define PORT_BASE_FREQ      7
#define PORT_FREQ_LIMIT     8
#define PORT_FREQ_RAMP      9
#define PORT_FREQ_DRAMP    10
#define PORT_VIB_STRENGTH  11
#define PORT_VIB_SPEED     12
#define PORT_ARP_MOD       13
#define PORT_ARP_SPEED     14
#define PORT_DUTY          15
#define PORT_DUTY_RAMP     16
#define PORT_REPEAT_SPEED  17
#define PORT_PHA_OFFSET    18
#define PORT_PHA_RAMP      19
#define PORT_LPF_FREQ      20
#define PORT_LPF_RAMP      21
#define PORT_LPF_RESONANCE 22
#define PORT_HPF_FREQ      23
#define PORT_HPF_RAMP      24
/* #define PORT_VIB_DELAY   - not used */
/* #define PORT_FILTER_ON   - not used */

#define PORTS_COUNT  25

/* in sfxr_ports.c: */
extern const LADSPA_PortDescriptor port_descriptors[PORTS_COUNT];
extern const char * const          port_names[PORTS_COUNT];
extern const LADSPA_PortRangeHint  port_hints[PORTS_COUNT];

#endif /* _PORTS_H */

