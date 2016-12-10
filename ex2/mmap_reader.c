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

#define FILE_PATH "/tmp/mmapped.bin"


void signal_handler(int signum) {
  int fd = -1; 
  int file_size = 0; 
  struct timeval t1, t2;
  double elapsed_microsec;
  char* arr = NULL;
  int i =0;
  int success = 0; 
  int ret_val = -1; 
  
  fd = open(FILE_PATH, O_RDWR);
  if (fd < 0) {
    printf( "error opening %s: %s\n", FILE_PATH, strerror(errno));
    goto cleanup; 
  }

  file_size = lseek(fd, SEEK_SET, SEEK_END);
  if (file_size < 0) {
    printf("error using lseek to detemine the file size: %s\n", strerror(errno));
    goto cleanup; 
  }

  if (file_size == 0) {
    printf("the file %s is empty\n", FILE_PATH);
    goto cleanup; 
  }


  //start time measurements
  success = gettimeofday(&t1, NULL);
  if (success == -1) {
    printf("error starting time measurements: %s\n", strerror(errno));
    goto cleanup; 
  }
  
  //map the file
  arr = (char*) mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		     fd,0);

  if (arr == MAP_FAILED) {
    printf("error mmapping the file: %s\n", strerror(errno));
    goto cleanup; 
  }

  while (arr[i] != '\0') {
    i++; 
   }
  
  success = gettimeofday(&t2, NULL);
  if (success == -1) {
    printf("Error getting time measurements: %s\n", strerror(errno));
    goto cleanup;  
  }
  
  // Counting time elapsed
  elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  printf("%d were read in %f microseconds through MMAP\n", i,elapsed_microsec ); 

  //if we got here, no error occured
  ret_val = 0; 
 cleanup: 
  //close the file
  if (fd >= 0) {
    close(fd);
  }
  //delete it
  if (unlink(FILE_PATH) == -1 ) {
    printf("error unlink the file%s: %s\n",FILE_PATH, strerror(errno));
    exit(-1); 
  }

  if (ret_val < 0) {
    exit(ret_val); 
  }
  
}


int main() {
  struct sigaction act;

  //assign pointer to the handler function
  act.sa_handler = signal_handler;

  //remove any special flag
  act.sa_flags = 0;

  //register the handler
  if (sigaction (SIGUSR1, &act, NULL) != 0 ){
    printf("signal handle registration failed: %s\n",strerror(errno));
    return -1;
  }
  
  while (1) {
    sleep(2);
  }

  return 0; 
} 
