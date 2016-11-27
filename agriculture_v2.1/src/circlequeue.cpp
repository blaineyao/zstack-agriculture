/*
 * circlequeue.cpp
 *
 *  Created on: 2015年12月5日
 *      Author: yaobin
 */
#include "../inc/circlequeue.hpp"
template <class T>
void CircleQueue<T>::InserData(T data)
{
	if(QueueHead!=NULL)
	{
		QueueHead[w_index++]=data;
		w_index%=capacity;
	}
}
template <class T>
void CircleQueue<T>::InsertDatas(T*data,int len)
{
	if(data==NULL||len<=0)return;
	for(int i=0;i<len;i++)
		InserData(data[i]);
}
template <class T>
T CircleQueue<T>::GetData(int index)
{
	if(QueueHead!=NULL)return QueueHead[index%capacity];
	else return -1;
}

template <class T>
void CacheQueue<T>::InsertData(T data)
{
	queue.InserData(data);
	pthread_mutex_lock(&size.lock);
	size.len++;
	pthread_mutex_unlock(&size.lock);
}
template <class T>
void CacheQueue<T>::InsertDatas(T*data,int len)
{
	queue.InsertDatas(data,len);
	pthread_mutex_lock(&size.lock);
	size.len+=len;
	pthread_mutex_unlock(&size.lock);
}
template <class T>
void CacheQueue<T>::DelDatas(int len)
{
	pthread_mutex_lock(&size.lock);
	if(len<=size.len)
	{
		r_index+=len;
		size.len-=len;
	}
	else
	{
		r_index+=size.len;
		size.len=0;
	}
	pthread_mutex_unlock(&size.lock);
}
template class CircleQueue<unsigned char>;
template class CacheQueue<unsigned char>;
