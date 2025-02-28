/**    
    @file reverb.h
    @brief An implementation of Jon Dattoro's reverb algorithm.
    See J. Dattoro, "Effect Design: Part 1: Reverberator and Other Filters" J. Audio Eng. Soc. 45:9 September 1997
    
    @author John Williamson
    
    Copyright (c) 2011-2025 All rights reserved.
    Licensed under the MIT License, 2025.
    
*/              

#ifndef __REVERB_H__
#define __REVERB_H__
#define INIT_DELAY_MAX 1024


typedef struct DelayLine
{
    float *samples;
    int n_samples;
    int max_n_samples;    
    
    int read_head;
    int write_head;    
        
} DelayLine;

DelayLine *create_delay();
void destroy_delay(DelayLine *delay);
float delay(DelayLine *delay, float sample);
void delay_in(DelayLine *delay, float sample);
float delay_out(DelayLine *delay);
void set_delay(DelayLine *delay, int delay_length);
float get_delay(DelayLine *delay, int index);
void expand_delay(DelayLine *delay, int samples);

#define MODDELAY_INTERPOLATION_LINEAR 0
#define MODDELAY_INTERPOLATION_ALLPASS 1

typedef struct ModDelayLine
{
    float *samples;
    int n_samples;
    int max_n_samples;    
    int read_offset;
    int read_head;
    int write_head;
    
    // for modulation
    float read_fraction;
    int excursion;
    float phase;
    float modulation_frequency;
    float modulation_extent;    
    
    int interpolation_mode;
    
    int modulated;
    float allpass_a;
    int sample_rate;
} ModDelayLine;

ModDelayLine *create_mdelay(void);


void set_interpolation_mode_mdelay(ModDelayLine *delay, int mode);
void set_modulation_mdelay(ModDelayLine *delay,  float modulation_extent, float modulation_frequency);
void destroy_mdelay(ModDelayLine *delay);
float mdelay(ModDelayLine *delay, float sample);
void mdelay_in(ModDelayLine *delay, float sample);
float mdelay_out(ModDelayLine *delay);
float unmodulated_mdelay_out(ModDelayLine *delay);
void set_mdelay(ModDelayLine *delay, int delay_length);
float get_mdelay(ModDelayLine *delay, int index);


/** @struct DattoroReverb A reverb structure, consisting of a predelay delayline and
    twelve delaylines which form a Dattoro reverb network, two of
    which are modulating, and a set of parameters giving the feedback
    for the various elements of the reverb network */
typedef struct DattoroReverb
{

    DelayLine *pre_delay;
    float bandwidth;
    float damping;
    float decay;
    float decay_diffusion_1;
    float decay_diffusion_2;
    float input_diffusion_1;
    float input_diffusion_2;
    
    float max_excursion_1;
    float max_excursion_2;
        
    DelayLine *delay_142;
    DelayLine *delay_379;
    DelayLine *delay_107;
    DelayLine *delay_277;
    ModDelayLine *delay_672;
    ModDelayLine *delay_908;
    DelayLine *delay_4453;
    DelayLine *delay_4217;
    DelayLine *delay_3720;
    DelayLine *delay_3163;
    DelayLine *delay_1800;
    DelayLine *delay_2656;
    
    float pre_sample;    
    float diffusion_sample_a;
    float diffusion_sample_b;
    float wet_gain;
    float dry_gain;
    int sample_rate; 
} DattoroReverb;

DattoroReverb *create_reverb(int sample_rate);

void set_gain_reverb(DattoroReverb *reverb, float dry_gain_db, float wet_gain_db);
void set_size_reverb(DattoroReverb *reverb, float factor);
void set_predelay_reverb(DattoroReverb *reverb, double predelay);
void set_bandwidth_reverb(DattoroReverb *reverb, double bandwidth);
void set_damping_reverb(DattoroReverb *reverb, double damping);
void set_decay_reverb(DattoroReverb *reverb, double decay);
void set_decay_diffusion_1_reverb(DattoroReverb *reverb, double decay_diffusion_1);
void set_decay_diffusion_2_reverb(DattoroReverb *reverb, double decay_diffusion_2);
void set_input_diffusion_1_reverb(DattoroReverb *reverb, double decay_diffusion_1);
void set_input_diffusion_2_reverb(DattoroReverb *reverb, double decay_diffusion_2);
void set_modulation_reverb(DattoroReverb *reverb, double modulation);
void destroy_reverb(DattoroReverb *reverb);
void set_default_reverb(DattoroReverb *reverb);
void compute_reverb(DattoroReverb *reverb, float l, float r, float *out_l, float *out_r);
void mono_reverb_buffer(DattoroReverb *reverb, float *buffer, int n_samples);
void stereo_reverb_buffer(DattoroReverb *reverb, float *buffer, int n_samples);

#endif