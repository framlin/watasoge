#include "main.h"
#include "audio_config.h"
#include "synthesis.h"
#include "watasoge_module.h"
#include <fracanto/audio_pipeline.h>
#include <fracanto/module.h>
#include "fracanto_hal_stm32_fdcan.h"
#include "fracanto_pcm5102a.h"

/* --- SAI/DMA handles (defined in hal_msp.c) --- */
extern SAI_HandleTypeDef hsai1a;
extern DMA_HandleTypeDef hdma_sai1a;

/* --- FDCAN handle (referenced by it.c via extern) --- */
FDCAN_HandleTypeDef hfdcan1;

/* --- fracanto --- */
static fracanto_hal_t can_hal;
static fracanto_module_t mod;
static fracanto_audio_pipeline_t audio_pipe;
fracanto_pcm5102a_ctx_t dac_ctx;

static int16_t out_buf[2 * FRACANTO_AUDIO_BLOCK_SIZE * 2]
    __attribute__((aligned(4)));

/* --- Platform hooks --- */
uint32_t watasoge_get_tick_ms(void) { return HAL_GetTick(); }
void     watasoge_toggle_led(void)  { HAL_GPIO_TogglePin(LED2_GPIO_PORT, LED2_PIN); }

/* --- I2S DMA --- */

static int i2s_start_dma(void *hw_ctx, const int16_t *data,
                          uint16_t num_samples)
{
    (void)hw_ctx; (void)data; (void)num_samples;
    if (HAL_SAI_Transmit_DMA(&hsai1a, (uint8_t *)out_buf,
                              2 * FRACANTO_AUDIO_BLOCK_SIZE * 2) != HAL_OK)
        return -1;
    return 0;
}

static void i2s_stop_dma(void *hw_ctx)
{
    (void)hw_ctx;
    HAL_SAI_DMAStop(&hsai1a);
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    fracanto_pcm5102a_dma_complete(&dac_ctx);
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    fracanto_pcm5102a_dma_complete(&dac_ctx);
}

/* --- Peripheral init --- */

static void SAI1_Init(void)
{
    hsai1a.Instance = SAI1_Block_A;
    hsai1a.Init.AudioMode = SAI_MODEMASTER_TX;
    hsai1a.Init.Synchro = SAI_ASYNCHRONOUS;
    hsai1a.Init.OutputDrive = SAI_OUTPUTDRIVE_ENABLE;
    hsai1a.Init.FIFOThreshold = SAI_FIFOTHRESHOLD_1QF;
    hsai1a.Init.AudioFrequency = SAI_AUDIO_FREQUENCY_48K;
    hsai1a.Init.NoDivider = SAI_MASTERDIVIDER_ENABLE;
    hsai1a.Init.MonoStereoMode = SAI_STEREOMODE;
    hsai1a.Init.DataSize = SAI_DATASIZE_16;
    hsai1a.Init.FirstBit = SAI_FIRSTBIT_MSB;
    hsai1a.Init.ClockStrobing = SAI_CLOCKSTROBING_FALLINGEDGE;
    hsai1a.Init.MckOutput = SAI_MCK_OUTPUT_DISABLE;
    hsai1a.Init.Protocol = SAI_FREE_PROTOCOL;
    hsai1a.Init.CompandingMode = SAI_NOCOMPANDING;
    hsai1a.Init.TriState = SAI_OUTPUT_NOTRELEASED;
    hsai1a.FrameInit.FrameLength = 32;
    hsai1a.FrameInit.ActiveFrameLength = 16;
    hsai1a.FrameInit.FSDefinition = SAI_FS_CHANNEL_IDENTIFICATION;
    hsai1a.FrameInit.FSPolarity = SAI_FS_ACTIVE_LOW;
    hsai1a.FrameInit.FSOffset = SAI_FS_BEFOREFIRSTBIT;
    hsai1a.SlotInit.FirstBitOffset = 0;
    hsai1a.SlotInit.SlotSize = SAI_SLOTSIZE_DATASIZE;
    hsai1a.SlotInit.SlotNumber = 2;
    hsai1a.SlotInit.SlotActive = SAI_SLOTACTIVE_0 | SAI_SLOTACTIVE_1;
    if (HAL_SAI_Init(&hsai1a) != HAL_OK)
        Error_Handler();
}

