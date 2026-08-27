/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

/* Standard includes. */
#include <stdio.h>
#include <string.h>

#include <stm32l4xx.h>
#include <led.h>
#include <usart2.h>
#include <cli.h>
#include <fmofdm.h>


int main( void )
{

    FLASH->ACR |= FLASH_ACR_LATENCY_1WS; // Set Flash latency to 1 wait state

    // Change SYSCLK to 80 MHz
    RCC->PLLCFGR = (0b01 << RCC_PLLCFGR_PLLSRC_Pos) |   // Set PLL source to MSI @ 4 MHz
                    (0b000 << RCC_PLLCFGR_PLLM_Pos) |   // Set PLLM to 1
                    (40 << RCC_PLLCFGR_PLLN_Pos)    |   // Set PLLN to 40 VCO Freq = 160 MHz
                    (0b00 << RCC_PLLCFGR_PLLR_Pos)  |   // Set PLLR to 2
                    (RCC_PLLCFGR_PLLREN);               // Enable PLLR output

    RCC->CR |= RCC_CR_PLLON;            // Enable PLL
    while(!(RCC->CR & RCC_CR_PLLRDY));  // Wait for PLL to lock

    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW_Msk) | (0b11 << RCC_CFGR_SW_Pos); // Select PLL as system clock
    while((RCC->CFGR & RCC_CFGR_SWS_Msk) != (0b11 << RCC_CFGR_SWS_Pos));    // Wait for PLL to be selected as system clock

    SystemCoreClockUpdate();

    LD3_init();
    usart2_init();

    static StaticTask_t cliTaskTCB;
    static StackType_t cliTaskStack[ configMINIMAL_STACK_SIZE*2 ];

    ( void ) xTaskCreateStatic( &cliTask,
                                "cli",
                                configMINIMAL_STACK_SIZE*2,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( cliTaskStack[ 0 ] ),
                                &( cliTaskTCB ) );

    static StaticTask_t fmofdmTaskTCB;
    static StackType_t fmofdmTaskStack[configMINIMAL_STACK_SIZE*2];

    ( void ) xTaskCreateStatic( &fmofdmTask,
                                "fmofdm",
                                configMINIMAL_STACK_SIZE*2,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( fmofdmTaskStack[ 0 ] ),
                                &( fmofdmTaskTCB ) );
    /* Start the scheduler. */
    vTaskStartScheduler();

    for( ; ; )
    {
        /* Should not reach here. */
    }

    return 0;
}
/*-----------------------------------------------------------*/

#if ( configCHECK_FOR_STACK_OVERFLOW > 0 )

    void vApplicationStackOverflowHook( TaskHandle_t xTask,
                                        char * pcTaskName )
    {
        /* Check pcTaskName for the name of the offending task,
         * or pxCurrentTCB if pcTaskName has itself been corrupted. */
        ( void ) xTask;
        ( void ) pcTaskName;
        while(1)
        {
            __NOP();
        } /* Loop forever */
    }

#endif /* #if ( configCHECK_FOR_STACK_OVERFLOW > 0 ) */
/*-----------------------------------------------------------*/

#if ( configSUPPORT_STATIC_ALLOCATION == 1 )

    void vApplicationGetIdleTaskMemory( StaticTask_t ** ppxIdleTaskTCBBuffer,
                                        StackType_t ** ppxIdleTaskStackBuffer,
                                        uint32_t * pulIdleTaskStackSize )
    {
        static StaticTask_t xIdleTaskTCB;
        static StackType_t xIdleStack[ configMINIMAL_STACK_SIZE ];

        *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
        *ppxIdleTaskStackBuffer = &xIdleStack[ 0 ];
        *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    }

#endif /* #if ( configSUPPORT_STATIC_ALLOCATION == 1 ) */
/*-----------------------------------------------------------*/
