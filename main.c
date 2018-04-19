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
  int status;
  pid_t *child = NULL;

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
  //start forking workers
  for(int i=0; i<num_workers; i++){
    paths = total_paths + paths_until_now;
    if ((child[i] = fork()) == 0){
      //worker process
      free(child);  //copy on write, should be lightweight
      child = NULL;
      //worker operate
      worker_operate(paths, pathsize, job_to_w[i], w_to_job[i]);

      free(docfile);
      for(int i=0; i<total_pathsize; i++)
        free(total_paths[i]);
      free(total_paths);
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

  for (int i=0; i<num_workers; i++)
        wait(&status);
  for(int i=0; i<total_pathsize; i++)
    free(total_paths[i]);
  free(child);
  free(docfile);
  free(total_paths);
}
