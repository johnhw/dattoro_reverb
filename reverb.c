/**
    @file reverb.c
    @brief An implementation of Jon Dattoro's reverb algorithm.
    See J. Dattoro, "Effect Design: Part 1: Reverberator and Other Filters" J. Audio Eng. Soc. 45:9 September 1997

    @author John Williamson

    Copyright (c) 2011-2025 All rights reserved.
    Licensed under the MIT License, 2025.

*/
#include "reverb.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

// Create a delay line with a given maximum length
// Delay will start out with a delay equal to the maximum
DelayLine *create_delay()
{
    DelayLine *delay = (DelayLine *)malloc(sizeof(*delay));

    delay->max_n_samples = INIT_DELAY_MAX * 2;
    delay->n_samples = INIT_DELAY_MAX * 2;
    delay->read_offset = INIT_DELAY_MAX;
    delay->interpolation_mode = MODDELAY_INTERPOLATION_ALLPASS;
    delay->write_head = 0;
    delay->n_samples = 0;
    delay->modulation_extent = 0.0;
    delay->modulation_frequency = 0.0;
    delay->phase = 0.0;
    delay->read_fraction = 0.0;
    delay->excursion = 0.0;
    delay->samples = (float *)calloc(sizeof(*delay->samples), delay->max_n_samples);
    delay->allpass_a = 0.0;
    delay->modulated = 0;
    delay->feedback = 0.0;

    return delay;
}

// set the frequency and extent of the modulation on this delay line
// if modulation extent is 0, don't modulate
void set_modulation_delay(DelayLine *delay, float modulation_extent, float modulation_frequency)
{

    if (delay->modulation_extent >= delay->read_offset)
        delay->modulation_extent = delay->read_offset - 1;

    if (delay->modulation_extent == 0.0)
    {
        delay->modulated = 0;
        delay->excursion = 0;
    }
    else
        delay->modulated = 1;

    delay->modulation_extent = modulation_extent;
    delay->modulation_frequency = modulation_frequency;
}

// Destroy a delay line object
void destroy_delay(DelayLine *delay)
{
    free(delay->samples);
    free(delay);
}

// Take a new sample into the delay line
void delay_in(DelayLine *delay, float sample)
{
    double offset;
    delay->samples[delay->write_head++] = sample;

    if (delay->modulated)
    {
        // update phase
        delay->phase += (2 * M_PI * delay->modulation_frequency);
        offset = sin(delay->phase) * delay->modulation_extent;
        delay->excursion = floor(offset);
        delay->read_fraction = offset - floor(offset);
    }
    if (delay->write_head >= delay->n_samples)
        delay->write_head = 0;

    // feedback, if enabled
    if (delay->feedback != 0.0)
    {
        delay->samples[delay->write_head] += delay->feedback * delay_out(delay);
    }
}

// set the interpolation mode (linear or allpass)
void set_interpolation_mode_delay(DelayLine *delay, int mode)
{
    delay->interpolation_mode = mode;
}

// Get the sample at (write_head - index)
float tap_delay(DelayLine *delay, int index)
{
    int rindex = delay->write_head - index;
    while (rindex < 0)
        rindex += delay->n_samples;
    return delay->samples[rindex];
}

// Return the current output of the delay line
// Does not adjust the delay line state
float delay_out(DelayLine *delay)
{

    int aread, bread;
    float an, bn, fr, out;

    aread = delay->write_head + delay->read_offset + delay->excursion;
    bread = delay->write_head + delay->read_offset + delay->excursion + 1;

    if (bread >= delay->n_samples)
        bread -= delay->n_samples;
    if (aread >= delay->n_samples)
        aread -= delay->n_samples;

    an = delay->samples[aread];
    bn = delay->samples[bread];

    if (delay->read_fraction == 0.0 || delay->interpolation_mode == MODDELAY_INTERPOLATION_NONE)
        return an;
        
    if (delay->interpolation_mode == MODDELAY_INTERPOLATION_LINEAR)
    {
        // read from write_head + delay line length + modulation factor
        return (1 - delay->read_fraction) * an + (delay->read_fraction) * bn;
    }
    else
    {
        // allpass coefficient
        fr = (1 - (1 - delay->read_fraction)) / (1 + (1 - delay->read_fraction));
        out = bn * fr + an - fr * delay->allpass_a;
        delay->allpass_a = out;
        return out;
    }
}

// Set the delay line length
void set_delay(DelayLine *delay, float length)
{
    // expand the delay line if the new delay is longer than the current delay line
    // always need 2*delay_length samples
    // read head is centered on write_head + delay_length
    int delay_length = (int)length;
    if (delay_length * 2 >= delay->max_n_samples-1)
    {
        int i, old_length;
        old_length = delay->max_n_samples;
        delay->max_n_samples = delay_length * 2 + 1;
        delay->samples = (float *)realloc(delay->samples, sizeof(*delay->samples) * delay->max_n_samples);
        for (i = old_length; i < delay->max_n_samples; i++)
            delay->samples[i] = 0.0;
    }

    if (delay_length > 2)
        delay->read_offset = delay_length;

    delay->n_samples = delay_length * 2;
    delay->read_fraction = length - delay_length;
}

