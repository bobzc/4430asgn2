#include"myproxy.h"


/*******************************************************************************************/
/*******************************************************************************************/
/******************************* MILESTONE ONE BEGIN ***************************************/
/*
char *get_domain_port(char *field){
		char *position = strstr(field, "\r\n");
		int field_len = (int)(position - field);
		char *msg = (char *)calloc(field_len + 1, sizeof(char));
		memcpy(msg, field, field_len);
		msg[field_len] = '\0';
		return msg;
}

void resolve_host(char *field, struct addrinfo **host_addr){
	if(memcmp(field, HOST_FIELD, strlen(HOST_FIELD)) == 0){
		char *addr = get_domain_port(field + strlen(HOST_FIELD));		
		char *domain = strtok(addr, ":");
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
		free(addr);
	}
}

void mod_field(char *begin, char *end, int length, char *content){
	char *from = end;
	char *to = begin + strlen(content);
	strcpy(begin, content);
	memmove(to, from, length);
}

int proc_req(char *buf_begin, char *buf_end, struct addrinfo **host_addr){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		if(strncmp(field_begin, CONN_ALIVE_FIELD, field_len) == 0){
			int gap = strlen(CONN_ALIVE_FIELD) - strlen(CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
		}else if(strncmp(field_begin, PROXY_CONN_ALIVE_FIELD, field_len) == 0){
			int gap = strlen(PROXY_CONN_ALIVE_FIELD) - strlen(PROXY_CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
		}
	}while(field_len != 0);	
	return (int)(buf_end - buf_begin);
}

void insert_proxy_conn(char *buf, int *count){
	char *position = strstr(buf, "\r\n");
	char *from = position + 2;
	char *to = from + strlen(PROXY_CONN_CLOSE_FIELD) + 2;
	memmove(to, from, *count - (int)(position - buf + 2) );
	*count = *count + strlen(PROXY_CONN_CLOSE_FIELD) + 2;
	memcpy(from, PROXY_CONN_CLOSE_FIELD, strlen(PROXY_CONN_CLOSE_FIELD));
	from[strlen(PROXY_CONN_CLOSE_FIELD)] = '\r';
	from[strlen(PROXY_CONN_CLOSE_FIELD) + 1] = '\n';
}

int get_content_len(char *begin, char *end){
	if(memcmp(begin, CONTENT_LEN_FIELD, strlen(CONTENT_LEN_FIELD)) == 0){
		char num[10];
		int num_len = (int)(end - begin) - strlen(CONTENT_LEN_FIELD);
		strncpy(num, begin + strlen(CONTENT_LEN_FIELD), num_len);
		num[num_len] = '\0';
		return atoi(num);
	}
	return 0;
}

int check_response_header(char *buf, int *count, int *header_len){
	char *position = buf - 2;
	char *pre_position = buf - 2;
	char has_proxy_conn = 0;
	int content_len = 0;
	do{
		pre_position = position;
		position = strstr(position + 2, "\r\n");
		content_len += get_content_len(pre_position + 2, position);
		has_proxy_conn += mod_field(pre_position + 2, count, *count - (int)(pre_position - buf + 2));
	}while((int)(position - pre_position) != 2);	
	*header_len = (int) (position - buf);
	if(has_proxy_conn){
		insert_proxy_conn(buf, count);
		*header_len += strlen(PROXY_CONN_CLOSE_FIELD) + 2;
	}
	return content_len;
}


void forward_response(int client_fd, int server_fd){
	int count;
	int header_len = 0;
	char buf[HEADER_BUFFER_SIZE];
	memset(buf, 0, HEADER_BUFFER_SIZE);
	if((count = read(server_fd, buf, HEADER_BUFFER_SIZE - 256)) < 0){
		perror("Read error.");
		pthread_exit(NULL);
	}
	int content_len = check_response_header(buf, &count, &header_len);
	write(client_fd, buf, count);
	printf("Modified Response header:\n");
//	write(1, buf, header_len);
	printf("Response content length: %d\n", content_len);
	while(count < content_len + header_len){
		printf("1\n");
		int count_cur = read(server_fd, buf, sizeof(buf));
		write(client_fd, buf, count_cur);
		count += count_cur;
	}
}

int forward_request(char *buf, int count, struct addrinfo *host_addr){
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

}*/


