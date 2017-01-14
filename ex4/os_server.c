#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define URANDOM_PATH "/dev/urandom"
#define BUFF_SIZE 4096


/*
 didn't check the conditions for the keyfile
 */


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


int main(int argc, char* argv[]){
  short port = 0;
  char* key_path = NULL;
  int key_len = 0; 
  int success = 0; 
  
  if (argc < 3 || argc > 4) {
    printf("Usage: %s <PORT> <KEY> [KEYLEN]\n", argv[0]); 
    return 0; 
  }
  
  parse_args(argc, argv,  &port, &key_path, &key_len);

  if (key_len !=0) {
    init_key_file(key_path, key_len); 
  }
  
 cleanup:
  
  return 0; 
}


