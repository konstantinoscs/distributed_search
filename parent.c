#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utilities.h"
#include "worker.h"

int child_exit = 0;
pid_t dead_child;

void parent_maxcount(int queriesNo, int num_workers, int *fifo_in, int *fifo_out){
  if(queriesNo > 1){
    int max_appears = 0, appears = 0, qlen, nread;
    char *doc = malloc(1), *maxdoc = malloc(1);
    for(int i=0; i<num_workers; i++){
      //read directory string length
      nread = read(fifo_in[i], &qlen, sizeof(int));
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      doc = realloc(doc, qlen);
      //read directory path
      nread = read(fifo_in[i], doc, qlen);
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      //read appearances of word
      nread = read(fifo_in[i], &appears, sizeof(int));
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      printf("Parent Appearances %d, path %s\n", appears, doc);
      if(appears > max_appears){
        max_appears = appears;
        maxdoc = realloc(maxdoc, qlen);
        strcpy(maxdoc, doc);
      }
    }
    printf("Parent max Appearances %d, path %s\n", max_appears, maxdoc);
    free(doc);
    free(maxdoc);
  }
  else{
    fprintf(stderr, "Parent: no word was given for maxcount\n");
  }
}


void parent_mincount(int queriesNo, int num_workers, int *fifo_in, int *fifo_out){
  if(queriesNo > 1){
    int min_appears = 0, appears = 0, qlen, nread;
    char *doc = malloc(1), *mindoc = malloc(1);

    nread = read(fifo_in[0], &qlen, sizeof(int));
    if (nread < 0) {
      perror ("problem in reading ");
      exit(5);
    }
    mindoc = realloc(mindoc, qlen);
    //read directory path
    nread = read(fifo_in[0], mindoc, qlen);
    if (nread < 0) {
      perror ("problem in reading ");
      exit(5);
    }
    //read appearances of word
    nread = read(fifo_in[0], &min_appears, sizeof(int));
    if (nread < 0) {
      perror ("problem in reading ");
      exit(5);
    }

    for(int i=1; i<num_workers; i++){
      //read directory string length
      nread = read(fifo_in[i], &qlen, sizeof(int));
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      doc = realloc(doc, qlen);
      //read directory path
      nread = read(fifo_in[i], doc, qlen);
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      //read appearances of word
      nread = read(fifo_in[i], &appears, sizeof(int));
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      printf("Parent Appearances %d, path %s\n", appears, doc);
      if(appears < min_appears){
        min_appears = appears;
        mindoc = realloc(mindoc, qlen);
        strcpy(mindoc, doc);
      }
    }
    printf("Parent min Appearances %d, path %s\n", min_appears, mindoc);
    free(doc);
    free(mindoc);
  }
  else{
    fprintf(stderr, "Parent: no word was given for maxcount\n");
  }
}

int parent_operate(int num_workers, pid_t *child, char *docfile, char **job_to_w,
  char **w_to_job){
  //variable declaration for father process
  int status, nwrite = 0, nread = 0; //child status and no of bytes written/read
  int *fifo_in = NULL, *fifo_out = NULL;
  char **queries = NULL, **paths = NULL;
  int total_pathsize=0, pathsize=0, paths_until_now=0;
  int queriesNo, qlen = 0;

  fifo_in = malloc(num_workers*sizeof(int));
  fifo_out = malloc(num_workers*sizeof(int));

  //open fifos to write to children
  for(int i=0; i<num_workers; i++){
    if ((fifo_out[i] = open(job_to_w[i], O_WRONLY)) < 0){
      perror ( "fifo out open error " ) ;
      exit(1);
    }
  }

  for(int i=0; i<num_workers; i++){
    if ((fifo_in[i] = open(w_to_job[i], O_RDONLY)) < 0){
      perror ( "fifo in open error " ) ;
      exit(1);
    }
  }
  parse_docfile(docfile, &paths, &total_pathsize);
  pathsize = ceil((double)total_pathsize/(double)num_workers);
  printf("Total pathsize/General pathsize: %d/%d\n", total_pathsize, pathsize);
  for(int i=0; i<num_workers; i++){
    if((nwrite = write(fifo_out[i], &pathsize, sizeof(int))) == -1){
      perror("Error in Writing ") ;
      exit(2);
    }
    //write paths to the fifo
    for(int j=0; j<pathsize; j++){
      qlen = strlen(paths[j+paths_until_now]) +1;
      if((nwrite = write(fifo_out[i], &qlen, sizeof(int))) == -1){
        perror("Error in Writing ") ;
        exit(2);
      }
      if((nwrite = write(fifo_out[i], paths[j+paths_until_now], qlen)) == -1){
        perror("Error in Writing ") ;
        exit(2);
      }
    }
    paths_until_now += pathsize;
    if(total_pathsize-paths_until_now < pathsize){
      pathsize = total_pathsize - paths_until_now;
    }
  }
  //father process
  while(1){
    readQueries(&queries, &queriesNo);
    printf("Got queries:\n");
    for(int j=0; j<queriesNo; j++)
      printf("%s\n", queries[j]);
    for(int i=0; i<num_workers; i++){
      //write
      if((nwrite = write(fifo_out[i], &queriesNo, sizeof(int))) == -1){
        perror(" Error in Writing ") ;
        exit(2);
      }
    }
    for(int j=0; j<queriesNo; j++){
      qlen = strlen(queries[j]) +1; //add space for \0
      //write len of the query
      for(int i=0; i<num_workers; i++){
        if((nwrite = write(fifo_out[i], &qlen, sizeof(int))) == -1){
          perror(" Error in Writing ") ;
          exit(2);
        }
        if((nwrite = write(fifo_out[i], queries[j], qlen)) == -1){
          perror(" Error in Writing ") ;
          exit(2);
        }
      }
    }
    if(!strcmp(queries[0], "/search")){

    }
    else if(!strcmp(queries[0], "/maxcount")){
      parent_maxcount(queriesNo, num_workers, fifo_in, fifo_out);
    }
    else if(!strcmp(queries[0], "/mincount")){
      parent_mincount(queriesNo, num_workers, fifo_in, fifo_out);
    }
    else if(!strcmp(queries[0], "/wc")){
      int tsum = 0, sum = 0;
      for(int i=0; i<num_workers; i++){
        nread = read(fifo_in[i], &tsum, sizeof(int));
        if (nread < 0) {
          perror ("problem in reading ");
          exit(5);
        }
        sum += tsum;
      }
      printf("Total number of bytes in all files: %d\n", sum);
    }
    else if(!strcmp(queries[0], "/exit")){
      deleteQueries(&queries, queriesNo);
      break;
    }
    else{
      fprintf(stderr, "Unknown command, it will be ignored\n");
    }
    deleteQueries(&queries, queriesNo);
  }

  for (int i=0; i<num_workers; i++){
    close(fifo_out[i]);
    close(fifo_in[i]);
    unlink(job_to_w[i]);
    unlink(w_to_job[i]);
  }
  for (int i=0; i<num_workers; i++) // kill
        wait(&status);
  free_executor(child, docfile, total_pathsize, paths, num_workers,
    job_to_w, w_to_job, fifo_in, fifo_out);
}
