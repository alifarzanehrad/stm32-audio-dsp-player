#ifndef NOISE_REDUCTION_H
#define NOISE_REDUCTION_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

void AudioNoiseReduction_Init(void);
void AudioNoiseReduction_Process(
    float32_t *left,
    float32_t *right,
    uint32_t sampleCount
);

void AudioNoiseReduction_SetEnabled(uint8_t enabled);
uint8_t AudioNoiseReduction_IsEnabled(void);

#ifdef __cplusplus
}
#endif

#endif /* NOISE_REDUCTION_H */
