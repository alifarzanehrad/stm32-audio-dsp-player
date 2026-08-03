/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "cmsis_os.h"
#include "adc.h"
#include "crc.h"
#include "dcmi.h"
#include "dma2d.h"
#include "eth.h"
#include "fatfs.h"
#include "i2c.h"
#include "ltdc.h"
#include "quadspi.h"
#include "rtc.h"
#include "sai.h"
#include "sdmmc.h"
#include "spdifrx.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_host.h"
#include "gpio.h"
#include "fmc.h"
#include "stm32746g_discovery_audio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ff.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE BEGIN PV */
FATFS SDFatFs;
uint8_t audio_status;
/* ساختار هدر WAV استاندارد (44 بایت) */
typedef struct
{
  char     RIFF[4];        // "RIFF"
  uint32_t ChunkSize;
  char     WAVE[4];        // "WAVE"
  char     fmt[4];         // "fmt "
  uint32_t Subchunk1Size;
  uint16_t AudioFormat;    // 1 = PCM
  uint16_t NumChannels;    // 1=mono, 2=stereo
  uint32_t SampleRate;
  uint32_t ByteRate;
  uint16_t BlockAlign;
  uint16_t BitsPerSample;
  char     Subchunk2ID[4]; // "data"
  uint32_t Subchunk2Size;  // اندازه دیتای صوتی به بایت
} WAV_HeaderTypeDef;

#define AUDIO_BUFFER_SIZE   8192   // کل بافر (به بایت)
#define AUDIO_HALF_BUFFER   (AUDIO_BUFFER_SIZE / 2)

FIL WavFile;
WAV_HeaderTypeDef WavHeader;
static uint8_t AudioBuffer[AUDIO_BUFFER_SIZE];
volatile uint32_t AudioRemainingBytes = 0;
volatile uint8_t  AudioPlaying = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
uint8_t WavPlayer_Start(const char *filename);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* Configure the peripherals common clocks */
  PeriphCommonClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC3_Init();
  MX_CRC_Init();
  MX_DCMI_Init();
  MX_DMA2D_Init();
  MX_ETH_Init();
  MX_FMC_Init();
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  MX_QUADSPI_Init();
  MX_RTC_Init();
  MX_SAI2_Init();
  MX_SDMMC1_SD_Init();
  MX_SPDIFRX_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM5_Init();
  MX_TIM8_Init();
  MX_TIM12_Init();
  MX_USART1_UART_Init();
  MX_USART6_UART_Init();
  MX_FATFS_Init();
  /* USER CODE BEGIN 2 */
    HAL_UART_Transmit(&huart1, (uint8_t*)"RAW TEST\r\n", 10, HAL_MAX_DELAY);

    setvbuf(stdout, NULL, _IONBF, 0);
    printf("Boot OK, entering main setup...\r\n");

    uint8_t sd_init_ret = BSP_SD_Init();
    printf("BSP_SD_Init returned = %d (MSD_OK=0)\r\n", sd_init_ret);

    if (sd_init_ret != MSD_OK)
    {
      printf("Retrying BSP_SD_Init...\r\n");
      HAL_Delay(200);
      sd_init_ret = BSP_SD_Init();
      printf("BSP_SD_Init retry returned = %d\r\n", sd_init_ret);
    }

    uint8_t card_state = BSP_SD_GetCardState();
    printf("BSP_SD_GetCardState = %d (SD_TRANSFER_OK=0)\r\n", card_state);

    printf("Mounting SD card...\r\n");
    FRESULT mount_res = f_mount(&SDFatFs, "", 1);
    printf("f_mount result code = %d\r\n", mount_res);
    if(mount_res != FR_OK)
    {
        printf("f_mount FAILED!\r\n");
        Error_Handler();
    }
    printf("SD card mounted OK\r\n");

    printf("Calling WavPlayer_Start...\r\n");
    if (!WavPlayer_Start("one.wav"))
    {
      printf("WavPlayer_Start FAILED, entering Error_Handler\r\n");
      Error_Handler();
    }
    /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  //MX_FREERTOS_Init();

  /* Start scheduler */
  //osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Activate the Over-Drive mode
  */
  if (HAL_PWREx_EnableOverDrive() != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_6) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief Peripherals Common Clock Configuration
  * @retval None
  */
