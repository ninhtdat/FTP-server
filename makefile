CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm

all: server client

server: tcp-server status.o
	${CC} -pthread server.o status.o -o server

client: tcp-client status.o
	${CC} client.o status.o -o client

tcp-server: server.c
	${CC} ${CFLAGS} -o server.o server.c

tcp-client: client.c
	${CC} ${CFLAGS} -o client.o client.c

status: status.c
	${CC} ${CFLAGS} status.c

