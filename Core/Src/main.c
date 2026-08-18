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
#include "dma.h"
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
#include "app_touchgfx.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32746g_discovery_audio.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ff.h"
#include "arm_math.h"
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

#define AUDIO_BUFFER_SIZE   8192
#define AUDIO_HALF_BUFFER   (AUDIO_BUFFER_SIZE / 2)
FIL WavFile;
uint8_t AudioBuffer[AUDIO_BUFFER_SIZE];
volatile uint32_t AudioRemainingBytes = 0;
volatile uint8_t  AudioPlaying = 0;
volatile uint8_t  HalfBufferNeedsFill = 0;
volatile uint8_t  FullBufferNeedsFill = 0;
volatile uint8_t AudioTrackFinished = 0;
volatile uint8_t EQEnabled = 1;

#define FFT_SIZE 1024
#define FFT_BANDS 16
#define EQ_BLOCK_SIZE 1024
#define EQ_BAND_COUNT 5
#define BIQUAD_COEFFS_PER_STAGE 5
#define BIQUAD_STATE_PER_STAGE 4
#define EQ_MAX_HEADROOM_DB 6.0f
#define EQ_LIMITER_THRESHOLD 32000.0f
#define EQ_LIMITER_RELEASE 0.05f

float32_t fftBands[FFT_BANDS];
float32_t fftBandsSmoothed[FFT_BANDS];
float32_t fftInput[FFT_SIZE];
float32_t fftOutput[FFT_SIZE];
float32_t fftMagnitude[FFT_SIZE / 2];
float32_t hannWindow[FFT_SIZE];

static const float32_t eqBandFrequencies[EQ_BAND_COUNT] =
{
    100.0f,
    300.0f,
    1000.0f,
    3000.0f,
    8000.0f
};

float32_t eqBandGainsDB[EQ_BAND_COUNT] =
{
    0.0f,   // 100 Hz
    0.0f,   // 300 Hz
    0.0f,   // 1 kHz
    0.0f,   // 3 kHz
    0.0f    // 8 kHz
};

volatile float32_t eqRequestedGainsDB[EQ_BAND_COUNT] =
{
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
volatile uint8_t eqUpdatePending = 0;
float32_t eqSampleRate = 48000.0f;
float32_t eqPreampGain = 1.0f;
float32_t eqLimiterGain = 1.0f;

float32_t eqCoeffs[EQ_BAND_COUNT * BIQUAD_COEFFS_PER_STAGE];
float32_t eqStateLeft[EQ_BAND_COUNT * BIQUAD_STATE_PER_STAGE];
float32_t eqStateRight[EQ_BAND_COUNT * BIQUAD_STATE_PER_STAGE];
float32_t eqLeftBuffer[EQ_BLOCK_SIZE];
float32_t eqRightBuffer[EQ_BLOCK_SIZE];

arm_rfft_fast_instance_f32 fftInstance;
arm_biquad_casd_df1_inst_f32 eqLeft;
arm_biquad_casd_df1_inst_f32 eqRight;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void PeriphCommonClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram);

uint8_t WavPlayer_Start(const char *filename);
void WavPlayer_FillHalf(uint8_t *half);

void AudioFFT_Process(uint8_t *audioData);

