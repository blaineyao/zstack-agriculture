/*
 * udp_recv.h
 *
 *  Created on: 2015年12月5日
 *      Author: yaobin
 */

#ifndef UDP_RECV_HPP_
#define UDP_RECV_HPP_

#include <stdlib.h>
#include <stdint.h>
#include "./circlequeue.hpp"
typedef struct RecvBuf{
	CacheQueue<unsigned char> RecvQueue;
	pthread_rwlock_t rwlock;
}RecvBuf_t;

void GetWayInit(int &sockfd,const char*serverip,const char*serverport);
void InitUdpBuffer();
void freeRecvBuf(RecvBuf_t *recvbuf);
void* getwayrecv(void *argv);


#endif /* UDP_RECV_HPP_ */
