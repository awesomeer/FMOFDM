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

    // Change MSI clock to 32 MHz
    // FLASH->ACR |= FLASH_ACR_LATENCY_1WS; // Set Flash latency to 1 wait state
    // RCC->CR  = (RCC->CR & ~RCC_CR_MSIRANGE_Msk) | (0b1011 << RCC_CR_MSIRANGE_Pos); // Clear MSIRANGE bits and set to 32 MHz
    // RCC->CR |= RCC_CR_MSIRGSEL; // Select MSIRANGE from RCC_CR register
    // SystemCoreClockUpdate();

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