void AudioEQ_Init(float32_t sampleRate);
void AudioEQ_Process(uint8_t *audioData);
void AudioEQ_SetBandGain(uint8_t band, float32_t gainDB);
static void AudioEQ_CalculateCoefficients(void);
static void AudioEQ_UpdatePreampGain(void);
static void AudioEQ_ApplyPendingGains(void);
static void AudioEQ_ApplyLimiter(void);
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
  MX_DMA_Init();
  MX_ADC3_Init();
  MX_CRC_Init();
  MX_DCMI_Init();
  MX_DMA2D_Init();
  MX_ETH_Init();
  MX_FMC_Init();
  SDRAM_Initialization_Sequence(&hsdram1);
  MX_I2C1_Init();
  MX_I2C3_Init();
  MX_LTDC_Init();
  HAL_GPIO_WritePin(GPIOI, GPIO_PIN_12, GPIO_PIN_SET); // LCD_DISP
  HAL_GPIO_WritePin(GPIOK, GPIO_PIN_3, GPIO_PIN_SET);  // Backlight

  SCB_CleanDCache_by_Addr((uint32_t *)0xC0000000, 480 * 272 * 2);
  __DSB();
  __ISB();

  HAL_LTDC_SetAddress(&hltdc, 0xC0000000, LTDC_LAYER_1);
  HAL_LTDC_Reload(&hltdc, LTDC_RELOAD_IMMEDIATE);
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
  MX_TouchGFX_Init();
  /* Call PreOsInit function */
  MX_TouchGFX_PreOSInit();
  /* USER CODE BEGIN 2 */
  if (arm_rfft_fast_init_f32(&fftInstance, FFT_SIZE) != ARM_MATH_SUCCESS)
  {
      printf("FFT init failed\r\n");
      Error_Handler();
  }
  for (uint32_t i = 0; i < FFT_SIZE; i++)
  {
      hannWindow[i] =
          0.5f *
          (1.0f -
           cosf((2.0f * PI * i) / (FFT_SIZE - 1)));
  }
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

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
static void SDRAM_Initialization_Sequence(SDRAM_HandleTypeDef *hsdram)
{
    FMC_SDRAM_CommandTypeDef Command;
    uint32_t modeRegister;

    /* Step 1: Enable SDRAM clock */
    Command.CommandMode = FMC_SDRAM_CMD_CLK_ENABLE;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0xFFFF) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 2: At least 100 us delay */
    HAL_Delay(1);

    /* Step 3: Precharge all */
    Command.CommandMode = FMC_SDRAM_CMD_PALL;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0xFFFF) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 4: Auto refresh */
    Command.CommandMode = FMC_SDRAM_CMD_AUTOREFRESH_MODE;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 8;
    Command.ModeRegisterDefinition = 0;

    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0xFFFF) != HAL_OK)
    {
        Error_Handler();
    }

    /*
     * Step 5: Load Mode Register
     * CubeMX FMC setting = CAS Latency 3
     */
    modeRegister =
          0x0000U       /* Burst length 1 */
        | 0x0000U       /* Sequential */
        | 0x0030U       /* CAS latency 3 */
        | 0x0000U       /* Standard operating mode */
        | 0x0200U;      /* Single write burst */

    Command.CommandMode = FMC_SDRAM_CMD_LOAD_MODE;
    Command.CommandTarget = FMC_SDRAM_CMD_TARGET_BANK1;
    Command.AutoRefreshNumber = 1;
    Command.ModeRegisterDefinition = modeRegister;

    if (HAL_SDRAM_SendCommand(hsdram, &Command, 0xFFFF) != HAL_OK)
    {
        Error_Handler();
    }

    /* Step 6: Refresh rate for 100 MHz SDRAM clock */
    if (HAL_SDRAM_ProgramRefreshRate(hsdram, 0x0603) != HAL_OK)
    {
        Error_Handler();
    }
}

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

void WavPlayer_FillHalf(uint8_t *half)
{
  UINT bytesread = 0;

  if (!AudioPlaying)
    return;

  if (AudioRemainingBytes > 0)
  {
    UINT toRead = (AudioRemainingBytes < AUDIO_HALF_BUFFER) ? AudioRemainingBytes : AUDIO_HALF_BUFFER;
    f_read(&WavFile, half, toRead, &bytesread);
    AudioRemainingBytes -= bytesread;

    if (bytesread < AUDIO_HALF_BUFFER)
    {
      memset(&half[bytesread], 0, AUDIO_HALF_BUFFER - bytesread);
    }
  }
  else
  {
      memset(half, 0, AUDIO_HALF_BUFFER);

      AudioPlaying = 0;
      HalfBufferNeedsFill = 0;
      FullBufferNeedsFill = 0;

      f_close(&WavFile);

      AudioTrackFinished = 1;

      printf("Playback finished!\r\n");
  }
}

void WavPlayer_Stop(void)
{
    if (AudioPlaying)
    {
        BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);

        AudioPlaying = 0;

        HalfBufferNeedsFill = 0;
        FullBufferNeedsFill = 0;

        f_close(&WavFile);

        printf("Playback stopped\r\n");
    }
}

void BSP_AUDIO_OUT_HalfTransfer_CallBack(void)
{
  HalfBufferNeedsFill = 1;
}

void BSP_AUDIO_OUT_TransferComplete_CallBack(void)
{
  FullBufferNeedsFill = 1;
}

