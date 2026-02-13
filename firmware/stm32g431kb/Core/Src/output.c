#include "main.h"
#include "output.h"
#include "synthesis.h"

/* --- SAI and DMA handles (global: referenced by hal_msp.c and it.c via extern) --- */
SAI_HandleTypeDef hsai1a;
DMA_HandleTypeDef hdma_sai1a;

/* --- Audio DMA buffer: 128 stereo frames = 256 halfwords --- */
static int16_t audio_buffer[256];

/* --- Switchable audio source --- */
static fill_buffer_fn audio_source = synthesis_fill_buffer;

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

void output_init(void)
{
    SAI1_Init();
    HAL_SAI_Transmit_DMA(&hsai1a, (uint8_t *)audio_buffer, 256);
}

void output_set_source(fill_buffer_fn fn)
{
    audio_source = fn;
}

void HAL_SAI_TxHalfCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    audio_source(&audio_buffer[0], 128);
}

void HAL_SAI_TxCpltCallback(SAI_HandleTypeDef *hsai)
{
    (void)hsai;
    audio_source(&audio_buffer[128], 128);
}
