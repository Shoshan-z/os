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

int read_all_from_file(int fd, char* buffer, int size){
  int total = 0; 
  int read_bytes =0; 
  
   do {
    read_bytes = read(fd, buffer+total, size-total);
    if (read_bytes < 0) {
      printf("error reading from file: %s\n", strerror(errno));
      exit(errno); 
    }
    total += read_bytes;
  }
   while (total < size && read_bytes > 0);

    return total; 
}


int write_all_to_file(int fd, char* buffer, int size){
  int total = 0; 
  int written =0; 
  
  while (total < size) {
    written = write(fd, buffer+total, size-total);
    if (written < 0) {
      printf("error writing to file: %s\n", strerror(errno));
      exit(errno); 
    }
    total += written;
  }

  return total; 
}

void send_with_size(int connfd, char* buffer, int size){
	
	write_all_to_file(connfd, (char*) &size, sizeof(size)); 
	
	if (size != 0) {
		write_all_to_file(connfd, buffer, size); 
	}
}

//recive the message size, recive the message and return the size
int recv_with_size(int connfd, char* buffer){
	int size = 0; 
	
	read_all_from_file(connfd, (char*)&size, sizeof(size)); 
	
	if (size != 0) {
		read_all_from_file(connfd, buffer, size);
	}

	return size; 
}

int main(int argc, char* argv[]){
  int sock_fd =0; 
        char send_buffer[BUFF_SIZE] = {0};
	char recv_buffer[BUFF_SIZE] = {0}; 
	struct sockaddr_in serv_addr = {0}; 
  //struct sockaddr_in my_addr = {0}; //check if needed  
  //socklen_t addrsize = sizeof(struct sockaddr_in );
	char* ip = NULL; 
	short port =0; 
	char* input_path = NULL; 
	char* output_path = NULL; 
	//char* test = "data sent to server"; 
	int success = 0; 
	struct stat input_file_info; 
	int input_fd = 0;
	int output_fd = 0;
	int bytes_read = 0; 
	
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
	
	success = stat(input_path, &input_file_info);
  if (success < 0 ) {
    if (errno != ENOENT) { //the error doesn't say the file doesn't exist
      printf( "error checking the file status %s: %s\n", input_path, strerror(errno));
      exit(errno); 
    }
    else { //the file doesn't exist
      printf("IN file doesn't exist: %s\n", strerror(errno)); 
      exit(errno); 
    }
 }
	
  //open the input file, read only
  input_fd = open(input_path, O_RDONLY); 
  if (input_fd < 0 ){
    printf( "error opening %s: %s\n", input_path, strerror(errno));
    exit(errno);
  }

  output_fd = open(output_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU |S_IRWXG | S_IROTH); //open file for writing, create it if not exist
    if (output_fd <0 ){
      printf( "error opening %s: %s\n", output_path, strerror(errno));
      exit(errno); 
    }
  
	
  do {
    bytes_read = read_all_from_file(input_fd, send_buffer, sizeof(send_buffer)); 
    send_with_size(sock_fd, send_buffer, bytes_read); 

    if (bytes_read > 0) {
      recv_with_size(sock_fd, recv_buffer); 
      write_all_to_file(output_fd, recv_buffer, bytes_read); 
      }
    } while (bytes_read > 0);  


  close(input_fd);
  close(output_fd);
  close(sock_fd); 

  return 0; 
	
}
