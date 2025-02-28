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
    DelayLine *delay = (DelayLine *)(malloc(sizeof(*delay)));
    delay->max_n_samples = INIT_DELAY_MAX;
    delay->n_samples = 0;
    delay->read_head = 1;    
    delay->write_head = 0;        
    delay->samples = (float *)calloc(sizeof(*delay->samples), INIT_DELAY_MAX);
    return delay;
}

// Destroy a delay line object
void destroy_delay(DelayLine *delay)
{
    free(delay->samples);
    free(delay);
}

// Delay a sample
float delay(DelayLine *delay, float sample)
{
    delay_in(delay, sample);
    return delay_out(delay);
}

// Take a new sample into the delay line
void delay_in(DelayLine *delay, float sample)
{
    delay->samples[delay->write_head++] = sample;
    delay->read_head++;
    if (delay->read_head >= delay->n_samples)
        delay->read_head = 0;
    if (delay->write_head >= delay->n_samples)
        delay->write_head = 0;
}

// Get the sample at (write_head - index)
float get_delay(DelayLine *delay, int index)
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
    return delay->samples[delay->read_head];
}

// expand the size of the delay line to fit
void expand_delay(DelayLine *delay, int samples)
{
    int i, old_length;
    old_length = delay->max_n_samples;

    delay->max_n_samples = samples;
    delay->samples = (float *)realloc(delay->samples, sizeof(*delay->samples) * delay->max_n_samples);

    // zero out new part
    for (i = old_length; i < delay->max_n_samples; i++)
    {
        delay->samples[i] = 0.0;
    }
}

// Set the delay line length
void set_delay(DelayLine *delay, int delay_length)
{

    while (delay_length >= delay->max_n_samples)
    {
        expand_delay(delay, delay_length + 1000);
    }

    if (delay_length > 2 && delay_length <= delay->max_n_samples)
    {
        delay->n_samples = delay_length;
    }

    if (delay->read_head >= delay->n_samples)
        delay->read_head = 0;
    if (delay->write_head >= delay->n_samples)
        delay->write_head = 0;

    // stop pointers from overlapping
    if (delay->read_head == delay->write_head)
    {
        delay->read_head = 1;
    }
}

// Create a delay line with a given maximum length
// Delay will start out with a delay equal to the maximum
ModDelayLine *create_mdelay()
{
    ModDelayLine *delay = (ModDelayLine *)malloc(sizeof(*delay));

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

    return delay;
}

// set the frequency and extent of the modulation on this delay line
// if modulation extent is 0, don't modulate
void set_modulation_mdelay(ModDelayLine *delay, float modulation_extent, float modulation_frequency)
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
void destroy_mdelay(ModDelayLine *delay)
{
    free(delay->samples);
    free(delay);
}

// Delay a sample
float mdelay(ModDelayLine *delay, float sample)
{

    mdelay_in(delay, sample);
    return mdelay_out(delay);
}

// return the delayed sample _without_ the modulation
float unmodulated_mdelay_out(ModDelayLine *delay)
{
    int aread;

    aread = delay->write_head + delay->read_offset;
    if (aread >= delay->n_samples)
        aread -= delay->n_samples;
    return delay->samples[aread];
}

// Take a new sample into the delay line
void mdelay_in(ModDelayLine *delay, float sample)
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
}

// set the interpolation mode (linear or allpass)
void set_interpolation_mode_mdelay(ModDelayLine *delay, int mode)
{
    delay->interpolation_mode = mode;
}

// Get the sample at (write_head - index)
float get_mdelay(ModDelayLine *delay, int index)
{
    int rindex = delay->write_head - index;
    while (rindex < 0)
        rindex += delay->n_samples;
    return delay->samples[rindex];
}

// Return the current output of the delay line
// Does not adjust the delay line state
float mdelay_out(ModDelayLine *delay)
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

    if (!delay->modulated)
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
void set_mdelay(ModDelayLine *delay, int delay_length)
{
    // expand the delay line if the new delay is longer than the current delay line
    // always need 2*delay_length samples
    // read head is centered on write_head + delay_length
    if (delay_length * 2 >= delay->max_n_samples)
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
}

// Set the default parameters for a Dattoro reverberator
void set_default_reverb(DattoroReverb *reverb)
{
    reverb->decay = 0.7;
    reverb->decay_diffusion_1 = 0.6;
    reverb->decay_diffusion_2 = 0.6;
    reverb->input_diffusion_1 = 0.55;
    reverb->input_diffusion_2 = 0.625;
    reverb->bandwidth = 0.5;
    reverb->damping = 0.05;
}

// set the predelay, in seconds
void set_predelay_reverb(DattoroReverb *reverb, double predelay)
{
    set_delay(reverb->pre_delay, predelay * reverb->sample_rate);
}

