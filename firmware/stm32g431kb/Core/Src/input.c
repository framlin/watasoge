#include "input.h"
#include "stm32g4xx_hal.h"
#include <math.h>

#define CV_BASE_FREQ      32.703f          /* C1 bei 0V */
#define CV_VOLTS_PER_LSB  (3.3f / 4096.0f)

static volatile uint8_t gate_on_flag;
static volatile uint8_t gate_off_flag;
static ADC_HandleTypeDef hadc1;

void input_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Gate-Input PA0: EXTI Rising+Falling */
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = GPIO_PULLDOWN;
    HAL_GPIO_Init(GPIOA, &gpio);

    HAL_NVIC_SetPriority(EXTI0_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI0_IRQn);

    /* ADC1 Kanal 2 (PA1): 12-Bit, Single Conversion, Software-Trigger */
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV4;
    hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
    hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc1.Init.LowPowerAutoWait      = DISABLE;
    hadc1.Init.ContinuousConvMode    = DISABLE;
    hadc1.Init.NbrOfConversion       = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc1.Init.OversamplingMode      = DISABLE;
    HAL_ADC_Init(&hadc1);

    /* Kanal 2 (PA1) konfigurieren */
    ADC_ChannelConfTypeDef ch = {0};
    ch.Channel      = ADC_CHANNEL_2;
    ch.Rank         = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_47CYCLES_5;
    ch.SingleDiff   = ADC_SINGLE_ENDED;
    ch.OffsetNumber = ADC_OFFSET_NONE;
    HAL_ADC_ConfigChannel(&hadc1, &ch);

    /* Kalibrierung */
    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
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

float input_pitch_cv(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    float volts = (float)raw * CV_VOLTS_PER_LSB;
    return CV_BASE_FREQ * exp2f(volts);
}
