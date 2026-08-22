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
#include "audio_equalizer.h"
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
volatile uint8_t EchoEnabled = 0;
volatile uint8_t echoResetPending = 0;
volatile uint8_t ReverbEnabled = 0;
volatile uint8_t reverbResetPending = 0;
volatile uint8_t NoiseReductionEnabled = 0;
volatile uint8_t noiseReductionResetPending = 0;

extern volatile uint8_t AudioVolume;

#define FFT_SIZE 1024
#define FFT_BANDS 16
#define EQ_LIMITER_THRESHOLD 32000.0f
#define EQ_LIMITER_RELEASE 0.05f

#define ECHO_BUFFER_ADDRESS 0xC0100000U
#define ECHO_MAX_SAMPLE_RATE 48000U
#define ECHO_MAX_DELAY_MS 500U
#define ECHO_DELAY_MS 280U
#define ECHO_CHANNEL_COUNT 2U
#define ECHO_MIX 0.28f
#define ECHO_FEEDBACK 0.37f
#define ECHO_MAX_DELAY_SAMPLES \
    ((ECHO_MAX_SAMPLE_RATE * ECHO_MAX_DELAY_MS) / 1000U)
#define ECHO_BUFFER_FLOAT_COUNT \
    (ECHO_MAX_DELAY_SAMPLES * ECHO_CHANNEL_COUNT)

#define REVERB_BUFFER_ADDRESS 0xC0140000U
#define REVERB_CHANNEL_COUNT 2U
#define REVERB_COMB_COUNT 4U
#define REVERB_ALLPASS_COUNT 2U
#define REVERB_MAX_SAMPLE_RATE 48000U
#define REVERB_COMB_MAX_DELAY_MS 50U
#define REVERB_ALLPASS_MAX_DELAY_MS 8U
#define REVERB_COMB_MAX_SAMPLES \
    ((REVERB_MAX_SAMPLE_RATE * REVERB_COMB_MAX_DELAY_MS) / 1000U)
#define REVERB_ALLPASS_MAX_SAMPLES \
    ((REVERB_MAX_SAMPLE_RATE * REVERB_ALLPASS_MAX_DELAY_MS) / 1000U)
#define REVERB_MIX 0.22f
#define REVERB_FEEDBACK 0.72f
#define REVERB_DAMPING 0.25f
#define REVERB_ALLPASS_GAIN 0.50f

#define NR_FRAME_SIZE 512U
#define NR_HOP_SIZE (NR_FRAME_SIZE / 2U)
#define NR_BIN_COUNT (NR_FRAME_SIZE / 2U + 1U)
#define NR_INIT_FRAMES 12U
#define NR_OVERSUBTRACTION 2.0f
#define NR_MIN_GAIN 0.08f
#define NR_NOISE_SMOOTHING 0.97f
#define NR_GAIN_SMOOTHING 0.60f
#define NR_NOISE_UPDATE_RATIO 2.5f
#define NR_EPSILON 1.0e-12f

float32_t fftBands[FFT_BANDS];
float32_t fftBandsSmoothed[FFT_BANDS];
float32_t fftInput[FFT_SIZE];
float32_t fftOutput[FFT_SIZE];
float32_t fftMagnitude[FFT_SIZE / 2];
float32_t hannWindow[FFT_SIZE];

typedef struct
{
    float32_t inputFrame[NR_FRAME_SIZE];
    float32_t outputOverlap[NR_FRAME_SIZE];
    float32_t noisePower[NR_BIN_COUNT];
    float32_t previousGain[NR_BIN_COUNT];
    uint32_t initFrameCount;
} NoiseReductionChannel;

static NoiseReductionChannel nrLeft;
static NoiseReductionChannel nrRight;
static float32_t nrWindow[NR_FRAME_SIZE];
static float32_t nrFftInput[NR_FRAME_SIZE];
static float32_t nrFftOutput[NR_FRAME_SIZE];
static float32_t nrCurrentGain[NR_BIN_COUNT];

