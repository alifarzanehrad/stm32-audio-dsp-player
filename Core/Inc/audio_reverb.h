#ifndef AUDIO_REVERB_H
#define AUDIO_REVERB_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

void AudioReverb_Init(float32_t sampleRate);
void AudioReverb_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
);

void AudioReverb_SetEnabled(uint8_t enabled);
uint8_t AudioReverb_IsEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_REVERB_H */
