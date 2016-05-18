#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "Common.h"
#include "DebugTrace.h"
#include <assert.h>

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"


/**Add By  Yao Bin ***/
#if !defined( SERIAL_APP_TX_MAX )
#define SERIAL_APP_TX_MAX  64
#endif
#if !defined( SERIAL_APP_RADIO_MAX )
#define SERIAL_APP_RADIO_MAX  64
#endif

byte send_modbus_data[8]={Modbus_Addr,0x03};
#ifdef METEOROLOGICAL
  #define TOTAL_REGISTER_NUM 22
  #define REGISTER_NUM 11
  #define START_ADDR 0x0000
  #define DEVICE_TYPE 1
#elif PV_COMBINER
  #define TOTAL_REGISTER_NUM 48
  #define REGISTER_NUM 12
  #define START_ADDR 0x1000
  #define DEVICE_TYPE 2
#else
  assert(0);//需要定义 PV_COMBINER or METEOROLOGICAL
#endif
byte RxBuf[SERIAL_APP_TX_MAX+1];
byte RadioBuf[SERIAL_APP_RADIO_MAX+1];
static uint8 RadioHead_TxLen=0;
static uint8 RadioApp_TxLen=0;
static uint8 SerialApp_TxLen;
byte *eui64;
static uint8 group=TOTAL_REGISTER_NUM/REGISTER_NUM;
uint8 group_index;
/**END**/

const cId_t GenericApp_ClusterList[GENERICAPP_MAX_CLUSTERS] =
{
  GENERICAPP_CLUSTERID
};

const SimpleDescriptionFormat_t GenericApp_SimpleDesc =
{
  GENERICAPP_ENDPOINT,              //  int Endpoint;
  GENERICAPP_PROFID,                //  uint16 AppProfId[2];
  GENERICAPP_DEVICEID,              //  uint16 AppDeviceId[2];
  GENERICAPP_DEVICE_VERSION,        //  int   AppDevVer:4;
  GENERICAPP_FLAGS,                 //  int   AppFlags:4; 
  GENERICAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)GenericApp_ClusterList,  //  byte *pAppInClusterList;
  GENERICAPP_MAX_CLUSTERS,          //  byte  AppNumInClusters;
  (cId_t *)GenericApp_ClusterList   //  byte *pAppInClusterList;
};

endPointDesc_t GenericApp_epDesc;
byte GenericApp_TaskID;
byte GenericApp_TransID;
devStates_t GenericApp_NwkState;

//void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
void GenericApp_SendTheMessage(byte *RxBuf,uint8 len);
void rxCB(uint8 port,uint8 event);
uint16 CRC16_Check(uint8 *Pushdata,uint8 length);
void modus_first_deal(byte *RxBuf,uint8 len);
void modus_second_deal(byte *RxBuf,uint8 len);
void modus_third_deal(byte *RxBuf,uint8 len);

