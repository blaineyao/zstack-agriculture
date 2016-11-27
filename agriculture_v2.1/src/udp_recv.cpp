/*
 * udp_recv.cpp
 *
 *  Created on: 2015年12月5日
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
#include <sys/stat.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
using namespace std;
#include "../inc/udp_recv.hpp"
#include "../inc/circlequeue.hpp"
#define MAX_BUF 2048
RecvBuf_t *recvbuf;
sockaddr_in s_addr;//store the server ip information
void GetWayInit(int &sockfd,const char*serverip,const char*serverport)
{
	if(-1==(sockfd=socket(AF_INET,SOCK_DGRAM,0)))//ipv4, udp
	{
		cerr<<"Create socket error"<<endl;
		exit(errno);
	}
	cout<<"Create socket successful"<<endl;
	int is_reuse_addr=1;
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,(const char*)&is_reuse_addr,sizeof(is_reuse_addr));
	memset(&s_addr,0,sizeof(sockaddr_in));
	s_addr.sin_family=AF_INET;//IPV4
	s_addr.sin_port=htons(atoi(serverport));
	s_addr.sin_addr.s_addr=inet_addr(serverip);
	//s_addr.sin_addr.s_addr=INADDR_ANY;
	if(-1==(bind(sockfd,(sockaddr*)&s_addr,sizeof(s_addr))))
	{
		cerr<<"Blind Socket error\n"<<endl;
		exit(errno);
	}
	cout<<"bind socket successful"<<endl;
}
void InitUdpBuffer()
{
	recvbuf=new RecvBuf_t;
	pthread_rwlock_init(&recvbuf->rwlock,NULL);
}

void freeRecvBuf(RecvBuf_t *recvbuf)
{
	delete recvbuf;
	pthread_rwlock_destroy(&recvbuf->rwlock);
}

void* getwayrecv(void *argv)
{
	unsigned char buff[1024];
	int socketfd=*(int*)argv;
	socklen_t addr_len=sizeof(sockaddr);
	sockaddr_in c_addr;
	int len=-1;
	while(1)
	{
		len=recvfrom(socketfd,buff,sizeof(buff)-1,0,(sockaddr*)&c_addr,&addr_len);
		if(len>-1)
		{
			if(0==pthread_rwlock_wrlock(&recvbuf->rwlock))
			{
				recvbuf->RecvQueue.InsertDatas(buff,len);
				pthread_rwlock_unlock(&recvbuf->rwlock);
			}
		}
		else break;
	}
	return NULL;
}



