/*
 * redis_handle.cpp
 *
 *  Created on: 2015年12月5日
 *      Author: yaobin
 */
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <net/if.h>
#include <time.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <hiredis/hiredis.h>
#include "../inc/redis_handle.hpp"
#include "../inc/udp_recv.hpp"
#include "../inc/circlequeue.hpp"
using namespace std;
#define StartByte1 0xAA
#define StartByte2 0x5A
#define StartByte3 0xA5
#define StartByte4 0xA0
#define EndByte 0xAA
#define ReadIntval 10000//us
typedef struct sensorpacket{
	uint32_t SensorAddr;
	unsigned char flag;
	float Co2;
	float Light;
	float Temp;
	float Humi;
	time_t timep;
}SensorPacket_t;
typedef struct getwaypacket{
	uint32_t GetwayAddr;
	int node_num;
	SensorPacket_t *sensordata;
}GetwayPacket_t;
extern RecvBuf_t *recvbuf;
redisContext *redis_c;
extern bool daemonmode;
extern bool recvout;
void redis_init(const char*redisip,const char*redisport)
{
    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
    redis_c= redisConnectWithTimeout(redisip, atoi(redisport), timeout);
    if (redis_c == NULL || redis_c->err) {
        if (redis_c) {
            printf("Connection error: %s\n", redis_c->errstr);
            redisFree(redis_c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        fflush(0);
        exit(0);
    }
	redisReply *reply;
	reply =(redisReply *) redisCommand(redis_c,"SELECT 8");
	freeReplyObject(reply);
}
void freeredis()
{
	redisFree(redis_c);
}
bool findhead(RecvBuf_t *recvbuf,int index)
{
	bool flag=false;
	if(recvbuf->RecvQueue.GetData(index++)==StartByte1&&
			recvbuf->RecvQueue.GetData(index++)==StartByte2&&
			recvbuf->RecvQueue.GetData(index++)==StartByte3&&
			recvbuf->RecvQueue.GetData(index++)==StartByte4)flag=true;
	return flag;
}
bool CheckSum(RecvBuf_t *recvbuf,int index)
{
	bool flag=false;
	int payloadlength=recvbuf->RecvQueue.GetData(index++);
	uint32_t sum=payloadlength;
	for(int i=0;i<payloadlength-5;i++)
		sum+=recvbuf->RecvQueue.GetData(index++);
	uint32_t check_sum=recvbuf->RecvQueue.GetData(index++)<<24;
	check_sum+=recvbuf->RecvQueue.GetData(index++)<<16;
	check_sum+=recvbuf->RecvQueue.GetData(index++)<<8;
	check_sum+=recvbuf->RecvQueue.GetData(index++);
	if(check_sum==sum&&recvbuf->RecvQueue.GetData(index)==EndByte)flag=true;
	return flag;
}
bool search(RecvBuf_t *recvbuf,int&preRIndex)
{
	bool flag=false;
	int rindex=recvbuf->RecvQueue.GetRIndex();
	for(int i=0;i<recvbuf->RecvQueue.GetRSize();i++)
	{
		if(findhead(recvbuf,rindex))
		{
			rindex+=4;
			if(recvbuf->RecvQueue.GetData(rindex)>=10&&CheckSum(recvbuf,rindex))
			{
				flag=true;
				preRIndex=rindex;
				int distance=rindex-recvbuf->RecvQueue.GetRIndex()+1;
				recvbuf->RecvQueue.DelDatas(recvbuf->RecvQueue.GetData(rindex)+distance);
				break;
			}
		}
		rindex++;
	}
	return flag;
}
void redis_store(GetwayPacket_t *getwaydata)
{
	redisReply *reply;
	reply =(redisReply *) redisCommand(redis_c,"SADD %s %x", "GetwayAddr", getwaydata->GetwayAddr);
	freeReplyObject(reply);
	for(int i=0;i<getwaydata->node_num;i++)
	{
		reply =(redisReply *) redisCommand(redis_c,"SADD %x %x", getwaydata->GetwayAddr, getwaydata->sensordata[i].SensorAddr);
		freeReplyObject(reply);
	}
	for(int i=0;i<getwaydata->node_num;i++)
	{
		if(getwaydata->sensordata[i].flag&0x01){
			reply =(redisReply *) redisCommand(redis_c,"HMSET %x %s %f", getwaydata->sensordata[i].SensorAddr,"Co2",getwaydata->sensordata[i].Co2);
			freeReplyObject(reply);
		}
		if(getwaydata->sensordata[i].flag&0x02){
			reply =(redisReply *) redisCommand(redis_c,"HMSET %x %s %f", getwaydata->sensordata[i].SensorAddr,"Light",getwaydata->sensordata[i].Light);
			freeReplyObject(reply);
		}
		if(getwaydata->sensordata[i].flag&0x04){
			reply =(redisReply *) redisCommand(redis_c,"HMSET %x %s %f", getwaydata->sensordata[i].SensorAddr,"Temperature",getwaydata->sensordata[i].Temp);
			freeReplyObject(reply);
		}
		if(getwaydata->sensordata[i].flag&0x08){
			reply =(redisReply *) redisCommand(redis_c,"HMSET %x %s %f", getwaydata->sensordata[i].SensorAddr,"Humidity",getwaydata->sensordata[i].Humi);
			freeReplyObject(reply);
		}
		char temp[25];
		strftime(temp,25,"%F %T",localtime(&getwaydata->sensordata[i].timep));
		reply =(redisReply *) redisCommand(redis_c,"HMSET %x %s %s", getwaydata->sensordata[i].SensorAddr,"Time",temp);
		freeReplyObject(reply);
		reply =(redisReply *) redisCommand(redis_c,"HMSET %x %s %s", getwaydata->sensordata[i].SensorAddr,"Status","1");
		freeReplyObject(reply);
	}
}
void analyse(RecvBuf_t *recvbuf,int preRIndex)
{
	GetwayPacket_t *getwaydata=new GetwayPacket_t;
	getwaydata->GetwayAddr=recvbuf->RecvQueue.GetData(preRIndex++)<<24;
	getwaydata->GetwayAddr+=recvbuf->RecvQueue.GetData(preRIndex++)<<16;
	getwaydata->GetwayAddr+=recvbuf->RecvQueue.GetData(preRIndex++)<<8;
	getwaydata->GetwayAddr+=recvbuf->RecvQueue.GetData(preRIndex++);
	getwaydata->node_num=recvbuf->RecvQueue.GetData(preRIndex++);
	getwaydata->sensordata=new SensorPacket_t[getwaydata->node_num];
	for(int i=0;i<getwaydata->node_num;i++)
	{
		getwaydata->sensordata[i].SensorAddr=recvbuf->RecvQueue.GetData(preRIndex++)<<24;
		getwaydata->sensordata[i].SensorAddr+=recvbuf->RecvQueue.GetData(preRIndex++)<<16;
		getwaydata->sensordata[i].SensorAddr+=recvbuf->RecvQueue.GetData(preRIndex++)<<8;
		getwaydata->sensordata[i].SensorAddr+=recvbuf->RecvQueue.GetData(preRIndex++);
		getwaydata->sensordata[i].flag=recvbuf->RecvQueue.GetData(preRIndex++);
		if(getwaydata->sensordata[i].flag&0x01){
			getwaydata->sensordata[i].Co2=(int)recvbuf->RecvQueue.GetData(preRIndex++);
			getwaydata->sensordata[i].Co2+=0.1*(int )recvbuf->RecvQueue.GetData(preRIndex++);
		}
		if(getwaydata->sensordata[i].flag&0x02){
			getwaydata->sensordata[i].Light=(int)recvbuf->RecvQueue.GetData(preRIndex++);
			getwaydata->sensordata[i].Light+=0.1*(int )recvbuf->RecvQueue.GetData(preRIndex++);
		}
		if(getwaydata->sensordata[i].flag&0x04){
			getwaydata->sensordata[i].Temp=(int)recvbuf->RecvQueue.GetData(preRIndex++);
			getwaydata->sensordata[i].Temp+=0.1*(int )recvbuf->RecvQueue.GetData(preRIndex++);
		}
		if(getwaydata->sensordata[i].flag&0x08){
			getwaydata->sensordata[i].Humi=(int)recvbuf->RecvQueue.GetData(preRIndex++);
			getwaydata->sensordata[i].Humi+=0.1*(int )recvbuf->RecvQueue.GetData(preRIndex++);
		}
		time(&getwaydata->sensordata[i].timep);
	}
	redis_store(getwaydata);
	if(daemonmode==false||recvout==true)
	{
		printf("|->GetwayAddress:0x%x\t",getwaydata->GetwayAddr);
		printf("nodenum:%d\n",getwaydata->node_num);
		for(int i=0;i<getwaydata->node_num;i++)
		{
			printf("   |->SensorAddress:0x%x\n",getwaydata->sensordata[i].SensorAddr);
			printf("      |->flag:0x%x\n",getwaydata->sensordata[i].flag);
			printf("      |->co2:%f\n",getwaydata->sensordata[i].Co2);
			printf("      |->light:%f\n",getwaydata->sensordata[i].Light);
			printf("      |->temp:%f\n",getwaydata->sensordata[i].Temp);
			printf("      |->humi:%f\n",getwaydata->sensordata[i].Humi);
			printf("      |->time:%s",asctime(localtime(&getwaydata->sensordata[i].timep)));
		}
	}
	delete []getwaydata->sensordata;
	delete []getwaydata;
}
void* datarestore(void *argv)
{
	bool flag=false;
	int preRIndex=-1;
	while(1)
	{
		usleep(ReadIntval);
		pthread_rwlock_rdlock(&recvbuf->rwlock);
		if(recvbuf->RecvQueue.GetRSize()>0)
			flag=search(recvbuf,preRIndex);
		pthread_rwlock_unlock(&recvbuf->rwlock);
		if(flag==true&&preRIndex>=0){
			flag=false;
			analyse(recvbuf,++preRIndex);
			preRIndex=-1;
		}
		fflush(0);
	}
	return NULL;
}


