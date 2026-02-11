#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32g4xx_hal.h"

/* LED2 (LD2) on NUCLEO-G431KB: PB8 */
#define LED2_PIN                GPIO_PIN_8
#define LED2_GPIO_PORT          GPIOB
#define LED2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOB_CLK_ENABLE()

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
