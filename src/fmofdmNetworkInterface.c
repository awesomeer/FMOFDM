#include <stdbool.h>

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "NetworkBufferManagement.h"
#include "NetworkInterface.h"

#include <fmofdm.h>


/*-----------------------------------------------------------*/

NetworkInterface_t * xFMOFDMInterface;

static BaseType_t xFMOFDM_Initialise( NetworkInterface_t * pxInterface );
static BaseType_t xFMOFDM_Output( NetworkInterface_t * pxInterface,
                                      NetworkBufferDescriptor_t * const pxGivenDescriptor,
                                      BaseType_t bReleaseAfterSend );
static BaseType_t xFMOFDM_GetPhyLinkStatus( NetworkInterface_t * pxInterface );

NetworkInterface_t * pxFMOFDM_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                         NetworkInterface_t * pxInterface );

void xFMOFDM_recvTask(void * parameters);

/*-----------------------------------------------------------*/

static BaseType_t xFMOFDM_Initialise( NetworkInterface_t * pxInterface )
{
    ( void ) pxInterface;
    static bool tasksCreated = false;

    if(!tasksCreated)
    {

        static StaticTask_t fmofdmTaskTCB;
        static StackType_t fmofdmTaskStack[configMINIMAL_STACK_SIZE*2];
        xTaskCreateStatic( &fmofdmTask,
                        "fmofdm",
                        configMINIMAL_STACK_SIZE*2,
                        NULL,
                        configMAX_PRIORITIES - 1U,
                        &( fmofdmTaskStack[ 0 ] ),
                        &( fmofdmTaskTCB ) );

        static StaticTask_t fmofdmRecvTaskTCB;
        static StackType_t fmofdmRecvTaskStack[configMINIMAL_STACK_SIZE*2];
        xTaskCreateStatic( &xFMOFDM_recvTask,
                        "fmofdmrecvTask",
                        configMINIMAL_STACK_SIZE*2,
                        NULL,
                        configMAX_PRIORITIES - 1U,
                        &( fmofdmRecvTaskStack[ 0 ] ),
                        &( fmofdmRecvTaskTCB ) );

        tasksCreated = true;
    }

    return pdTRUE;
}
/*-----------------------------------------------------------*/


NetworkInterface_t * pxFMOFDM_FillInterfaceDescriptor( BaseType_t xEMACIndex,
                                                         NetworkInterface_t * pxInterface )
{

    // Fill in the interface descriptor with the FMOFDM interface details
    memset( pxInterface, '\0', sizeof( *pxInterface ) );
    pxInterface->pcName = "FMOFDM";
    pxInterface->pvArgument = ( void * ) xEMACIndex;
    pxInterface->pfInitialise = xFMOFDM_Initialise;
    pxInterface->pfOutput = xFMOFDM_Output;
    pxInterface->pfGetPhyLinkStatus = xFMOFDM_GetPhyLinkStatus;

    // Add the interface to the FreeRTOS+TCP stack
    FreeRTOS_AddNetworkInterface( pxInterface );
    xFMOFDMInterface = pxInterface;

    return pxInterface;
}
/*-----------------------------------------------------------*/

static BaseType_t xFMOFDM_GetPhyLinkStatus( NetworkInterface_t * pxInterface )
{
    ( void ) pxInterface;

    // The FMOFDM interface is always up, so return pdTRUE.
    return pdTRUE;
}
/*-----------------------------------------------------------*/

static BaseType_t xFMOFDM_Output( NetworkInterface_t * pxInterface,
                                      NetworkBufferDescriptor_t * const pxGivenDescriptor,
                                      BaseType_t bReleaseAfterSend )
{
    NetworkBufferDescriptor_t * pxDescriptor = pxGivenDescriptor;

    ( void ) pxInterface;

    IPPacket_t * a = ( IPPacket_t * ) ( pxDescriptor->pucEthernetBuffer );

    // Calculate the checksum for IPv4 packets before sending
    // even though we do not send the checksum
    if( a->xEthernetHeader.usFrameType == ipIPv4_FRAME_TYPE )
    {
        usGenerateProtocolChecksum( pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength, pdTRUE );
    }

    // Send Ethernet frame to FMOFDM interface
    fmofdm_send_data( pxDescriptor->pucEthernetBuffer, pxDescriptor->xDataLength );

    if( bReleaseAfterSend != pdFALSE )
    {
        vReleaseNetworkBufferAndDescriptor( pxDescriptor );
    }

    /* The return value is actually ignored by the IP-stack. */
    return pdTRUE;
}
/*-----------------------------------------------------------*/

void xFMOFDM_recvTask(void * parameters)
{
    while(true)
    {
        NetworkBufferDescriptor_t * pxDescriptor = pxGetNetworkBufferWithDescriptor( 128, portMAX_DELAY );

        if( pxDescriptor != NULL )
        {
            IPStackEvent_t xRxEvent;

            xRxEvent.eEventType = eNetworkRxEvent;
            xRxEvent.pvData = ( void * ) pxDescriptor;

            pxDescriptor->xDataLength = fmofdm_recv_data( pxDescriptor->pucEthernetBuffer, 128, portMAX_DELAY );

            pxDescriptor->pxInterface = xFMOFDMInterface;
            pxDescriptor->pxEndPoint = FreeRTOS_MatchingEndpoint( xFMOFDMInterface, pxDescriptor->pucEthernetBuffer );

            if( pxDescriptor->pxEndPoint == NULL )
            {
                vReleaseNetworkBufferAndDescriptor( pxDescriptor );
                iptraceETHERNET_RX_EVENT_LOST();
                FreeRTOS_printf( ( "xFMOFDM_recvTask: Can not find a proper endpoint\n" ) );
            }
            else if( xSendEventStructToIPTask( &xRxEvent, 0u ) != pdTRUE )
            {
                /* Sending failed, release the descriptor. */
                vReleaseNetworkBufferAndDescriptor( pxDescriptor );
                iptraceETHERNET_RX_EVENT_LOST();
                FreeRTOS_printf( ( "xFMOFDM_recvTask: Can not queue return packet!\n" ) );
            }
        }
        else
        {
            while(pdTRUE)
            {
                __NOP();
            }
        }
    }
}