float32_t eqLimiterGain = 1.0f;
float32_t eqLeftBuffer[EQ_BLOCK_SIZE];
float32_t eqRightBuffer[EQ_BLOCK_SIZE];

static float32_t *const echoBuffer =
    (float32_t *)ECHO_BUFFER_ADDRESS;
static uint32_t echoIndex = 0;
static uint32_t echoDelaySamples = 1;

typedef struct
{
    float32_t *buffer;
    uint32_t length;
    uint32_t index;
    float32_t dampingState;
} ReverbDelayLine;

static float32_t *const reverbBuffer =
    (float32_t *)REVERB_BUFFER_ADDRESS;
static ReverbDelayLine
    reverbComb[REVERB_CHANNEL_COUNT][REVERB_COMB_COUNT];
static ReverbDelayLine
    reverbAllpass[REVERB_CHANNEL_COUNT][REVERB_ALLPASS_COUNT];

arm_rfft_fast_instance_f32 fftInstance;
arm_rfft_fast_instance_f32 nrFftInstance;
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

void AudioEQ_Process(uint8_t *audioData);
void AudioEcho_SetEnabled(uint8_t enabled);
uint8_t AudioEcho_IsEnabled(void);
void AudioReverb_SetEnabled(uint8_t enabled);
uint8_t AudioReverb_IsEnabled(void);
void AudioNoiseReduction_SetEnabled(uint8_t enabled);
uint8_t AudioNoiseReduction_IsEnabled(void);
static void AudioEQ_ApplyLimiter(void);
static void AudioEcho_Init(float32_t sampleRate);
static void AudioEcho_Reset(void);
static void AudioEcho_Process(void);
static void AudioReverb_Init(float32_t sampleRate);
static void AudioReverb_Reset(void);
static void AudioReverb_Process(void);
static void AudioNoiseReduction_Init(void);
static void AudioNoiseReduction_Reset(void);
static void AudioNoiseReduction_Process(void);
static void AudioNoiseReduction_ProcessChannel(
    NoiseReductionChannel *channel,
    float32_t *samples
);
static float32_t AudioReverb_ProcessComb(
    ReverbDelayLine *line,
    float32_t input
);
static float32_t AudioReverb_ProcessAllpass(
    ReverbDelayLine *line,
    float32_t input
);
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

  AudioPlaying = 0;
  HalfBufferNeedsFill = 0;
  FullBufferNeedsFill = 0;
  AudioTrackFinished = 0;

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
  AudioEqualizer_Init((float32_t)sampleRate);
  eqLimiterGain = 1.0f;
  AudioEcho_Init((float32_t)sampleRate);
  AudioReverb_Init((float32_t)sampleRate);
  AudioNoiseReduction_Init();
  printf("Calling BSP_AUDIO_OUT_Init with sampleRate=%lu\r\n", (unsigned long)sampleRate);
  audio_status = BSP_AUDIO_OUT_Init(
      OUTPUT_DEVICE_HEADPHONE,
      AudioVolume,
      sampleRate
  );
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
    BSP_AUDIO_OUT_Stop(CODEC_PDWN_SW);
    AudioPlaying = 0;
    f_close(&WavFile);
    return 0;
  }
  printf("Playback started!\r\n");

  return 1;
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

void AudioEcho_SetEnabled(uint8_t enabled)
{
    uint8_t newState = enabled ? 1U : 0U;

    if (EchoEnabled != newState)
    {
        EchoEnabled = newState;
        echoResetPending = 1U;
    }
}

uint8_t AudioEcho_IsEnabled(void)
{
    return EchoEnabled;
}

static void AudioEcho_Reset(void)
{
    echoIndex = 0U;

    memset(
        echoBuffer,
        0,
        ECHO_BUFFER_FLOAT_COUNT * sizeof(float32_t)
    );
}

