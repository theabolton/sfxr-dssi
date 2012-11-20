/* sfxr-dssi DSSI software synthesizer plugin
 *
 * Copyright (C) 2011 Sean Bolton and others.
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

#define _BSD_SOURCE    1
#define _SVID_SOURCE   1
#define _ISOC99_SOURCE 1

#include <stdlib.h>

#include <ladspa.h>
#include <dssi.h>

#include "sfxr.h"
#include "ports.h"
#include "main.h"

const char *plugin_name = "sfxr-dssi.so";

static void sfxr_run_synth(LADSPA_Handle handle, unsigned long sample_count,
                           snd_seq_event_t *events, unsigned long event_count); /* forward */

/* ---- LADSPA interface ---- */

/*
 * sfxr_instantiate
 *
 * implements LADSPA (*instantiate)()
 */
static LADSPA_Handle
sfxr_instantiate(const LADSPA_Descriptor *descriptor, unsigned long sample_rate)
{
    sfxr_instance_t *instance;

    instance = (sfxr_instance_t *)malloc(sizeof(sfxr_instance_t));
    if (instance == NULL)
        return NULL;

    // instance->sample_rate = (float)sample_rate;  !FIX!

    return (LADSPA_Handle)instance;
}

/*
 * sfxr_connect_port
 *
 * implements LADSPA (*connect_port)()
 */
static void
sfxr_connect_port(LADSPA_Handle handle, unsigned long port, LADSPA_Data *data)
{
    sfxr_instance_t *instance = (sfxr_instance_t *)handle;

    switch (port) {
      case PORT_OUTPUT:         instance->output            = data;  break;
      case PORT_SOUND_VOL:      instance->sound_vol         = data;  break;
      case PORT_WAVE_TYPE:      instance->wave_type         = data;  break;
      case PORT_ENV_ATTACK:     instance->p_env_attack      = data;  break;
      case PORT_ENV_SUSTAIN:    instance->p_env_sustain     = data;  break;
      case PORT_ENV_PUNCH:      instance->p_env_punch       = data;  break;
      case PORT_ENV_DECAY:      instance->p_env_decay       = data;  break;
      case PORT_BASE_FREQ:      instance->p_base_freq       = data;  break;
      case PORT_FREQ_LIMIT:     instance->p_freq_limit      = data;  break;
      case PORT_FREQ_RAMP:      instance->p_freq_ramp       = data;  break;
      case PORT_FREQ_DRAMP:     instance->p_freq_dramp      = data;  break;
      case PORT_VIB_STRENGTH:   instance->p_vib_strength    = data;  break;
      case PORT_VIB_SPEED:      instance->p_vib_speed       = data;  break;
      case PORT_ARP_MOD:        instance->p_arp_mod         = data;  break;
      case PORT_ARP_SPEED:      instance->p_arp_speed       = data;  break;
      case PORT_DUTY:           instance->p_duty            = data;  break;
      case PORT_DUTY_RAMP:      instance->p_duty_ramp       = data;  break;
      case PORT_REPEAT_SPEED:   instance->p_repeat_speed    = data;  break;
      case PORT_PHA_OFFSET:     instance->p_pha_offset      = data;  break;
      case PORT_PHA_RAMP:       instance->p_pha_ramp        = data;  break;
      case PORT_LPF_FREQ:       instance->p_lpf_freq        = data;  break;
      case PORT_LPF_RAMP:       instance->p_lpf_ramp        = data;  break;
      case PORT_LPF_RESONANCE:  instance->p_lpf_resonance   = data;  break;
      case PORT_HPF_FREQ:       instance->p_hpf_freq        = data;  break;
      case PORT_HPF_RAMP:       instance->p_hpf_ramp        = data;  break;

      default:
        break;
    }
}

// optional:
//  void (*activate)(LADSPA_Handle Instance);

/*
 * sfxr_ladspa_run
 *
 * implements LADSPA (*run)() by calling run_synth() with no events
 */
static void
sfxr_ladspa_run(LADSPA_Handle instance, unsigned long sample_count)
{
    sfxr_run_synth(instance, sample_count, NULL, 0);
    return;
}

// optional:
//  void (*run_adding)(LADSPA_Handle Instance,
//                     unsigned long SampleCount);
//  void (*set_run_adding_gain)(LADSPA_Handle Instance,
//                              LADSPA_Data   Gain);
//  void (*deactivate)(LADSPA_Handle Instance);

/*
 * sfxr_cleanup
 *
 * implements LADSPA (*cleanup)()
 */
static void
sfxr_cleanup(LADSPA_Handle handle)
{
    sfxr_instance_t *instance = (sfxr_instance_t *)handle;

    free(instance);
}

