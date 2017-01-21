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


#define URANDOM_PATH "/dev/urandom"
#define BUFF_SIZE 5


/*
 didn't check the conditions for the keyfile
 */

 

//global variables
int done = 0; 
int clients_cnt = 0; //delete afterwards


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
	
	read_all_from_file(connfd, (char*) &size, sizeof(size)); 
	
	if (size != 0) {
		read_all_from_file(connfd, buffer, size);
	}

	return size; 
}


void init_key_file(char* key_path, int key_len){
  int key_fd = 0; 
  int random_fd = 0; 
  char buffer[BUFF_SIZE] = {0}; 
  int bytes_read = 0;
  int bytes_written = 0; 
  int bytes_left =0; 
  
  key_fd = open(key_path, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU |S_IRWXG | S_IROTH); 
  if (key_fd < 0 ){
		printf( "error opening the key file: %s\n", strerror(errno));
    exit(errno);
  }

  random_fd = open(URANDOM_PATH, O_RDONLY); 
  if (random_fd < 0 ){
		printf( "error opening %s: %s\n", URANDOM_PATH, strerror(errno));
    exit(errno);
  }

  while (bytes_read < key_len) {

    if (bytes_read + sizeof(buffer) > key_len) {
     
      bytes_left = key_len - bytes_read; //number of bytes left to read/write
      bytes_read += read_all_from_file(random_fd, buffer, bytes_left);
      bytes_written += write_all_to_file(key_fd, buffer, bytes_left); 
    }

    else {
      bytes_read += read_all_from_file(random_fd, buffer, sizeof(buffer)); 
      bytes_written += write_all_to_file(key_fd, buffer, sizeof(buffer)); 
    }
  }

  close(random_fd);
  close(key_fd); 
}


void parse_args(int argc, char* argv[], short* port, char** key_path, int* key_len) {
 
  errno = 0; 
  *port = strtol(argv[1], NULL, 10);
  if (errno != 0) {
    printf("error converting PORT to short: %s\n",strerror(errno));
    exit(errno);  
  }

  *key_path = argv[2];

  if (argc > 3) {
    errno = 0; 
    *key_len = strtol(argv[3], NULL, 10);
    if (errno != 0) {
      printf("error converting KEYLEN to int: %s\n",strerror(errno));
      exit(errno);  
    }
  }

}


void sigint_handler (int signum)
{
  printf("sigint recieved\n"); 
  done =1; 
}

void handle_client(int connfd) {
	char buffer[BUFF_SIZE] = {0}; 
	int read_bytes = 0; 
	//int sent_bytes = 0; 
	
	clients_cnt++; 
  printf("handling client %d\n", clients_cnt);   
	
	do {
		read_bytes = recv_with_size(connfd, buffer); 
		send_with_size(connfd, buffer, read_bytes); 
		
	}while (read_bytes != 0); 
	
}

int main(int argc, char* argv[]){
  short port = 0;
  char* key_path = NULL;
  int key_len = 0; 
  //int success = 0; 
  struct sockaddr_in serv_addr = {0};
  //struct sockaddr_in my_addr = {0};  //check if i need this 
  //struct sockaddr_in peer_addr = {0}; //check if i need this
  int listenfd = 0, connfd = 0;
  struct sigaction sigint_action;
  //int f = 0; 
  
  if (argc < 3 || argc > 4) {
    printf("usage: %s <PORT> <KEY> [KEYLEN]\n", argv[0]); 
    return 0; 
  }
  
  parse_args(argc, argv,  &port, &key_path, &key_len);

  if (key_len !=0) {
    init_key_file(key_path, key_len); 
  }
  
  listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if (listenfd < 0) {
	printf("error in socket(): %s\n", strerror(errno)); 
	exit(errno); 
  }
   
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // INADDR_ANY = any local machine address
  serv_addr.sin_port = htons(port);
  
  
  if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))){
    printf("error in bind(): %s \n", strerror(errno));
    exit(errno); 
  }

  if(listen(listenfd, 10)){
    printf("error in listen: %s \n", strerror(errno));
    exit(errno); 
  }
  
  //assign pointer to our handler functions
  sigint_action.sa_handler = sigint_handler; 
  
  //remove any special flags
  sigint_action.sa_flags = 0;

  if (sigaction (SIGINT, &sigint_action, NULL) != 0) {
    printf("signal handle registration failed %s\n",strerror(errno));
    exit(errno); 
  }
  
  
  while (!done) {
	    
		//socklen_t addrsize = sizeof(struct sockaddr_in );

     /* accpeting connection. can use NULL in 2nd and 3rd arguments
    but we want to print the client socket details*/
    connfd = accept(listenfd, NULL, NULL);
    if(connfd<0){
	  printf("error in accept: %s\n", strerror(errno));
      exit(errno); 
		}
		handle_client(connfd);
	
	/*
		f = fork();
		if (f < 0) {
			printf("fork failed: %s\n", strerror(errno));
			exit(errno);
		}
	
		if (f == 0) { //inside son process
			//close(listenfd);
			//handle_client(connfd); 
			
		}
		else { //inside parent process 
			//close(connfd); 
		}
	  */
  }
  
 //cleanup:
  
  
  
  return 0; 
}