static void AudioEcho_Init(float32_t sampleRate)
{
    uint32_t requestedSamples =
        (uint32_t)((sampleRate * ECHO_DELAY_MS) / 1000.0f);

    if (requestedSamples < 1U)
    {
        requestedSamples = 1U;
    }
    else if (requestedSamples > ECHO_MAX_DELAY_SAMPLES)
    {
        requestedSamples = ECHO_MAX_DELAY_SAMPLES;
    }

    echoDelaySamples = requestedSamples;
    AudioEcho_Reset();
}

static void AudioEcho_Process(void)
{
    const float32_t dryMix = 1.0f - ECHO_MIX;

    for (uint32_t i = 0; i < EQ_BLOCK_SIZE; i++)
    {
        uint32_t bufferIndex = echoIndex * ECHO_CHANNEL_COUNT;

        float32_t dryLeft = eqLeftBuffer[i];
        float32_t dryRight = eqRightBuffer[i];
        float32_t delayedLeft = echoBuffer[bufferIndex];
        float32_t delayedRight = echoBuffer[bufferIndex + 1U];

        /* y[n] = (1-M)x[n] + M d[n] */
        eqLeftBuffer[i] =
            dryMix * dryLeft + ECHO_MIX * delayedLeft;
        eqRightBuffer[i] =
            dryMix * dryRight + ECHO_MIX * delayedRight;

        /* b[n] = x[n] + F d[n] */
        echoBuffer[bufferIndex] =
            dryLeft + ECHO_FEEDBACK * delayedLeft;
        echoBuffer[bufferIndex + 1U] =
            dryRight + ECHO_FEEDBACK * delayedRight;

        echoIndex++;

        if (echoIndex >= echoDelaySamples)
        {
            echoIndex = 0U;
        }
    }
}

void AudioReverb_SetEnabled(uint8_t enabled)
{
    uint8_t newState = enabled ? 1U : 0U;

    if (ReverbEnabled != newState)
    {
        ReverbEnabled = newState;
        reverbResetPending = 1U;
    }
}

uint8_t AudioReverb_IsEnabled(void)
{
    return ReverbEnabled;
}

static void AudioReverb_Reset(void)
{
    const uint32_t combFloatCount =
        REVERB_CHANNEL_COUNT *
        REVERB_COMB_COUNT *
        REVERB_COMB_MAX_SAMPLES;
    const uint32_t allpassFloatCount =
        REVERB_CHANNEL_COUNT *
        REVERB_ALLPASS_COUNT *
        REVERB_ALLPASS_MAX_SAMPLES;

    memset(
        reverbBuffer,
        0,
        (combFloatCount + allpassFloatCount) * sizeof(float32_t)
    );

    for (uint32_t channel = 0U;
         channel < REVERB_CHANNEL_COUNT;
         channel++)
    {
        for (uint32_t stage = 0U;
             stage < REVERB_COMB_COUNT;
             stage++)
        {
            reverbComb[channel][stage].index = 0U;
            reverbComb[channel][stage].dampingState = 0.0f;
        }

        for (uint32_t stage = 0U;
             stage < REVERB_ALLPASS_COUNT;
             stage++)
        {
            reverbAllpass[channel][stage].index = 0U;
            reverbAllpass[channel][stage].dampingState = 0.0f;
        }
    }
}

