/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32746g_discovery_audio.h"
#include "fatfs.h"
#include <stdio.h>
#include "sdmmc.h"
#include "audio_pipeline.h"
#include "audio_benchmark.h"
#include "audio_spectrum.h"
#include "audio_player.h"
#include <string.h>

extern SD_HandleTypeDef hsd1;

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

typedef enum
{
    PLAYER_STOPPED = 0,
    PLAYER_PLAYING,
    PLAYER_PAUSED
} PlayerState_t;

volatile PlayerState_t PlayerState = PLAYER_STOPPED;

volatile uint8_t AudioPlayPauseRequested = 0;

#define PLAYLIST_MAX_TRACKS 32U
#define PLAYLIST_FILENAME_SIZE 128U

static char playlist[PLAYLIST_MAX_TRACKS][PLAYLIST_FILENAME_SIZE];
static uint16_t trackCount;

volatile int currentTrack = 0;

volatile int8_t AudioTrackChangePending = 0;
volatile int8_t AudioVolumeChangePending = 0;

volatile uint8_t AudioVolume = 70;

/* The LCD only needs about 23 spectrum updates per second. */
static uint8_t spectrumDecimationCounter;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId TouchGFXTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void AudioPlayer_ClearTransferFlags(void);
static uint8_t AudioPlayer_ScanPlaylist(void);
static uint8_t AudioPlayer_IsWavFile(const char *filename);
static void AudioPlayer_SortPlaylist(void);
static uint8_t AudioPlayer_StartSelectedTrack(const char *reason);
static uint8_t AudioPlayer_ChangeTrack(int8_t delta, const char *reason);
static void AudioPlayer_ServiceBuffer(uint8_t *buffer);
static void AudioPlayer_ReportRuntimeDiagnostics(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
extern void TouchGFX_Task(void const * argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* Hook prototypes */
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 2 */
__weak void vApplicationIdleHook( void )
{
}
/* USER CODE END 2 */

/* USER CODE BEGIN 4 */
__weak void vApplicationStackOverflowHook(xTaskHandle xTask, signed char *pcTaskName)
{
}
/* USER CODE END 4 */

/* USER CODE BEGIN 5 */
__weak void vApplicationMallocFailedHook(void)
{
}
/* USER CODE END 5 */

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  /*
   * Audio refill must preempt TouchGFX rendering. Both tasks at the same
   * priority allowed screen transitions to consume the DMA refill deadline.
   */
  /*
   * Measured high-water usage is below 200 words. Keep more than 5x margin
   * for audio processing and diagnostics.
   */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityAboveNormal, 0, 1024);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of TouchGFXTask */
  /*
   * TouchGFX used about 570 words during screen-transition stress testing.
   * 2048 words preserves a wide margin for future UI changes.
   */
  osThreadDef(TouchGFXTask, TouchGFX_Task, osPriorityNormal, 0, 2048);
  TouchGFXTaskHandle = osThreadCreate(osThread(TouchGFXTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for USB_HOST */
  /* USER CODE BEGIN StartDefaultTask */

  setvbuf(stdout, NULL, _IONBF, 0);
    printf("Task started, mounting SD card...\r\n");

    printf("BSP_SD_IsDetected = %d (SD_PRESENT=1)\r\n", BSP_SD_IsDetected());

    uint8_t sd_init_ret = BSP_SD_Init();
    printf("BSP_SD_Init returned = %d (MSD_OK=0)\r\n", sd_init_ret);
    printf("HAL_SD_GetError = 0x%08lX\r\n", (unsigned long)HAL_SD_GetError(&hsd1));

    FRESULT mount_res = f_mount(&SDFatFs, "", 1);
    printf("f_mount result code = %d\r\n", mount_res);
    if(mount_res != FR_OK)
    {
      printf("f_mount FAILED!\r\n");
      Error_Handler();
    }
  printf("SD card mounted OK\r\n");

  AudioPlayer_ScanPlaylist();

  /* Infinite loop */
  for(;;)
  {
      uint8_t fillFirstHalf;
      uint8_t fillSecondHalf;
      int8_t trackDelta;
      int8_t volumeSteps;

      /*
       * Service DMA first. TouchGFX commands are intentionally handled only
       * after every pending audio half-buffer has been refilled.
       */
      taskENTER_CRITICAL();
      fillFirstHalf = HalfBufferNeedsFill;
      fillSecondHalf = FullBufferNeedsFill;
      HalfBufferNeedsFill = 0U;
      FullBufferNeedsFill = 0U;
      taskEXIT_CRITICAL();

      if (fillFirstHalf != 0U)
      {
          AudioPlayer_ServiceBuffer(&AudioBuffer[0]);
      }

      if (fillSecondHalf != 0U)
      {
          AudioPlayer_ServiceBuffer(
              &AudioBuffer[AUDIO_PLAYER_HALF_BUFFER_SIZE]
          );
      }

      taskENTER_CRITICAL();
      trackDelta = AudioTrackChangePending;
      AudioTrackChangePending = 0;
      volumeSteps = AudioVolumeChangePending;
      AudioVolumeChangePending = 0;
      taskEXIT_CRITICAL();

      /* Manual track changes have priority over automatic next. */
      if (trackDelta != 0)
      {
          AudioTrackFinished = 0U;
          AudioPlayer_ChangeTrack(trackDelta, "TRACK CHANGE");
      }
      else if (AudioTrackFinished != 0U)
      {
          AudioTrackFinished = 0U;
          AudioPlayer_ChangeTrack(1, "AUTO NEXT");
      }

      if (AudioPlayPauseRequested != 0U)
      {
          AudioPlayPauseRequested = 0U;

          if (PlayerState == PLAYER_STOPPED)
          {
              AudioPlayer_ResetDeadlineMisses();
              AudioPlayer_StartSelectedTrack("PLAY");
          }
          else if (PlayerState == PLAYER_PLAYING)
          {
              BSP_AUDIO_OUT_Pause();
              AudioPlayer_ClearTransferFlags();
              PlayerState = PLAYER_PAUSED;

              /* Print only after DMA playback is paused. */
              AudioBenchmark_Report();
              AudioPlayer_ReportRuntimeDiagnostics();
              printf("Player state = PAUSED\r\n");
          }
          else if (PlayerState == PLAYER_PAUSED)
          {
              /* Finish UART output before DMA playback resumes. */
              printf("Player state = PLAYING\r\n");

              AudioPlayer_ClearTransferFlags();
              AudioPlayer_ResetDeadlineMisses();
              AudioBenchmark_Reset();
              BSP_AUDIO_OUT_Resume();
              PlayerState = PLAYER_PLAYING;
          }
      }

      if (volumeSteps != 0)
      {
          int newVolume =
              (int)AudioVolume + ((int)volumeSteps * 5);

          if (newVolume < 0)
          {
              newVolume = 0;
          }
          else if (newVolume > 100)
          {
              newVolume = 100;
          }

          AudioVolume = (uint8_t)newVolume;

          if (PlayerState != PLAYER_STOPPED)
          {
              BSP_AUDIO_OUT_SetVolume(AudioVolume);
          }

          /* Do not use blocking UART while the DMA stream is active. */
      }

      osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void AudioPlayer_ClearTransferFlags(void)
{
    taskENTER_CRITICAL();
    HalfBufferNeedsFill = 0U;
    FullBufferNeedsFill = 0U;
    AudioTrackFinished = 0U;
    taskEXIT_CRITICAL();
}

static uint8_t AudioPlayer_IsWavFile(const char *filename)
{
    size_t length = strlen(filename);

    if (length < 4U)
    {
        return 0U;
    }

    const char *extension = &filename[length - 4U];

    return (extension[0] == '.') &&
           ((extension[1] == 'w') || (extension[1] == 'W')) &&
           ((extension[2] == 'a') || (extension[2] == 'A')) &&
           ((extension[3] == 'v') || (extension[3] == 'V'));
}

static void AudioPlayer_SortPlaylist(void)
{
    char temporary[PLAYLIST_FILENAME_SIZE];

    for (uint16_t i = 1U; i < trackCount; i++)
    {
        uint16_t position = i;
        memcpy(temporary, playlist[i], sizeof(temporary));

        while ((position > 0U) &&
               (strcmp(playlist[position - 1U], temporary) > 0))
        {
            memcpy(
                playlist[position],
                playlist[position - 1U],
                sizeof(playlist[position])
            );
            position--;
        }

        memcpy(playlist[position], temporary, sizeof(playlist[position]));
    }
}

static uint8_t AudioPlayer_ScanPlaylist(void)
{
    DIR directory;
    FILINFO fileInfo;
    FRESULT result;

    trackCount = 0U;
    currentTrack = 0;

    result = f_opendir(&directory, "/");

    if (result != FR_OK)
    {
        printf("Playlist scan failed: %d\r\n", result);
        return 0U;
    }

    while (trackCount < PLAYLIST_MAX_TRACKS)
    {
        result = f_readdir(&directory, &fileInfo);

        if ((result != FR_OK) || (fileInfo.fname[0] == '\0'))
        {
            break;
        }

        if (((fileInfo.fattrib & AM_DIR) != 0U) ||
            (AudioPlayer_IsWavFile(fileInfo.fname) == 0U))
        {
            continue;
        }

        size_t filenameLength = strlen(fileInfo.fname);

        if (filenameLength >= PLAYLIST_FILENAME_SIZE)
        {
            printf("Skipping long filename: %s\r\n", fileInfo.fname);
            continue;
        }

        memcpy(
            playlist[trackCount],
            fileInfo.fname,
            filenameLength + 1U
        );
        trackCount++;
    }

    f_closedir(&directory);

    if (result != FR_OK)
    {
        printf("Playlist scan failed: %d\r\n", result);
        trackCount = 0U;
        return 0U;
    }

    AudioPlayer_SortPlaylist();
    printf("Playlist: %u WAV file(s) found\r\n", (unsigned int)trackCount);

    for (uint16_t i = 0U; i < trackCount; i++)
    {
        printf("  %u: %s\r\n", (unsigned int)(i + 1U), playlist[i]);
    }

    return (trackCount > 0U) ? 1U : 0U;
}

static void AudioPlayer_ServiceBuffer(uint8_t *buffer)
{
    WavPlayer_FillHalf(buffer);

    if (AudioPlaying == 0U)
    {
        return;
    }

    AudioPipeline_Process(buffer);

    /*
     * Spectrum data is displayed much slower than the audio block rate.
     * Analyze every second block to leave more CPU time for TouchGFX.
     */
    spectrumDecimationCounter++;

    if (spectrumDecimationCounter >= 2U)
    {
        spectrumDecimationCounter = 0U;
        AudioFFT_Process(buffer);
    }
}

static void AudioPlayer_ReportRuntimeDiagnostics(void)
{
    UBaseType_t audioStackFreeWords =
        uxTaskGetStackHighWaterMark((TaskHandle_t)defaultTaskHandle);

    UBaseType_t touchGFXStackFreeWords =
        uxTaskGetStackHighWaterMark((TaskHandle_t)TouchGFXTaskHandle);

    size_t freeHeapBytes = xPortGetFreeHeapSize();
    size_t minimumFreeHeapBytes = xPortGetMinimumEverFreeHeapSize();

    printf("\r\nRuntime memory diagnostics:\r\n");
    printf(
        "Audio stack min free    = %lu words (%lu bytes)\r\n",
        (unsigned long)audioStackFreeWords,
        (unsigned long)(audioStackFreeWords * sizeof(StackType_t))
    );
    printf(
        "TouchGFX stack min free = %lu words (%lu bytes)\r\n",
        (unsigned long)touchGFXStackFreeWords,
        (unsigned long)(touchGFXStackFreeWords * sizeof(StackType_t))
    );
    printf(
        "Heap free now           = %lu bytes\r\n",
        (unsigned long)freeHeapBytes
    );
    printf(
        "Heap minimum ever free  = %lu bytes\r\n",
        (unsigned long)minimumFreeHeapBytes
    );
    printf(
        "Audio deadline misses   = %lu\r\n",
        (unsigned long)AudioPlayer_GetDeadlineMisses()
    );
}

static uint8_t AudioPlayer_StartSelectedTrack(const char *reason)
{
    if (trackCount == 0U)
    {
        printf("No WAV files found on SD card\r\n");
        PlayerState = PLAYER_STOPPED;
        return 0U;
    }

    AudioPlayer_ClearTransferFlags();
    spectrumDecimationCounter = 0U;

    printf("%s -> %s\r\n", reason, playlist[currentTrack]);

    if (WavPlayer_Start(playlist[currentTrack]))
    {
        PlayerState = PLAYER_PLAYING;
        return 1;
    }

    PlayerState = PLAYER_STOPPED;
    printf("Playback start failed: %s\r\n", playlist[currentTrack]);
    return 0;
}

static uint8_t AudioPlayer_ChangeTrack(int8_t delta, const char *reason)
{
    if (trackCount == 0U)
    {
        printf("Track change ignored: playlist is empty\r\n");
        return 0U;
    }

    PlayerState = PLAYER_STOPPED;

    /*
     * Always stop DMA. At EOF AudioPlaying is already zero, but the previous
     * DMA stream can still be active until explicitly stopped.
     */
    WavPlayer_Stop();

    /* Let DMA and codec stop before starting the next track. */
    osDelay(15);
    AudioPlayer_ClearTransferFlags();

    int nextTrack = currentTrack + (int)delta;
    int availableTracks = (int)trackCount;

    nextTrack %= availableTracks;

    if (nextTrack < 0)
    {
        nextTrack += trackCount;
    }

    currentTrack = nextTrack;

    if (AudioPlayer_StartSelectedTrack(reason))
    {
        return 1;
    }

    /* Retry once after a transient SD or codec failure. */
    osDelay(30);
    AudioPlayer_ClearTransferFlags();

    return AudioPlayer_StartSelectedTrack("RETRY");
}

void AudioPlayer_RequestPlayPause(void)
{
    AudioPlayPauseRequested = 1;
}

void AudioPlayer_RequestNext(void)
{
    taskENTER_CRITICAL();

    if (AudioTrackChangePending < (int8_t)PLAYLIST_MAX_TRACKS)
    {
        AudioTrackChangePending++;
    }

    taskEXIT_CRITICAL();
}

void AudioPlayer_RequestPrevious(void)
{
    taskENTER_CRITICAL();

    if (AudioTrackChangePending > -(int8_t)PLAYLIST_MAX_TRACKS)
    {
        AudioTrackChangePending--;
    }

    taskEXIT_CRITICAL();
}

void AudioPlayer_RequestVolumeUp(void)
{
    taskENTER_CRITICAL();

    /* Coalesce repeated GUI events; one processed press equals one 5% step. */
    AudioVolumeChangePending = 1;

    taskEXIT_CRITICAL();
}

void AudioPlayer_RequestVolumeDown(void)
{
    taskENTER_CRITICAL();

    /* Coalesce repeated GUI events; one processed press equals one 5% step. */
    AudioVolumeChangePending = -1;

    taskEXIT_CRITICAL();
}

/* USER CODE END Application */
