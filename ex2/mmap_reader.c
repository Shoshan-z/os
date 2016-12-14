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


/*gloabl variables: the sigterm sigaction structs are global so I'll be able
to register the in the main but restore them in the SIGUSR1 signal handler
 */
struct sigaction sigterm_action;
struct sigaction old_sigterm_action;

//signal handler for SIGUSR1
void signal_handler(int signum) {
  int fd = -1; 
  int file_size = 0; 
  struct timeval t1, t2;
  double elapsed_microsec;
  char* arr = NULL;
  int i =0;
  int success = 0; 
  int ret_val = EXIT_FAILURE; 
  
  fd = open(FILE_PATH, O_RDWR);
  if (fd < 0) {
    printf( "error opening %s: %s\n", FILE_PATH, strerror(errno));
    ret_val = errno; 
    success = -1; 
    goto cleanup; 
  }

  file_size = lseek(fd, SEEK_SET, SEEK_END);
  if (file_size < 0) {
    printf("error using lseek to detemine the file size: %s\n", strerror(errno));
    ret_val = errno;
    success = -1; 
    goto cleanup; 
  }

  if (file_size == 0) {
    printf("the file %s is empty\n", FILE_PATH);
    success = -1; 
    goto cleanup; 
  }


  //start time measurements
  success = gettimeofday(&t1, NULL);
  if (success == -1) {
    printf("error starting time measurements: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup; 
  }
  
  //map the file
  arr = (char*) mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		     fd,0);

  if (arr == MAP_FAILED) {
    printf("error mmapping the file: %s\n", strerror(errno));
    ret_val = errno;
    success = -1; 
    goto cleanup; 
  }

  while (arr[i] != '\0') {
    if (i == file_size-1) {
      printf("error in file %s - no null terminator\n", FILE_PATH); 
      break; 
    }

    if (arr[i] == 'a') {
      i++;
    }
   }

  //counte the null terminator
  if (arr[i] == '\0') {
    i++; 
  }
  
  success = gettimeofday(&t2, NULL);
  if (success == -1) {
    printf("error getting time measurements: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup;  
  }
  
  // counting time elapsed
  elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  printf("%d were read in %f microseconds through MMAP\n", i,elapsed_microsec ); 

  //if we got here, no error occured
  ret_val = 0;
  success = 0; 
 cleanup: 

  //restore the SIGTERM signal handler
  if (sigaction (SIGTERM, &old_sigterm_action, NULL) != 0 ){
    printf("signal handle registration failed: %s\n",strerror(errno));
    success = -1; 
    ret_val = errno;
  }
 
  //close the file
  if (fd >= 0) {
    close(fd);
  }
  //delete it
   if (unlink(FILE_PATH) == -1 ) {
    printf("error unlink the file%s: %s\n",FILE_PATH, strerror(errno));
    ret_val =0 ;
    success = -1;
  }

  if (success < 0) {
    exit(ret_val); 
  }
  
}


int main() {
  struct sigaction sigusr_action;

  //assign pointer to the handler function
  sigusr_action.sa_handler = signal_handler;
  sigterm_action.sa_handler = SIG_IGN;
  
  //remove any special flag
  sigusr_action.sa_flags = 0;
  sigterm_action.sa_flags = 0;
  
  //register the handlers
  if (sigaction (SIGUSR1, &sigusr_action, NULL) != 0 ){
    printf("signal handle registration failed: %s\n",strerror(errno));
    return errno;
  }

  if (sigaction (SIGTERM, &sigterm_action, &old_sigterm_action) != 0 ){
    printf("signal handle registration failed: %s\n",strerror(errno));
    return errno;
  }
  
  while (1) {
    sleep(2);
  }

  return 0; 
} 
