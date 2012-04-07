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

#ifndef _SFXR_H
#define _SFXR_H

#include <ladspa.h>
#include <dssi.h>

#include "debug.h"
#include "ports.h"

extern const char *plugin_name;

typedef struct _sfxr_instance_t   sfxr_instance_t;

/*
 * sfxr_instance_t
 */
struct _sfxr_instance_t
{
    /* LADSPA ports */
    LADSPA_Data *output;
    LADSPA_Data *sound_vol;
    LADSPA_Data *wave_type;  /* int */
    LADSPA_Data *p_env_attack;
    LADSPA_Data *p_env_sustain;
    LADSPA_Data *p_env_punch;
    LADSPA_Data *p_env_decay;
    LADSPA_Data *p_base_freq;
    LADSPA_Data *p_freq_limit;
    LADSPA_Data *p_freq_ramp;
    LADSPA_Data *p_freq_dramp;
    LADSPA_Data *p_vib_strength;
    LADSPA_Data *p_vib_speed;
    LADSPA_Data *p_arp_mod;
    LADSPA_Data *p_arp_speed;
    LADSPA_Data *p_duty;
    LADSPA_Data *p_duty_ramp;
    LADSPA_Data *p_repeat_speed;
    LADSPA_Data *p_pha_offset;
    LADSPA_Data *p_pha_ramp;
    LADSPA_Data *p_lpf_freq;
    LADSPA_Data *p_lpf_ramp;
    LADSPA_Data *p_lpf_resonance;
    LADSPA_Data *p_hpf_freq;
    LADSPA_Data *p_hpf_ramp;
    // bool filter_on;     - not used
    // float p_vib_delay;  - not used

    int playing_sample;
    int phase;
    double fperiod;
    double fmaxperiod;
    double fslide;
    double fdslide;
    int period;
    float square_duty;
    float square_slide;
    int env_stage;
    int env_time;
    int env_length[3];
    float env_vol;
    float fphase;
    float fdphase;
    int iphase;
    float phaser_buffer[1024];
    int ipp;
    float noise_buffer[32];
    float fltp;
    float fltdp;
    float fltw;
    float fltw_d;
    float fltdmp;
    float fltphp;
    float flthp;
    float flthp_d;
    float vib_phase;
    float vib_speed;
    float vib_amp;
    int rep_time;
    int rep_limit;
    int arp_time;
    int arp_limit;
    double arp_mod;
};

#endif /* _SFXR_H */

