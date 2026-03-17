/* Time base functions for FreeRTOS */

#include <FreeRTOS.h>
#include <stm32l432xx.h>

static configRUN_TIME_COUNTER_TYPE tick_1ms;
void TIM7_init(void)
{
    /* Enable the TIM7 clock. */
    RCC->APB1ENR1 |= RCC_APB1ENR1_TIM7EN;

    /* Set the prescaler so the timer counts at 1 MHz. */
    TIM7->PSC = SystemCoreClock/1000000 - 1;

    /* Set the auto-reload value to 999, so the timer overflows every 1 ms. */
    TIM7->ARR = 999;

    /* Enable the update interrupt. */
    TIM7->DIER |= TIM_DIER_UIE;

    /* Enable the timer. */
    TIM7->CR1 |= TIM_CR1_CEN;

    /* Enable the TIM7 interrupt in the NVIC. */
    NVIC_EnableIRQ(TIM7_IRQn);
}

configRUN_TIME_COUNTER_TYPE getTimeBase(void)
{
    /* Return the current tick count. */
    return (tick_1ms*1000) + TIM7->CNT;
}

void TIM7_IRQHandler(void)
{
    /* Check if the update interrupt flag is set. */
    if (TIM7->SR & TIM_SR_UIF)
    {
        /* Clear the update interrupt flag. */
        TIM7->SR &= ~TIM_SR_UIF;

        /* Increment the RTOS tick. */
        tick_1ms++;
    }
}