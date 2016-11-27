#copy right:shanghai university
#author:yao bin
#date:2015/12/6
CC=g++
TARGET=../agriculture
INCDIR = ../inc
SRC = agriculture.cpp redis_handle.cpp udp_recv.cpp monitor_handle.cpp circlequeue.cpp
INC = $(INCDIR)/redis_handle.hpp $(INCDIR)/udp_recv.hpp $(INCDIR)/monitor_handle.hpp $(INCDIR)/circlequeue.hpp
OBJECT=agriculture.o redis_handle.o udp_recv.o monitor_handle.o circlequeue.o
LDFLAGS =-lhiredis -lpthread

all:$(TARGET)
$(OBJECT):$(SRC) $(INC)
	$(CC) -c ${SRC} $(INC)
$(TARGET):$(OBJECT)
	$(CC) -o $@ $(OBJECT) $(LDFLAGS)
clean:
	rm -rf $(OBJECT) $(TARGET) *~
