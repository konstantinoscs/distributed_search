#include <dirent.h>
#include <stdio.h>
#include <unistd.h>

int worker_operate(char **paths, int pathsize, char *job_to_w, char *w_to_job){
  printf("Process:%d, pathsize %d\n", getpid(), pathsize);
  printf("Paths:\n");
  for(int i=0; i<pathsize; i++){
    printf("Process:%d path %s\n", getpid(), paths[i]);
  }
  printf("Process:%d fifo %s\n", getpid(), job_to_w);
  printf("Process:%d fifo %s\n", getpid(), w_to_job);
  //here load everything to memory - trie

  //here 
}
