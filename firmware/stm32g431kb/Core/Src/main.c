#include "main.h"

/* --- Sine lookup table: 256 entries, ~75% full-scale --- */
static const int16_t sine_table[256] = {
        0,    589,   1178,   1766,   2352,   2938,   3522,   4103,
     4682,   5258,   5832,   6401,   6967,   7528,   8085,   8637,
     9184,   9726,  10261,  10791,  11314,  11830,  12338,  12840,
    13334,  13819,  14297,  14766,  15225,  15676,  16117,  16549,
    16971,  17382,  17783,  18173,  18552,  18920,  19277,  19622,
    19955,  20276,  20585,  20882,  21166,  21437,  21696,  21941,
    22173,  22392,  22597,  22789,  22967,  23131,  23281,  23417,
    23539,  23647,  23740,  23820,  23884,  23935,  23971,  23993,
    24000,  23993,  23971,  23935,  23884,  23820,  23740,  23647,
    23539,  23417,  23281,  23131,  22967,  22789,  22597,  22392,
    22173,  21941,  21696,  21437,  21166,  20882,  20585,  20276,
    19955,  19622,  19277,  18920,  18552,  18173,  17783,  17382,
    16971,  16549,  16117,  15676,  15225,  14766,  14297,  13819,
    13334,  12840,  12338,  11830,  11314,  10791,  10261,   9726,
     9184,   8637,   8085,   7528,   6967,   6401,   5832,   5258,
     4682,   4103,   3522,   2938,   2352,   1766,   1178,    589,
        0,   -589,  -1178,  -1766,  -2352,  -2938,  -3522,  -4103,
    -4682,  -5258,  -5832,  -6401,  -6967,  -7528,  -8085,  -8637,
    -9184,  -9726, -10261, -10791, -11314, -11830, -12338, -12840,
   -13334, -13819, -14297, -14766, -15225, -15676, -16117, -16549,
   -16971, -17382, -17783, -18173, -18552, -18920, -19277, -19622,
   -19955, -20276, -20585, -20882, -21166, -21437, -21696, -21941,
   -22173, -22392, -22597, -22789, -22967, -23131, -23281, -23417,
   -23539, -23647, -23740, -23820, -23884, -23935, -23971, -23993,
   -24000, -23993, -23971, -23935, -23884, -23820, -23740, -23647,
   -23539, -23417, -23281, -23131, -22967, -22789, -22597, -22392,
   -22173, -21941, -21696, -21437, -21166, -20882, -20585, -20276,
   -19955, -19622, -19277, -18920, -18552, -18173, -17783, -17382,
   -16971, -16549, -16117, -15676, -15225, -14766, -14297, -13819,
   -13334, -12840, -12338, -11830, -11314, -10791, -10261,  -9726,
    -9184,  -8637,  -8085,  -7528,  -6967,  -6401,  -5832,  -5258,
    -4682,  -4103,  -3522,  -2938,  -2352,  -1766,  -1178,   -589
};

/* --- SAI and DMA handles --- */
SAI_HandleTypeDef hsai1a;
DMA_HandleTypeDef hdma_sai1a;

/* --- Audio DMA buffer: 128 stereo frames = 256 halfwords --- */
static int16_t audio_buffer[256];

/* --- Phase accumulator (16.16 fixed-point) --- */
static uint32_t phase_acc = 0;
/* 440 Hz at ~44.27 kHz: (440 * 256 * 65536) / 44100 ≈ 167607 */
static const uint32_t phase_inc = 167607;

static void SystemClock_Config(void);
static void GPIO_Init(void);
static void SAI1_Init(void);
static void fill_audio_buffer(int16_t *buf, uint16_t len);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();
    SAI1_Init();

    HAL_SAI_Transmit_DMA(&hsai1a, (uint8_t *)audio_buffer, 256);

    while (1)
    {
        HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN);
        HAL_Delay(125);
    }
}

/**
 * System Clock: HSI 16 MHz -> PLL -> 170 MHz
 * PLLM=4, PLLN=85, PLLR=2 => 16/4*85/2 = 170 MHz
 * Voltage Scale 1 Boost, Flash Latency 4 WS
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Configure the main internal regulator output voltage */
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);

    /* Initializes the RCC Oscillators: HSI -> PLL */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4;
    RCC_OscInitStruct.PLL.PLLN = 85;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* Initializes the CPU, AHB and APB buses clocks */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * GPIO Init: PB8 as Push-Pull Output (LED2)
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    LED2_GPIO_CLK_ENABLE();

    HAL_GPIO_WritePin(LED2_GPIO_PORT, LED2_PIN, GPIO_PIN_RESET);

    GPIO_InitStruct.Pin = LED2_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(LED2_GPIO_PORT, &GPIO_InitStruct);
}

/**
 * SAI1 Block A: I2S Master TX, 16-bit stereo, ~44.1 kHz
 */
static void SAI1_Init(void)
{
    hsai1a.Instance = SAI1_Block_A;
    hsai1a.Init.AudioMode = SAI_MODEMASTER_TX;
    hsai1a.Init.Synchro = SAI_ASYNCHRONOUS;
    hsai1a.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    hsai1a.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai1a.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_44K;
    hsai1a.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    hsai1a.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai1a.Init.DataSize = SAI_DATASIZE_16;
    hsai1a.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai1a.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    hsai1a.Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
    hsai1a.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai1a.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai1a.Init.TriState = SAI_OUTPUT_NOTRELEASED;

    /* Frame: 32 SCK per frame, 16 active bits, FS = channel ID, active low */
    hsai1a.FrameInit.FrameLength = 32;
    hsai1a.FrameInit.ActiveFrameLength = 16;
    hsai1a.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai1a.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai1a.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;

    /* Slots: 2 active (L + R), slot size = data size */
    hsai1a.SlotInit.FirstBitOffset = 0;
    hsai1a.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
    hsai1a.SlotInit.SlotNumber = 2;
    hsai1a.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;

    if (HAL_SAI_Init(&hsai1a) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * Fill audio buffer with stereo sine samples (L=R)
 */
static void fill_audio_buffer(int16_t *buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i += 2)
    {
        int16_t sample = sine_table[(phase_acc >> 16) & 0xFF];
        buf[i]     = sample;  /* L */
        buf[i + 1] = sample;  /* R */
        phase_acc += phase_inc;
    }
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    fill_audio_buffer(&audio_buffer[0], 128);
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    fill_audio_buffer(&audio_buffer[128], 128);
}

void Error_Handler(void)
{
    __disable_irq();
    while (1)
    {
    }
}
