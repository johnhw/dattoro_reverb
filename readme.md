# Dattoro Reverb in plain C

A simple implementation Jon Dattoro's reverb algorithm. See *J. Dattoro, "Effect Design: Part 1: Reverberator and Other Filters" J. Audio Eng. Soc. 45:9 September 1997*

Implements a basic delay line and interpolated modulating delay line and constructs the reverb from those components. The reverb is a stereo reverb with independent delay lines for left and right channels.

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

- **Predelay:** Sets the delay before the reverb effect begins, in seconds.
  ```c
  set_predelay_reverb(reverb, predelay_seconds);
  ```
- **Bandwidth:** Controls the frequency range that enters the reverb, in Hz.
  ```c
  set_bandwidth_reverb(reverb, bandwidth);
  ```
- **Damping:** Adjusts the high-frequency attenuation inside the reverb. 0-1
  ```c
  set_damping_reverb(reverb, damping);
  ```
- **Decay:** Defines the reverb tail length. 0-1
  ```c
  set_decay_reverb(reverb, decay);
  ```
- **Diffusion:** Controls how quickly reflections become dense. 0-1
  ```c
  set_decay_diffusion_1_reverb(reverb, diffusion_1);
  set_decay_diffusion_2_reverb(reverb, diffusion_2);
  set_input_diffusion_1_reverb(reverb, input_diffusion_1);
  set_input_diffusion_2_reverb(reverb, input_diffusion_2);
  ```
- **Modulation:** Adds subtle modulation to the delay lines for a more natural sound. 0-1
  ```c
  set_modulation_reverb(reverb, modulation_depth);
  ```
- **Size:** Adjusts the overall scale of the reverb space. 0-unbounded, but usually 1
  ```c
  set_size_reverb(reverb, size_factor);
  ```
- **Gain:** Balances the dry (original) and wet (reverberated) signal levels. in dB. Defaults to 0 dB for both.
  ```c
  set_gain_reverb(reverb, dry_gain_db, wet_gain_db);
  ```

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


