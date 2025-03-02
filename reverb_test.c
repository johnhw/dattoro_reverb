
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include "reverb.h"

static void writeWavStereo16(const char *filename,
                             const float *samples,
                             int numSamples,
                             int sampleRate)
{
    /* RIFF header fields */
    uint32_t dataSize = numSamples * sizeof(int16_t) * 2;
    uint32_t fileSize = 36 + dataSize; /* 36 + subchunk2Size */
    uint16_t channels = 2;
    uint16_t bitsPerSample = 16;
    uint16_t audioFormat = 1; /* PCM */
    uint32_t byteRate = sampleRate * channels * (bitsPerSample / 8);
    uint16_t blockAlign = channels * (bitsPerSample / 8);

    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        fprintf(stderr, "Cannot open %s for writing.\n", filename);
        exit(1);
    }

    /* Write the RIFF chunk descriptor */
    fwrite("RIFF", 1, 4, fp);
    fwrite(&fileSize, 4, 1, fp);
    fwrite("WAVE", 1, 4, fp);

    /* Write the 'fmt ' sub-chunk */
    fwrite("fmt ", 1, 4, fp);
    {
        uint32_t subchunkSize = 16; /* PCM */
        fwrite(&subchunkSize, 4, 1, fp);
        fwrite(&audioFormat, 2, 1, fp);
        fwrite(&channels, 2, 1, fp);
        fwrite(&sampleRate, 4, 1, fp);
        fwrite(&byteRate, 4, 1, fp);
        fwrite(&blockAlign, 2, 1, fp);
        fwrite(&bitsPerSample, 2, 1, fp);
    }

    /* Write the 'data' sub-chunk */
    fwrite("data", 1, 4, fp);
    fwrite(&dataSize, 4, 1, fp);

    /* Write the samples */
    int16_t *shortSamples = (int16_t *)malloc(numSamples * 2 * sizeof(int16_t));
    for (int i = 0; i < numSamples * 2; i++)
    {
        shortSamples[i] = (int16_t)(samples[i] * 32768.0f);
    }

    fwrite(shortSamples, sizeof(int16_t), numSamples * 2, fp);
    free(shortSamples);

    fclose(fp);
}

void readWavStereo16(const char *filename,
                     float **samples,
                     int *numSamples,
                     int *sampleRate)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp)
    {
        fprintf(stderr, "Cannot open %s for reading.\n", filename);
        exit(1);
    }

    /* RIFF header fields */
    char chunkId[5];
    uint32_t chunkSize;
    char format[5];
    char subchunk1Id[5];
    uint32_t subchunk1Size;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate_;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
    char subchunk2Id[5];
    uint32_t subchunk2Size;

    fread(chunkId, 1, 4, fp);
    fread(&chunkSize, 4, 1, fp);
    fread(format, 1, 4, fp);

    fread(subchunk1Id, 1, 4, fp);
    fread(&subchunk1Size, 4, 1, fp);
    fread(&audioFormat, 2, 1, fp);
    fread(&numChannels, 2, 1, fp);
    fread(&sampleRate_, 4, 1, fp);
    fread(&byteRate, 4, 1, fp);
    fread(&blockAlign, 2, 1, fp);
    fread(&bitsPerSample, 2, 1, fp);

    fread(subchunk2Id, 1, 4, fp);
    fread(&subchunk2Size, 4, 1, fp);

    // verify header
    if (strncmp(chunkId, "RIFF", 4) != 0 ||
        strncmp(format, "WAVE", 4) != 0 ||
        strncmp(subchunk1Id, "fmt ", 4) != 0 ||
        audioFormat != 1 || // PCM
        subchunk1Size != 16 ||
        numChannels != 2 ||
        bitsPerSample != 16 ||
        numSamples == 0 ||
        strncmp(subchunk2Id, "data", 4) != 0)
    {
        fprintf(stderr, "Invalid WAV file. Must be 16 bit stereo PCM audio.\n");
        fclose(fp);
        exit(1);
    }

    *sampleRate = sampleRate_;
    *numSamples = subchunk2Size / (numChannels * (bitsPerSample / 8));
    int16_t *shortSamples = (int16_t *)malloc(subchunk2Size);
    fread(shortSamples, 1, subchunk2Size, fp);
    *samples = (float *)malloc(*numSamples * numChannels * sizeof(float));
    for (int i = 0; i < *numSamples * numChannels; i++)
    {
        (*samples)[i] = shortSamples[i] / 32768.0f;
    }
    free(shortSamples);

    fclose(fp);
}

int main(int argc, char **argv)
{
    // check for input file
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <input.wav>\n", argv[0]);
        return 1;
    }
    // read it
    int sampleRate;
    int nSamples;
    float *samples;
    readWavStereo16(argv[1], &samples, &nSamples, &sampleRate);
    fprintf(stdout, "Read %d samples at %d Hz\n", nSamples, sampleRate);
    // construct and configure reverb
    DattoroReverb *reverb = create_reverb(sampleRate);
    set_reverb_param(reverb, REVERB_SIZE, 2);
    set_reverb_param(reverb, REVERB_WET, -1);
    // render reverb
    int reverbSamples = nSamples + 10 * sampleRate;
    float *reverbBuffer = (float *)malloc(reverbSamples * 2 * sizeof(float));
    memset(reverbBuffer, 0, reverbSamples * 2 * sizeof(float));
    memcpy(reverbBuffer, samples, nSamples * 2 * sizeof(float));
    stereo_reverb_buffer(reverb, reverbBuffer, reverbSamples);
    // write it
    char *suffix = "_reverb.wav";
    char *output = (char *)malloc(strlen(argv[1]) + strlen(suffix) + 1);    
    strcpy(output, argv[1]);
    strcat(output, suffix);
    fprintf(stdout, "Writing %d samples at %d Hz to %s\n", reverbSamples, sampleRate, output);
    writeWavStereo16(output, reverbBuffer, reverbSamples, sampleRate);
    // free everything
    free(output);
    free(samples);
    free(reverbBuffer);
    destroy_reverb(reverb);
}