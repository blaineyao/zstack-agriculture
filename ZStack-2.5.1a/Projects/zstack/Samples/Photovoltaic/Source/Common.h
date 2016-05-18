#ifndef COMMON_H
#define COMMON_H

#include "ZComDef.h"

/**Add By  Yao Bin ***/
//#define SINK_ADDRESS  0x00
//#define ENDPOINT_ADDRESS  0x01
#define Modbus_Addr 0x01
#if !defined( SERIAL_APP_RADIO_HEAD )
#define SERIAL_APP_RADIO_HEAD1  0x7E
#define SERIAL_APP_RADIO_HEAD2  0xA5
#endif
/**END**/

#define GENERICAPP_ENDPOINT            10

#define GENERICAPP_PROFID          0x0F04
#define GENERICAPP_DEVICEID        0x0001
#define GENERICAPP_DEVICE_VERSION       0
#define GENERICAPP_FLAGS                0
#define GENERICAPP_MAX_CLUSTERS         1
#define GENERICAPP_CLUSTERID            1

#define INTERVAL_SAMPLE               10 //min
#define INTERVAL_SAMPLE_ACTUAL        60000//1min
#define GENERICAPP_SEND_MSG_TIMEOUT   2500     // Every 4 seconds

#define GENERICAPP_SEND_MSG_EVT       0x0001
#define IO_OUTPUT_485 P0DIR|=0x80
#define RECV_485 P0_7=0
#define SEND_485 P0_7=1


#ifdef UART_DMA
#define HAL_UART_DMA2 2
#define HAL_UART_DMA1 1
#else
#define HAL_UART_DMA2 0
#define HAL_UART_DMA1 0
#endif
extern void GenericApp_Init( byte task_id );
extern UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events );
#endif










