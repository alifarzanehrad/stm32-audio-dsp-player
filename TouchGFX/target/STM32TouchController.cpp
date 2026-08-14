/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : STM32TouchController.cpp
  ******************************************************************************
  * This file was created by TouchGFX Generator 4.26.1. This file is only
  * generated once! Delete this file from your project and re-generate code
  * using STM32CubeMX or change this file manually to update it.
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

/* USER CODE BEGIN STM32TouchController */

#include <STM32TouchController.hpp>

extern "C"
{
#include "stm32746g_discovery_ts.h"
}

void STM32TouchController::init()
{
    BSP_TS_Init(480, 272);
}

bool STM32TouchController::sampleTouch(int32_t& x, int32_t& y)
{
    TS_StateTypeDef TS_State;

    BSP_TS_GetState(&TS_State);

    if (TS_State.touchDetected > 0)
    {
        x = TS_State.touchX[0];
        y = TS_State.touchY[0];

        return true;
    }

    return false;
}

/* USER CODE END STM32TouchController */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
