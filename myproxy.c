#include"myproxy.h"

int tn = 0;

HOST_INFO host_info[10];
pthread_mutex_t mutex;
pthread_mutex_t fd_mutex[1000];

int recv_req(int fd, char *buf);
void get_host(char *buf_begin, char *buf_end, struct addrinfo **host_addr);
int recv_rpn(int server_fd, char *recv_buf);
int get_cont_len(char *buf_begin, char *buf_end);
int get_header_len(char *buf);
int send_rpn(int client_fd, int server_fd, char *recv_buf, int count_now, int cont_len, int header_len);

/*******************************************************************************************/
/*******************************************************************************************/
/******************************* MILESTONE ONE BEGIN ***************************************/


/* case insensitive compare*/
int strncmp_i(char *a, char *b, int len){
	int i = 0;
	for(i = 0; i < len; i++){
		if(toupper(a[i]) != toupper(b[i]))
			return 1;
	}
	return 0;
}

/* get the value of a field */
char *get_field_value(char *begin, char *end){
	char *ret = (char *)calloc((int)(end - begin) + 1, sizeof(char));
	memcpy(ret, begin, (int)(end - begin));
	ret[(int)(end - begin)] = '\0';
	return ret;
}

/* modify the value of a field*/
void mod_field(char *begin, char *end, int length, char *content){
	char *from = end;
	char *to = begin + strlen(content);
	memmove(to, from, length);
	memcpy(begin, content, strlen(content));
}

