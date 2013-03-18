#include"myproxy.h"


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
int proc_rpn(char *buf_begin, char *buf_end, int *cont_len, int *header_len){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	int has_proxy_field = 0;
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
		}else if(strncmp_i(field_begin, PROXY_CONN_ALIVE_FIELD, strlen(PROXY_CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(PROXY_CONN_ALIVE_FIELD) - strlen(PROXY_CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
			has_proxy_field = 1;
		}else if(strncmp_i(field_begin, CONTENT_LEN_FIELD, strlen(CONTENT_LEN_FIELD)) == 0){
			char *val = get_field_value(field_begin + strlen(CONTENT_LEN_FIELD), field_end);
			*cont_len = atoi(val);
		}
	}while(field_len != 0);
	/* if the header do not have proxy-connection field, append one */
	if(!has_proxy_field){
		mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD_B);
		buf_end += strlen(PROXY_CONN_CLOSE_FIELD_B);
		field_end += strlen(PROXY_CONN_CLOSE_FIELD_B);
	}
	
	*header_len = (int)(field_end - buf_begin) + 2;
	return (int)(buf_end - buf_begin);
}

/* forward response */
void fwd_rpn(int client_fd, int server_fd){
	int count;
	int header_len = 0;
	int cont_len = 0;
	int buf_len;
	char buf[HEADER_BUFFER_SIZE];
	/* receive response from web server */
	memset(buf, 0, HEADER_BUFFER_SIZE);
	if((count = read(server_fd, buf, HEADER_BUFFER_SIZE - 256)) < 0){
		perror("Read error.");
		pthread_exit(NULL);
	}

	buf_len = proc_rpn(buf, buf + count, &cont_len, &header_len);
/*	printf("Modified Response header:\n");
	write(1, buf, header_len);
	printf("-----------------------------------------\n");
	printf("Received: %d\n", count);
	printf("Response header length: %d\n", header_len);
	printf("Response content length: %d\n", cont_len);
	printf("Response buffer length: %d\n", buf_len);
	printf("-----------------------------------------\n");
*/
	/* send response to browser */
	count = 0;	
	while(count < buf_len){
		count += write(client_fd, buf, buf_len);
	}
	int sent_cnt = buf_len - header_len;
	while(sent_cnt < cont_len){
		int recv_cnt = read(server_fd, buf, sizeof(buf));
		write(client_fd, buf, recv_cnt);
		sent_cnt += recv_cnt;
	}
//	printf("Forwarded content length: %d\n", sent_cnt);
//	printf("-----------------------------------------\n");
}


/* forward request and return fd */
int fwd_req(char *buf, int count, struct addrinfo *host_addr){
	struct addrinfo *p_host_addr = host_addr;
	int i;
	for(i = 0; p_host_addr != NULL;p_host_addr = p_host_addr -> ai_next, i++){
		struct sockaddr_in *sa = (struct sockaddr_in *)p_host_addr->ai_addr;
		int fd;
		if((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
			perror("Cannot initialize socket.");
			continue;
		}
		if(connect(fd, (struct sockaddr *)sa, sizeof(struct sockaddr_in)) == -1){
			perror("Cannot connect to server.");
			continue;
		}
		write(fd, buf, count);
		return fd;
	}
	return -1;
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

/* process request, and get host address and return header length */

int proc_req(char *buf_begin, char *buf_end, struct addrinfo **host_addr){
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
		}else if(strncmp_i(field_begin, HOST_FIELD, strlen(HOST_FIELD)) == 0){
			char *val = get_field_value(field_begin + strlen(HOST_FIELD), field_end);
			resolve_host(val, host_addr);
		}
	}while(field_len != 0);	
	return (int)(buf_end - buf_begin);
}



void milestone_1(int fd){
	int count;
	int total_count = 0;
	char buf[HEADER_BUFFER_SIZE];
	memset(buf, 0, HEADER_BUFFER_SIZE);
	int wait_max = 0;
	while(total_count <= 100){
		total_count = read(fd, buf, HEADER_BUFFER_SIZE);
	}
	if(total_count <= 0)
		pthread_exit(NULL);
	printf("Get request header(%d):\n", total_count);
//	write(1, buf, total_count);
//	printf("-----------------------------------------\n");
	/* process request header, and get host address */
	struct addrinfo *host_addr;
	total_count = proc_req(buf, buf + total_count, &host_addr);
	
/*	printf("Processed request header(%d):\n", total_count);
	write(1, buf, total_count);
	printf("-----------------------------------------\n");
*/

/*	printf("Resolve host: \n");
	struct addrinfo *p_host_addr = host_addr;
	int i;
	for(i = 0; p_host_addr != NULL;p_host_addr = p_host_addr -> ai_next){
		struct sockaddr_in *sa = (struct sockaddr_in *)p_host_addr->ai_addr;
		printf("[%d] %s:%hd\n", i++,inet_ntoa(sa->sin_addr),ntohs(sa->sin_port));
	}
	printf("-----------------------------------------\n");
*/
	int server_fd;
	if((server_fd = fwd_req(buf, total_count, host_addr)) != -1){
		printf("Forward request.\n");
		fwd_rpn(fd, server_fd);
		printf("Response finished\n");
		printf("-----------------------------------------\n");
		close(server_fd);
	}
	freeaddrinfo(host_addr);
	close(fd);
}

/******************************* MILESTONE ONE END *****************************************/
/*******************************************************************************************/
/*******************************************************************************************/

void cleanup(){
	printf("A thread cleans up.\n");
}

void *run(void *args){
	pthread_cleanup_push(cleanup, NULL);
	printf("\n\n################# thread begin ######################\n");
	int fd = *((int *)args);
	int milestone = *((int *)args + 1);
	if(milestone == 1)
		milestone_1(fd);
	printf("\n################# thread end ####################\n");
	pthread_cleanup_pop(1);
}

int main(int argc, char **argv){
	signal(SIGPIPE, SIG_IGN);
	if(argc != 3){
		printf("Usage: %s [port] [milestone]\n", argv[0]);
		exit(1);
	}

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
