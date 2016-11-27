/*
 * sample_client.cpp
 *
 *  Created on: 2015年11月22日
 *      Author: yaobin
 */
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
using namespace std;
const char*serverip="202.120.126.77";//服务器地址
//const char*serverip="127.0.0.1";//服务器地址
const char*serverport="8888";//服务器端口号
sockaddr_in s_addr;

#define StartByte1 0xAA//包头1
#define StartByte2 0x5A//包头2
#define StartByte3 0xA5//包头3
#define StartByte4 0xA0//包头4

#define EndByte	0xAA//包尾
/*
sensorpacket包括了从节点采集的数据的格式.
|--------------------------------------------|
| 节点地址 | 标志位 | Co2 | 光强 | 温度 | 湿度 |
|--------------------------------------------|
节点地址:4Bytes
标志位:1Byte,标志位flag表示后边传感器有无,flag二进制0b00000000,从右向左第一位置1表示有co2数据,第二位置1表示有光强数据,第二位置1表示有温度数据,第二位置1表示有湿度数据.
Co2:2Bytes, 前一个字节代表整数部分,后一个字节表示小数部分,精确度0.1,因此小数部分字节不要大于0x0a.如果没有该传感器,则没有改选项
光强:2Bytes,同上
温度:2Bytes,同上
湿度:2Bytes,同上
*/
typedef struct sensorpacket{
	uint8_t SensorAddr[4];//节点地址
	uint8_t flag;//标志位
	uint8_t Co2[2];//Co2
	uint8_t Light[2];//光强
	uint8_t Temp[2];//温度
	uint8_t Humi[2];//湿度
}SensorPacket_t;