static void AudioReverb_Init(float32_t sampleRate)
{
    static const float32_t combDelayMs
        [REVERB_CHANNEL_COUNT][REVERB_COMB_COUNT] =
    {
        {29.7f, 37.1f, 41.1f, 43.7f},
        {30.9f, 38.3f, 42.3f, 44.9f}
    };
    static const float32_t allpassDelayMs
        [REVERB_CHANNEL_COUNT][REVERB_ALLPASS_COUNT] =
    {
        {5.0f, 1.7f},
        {5.5f, 2.1f}
    };

    uint32_t allpassBase =
        REVERB_CHANNEL_COUNT *
        REVERB_COMB_COUNT *
        REVERB_COMB_MAX_SAMPLES;

    for (uint32_t channel = 0U;
         channel < REVERB_CHANNEL_COUNT;
         channel++)
    {
        for (uint32_t stage = 0U;
             stage < REVERB_COMB_COUNT;
             stage++)
        {
            uint32_t slot =
                channel * REVERB_COMB_COUNT + stage;
            uint32_t length = (uint32_t)(
                sampleRate * combDelayMs[channel][stage] / 1000.0f
            );

            if (length < 1U)
            {
                length = 1U;
            }
            else if (length > REVERB_COMB_MAX_SAMPLES)
            {
                length = REVERB_COMB_MAX_SAMPLES;
            }

            reverbComb[channel][stage].buffer =
                &reverbBuffer[slot * REVERB_COMB_MAX_SAMPLES];
            reverbComb[channel][stage].length = length;
        }

        for (uint32_t stage = 0U;
             stage < REVERB_ALLPASS_COUNT;
             stage++)
        {
            uint32_t slot =
                channel * REVERB_ALLPASS_COUNT + stage;
            uint32_t length = (uint32_t)(
                sampleRate * allpassDelayMs[channel][stage] / 1000.0f
            );

            if (length < 1U)
            {
                length = 1U;
            }
            else if (length > REVERB_ALLPASS_MAX_SAMPLES)
            {
                length = REVERB_ALLPASS_MAX_SAMPLES;
            }

            reverbAllpass[channel][stage].buffer =
                &reverbBuffer[
                    allpassBase +
                    slot * REVERB_ALLPASS_MAX_SAMPLES
                ];
            reverbAllpass[channel][stage].length = length;
        }
    }

    AudioReverb_Reset();
}

static float32_t AudioReverb_ProcessComb(
    ReverbDelayLine *line,
    float32_t input
)
{
    float32_t delayed = line->buffer[line->index];

    /* f[n] = (1-D)d[n] + D f[n-1] */
    line->dampingState =
        (1.0f - REVERB_DAMPING) * delayed +
        REVERB_DAMPING * line->dampingState;

    /* b[n] = x[n] + G f[n] */
    line->buffer[line->index] =
        input + REVERB_FEEDBACK * line->dampingState;

    line->index++;

    if (line->index >= line->length)
    {
        line->index = 0U;
    }

    return delayed;
}

static float32_t AudioReverb_ProcessAllpass(
    ReverbDelayLine *line,
    float32_t input
)
{
    float32_t delayed = line->buffer[line->index];

    /* y[n] = -G x[n] + d[n] */
    float32_t output =
        -REVERB_ALLPASS_GAIN * input + delayed;

    /* b[n] = x[n] + G y[n] */
    line->buffer[line->index] =
        input + REVERB_ALLPASS_GAIN * output;

    line->index++;

    if (line->index >= line->length)
    {
        line->index = 0U;
    }

    return output;
}

static void AudioReverb_Process(void)
{
    for (uint32_t i = 0U; i < EQ_BLOCK_SIZE; i++)
    {
        float32_t dry[REVERB_CHANNEL_COUNT] =
        {
            eqLeftBuffer[i],
            eqRightBuffer[i]
        };
        float32_t wet[REVERB_CHANNEL_COUNT] = {0.0f, 0.0f};

        for (uint32_t channel = 0U;
             channel < REVERB_CHANNEL_COUNT;
             channel++)
        {
            for (uint32_t stage = 0U;
                 stage < REVERB_COMB_COUNT;
                 stage++)
            {
                wet[channel] += AudioReverb_ProcessComb(
                    &reverbComb[channel][stage],
                    dry[channel]
                );
            }

            wet[channel] /= (float32_t)REVERB_COMB_COUNT;

            for (uint32_t stage = 0U;
                 stage < REVERB_ALLPASS_COUNT;
                 stage++)
            {
                wet[channel] = AudioReverb_ProcessAllpass(
                    &reverbAllpass[channel][stage],
                    wet[channel]
                );
            }
        }

        /* output[n] = dry[n] + M wet[n] */
        eqLeftBuffer[i] =
            dry[0] + REVERB_MIX * wet[0];
        eqRightBuffer[i] =
            dry[1] + REVERB_MIX * wet[1];
    }
}