// Set the default parameters for a Dattoro reverberator
void set_default_reverb(DattoroReverb *reverb)
{
    set_reverb_param(reverb, REVERB_PREDELAY, 0.001);
    set_reverb_param(reverb, REVERB_BANDWIDTH, reverb->sample_rate / 2);
    set_reverb_param(reverb, REVERB_DAMPING, 0.05);
    set_reverb_param(reverb, REVERB_DECAY, 0.7);
    set_reverb_param(reverb, REVERB_DIFFUSION_1, 0.6);
    set_reverb_param(reverb, REVERB_DIFFUSION_2, 0.6);
    set_reverb_param(reverb, REVERB_INPUT_DIFFUSION_1, 0.55);
    set_reverb_param(reverb, REVERB_INPUT_DIFFUSION_2, 0.625);
    set_reverb_param(reverb, REVERB_MODULATION, 1.0);
    set_reverb_param(reverb, REVERB_SIZE, 1.0);
    set_reverb_param(reverb, REVERB_WET, -6.0);
    set_reverb_param(reverb, REVERB_DRY, 0.0);
}

enum DELAY_NAMES
{
    DELAY_142,
    DELAY_379,
    DELAY_107,
    DELAY_277,
    DELAY_672,
    DELAY_908,
    DELAY_4453,
    DELAY_4217,
    DELAY_3720,
    DELAY_3163,
    DELAY_1800,
    DELAY_2656,
    DELAY_MAX
};

void set_reverb_param(DattoroReverb *reverb, int param, double value)
{
    const int delay_times[DELAY_MAX] = {142, 379, 107, 277, 672, 908, 4453, 4217, 3720, 3163, 1800, 2656};

    double sr_ratio;
    switch (param)
    {
    case REVERB_PREDELAY:
        set_delay(reverb->pre_delay, value * reverb->sample_rate);
        break;
    case REVERB_BANDWIDTH:
        reverb->bandwidth = value / reverb->sample_rate;
        break;
    case REVERB_DAMPING:
        reverb->damping = value;
        break;
    case REVERB_DECAY:
        reverb->decay = value;
        break;
    case REVERB_DIFFUSION_1:
        reverb->decay_diffusion_1 = value;
        break;
    case REVERB_DIFFUSION_2:
        reverb->decay_diffusion_2 = value;
        break;
    case REVERB_INPUT_DIFFUSION_1:
        reverb->input_diffusion_1 = value;
        break;
    case REVERB_INPUT_DIFFUSION_2:
        reverb->input_diffusion_2 = value;
        break;
    case REVERB_MODULATION:
        set_modulation_delay(reverb->delay_lines[DELAY_672], 60.0 * value, 1.25 / reverb->sample_rate);
        set_modulation_delay(reverb->delay_lines[DELAY_908], 40.0 * value, 4.87 / reverb->sample_rate);
        break;
    case REVERB_SIZE:
        sr_ratio = value * (reverb->sample_rate) / 29761.0;
        for (int i = 0; i < DELAY_MAX; i++)
            set_delay(reverb->delay_lines[i], delay_times[i] * sr_ratio);
        break;
    case REVERB_WET:
        reverb->wet_gain = pow(10.0, value / 20.0);
        break;
    case REVERB_DRY:
        reverb->dry_gain = pow(10.0, value / 20.0);
        break;
    }
}

// Create a new reverb
DattoroReverb *create_reverb(int sample_rate)
{
    DattoroReverb *reverb = (DattoroReverb *)malloc(sizeof(*reverb));
    reverb->pre_delay = create_delay();
    reverb->sample_rate = sample_rate;
    reverb->pre_sample = 0;
    reverb->diffusion_sample_a = 0;
    reverb->diffusion_sample_b = 0;
    for (int i = 0; i < DELAY_MAX; i++)
        reverb->delay_lines[i] = create_delay();
    set_default_reverb(reverb);
    return reverb;
}

// Destroy a reverb and free all the delay lines
void destroy_reverb(DattoroReverb *reverb)
{
    for (int i = 0; i < DELAY_MAX; i++)
        destroy_delay(reverb->delay_lines[i]);
    free(reverb);
}

float apply_diffusion(DelayLine *delay, float x, float diffusion)
{
    float y = delay_out(delay);
    float z = x - y * diffusion;
    delay_in(delay, z);
    return y + z * diffusion;
}