/*
|-----------------------------------------------------------------|
| 包头 | 长度 | 网关地址 | 节点个数 |  sensorpacket | 校检和 | 包尾 |
|-----------------------------------------------------------------|
包头:4Bytes
长度:1Bytes,该值=网关地址长度+节点个数长度+sensorpacket长度+校检和长度+包尾长度,因此后面所有数据的长度不能达于255
网关地址:4Bytes
节点个数:1Bytes
sensorpacket:见SensorPacket_t
校检和:4Bytes,该值=长度值+网关地址值+节点个数值+sensorpacket值
包尾:1Bytes
*/
typedef struct getwaypacket{
	uint8_t head[4];//包头
	uint8_t payloadlen;//数据包长度,不包括包头.
	uint8_t GetwayAddr[4];//网关地址
	uint8_t SensorNum;//节点个数
	SensorPacket_t *sensordata;//节点数据
	uint8_t  check_sum[4];//从payloadlen到sensordata所有数据的校检和
	uint8_t tail;//包尾
}GetwayPacket_t;
void  GenerateData(GetwayPacket_t* getwaydata,int argc, char **argv);
void FormData(uint8_t *buf,GetwayPacket_t *getwaydata);
/*
函数名:init_data()
返回值:NONE
说明:初始化包头和包尾巴
*/
void init_data(GetwayPacket_t* getwaydata)
{
	getwaydata->head[0]=StartByte1;//包头1
	getwaydata->head[1]=StartByte2;//包头2
	getwaydata->head[2]=StartByte3;//包头3
	getwaydata->head[3]=StartByte4;//包头4
	getwaydata->tail=EndByte;//包尾
	getwaydata->sensordata=NULL;
}
int main(int argc, char **argv)
{
	time_t nowtime;
	if(argc<=2)return 0;
	uint8_t buff[1024];
	GetwayPacket_t* getwaydata=new GetwayPacket_t;
	init_data(getwaydata);
	int sockfd;
	if(-1==(sockfd=socket(AF_INET,SOCK_DGRAM,0)))//ipv4, udp
		return 0;
	int is_reuse_addr=1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&is_reuse_addr,sizeof(is_reuse_addr));
	memset(&s_addr,0,sizeof(sockaddr_in));
	s_addr.sin_family=AF_INET;//IPV4
	s_addr.sin_port=htons(atoi(serverport));
	s_addr.sin_addr.s_addr=inet_addr(serverip);
	int addr_len=sizeof(s_addr);
	int noize=6;//噪声长度
	while(1)
	{
		sleep(5);
		GenerateData(getwaydata,argc,argv);
		FormData(buff+noize,getwaydata);
time(&nowtime);
printf("time:%s",asctime(localtime(&nowtime)));
for(int i=0;i<buff[4+noize]+5+noize*2;i++)
	printf("%x ",buff[i]);
cout<<endl;
		fflush(0);
		sendto(sockfd,buff,buff[4]+5+noize*2,0,(sockaddr*)&s_addr,addr_len);
	}
	return 0;
}
/*
函数名:GenerateData()
返回值:NONE
说明:随机产生节点数据
*/
void  GenerateData(GetwayPacket_t* getwaydata,int argc, char **argv)
{
	getwaydata->GetwayAddr[0]=0x01;
	getwaydata->GetwayAddr[1]=0x01;
	getwaydata->GetwayAddr[2]=0x01;
	getwaydata->GetwayAddr[3]=atoi(argv[1]);
	getwaydata->SensorNum=argc-2;
	getwaydata->sensordata=new SensorPacket_t[getwaydata->SensorNum];
	for(int i=0;i<getwaydata->SensorNum;i++)
	{
		getwaydata->sensordata[i].SensorAddr[0]=0x01;
		getwaydata->sensordata[i].SensorAddr[1]=0x01;
		getwaydata->sensordata[i].SensorAddr[2]=0x01;
		getwaydata->sensordata[i].SensorAddr[3]=atoi(argv[i+2]);
		getwaydata->sensordata[i].flag=0xf;
		usleep(500000);
		srand( (unsigned)time(NULL) );
		getwaydata->sensordata[i].Co2[0]=rand()%(80-50)+50;
		getwaydata->sensordata[i].Co2[1]=rand()%10;
		usleep(500000);
		srand( (unsigned)time(NULL) );
		getwaydata->sensordata[i].Light[0]=rand()%(120-80)+80;
		getwaydata->sensordata[i].Light[1]=rand()%10;
		usleep(500000);
		srand( (unsigned)time(NULL) );
		getwaydata->sensordata[i].Temp[0]=rand()%(17-15)+15;
		getwaydata->sensordata[i].Temp[1]=rand()%10;
		usleep(500000);
		srand( (unsigned)time(NULL) );
		getwaydata->sensordata[i].Humi[0]=rand()%(100-60)+60;
		getwaydata->sensordata[i].Humi[1]=rand()%10;
	}
}
/*
函数名:FormData()
返回值:NONE
说明:对采集到的节点数据打包并和网关一起打包封装成完整的数据包
*/
void FormData(uint8_t *buf,GetwayPacket_t *getwaydata)
{
	uint32_t check_sum=0;
	int position=0;
	memcpy(buf+position,getwaydata->head,4);
	position+=4;
	buf[position++]=getwaydata->payloadlen;
	memcpy(buf+position,getwaydata->GetwayAddr,4);
	position+=4;
	buf[position++]=getwaydata->SensorNum;
	for(int i=0;i<getwaydata->SensorNum;i++)
	{
		memcpy(buf+position,getwaydata->sensordata[i].SensorAddr,4);
		position+=4;
		buf[position++]=getwaydata->sensordata[i].flag;
		if(getwaydata->sensordata[i].flag&0x01){
			memcpy(buf+position,getwaydata->sensordata[i].Co2,2);
			position+=2;
		}
		if(getwaydata->sensordata[i].flag&0x02){
			memcpy(buf+position,getwaydata->sensordata[i].Light,2);
			position+=2;
		}
		if(getwaydata->sensordata[i].flag&0x04){
			memcpy(buf+position,getwaydata->sensordata[i].Temp,2);
			position+=2;
		}
		if(getwaydata->sensordata[i].flag&0x08){
			memcpy(buf+position,getwaydata->sensordata[i].Humi,2);
			position+=2;
		}
	}
	buf[4]=getwaydata->payloadlen=position;
	for(int i=4;i<buf[4];i++)
		check_sum+=buf[i];
	buf[position++]=getwaydata->check_sum[0]=check_sum>>24&0xff;
	buf[position++]=getwaydata->check_sum[1]=check_sum>>16&0xff;
	buf[position++]=getwaydata->check_sum[2]=check_sum>>8&0xff;
	buf[position++]=getwaydata->check_sum[3]=check_sum&0xff;
	buf[position]=getwaydata->tail;
}