void AudioNoiseReduction_SetEnabled(uint8_t enabled)
{
    uint8_t newState = enabled ? 1U : 0U;

    if (NoiseReductionEnabled != newState)
    {
        NoiseReductionEnabled = newState;
        noiseReductionResetPending = 1U;
    }
}

uint8_t AudioNoiseReduction_IsEnabled(void)
{
    return NoiseReductionEnabled;
}

static void AudioNoiseReduction_Reset(void)
{
    memset(&nrLeft, 0, sizeof(nrLeft));
    memset(&nrRight, 0, sizeof(nrRight));

    for (uint32_t k = 0U; k < NR_BIN_COUNT; k++)
    {
        nrLeft.previousGain[k] = 1.0f;
        nrRight.previousGain[k] = 1.0f;
    }
}

static void AudioNoiseReduction_Init(void)
{
    if (arm_rfft_fast_init_f32(
            &nrFftInstance,
            NR_FRAME_SIZE
        ) != ARM_MATH_SUCCESS)
    {
        Error_Handler();
    }

    for (uint32_t i = 0U; i < NR_FRAME_SIZE; i++)
    {
        /* Square-root Hann gives perfect 50% overlap reconstruction. */
        float32_t hann =
            0.5f -
            0.5f * cosf((2.0f * PI * i) / NR_FRAME_SIZE);
        nrWindow[i] = sqrtf(hann);
    }

    AudioNoiseReduction_Reset();
}

