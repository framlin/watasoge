/**
  ******************************************************************************
  * @file    stm32g4xx_hal_msp.c
  * @brief   MSP Initialization
  ******************************************************************************
  */

#include "main.h"

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
    HAL_PWREx_DisableUCPDDeadBattery();
}
