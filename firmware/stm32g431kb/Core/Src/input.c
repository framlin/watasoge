#include "input.h"
#include "stm32g4xx_hal.h"

static volatile uint8_t gate_on_flag;
static volatile uint8_t gate_off_flag;

void input_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_0) {
        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET) {
            gate_on_flag = 1;
        } else {
            gate_off_flag = 1;
        }
    }
}

uint8_t input_gate_on_pending(void)
{
    if (gate_on_flag) {
        gate_on_flag = 0;
        return 1;
    }
    return 0;
}

uint8_t input_gate_off_pending(void)
{
    if (gate_off_flag) {
        gate_off_flag = 0;
        return 1;
    }
    return 0;
}
