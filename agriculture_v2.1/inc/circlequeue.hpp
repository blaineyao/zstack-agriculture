/*
 * circlequeue.hpp
 *
 *  Created on: 2015年12月5日
 *      Author: yaobin
 */

#ifndef CIRCLEQUEUE_HPP_
#define CIRCLEQUEUE_HPP_
#include <stdlib.h>
#include <pthread.h>
#define DEFAULT_CAPACITY 2048
template <class T>
class CircleQueue{
	int w_index;
	T *QueueHead;
	int capacity;
public:
	CircleQueue(int capacity=DEFAULT_CAPACITY){
		QueueHead=new T[capacity];
		if(QueueHead==NULL)this->capacity=0;
		else this->capacity=capacity;
		w_index=0;
	}
	void InserData(T data);
	void InsertDatas(T*data,int len);
	T GetData(int index);
	~CircleQueue(){
		delete []QueueHead;
		QueueHead=NULL;
		capacity=0;
		w_index=0;
	}
};
typedef struct QUEUESIZE{
	int len;
	pthread_mutex_t lock;
}QueueSize_t;
template <class T>
class CacheQueue{
	CircleQueue<T> queue;
	QueueSize_t size;
	int r_index;
public:
	CacheQueue(int capacity=DEFAULT_CAPACITY):queue(capacity)
	{
		r_index=0;
		size.len=0;
		pthread_mutex_init(&size.lock,NULL);
	}
	T GetData(int index)
	{
		return queue.GetData(index);
	}
	void GetDatas(T*data,int index,int len)
	{
		for(int i=0;i<len;i++)
			data[i]=queue.GetData(index+i);
	}
	int GetRIndex(){return r_index;}
	int GetRSize()
	{
		int size_temp=0;
		pthread_mutex_lock(&size.lock);
		size_temp=size.len;
		pthread_mutex_unlock(&size.lock);
		return size_temp;
	}
	void InsertData(T data);
	void InsertDatas(T *data,int len);
	void DelDatas(int len);
	~CacheQueue(){
		pthread_mutex_destroy(&size.lock);
	}
};
#endif /* CIRCLEQUEUE_HPP_ */