void resolve_host(char *field, struct addrinfo **host_addr){
	if(memcmp(field, HOST_FIELD, strlen(HOST_FIELD)) == 0){
		char *addr = get_domain_port(field + strlen(HOST_FIELD));		
		char *domain = strtok(addr, ":");
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
		free(addr);
	}
}



/* modify field to value 'content' */

void mod_field(char *begin, char *end, int length, char *content){
	char *from = end;
	char *to = begin + strlen(content);
	strcpy(begin, content);
	memmove(to, from, length);
}

char *get_field_value(char *begin, char *end){
	char *ret = (char *)malloc((int)(end - begin) + 1, sizeof(char));
	strncpy(ret, begin, (int)(end - begin));
	ret[(int)(end - begin)] = '\0';
	return ret;
}

/* process request, and get host address */

int proc_req(char *buf_begin, char *buf_end, struct addrinfo **host_addr){
	char *field_begin = buf_begin - 2;
	char *field_end = buf_begin - 2;
	int field_len;
	do{
		field_begin = field_end + 2;
		field_end = strstr(field_begin, "\r\n");
		field_len = (int)(field_end - field_begin);
		if(strncmp(field_begin, CONN_ALIVE_FIELD, strlen(CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(CONN_ALIVE_FIELD) - strlen(CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
		}else if(strncmp(field_begin, PROXY_CONN_ALIVE_FIELD, strlen(PROXY_CONN_ALIVE_FIELD)) == 0){
			int gap = strlen(PROXY_CONN_ALIVE_FIELD) - strlen(PROXY_CONN_CLOSE_FIELD);
			mod_field(field_begin, field_end, (int)(buf_end - field_end), PROXY_CONN_CLOSE_FIELD);
			field_end -= gap;
			buf_end -= gap;
		}else if(strncmp(field_begin, HOST_FIELD, strlen(HOST_FIELD)) == 0){
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

	total_count = read(fd, buf, HEADER_BUFFER_SIZE);

	printf("Get request header(%d):\n", total_count);
	write(1, buf, total_count);
	
	/* process request header, and get host address */
	struct addrinfo *host_addr;
	total_count = proc_req(buf, buf + total_count, &host_addr);
	
	printf("Processed request header(%d):\n", total_count);
	write(1, buf, total_count);
	
/*	
	printf("HTTP header changed: |%s|\n", buf);
	struct addrinfo *p_host_addr = host_addr;
	int i;
	for(i = 0; p_host_addr != NULL;p_host_addr = p_host_addr -> ai_next){
		struct sockaddr_in *sa = (struct sockaddr_in *)p_host_addr->ai_addr;
		printf("[%d] %s:%hd\n", i++,inet_ntoa(sa->sin_addr),ntohs(sa->sin_port));
	}
	int server_fd;
	if((server_fd = forward_request(buf, count, host_addr)) != -1)
	forward_response(fd, server_fd);
	freeaddrinfo(host_addr);
	close(fd);
*/
}

/******************************* MILESTONE ONE END *****************************************/
/*******************************************************************************************/
/*******************************************************************************************/

void cleanup(){
	printf("A thread cleans up.\n");
}

void *run(void *args){
	pthread_cleanup_push(cleanup, NULL);
	printf("\n\n------------------ thread begin ---------------------\n");
	int fd = *((int *)args);
	int milestone = *((int *)args + 1);
	if(milestone == 1)
		milestone_1(fd);
	printf("\n------------------- thread end ------------------------\n");
	pthread_cleanup_pop(1);
}

int main(int argc, char **argv){
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
