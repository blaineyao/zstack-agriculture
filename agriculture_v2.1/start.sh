#!/bin/bash
localip=202.120.126.77
localport=8888
RedisServerIp=127.0.0.1
RedisServerPort=6379
TARGET=agriculture
echo "export the lib"
LD_LIBRARY_PATH="./lib" 
export LD_LIBRARY_PATH
echo "|-------------------start the agriculture-------------------|"
killall $TARGET
chmod u+x $TARGET
./$TARGET -l $localip -p $localport -R $RedisServerIp -P $RedisServerPort 1>$TARGET.log 2>$TARGET.log
netstat -unlp | grep $TARGET
echo "|-------------------exit-------------------|"
ps -aux | grep agriculture
