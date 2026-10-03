#include <FreeRTOS.h>
#include <FreeRTOS_IP.h>
#include <FreeRTOS_ARP.h>

#include <stm32l4xx.h>

#include <usart2.h>
#include <udp.h>

#define UDP_CLIENT_IP "192.168.1.22" // "192.168.1.22" "127.0.0.1"

// Dummy IP Address and MAC Address for FMOFDM interface
static const uint8_t ucMACAddress[ 6 ] = { 0x00, 0x11, 0x22, 0x33, 0x44, 0x55 };
static const uint8_t ucIPAddress[ 4 ] = { 192, 168, 1, 22 };
static const uint8_t ucNetMask[ 4 ] = { 255, 255, 255, 0 };
static const uint8_t ucGatewayAddress[ 4 ] = { 192, 168, 1, 1 };
static const uint8_t ucDNSServerAddress[ 4 ] = { 208, 67, 222, 222 };

NetworkInterface_t xInterfaces[ 1 ];
NetworkEndPoint_t xEndPoints[ 1 ];

void tcp_ip_stack_init()
{
    // Fill Interface Descriptor for FMOFDM interface
    pxFMOFDM_FillInterfaceDescriptor( 0, &( xInterfaces[ 0 ] ) );

    // Create endpoint for FMOFDM interface
    FreeRTOS_FillEndPoint( &( xInterfaces[ 0 ] ), &( xEndPoints[ 0 ] ), ucIPAddress,
            ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );

    // Initialize the FreeRTOS+TCP stack
    FreeRTOS_IPInit_Multi();
}

void UDPClientTask(void * parameters)
{
	(void) parameters;

   // Send strings to port 10000 on IP address 192.168.1.22
   xFreertosSocAddr xDestinationAddress;
   memset( &xDestinationAddress, 0, sizeof(xDestinationAddress) );
   xDestinationAddress.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr( UDP_CLIENT_IP );
   xDestinationAddress.sin_family = FREERTOS_AF_INET4;
   xDestinationAddress.sin_port = FreeRTOS_htons( 10000 );

   // Create IPv4 UDP socket
   Socket_t xSocket;
   xSocket = FreeRTOS_socket(   FREERTOS_AF_INET,
                                FREERTOS_SOCK_DGRAM,
                              	FREERTOS_IPPROTO_UDP );


    // Start sending strings to the UDP server every second
    uint8_t cString[ 50 ];
	uint32_t ulCount = 0UL;
	for( ;; )
	{
        // Create Message String
		sprintf( cString,
				"Standard send message number %lurn\n\r",
				ulCount );

		// Send Message String to UDP Server
		FreeRTOS_sendto( xSocket,
						cString,
						strlen( cString ),
						0,
						&xDestinationAddress,
						sizeof( xDestinationAddress ) );

		ulCount++;

        // Wait for 1 second before next message
		vTaskDelay( pdMS_TO_TICKS(1000) );
	}
}

void UDPServerTask(void * parameters)
{
    long lBytes;
    uint8_t cReceivedString[ 60 ];
    struct freertos_sockaddr xClient, xBindAddress;
    uint32_t xClientLength = sizeof( xClient );
    Socket_t xListeningSocket;

    /* Attempt to open the socket. */
    xListeningSocket = FreeRTOS_socket( FREERTOS_AF_INET,
                                        FREERTOS_SOCK_DGRAM,   /* FREERTOS_SOCK_DGRAM for UDP */
                                        FREERTOS_IPPROTO_UDP );

    /* Check the socket was created. */
    configASSERT( xListeningSocket != FREERTOS_INVALID_SOCKET );

    /* Bind to port 10000. */
    memset( &xBindAddress, 0, sizeof(xBindAddress) );
    xBindAddress.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr( UDP_CLIENT_IP );
    xBindAddress.sin_port = FreeRTOS_htons( 10000 );
    xBindAddress.sin_family = FREERTOS_AF_INET4;
    FreeRTOS_bind( xListeningSocket, &xBindAddress, sizeof( xBindAddress ) );

   for( ;; )
   {
       /* Receive data from the socket. ulFlags is zero, so the standard
          interface is used. By default the block time is portMAX_DELAY, but it
          can be changed using FreeRTOS_setsockopt(). */
       lBytes = FreeRTOS_recvfrom( xListeningSocket,
                                   cReceivedString,
                                   sizeof( cReceivedString ),
                                   0,
                                   &xClient,
                                   &xClientLength );

       if( lBytes > 0 )
       {
           // Print received string
           usart2_write(cReceivedString, lBytes);
       }
   }
}

void vApplicationIPNetworkEventHook_Multi( eIPCallbackEvent_t eNetworkEvent,
                                           struct xNetworkEndPoint * pxEndPoint )
{
    static BaseType_t xTasksAlreadyCreated = pdFALSE;

    if( eNetworkEvent == eNetworkUp )
    {

        // Create the UDP Client and Server Tasks after FMOFDM is up
        if( xTasksAlreadyCreated == pdFALSE )
        {

            static StaticTask_t UDPClientTaskTCB;
            static StackType_t UDPClientTaskStack[ configMINIMAL_STACK_SIZE*2 ];

            ( void ) xTaskCreateStatic( &UDPClientTask,
                                        "UDPClient",
                                        configMINIMAL_STACK_SIZE*2,
                                        NULL,
                                        configMAX_PRIORITIES - 2U,
                                        &( UDPClientTaskStack[ 0 ] ),
                                        &( UDPClientTaskTCB ) );

            static StaticTask_t UDPServerTaskTCB;
            static StackType_t UDPServerTaskStack[ configMINIMAL_STACK_SIZE*2 ];

            ( void ) xTaskCreateStatic( &UDPServerTask,
                                        "UDPServer",
                                        configMINIMAL_STACK_SIZE*2,
                                        NULL,
                                        configMAX_PRIORITIES - 2U,
                                        &( UDPServerTaskStack[ 0 ] ),
                                        &( UDPServerTaskTCB ) );

            xTasksAlreadyCreated = pdTRUE;
        }

        // Add MAC Address and IP Address to ARP cache
        // FreeRTOS ARP request will clash since IP address is the same for loopback
        MACAddress_t * MACaddress = &pxEndPoint->xMACAddress;
        uint32_t ipaddress = pxEndPoint->ipv4_settings.ulIPAddress;

        vARPRefreshCacheEntry( MACaddress, ipaddress, pxEndPoint );
    }
}

// Get a Random Number from the STM32L4's TRNG peripheral for FreeRTOS+TCP to use
BaseType_t xApplicationGetRandomNumber( uint32_t * pulNumber )
{
    RNG->CR |= RNG_CR_RNGEN;            // Enable the RNG peripheral
    while(!(RNG->SR & RNG_SR_DRDY));    // Wait for the random number
    *pulNumber = RNG->DR;               // Read the random number
    RNG->CR &= ~RNG_CR_RNGEN;           // Disable the RNG peripheral
    return pdTRUE;
}