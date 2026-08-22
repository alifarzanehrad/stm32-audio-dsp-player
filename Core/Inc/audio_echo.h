#ifndef AUDIO_ECHO_H
#define AUDIO_ECHO_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

void AudioEcho_Init(float32_t sampleRate);
void AudioEcho_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
);

void AudioEcho_SetEnabled(uint8_t enabled);
uint8_t AudioEcho_IsEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_ECHO_H */
