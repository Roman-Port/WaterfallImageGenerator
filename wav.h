#pragma once
#include <stdint.h>

#define WAV_HEADER_SIZE 44

struct wav_header_data {
    int length;
    short channels;
    int sample_rate;
    int bits_per_sample;
};

bool read_wav_header(uint8_t buffer[WAV_HEADER_SIZE], wav_header_data* output);