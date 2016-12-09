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
  int FILESIZE = atoi(argv[1]);
  int RPID = atoi(argv[2]);
  char* arr = NULL;
  int fd = 0;
  int success = 0;
  int i = 0; 
  struct timeval t1, t2;
  double elapsed_microsec;

  
  //open the file for writing //
  fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
  if (fd < 0 ){
      printf( "Error opening %s: %s\n", FILE_PATH, strerror(errno));
      return -1;
   }

  //change the file permissions
   if (chmod(FILE_PATH, S_IRUSR | S_IWUSR) < 0 ) {
     printf( "Error setting file permissions: %s\n", strerror(errno));
     return -1;
   }


  // Force the file to be of the same size as the (mmapped) array
  success = lseek(fd, FILESIZE-1, SEEK_SET);
  if (success == -1) {
    printf("Error calling lseek() to 'stretch' the file: %s\n", strerror(errno));
    return -1;
  }

  
  // Something has to be written at the end of the file,
  // so the file actually has the new size. 
  success = write(fd, "", 1);
  if (success != 1) {
    printf("Error writing last byte of the file: %s\n", strerror(errno));
    return -1;
  }

  //map the file
  arr = (char*) mmap(NULL, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
		     fd,0);

  if (arr == MAP_FAILED) {
    printf("Error mmapping the file: %s\n", strerror(errno));
    return -1;
  }

  //start time measurements
  success = gettimeofday(&t1, NULL);
  if (success == -1) {
    printf("Error starting time measurements: %s\n", strerror(errno));
    return -1;
  }

  //write to the file
  for (i = 0; i < FILESIZE; i++) {
    arr[i] = 'a';
  }

  //send a signal to the reader process
  success = kill(RPID, SIGUSR1);
  if (success == -1) {
    printf("Error sending signal to the reader process: %s\n", strerror(errno));
    return -1;
  }

  success = gettimeofday(&t2, NULL);
  if (success == -1) {
    printf("Error getting time measurements: %s\n", strerror(errno));
    return -1;
  }
  
  // Counting time elapsed
  elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  //print results
  printf("%d were written in %f microseconds through MMAP\n", FILESIZE, elapsed_microsec); 
  
  //unnmap
  if (munmap(arr, FILESIZE) == -1 ) {
    printf("Error un-mmapping the file: %s\n", strerror(errno));
    return -1;
  }

  // close the file
  close(fd);

  return 0; 
}
