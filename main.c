#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

#include "utilities.h"

int main(int argc, char **argv){
  char *docfile = NULL **paths=NULL;
  int num_workers = 0, pathsize=0;
  pid_t *child = NULL;
  if(!parseArguments(argc, argv, &docfile, &num_workers)){
    exit(-1);
  }
  child = malloc(num_workers*sizeof(pid_t));
  parse_docfile(docfile, &paths, &pathsize);
  //HERE assign number of paths to workers/ ceil(pathsize/workers)?
  //start forking workers
  for(int i=0; i<num_workers; i++){
    if ((child[i] = fork()) == 0){
      //worker process
      free(child);  //copy on write, should be lightweight
      free(docfile);
      free(paths);
      exit(0);
    }
  }
  //father process
}