static void FDCAN1_Init(void)
{
    hfdcan1.Instance = FDCAN1;
    hfdcan1.Init.ClockDivider = FDCAN_CLOCK_DIV1;
    hfdcan1.Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    hfdcan1.Init.Mode = FDCAN_MODE_NORMAL;
    hfdcan1.Init.AutoRetransmission = ENABLE;
    hfdcan1.Init.TransmitPause = ENABLE;
    hfdcan1.Init.ProtocolException = DISABLE;
    hfdcan1.Init.NominalPrescaler = 17;
    hfdcan1.Init.NominalSyncJumpWidth = 4;
    hfdcan1.Init.NominalTimeSeg1 = 15;
    hfdcan1.Init.NominalTimeSeg2 = 4;
    hfdcan1.Init.DataPrescaler = 1;
    hfdcan1.Init.DataSyncJumpWidth = 1;
    hfdcan1.Init.DataTimeSeg1 = 1;
    hfdcan1.Init.DataTimeSeg2 = 1;
    hfdcan1.Init.StdFiltersNbr = 0;
    hfdcan1.Init.ExtFiltersNbr = 0;
    hfdcan1.Init.TxFifoQueueMode = FDCAN_TX_FIFO_OPERATION;
    if (HAL_FDCAN_Init(&hfdcan1) != HAL_OK)
        Error_Handler();
}

static void SystemClock_Config(void);
static void GPIO_Init(void);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    GPIO_Init();

    /* Audio */
    SAI1_Init();
    fracanto_pcm5102a_hw_t hw = {
        .i2s_start_dma = i2s_start_dma,
        .i2s_stop_dma  = i2s_stop_dma,
        .hw_ctx        = NULL
    };
    fracanto_pcm5102a_init(&dac_ctx, &hw);
    fracanto_audio_pipeline_init(&audio_pipe, fracanto_pcm5102a_ops(),
                                  &dac_ctx, (uint32_t)SAMPLE_RATE,
                                  FRACANTO_AUDIO_BLOCK_SIZE,
                                  NULL, 0, out_buf, 2);

    /* CAN */
    FDCAN1_Init();
    fracanto_hal_stm32_fdcan_config_t fdcan_cfg = { .fd_enabled = 0 };
    fracanto_hal_stm32_fdcan_create(&can_hal, &hfdcan1, &fdcan_cfg);

    /* Module */
    static const fracanto_node_config_t node_cfg = {
        .node_id = 2, .module_type = 0x10, .fw_version = 0x01,
        .inputs = 0, .outputs = 1, .capabilities = 0,
        .heartbeat_interval_ms = 1000, .announce_interval_ms = 500,
        .announce_retries = 3, .error_recovery_ms = 2000,
    };
    fracanto_module_init(&mod, &can_hal, &node_cfg,
                          NULL, &watasoge_ops, &watasoge_ctx);
    fracanto_module_set_audio_pipeline(&mod, &audio_pipe);
    /* Start CAN node — non-fatal: audio works even if CAN bus has issues.
       Node stays in INIT state and won't process CAN messages until
       the bus is healthy and module_start succeeds on retry. */
    fracanto_module_start(&mod);

    /* Start audio pipeline (triggers DMA) */
    if (fracanto_audio_pipeline_start(&audio_pipe) != 0)
        Error_Handler();

    /* Circular-DMA starts at out_buf[0]. fracanto_audio_pipeline_start()
       sets active_idx=0, but the first Half-Complete would then write to
       buffer[1] which DMA is currently reading. Setting active_idx=1
       ensures the first tick writes to buffer[0] (just finished by DMA).
       This is a watasoge-specific fix for the Circular-DMA override in
       i2s_start_dma(). */
    audio_pipe.active_idx = 1;

    while (1)
    {
        fracanto_module_tick(&mod);
    }
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
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
        Error_Handler();
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                                | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
        Error_Handler();

    /* FDCAN kernel clock: PCLK1 (170 MHz) */
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_FDCAN;
    PeriphClkInit.FdcanClockSelection = RCC_FDCANCLKSOURCE_PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit);
}

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

void Error_Handler(void)
{
    __disable_irq();
    while (1) {}
}