/* ---- DSSI interface ---- */

// optional:
//  char *(*configure)(LADSPA_Handle Instance,
//                     const char *Key,
//                     const char *Value);
//  const DSSI_Program_Descriptor *(*get_program)(LADSPA_Handle Instance,
//                                                unsigned long Index);
//  void (*select_program)(LADSPA_Handle Instance,
//                         unsigned long Bank,
//                         unsigned long Program);
//  int (*get_midi_controller_for_port)(LADSPA_Handle Instance,
//                                      unsigned long Port);

/*
 * sfxr_handle_event
 */
static inline void
sfxr_handle_event(sfxr_instance_t *instance, snd_seq_event_t *event)
{
    DEBUG_MESSAGE(DB_DSSI, " sfxr_handle_event called with event type %d\n", event->type);

    if (event->type == SND_SEQ_EVENT_NOTEON)
        PlaySample(instance, event->data.note.note);
}

/*
 * sfxr_render
 */
static void
sfxr_render(sfxr_instance_t *instance, LADSPA_Data *out, unsigned long sample_count)
{
    SynthSample(instance, (int)sample_count, out);
}

/*
 * sfxr_run_synth
 *
 * implements DSSI (*run_synth)()
 */
static void
sfxr_run_synth(LADSPA_Handle handle, unsigned long sample_count,
               snd_seq_event_t *events, unsigned long event_count)
{
    sfxr_instance_t *instance = (sfxr_instance_t *)handle;
    unsigned long samples_done = 0;
    unsigned long event_index = 0;
    unsigned long burst_size;

    while (samples_done < sample_count) {

        /* process any ready events */
        while (event_index < event_count
               && samples_done == events[event_index].time.tick) {
            sfxr_handle_event(instance, &events[event_index]);
            event_index++;
        }

        /* calculate the sample count (burst_size) for the next
         * sfxr_client_voice_render() call to be the smallest of:
         * - the number of samples left in this run
         * - the number of samples until the next event is ready
         */
        burst_size = sample_count - samples_done;
        if (event_index < event_count
            && events[event_index].time.tick - samples_done < burst_size) {
            /* reduce burst size to end when next event is ready */
            burst_size = events[event_index].time.tick - samples_done;
        }

        /* render the burst */
        sfxr_render(instance, instance->output + samples_done, burst_size);
        samples_done += burst_size;
    }
#if defined(DSSP_DEBUG) && (DSSP_DEBUG & DB_AUDIO)
*instance->output += 0.10f; /* add a 'buzz' to output so there's something audible even when quiescent */
#endif /* defined(DSSP_DEBUG) && (DSSP_DEBUG & DB_AUDIO) */
}

// optional:
//    void (*run_synth_adding)(LADSPA_Handle    Instance,
//                             unsigned long    SampleCount,
//                             snd_seq_event_t *Events,
//                             unsigned long    EventCount);
//    void (*run_multiple_synths)(unsigned long     InstanceCount,
//                                LADSPA_Handle    *Instances,
//                                unsigned long     SampleCount,
//                                snd_seq_event_t **Events,
//                                unsigned long    *EventCounts);
//    void (*run_multiple_synths_adding)(unsigned long     InstanceCount,
//                                       LADSPA_Handle   **Instances,
//                                       unsigned long     SampleCount,
//                                       snd_seq_event_t **Events,
//                                       unsigned long    *EventCounts);

/* ---- export ---- */

static const LADSPA_Descriptor sfxr_ladspa_descriptor =
{
    0,
    "sfxr-dssi",
    0,
    "sfxr-dssi " PACKAGE_VERSION " - noisemaking DSSI plugin",
    "Tomas Pettersson and Sean Bolton",
    "Copyright (c) 2011, licensed under the MIT license",
    PORTS_COUNT,
    port_descriptors,
    port_names,
    port_hints,
    0,
    sfxr_instantiate,
    sfxr_connect_port,
    NULL, /* activate */
    sfxr_ladspa_run,
    NULL, /* run_adding */
    NULL, /* set_run_adding_gain */
    NULL, /* deactivate */
    sfxr_cleanup
};

static const DSSI_Descriptor sfxr_dssi_descriptor =
{
    1,
    &sfxr_ladspa_descriptor,
    NULL, /* configure */
    NULL, /* get_program */
    NULL, /* select_program */
    NULL, /* get_midi_controller_for_port */
    sfxr_run_synth,
    NULL, /* run_synth_adding */
    NULL, /* run_multiple_synths */
    NULL  /* run_multiple_synths_adding */
};

const DSSI_Descriptor *
dssi_descriptor(unsigned long i)
{
    if (i)
        return NULL;
    else
        return &sfxr_dssi_descriptor;
}

