#ifndef AUDIO_EQUALIZER_H
#define AUDIO_EQUALIZER_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EQ_BLOCK_SIZE 1024U
#define EQ_BAND_COUNT 5U

void AudioEqualizer_Init(float32_t sampleRate);
void AudioEqualizer_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
);

/* TouchGFX-facing API kept for compatibility. */
void AudioEQ_SetBandGain(uint8_t band, float32_t gainDB);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_EQUALIZER_H */
