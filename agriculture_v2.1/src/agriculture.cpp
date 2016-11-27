/*
 * agriculture.cpp
 *
 *  Created on: 2015年11月21日
 *      Author: yaobin
 */
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>

#include "../inc/monitor_handle.hpp"
#include "../inc/redis_handle.hpp"
#include "../inc/udp_recv.hpp"
using namespace std;
bool daemonmode=true;
bool recvout=false;
extern RecvBuf_t *recvbuf;
char LocalIp[20];
char LocalPort[10];
char RedisServerIp[20];
char RedisServerPort[10];
void init_daemon();
void parse_command(int argc, char **argv);
void exitprogram();
int main(int argc, char **argv)
{
	parse_command(argc,argv);
	int sockfd;
	if(daemonmode==true)init_daemon();
	GetWayInit(sockfd,LocalIp,LocalPort);
	redis_init(RedisServerIp,RedisServerPort);
	InitUdpBuffer();
	fflush(0);
	atexit(exitprogram);

	pthread_t getwayrecv_t;
	pthread_t datarestore_t;
	pthread_t sensormonitor_t;
	pthread_create(& getwayrecv_t,NULL, getwayrecv,&sockfd);
	pthread_create(& datarestore_t,NULL, datarestore,NULL);
	pthread_create(& sensormonitor_t,NULL, sensormonitor,NULL);
	pthread_join(datarestore_t,NULL);
	pthread_join(getwayrecv_t,NULL);
	pthread_join(sensormonitor_t,NULL);

}
void usage()
{
	cout<<"*Copyright (c) 2015 Shanghai University*"<<endl;
	cout<<"*AUTHOR:Yao Bin*"<<endl;
	cout<<"*Email:1349970640@qq.com*"<<endl;
	cout<<"usage:"<<endl;
	cout<<"      -l <local udp  ip>"<<endl;
	cout<<"      -p <locap udp port>"<<endl;
	cout<<"      -R <redis server ip>"<<endl;
	cout<<"      -R <redis server port,default is 6379>"<<endl;
	cout<<"      [-h] <help information>"<<endl;
	cout<<"      [-X] run in debug mode"<<endl;
	cout<<"      [-x] output the receive messages"<<endl;
	cout<<"examples:"<<endl;
	cout<<"       ./agriculture -x -l 127.0.0.1 -p 6001 -R 127.0.0.1 -P 6379"<<endl;
}
void parse_command(int argc, char **argv)
{
	  char ret;
	  while ((ret=getopt(argc, argv, "l:p:R:P:hXx")) != -1)
	  {
		  switch (ret)
		  {
		  case 'l':
			  strcpy(LocalIp, optarg);
			  break;
		  case 'p':
			  strcpy(LocalPort, optarg);
			  break;
		  case 'R':
			  strcpy(RedisServerIp, optarg);
			  break;
		  case 'P':
			  strcpy(RedisServerPort, optarg);
			  break;
		  case 'X':
			  daemonmode=false;
			  break;
		  case 'x':
			  recvout=true;
			  break;
		  case 'h':
		  default:
			  usage();
			  exit(0);
			  break;
		  }
	  }
}
void init_daemon()
{
	int pid;
	signal(SIGTTOU,SIG_IGN);
	signal(SIGTTIN,SIG_IGN);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGHUP ,SIG_IGN);
	if(0!=(pid=fork()))
		exit(EXIT_SUCCESS);
	else if(pid< 0){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	setsid();
	if(0!=(pid=fork()))
		exit(EXIT_SUCCESS);
	else if(pid< 0){
		perror("fork");
		exit(EXIT_FAILURE);
	}
	//for(int i=0;i< NOFILE;++i)
	//	close(i);
	//chdir(dir);
	umask(0);
	signal(SIGCHLD,SIG_IGN);
}
void exitprogram()
{
	freeRecvBuf(recvbuf);
	freeredis();
}



