#include "ZComDef.h"
#include "hal_drivers.h"  //硬件驱动头文件
#include "OSAL.h"         //操作系统头文件
#include "OSAL_Tasks.h"   //操作系统任务头文件

#if defined ( MT_TASK )   //串口应用头文件
  #include "MT.h"
  #include "MT_TASK.h"
#endif

#include "nwk.h"          //网络层头文件
#include "APS.h"          //应用支持层头文件
#include "ZDApp.h"        //设备对象头文件

#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  #include "ZDNwkMgr.h"
#endif

#if defined ( ZIGBEE_FRAGMENTATION )
  #include "aps_frag.h"
#endif
#include "Common.h"

// 任务注册
const pTaskEventHandlerFn tasksArr[] = {
  macEventLoop,           //MAC任务循环
  nwk_event_loop,         //网络层任务函数
  Hal_ProcessEvent,       //硬件层函数
#if defined( MT_TASK )
  MT_ProcessEvent,        //串口支持层定义
#endif
  APS_event_loop,         //应用支持层任务事件函数
#if defined ( ZIGBEE_FRAGMENTATION )
  APSF_ProcessEvent,
#endif
  ZDApp_event_loop,       //设备对象层函数
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  ZDNwkMgr_event_loop,
#endif
  GenericApp_ProcessEvent //自己定义的任务处理函数
};

const uint8 tasksCnt = sizeof( tasksArr ) / sizeof( tasksArr[0] );
uint16 *tasksEvents;

void osalInitTasks( void )//完成了任务ID的分配，以及所有任务的初始化
{
  uint8 taskID = 0;
  //分配内存空间
  tasksEvents = (uint16 *)osal_mem_alloc( sizeof( uint16 ) * tasksCnt);
  osal_memset( tasksEvents, 0, (sizeof( uint16 ) * tasksCnt));

  macTaskInit( taskID++ );//MAC层的任务ID号
  nwk_init( taskID++ );   //网络ID分配
  Hal_Init( taskID++ );   //硬件ID分配
#if defined( MT_TASK )
  MT_TaskInit( taskID++ );
#endif
  APS_Init( taskID++ );
#if defined ( ZIGBEE_FRAGMENTATION )
  APSF_Init( taskID++ );
#endif
  ZDApp_Init( taskID++ );
#if defined ( ZIGBEE_FREQ_AGILITY ) || defined ( ZIGBEE_PANID_CONFLICT )
  ZDNwkMgr_Init( taskID++ );
#endif
  GenericApp_Init( taskID );//自己任务初始化函数
}

/*********************************************************************
*********************************************************************/
