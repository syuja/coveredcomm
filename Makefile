#Makefile
CC=g++
CFLAGS= -std=c++11 -g
# CFLAGS= -std=c++11
LIBS = -pthread


#Bits to Transfer at a Time
NUM_PORTS = 64
#Server IP
SERV_IP = 144.174.173.65
#Location of SSH Keys
# KEYPATH = /home/syuja/.ssh
KEYPATH = ~/.ssh/covert_channel
SAVED = bin_file.txt
WAIT_TIME = 2


.Phony: all clean blink key bin2key runClient runServer

all: key bin2key blink setdefault


runServer: blink key
	./key $(NUM_PORTS) $(KEYPATH) | ./blink $(NUM_PORTS) $(WAIT_TIME)

runClient: client bin2key
	./client $(SERV_IP) $(NUM_PORTS) $(WAIT_TIME) $(SAVED) && ./bin2key $(SAVED)

#SERVER:
blink: blink.o
	$(CC) $(CFLAGS) $(LIBS) blink.o -o blink

blink.o:	blink.cpp
	$(CC) $(CFLAGS) blink.cpp -o blink.o -c

key: key.o
	$(CC) $(CFLAGS) key.o -o key

key.o: keyconv.cpp
	$(CC) $(CFLAGS) keyconv.cpp -o key.o -c


#CLIENT:
client: client.o
	$(CC) $(CFLAGS) $(LIBS) client.o -o client

client.o: client.cpp
	$(CC) $(CFLAGS)  client.cpp -o client.o -c

bin2key: bin2key.o
	$(CC) $(CFLAGS) bin2key.o -o bin2key

bin2key.o: bin2key.cpp
	$(CC) $(CFLAGS) bin2key.cpp -o bin2key.o -c


clean:
	rm -rf *.o bin_file.txt bin2key blink key client
