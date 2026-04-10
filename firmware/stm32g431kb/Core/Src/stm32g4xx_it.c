/**
  * @file    stm32g4xx_it.c
  * @brief   Interrupt Service Routines
  */

#include "main.h"
#include "stm32g4xx_it.h"

extern DMA_HandleTypeDef hdma_sai1a;

extern FDCAN_HandleTypeDef hfdcan1;

void NMI_Handler(void) {}
void HardFault_Handler(void) { while (1) {} }
void MemManage_Handler(void) { while (1) {} }
void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void DMA1_Channel1_IRQHandler(void)
{
    HAL_DMA_IRQHandler(&hdma_sai1a);
}

volatile uint8_t fdcan_irq_fired = 0;

void FDCAN1_IT0_IRQHandler(void)
{
    fdcan_irq_fired = 1;
    HAL_FDCAN_IRQHandler(&hfdcan1);
}
