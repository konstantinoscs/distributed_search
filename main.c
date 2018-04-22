#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utilities.h"
#include "worker.h"

int main(int argc, char **argv){
  char *docfile = NULL, **total_paths=NULL;
  char **paths=NULL; //for each process this is different
  char ** job_to_w=NULL, **w_to_job = NULL;
  int num_workers = 0, total_pathsize=0, pathsize=0, paths_until_now=0;
  int status, nwrite = 0, id; //child status and no of bytes written/read
  pid_t *child = NULL;
  int *fifo_in = NULL, *fifo_out = NULL;
  int msgsize = 66;
  char msgbuf[66]; //= malloc(msgsize);

  if(!parse_arguments(argc, argv, &docfile, &num_workers)){
    exit(-1);
  }
  child = malloc(num_workers*sizeof(pid_t));
  parse_docfile(docfile, &total_paths, &total_pathsize);
  paths = total_paths;
  printf("here\n");
  make_fifo_arrays(&job_to_w, &w_to_job, num_workers);
  //HERE assign number of paths to workers/ ceil(pathsize/workers)?
  pathsize = ceil((double)total_pathsize/(double)num_workers);
  printf("Total pathsize/General pathsize: %d/%d\n", total_pathsize, pathsize);
  //start forking workers
  for(int i=0; i<num_workers; i++){
    paths = total_paths + paths_until_now;
    if ((child[i] = fork()) == 0){
      id = i;
      //worker process
      worker_operate(paths, pathsize, job_to_w[i], w_to_job[i]);
      //free everything
      free_worker(child, docfile, total_pathsize, total_paths, num_workers,
        job_to_w, w_to_job);
      printf("child %d exiting!\n", id);
      exit(0);
    }
    else{
      //check if less paths than pathsize are left
      paths_until_now += pathsize;
      if(total_pathsize-paths_until_now < pathsize){
        pathsize = total_pathsize - paths_until_now;
      }
    }
  }

  fifo_in = malloc(num_workers*sizeof(int));
  fifo_out = malloc(num_workers*sizeof(int));

  /*for(int i=0; i<num_workers; i++){
    if ((fifo_out[i] = open(job_to_w[i], O_WRONLY)) < 0){
      perror ( "fifo out open error " ) ;
      exit(1);
    }
  }*/

  /*for(int i=0; i<num_workers; i++){
    sprintf(msgbuf, "Hello process %d", i);
    if((nwrite = write (fifo_out[i], msgbuf, msgsize) ) == -1){
      perror(" Error in Writing ") ;
      exit(2);
    }
  }*/
  printf("Exit\n");

  //father process
  /*while(1){
    scanf("%s", cmd);
    if(!strcmp(cmd, "/search")){
      if(!readQueries(&queries, &queriesNo)){
        fprintf(stderr, "No queries given!\n");
        continue;
      }
      search(trie, documents, doc_length, docsize, queries, queriesNo, k1, b,
        avgdl, k);
      deleteQueries(&queries, queriesNo);
    }
    else if(!strcmp(cmd, "/df")){
      if(!readQueries(&queries, &queriesNo)){
        document_frequency(trie);
        continue;
      }
      else if(queriesNo!=1){    //maybe support multiple words
        fprintf(stderr, "Wrong queries given!\n");
      }
      else{
        int df = document_appearances(trie, queries[0]);
        printf("%s %d\n", queries[0], df);
      }
      deleteQueries(&queries, queriesNo);
    }
    else if(!strcmp(cmd, "/tf")){
      if(!readQueries(&queries, &queriesNo)){
        fprintf(stderr, "No queries given!\n");
        continue;
      }
      else if(queriesNo!=2){
        fprintf(stderr, "Wrong queries given!\n");
      }
      else{
        int termf = term_frequency(trie, queries[1], atoi(queries[0]));
        printf("%d %s %d\n", atoi(queries[0]), queries[1], termf);
      }
      deleteQueries(&queries, queriesNo);
    }
    else if(!strcmp(cmd, "/exit")){
      break;
    }
    else{
      fprintf(stderr, "Unknown command, try again\n");
    }
    printf("\n");
  }*/
  for (int i=0; i<num_workers; i++){
    //close(fifo_out[i]);
    unlink(job_to_w[i]);
    unlink(w_to_job[i]);
  }

  for (int i=0; i<num_workers; i++) // kill
        wait(&status);
  free_executor(child, docfile, total_pathsize, total_paths, num_workers,
    job_to_w, w_to_job, fifo_in, fifo_out);
}
