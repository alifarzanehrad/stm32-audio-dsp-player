#include "audio_player.h"

#include "audio_pipeline.h"
#include "stm32746g_discovery_audio.h"

#include <stdio.h>
#include <stddef.h>
#include <string.h>

FATFS SDFatFs;
uint8_t AudioBuffer[AUDIO_PLAYER_BUFFER_SIZE];

volatile uint8_t AudioPlaying;
volatile uint8_t HalfBufferNeedsFill;
volatile uint8_t FullBufferNeedsFill;
volatile uint8_t AudioTrackFinished;
volatile uint32_t AudioBufferDeadlineMisses;

static FIL wavFile;
static volatile uint32_t remainingBytes;
static uint8_t wavFileOpen;

static void WavPlayer_CloseFile(void)
{
    if (wavFileOpen != 0U)
    {
        f_close(&wavFile);
        wavFileOpen = 0U;
    }
}

extern volatile uint8_t AudioVolume;

void WavPlayer_FillHalf(uint8_t *half)
{
    UINT bytesRead = 0U;

    if ((AudioPlaying == 0U) || (half == NULL))
    {
        return;
    }

    if (remainingBytes > 0U)
    {
        UINT bytesToRead =
            (remainingBytes < AUDIO_PLAYER_HALF_BUFFER_SIZE)
                ? (UINT)remainingBytes
                : (UINT)AUDIO_PLAYER_HALF_BUFFER_SIZE;

        FRESULT result = f_read(
            &wavFile,
            half,
            bytesToRead,
            &bytesRead
        );

        if (result != FR_OK)
        {
            bytesRead = 0U;
            remainingBytes = 0U;
            AudioTrackFinished = 1U;
        }
        else
        {
            remainingBytes -= bytesRead;
        }

        if (bytesRead < AUDIO_PLAYER_HALF_BUFFER_SIZE)
        {
            memset(
                &half[bytesRead],
                0,
                AUDIO_PLAYER_HALF_BUFFER_SIZE - bytesRead
            );
        }
    }
    else
    {
        memset(
            half,
            0,
            AUDIO_PLAYER_HALF_BUFFER_SIZE
        );

        AudioPlaying = 0U;
        HalfBufferNeedsFill = 0U;
        FullBufferNeedsFill = 0U;

        /*
         * Keep the file open until the player task stops DMA. This avoids
         * starting the next track while the previous DMA stream is alive.
         */
        AudioTrackFinished = 1U;
    }
}

void WavPlayer_Stop(void)
{
    /*
     * Stop the peripheral even when AudioPlaying was cleared at EOF. DMA may
     * still be producing callbacks until BSP_AUDIO_OUT_Stop() is called.
     */
    BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);

    AudioPlaying = 0U;
    HalfBufferNeedsFill = 0U;
    FullBufferNeedsFill = 0U;

    WavPlayer_CloseFile();
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
    if (HalfBufferNeedsFill != 0U)
    {
        AudioBufferDeadlineMisses++;
    }

    HalfBufferNeedsFill = 1U;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
    if (FullBufferNeedsFill != 0U)
    {
        AudioBufferDeadlineMisses++;
    }

    FullBufferNeedsFill = 1U;
}

void AudioPlayer_ResetDeadlineMisses(void)
{
    AudioBufferDeadlineMisses = 0U;
}

uint32_t AudioPlayer_GetDeadlineMisses(void)
{
    return AudioBufferDeadlineMisses;
}

