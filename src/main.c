/* sfxr-dssi DSSI software synthesizer plugin
 *
 * Almost the entire contents of this file, with only slight
 * modifications, comes from the SDL version of sfxr, which is:
 *
 * Copyright (c) 2007 Tomas Pettersson
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

#include <stdlib.h>
#include <math.h>

#include "sfxr.h"
#include "main.h"

#define false (0)
#define true  (1)
typedef int bool;

#define rnd(n) (rand()%(n+1))

float frnd(float range)
{
	return (float)rnd(10000)/10000.0f*range;
}

float master_vol=0.05f;

#define p(x) (*(si->x))
#define ip(x) lrintf(*(si->x))

void ResetSample(sfxr_instance_t *si, bool restart)
{
        int i;
	double freqtmp;
	double freqscale = pow(2.0, (double)(si->key - 60) / 12.0);

	if(!restart)
		si->phase=0;
	freqtmp = (p(p_base_freq)*p(p_base_freq)+0.001) * freqscale;
	if (freqtmp < 0.0) freqtmp = 0.0;
	else if (freqtmp > 1.0) freqtmp = 1.0;
	si->fperiod=100.0/freqtmp;
	si->period=(int)si->fperiod;
	freqtmp = (p(p_freq_limit)*p(p_freq_limit)+0.001) * freqscale;
	if (freqtmp < 0.0) freqtmp = 0.0;
	else if (freqtmp > 1.0) freqtmp = 1.0;
	si->fmaxperiod=100.0/freqtmp;
	si->fslide=1.0-pow((double)p(p_freq_ramp), 3.0)*0.01;
	si->fdslide=-pow((double)p(p_freq_dramp), 3.0)*0.000001;
	si->square_duty=0.5f-p(p_duty)*0.5f;
 	si->square_slide=-p(p_duty_ramp)*0.00005f;
	if(p(p_arp_mod)>=0.0f)
		si->arp_mod=1.0-pow((double)p(p_arp_mod), 2.0)*0.9;
	else
		si->arp_mod=1.0+pow((double)p(p_arp_mod), 2.0)*10.0;
	si->arp_time=0;
	si->arp_limit=(int)(pow(1.0f-p(p_arp_speed), 2.0f)*20000+32);
	if(p(p_arp_speed)==1.0f)
		si->arp_limit=0;
	if(!restart)
	{
		// reset filter
		si->fltp=0.0f;
		si->fltdp=0.0f;
		si->fltw=pow(p(p_lpf_freq), 3.0f)*0.1f;
		si->fltw_d=1.0f+p(p_lpf_ramp)*0.0001f;
		si->fltdmp=5.0f/(1.0f+pow(p(p_lpf_resonance), 2.0f)*20.0f)*(0.01f+si->fltw);
		if(si->fltdmp>0.8f) si->fltdmp=0.8f;
		si->fltphp=0.0f;
		si->flthp=pow(p(p_hpf_freq), 2.0f)*0.1f;
		si->flthp_d=1.0+p(p_hpf_ramp)*0.0003f;
		// reset vibrato
		si->vib_phase=0.0f;
		si->vib_speed=pow(p(p_vib_speed), 2.0f)*0.01f;
		si->vib_amp=p(p_vib_strength)*0.5f;
		// reset envelope
		si->env_vol=0.0f;
		si->env_stage=0;
		si->env_time=0;
		si->env_length[0]=(int)(p(p_env_attack)*p(p_env_attack)*100000.0f);
		si->env_length[1]=(int)(p(p_env_sustain)*p(p_env_sustain)*100000.0f);
		si->env_length[2]=(int)(p(p_env_decay)*p(p_env_decay)*100000.0f);

		si->fphase=pow(p(p_pha_offset), 2.0f)*1020.0f;
 		if(p(p_pha_offset)<0.0f) si->fphase=-si->fphase;
		si->fdphase=pow(p(p_pha_ramp), 2.0f)*1.0f;
		if(p(p_pha_ramp)<0.0f) si->fdphase=-si->fdphase;
		si->iphase=abs((int)si->fphase);
		si->ipp=0;
		for(i=0;i<1024;i++)
			si->phaser_buffer[i]=0.0f;

		for(i=0;i<32;i++)
			si->noise_buffer[i]=frnd(2.0f)-1.0f;

		si->rep_time=0;
		si->rep_limit=(int)(pow(1.0f-p(p_repeat_speed), 2.0f)*20000+32);
		if(p(p_repeat_speed)==0.0f)
			si->rep_limit=0;
	}
}

void PlaySample(sfxr_instance_t *si, unsigned char key)
{
	si->key = key;
	ResetSample(si, false);
	si->playing_sample=true;
}

void SynthSample(sfxr_instance_t *si, int length, float* buffer)
{
        int i, j, k;

	for(i=0;i<length;i++)
	{
		if(!si->playing_sample) {
			*buffer++=0.0f;
			continue;
                }

		si->rep_time++;
		if(si->rep_limit!=0 && si->rep_time>=si->rep_limit)
		{
			si->rep_time=0;
			ResetSample(si, true);
		}

		// frequency envelopes/arpeggios
		si->arp_time++;
		if(si->arp_limit!=0 && si->arp_time>=si->arp_limit)
		{
			si->arp_limit=0;
			si->fperiod*=si->arp_mod;
		}
		si->fslide+=si->fdslide;
		si->fperiod*=si->fslide;
		if(si->fperiod>si->fmaxperiod)
		{
			si->fperiod=si->fmaxperiod;
			if(p(p_freq_limit)>0.0f)
				si->playing_sample=false;
		}
		float rfperiod=si->fperiod;
		if(si->vib_amp>0.0f)
		{
			si->vib_phase+=si->vib_speed;
			rfperiod=si->fperiod*(1.0+sin(si->vib_phase)*si->vib_amp);
		}
		si->period=(int)rfperiod;
		if(si->period<8) si->period=8;
		si->square_duty+=si->square_slide;
		if(si->square_duty<0.0f) si->square_duty=0.0f;
		if(si->square_duty>0.5f) si->square_duty=0.5f;		
		// volume envelope
		si->env_time++;
		if(si->env_time>si->env_length[si->env_stage])
		{
			si->env_time=0;
			si->env_stage++;
			if(si->env_stage==3)
				si->playing_sample=false;
		}
		if(si->env_stage==0)
			si->env_vol=(float)si->env_time/si->env_length[0];
		if(si->env_stage==1)
			si->env_vol=1.0f+pow(1.0f-(float)si->env_time/si->env_length[1], 1.0f)*2.0f*p(p_env_punch);
		if(si->env_stage==2)
			si->env_vol=1.0f-(float)si->env_time/si->env_length[2];

		// phaser step
		si->fphase+=si->fdphase;
		si->iphase=abs((int)si->fphase);
		if(si->iphase>1023) si->iphase=1023;

		if(si->flthp_d!=0.0f)
		{
			si->flthp*=si->flthp_d;
			if(si->flthp<0.00001f) si->flthp=0.00001f;
			if(si->flthp>0.1f) si->flthp=0.1f;
		}

		float ssample=0.0f;
		for(j=0;j<8;j++) // 8x supersampling
		{
			float sample=0.0f;
			si->phase++;
			if(si->phase>=si->period)
			{
//				si->phase=0;
				si->phase%=si->period;
				if(ip(wave_type)==3)
					for(k=0;k<32;k++)
						si->noise_buffer[k]=frnd(2.0f)-1.0f;
			}
			// base waveform
			float fp=(float)si->phase/si->period;
			switch(ip(wave_type))
			{
			case 0: // square
				if(fp<si->square_duty)
					sample=0.5f;
				else
					sample=-0.5f;
				break;
			case 1: // sawtooth
				sample=1.0f-fp*2;
				break;
			case 2: // sine
				sample=(float)sin(fp*2*M_PI);
				break;
			case 3: // noise
				sample=si->noise_buffer[si->phase*32/si->period];
				break;
			}
			// lp filter
			float pp=si->fltp;
			si->fltw*=si->fltw_d;
			if(si->fltw<0.0f) si->fltw=0.0f;
			if(si->fltw>0.1f) si->fltw=0.1f;
			if(p(p_lpf_freq)!=1.0f)
			{
				si->fltdp+=(sample-si->fltp)*si->fltw;
				si->fltdp-=si->fltdp*si->fltdmp;
			}
			else
			{
				si->fltp=sample;
				si->fltdp=0.0f;
			}
			si->fltp+=si->fltdp;
			// hp filter
			si->fltphp+=si->fltp-pp;
			si->fltphp-=si->fltphp*si->flthp;
			sample=si->fltphp;
			// phaser
			si->phaser_buffer[si->ipp&1023]=sample;
			sample+=si->phaser_buffer[(si->ipp-si->iphase+1024)&1023];
			si->ipp=(si->ipp+1)&1023;
			// final accumulation and envelope application
			ssample+=sample*si->env_vol;
		}
		ssample=ssample/8*master_vol;

		ssample*=2.0f*p(sound_vol);

		if(ssample>1.0f) ssample=1.0f;
		if(ssample<-1.0f) ssample=-1.0f;
		*buffer++=ssample;
	}
}

