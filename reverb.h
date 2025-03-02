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
#include <stdbool.h>

typedef struct Biquad
{
    float a0, a1, a2, b0, b1, b2;
    float x1, x2, y1, y2;
    float feedback;
    bool saturate;
} Biquad;

enum BIQUAD_TYPE
{
    BIQUAD_LOWPASS,
    BIQUAD_HIGHPASS,
    BIQUAD_BANDPASS,
    BIQUAD_NOTCH,
    BIQUAD_PEAK,
    BIQUAD_LOW_SHELF,
    BIQUAD_HIGH_SHELF,
    BIQUAD_FORMANT,
    BIQUAD_ALLPASS
};

Biquad *create_biquad();
void destroy_biquad(Biquad *biquad);
void reset_biquad(Biquad *biquad);
// a can be q, bw or slope
// b can be gain or unused
void set_biquad(Biquad *biquad, int type, float freq, float a, float b);
float process_biquad(Biquad *biquad, float sample);

#define INIT_DELAY_MAX 256

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
    float feedback;
    int modulated;
    float allpass_a;
    int sample_rate;
} ModDelayLine;

ModDelayLine *create_delay(void);

void set_interpolation_mode_delay(ModDelayLine *delay, int mode);
void set_modulation_delay(ModDelayLine *delay, float modulation_extent, float modulation_frequency);
void set_feedback_delay(ModDelayLine *delay, float feedback);
void destroy_delay(ModDelayLine *delay);
void delay_in(ModDelayLine *delay, float sample);
float delay_out(ModDelayLine *delay);
void set_delay(ModDelayLine *delay, int delay_length);
float tap_delay(ModDelayLine *delay, int index);

/** @struct DattoroReverb A reverb structure, consisting of a predelay delayline and
    twelve delaylines which form a Dattoro reverb network, two of
    which are modulating, and a set of parameters giving the feedback
    for the various elements of the reverb network */
typedef struct DattoroReverb
{

    ModDelayLine *pre_delay;
    float bandwidth;
    float damping;
    float decay;
    float decay_diffusion_1;
    float decay_diffusion_2;
    float input_diffusion_1;
    float input_diffusion_2;

    float max_excursion_1;
    float max_excursion_2;
    ModDelayLine *delay_lines[12];

    float pre_sample;
    float diffusion_sample_a;
    float diffusion_sample_b;
    float wet_gain;
    float dry_gain;
    int sample_rate;
} DattoroReverb;

enum reverb_params
{
    REVERB_PREDELAY,
    REVERB_BANDWIDTH,
    REVERB_DAMPING,
    REVERB_DECAY,
    REVERB_DIFFUSION_1,
    REVERB_DIFFUSION_2,
    REVERB_INPUT_DIFFUSION_1,
    REVERB_INPUT_DIFFUSION_2,
    REVERB_MODULATION,
    REVERB_SIZE,
    REVERB_WET,
    REVERB_DRY,
    REVERB_MAX_PARAMS
};

DattoroReverb *create_reverb(int sample_rate);
void set_reverb_param(DattoroReverb *reverb, int param, double value);
void destroy_reverb(DattoroReverb *reverb);
void set_default_reverb(DattoroReverb *reverb);
void compute_reverb(DattoroReverb *reverb, float l, float r, float *out_l, float *out_r);
void mono_reverb_buffer(DattoroReverb *reverb, float *buffer, int n_samples);
void stereo_reverb_buffer(DattoroReverb *reverb, float *buffer, int n_samples);

#endif