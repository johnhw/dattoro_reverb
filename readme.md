# Dattoro Reverb in plain C

A simple implementation Jon Dattoro's reverb algorithm. See *J. Dattoro, "Effect Design: Part 1: Reverberator and Other Filters" J. Audio Eng. Soc. 45:9 September 1997*

Implements an interpolated modulating delay line and constructs the reverb from those components. The reverb is a mono input stereo output reverb with independent delay lines for left and right channels.

Originally part of the OpenGrain project, extracted to its own repository for easier reuse.

Re-licensed under the MIT license (originally OpenGrain was BSD 3-clause)

## Usage

## Creating a Reverb Instance

```c
DattoroReverb *reverb = create_reverb(sample_rate);
```

This initializes the reverb system with a given sample rate and default parameter values.

## Configuring the Reverb
Several parameters can be adjusted to modify the reverberation characteristics:

`set_reverb_param(reverb, param, value);`

- `reverb` is a pointer to the reverb instance.
- `param` is one of the following parameters:
    - `REVERB_PREDELAY` - The time in seconds before the first reflection. Default 1ms.
    - `REVERB_BANDWIDTH` - The bandwidth of the all-pass filters, in Hz. Default sample rate/2
    - `REVERB_DAMPING` - The damping factor for the low-pass filters. Default 0.05.
    - `REVERB_DECAY` - The decay factor. Default 0.7.
    - `REVERB_DIFFUSION_1` - The first diffusion coefficient. Default 0.6.
    - `REVERB_DIFFUSION_2` - The second diffusion coefficient. Default 0.6.
    - `REVERB_INPUT_DIFFUSION_1` - The input diffusion coefficient for the first delay line. Default 0.55.
    - `REVERB_INPUT_DIFFUSION_2` - The input diffusion coefficient for the second delay line. Default 0.625
    - `REVERB_MODULATION` - The modulation depth. Default 1.0.
    - `REVERB_SIZE` - The size of the reverb (as a factor; 1.0=default)
    - `REVERB_WET` - The gain of the wet signal. (default -6dB)
    - `REVERB_DRY` - The gain of the dry signal. (default 0dB)

## Applying Reverb to Audio Buffers
Once configured, the reverb effect can be applied in-place to float audio buffers using the following functions:

### Mono Processing
To apply reverb to a mono buffer:
```c
mono_reverb_buffer(DattoroReverb *reverb, float *buffer, int32_t bufferLen);
```
- `buffer` is a pointer to the mono float buffer.
- `bufferLen` is the number of samples in the buffer.

Each sample is processed, and the reverberated output is mixed back with the original signal using the configured dry and wet gain values.

### Stereo Processing
For stereo processing with an interleaved buffer:
```c
stereo_reverb_buffer(DattoroReverb *reverb, float *buffer, int32_t bufferLen);
```
- `buffer` is an interleaved stereo buffer (`[L0, R0, L1, R1, ...]`).
- `bufferLen` is the number of **samples** (not frames, so it should be `2 * num_frames`).

Each left and right channel is independently processed through the `compute_reverb` function, preserving the stereo field.

## Destroying the Reverb Instance
When no longer needed, the reverb instance should be freed to avoid memory leaks:
```c
destroy_reverb(reverb);
```


