
#include <adc.h>
#include <fmofdm.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stream_buffer.h>
#include <stm32l432xx.h>


static q15_t adc_rx_buffer[10*NUM_FFT_LEN];

static StaticStreamBuffer_t adcRxStream;
static StreamBufferHandle_t adcRxStreamHandle;


static q15_t adc_dma_buffer[2*NUM_FFT_LEN];

void adc_init(void)
{

    adcRxStreamHandle = xStreamBufferCreateStatic( sizeof(adc_rx_buffer), 1, (void *)adc_rx_buffer, &adcRxStream );

    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;					// Enable ADC Clock
    ADC1_COMMON->CCR = 0b01 << ADC_CCR_CKMODE_Pos;      // Select SYS Clock

    ADC1->CR &= ~ADC_CR_DEEPPWD;						// Disable DEEPPWD
    ADC1->CR |= ADC_CR_ADVREGEN;						// Enable ADC VReg

    for(int i = 0; i < 1000; i++)                       // Wait >= 20us for Vreg
        __NOP();

    ADC1->CR |= ADC_CR_ADCAL;							// Start Single-ended Calibration
    while(ADC1->CR & ADC_CR_ADCAL);						// Wait until Calibration is done

    ADC1->CR |= ADC_CR_ADCALDIF;						// Enable Differential Calibration
    ADC1->CR |= ADC_CR_ADCAL;							// Start Calibration
    while(ADC1->CR & ADC_CR_ADCAL);						// Wait until Calibration is done

    ADC1->CR |= ADC_CR_ADEN;							// Enable ADC
    while(!(ADC1->ISR & ADC_ISR_ADRDY));				// Wait until ADC is ready
    ADC1->ISR = ADC_ISR_ADRDY;							// Clear ADRDY bit

    ADC1->SMPR2 |= (0b111 << ADC_SMPR2_SMP17_Pos);      // Max Sampling Time
    ADC1->SQR1 = 0 << ADC_SQR1_L_Pos |					// 1 conversion
                17 << ADC_SQR1_SQ1_Pos;					// Channel 17 DAC1


    ADC1->CFGR |= ADC_CFGR_DMAEN                |       // Enable DMA
                ADC_CFGR_DMACFG                 |       // Circular DMA
                ADC_CFGR_ALIGN                  |       // Left Align
                (0b10 << ADC_CFGR_EXTEN_Pos)    |		// External Rising Edge
                (13 << ADC_CFGR_EXTSEL_Pos);			// TIM6_TRGO External input


    // ADC DMA Configuration
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;

    DMA1_Channel1->CCR = (0b01 << DMA_CCR_MSIZE_Pos)    | 	// 16-bit Memory Size
                        (0b10 << DMA_CCR_PSIZE_Pos)     | 	// 32-bit Peripheral Size
                        DMA_CCR_MINC                    |   // Increment Memory Address
                        DMA_CCR_CIRC                    | 	// Circular DMA
                        DMA_CCR_HTIE					|	// Enable Half Transfer Complete Interrupt
                        DMA_CCR_TCIE;						// Enable Transfer Complete Interrupt

    DMA1_Channel1->CPAR = (uint32_t) &ADC1->DR;			    // Source Peripheral Address ADC1 Data Register
    DMA1_Channel1->CMAR = (uint32_t) adc_dma_buffer;        // Destinitaion memory address
    DMA1_Channel1->CNDTR = 2*NUM_FFT_LEN;					// Number of data items to transfer

    DMA1_CSELR->CSELR = (DMA1_CSELR->CSELR & ~DMA_CSELR_C1S_Msk) | (0b0000 << DMA_CSELR_C1S_Pos);      // Select ADC1 Request for Channel 1

    NVIC_SetPriority(DMA1_Channel1_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY >> __NVIC_PRIO_BITS);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    DMA1_Channel1->CCR |= DMA_CCR_EN;               // Enable DMA1 Channel 1

}


void adc_start(void)
{
    ADC1->CR |= ADC_CR_ADSTART;
}



/*
    Receive length q15_t samples from adc buffer
*/
void adc_readData(q15_t * buffer, uint32_t length)
{
    uint32_t byte_length = length*2;
    uint8_t * buffer_byte = (uint8_t *)buffer;
    while(byte_length)
    {
        size_t read = xStreamBufferReceive(adcRxStreamHandle, buffer_byte, byte_length, portMAX_DELAY);
        buffer_byte += read;
        byte_length -= read;
    }
}

void adc_flush(void)
{
    xStreamBufferReset(adcRxStreamHandle);
}

void DMA1_Channel1_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    if(DMA1->ISR & DMA_ISR_HTIF1)
    {
        for(int i = 0; i < NUM_FFT_LEN; i++) // I don't know why changing to bipolar format here makes it work
        {
            adc_dma_buffer[i] += 0x8000;
        }
        xStreamBufferSendFromISR(adcRxStreamHandle, &adc_dma_buffer[0], sizeof(q15_t)*NUM_FFT_LEN, &xHigherPriorityTaskWoken);
        DMA1->IFCR = DMA_IFCR_CHTIF1;
    }

    if(DMA1->ISR & DMA_ISR_TCIF1)
    {
        for(int i = 0; i < NUM_FFT_LEN; i++) // I don't know why changing to bipolar format here makes it work
        {
            adc_dma_buffer[NUM_FFT_LEN + i] += 0x8000;
        }
        xStreamBufferSendFromISR(adcRxStreamHandle, &adc_dma_buffer[NUM_FFT_LEN], sizeof(q15_t)*NUM_FFT_LEN, &xHigherPriorityTaskWoken);
        DMA1->IFCR = DMA_IFCR_CTCIF1;
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
