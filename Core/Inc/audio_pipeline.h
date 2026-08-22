#ifndef AUDIO_PIPELINE_H
#define AUDIO_PIPELINE_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PIPELINE_FRAME_COUNT 1024U

void AudioPipeline_Init(float32_t sampleRate);
void AudioPipeline_Process(uint8_t *audioData);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PIPELINE_H */
