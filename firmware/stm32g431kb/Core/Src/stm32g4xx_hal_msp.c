/**
  ******************************************************************************
  * @file    stm32g4xx_hal_msp.c
  * @brief   MSP Initialization
  ******************************************************************************
  */

#include "main.h"

/* SAI/DMA handles (global, referenziert von it.c und main.c via extern) */
SAI_HandleTypeDef hsai1a;
DMA_HandleTypeDef hdma_sai1a;

void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* Disable the internal Pull-Up in Dead Battery pins of UCPD peripheral */
    HAL_PWREx_DisableUCPDDeadBattery();
}

void HAL_SAI_MspInit(SAI_HandleTypeDef *hsai)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hsai->Instance == SAI1_Block_A)
    {
        __HAL_RCC_SAI1_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA8  = SAI1_SCK_A (AF14)
         * PA9  = SAI1_FS_A  (AF14)
         * PA10 = SAI1_SD_A  (AF14) */
        GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF14_SAI1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* DMA1 Channel1 for SAI1_A TX */
        __HAL_RCC_DMA1_CLK_ENABLE();
        __HAL_RCC_DMAMUX1_CLK_ENABLE();

        hdma_sai1a.Instance = DMA1_Channel1;
        hdma_sai1a.Init.Request = DMA_REQUEST_SAI1_A;
        hdma_sai1a.Init.Direction = DMA_MEMORY_TO_PERIPH;
        hdma_sai1a.Init.PeriphInc = DMA_PINC_DISABLE;
        hdma_sai1a.Init.MemInc = DMA_MINC_ENABLE;
        hdma_sai1a.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
        hdma_sai1a.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
        hdma_sai1a.Init.Mode = DMA_CIRCULAR;
        hdma_sai1a.Init.Priority = DMA_PRIORITY_HIGH;
        HAL_DMA_Init(&hdma_sai1a);

        __HAL_LINKDMA(hsai, hdmatx, hdma_sai1a);

        HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
    }
}

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *hfdcan)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (hfdcan->Instance == FDCAN1)
    {
        __HAL_RCC_FDCAN_CLK_ENABLE();
        __HAL_RCC_GPIOA_CLK_ENABLE();

        /* PA11 = FDCAN1_RX (AF9)
         * PA12 = FDCAN1_TX (AF9) */
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF9_FDCAN1;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* NVIC: DMA(2) > FDCAN(4) > SysTick(8) */
        HAL_NVIC_SetPriority(FDCAN1_IT0_IRQn, 4, 0);
        HAL_NVIC_EnableIRQ(FDCAN1_IT0_IRQn);
    }
}
