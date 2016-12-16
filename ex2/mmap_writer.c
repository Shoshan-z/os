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


int main(int arcg, char* argv[]) {
  int FILESIZE = 0; 
  int RPID =  0 ; 
  char* arr = NULL;
  int fd = -1; 
  int success = 0;
  int i = 0; 
  struct timeval t1, t2;
  double elapsed_microsec;
  int ret_val = -1;
  struct sigaction sigterm_action; 
  struct sigaction old_sigterm_action;


  errno = 0; 
  FILESIZE = strtol(argv[1], NULL, 10);
  if (errno != 0) {
    printf("error converting first parameter to number %s\n",strerror(errno));
    ret_val = errno; 
    goto cleanup;
  }


  errno = 0; 
  RPID = strtol(argv[2], NULL, 10);
  if (errno != 0) {
    printf("error converting first parameter to number %s\n",strerror(errno));
    ret_val = errno; 
    goto cleanup;
  }

  
  //assign pointer to our handler functions
  sigterm_action.sa_handler = SIG_IGN;

  //remove any special flag
  sigterm_action.sa_flags = 0;

  //register the signal handler
  if (sigaction (SIGTERM, &sigterm_action, &old_sigterm_action) != 0) {
    printf("signal handle registration failed %s\n",strerror(errno));
    ret_val = errno; 
    goto cleanup;
  }
  
  //open the file for writing 
  fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0 ){
      printf( "error opening %s: %s\n", FILE_PATH, strerror(errno));
      ret_val = errno; 
      goto cleanup;
   }

  //change the file permissions
   if (chmod(FILE_PATH, S_IRUSR | S_IWUSR) < 0 ) {
     printf( "error setting file permissions: %s\n", strerror(errno));
     ret_val = errno;
     goto cleanup;
   }


  // force the file to be of the same size as the (mmapped) array
  success = lseek(fd, FILESIZE-1, SEEK_SET);
  if (success == -1) {
    printf("error calling lseek() to 'stretch' the file: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup; 
  }

  
  // something has to be written at the end of the file,
  // so the file actually has the new size. 
  success = write(fd, "", 1);
  if (success != 1) {
    printf("error writing last byte of the file: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup; 
  }

  //map the file
  arr = (char*) mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		     fd,0);

  if (arr == MAP_FAILED) {
    printf("error mmapping the file: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup; 
  }

  //start time measurements
  success = gettimeofday(&t1, NULL);
  if (success == -1) {
    printf("error starting time measurements: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup; 
  }

  //write to the file
  for (i = 0; i < FILESIZE-1; i++) {
    arr[i] = 'a';
  }

  //send a signal to the reader process
  success = kill(RPID, SIGUSR1);
  if (success == -1) {
    printf("error sending signal to the reader process: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup;
  }

  success = gettimeofday(&t2, NULL);
  if (success == -1) {
    printf("error getting time measurements: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup;
  }
  
  // Counting time elapsed
  elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  //print results
  printf("%d were written in %f microseconds through MMAP\n", FILESIZE, elapsed_microsec); 
  
  //unnmap
  if (munmap(arr, FILESIZE) == -1 ) {
    printf("error un-mmapping the file: %s\n", strerror(errno));
    ret_val = errno;
    goto cleanup;
  }

  //if we got here, no error occured - return 0 
  ret_val = 0; 

 cleanup:

  //restore the signal handler
  if (sigaction (SIGTERM, &old_sigterm_action, NULL) != 0) {
    printf("failed restoring the SIGTERM signal handler: %s\n",strerror(errno));
  }

  // close the file
  if (fd >= 0) {
      close(fd);
  } 
 
  return ret_val; 
}