void PeriphCommonClock_Config(void)
{
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Initializes the peripherals clock
  */
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_LTDC|RCC_PERIPHCLK_SAI2
                              |RCC_PERIPHCLK_SDMMC1|RCC_PERIPHCLK_CLK48;
  PeriphClkInitStruct.PLLSAI.PLLSAIN = 384;
  PeriphClkInitStruct.PLLSAI.PLLSAIR = 5;
  PeriphClkInitStruct.PLLSAI.PLLSAIQ = 2;
  PeriphClkInitStruct.PLLSAI.PLLSAIP = RCC_PLLSAIP_DIV8;
  PeriphClkInitStruct.PLLSAIDivQ = 1;
  PeriphClkInitStruct.PLLSAIDivR = RCC_PLLSAIDIVR_8;
  PeriphClkInitStruct.Sai2ClockSelection = RCC_SAI2CLKSOURCE_PLLSAI;
  PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLLSAIP;
  PeriphClkInitStruct.Sdmmc1ClockSelection = RCC_SDMMC1CLKSOURCE_CLK48;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

int __io_putchar(int ch)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
  return ch;
}

int _write(int file, char *ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}
uint8_t WavPlayer_Start(const char *filename)
{
  FRESULT res;
  UINT bytesread;

  printf("Opening file %s...\r\n", filename);
  res = f_open(&WavFile, filename, FA_READ);
  if (res != FR_OK)
  {
    printf("f_open failed, res=%d\r\n", res);
    return 0;
  }
  printf("File opened OK\r\n");

  res = f_read(&WavFile, &WavHeader, sizeof(WAV_HeaderTypeDef), &bytesread);
  if (res != FR_OK || bytesread != sizeof(WAV_HeaderTypeDef))
  {
    printf("Header read failed, res=%d, bytesread=%u\r\n", res, bytesread);
    f_close(&WavFile);
    return 0;
  }
  printf("Header read OK, RIFF=%.4s WAVE=%.4s fmt=%.4s dataTag=%.4s\r\n",
         WavHeader.RIFF, WavHeader.WAVE, WavHeader.fmt, WavHeader.Subchunk2ID);
  printf("AudioFormat=%u NumChannels=%u SampleRate=%lu BitsPerSample=%u DataSize=%lu\r\n",
         WavHeader.AudioFormat, WavHeader.NumChannels,
         (unsigned long)WavHeader.SampleRate, WavHeader.BitsPerSample,
         (unsigned long)WavHeader.Subchunk2Size);

  if (strncmp(WavHeader.RIFF, "RIFF", 4) != 0 ||
      strncmp(WavHeader.WAVE, "WAVE", 4) != 0 ||
      WavHeader.AudioFormat != 1)
  {
    printf("Invalid WAV file!\r\n");
    f_close(&WavFile);
    return 0;
  }

  AudioRemainingBytes = WavHeader.Subchunk2Size;

  audio_status = BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, 70, WavHeader.SampleRate);
  if (audio_status != AUDIO_OK)
  {
    printf("BSP_AUDIO_OUT_Init failed, status=%u\r\n", audio_status);
    f_close(&WavFile);
    return 0;
  }
  printf("Audio codec init OK\r\n");

  f_read(&WavFile, AudioBuffer, AUDIO_BUFFER_SIZE, &bytesread);
  if (bytesread < AUDIO_BUFFER_SIZE)
  {
    memset(&AudioBuffer[bytesread], 0, AUDIO_BUFFER_SIZE - bytesread);
  }
  AudioRemainingBytes -= bytesread;
  printf("Initial buffer filled, bytesread=%u\r\n", bytesread);

  AudioPlaying = 1;

  audio_status = BSP_AUDIO_OUT_Play((uint16_t *)AudioBuffer, AUDIO_BUFFER_SIZE);
  if (audio_status != AUDIO_OK)
  {
    printf("BSP_AUDIO_OUT_Play failed, status=%u\r\n", audio_status);
    AudioPlaying = 0;
    f_close(&WavFile);
    return 0;
  }
  printf("Playback started!\r\n");

  return 1;
}

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM6 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM6) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {

  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
