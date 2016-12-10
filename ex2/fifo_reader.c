#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <signal.h>

#define BUFF_SIZE 4096
#define FILE_PATH "/tmp/osfifo"


int main(){
  int fd = -1; 
  struct timeval t1, t2;
  double elapsed_microsec;
  int success = 0; 
  char buffer[BUFF_SIZE]; 
  int bytes_read =0; 
  int total =0 ;
  char* path = FILE_PATH;
  int i =0; 
  int ret_val = 0; 
  
  sleep(2);
  
  fd = open(path, O_RDONLY);
  if (fd < 0 ) {
    printf( "error opening %s: %s\n", path, strerror(errno));
    goto cleanup;
  }
  
  
  //start time measurements
  success = gettimeofday(&t1, NULL);
  if (success == -1) {
    printf("error starting time measurements: %s\n", strerror(errno));
    goto cleanup;
  }

  do {
    bytes_read = read(fd, buffer, sizeof(buffer));
    if (bytes_read < 0) {
      printf("error reading from file: %s\n", strerror(errno));
      goto cleanup;
    }

    //count the 'a' chars
    for (i=0; i<bytes_read; i++){
      if (buffer[i] == 'a') {
	total++; 
      }
    }
    
  } while (bytes_read !=0); 
  
  //end time measurements
  success = gettimeofday(&t2, NULL);
  if (success == -1) {
    printf("Error getting time measurements: %s\n", strerror(errno));
    goto cleanup;
  }

  
  // Counting time elapsed
  elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;


  printf("%d were read in %f microseconds through FIFO\n", total, elapsed_microsec);

  ret_val = 0; 
  
 cleanup:
  if (fd >= 0) {
    close(fd);
  }
  return ret_val;  
}