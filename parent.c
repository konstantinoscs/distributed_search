#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "utilities.h"
#include "worker.h"

int child_exit;
pid_t *dead_child;

void child_death(int sig){
  printf("Caught a child_death\n");
  while(1){
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid <= 0) {
        break;
    }
    printf("Child that died is %d\n", pid);
    dead_child = realloc(dead_child, (++child_exit)*sizeof(pid_t));
    dead_child[child_exit-1] = pid;
    // something happened with child 'pid', do something about it...
    // Details are in 'status', see waitpid() manpage
  }
}

void remake_fifos(int fifo_in, int fifo_out, char *job_to_w, char *w_to_job){
  //destroy existing fifos
  close(fifo_out);
  close(fifo_in);
  unlink(job_to_w);
  unlink(w_to_job);
  //make new fifo
  if(mkfifo(job_to_w, 0666) == -1){
    if(errno != EEXIST ){
      perror("receiver : mkfifo ");
      exit(6) ;
    }
  }
  if(mkfifo(w_to_job, 0666) == -1){
    if (errno != EEXIST ){
      perror ("receiver : mkfifo ");
      exit(6);
    }
  }
}

void child_spawn(pid_t *child, int num_workers, int total_pathsize, char** paths,
  char** job_to_w, char **w_to_job, int *fifo_in, int *fifo_out, char *docfile,
  char ***queries, int queriesNo){

  int paths_until_now=0, nwrite, qlen;
  int pathsize = ceil((double)total_pathsize/(double)num_workers);
  printf("Located a dead child\n");
  for(int i=0; i<num_workers; i++){
    for(int k=0; k<child_exit; k++){
      if(dead_child[k] == child[i]){
        //open new fifo here
        //keep descriptors to give to new worker
        int fin;
        int fout;
        printf("Will remake fifos\n");
        remake_fifos(fifo_in[i], fifo_out[i], job_to_w[i], w_to_job[i]);
        printf("Remade fifos\n");
        //fork here
        if((child[i]=fork()) == 0){
          //keep only the name of the correct fifos
          char *jtw = malloc(strlen(job_to_w[i])+1);
          strcpy(jtw, job_to_w[i]);
          char *wtj = malloc(strlen(w_to_job[i])+1);
          strcpy(wtj, w_to_job[i]);
          //close all the fifos except those of the forked process
          for (int j=0; j<num_workers; j++){
            //if(j==i) continue;
            close(fifo_out[j]);
            close(fifo_in[j]);
          }
          //free memory inherrited from father
          deleteQueries(queries, queriesNo);
          free_executor(child, docfile, total_pathsize, paths, num_workers, job_to_w,
            w_to_job, fifo_in, fifo_out);
          printf("i:%d, fifo in: %s, fifo out: %s\n", i, jtw, wtj);
          worker_operate(jtw, wtj);
          free(jtw);
          free(wtj);
          free(dead_child);
          exit(0);
        }
        else{
          if((fifo_out[i] = open(job_to_w[i], O_WRONLY)) < 0){
            perror ("fifo out open error") ;
            exit(1);
          }
          printf("Parent opened fifo\n");
          if((fifo_in[i] = open(w_to_job[i], O_RDONLY)) < 0){
            perror ("fifo in parent open error ");
            exit(1);
          }

          printf("parent tries to write\n");
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
          break;
        }
      }
    }
    paths_until_now += pathsize;
    if(total_pathsize-paths_until_now < pathsize){
      pathsize = total_pathsize - paths_until_now;
    }
  }
}

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
  //initialize signal handler
  struct sigaction new_worker;
  new_worker.sa_handler = child_death;
  sigemptyset (&(new_worker.sa_mask));
  //new_worker.sa_flags = SA_SIGINFO;
  new_worker.sa_flags = 0;
  sigaction(SIGCHLD, &new_worker, NULL);

  fifo_in = malloc(num_workers*sizeof(int));
  fifo_out = malloc(num_workers*sizeof(int));

  //open fifos to write to children
  for(int i=0; i<num_workers; i++){
    if((fifo_out[i] = open(job_to_w[i], O_WRONLY)) < 0){
      perror ( "fifo out open error " ) ;
      exit(1);
    }
  }

  for(int i=0; i<num_workers; i++){
    if((fifo_in[i] = open(w_to_job[i], O_RDONLY)) < 0){
      perror ( "fifo in open error ");
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
    if(child_exit){
      printf("Dead children!\n");
      child_spawn(child, num_workers, total_pathsize, paths, job_to_w, w_to_job,
        fifo_in, fifo_out, docfile, &queries, queriesNo);
      child_exit = 0;
    }
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
  free(dead_child);
}
