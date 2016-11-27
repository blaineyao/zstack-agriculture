/*
 * redis_handle.hpp
 *
 *  Created on: 2015年12月5日
 *      Author: yaobin
 */

#ifndef REDIS_HANDLE_H_
#define REDIS_HANDLE_H_

void redis_init(const char*redisip,const char*redisport);
void freeredis();
void* datarestore(void *argv);


#endif /* REDIS_HANDLE_H_ */
