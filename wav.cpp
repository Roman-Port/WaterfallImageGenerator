#include "wav.h"
#include <cstring>

template <class T>
T _wav_read(uint8_t** buffer, size_t* remaining) {
    //Read
    T value;
    memcpy(&value, *buffer, sizeof(T));

    //Update state
    (*buffer) += sizeof(T);
    (*remaining) -= sizeof(T);

    return value;
}

bool read_wav_header(uint8_t buffer[WAV_HEADER_SIZE], wav_header_data* output) {
    //Read all
    size_t remaining = WAV_HEADER_SIZE;
    uint32_t tagRiff = _wav_read<uint32_t>(&buffer, &remaining);
    int32_t length = _wav_read<int32_t>(&buffer, &remaining);
    uint32_t tagWave = _wav_read<uint32_t>(&buffer, &remaining);
    uint32_t tagFmt = _wav_read<uint32_t>(&buffer, &remaining);
    int32_t fmtLength = _wav_read<int32_t>(&buffer, &remaining);
    int16_t formatTag = _wav_read<int16_t>(&buffer, &remaining);
    int16_t channels = _wav_read<int16_t>(&buffer, &remaining);
    int32_t sampleRate = _wav_read<int32_t>(&buffer, &remaining);
    int32_t avgBytesPerSec = _wav_read<int32_t>(&buffer, &remaining);
    int16_t blockAlign = _wav_read<int16_t>(&buffer, &remaining);
    int16_t bitsPerSample = _wav_read<int16_t>(&buffer, &remaining);
    uint32_t tagData = _wav_read<uint32_t>(&buffer, &remaining);
    int32_t dataLen = _wav_read<int32_t>(&buffer, &remaining);

    //Validate tags
    if (tagRiff != 1179011410 || tagWave != 1163280727 || tagFmt != 544501094)
        return false;

    //Parse
    output->channels = channels;
    output->length = dataLen;
    output->sample_rate = sampleRate;
    output->bits_per_sample = bitsPerSample;

    return true;
}