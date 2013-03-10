#include"myproxy.h"


/*******************************************************************************************/
/*******************************************************************************************/
/******************************* MILESTONE ONE BEGIN ***************************************/

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



void connection_off(char *buf, int *count){
	char *position = buf - 2;
	char *pre_position = buf - 2;
	do{
		pre_position = position;
		position = strstr(position + 2, "\r\n");
		mod_field(pre_position + 2, count, *count - (int)(pre_position - buf + 2));
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
	connection_off(buf, &count);
	close(fd);
}

/*******************************************************************************************/
/*******************************************************************************************/
/******************************* MILESTONE ONE END *****************************************/

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
