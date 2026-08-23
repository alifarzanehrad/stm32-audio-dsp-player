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

const char *playlist[] =
{
    "one.wav",
    "two.wav",
    "three.wav",
    "four.wav",
    "five.wav",
    "six.wav"
};

#define TRACK_COUNT (sizeof(playlist) / sizeof(playlist[0]))

volatile int currentTrack = 0;

volatile int8_t AudioTrackChangePending = 0;
volatile int8_t AudioVolumeChangePending = 0;

volatile uint8_t AudioVolume = 70;

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId TouchGFXTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
static void AudioPlayer_ClearTransferFlags(void);
static uint8_t AudioPlayer_StartSelectedTrack(const char *reason);
static uint8_t AudioPlayer_ChangeTrack(int8_t delta, const char *reason);
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
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 4096);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of TouchGFXTask */
  osThreadDef(TouchGFXTask, TouchGFX_Task, osPriorityNormal, 0, 4096);
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

//  if (!WavPlayer_Start("one.wav"))
//  {
//    printf("WavPlayer_Start FAILED\r\n");
//    Error_Handler();
//  }

  /* Infinite loop */
  for(;;)
  {
      int8_t trackDelta;
      int8_t volumeSteps;

      taskENTER_CRITICAL();
      trackDelta = AudioTrackChangePending;
      AudioTrackChangePending = 0;
      volumeSteps = AudioVolumeChangePending;
      AudioVolumeChangePending = 0;
      taskEXIT_CRITICAL();

      /* Manual track changes have priority over automatic next. */
      if (trackDelta != 0)
      {
          AudioTrackFinished = 0;
          AudioPlayer_ChangeTrack(trackDelta, "TRACK CHANGE");
      }
      else if (AudioTrackFinished)
      {
          AudioTrackFinished = 0;
          AudioPlayer_ChangeTrack(1, "AUTO NEXT");
      }

      if (AudioPlayPauseRequested)
      {
          AudioPlayPauseRequested = 0;

          if (PlayerState == PLAYER_STOPPED)
          {
              AudioPlayer_StartSelectedTrack("PLAY");
          }
          else if (PlayerState == PLAYER_PLAYING)
          {
              BSP_AUDIO_OUT_Pause();
              PlayerState = PLAYER_PAUSED;

              /* Print only after DMA playback is paused. */
              AudioBenchmark_Report();
              printf("Player state = PAUSED\r\n");
          }
          else if (PlayerState == PLAYER_PAUSED)
          {
              /* Finish UART output before DMA playback resumes. */
              printf("Player state = PLAYING\r\n");

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

          printf("Volume = %u\r\n", AudioVolume);
      }

	  if (HalfBufferNeedsFill)
	  {
	      HalfBufferNeedsFill = 0;

	      WavPlayer_FillHalf(&AudioBuffer[0]);

	      if (AudioPlaying)
	      {
	          AudioPipeline_Process(&AudioBuffer[0]);

	          AudioFFT_Process(&AudioBuffer[0]);
	      }
	  }

	  if (FullBufferNeedsFill)
	  {
	      FullBufferNeedsFill = 0;

	      WavPlayer_FillHalf(
	          &AudioBuffer[AUDIO_PLAYER_HALF_BUFFER_SIZE]
	      );

	      if (AudioPlaying)
	      {
	          AudioPipeline_Process(
	              &AudioBuffer[AUDIO_PLAYER_HALF_BUFFER_SIZE]
	          );

	          AudioFFT_Process(
	              &AudioBuffer[AUDIO_PLAYER_HALF_BUFFER_SIZE]
	          );
	      }
	  }

      osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

static void AudioPlayer_ClearTransferFlags(void)
{
    HalfBufferNeedsFill = 0;
    FullBufferNeedsFill = 0;
    AudioTrackFinished = 0;
}

static uint8_t AudioPlayer_StartSelectedTrack(const char *reason)
{
    AudioPlayer_ClearTransferFlags();

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
    PlayerState = PLAYER_STOPPED;

    if (AudioPlaying)
    {
        WavPlayer_Stop();
    }

    /* Let DMA and codec stop before starting the next track. */
    osDelay(15);
    AudioPlayer_ClearTransferFlags();

    int nextTrack = currentTrack + (int)delta;
    int trackCount = (int)TRACK_COUNT;

    nextTrack %= trackCount;

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

    if (AudioTrackChangePending < (int8_t)TRACK_COUNT)
    {
        AudioTrackChangePending++;
    }

    taskEXIT_CRITICAL();
}

void AudioPlayer_RequestPrevious(void)
{
    taskENTER_CRITICAL();

    if (AudioTrackChangePending > -(int8_t)TRACK_COUNT)
    {
        AudioTrackChangePending--;
    }

    taskEXIT_CRITICAL();
}

void AudioPlayer_RequestVolumeUp(void)
{
    taskENTER_CRITICAL();

    if (AudioVolumeChangePending < 20)
    {
        AudioVolumeChangePending++;
    }

    taskEXIT_CRITICAL();
}

void AudioPlayer_RequestVolumeDown(void)
{
    taskENTER_CRITICAL();

    if (AudioVolumeChangePending > -20)
    {
        AudioVolumeChangePending--;
    }

    taskEXIT_CRITICAL();
}

/* USER CODE END Application */