uint8_t WavPlayer_Start(const char *filename)
{
  FRESULT res;
  UINT bytesread;
  uint8_t audio_status;
  char chunkId[4];
  uint32_t chunkSize;

  printf("Opening file %s...\r\n", filename);
  res = f_open(&WavFile, filename, FA_READ);
  if (res != FR_OK)
  {
    printf("f_open failed, res=%d\r\n", res);
    return 0;
  }
  printf("File opened OK\r\n");

  /* Read RIFF header (12 bytes total: "RIFF" + size(4) + "WAVE") */
  char riff[4], wave[4];
  uint32_t riffSize;
  f_read(&WavFile, riff, 4, &bytesread);
  f_read(&WavFile, &riffSize, 4, &bytesread);
  f_read(&WavFile, wave, 4, &bytesread);

  if (strncmp(riff, "RIFF", 4) != 0 || strncmp(wave, "WAVE", 4) != 0)
  {
    printf("Invalid WAV file (bad RIFF/WAVE header)!\r\n");
    f_close(&WavFile);
    return 0;
  }
  printf("RIFF/WAVE header OK\r\n");

  uint8_t foundFmt = 0, foundData = 0;
  uint16_t audioFormat = 0, numChannels = 0, bitsPerSample = 0, blockAlign = 0;
  uint32_t sampleRate = 0, byteRate = 0;
  uint32_t dataSize = 0;

  /* Walk chunks until we find both "fmt " and "data" */
  while (!foundData)
  {
    res = f_read(&WavFile, chunkId, 4, &bytesread);
    if (res != FR_OK || bytesread != 4)
    {
      printf("Failed to read chunk id (EOF before data chunk found)\r\n");
      f_close(&WavFile);
      return 0;
    }

    res = f_read(&WavFile, &chunkSize, 4, &bytesread);
    if (res != FR_OK || bytesread != 4)
    {
      printf("Failed to read chunk size\r\n");
      f_close(&WavFile);
      return 0;
    }

    printf("Found chunk '%.4s' size=%lu\r\n", chunkId, (unsigned long)chunkSize);

    if (strncmp(chunkId, "fmt ", 4) == 0)
    {
      FSIZE_t fmtChunkStart = f_tell(&WavFile);

      f_read(&WavFile, &audioFormat, 2, &bytesread);
      f_read(&WavFile, &numChannels, 2, &bytesread);
      f_read(&WavFile, &sampleRate, 4, &bytesread);
      f_read(&WavFile, &byteRate, 4, &bytesread);
      f_read(&WavFile, &blockAlign, 2, &bytesread);
      f_read(&WavFile, &bitsPerSample, 2, &bytesread);

      f_lseek(&WavFile, fmtChunkStart + chunkSize);
      foundFmt = 1;
    }
    else if (strncmp(chunkId, "data", 4) == 0)
    {
      dataSize = chunkSize;
      foundData = 1;
    }
    else
    {
      FSIZE_t currentPos = f_tell(&WavFile);
      f_lseek(&WavFile, currentPos + chunkSize);
    }
  }

  if (!foundFmt)
  {
    printf("No fmt chunk found!\r\n");
    f_close(&WavFile);
    return 0;
  }

  printf("AudioFormat=%u NumChannels=%u SampleRate=%lu BitsPerSample=%u DataSize=%lu\r\n",
         audioFormat, numChannels, (unsigned long)sampleRate, bitsPerSample, (unsigned long)dataSize);

  if (audioFormat != 1)
  {
    printf("Not PCM format!\r\n");
    f_close(&WavFile);
    return 0;
  }

  AudioRemainingBytes = dataSize;
  AudioEQ_Init((float32_t)sampleRate);
  printf("Calling BSP_AUDIO_OUT_Init with sampleRate=%lu\r\n", (unsigned long)sampleRate);
  audio_status = BSP_AUDIO_OUT_Init(OUTPUT_DEVICE_HEADPHONE, 70, sampleRate);
  if (audio_status != AUDIO_OK)
  {
    printf("BSP_AUDIO_OUT_Init failed, status=%u\r\n", audio_status);
    f_close(&WavFile);
    return 0;
  }
  BSP_AUDIO_OUT_SetAudioFrameSlot(CODEC_AUDIOFRAME_SLOT_02);
  printf("Audio codec init OK\r\n");

  f_read(&WavFile, AudioBuffer, AUDIO_BUFFER_SIZE, &bytesread);
  if (bytesread < AUDIO_BUFFER_SIZE)
  {
    memset(&AudioBuffer[bytesread], 0, AUDIO_BUFFER_SIZE - bytesread);
  }
  AudioRemainingBytes -= bytesread;

  AudioEQ_Process(&AudioBuffer[0]);

  AudioEQ_Process(
      &AudioBuffer[AUDIO_HALF_BUFFER]
  );

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

static void AudioEQ_CalculateCoefficients(void)
{
    const float32_t Q = 1.0f;

    for (uint32_t band = 0; band < EQ_BAND_COUNT; band++)
    {
        float32_t frequency = eqBandFrequencies[band];
        float32_t gainDB = eqBandGainsDB[band];
        float32_t A = powf(10.0f, gainDB / 40.0f);
        float32_t omega = 2.0f * PI * frequency / eqSampleRate;
        float32_t alpha = sinf(omega) / (2.0f * Q);
        float32_t cosOmega = cosf(omega);

        float32_t b0 = 1.0f + alpha * A;
        float32_t b1 = -2.0f * cosOmega;
        float32_t b2 = 1.0f - alpha * A;
        float32_t a0 = 1.0f + alpha / A;
        float32_t a1 = -2.0f * cosOmega;
        float32_t a2 = 1.0f - alpha / A;

        uint32_t coeffIndex = band * BIQUAD_COEFFS_PER_STAGE;

        eqCoeffs[coeffIndex] = b0 / a0;
        eqCoeffs[coeffIndex + 1] = b1 / a0;
        eqCoeffs[coeffIndex + 2] = b2 / a0;

        /* CMSIS-DSP DF1 expects the feedback coefficients with inverted signs. */
        eqCoeffs[coeffIndex + 3] = -(a1 / a0);
        eqCoeffs[coeffIndex + 4] = -(a2 / a0);
    }
}

static void AudioEQ_UpdatePreampGain(void)
{
    float32_t maxBoostDB = 0.0f;

    for (uint32_t band = 0; band < EQ_BAND_COUNT; band++)
    {
        if (eqBandGainsDB[band] > maxBoostDB)
        {
            maxBoostDB = eqBandGainsDB[band];
        }
    }

    float32_t headroomDB = maxBoostDB;

    if (headroomDB > EQ_MAX_HEADROOM_DB)
    {
        headroomDB = EQ_MAX_HEADROOM_DB;
    }

    eqPreampGain = powf(10.0f, -headroomDB / 20.0f);
}

void AudioEQ_Init(float32_t sampleRate)
{
    eqSampleRate = sampleRate;

    for (uint32_t band = 0; band < EQ_BAND_COUNT; band++)
    {
        eqBandGainsDB[band] = eqRequestedGainsDB[band];
    }

    eqUpdatePending = 0;
    eqLimiterGain = 1.0f;
    AudioEQ_UpdatePreampGain();
    AudioEQ_CalculateCoefficients();

    arm_biquad_cascade_df1_init_f32(
        &eqLeft,
        EQ_BAND_COUNT,
        eqCoeffs,
        eqStateLeft
    );

    arm_biquad_cascade_df1_init_f32(
        &eqRight,
        EQ_BAND_COUNT,
        eqCoeffs,
        eqStateRight
    );
}

void AudioEQ_SetBandGain(uint8_t band, float32_t gainDB)
{
    if (band >= EQ_BAND_COUNT)
    {
        return;
    }

    if (gainDB < -12.0f)
    {
        gainDB = -12.0f;
    }
    else if (gainDB > 12.0f)
    {
        gainDB = 12.0f;
    }

    eqRequestedGainsDB[band] = gainDB;
    eqUpdatePending = 1;
}

static void AudioEQ_ApplyPendingGains(void)
{
    if (!eqUpdatePending)
    {
        return;
    }

    eqUpdatePending = 0;

    for (uint32_t band = 0; band < EQ_BAND_COUNT; band++)
    {
        eqBandGainsDB[band] = eqRequestedGainsDB[band];
    }

    AudioEQ_UpdatePreampGain();
    AudioEQ_CalculateCoefficients();
}

static void AudioEQ_ApplyLimiter(void)
{
    float32_t peak = 0.0f;

    for (uint32_t i = 0; i < EQ_BLOCK_SIZE; i++)
    {
        float32_t leftPeak = fabsf(eqLeftBuffer[i]);
        float32_t rightPeak = fabsf(eqRightBuffer[i]);

        if (leftPeak > peak)
        {
            peak = leftPeak;
        }

        if (rightPeak > peak)
        {
            peak = rightPeak;
        }
    }

    float32_t targetGain = 1.0f;

    if (peak > EQ_LIMITER_THRESHOLD)
    {
        targetGain = EQ_LIMITER_THRESHOLD / peak;
    }

    if (targetGain < eqLimiterGain)
    {
        eqLimiterGain = targetGain;
    }
    else
    {
        eqLimiterGain +=
            EQ_LIMITER_RELEASE * (targetGain - eqLimiterGain);
    }

    for (uint32_t i = 0; i < EQ_BLOCK_SIZE; i++)
    {
        eqLeftBuffer[i] *= eqLimiterGain;
        eqRightBuffer[i] *= eqLimiterGain;
    }
}

void AudioEQ_Process(uint8_t *audioData)
{
    if (!EQEnabled)
    {
        return;
    }

    AudioEQ_ApplyPendingGains();

    int16_t *samples =
        (int16_t *)audioData;

    for (uint32_t i = 0; i < EQ_BLOCK_SIZE; i++)
    {
        eqLeftBuffer[i] =
            (float32_t)samples[2 * i] * eqPreampGain;

        eqRightBuffer[i] =
            (float32_t)samples[2 * i + 1] * eqPreampGain;
    }

    arm_biquad_cascade_df1_f32(
        &eqLeft,
        eqLeftBuffer,
        eqLeftBuffer,
        EQ_BLOCK_SIZE
    );

    arm_biquad_cascade_df1_f32(
        &eqRight,
        eqRightBuffer,
        eqRightBuffer,
        EQ_BLOCK_SIZE
    );

    AudioEQ_ApplyLimiter();

    for (uint32_t i = 0; i < EQ_BLOCK_SIZE; i++)
    {
        float32_t left = eqLeftBuffer[i];
        float32_t right = eqRightBuffer[i];

        if (left > 32767.0f)
            left = 32767.0f;

        if (left < -32768.0f)
            left = -32768.0f;

        if (right > 32767.0f)
            right = 32767.0f;

        if (right < -32768.0f)
            right = -32768.0f;

        samples[2 * i] =
            (int16_t)left;

        samples[2 * i + 1] =
            (int16_t)right;
    }
}

void AudioFFT_Process(uint8_t *audioData)
{
    int16_t *samples = (int16_t *)audioData;

    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        int32_t left  = samples[2 * i];
        int32_t right = samples[2 * i + 1];

        fftInput[i] = ((float32_t)left + (float32_t)right) * 0.5f;
    }

    float32_t mean = 0.0f;

    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        mean += fftInput[i];
    }

    mean /= FFT_SIZE;

    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        fftInput[i] -= mean;
    }

    for (uint32_t i = 0; i < FFT_SIZE; i++)
    {
        fftInput[i] *= hannWindow[i];
    }

    arm_rfft_fast_f32(
        &fftInstance,
        fftInput,
        fftOutput,
        0
    );

    fftMagnitude[0] = fabsf(fftOutput[0])/ FFT_SIZE;

    for (uint32_t k = 1; k < FFT_SIZE / 2; k++)
    {
        float32_t real = fftOutput[2 * k];
        float32_t imag = fftOutput[2 * k + 1];

        fftMagnitude[k] =
            sqrtf(real * real + imag * imag)/ FFT_SIZE;
    }

    uint32_t maxBin = 1;
    float32_t maxMagnitude = fftMagnitude[1];

    for (uint32_t k = 2; k < FFT_SIZE / 2; k++)
    {
        if (fftMagnitude[k] > maxMagnitude)
        {
            maxMagnitude = fftMagnitude[k];
            maxBin = k;
        }
    }
    static const uint16_t bandEdges[FFT_BANDS + 1] =
    {
        1,
        2,
        3,
        4,
        6,
        9,
        13,
        19,
        28,
        41,
        60,
        88,
        129,
        189,
        277,
        405,
        511
    };

    for (uint32_t band = 0; band < FFT_BANDS; band++)
    {
        float32_t sum = 0.0f;

        uint32_t startBin = bandEdges[band];
        uint32_t endBin   = bandEdges[band + 1];

        for (uint32_t k = startBin; k < endBin; k++)
        {
            sum += fftMagnitude[k];
        }

        uint32_t count = endBin - startBin;

        if (count > 0)
        {
            fftBands[band] = sum / (float32_t)count;
        }
        else
        {
            fftBands[band] = 0.0f;
        }
    }
    float32_t maxBand = 1.0f;

    for (uint32_t band = 0; band < FFT_BANDS; band++)
    {
        if (fftBands[band] > maxBand)
        {
            maxBand = fftBands[band];
        }
    }

    for (uint32_t band = 0; band < FFT_BANDS; band++)
    {
        float32_t normalized =
            (fftBands[band] / maxBand) * 100.0f;

        if (normalized > 100.0f)
        {
            normalized = 100.0f;
        }

        fftBandsSmoothed[band] =
            0.7f * fftBandsSmoothed[band] +
            0.3f * normalized;
    }

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