void GenericApp_Init( byte task_id )
{ 
/***Add By Bin Yao for UART test****/
  RadioBuf[RadioHead_TxLen++]=SERIAL_APP_RADIO_HEAD1;
  RadioBuf[RadioHead_TxLen++]=SERIAL_APP_RADIO_HEAD2;
  eui64=NLME_GetExtAddr();//get the EUI-64 Address
  osal_memcpy(RadioBuf+RadioHead_TxLen,eui64,8);
  RadioHead_TxLen+=8;
  RadioBuf[RadioHead_TxLen++]=DEVICE_TYPE;//设备类型
  halUARTCfg_t uartConfig;
  uartConfig.configured           = TRUE;              // 2x30 don't care - see uart driver.
  uartConfig.baudRate             = HAL_UART_BR_9600;
  uartConfig.flowControl          = FALSE;
  uartConfig.flowControlThreshold = 64;   // 2x30 don't care - see uart driver.
  uartConfig.rx.maxBufSize        = 128;  // 2x30 don't care - see uart driver.
  uartConfig.tx.maxBufSize        = 128;  // 2x30 don't care - see uart driver.
  uartConfig.idleTimeout          = 6;    // 2x30 don't care - see uart driver.
  uartConfig.intEnable            = TRUE; // 2x30 don't care - see uart driver.
  uartConfig.callBackFunc         = rxCB;
  HalUARTOpen (0, &uartConfig);
  
  IO_OUTPUT_485;                //设置485/IO口为输出
  RECV_485;//receive
  group_index=0;
/**END*****/  
  
  GenericApp_TaskID = task_id;
  GenericApp_NwkState=DEV_INIT;
  GenericApp_TransID = 0;
    
  GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;
  GenericApp_epDesc.task_id = &GenericApp_TaskID;
  GenericApp_epDesc.simpleDesc= (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
  GenericApp_epDesc.latencyReq = noLatencyReqs;
  afRegister( &GenericApp_epDesc ); 
}

UINT16 GenericApp_ProcessEvent( byte task_id, UINT16 events )
{
    afIncomingMSGPacket_t *MSGpkt;
    if ( events & SYS_EVENT_MSG )
    {
        MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
        while ( MSGpkt )
        {
            switch ( MSGpkt->hdr.event )
            {
            case AF_INCOMING_MSG_CMD:
                //GenericApp_MessageMSGCB(MSGpkt);
                break;
            case ZDO_STATE_CHANGE:
                GenericApp_NwkState = (devStates_t)(MSGpkt->hdr.status);
                if(GenericApp_NwkState == DEV_ROUTER||GenericApp_NwkState == DEV_END_DEVICE)
                {
                  HalLedBlink(HAL_LED_2, 3, 50, 500);//Led2闪烁3次，表示组网成功.
                  osal_start_timerEx( GenericApp_TaskID,
                                     GENERICAPP_SEND_MSG_EVT,
                                     GENERICAPP_SEND_MSG_TIMEOUT );
                }
                break;
            default:
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
        }     
        // return unprocessed events
        return (events ^ SYS_EVENT_MSG);
    }
    if ( events & GENERICAPP_SEND_MSG_EVT )
    {
      // Setup to send message again
      osal_start_timerEx( GenericApp_TaskID,
                          GENERICAPP_SEND_MSG_EVT,
                        GENERICAPP_SEND_MSG_TIMEOUT );
      HalLedSet(HAL_LED_1,HAL_LED_MODE_ON);
      group_index=group_index%group;
      uint16 addr=START_ADDR+group_index*REGISTER_NUM;
      group_index++;
      send_modbus_data[2]=addr>>8&0xff;
      send_modbus_data[3]=addr&0xff;
      send_modbus_data[4]=REGISTER_NUM>>8&0xff;
      send_modbus_data[5]=REGISTER_NUM&0xff;   
      uint16 crc=CRC16_Check(send_modbus_data,6);
      send_modbus_data[6]=crc>>8&0xff;
      send_modbus_data[7]=crc&0xff;  
      HalUARTWrite(0, send_modbus_data, 8);
      // return unprocessed events
      return (events ^ GENERICAPP_SEND_MSG_EVT);
    }
    return 0;
}


void GenericApp_SendTheMessage(byte *RxBuf,uint8 len)
{    
    HalLedBlink(HAL_LED_2, 1, 50, 500);
    afAddrType_t devDstAddr;
    devDstAddr.addrMode=(afAddrMode_t)Addr16Bit;
    devDstAddr.endPoint=GENERICAPP_ENDPOINT;
    devDstAddr.addr.shortAddr=0x0000; 
    
    AF_DataRequest(&devDstAddr,
        &GenericApp_epDesc,
        GENERICAPP_CLUSTERID,
        len,//send len
        RxBuf,//send buf
        &GenericApp_TransID,
        AF_DISCV_ROUTE,
        AF_DEFAULT_RADIUS);
}

static void rxCB(uint8 port,uint8 event)
{
  if ((event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT)) &&
#if SERIAL_APP_LOOPBACK
      (SerialApp_TxLen < SERIAL_APP_TX_MAX))
#else
      !SerialApp_TxLen)
#endif
  {
    SerialApp_TxLen = HalUARTRead(0, RxBuf, SERIAL_APP_TX_MAX);
    if (SerialApp_TxLen&&SerialApp_TxLen>RxBuf[2])
    {
      if(RxBuf[0]==Modbus_Addr&&RxBuf[1]==0x03)
      {
        uint8 len=RxBuf[2];
        uint16 crc=CRC16_Check(RxBuf,len+3);
        uint8 crc_high=crc>>8&0xff;
        uint8 crc_low=crc&0xff;
        if(crc_high==RxBuf[len+3]&&crc_low==RxBuf[len+4])//确保从modbus_485接收到的数据没有错.
        {
          RadioApp_TxLen=RadioHead_TxLen;
          RadioBuf[RadioApp_TxLen++]=group_index;
          osal_memcpy(RadioBuf+RadioApp_TxLen,RxBuf,len+3);
          RadioApp_TxLen+=len+3;
          GenericApp_SendTheMessage(RadioBuf,RadioApp_TxLen);//发送数据到协调器
        }
      }
      HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
      SerialApp_TxLen=0;
    }
  }
}

uint16 CRC16_Check(uint8 *Pushdata,uint8 length)  
{  
  uint16 Reg_CRC=0xffff;  
  uint8 Temp_reg=0x00;  
  uint8 i,j;   
  for( i = 0; i<length; i ++)
  {  
	Reg_CRC^= *Pushdata++;  
	for (j = 0; j<8; j++)  
	{       
	if (Reg_CRC & 0x0001)  
		Reg_CRC=Reg_CRC>>1^0xA001;  
	else  
		Reg_CRC >>=1;  
   }
  }
  Temp_reg=Reg_CRC>>8;
  return (Reg_CRC<<8|Temp_reg);  
}

/*
void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{  
    switch ( pkt->clusterId )
    {
    case GENERICAPP_CLUSTERID:
      
        osal_memset(buf, 0 , 3);
        osal_memcpy(buf, pkt->cmd.Data, 2);
        if(buf[0]=='D' && buf[1]=='1')       
        {
            HalLedBlink(HAL_LED_1, 0, 50, 500);                                   
        }
        else
        {
            HalLedSet(HAL_LED_1,HAL_LED_MODE_ON);                   
        }
      
        break;
    }
}
*/