// Take a stereo signal and compute the Dattoro reverb of it
void compute_reverb(DattoroReverb *reverb, float l, float r, float *out_l, float *out_r)
{
    float x, y, z, p, q, yl, yr;

    // Initial computation
    x = (l + r) / 2.0;
    delay_in(reverb->pre_delay, x);
    x = delay_out(reverb->pre_delay);
    x = reverb->bandwidth * x + (1 - reverb->bandwidth) * reverb->pre_sample;
    reverb->pre_sample = x;

    // Sequential part

    // delay line 142
    x = apply_diffusion(reverb->delay_lines[DELAY_142], x, reverb->input_diffusion_1);
    x = apply_diffusion(reverb->delay_lines[DELAY_107], x, reverb->input_diffusion_1);
    x = apply_diffusion(reverb->delay_lines[DELAY_379], x, reverb->input_diffusion_2);
    x = apply_diffusion(reverb->delay_lines[DELAY_277], x, reverb->input_diffusion_2);

    p = reverb->decay * delay_out(reverb->delay_lines[DELAY_3720]) + x;
    q = reverb->decay * delay_out(reverb->delay_lines[DELAY_3163]) + x;

    // P Loop
    // delay line 672
    y = delay_out(reverb->delay_lines[DELAY_672]);
    z = p - y * reverb->decay_diffusion_1;
    delay_in(reverb->delay_lines[DELAY_672], z);
    p = y + z * reverb->decay_diffusion_1;

    // delay/filter 4453
    p = delay_out(reverb->delay_lines[DELAY_4453]);
    p = (1 - reverb->damping) * p + reverb->damping * reverb->diffusion_sample_a;
    reverb->diffusion_sample_a = p;

    p = p * reverb->decay;

    // delay line 1800
    p = apply_diffusion(reverb->delay_lines[DELAY_1800], p, reverb->decay_diffusion_2);

    // delay line 3720
    delay_in(reverb->delay_lines[DELAY_3720], p);

    // Q loop
    // delay line 908
    y = delay_out(reverb->delay_lines[DELAY_908]);
    z = q - y * reverb->decay_diffusion_1;
    delay_in(reverb->delay_lines[DELAY_908], z);
    q = y + z * reverb->decay_diffusion_1;

    // delay/filter 4217
    delay_in(reverb->delay_lines[DELAY_4217], q);
    q = delay_out(reverb->delay_lines[DELAY_4217]);
    q = (1 - reverb->damping) * q + reverb->damping * reverb->diffusion_sample_b;
    reverb->diffusion_sample_b = q;

    q = q * reverb->decay;

    // delay line 2656
    q = apply_diffusion(reverb->delay_lines[DELAY_2656], q, reverb->decay_diffusion_2);

    // delay line 3720
    delay_in(reverb->delay_lines[DELAY_3163], q);

    // left taps
    yl = 0.6 * tap_delay(reverb->delay_lines[DELAY_4217], 266);
    yl += 0.6 * tap_delay(reverb->delay_lines[DELAY_4217], 2974);
    yl -= 0.6 * tap_delay(reverb->delay_lines[DELAY_2656], 1913);
    yl += 0.6 * tap_delay(reverb->delay_lines[DELAY_3163], 1996);
    yl -= 0.6 * tap_delay(reverb->delay_lines[DELAY_4453], 1990);
    yl -= 0.6 * tap_delay(reverb->delay_lines[DELAY_1800], 187);
    yl -= 0.6 * tap_delay(reverb->delay_lines[DELAY_3720], 1066);

    // right taps
    yr = 0.6 * tap_delay(reverb->delay_lines[DELAY_4453], 353);
    yr += 0.6 * tap_delay(reverb->delay_lines[DELAY_4453], 3627);
    yr -= 0.6 * tap_delay(reverb->delay_lines[DELAY_1800], 1228);
    yr += 0.6 * tap_delay(reverb->delay_lines[DELAY_3720], 2673);
    yr -= 0.6 * tap_delay(reverb->delay_lines[DELAY_4217], 2111);
    yr -= 0.6 * tap_delay(reverb->delay_lines[DELAY_2656], 335);
    yr -= 0.6 * tap_delay(reverb->delay_lines[DELAY_3163], 121);

    *out_l = yl;
    *out_r = yr;
}

void mono_reverb_buffer(DattoroReverb *reverb, float *buffer, int bufferLen)
{
    int i;
    float l, r;
    for (i = 0; i < bufferLen; i++)
    {
        compute_reverb(reverb, buffer[i], buffer[i], &l, &r);
        buffer[i] = reverb->dry_gain * buffer[i] + reverb->wet_gain * l;
    }
}

// assumes interleaved stereo
void stereo_reverb_buffer(DattoroReverb *reverb, float *buffer, int bufferLen)
{
    int i;
    float l, r;

    for (i = 0; i < bufferLen; i += 2)
    {
        compute_reverb(reverb, buffer[i], buffer[i + 1], &l, &r);
        buffer[i] = reverb->dry_gain * buffer[i] + reverb->wet_gain * l;
        buffer[i + 1] = reverb->dry_gain * buffer[i + 1] + reverb->wet_gain * r;
    }
}
