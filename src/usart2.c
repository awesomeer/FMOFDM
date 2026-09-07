#include <usart2.h>
#include <stream_buffer.h>

static uint8_t tx_buffer[128];
static uint8_t rx_buffer[128];

static StaticStreamBuffer_t txStream;
static StreamBufferHandle_t txStreamHandle;

static StaticStreamBuffer_t rxStream;
static StreamBufferHandle_t rxStreamHandle;

void usart2_init(void)
{
    txStreamHandle = xStreamBufferCreateStatic( sizeof(tx_buffer), 1, tx_buffer, &txStream );
    rxStreamHandle = xStreamBufferCreateStatic( sizeof(rx_buffer), 1, rx_buffer, &rxStream );

    /* Enable GPIOA clock for pins A2 and A15. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;

    /* Configure PA2 (TX) and PA15 (RX) as alternate function mode. */
    GPIOA->MODER &= ~(GPIO_MODER_MODER2 | GPIO_MODER_MODER15);
    GPIOA->MODER |= (GPIO_MODER_MODER2_1 | GPIO_MODER_MODER15_1);

    /* Set alternate function 7 (USART2) for PA2 and PA15. */
    GPIOA->AFR[0] &= ~(GPIO_AFRL_AFSEL2);
    GPIOA->AFR[0] |= (7 << GPIO_AFRL_AFSEL2_Pos);
    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL15);
    GPIOA->AFR[1] |= (3 << GPIO_AFRH_AFSEL15_Pos);

    /* Enable the USART2 clock. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_USART2EN;

    /* Configure USART2 for 115200 baud, 8 data bits, no parity, 1 stop bit. */
    USART2->BRR = SystemCoreClock / 115200;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
    USART2->CR1 |=  USART_CR1_RXNEIE;

    NVIC_SetPriority(USART2_IRQn, configMAX_SYSCALL_INTERRUPT_PRIORITY >> __NVIC_PRIO_BITS);
    NVIC_EnableIRQ(USART2_IRQn);
}

void usart2_write(const uint8_t *data, uint32_t len)
{
    while(len)
    {
        size_t bytes_written = xStreamBufferSend(txStreamHandle, data, len, portMAX_DELAY);
        data += bytes_written;
        len -= bytes_written;
        USART2->CR1 |= USART_CR1_TXEIE;  // Enable TXE interrupt to start sending
    }
}

uint32_t usart2_read(uint8_t *buffer, uint32_t len, TickType_t delay)
{   
    uint32_t bytes_received = 0;

    while(len)
    {
        size_t bytes_read = xStreamBufferReceive(rxStreamHandle, buffer, len, delay);
        if (bytes_read == 0) {
            break; // No more data available or timeout occurred
        }
        buffer += bytes_read;
        len -= bytes_read;
        bytes_received += bytes_read;
    }
    return bytes_received;
}


void USART2_IRQHandler(void)
{
    static uint8_t echo = 0;

    if(USART2->ISR & USART_ISR_RXNE)
    {
        echo = USART2->RDR;
        xStreamBufferSendFromISR(rxStreamHandle, &echo, 1, NULL);
    }

    if(USART2->ISR & USART_ISR_TXE)
    {
        uint8_t byte_to_send;
        if(echo) {
            USART2->TDR = echo;
            echo = 0;
        }
        else if (xStreamBufferReceiveFromISR(txStreamHandle, &byte_to_send, 1, NULL) == pdTRUE)
        {
            USART2->TDR = byte_to_send;  // Use TDR for clarity (same as DR in L4)
        }
        else
        {
            USART2->CR1 &= ~USART_CR1_TXEIE;  // No more data to send, disable TXE interrupt
        }
    }
}