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


//global variables
int g_total_written = 0; 
struct timeval t1;

void compute_time_and_print(struct timeval t1) {
  double elapsed_microsec;
  struct timeval t2;
  int success =0; 

  //end time measurements
  success = gettimeofday(&t2, NULL);
  if (success == -1) {
    printf("error getting time measurements: %s\n", strerror(errno)); 
    return; 
  }
  
  // counting time elapsed
  elapsed_microsec = (t2.tv_sec - t1.tv_sec) * 1000.0;
  elapsed_microsec += (t2.tv_usec - t1.tv_usec) / 1000.0;

  printf("%d were written in %f microseconds through FIFO\n", g_total_written, elapsed_microsec); 

}

/*
the signal handler computes the time and prints the bytes written so far. 
then it returns to the main flow - since write fails, cleanup and exit
will be done anyway
 */
void handle_sigpipe(int signum) {

  compute_time_and_print(t1); 

}

int write_all_to_file(int fd, char* buffer, int size){
  int total = 0; 
  int written =0; 
  
  while (total < size) {
    written = write(fd, buffer+total, size-total);
    if (written < 0) {
      printf("error writing to file: %s\n", strerror(errno));
      return -1; 
    }
    total += written;
    g_total_written += written; //the global counter
  }

  return total; 
}


int write_num_chars_to_file(int fd, int num) {
  int total = 0;
  int written = 0; 
  char buffer[BUFF_SIZE] = {0};

  memset(buffer, 'a', sizeof(buffer)); 

  while (total < num) {
    if (total + BUFF_SIZE > num) { //less then buffsize bytes remained to write
      written= write_all_to_file(fd,buffer, num-total);
      if (written < 0) {
	  return -1; 
	} 
    }
    else
    {
    written= write_all_to_file(fd,buffer, sizeof(buffer));
    if (written < 0) {
	return -1; 
       }
    }	
    total += written;
  } //end while

  return 0; 
  }


int main(int argc, char* argv[]){
  int num = atoi(argv[1]);
  char* path = FILE_PATH;
  int fd = 0; 
  int success = 0;
  struct stat file_info; 
  int file_exists = 0; 
  int ret_val = -1; 
  struct sigaction sigpipe_action;
  struct sigaction sigint_action; 
  struct sigaction old_sigint_action;
  
  //assign pointer to our handler functions
  sigpipe_action.sa_handler = handle_sigpipe;
  sigint_action.sa_handler = SIG_IGN; 

  //remove any special flag
  sigpipe_action.sa_flags = 0;
  sigint_action.sa_flags = 0;
  
  if (sigaction (SIGPIPE, &sigpipe_action, NULL) != 0) {
    printf("signal handle registration failed %s\n",strerror(errno));
    goto cleanup;
  }

  if (sigaction (SIGINT, &sigint_action, &old_sigint_action) != 0) {
    printf("signal handle registration failed %s\n",strerror(errno));
    goto cleanup;
  }
  
  //check if the fifo file exist
  success = stat(path, &file_info);
  if (success < 0 ) {
    if (errno != ENOENT) { //the error doesn't say the file doesn't exist
      printf( "error checking the file status %s: %s\n", path, strerror(errno));
      goto cleanup; 
    }
    else { //the file doesn't exist
      //create the fifo file
      success = mkfifo(path, 0600);
      if (success == -1) {
	printf( "error creating fifo %s: %s\n", path, strerror(errno));
	goto cleanup;
      } 
    }
  }
  else { //the file exists

    file_exists = 1; 
    
    //change the file permissions
    if (chmod(path, S_IRUSR | S_IWUSR) < 0 ) {
      printf( "error setting file permissions: %s\n", strerror(errno));
      goto cleanup; 
   }

  }
  
  
  //open, write only
  fd = open(path, O_WRONLY);
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

  success = write_num_chars_to_file(fd,num);
  if (success == -1) {
    printf("error writing %d bytes to file\n", num);
    goto cleanup; 
  } 

  compute_time_and_print(t1);
  ret_val = 0; 

 cleanup:
  if (sigaction (SIGINT, &old_sigint_action, NULL) != 0) {
    printf("failed restoring the SIGINT signal handler: %s\n",strerror(errno));
  }

  if (fd >=0) {
    close(fd);
  }

  if (file_exists) {
    if (unlink(path) == -1 ) {
      printf("error unlink the file%s: %s\n", path, strerror(errno));  
    }
  }

  return(ret_val); 
}
