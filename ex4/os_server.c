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
#define BUFF_SIZE 4096

//open issues:
//1. error in accpet - the sigint interrupt it in the middle - is that ok?
//2. when the son process finishes- is it ok to exit? should I wait?
//3. check a bit deeper (consult lilach)

//global variables
int done = 0; 

//safe read
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

//safe write
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


//taken from my solution to ex1
//the function writes into target_buffer the xor values of the key and the encrypted/decrypted file
//the number fo bytes to write is determined before calling it and is passed as num_of_bytes
void xor_with_key(char* file_buffer, char* key_buffer, char* target_buffer, int num_of_bytes) {
  int i =0;
  for (i=0; i<num_of_bytes; i++) {
    target_buffer[i] = file_buffer[i]^key_buffer[i]; 
  }
}


//taken from my solution to ex1
//prepare a key of size "key_len".
//if the key is shorter than wanted, it will repeat itself 
void prepare_key(int key_fd, char* key_buffer, int key_len) {
  ssize_t read_bytes = 0;
  int curr_len = 0;
  int seek_success = 0; 
  
  while (curr_len < key_len) {
    read_bytes = read(key_fd, key_buffer, key_len-curr_len); 
    if (read_bytes < 0) {
      printf("error reading from key file: %s\n", strerror(errno));
      exit(errno);
      }

    if (read_bytes == 0 ){
      seek_success =  lseek(key_fd, SEEK_SET, 0); 
      if (seek_success <0) {
	printf("Error seeking in key file: %s\n", strerror(errno));
	exit(errno); 
      }
    
    }
    curr_len += read_bytes;
    key_buffer += read_bytes; 
  }//end while 
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
  done =1; 
}

void handle_client(int connfd, char* key_path) {
  char recv_buffer[BUFF_SIZE] = {0}; 
  char send_buffer[BUFF_SIZE] = {0};
  int read_bytes = 0; 
  char key_buffer[BUFF_SIZE] = {0};
  int key_fd = 0;
  struct stat key_stat = {0};

  while (!done) {
    //open the key file and make sure it's not empty
    key_fd = open(key_path, O_RDONLY); 
    if (key_fd < 0 ){
      printf( "error opening the key file: %s\n", strerror(errno));
      exit(errno);
    }

    if (stat(key_path, &key_stat) <0 ) {
      printf("error getting key file information: %s\n", strerror(errno));
      exit(errno); 
    }

    if (key_stat.st_size == 0) {
      printf("error - key file is empty\n");
      exit(EXIT_FAILURE); 
    }

    do {
      read_bytes = recv_with_size(connfd, recv_buffer); 

      if (read_bytes >0 ) { 
	prepare_key(key_fd, key_buffer, read_bytes);
	xor_with_key(recv_buffer, key_buffer, send_buffer, read_bytes);
	send_with_size(connfd, send_buffer, read_bytes);
      }
    } while (read_bytes != 0);
    goto cleanup; 
  }
 cleanup: //we get here if done=1 or we've finished the ecnrypting while loop
  if (key_fd >0) {
    close(key_fd);
  }
  if (connfd>0){
    close(connfd); 
  }
  exit(0);//exit to prevent from doing "parent process code"
  
}

int main(int argc, char* argv[]){
  short port = 0;
  char* key_path = NULL;
  int key_len = 0; 
  struct sockaddr_in serv_addr = {0}; 
  int listenfd = 0, connfd = 0;
  struct sigaction sigint_action;
  int f = 0; 
  
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

    //accept a new connection
    printf("listen fd %d\n", listenfd); 
    connfd = accept(listenfd, NULL, NULL);
    if(connfd<0){
      printf("error in accept: %s\n", strerror(errno));
      exit(errno); 
      }

    f = fork();
    if (f < 0) {
      printf("fork failed: %s\n", strerror(errno));
      exit(errno);
      }
	
    if (f == 0) { //inside son process
     close(listenfd);
     handle_client(connfd, key_path); 			
     }
    else { //inside parent process 
      close(connfd); 
    }	  
  }

  close(listenfd); 
  
  return 0; 
}


