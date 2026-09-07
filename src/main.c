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

/*-----------------------------------------------------------*/

static void exampleTask( void * parameters ) __attribute__( ( noreturn ) );

/*-----------------------------------------------------------*/

uint8_t uart_rx[128];
uint8_t buffer[128];
static void exampleTask( void * parameters )
{
    /* Unused parameters. */
    ( void ) parameters;

    for( ; ; )
    {
        /* Example Task Code */
        LD3_toggle();

        TickType_t tick_count = xTaskGetTickCount();
        uint32_t len = snprintf ( ( char * ) buffer, sizeof(buffer), "Hello from FreeRTOS! Tick count: %d\n\r",  (uint32_t)tick_count);
        usart2_write( buffer, len );

        vTaskGetRunTimeStats( ( char * ) buffer );
        usart2_write( buffer, strlen(buffer) );

        len = usart2_read(uart_rx, sizeof(uart_rx)-1, 0 );
        if (len > 0) {
            uart_rx[len] = '\0'; // Null-terminate the received string
            len = snprintf ( ( char * ) buffer, sizeof(buffer),"Received: %d bytes: %s\n\r", len, uart_rx );
            usart2_write( buffer, len );
        }

        vTaskDelay( pdMS_TO_TICKS( 1000 ) ); /* delay 1 second */
    }
}
/*-----------------------------------------------------------*/

int main( void )
{

    LD3_init();
    usart2_init();
    static StaticTask_t exampleTaskTCB;
    static StackType_t exampleTaskStack[ configMINIMAL_STACK_SIZE*2 ];

    ( void ) xTaskCreateStatic( &exampleTask,
                                "example",
                                configMINIMAL_STACK_SIZE*2,
                                NULL,
                                configMAX_PRIORITIES - 1U,
                                &( exampleTaskStack[ 0 ] ),
                                &( exampleTaskTCB ) );

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
