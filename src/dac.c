
#include <FreeRTOS.h>
#include <semphr.h>

#include <stm32l432xx.h>
#include <dac.h>


#define DAC_DMA_CHANNEL DMA1_Channel3


static StaticSemaphore_t dacDMASemaphore;
static SemaphoreHandle_t dacDMASemaphoreHandle;

void dac_init(void)
{

    dacDMASemaphoreHandle = xSemaphoreCreateBinaryStatic(&dacDMASemaphore);
    xSemaphoreGive(dacDMASemaphoreHandle);

    /* Enable the DAC clock. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_DAC1EN;

    /* Configure PA4 as analog mode. */
    GPIOA->MODER |= GPIO_MODER_MODER4;

    /* Enable the DAC channel 1. */
    DAC1->MCR = 0b001 << DAC_MCR_MODE1_Pos;             // On chip peripherials buffer disabled
    DAC1->CR |= DAC_CR_EN1;
    DAC1->DHR12L1 = 0x8000;

    // Configure DMA for DAC
    // DMA should trigger on TIM6 UF
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    DAC_DMA_CHANNEL->CCR = DMA_CCR_MINC                     // Increment memory address
                            | DMA_CCR_DIR                   // Memory to peripheral
                            | DMA_CCR_TEIE                  // Transfer error interrupt
                            //| DMA_CCR_HTIE                  // Half transfer interrupt
                            | DMA_CCR_TCIE                  // Transfer complete interrupt
                            | (0b01 << DMA_CCR_MSIZE_Pos)   // Memory size: 16 bits
                            | (0b01 << DMA_CCR_PSIZE_Pos);  // Peripheral size: 16 bits

    DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C3S) | (0b0110 << DMA_CSELR_C3S_Pos); // Select TIM6_UP

    DAC_DMA_CHANNEL->CPAR = (uint32_t)&DAC1->DHR12L1;       // Peripheral address

    NVIC_SetPriority(DMA1_Channel3_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY >> __NVIC_PRIO_BITS);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);
}

void dac_set(uint16_t value)
{
    /* Set the DAC channel 1 output value. */
    DAC1->DHR12R1 = value;
}

void dac_transmit(uint16_t *buffer, uint32_t length)
{

    xSemaphoreTake(dacDMASemaphoreHandle, portMAX_DELAY);

    /* Disable the DMA channel before configuring it. */
    DAC_DMA_CHANNEL->CCR &= ~DMA_CCR_EN;

    /* Set the memory address and number of data items to transfer. */
    DAC_DMA_CHANNEL->CMAR = (uint32_t)buffer;  // Memory address
    DAC_DMA_CHANNEL->CNDTR = length;           // Number of data items

    /* Enable the DMA channel. */
    DAC_DMA_CHANNEL->CCR |= DMA_CCR_EN;
}

void DMA1_Channel3_IRQHandler(void)
{
    if(DMA1->ISR & DMA_ISR_TEIF3)
    {
        DMA1->IFCR = DMA_IFCR_CTEIF3;
        xSemaphoreGiveFromISR(dacDMASemaphoreHandle, NULL);
    }

    if (DMA1->ISR & DMA_ISR_HTIF3)
    {
        DMA1->IFCR = DMA_IFCR_CHTIF3;
    }

    if (DMA1->ISR & DMA_ISR_TCIF3)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF3;
        xSemaphoreGiveFromISR(dacDMASemaphoreHandle, NULL);
#if 0
        DAC_DMA_CHANNEL->CCR &= ~DMA_CCR_EN;
        DAC_DMA_CHANNEL->CNDTR = 32;
        DAC_DMA_CHANNEL->CCR |= DMA_CCR_EN; // Debug just restart DMA to continue outputting the same buffer
#else
        // Set DAC to "0"
        DAC1->DHR12R1 = 0x800;
#endif
    }

}
