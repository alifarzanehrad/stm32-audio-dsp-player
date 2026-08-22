#ifndef AUDIO_SPECTRUM_H
#define AUDIO_SPECTRUM_H

#include <stdint.h>
#include "arm_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_SPECTRUM_FFT_SIZE 1024U
#define AUDIO_SPECTRUM_BAND_COUNT 16U

extern float32_t
    fftBandsSmoothed[AUDIO_SPECTRUM_BAND_COUNT];

void AudioSpectrum_Init(void);

/* Existing player-facing name kept for compatibility. */
void AudioFFT_Process(uint8_t *audioData);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_SPECTRUM_H */
