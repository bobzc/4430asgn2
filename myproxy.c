#include"myproxy.h"


/*******************************************************************************************/
/*******************************************************************************************/
/******************************* MILESTONE ONE BEGIN ***************************************/

void get_domain_port(char *field, char **domain, char *port){
		char *position = strstr(field, "\r\n");
		int field_len = (int)(position - field);
		char *msg = (char *)calloc(field_len + 1, sizeof(char));
		memcpy(msg, field, field_len);
		msg[field_len] = '\0';
		char *p_domain = strtok(msg, ":");
		char *p_port = strtok(NULL, ":");
		if(p_port == NULL)
			p_port = "80";
		*domain = (char *)calloc(strlen(p_domain) + 1, sizeof(char));
		*port = (char *)calloc(strlen(p_port) + 1, sizeof(char));
		memcpy(*domain, p_domain, strlen(p_domain));
		memcpy(*port, p_port, strlen(p_port));
		*domain[strlen(p_domain)] = '\0';
		*port[strlen(p_port)] = '\0';
		printf("%s %s\n", *domain, *port);
}

void resolve_host(char *field, struct addrinfo *host_addr){
	if(memcmp(field, HOST_FIELD, strlen(HOST_FIELD)) == 0){
		char *domain, *port;
		get_domain_port(field + strlen(HOST_FIELD), &domain, &port);		
	
		struct addrinfo hints;
		memset(&hints, 0, sizeof(struct addrinfo));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_flags = 0;
		hints.ai_protocol = 0;
		printf("|%s|   |%s|\n", domain, port);
		if(getaddrinfo(domain, port, &hints, &host_addr) != 0){
			perror("Error in getaddrinfo().");
			exit(1);
		}
	}

}

void mod_field(char *field, int *count, int cur_count){
	if(memcmp(field, CONN_ALIVE_FIELD, strlen(CONN_ALIVE_FIELD)) == 0){
			char *from = field + strlen(CONN_ALIVE_FIELD);
			char *to = field + strlen(CONN_CLOSE_FIELD);
			memcpy(field, CONN_CLOSE_FIELD, strlen(CONN_CLOSE_FIELD));
			memmove(to, from, cur_count - strlen(CONN_ALIVE_FIELD));
			*count -= strlen(CONN_ALIVE_FIELD) - strlen(CONN_CLOSE_FIELD);
	}else if(memcmp(field, PROXY_CONN_ALIVE_FIELD, strlen(PROXY_CONN_ALIVE_FIELD)) == 0){
			char *from = field + strlen(PROXY_CONN_ALIVE_FIELD);
			char *to = field + strlen(PROXY_CONN_CLOSE_FIELD);
			memcpy(field, PROXY_CONN_CLOSE_FIELD, strlen(PROXY_CONN_CLOSE_FIELD));
			memmove(to, from, cur_count - strlen(PROXY_CONN_ALIVE_FIELD));
			*count -= strlen(PROXY_CONN_ALIVE_FIELD) - strlen(PROXY_CONN_CLOSE_FIELD);
	}
}

void check_header(char *buf, struct addrinfo *host_addr, int *count){
	char *position = buf - 2;
	char *pre_position = buf - 2;
	do{
		pre_position = position;
		position = strstr(position + 2, "\r\n");
		mod_field(pre_position + 2, count, *count - (int)(pre_position - buf + 2));
		resolve_host(pre_position + 2, host_addr);
	}while((int)(position - pre_position) != 2);	
	puts(buf);
}


void milestone_1(int fd){
	int count;
	char buf[HEADER_BUFFER_SIZE];
	memset(buf, 0, HEADER_BUFFER_SIZE);
	if((count = read(fd, buf, HEADER_BUFFER_SIZE)) < 0){
		perror("Read error.");
		return;
	}
//	printf("%d\n", count);
//	puts(buf);
	struct addrinfo *host_addr;
	check_header(buf, host_addr, &count);
	struct addrinfo *p_host_addr;
	int i;
//	for(i = 0; p_host_addr != NULL;p_host_addr = p_host_addr -> ai_next){
//		struct sockaddr_in *sa = (struct sockaddr_in *)p_host_addr->ai_addr;
//		printf("[%d] %s:%hd\n", i++,inet_ntoa(sa->sin_addr),ntohs(sa->sin_port));
//	}

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
	int fd = *((int *)args);
	int milestone = *((int *)args + 1);
	if(milestone == 1)
		milestone_1(fd);
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
