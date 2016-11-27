
/*
 * sensor_monitor.cpp
 *
 *  Created on: 2015年11月25日
 *      Author: yaobin
 */
#include <iostream>
#include <string.h>
#include <hiredis/hiredis.h>
#include <stdlib.h>
#include <unistd.h>
#include "../inc/monitor_handle.hpp"
using namespace std;
extern redisContext *redis_c;
#define Monitor_Interval 1//min
const char fmt[] = "%Y-%m-%d %H:%M:%S";
typedef struct getway{
	string getwayaddr;
	string *sensoraddr;
	int sensor_num;
}Getway_t;
int CollectAddr(Getway_t *&getwaydata)
{
	int getway_num=0;
	redisReply *reply=NULL;
    reply = (redisReply *)redisCommand(redis_c,"SMEMBERS %s", "GetwayAddr");
    if (reply->type == REDIS_REPLY_ARRAY) {
    	getway_num=reply->elements;
		getwaydata=new Getway_t[getway_num];
        for (int i = 0; i < getway_num; i++) {
        	getwaydata[i].getwayaddr=reply->element[i]->str;
        }
    }
    freeReplyObject(reply);
    for (int i = 0; i < getway_num; i++) {
    	reply =(redisReply *) redisCommand(redis_c,"SMEMBERS %s", getwaydata[i].getwayaddr.data());
    	getwaydata[i].sensor_num=reply->elements;
    	getwaydata[i].sensoraddr=new string[getwaydata[i].sensor_num];
    	 for (int k= 0; k < getwaydata[i].sensor_num; k++) {
    		 getwaydata[i].sensoraddr[k]=reply->element[k]->str;
    	 }
    	freeReplyObject(reply);
    }
    return  getway_num;
}
Getway_t * freeaddr(Getway_t *getwaydata,int getway_num)
{
    for(int i=0;i<getway_num;i++)
    	delete []getwaydata[i].sensoraddr;
    delete []getwaydata;
    return NULL;
}
void analyseaddr(Getway_t *getwaydata,int getway_num)
{
	redisReply *reply=NULL;
    for(int i=0;i<getway_num;i++)
    {
    	for (int k= 0; k < getwaydata[i].sensor_num; k++)
    	{
			reply =(redisReply *) redisCommand(redis_c,"HGET %s %s",  getwaydata[i].sensoraddr[k].data(),"Status");
			int flag=strcmp(reply->str,"1");
			freeReplyObject(reply);
			if(flag==0)
			{
				reply =(redisReply *) redisCommand(redis_c,"HGET %s %s",  getwaydata[i].sensoraddr[k].data(),"Time");
				tm tb;
				strptime(reply->str, fmt, &tb);
				freeReplyObject(reply);
				time_t t1=mktime(&tb);
				time_t t2;
				time(&t2);
//printf("time_old:%s",asctime(localtime(&t1)));
//printf("time_new:%s",asctime(localtime(&t2)));
//cout<<difftime(t2,t1)<<endl;
//fflush(0);
				double interval=difftime(t2,t1);
				if(interval>=1.0*Monitor_Interval*60)
				{
					reply =(redisReply *) redisCommand(redis_c,"HMSET %s %s %s",  getwaydata[i].sensoraddr[k].data(),"Status","0");
					freeReplyObject(reply);
				}
			}
    	}
    }
}
void* sensormonitor(void *argv)
{
    int getway_num=0;
    Getway_t *getwaydata=NULL;
	while(1)
	{
		getway_num=CollectAddr(getwaydata);
		analyseaddr(getwaydata,getway_num);
		getwaydata=freeaddr(getwaydata,getway_num);
		sleep(Monitor_Interval*60);
	}

	return NULL;
}