uint8_t WavPlayer_Start(const char *filename)
{
    FRESULT result;
    UINT bytesRead;
    uint8_t audioStatus;
    char chunkId[4];
    uint32_t chunkSize;

    AudioPlaying = 0U;
    HalfBufferNeedsFill = 0U;
    FullBufferNeedsFill = 0U;
    AudioTrackFinished = 0U;

    printf("Opening file %s...\r\n", filename);

    WavPlayer_CloseFile();
    result = f_open(&wavFile, filename, FA_READ);

    if (result != FR_OK)
    {
        printf("f_open failed, res=%d\r\n", result);
        return 0U;
    }

    wavFileOpen = 1U;
    printf("File opened OK\r\n");

    char riff[4];
    char wave[4];
    uint32_t riffSize;

    f_read(&wavFile, riff, 4U, &bytesRead);
    f_read(&wavFile, &riffSize, 4U, &bytesRead);
    f_read(&wavFile, wave, 4U, &bytesRead);

    if ((strncmp(riff, "RIFF", 4U) != 0) ||
        (strncmp(wave, "WAVE", 4U) != 0))
    {
        printf("Invalid WAV file (bad RIFF/WAVE header)!\r\n");
        WavPlayer_CloseFile();
        return 0U;
    }

    printf("RIFF/WAVE header OK\r\n");

    uint8_t foundFormat = 0U;
    uint8_t foundData = 0U;
    uint16_t audioFormat = 0U;
    uint16_t channelCount = 0U;
    uint16_t bitsPerSample = 0U;
    uint16_t blockAlign = 0U;
    uint32_t sampleRate = 0U;
    uint32_t byteRate = 0U;
    uint32_t dataSize = 0U;

    /* Walk RIFF chunks until the format and audio data are found. */
    while (foundData == 0U)
    {
        result = f_read(
            &wavFile,
            chunkId,
            4U,
            &bytesRead
        );

        if ((result != FR_OK) || (bytesRead != 4U))
        {
            printf("EOF before data chunk was found\r\n");
            WavPlayer_CloseFile();
            return 0U;
        }

        result = f_read(
            &wavFile,
            &chunkSize,
            4U,
            &bytesRead
        );

        if ((result != FR_OK) || (bytesRead != 4U))
        {
            printf("Failed to read chunk size\r\n");
            WavPlayer_CloseFile();
            return 0U;
        }

        printf(
            "Found chunk '%.4s' size=%lu\r\n",
            chunkId,
            (unsigned long)chunkSize
        );

        if (strncmp(chunkId, "fmt ", 4U) == 0)
        {
            FSIZE_t formatStart = f_tell(&wavFile);

            f_read(&wavFile, &audioFormat, 2U, &bytesRead);
            f_read(&wavFile, &channelCount, 2U, &bytesRead);
            f_read(&wavFile, &sampleRate, 4U, &bytesRead);
            f_read(&wavFile, &byteRate, 4U, &bytesRead);
            f_read(&wavFile, &blockAlign, 2U, &bytesRead);
            f_read(&wavFile, &bitsPerSample, 2U, &bytesRead);

            f_lseek(&wavFile, formatStart + chunkSize);
            foundFormat = 1U;
        }
        else if (strncmp(chunkId, "data", 4U) == 0)
        {
            dataSize = chunkSize;
            foundData = 1U;
        }
        else
        {
            FSIZE_t currentPosition = f_tell(&wavFile);
            f_lseek(&wavFile, currentPosition + chunkSize);
        }
    }

    if (foundFormat == 0U)
    {
        printf("No fmt chunk found!\r\n");
        WavPlayer_CloseFile();
        return 0U;
    }

    printf(
        "AudioFormat=%u NumChannels=%u SampleRate=%lu "
        "BitsPerSample=%u DataSize=%lu\r\n",
        audioFormat,
        channelCount,
        (unsigned long)sampleRate,
        bitsPerSample,
        (unsigned long)dataSize
    );

    if (audioFormat != 1U)
    {
        printf("Not PCM format!\r\n");
        WavPlayer_CloseFile();
        return 0U;
    }

    remainingBytes = dataSize;
    AudioPipeline_Init((float32_t)sampleRate);

    printf(
        "Calling BSP_AUDIO_OUT_Init with sampleRate=%lu\r\n",
        (unsigned long)sampleRate
    );

    audioStatus = BSP_AUDIO_OUT_Init(
        OUTPUT_DEVICE_HEADPHONE,
        AudioVolume,
        sampleRate
    );

    if (audioStatus != AUDIO_OK)
    {
        printf(
            "BSP_AUDIO_OUT_Init failed, status=%u\r\n",
            audioStatus
        );

        WavPlayer_CloseFile();
        return 0U;
    }

    BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
    printf("Audio codec init OK\r\n");

    UINT initialBytesToRead =
        (remainingBytes < AUDIO_PLAYER_BUFFER_SIZE)
            ? (UINT)remainingBytes
            : (UINT)AUDIO_PLAYER_BUFFER_SIZE;

    result = f_read(
        &wavFile,
        AudioBuffer,
        initialBytesToRead,
        &bytesRead
    );

    if (result != FR_OK)
    {
        printf("Initial audio read failed, res=%d\r\n", result);
        BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
        WavPlayer_CloseFile();
        return 0U;
    }

    if (bytesRead < AUDIO_PLAYER_BUFFER_SIZE)
    {
        memset(
            &AudioBuffer[bytesRead],
            0,
            AUDIO_PLAYER_BUFFER_SIZE - bytesRead
        );
    }

    remainingBytes -= bytesRead;

    AudioPipeline_Process(&AudioBuffer[0]);

    AudioPipeline_Process(
        &AudioBuffer[AUDIO_PLAYER_HALF_BUFFER_SIZE]
    );

    printf(
        "Initial buffer filled, bytesread=%u\r\n",
        bytesRead
    );

    AudioPlaying = 1U;

    audioStatus = BSP_AUDIO_OUT_Play(
        (uint16_t *)AudioBuffer,
        AUDIO_PLAYER_BUFFER_SIZE
    );

    if (audioStatus != AUDIO_OK)
    {
        printf(
            "BSP_AUDIO_OUT_Play failed, status=%u\r\n",
            audioStatus
        );

        BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
        AudioPlaying = 0U;
        WavPlayer_CloseFile();
        return 0U;
    }

    printf("Playback started!\r\n");
    return 1U;
}
