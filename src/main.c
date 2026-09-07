/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#include <timers.h>
#include <semphr.h>

/* Standard includes. */
#include <stdio.h>

#include <stm32l4xx.h>
#include <led.h>

/*-----------------------------------------------------------*/

static void exampleTask( void * parameters ) __attribute__( ( noreturn ) );

/*-----------------------------------------------------------*/

static void exampleTask( void * parameters )
{
    /* Unused parameters. */
    ( void ) parameters;

    for( ; ; )
    {
        /* Example Task Code */
        vTaskDelay( pdMS_TO_TICKS(1000) ); /* delay 100 ticks */
        LD3_toggle();
    }
}
/*-----------------------------------------------------------*/

int main( void )
{

    LD3_init();
    static StaticTask_t exampleTaskTCB;
    static StackType_t exampleTaskStack[ configMINIMAL_STACK_SIZE ];

    ( void ) xTaskCreateStatic( &exampleTask,
                                "example",
                                configMINIMAL_STACK_SIZE,
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
