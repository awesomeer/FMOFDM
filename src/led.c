

#include <stm32l432xx.h>
#include <led.h>

void LD3_init(void)
{
    /* Enable the GPIOB clock. */
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;

    /* Set PB3 to output mode. */
    GPIOB->MODER &= ~(GPIO_MODER_MODE3_Msk);
    GPIOB->MODER |= (GPIO_MODER_MODE3_0);

    /* Set PB3 to push-pull output type. */
    GPIOB->OTYPER &= ~(GPIO_OTYPER_OT3);

    /* Set PB3 to no pull-up, no pull-down. */
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD3_Msk);
}

void LD3_toggle(void)
{
    /* Toggle PB3. */
    GPIOB->ODR ^= GPIO_ODR_OD3;
}