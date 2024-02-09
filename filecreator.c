#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

#define handle_error(msg) \
   do { perror(msg); exit(EXIT_FAILURE); } while (0)
   
int f, r, t;
char *d;  

void *thread_function(void *arg) {
  int thread_id = *(int*)arg;
  int start = thread_id * (f / t);
  int remainder = f % t;
  if (thread_id < remainder)  
    start += thread_id;
  else
    start += remainder;
    
  int end = start + (f / t); 
  if (thread_id < remainder) 
    end++;
  if(end > f) 
    end = f;

  for(int i = start; i < end; i++) {

    
    char filename[100];
    sprintf(filename, "%s/unsorted_%d.bin", d, i);
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) handle_error("File open failed");
    
    unsigned int seed = i;
    for(int j = 0; j < r; j++) {
      int rand_num = rand_r(&seed);
      write(fd, &rand_num, sizeof(rand_num));  
    }
    
    close(fd);
  }
  
  return NULL;
}

int main(int argc, char *argv[]) {

  if(argc != 5) handle_error("Invalid arguments\n");
  
  d = argv[1];

  struct stat st; 
  if (stat(d, &st) == -1) handle_error("Directory issue\n");
  
  if (!S_ISDIR(st.st_mode)) handle_error("Not a directory\n"); 
  
  f = atoi(argv[2]);
  r = atoi(argv[3]);
  t = atoi(argv[4]);


  pthread_t threads[t];
  int thread_args[t];
  
  for (int i = 0; i < t; i++) {
    thread_args[i] = i; 
    pthread_create(&threads[i], NULL, thread_function, &thread_args[i]);
  }

  for (int i = 0; i < t; i++) {
    pthread_join(threads[i], NULL); 
  }

  return 0;
}

//gcc -g -Wall -Werror filecreator.c -o filecreator -lpthread