// set the bandwidth, from 0.0 to 0.99999
void set_bandwidth_reverb(DattoroReverb *reverb, double bandwidth)
{
    reverb->bandwidth = bandwidth;
}

// set the damping, from 0.0 to 0.99999
void set_damping_reverb(DattoroReverb *reverb, double damping)
{
    reverb->damping = damping;
}

// set the decay, from 0.0 to 0.99999
void set_decay_reverb(DattoroReverb *reverb, double decay)
{
    reverb->decay = decay;
}

// set the first decay diffusion, from 0.0 to 0.99999
void set_decay_diffusion_1_reverb(DattoroReverb *reverb, double decay_diffusion_1)
{
    reverb->decay_diffusion_1 = decay_diffusion_1;
}

// set the second decay diffusion, from 0.0 to 0.99999
void set_decay_diffusion_2_reverb(DattoroReverb *reverb, double decay_diffusion_2)
{
    reverb->decay_diffusion_2 = decay_diffusion_2;
}

// set the first input diffusion, from 0.0 to 0.99999
void set_input_diffusion_1_reverb(DattoroReverb *reverb, double input_diffusion_1)
{
    reverb->input_diffusion_1 = input_diffusion_1;
}

// set the second input diffusion, from 0.0 to 0.99999
void set_input_diffusion_2_reverb(DattoroReverb *reverb, double input_diffusion_2)
{
    reverb->input_diffusion_2 = input_diffusion_2;
}

// set the modulation depth (0.0 -> 1.0)
void set_modulation_reverb(DattoroReverb *reverb, double modulation)
{
    set_modulation_mdelay(reverb->delay_672, 60.0 * modulation, 1.25 / reverb->sample_rate);
    set_modulation_mdelay(reverb->delay_908, 40.0 * modulation, 4.87 / reverb->sample_rate);
}

void set_size_reverb(DattoroReverb *reverb, float factor)
{
    double sr_ratio;

    // fix delay lengths for differing sample rates
    sr_ratio = factor * (reverb->sample_rate) / 29761.0;
    set_delay(reverb->delay_142, 142 * sr_ratio);
    set_delay(reverb->delay_379, 379 * sr_ratio);
    set_delay(reverb->delay_107, 107 * sr_ratio);
    set_delay(reverb->delay_277, 277 * sr_ratio);
    set_mdelay(reverb->delay_672, 672 * sr_ratio);
    set_delay(reverb->delay_4453, 4453 * sr_ratio);
    set_delay(reverb->delay_4217, 4217 * sr_ratio);
    set_delay(reverb->delay_3720, 3720 * sr_ratio);
    set_delay(reverb->delay_3163, 3163 * sr_ratio);
    set_delay(reverb->delay_1800, 1800 * sr_ratio);
    set_delay(reverb->delay_2656, 2656 * sr_ratio);
    set_mdelay(reverb->delay_908, 908 * sr_ratio);
}

// Create a new reverb
DattoroReverb *create_reverb(int sample_rate)
{

    DattoroReverb *reverb = (DattoroReverb *)malloc(sizeof(*reverb));
    reverb->pre_delay = create_delay();
    reverb->sample_rate = sample_rate;
    set_predelay_reverb(reverb, 0.001);
    reverb->pre_sample = 0;
    reverb->diffusion_sample_a = 0;
    reverb->diffusion_sample_b = 0;

    reverb->delay_142 = create_delay();
    reverb->delay_379 = create_delay();
    reverb->delay_107 = create_delay();
    reverb->delay_277 = create_delay();
    reverb->delay_672 = create_mdelay();
    reverb->delay_908 = create_mdelay();
    reverb->delay_4453 = create_delay();
    reverb->delay_4217 = create_delay();
    reverb->delay_3720 = create_delay();
    reverb->delay_3163 = create_delay();
    reverb->delay_1800 = create_delay();
    reverb->delay_2656 = create_delay();

    set_size_reverb(reverb, 1.0);

    set_modulation_mdelay(reverb->delay_672, 60.0, 1.25 / reverb->sample_rate);
    set_modulation_mdelay(reverb->delay_908, 40.0, 4.87 / reverb->sample_rate);

    set_default_reverb(reverb);
    return reverb;
}



// Destroy a reverb and free all the delay lines
void destroy_reverb(DattoroReverb *reverb)
{
    destroy_delay(reverb->pre_delay);
    destroy_delay(reverb->delay_142);
    destroy_delay(reverb->delay_379);
    destroy_delay(reverb->delay_277);
    destroy_mdelay(reverb->delay_672);
    destroy_mdelay(reverb->delay_908);
    destroy_delay(reverb->delay_4453);
    destroy_delay(reverb->delay_4217);
    destroy_delay(reverb->delay_3720);
    destroy_delay(reverb->delay_3163);
    destroy_delay(reverb->delay_1800);
    destroy_delay(reverb->delay_2656);

    free(reverb);
}

