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
#include "arm_math.h"
#include <math.h>

extern uint8_t WavPlayer_Start(const char *filename);
extern void WavPlayer_FillHalf(uint8_t *half);
extern void WavPlayer_Stop(void);
extern FATFS SDFatFs;
extern SD_HandleTypeDef hsd1;
extern volatile uint8_t AudioPlaying;
extern volatile uint8_t HalfBufferNeedsFill;
extern volatile uint8_t FullBufferNeedsFill;
extern volatile uint8_t AudioTrackFinished;
extern uint8_t AudioBuffer[];
extern void AudioFFT_Process(uint8_t *audioData);

#define AUDIO_HALF_BUFFER 4096
#define FFT_SIZE      1024
#define SAMPLE_RATE   48000.0f
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
    "three.wav"
};

#define TRACK_COUNT (sizeof(playlist) / sizeof(playlist[0]))

volatile int currentTrack = 0;

volatile uint8_t AudioNextRequested = 0;
volatile uint8_t AudioPreviousRequested = 0;

arm_rfft_fast_instance_f32 FFT_Instance;

float FFT_Input[FFT_SIZE];
float FFT_Output[FFT_SIZE];
float FFT_Magnitude[FFT_SIZE / 2];

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
osThreadId TouchGFXTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void FFT_Test(void);
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
extern void TouchGFX_Task(void const * argument);

extern void MX_USB_HOST_Init(void);
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
  MX_USB_HOST_Init();
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
  FFT_Test();

//  if (!WavPlayer_Start("one.wav"))
//  {
//    printf("WavPlayer_Start FAILED\r\n");
//    Error_Handler();
//  }

  /* Infinite loop */
  for(;;)
  {
	  if (AudioPlayPauseRequested)
	  {
	      AudioPlayPauseRequested = 0;

	      if (PlayerState == PLAYER_STOPPED)
	      {
	          printf("PLAY requested\r\n");
	          printf("Track: %s\r\n", playlist[currentTrack]);

	          if (WavPlayer_Start(playlist[currentTrack]))
	          {
	              PlayerState = PLAYER_PLAYING;

	              printf("Player state = PLAYING\r\n");
	          }
	          else
	          {
	              printf("WavPlayer_Start FAILED\r\n");
	          }
	      }
	      else if (PlayerState == PLAYER_PLAYING)
	      {
	          BSP_AUDIO_OUT_Pause();

	          PlayerState = PLAYER_PAUSED;

	          printf("Player state = PAUSED\r\n");
	      }
	      else if (PlayerState == PLAYER_PAUSED)
	      {
	          BSP_AUDIO_OUT_Resume();

	          PlayerState = PLAYER_PLAYING;

	          printf("Player state = PLAYING\r\n");
	      }
	  }

	  if (AudioTrackFinished)
	  {
	      AudioTrackFinished = 0;

	      PlayerState = PLAYER_STOPPED;

	      currentTrack++;

	      if (currentTrack >= TRACK_COUNT)
	      {
	          currentTrack = 0;
	      }

	      printf("AUTO NEXT -> %s\r\n", playlist[currentTrack]);

	      if (WavPlayer_Start(playlist[currentTrack]))
	      {
	          PlayerState = PLAYER_PLAYING;
	      }
	      else
	      {
	          printf("Auto next playback failed\r\n");
	      }
	  }

	  if (AudioNextRequested)
	  {
	      AudioNextRequested = 0;

	      WavPlayer_Stop();

	      currentTrack++;

	      if (currentTrack >= TRACK_COUNT)
	      {
	          currentTrack = 0;
	      }

	      printf("NEXT -> %s\r\n", playlist[currentTrack]);

	      if (WavPlayer_Start(playlist[currentTrack]))
	      {
	          PlayerState = PLAYER_PLAYING;
	      }
	  }

	  if (AudioPreviousRequested)
	  {
	      AudioPreviousRequested = 0;

	      WavPlayer_Stop();

	      currentTrack--;

	      if (currentTrack < 0)
	      {
	          currentTrack = TRACK_COUNT - 1;
	      }

	      printf("PREVIOUS -> %s\r\n", playlist[currentTrack]);

	      if (WavPlayer_Start(playlist[currentTrack]))
	      {
	          PlayerState = PLAYER_PLAYING;
	      }
	  }

	  if (HalfBufferNeedsFill)
	  {
	      HalfBufferNeedsFill = 0;

	      WavPlayer_FillHalf(&AudioBuffer[0]);

	      if (AudioPlaying)
	      {
	          AudioFFT_Process(&AudioBuffer[0]);
	      }
	  }

	  if (FullBufferNeedsFill)
	  {
	      FullBufferNeedsFill = 0;

	      WavPlayer_FillHalf(&AudioBuffer[AUDIO_HALF_BUFFER]);

	      if (AudioPlaying)
	      {
	          AudioFFT_Process(&AudioBuffer[AUDIO_HALF_BUFFER]);
	      }
	  }

      osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

void AudioPlayer_RequestPlay(void)
{
    AudioPlayPauseRequested = 1;
}

void AudioPlayer_RequestPause(void)
{
    AudioPlayPauseRequested = 1;
}

void AudioPlayer_RequestNext(void)
{
    AudioNextRequested = 1;
}

void AudioPlayer_RequestPrevious(void)
{
    AudioPreviousRequested = 1;
}

void FFT_Test(void)
{
    arm_status status;

    status = arm_rfft_fast_init_1024_f32(&FFT_Instance);

    if (status != ARM_MATH_SUCCESS)
    {
        printf("FFT init failed\r\n");
        return;
    }

    const float testFrequency = 1000.0f;

    for (uint32_t n = 0; n < FFT_SIZE; n++)
    {
    	float sample =
    	    sinf(
    	        2.0f *
    	        PI *
    	        testFrequency *
    	        (float)n /
    	        SAMPLE_RATE
    	    );

    	float window =
    	    0.5f *
    	    (1.0f -
    	     cosf(
    	         2.0f *
    	         PI *
    	         (float)n /
    	         (float)(FFT_SIZE - 1)
    	     ));

    	FFT_Input[n] = sample * window;
    }

    arm_rfft_fast_f32(
        &FFT_Instance,
        FFT_Input,
        FFT_Output,
        0   // 0 for fft and 1 for ifft
    );

    float maxMagnitude = 0.0f;
    uint32_t maxBin = 0;

    for (uint32_t k = 1; k < FFT_SIZE / 2; k++)
    {
        float real = FFT_Output[2 * k];
        float imag = FFT_Output[2 * k + 1];

        float magnitude =
            sqrtf(real * real + imag * imag);

        FFT_Magnitude[k] = magnitude;

        if (magnitude > maxMagnitude)
        {
            maxMagnitude = magnitude;
            maxBin = k;
        }
    }
    for (uint32_t k = 18; k <= 24; k++)
    {
        printf("Bin %lu: %.3f\r\n",
               (unsigned long)k,
               FFT_Magnitude[k]);
    }

    float peakFrequency =
        ((float)maxBin * SAMPLE_RATE) /
        (float)FFT_SIZE;

    printf("FFT TEST\r\n");
    printf("Peak bin = %lu\r\n", (unsigned long)maxBin);
    printf("Peak frequency = %.3f Hz\r\n", peakFrequency);
}
/* USER CODE END Application */

