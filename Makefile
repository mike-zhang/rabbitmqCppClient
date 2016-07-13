CC = g++
CFLAGS = -g -O2 -Wall
OBJS = rabbitmqMgr.o testMgr_send1.o testMgr_recv1.o testMgr_rpcClient1.o testMgr_rpcServer1.o
all:$(OBJS)
	$(CC) -o testMgr_send1 testMgr_send1.o rabbitmqMgr.o  -lrabbitmq
	$(CC) -o testMgr_recv1 testMgr_recv1.o rabbitmqMgr.o  -lrabbitmq
	$(CC) -o testMgr_rpcClient1 testMgr_rpcClient1.o rabbitmqMgr.o  -lrabbitmq
	$(CC) -o testMgr_rpcServer1 testMgr_rpcServer1.o rabbitmqMgr.o  -lrabbitmq

clean:
	rm -f testMgr_send1 testMgr_recv1 testMgr_rpcClient1 testMgr_rpcServer1
	rm -f *.o
	
.cpp.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