void db_to_gain(float db, float *gain)
{
    *gain = pow(10.0, db / 20.0);
}

void set_gain_reverb(DattoroReverb *reverb, float dry_gain_db, float wet_gain_db)
{
    db_to_gain(dry_gain_db, &reverb->dry_gain);
    db_to_gain(wet_gain_db, &reverb->wet_gain);
}


// Take a stereo signal and compute the Dattoro reverb of it
void compute_reverb(DattoroReverb *reverb, float l, float r, float *out_l, float *out_r)
{
    float x, y, z, p, q, yl, yr;

    // Initial computation
    x = (l + r) / 2.0;
    x = delay(reverb->pre_delay, x);
    x = reverb->bandwidth * x + (1 - reverb->bandwidth) * reverb->pre_sample;
    reverb->pre_sample = x;

    // Sequential part

    // delay line 142
    y = delay_out(reverb->delay_142) * reverb->input_diffusion_1;
    z = x - y;
    delay_in(reverb->delay_142, z);
    x = y + z * reverb->input_diffusion_1;

    // delay line 107
    y = delay_out(reverb->delay_107) * reverb->input_diffusion_1;
    z = x - y;
    delay_in(reverb->delay_107, z);
    x = y + z * reverb->input_diffusion_1;

    // delay line 379
    y = delay_out(reverb->delay_379);
    z = x - y * reverb->input_diffusion_2;
    delay_in(reverb->delay_379, z);
    x = y + z * reverb->input_diffusion_2;

    // delay line 277
    y = delay_out(reverb->delay_277);
    z = x - y * reverb->input_diffusion_2;
    delay_in(reverb->delay_277, z);
    x = y + z * reverb->input_diffusion_2;

    p = reverb->decay * delay_out(reverb->delay_3720) + x;
    q = reverb->decay * delay_out(reverb->delay_3163) + x;

    // P Loop
    // delay line 672
    y = mdelay_out(reverb->delay_672);
    z = p - y * reverb->decay_diffusion_1;
    mdelay_in(reverb->delay_672, z);
    p = y + z * reverb->decay_diffusion_1;

    // delay/filter 4453
    p = delay(reverb->delay_4453, p);
    p = (1 - reverb->damping) * p + reverb->damping * reverb->diffusion_sample_a;
    reverb->diffusion_sample_a = p;

    p = p * reverb->decay;

    // delay line 1800
    y = delay_out(reverb->delay_1800);
    z = p - y * reverb->decay_diffusion_2;
    delay_in(reverb->delay_1800, z);
    p = y + z * reverb->decay_diffusion_2;

    // delay line 3720
    delay_in(reverb->delay_3720, p);

    // Q loop
    // delay line 908
    y = mdelay_out(reverb->delay_908);
    z = q - y * reverb->decay_diffusion_1;
    mdelay_in(reverb->delay_908, z);
    q = y + z * reverb->decay_diffusion_1;

    // delay/filter 4217
    q = delay(reverb->delay_4217, q);
    q = (1 - reverb->damping) * q + reverb->damping * reverb->diffusion_sample_b;
    reverb->diffusion_sample_b = q;

    q = q * reverb->decay;

    // delay line 2656
    y = delay_out(reverb->delay_2656);
    z = q - y * reverb->decay_diffusion_2;
    delay_in(reverb->delay_2656, z);
    q = y + z * reverb->decay_diffusion_2;

    // delay line 3720
    delay_in(reverb->delay_3163, q);

    // left taps
    yl = 0.6 * get_delay(reverb->delay_4217, 266);
    yl += 0.6 * get_delay(reverb->delay_4217, 2974);
    yl -= 0.6 * get_delay(reverb->delay_2656, 1913);
    yl += 0.6 * get_delay(reverb->delay_3163, 1996);
    yl -= 0.6 * get_delay(reverb->delay_4453, 1990);
    yl -= 0.6 * get_delay(reverb->delay_1800, 187);
    yl -= 0.6 * get_delay(reverb->delay_3720, 1066);

    // right taps
    yr = 0.6 * get_delay(reverb->delay_4453, 353);
    yr += 0.6 * get_delay(reverb->delay_4453, 3627);
    yr -= 0.6 * get_delay(reverb->delay_1800, 1228);
    yr += 0.6 * get_delay(reverb->delay_3720, 2673);
    yr -= 0.6 * get_delay(reverb->delay_4217, 2111);
    yr -= 0.6 * get_delay(reverb->delay_2656, 335);
    yr -= 0.6 * get_delay(reverb->delay_3163, 121);

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

