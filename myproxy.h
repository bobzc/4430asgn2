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
#include<signal.h>
#include<ctype.h>

#define HEADER_BUFFER_SIZE 16384
#define CONN_ALIVE_FIELD "Connection: keep-alive"
#define CONN_CLOSE_FIELD "Connection: close"
#define PROXY_CONN_ALIVE_FIELD "Proxy-Connection: keep-alive"
#define PROXY_CONN_CLOSE_FIELD "Proxy-Connection: close"
#define PROXY_CONN_CLOSE_FIELD_B "Proxy-Connection: close\r\n"
#define CONTENT_LEN_FIELD "Content-Length: "
#define HOST_FIELD "Host: "


#define SOCK_ADDR_IN_PTR(sa) ((struct sockaddr_in *)(sa))
#define SOCK_ADDR_IN_PORT(sa) SOCK_ADDR_IN_PTR(sa) -> sin_port
#define SOCK_ADDR_IN_ADDR(sa) SOCK_ADDR_IN_PTR(sa) -> sin_addr


typedef struct 
{
	struct sockaddr sa;
	int fd;
	int is_taken;
} HOST_INFO;

