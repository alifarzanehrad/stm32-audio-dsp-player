#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <stdint.h>
#include "ff.h"

#ifdef __cplusplus
extern "C" {
#endif

#define AUDIO_PLAYER_BUFFER_SIZE 8192U
#define AUDIO_PLAYER_HALF_BUFFER_SIZE \
    (AUDIO_PLAYER_BUFFER_SIZE / 2U)

extern FATFS SDFatFs;
extern uint8_t AudioBuffer[AUDIO_PLAYER_BUFFER_SIZE];

extern volatile uint8_t AudioPlaying;
extern volatile uint8_t HalfBufferNeedsFill;
extern volatile uint8_t FullBufferNeedsFill;
extern volatile uint8_t AudioTrackFinished;
extern volatile uint32_t AudioBufferDeadlineMisses;

uint8_t WavPlayer_Start(const char *filename);
void WavPlayer_FillHalf(uint8_t *half);
void WavPlayer_Stop(void);
void AudioPlayer_ResetDeadlineMisses(void);
uint32_t AudioPlayer_GetDeadlineMisses(void);
uint8_t AudioPlayer_GetCurrentTrackName(char *destination, uint16_t capacity);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_PLAYER_H */