/* process response */
int mod_rpn(char *buf_begin, char *buf_end){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	int has_proxy_field = 0;
	int has_conn_field = 0;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		/* process Connection, Proxy-Connection and content-length field */
		if(strncmp_i(field_begin, CONN_ALIVE_FIELD, strlen(CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(CONN_ALIVE_FIELD) - strlen(CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
			has_conn_field = 1;
		}else if(strncmp_i(field_begin, PROXY_CONN_ALIVE_FIELD, strlen(PROXY_CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(PROXY_CONN_ALIVE_FIELD) - strlen(PROXY_CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
			has_proxy_field = 1;
		}
	}while(field_len != 0);
	/* if the header do not have proxy-connection field, append one */
	if(!has_proxy_field && !has_conn_field){
		mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD_B);
		buf_end += strlen(PROXY_CONN_CLOSE_FIELD_B);
		field_end += strlen(PROXY_CONN_CLOSE_FIELD_B);
	}

	return (int)(buf_end - buf_begin);
}


/* resolve host according to field */
void resolve_host(char *field, struct addrinfo **host_addr){
		char *domain = strtok(field, ":");
		char *port = strtok(NULL, ":");
		if(port == NULL)
			port = "80";

		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;
		if(getaddrinfo(domain, port, &hints, host_addr) != 0){
			perror("Error in getaddrinfo().");
			pthread_exit(NULL);
		}
		free(field);
}

int conn_srv(struct sockaddr *sa){
	int fd = 0;
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("Cannot initialize socket.");
		pthread_exit(NULL);
	}
	
	if(connect(fd, (struct sockaddr *)sa, sizeof(struct sockaddr)) == -1){
		perror("Cannot connect to server.");
		pthread_exit(NULL);
	}
	printf("Connecting to host...\n");
	return fd;
}


/* process request, and get host address and return header length */

int mod_req(char *buf_begin, char *buf_end){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		/* handle connection, proxy-connection, host field */
		if(strncmp_i(field_begin, CONN_ALIVE_FIELD, strlen(CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(CONN_ALIVE_FIELD) - strlen(CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
		}else if(strncmp_i(field_begin, PROXY_CONN_ALIVE_FIELD, strlen(PROXY_CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(PROXY_CONN_ALIVE_FIELD) - strlen(PROXY_CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
		}
	}while(field_len != 0);	
	return (int)(buf_end - buf_begin);
}



void milestone_1(int fd){
	char buf[HEADER_BUFFER_SIZE];
	int buf_len = recv_req(fd, buf);

	if(buf_len <= 0){
		perror("No request...");
		pthread_exit("NULL");
	}	

	
	char *req_begin_next = buf;
	char *req_begin = buf;
	char *req_end = buf;
	while(1){
		struct addrinfo *host_addr;	
		int count = 0, total_count;
		int server_fd;		
		

		char *tmp;
		req_begin = req_begin_next;
	//	write(1, req_begin, 400);
		if((tmp = strstr(req_begin, "\r\n\r\n")) == NULL){
			break;	
		}
		req_end = tmp + 4;
		req_begin_next = req_end;

		/* get host address */
		get_host(req_begin, req_end, &host_addr);

		if(host_addr == NULL){
			perror("Cannot get host addr.");
			pthread_exit(NULL);
		}
	
		/* change connection and proxy-connection value to close*/
		req_end = mod_req(req_begin, req_end) + req_begin;
	
		server_fd = conn_srv((struct sockaddr *)host_addr -> ai_addr);
	
		write(1, req_begin, (int)(req_end - req_begin) - count);
		while(count < (int)(req_end - req_begin)){
			count += send(server_fd, req_begin + count, (int)(req_end - req_begin) - count, 0);
		}
		printf("Forward request...(%d Bytes)\n", (int)(req_end - req_begin));
		
		/* receive response */
		char recv_buf[HEADER_BUFFER_SIZE];
		total_count = recv_rpn(server_fd, recv_buf);
		if(total_count == 0){
			perror("End of response...");
			pthread_exit(NULL);
		}		

		/* get content length, header length and check connection field */
		total_count = mod_rpn(recv_buf, recv_buf + total_count);	
		int cont_len = get_cont_len(recv_buf, recv_buf + total_count);
		int header_len = get_header_len(recv_buf);
	
		/* send response to client */
		int sent_cnt = send_rpn(fd, server_fd, recv_buf, total_count, cont_len, header_len);
		printf("Forward response...");
		printf("(Header: %d Bytes) (Content: %d Bytes) (Total: %d Bytes)\n", 
			header_len, cont_len, sent_cnt + header_len);
		close(server_fd);	
	}
}

/******************************* MILESTONE ONE END *****************************************/
/*******************************************************************************************/
/*******************************************************************************************/





/*******************************************************************************************/
/*******************************************************************************************/
/******************************* MILESTONE TWO START *****************************************/


/* compare ip addr and port num */

int sock_addr_cmp_addr(const struct sockaddr *sa, const struct sockaddr *sb){
	if(sa -> sa_family != sb -> sa_family){
		return (sa -> sa_family - sb -> sa_family);
	}
	if(sa -> sa_family == AF_INET){
		return (SOCK_ADDR_IN_ADDR(sa).s_addr - SOCK_ADDR_IN_ADDR(sb).s_addr);
	}
}

int sock_addr_cmp_port(const struct sockaddr *sa, const struct sockaddr *sb){
	if(sa -> sa_family != sb -> sa_family){
		return (sa -> sa_family - sb -> sa_family);
	}
	if(sa -> sa_family == AF_INET){
		return (SOCK_ADDR_IN_PORT(sa) - SOCK_ADDR_IN_PORT(sb));
	}	
}


/* get connection and proxy-connection field value */
int is_conn_alive(char *buf_begin, char *buf_end){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		if(strncmp_i(field_begin, CONN_CLOSE_FIELD, strlen(CONN_CLOSE_FIELD)) == 0){
			return 0;
		}else if(strncmp_i(field_begin, PROXY_CONN_CLOSE_FIELD, strlen(PROXY_CONN_CLOSE_FIELD)) == 0){
			return 0;
		}
	}while(field_len != 0);
	return 1;
}

/* get content length */
int get_cont_len(char *buf_begin, char *buf_end){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		if(strncmp_i(field_begin, CONTENT_LEN_FIELD, strlen(CONTENT_LEN_FIELD)) == 0){
			char *val = get_field_value(field_begin + strlen(CONTENT_LEN_FIELD), field_end);
			return atoi(val);
		}
	}while(field_len != 0);
	return 0;
}


/* get heafer length */
int get_header_len(char *buf){
	char *tmp = strstr(buf, "\r\n\r\n");
	if(tmp == NULL){
		perror("Bad response.");
		return 0;
	}
	return (int)(tmp - buf) + 4;
}

/* get host addr */
void get_host(char *buf_begin, char *buf_end, struct addrinfo **host_addr){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		if(strncmp_i(field_begin, HOST_FIELD, strlen(HOST_FIELD)) == 0){
			char *val = get_field_value(field_begin + strlen(HOST_FIELD), field_end);
			resolve_host(val, host_addr);
			return;
		}
	}while(field_len != 0);
}

/* find a host fd for forwarding request */
int find_fd(struct sockaddr *sa){
	pthread_mutex_lock(&mutex);

	int fd = 0;
	int slot = -1;

	/* check if the host is connected */
	for(int i = 0; i < 10; i++){
		if(host_info[i].is_taken == 0){
			slot = i;
			continue;
		}else if(sock_addr_cmp_addr(sa, &host_info[i].sa) == 0
			&& sock_addr_cmp_port(sa, &host_info[i].sa) == 0){
			printf("Reusing tcp connection...\n");
			pthread_mutex_unlock(&mutex);
			return host_info[i].fd;
		}
	}
		
	/* if not connect to the host */
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("Cannot initialize socket.");
		pthread_mutex_unlock(&mutex);
		pthread_exit(NULL);
	}
	if(connect(fd, (struct sockaddr *)sa, sizeof(struct sockaddr)) == -1){
		perror("Cannot connect to server.");
		pthread_mutex_unlock(&mutex);
		pthread_exit(NULL);
	}
	printf("Connecting to host...\n");

	/* allocate a place to store the host info */	
	if(slot == -1){
		int slot = rand() % 10;
	}
	
	host_info[slot].fd = fd;
	host_info[slot].is_taken = 1;
	memcpy(&host_info[slot].sa, sa, sizeof(struct sockaddr));

	pthread_mutex_unlock(&mutex);
	return fd;
}


/* close fd */
void close_conn_host(int fd){
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < 10; i++){
		if(host_info[i].is_taken && host_info[i].fd == fd){
			host_info[i].is_taken = 0;
			close(fd);
			printf("Close connection to host...\n");
		}
	}
	pthread_mutex_unlock(&mutex);
}


/* receive request from client */
int recv_req(int fd, char *buf){
		int total_count = 0, count;
		do{
			count = recv(fd, buf + total_count, HEADER_BUFFER_SIZE - total_count, 0);
			total_count += count;
		}while(total_count > 0 && strstr(buf, "\r\n\r\n") == NULL);

		if(total_count < 0){
			perror("Receive request error.");
			pthread_mutex_unlock(&fd_mutex[fd]);
			pthread_exit(NULL);
		}
		if(total_count == 0){
			perror("End of request.");
			pthread_mutex_unlock(&fd_mutex[fd]);
			pthread_exit(NULL);
		}
		printf("Receive requests...(%d Bytes)\n", total_count);
}

/* receive response from server */
int recv_rpn(int server_fd, char *recv_buf){
		memset(recv_buf, 0, HEADER_BUFFER_SIZE);
		int total_count = 0;

		total_count += recv(server_fd, recv_buf + total_count, HEADER_BUFFER_SIZE - 256 - total_count, 0);
			

		if(total_count < 0){
			perror("Read Error.");
			pthread_mutex_unlock(&fd_mutex[server_fd]);
			pthread_exit(NULL);
		}
		if(total_count == 0){
			printf("Connection is down...\n");
		}	

		return total_count;
}

/* send response to client */
int send_rpn(int client_fd, int server_fd, char *recv_buf, int count_now, int cont_len, int header_len){
	int	count = 0;	
	while(count < count_now){
		count += send(client_fd, recv_buf + count, count_now - count, 0);
	}
	int sent_cnt = count_now - header_len;
	while(sent_cnt < cont_len){
		int recv_cnt = recv(server_fd, recv_buf, sizeof(recv_buf), 0);
		send(client_fd, recv_buf, recv_cnt, 0);
		sent_cnt += recv_cnt;
	}
	return sent_cnt;
}


void milestone_2(int fd){
	int count, total_count = 0;
	int conn_flag = 1;
	struct addrinfo *host_addr;
	int server_fd;

	char buf[HEADER_BUFFER_SIZE];
	
	while(conn_flag){
		/* receive request */
		memset(buf, 0, HEADER_BUFFER_SIZE);
		printf("Waiting for requests...\n");
		recv_req(fd, buf);
		
		char *req_begin = buf;
		char *req_end = buf;
		while(1){
			char *tmp;
			req_begin = req_end;
			if((tmp = strstr(req_begin, "\r\n\r\n")) == NULL){
				break;	
			}
			req_end = tmp + 4;

			printf("Handling request...(%d Bytes)\n", (int)(req_end - req_begin));

			/* check connection field */
			conn_flag = is_conn_alive(req_begin, req_end);

			/* get host address */
			get_host(req_begin, req_end, &host_addr);
	
			if(host_addr == NULL){
				perror("Cannot get host addr.");
				pthread_exit(NULL);
			}
	
			/* mapping host addr */
			server_fd = find_fd((struct sockaddr *)host_addr->ai_addr);
			pthread_mutex_lock(&fd_mutex[server_fd]);

			/* foward request */
			send(server_fd, req_begin, (int)(req_end - req_begin), 0);
			printf("Forward request...(%d Bytes)\n", (int)(req_end - req_begin));

			/* receive response */
			char recv_buf[HEADER_BUFFER_SIZE];
			total_count = recv_rpn(server_fd, recv_buf);

			/* if host has closed the connection */
			if(total_count == 0){
				/* close the connection */
				close_conn_host(server_fd);
				pthread_mutex_unlock(&fd_mutex[server_fd]);
			
				/* re-find a fd */
				server_fd = find_fd((struct sockaddr *)host_addr->ai_addr);
				pthread_mutex_lock(&fd_mutex[server_fd]);
			
				/* forward request again */
				send(server_fd, req_begin, (int)(req_end - req_begin), 0);
				printf("Re-Forward request...(%d Bytes)\n", (int)(req_end - req_begin));

				/* receive response */
				total_count = recv_rpn(server_fd, recv_buf);
			}	

			/* get content length, header length and check connection field */
			int cont_len = get_cont_len(recv_buf, recv_buf + total_count);
			int header_len = get_header_len(recv_buf);
			int host_conn_flag = is_conn_alive(recv_buf, recv_buf + total_count);
			
			/* send response to client */
			int sent_cnt = send_rpn(fd, server_fd, recv_buf, total_count, cont_len, header_len);
			printf("Forward response...");
			printf("(Header: %d Bytes) (Content: %d Bytes) (Total: %d Bytes)\n", 
				header_len, cont_len, sent_cnt + header_len);
	
			if(host_conn_flag == 0){	
				close_conn_host(server_fd);
			}	

			pthread_mutex_unlock(&fd_mutex[server_fd]);
		}
	}
}


/******************************* MILESTONE TWO END *****************************************/
/*******************************************************************************************/
/*******************************************************************************************/



void cleanup(){
	printf("thread num: %d\n", --tn);
	printf("\n################# thread end ####################\n");
}

void *run(void *args){
	pthread_cleanup_push(cleanup, NULL);
	printf("\n\n################# thread begin ######################\n");
	printf("thread num: %d\n", ++tn);
	int fd = *((int *)args);
	int milestone = *((int *)args + 1);
	if(milestone == 1)
		milestone_1(fd);
	if(milestone == 2)
		milestone_2(fd);
	pthread_cleanup_pop(1);
}

int main(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);
	if(argc != 3){
		printf("Usage: %s [port] [milestone]\n", argv[0]);
		exit(1);
	}
	pthread_mutex_init(&mutex, NULL);
	for(int i = 0; i < 1000; i++)
		pthread_mutex_init(&fd_mutex[i], NULL);

	int fd;
	int port = atoi(argv[1]), milestone = atoi(argv[2]);
	struct sockaddr_in  addr, tmp_addr;
	unsigned int addrlen = sizeof(struct sockaddr_in);
	
	if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
		perror("Error in creating socket.");
		exit(1);
	}
	
	memset(&addr, addrlen, 0);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	
	if(bind(fd, (struct sockaddr *) &addr, addrlen) == -1){
		perror("Error in bind().");
		exit(1);
	}
	
	if(listen(fd, 1024) == -1){
		perror("Error in listen().");
		exit(1);
	}
	while(1){
		int accept_fd;
		if((accept_fd = accept(fd, (struct sockaddr *) &tmp_addr, &addrlen)) == -1){
			perror("Error in accept().");
			exit(1);
		}

		pthread_t tid;
		pthread_attr_t attr;
		if(pthread_attr_init(&attr)){
			perror("Error: pthread_attr_init.");
			exit(1);
		}
		if(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)){
			perror("Error: pthread_attr_setdetachstate.");
			exit(1);
		}
	
		int pthread_arg[2];
		pthread_arg[0] = accept_fd;
		pthread_arg[1] = milestone;		
		pthread_create(&tid, &attr, run, pthread_arg);
	}
}
