#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#define BUFF_SIZE 4096


void parse_args(char* argv[], char** ip, short* port, char** input_path, char** output_path) {
 
	*ip = argv[1]; 
	
	errno = 0; 
  *port = strtol(argv[2], NULL, 10);
  if (errno != 0) {
    printf("error converting PORT to short: %s\n",strerror(errno));
    exit(errno);  
  }

	*input_path = argv[3]; 
	*output_path = argv[4]; 
  
}

int main(int argc, char* argv[]){
	int sock_fd =0; 
	char send_buffer[BUFF_SIZE] = {0}; 
	char recv_buffer[BUFF_SIZE] = {0}; 
	struct sockaddr_in serv_addr = {0}; 
  struct sockaddr_in my_addr = {0}; //check if needed  
  socklen_t addrsize = sizeof(struct sockaddr_in );
	char* ip = NULL; 
	short port =0; 
	char* input_path = NULL; 
	char* output_path = NULL; 
	chat* test = "data sent to server"; 
	
	if (argc != 5) {
		printf("usage: %s <IP> <PORT> <IN> <OUT>\n", argv[0]); 
    return 0; 
  }
	
	parse_args(argv, &ip, &port, &input_path, &output_path); 
	
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd < 0) {
		printf("error in socket(): %s\n", strerror(errno)); 
		exit(errno);
	}
	
	serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port); 
  serv_addr.sin_addr.s_addr = inet_addr(ip); 
	
	if( connect(sock_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		printf("error in connect(): %s\n", strerror(errno));
    exit(errno);
   }

	
	 

	

	


	return 0; 
	
}
