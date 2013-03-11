#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<errno.h>
#include<malloc.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

#define HEADER_BUFFER_SIZE 16384
#define CONN_ALIVE_FIELD "Connection: keep-alive"
#define CONN_CLOSE_FIELD "Connection: closed"
#define PROXY_CONN_ALIVE_FIELD "Proxy-Connection: keep-alive"
#define PROXY_CONN_CLOSE_FIELD "Proxy-Connection: closed"
#define HOST_FIELD "Host: "