static void AudioNoiseReduction_ProcessChannel(
    NoiseReductionChannel *channel,
    float32_t *samples
)
{
    for (uint32_t offset = 0U;
         offset < EQ_BLOCK_SIZE;
         offset += NR_HOP_SIZE)
    {
        memmove(
            channel->inputFrame,
            &channel->inputFrame[NR_HOP_SIZE],
            NR_HOP_SIZE * sizeof(float32_t)
        );

        memcpy(
            &channel->inputFrame[NR_HOP_SIZE],
            &samples[offset],
            NR_HOP_SIZE * sizeof(float32_t)
        );

        for (uint32_t i = 0U; i < NR_FRAME_SIZE; i++)
        {
            nrFftInput[i] =
                channel->inputFrame[i] * nrWindow[i];
        }

        arm_rfft_fast_f32(
            &nrFftInstance,
            nrFftInput,
            nrFftOutput,
            0
        );

        /*
         * P[k] = Re(X[k])^2 + Im(X[k])^2
         * N[k] = beta*N[k] + (1-beta)*P[k]
         * G[k] = max(Gmin, 1 - alpha*N[k]/(P[k] + epsilon))
         * Y[k] = G[k] * X[k]
         */
        for (uint32_t k = 0U; k < NR_BIN_COUNT; k++)
        {
            float32_t real;
            float32_t imag = 0.0f;

            if (k == 0U)
            {
                real = nrFftOutput[0];
            }
            else if (k == NR_FRAME_SIZE / 2U)
            {
                real = nrFftOutput[1];
            }
            else
            {
                real = nrFftOutput[2U * k];
                imag = nrFftOutput[2U * k + 1U];
            }

            float32_t power = real * real + imag * imag;

            if (channel->initFrameCount < NR_INIT_FRAMES)
            {
                channel->noisePower[k] +=
                    power / (float32_t)NR_INIT_FRAMES;
                nrCurrentGain[k] = 1.0f;
            }
            else
            {
                float32_t noise = channel->noisePower[k];

                if (power < NR_NOISE_UPDATE_RATIO * noise)
                {
                    noise =
                        NR_NOISE_SMOOTHING * noise +
                        (1.0f - NR_NOISE_SMOOTHING) * power;
                    channel->noisePower[k] = noise;
                }

                float32_t gain =
                    1.0f -
                    NR_OVERSUBTRACTION * noise /
                    (power + NR_EPSILON);

                if (gain < NR_MIN_GAIN)
                {
                    gain = NR_MIN_GAIN;
                }
                else if (gain > 1.0f)
                {
                    gain = 1.0f;
                }

                nrCurrentGain[k] = gain;
            }
        }

        if (channel->initFrameCount < NR_INIT_FRAMES)
        {
            channel->initFrameCount++;
        }
        else
        {
            for (uint32_t k = 0U; k < NR_BIN_COUNT; k++)
            {
                uint32_t previousBin = (k > 0U) ? k - 1U : k;
                uint32_t nextBin =
                    (k + 1U < NR_BIN_COUNT) ? k + 1U : k;

                float32_t frequencySmoothedGain =
                    (
                        nrCurrentGain[previousBin] +
                        2.0f * nrCurrentGain[k] +
                        nrCurrentGain[nextBin]
                    ) * 0.25f;

                float32_t gain =
                    NR_GAIN_SMOOTHING * channel->previousGain[k] +
                    (1.0f - NR_GAIN_SMOOTHING) *
                    frequencySmoothedGain;

                channel->previousGain[k] = gain;

                if (k == 0U)
                {
                    nrFftOutput[0] *= gain;
                }
                else if (k == NR_FRAME_SIZE / 2U)
                {
                    nrFftOutput[1] *= gain;
                }
                else
                {
                    nrFftOutput[2U * k] *= gain;
                    nrFftOutput[2U * k + 1U] *= gain;
                }
            }
        }

        arm_rfft_fast_f32(
            &nrFftInstance,
            nrFftOutput,
            nrFftInput,
            1
        );

        for (uint32_t i = 0U; i < NR_FRAME_SIZE; i++)
        {
            channel->outputOverlap[i] +=
                nrFftInput[i] * nrWindow[i];
        }

        memcpy(
            &samples[offset],
            channel->outputOverlap,
            NR_HOP_SIZE * sizeof(float32_t)
        );

        memmove(
            channel->outputOverlap,
            &channel->outputOverlap[NR_HOP_SIZE],
            NR_HOP_SIZE * sizeof(float32_t)
        );

        memset(
            &channel->outputOverlap[NR_HOP_SIZE],
            0,
            NR_HOP_SIZE * sizeof(float32_t)
        );
    }
}

static void AudioNoiseReduction_Process(void)
{
    AudioNoiseReduction_ProcessChannel(
        &nrLeft,
        eqLeftBuffer
    );

    AudioNoiseReduction_ProcessChannel(
        &nrRight,
        eqRightBuffer
    );
}

void AudioEQ_Process(uint8_t *audioData)
{
    if (echoResetPending)
    {
        echoResetPending = 0U;
        AudioEcho_Reset();
    }

    if (reverbResetPending)
    {
        reverbResetPending = 0U;
        AudioReverb_Reset();
    }

    if (noiseReductionResetPending)
    {
        noiseReductionResetPending = 0U;
        AudioNoiseReduction_Reset();
    }

    if (!EQEnabled &&
        !EchoEnabled &&
        !ReverbEnabled &&
        !NoiseReductionEnabled)
    {
        return;
    }

    int16_t *samples =
        (int16_t *)audioData;
    for (uint32_t i = 0; i < EQ_BLOCK_SIZE; i++)
    {
        eqLeftBuffer[i] =
            (float32_t)samples[2 * i];

        eqRightBuffer[i] =
            (float32_t)samples[2 * i + 1];
    }

    if (NoiseReductionEnabled)
    {
        AudioNoiseReduction_Process();
    }

    if (EQEnabled)
    {
        AudioEqualizer_Process(
            eqLeftBuffer,
            eqRightBuffer,
            EQ_BLOCK_SIZE
        );
    }

    if (EchoEnabled)
    {
        AudioEcho_Process();
    }

    if (ReverbEnabled)
    {
        AudioReverb_Process();
    }

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
