#include "OSAL.h"
#include "AF.h"
#include "ZDApp.h"
#include "ZDObject.h"
#include "ZDProfile.h"

#include "Common.h"
#include "DebugTrace.h"

#if !defined( WIN32 )
  #include "OnBoard.h"
#endif

/* HAL */
#include "hal_lcd.h"
#include "hal_led.h"
#include "hal_key.h"
#include "hal_uart.h"
#include "OSAL_Nv.h"

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
static uint8 SerialApp_TxLen;
byte *eui64;

#define MAX_DEVICE 2//总共2个设备
#define MAX_GPRS_BUF_LEN  128
byte Radio_GPRS_Buf[MAX_GPRS_BUF_LEN];

void GenericApp_MessageMSGCB( afIncomingMSGPacket_t *pckt );
uint16 CRC16_Check(uint8 *Pushdata,uint8 length);
void rxCB(uint8 port,uint8 event);
void GenericApp_Init( byte task_id )
{
  //int i;
  /**/
    eui64=NLME_GetExtAddr();//get the EUI-64 Address
    /*
    for(i=0;i<MAX_DEVICE;i++)
    {
      Radio_GPRS_Buf[i][0]=SERIAL_APP_RADIO_HEAD1;
      Radio_GPRS_Buf[i][1]=SERIAL_APP_RADIO_HEAD2;
      osal_memcpy(Radio_GPRS_Buf[i]+2,eui64,8);//coordinator address
    }*/
    
    
    halUARTCfg_t uartConfig;
    uartConfig.configured           = TRUE;              // 2x30 don't care - see uart driver.
    uartConfig.baudRate             = HAL_UART_BR_115200;
    uartConfig.flowControl          = FALSE;
    uartConfig.flowControlThreshold = 64;   // 2x30 don't care - see uart driver.
    uartConfig.rx.maxBufSize        = 128;  // 2x30 don't care - see uart driver.
    uartConfig.tx.maxBufSize        = 128;  // 2x30 don't care - see uart driver.
    uartConfig.idleTimeout          = 6;    // 2x30 don't care - see uart driver.
    uartConfig.intEnable            = TRUE; // 2x30 don't care - see uart driver.
    uartConfig.callBackFunc         = rxCB;
    HalUARTOpen (0, &uartConfig);
    IO_OUTPUT_485;                //设置485/IO口为输出
    SEND_485;//receive
  /**/
    GenericApp_TaskID = task_id;//osal分配的任务ID随着用户添加任务的增多而改变
    GenericApp_TransID = 0;//消息发送ID（多消息时有顺序之分）
    
    //定义本设备用来通信的APS层端点描述符
    GenericApp_epDesc.endPoint = GENERICAPP_ENDPOINT;//应用程序的端口号
    GenericApp_epDesc.task_id = &GenericApp_TaskID;  //描述符的任务ID
    GenericApp_epDesc.simpleDesc                     //简单描述符
        = (SimpleDescriptionFormat_t *)&GenericApp_SimpleDesc;
    GenericApp_epDesc.latencyReq = noLatencyReqs;    //延时策略
    afRegister( &GenericApp_epDesc );                //向AF层登记描述符
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
                GenericApp_MessageMSGCB(MSGpkt);
                break;
            default:
                break;
            }
            osal_msg_deallocate( (uint8 *)MSGpkt );
            MSGpkt = (afIncomingMSGPacket_t *)osal_msg_receive( GenericApp_TaskID );
        }
        return (events ^ SYS_EVENT_MSG);
    }
    return 0;
} 
void GenericApp_MessageMSGCB(afIncomingMSGPacket_t *pkt)
{
    //HalLedBlink(HAL_LED_2, 1, 50, 500);//Led2闪烁4次，表示组网成功.
    uint8 uart_len=0;
    uint8 playload_len=0;
    uint16 crc;
    int i;
    switch ( pkt->clusterId )
    {
    case GENERICAPP_CLUSTERID:
        for(i=0;i<pkt->cmd.DataLength;i++)
        {
          if(pkt->cmd.Data[i]==SERIAL_APP_RADIO_HEAD1&&pkt->cmd.Data[i+1]==SERIAL_APP_RADIO_HEAD2)
          {
            Radio_GPRS_Buf[uart_len++]=SERIAL_APP_RADIO_HEAD1;//head
            Radio_GPRS_Buf[uart_len++]=SERIAL_APP_RADIO_HEAD2;//head
            playload_len=uart_len++;
            osal_memcpy(Radio_GPRS_Buf+uart_len,eui64,8);//coordinator address
            uart_len+=8;
            //osal_memcpy(Radio_GPRS_Buf+8, pkt->cmd.Data+2, pkt->cmd.DataLength-2); 
            //uart_len+=pkt->cmd.DataLength-2;
            osal_memcpy(Radio_GPRS_Buf+uart_len, pkt->cmd.Data+2, 10);//endvice address without head
            uart_len+=10;
            osal_memcpy(Radio_GPRS_Buf+uart_len, pkt->cmd.Data+14, pkt->cmd.DataLength-14); 
            uart_len+=pkt->cmd.DataLength-14;
            Radio_GPRS_Buf[playload_len]=uart_len-playload_len-1;//payload length
            crc=CRC16_Check(Radio_GPRS_Buf,uart_len);
            Radio_GPRS_Buf[uart_len++]=crc>>8&0xff;
            Radio_GPRS_Buf[uart_len++]=crc&0xff;
            HalUARTWrite(0, Radio_GPRS_Buf, uart_len);           
            break;
          }
        }
        /*
        if(pkt->cmd.Data[0]==SERIAL_APP_RADIO_HEAD1&&pkt->cmd.Data[1]==SERIAL_APP_RADIO_HEAD2)
        {
          
          Radio_GPRS_Buf[uart_len++]=SERIAL_APP_RADIO_HEAD1;//head
          Radio_GPRS_Buf[uart_len++]=SERIAL_APP_RADIO_HEAD2;//head
          playload_len=uart_len++;
          osal_memcpy(Radio_GPRS_Buf+uart_len,eui64,8);//coordinator address
          uart_len+=8;
          //osal_memcpy(Radio_GPRS_Buf+8, pkt->cmd.Data+2, pkt->cmd.DataLength-2); 
          //uart_len+=pkt->cmd.DataLength-2;
          osal_memcpy(Radio_GPRS_Buf+uart_len, pkt->cmd.Data+2, 10);//endvice address without head
          uart_len+=10;
          osal_memcpy(Radio_GPRS_Buf+uart_len, pkt->cmd.Data+14, pkt->cmd.DataLength-14); 
          uart_len+=pkt->cmd.DataLength-14;
          Radio_GPRS_Buf[playload_len]=uart_len-playload_len-1;//payload length
          crc=CRC16_Check(Radio_GPRS_Buf,uart_len);
          Radio_GPRS_Buf[uart_len++]=crc>>8&0xff;
          Radio_GPRS_Buf[uart_len++]=crc&0xff;
          HalUARTWrite(0, Radio_GPRS_Buf, uart_len);  
        }
        */
        break;
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

static void rxCB(uint8 port,uint8 event)
{
  if ((event & (HAL_UART_RX_FULL | HAL_UART_RX_ABOUT_FULL | HAL_UART_RX_TIMEOUT)) &&
#if SERIAL_APP_LOOPBACK
      (SerialApp_TxLen < SERIAL_APP_TX_MAX))
#else
      !SerialApp_TxLen)
#endif
  {
    //SerialApp_TxLen = HalUARTRead(0, RxBuf, SERIAL_APP_TX_MAX);
    if (SerialApp_TxLen)
    {
      //GenericApp_SendTheMessage();//发送数据到协调器
      //HalLedSet(HAL_LED_1,HAL_LED_MODE_OFF);
      //SerialApp_TxLen=0;
    }
  }
}
/*
void GenericApp_SendTheMessage(void)
{
    byte SendData[3]="D1";
    
    afAddrType_t devDstAddr;
    devDstAddr.addrMode=(afAddrMode_t)Addr16Bit;
    devDstAddr.endPoint=GENERICAPP_ENDPOINT;
    devDstAddr.addr.shortAddr=0xFFFF; 
    
    AF_DataRequest(&devDstAddr,
        &GenericApp_epDesc,
        GENERICAPP_CLUSTERID,
        2,
        SendData,
        &GenericApp_TransID,
        AF_DISCV_ROUTE,
        AF_DEFAULT_RADIUS);
}
*/




