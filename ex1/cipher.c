#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define BYTES_TO_READ 131072
#define BYTES_TO_WRITE 131072
//stackoverflow claims that 131072 = 2**17 is optimal for read

/*the function writes into target_buffer the xor values of the key and the encrypted/decrypted file
the number fo bytes to write is determined before calling it and is passed as num_of_bytes
 */
void xor_with_key(char* file_buffer, char* key_buffer, char* target_buffer, int num_of_bytes) {
  int i =0;
  for (i=0; i<num_of_bytes; i++) {
    target_buffer[i] = file_buffer[i]^key_buffer[i]; 
  }
}


//prepare a key of size "key_len".
//if the key is shorter than wanted, it will repeat itself 
int prepare_key(int key_fd, char* key_buffer, int key_len) {
  ssize_t read_bytes = 0;
  int curr_len = 0;
  int seek_success = 0; 
  
  while (curr_len < key_len) {
    read_bytes = read(key_fd, key_buffer, key_len-curr_len); 
    if (read_bytes < 0) {
      printf("Error reading from key file: %s\n", strerror(errno));
      return -1;
      }

    if (read_bytes == 0 ){
      seek_success =  lseek(key_fd, SEEK_SET, 0); 
      if (seek_success <0) {
	printf("Error seeking in key file: %s\n", strerror(errno));
	return -1; 
      }
    
    }

    curr_len += read_bytes;
    key_buffer += read_bytes; 
  }//end while

  return 0; 
}

//make sure all the bytes I wish to write are written to the file
//returns 0 in case of success, -1 in case of error
int write_all_to_file(int write_file_fd, char* target_buffer,int bytes_to_write) {
  ssize_t bytes_written = 0; 
  int total = 0; 

  while (total<bytes_to_write) {
    bytes_written = write(write_file_fd, target_buffer+total, bytes_to_write-total); 
    if (bytes_written < 0) {
      printf("Error writing to file: %s\n", strerror(errno));
      return -1;
      }
    total += bytes_written; 
  }

  return 0; 
}


//this function encrypts/decrypts one file
//return 0 on success, -1 in case of error
int decrypt_file(int read_file_fd, int key_fd, int write_file_fd) {
  char file_buffer[BYTES_TO_READ] = {0};
  char key_buffer[BYTES_TO_READ] = {0};
  char target_buffer[BYTES_TO_WRITE] = {0};
  ssize_t bytes_read;
  int write_success = 0; 
  int read_key = 0;
  int seek_success = 0; 

  //rewind the key file (for the next input file)
  seek_success =  lseek(key_fd, SEEK_SET,0); 
  if (seek_success <0) {
    printf("Error seeking in key file: %s\n", strerror(errno));
    return -1; 
  }
  
  bytes_read = read(read_file_fd, file_buffer, BYTES_TO_READ); 
  if (bytes_read < 0) {
    printf("Error reading from file: %s\n", strerror(errno));
    return -1;
   }

  while (bytes_read > 0) {
    //prepare a key of the proper len
    read_key = prepare_key(key_fd, key_buffer, bytes_read); 
    if (read_key == -1) return -1;

    xor_with_key(file_buffer,key_buffer, target_buffer, bytes_read); 
    write_success = write_all_to_file(write_file_fd, target_buffer, bytes_read);
    if (write_success == -1) return -1;
    
    bytes_read = read(read_file_fd, file_buffer, BYTES_TO_READ);
    if (bytes_read < 0) {
      printf("Error reading from file: %s\n", strerror(errno));
      return -1;
   }

  }
  return 0; 
 }


int main(int argc, char* argv[]){
  char* input_dir_path = NULL;
  char* output_dir_path = NULL;
  DIR* input_files_dir = NULL;
  DIR* output_files_dir = NULL; 
  struct dirent* read_dir = NULL; 
  char curr_input_file_path[1024] = {0};
  char curr_output_file_path[1024] = {0};
  int curr_input_fd = 0;
  int curr_output_fd = 0;
  int output_dir_fd = 0; 
  int key_fd =0;
  int decrpyt_success = 0; 
  struct stat key_stat;

  //open the input files fir
  input_dir_path = argv[1];
  input_files_dir = opendir(input_dir_path);
  if (input_files_dir == NULL){
    printf("Error openning the input directory %s\n", strerror(errno));
    return -1; 
  }

  //open the output files fir
  output_dir_path = argv[3];  
  output_files_dir = opendir(output_dir_path);
  if (errno == ENOENT) { //the directory doesn't exist
    output_dir_fd = mkdir(output_dir_path, S_IFDIR | S_IRWXU | S_IRWXG | S_IROTH | S_IWOTH); //standard permissions that I took from looking at ls output 
     if (output_dir_fd <0) {
       printf("Error creating the output directory %s\n", strerror(errno));
       return -1; 
     }
     output_files_dir = opendir(output_dir_path); //try to open the directory again
  }

  //if there still an error nd the directory wasn't created, error and exit
  if (output_files_dir == NULL) {
    printf("Error openning/creating the output directory %s\n", strerror(errno));
      return -1; 
  }

  //get the key file descriptor
  key_fd = open(argv[2], O_RDONLY);
  if (key_fd < 0 ){
    printf( "Error opening the key file: %s\n", strerror(errno));
    return -1;
   }

  if (stat(argv[2], &key_stat) <0 ) {
    printf("Error getting key file information: %s\n", strerror(errno));
    return -1; 
  }

  if (key_stat.st_size == 0) {
    printf("Error - key file is empty\n");
    return -1; 
  }
  
  //iterate over the files
  read_dir = readdir(input_files_dir);
  if (read_dir == NULL) {
    printf("Error openning the input directory %s\n", strerror(errno));
    return -1; 
  }
  
  while (read_dir != NULL) {
    //skip "." and ".." which are in every folder
    if (strcmp(read_dir->d_name, ".") == 0 || strcmp(read_dir->d_name, "..") == 0 ) {
      read_dir = readdir(input_files_dir);
      continue; 
      }
    //create the current input file path
    sprintf(curr_input_file_path, "%s/%s", input_dir_path, read_dir->d_name);
    curr_input_fd = open(curr_input_file_path, O_RDONLY); //open, read only
    if (curr_input_fd < 0 ){
      printf( "Error opening %s: %s\n", curr_input_file_path, strerror( errno ) );
        return -1;
    }

    sprintf(curr_output_file_path, "%s/%s", output_dir_path, read_dir->d_name);
    curr_output_fd = open(curr_output_file_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU |S_IRWXG | S_IROTH); //open file for writing, create it if not exist
    if (curr_output_fd <0 ){
      printf( "Error opening %s: %s\n", curr_output_file_path, strerror( errno ) );
      return -1; 
    }
   
    decrpyt_success = decrypt_file(curr_input_fd, key_fd, curr_output_fd);

    if (decrpyt_success < 0) {
      return -1; 
    }
 
    //close the files and continue
    close(curr_input_fd);  
    close(curr_output_fd); 
    read_dir = readdir(input_files_dir);
  }

  close(key_fd);
  closedir(input_files_dir); 
  closedir(output_files_dir);  
    
  return 0; 

}
