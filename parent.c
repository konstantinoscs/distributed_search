#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <poll.h>
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

volatile sig_atomic_t send_kill;
volatile sig_atomic_t child_exit;
pid_t *dead_child;

//signal handler for a child death
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
  }
}

void alarm_off(int sig){
  send_kill = 1;
  printf("Driiiiiiiiiiiiiing, Alarm!\n");
}

void send_kill_children(int num_workers, pid_t *child){
  for(int i=0; i<num_workers; i++)
    kill(child[i], SIGALRM);
  //reset send_kill
  send_kill = 0;
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

struct pollfd *make_fds_array(int num_workers, int *fifo_in){
  struct pollfd *fds = malloc(num_workers*sizeof(struct pollfd));
  for(int i=0; i<num_workers; i++){
    fds[i].fd = fifo_in[i];
    fds[i].events = POLLIN;
  }
  return fds;
}

void child_spawn(pid_t *child, int num_workers, int total_pathsize, char** paths,
  char** job_to_w, char **w_to_job, int *fifo_in, int *fifo_out, char *docfile,
  char ***queries, int queriesNo, struct pollfd *fds){

  int paths_until_now=0, nwrite, qlen;
  int pathsize = ceil((double)total_pathsize/(double)num_workers);
  for(int i=0; i<num_workers; i++){
    for(int k=0; k<child_exit; k++){
      if(dead_child[k] == child[i]){
        //open new fifo here
        //keep descriptors to give to new worker
        int fin;
        int fout;
        remake_fifos(fifo_in[i], fifo_out[i], job_to_w[i], w_to_job[i]);
        //update fds array
        fds[i].fd = fifo_in[i];
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
          worker_operate(jtw, wtj);
          free(jtw);
          free(wtj);
          free(fds);
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

void print_search_results(int fin, char **queries, int queriesNo){
  int nread, results_no, qlen, size, line_no;
  char *doc = malloc(1), *line = malloc(1);
  for(int i=0; i<queriesNo; i++){
    printf("Results for query: %s\n", queries[i]);
    nread = read(fin, &results_no, sizeof(int));
    if(nread < 0){
      perror("problem in reading ");
      exit(5);
    }
    for(int j=0; j<results_no; j++){
      //read doc len
      if((nread = read(fin, &qlen, sizeof(int))) < 0){
        perror("problem in reading ");
        exit(5);
      }
      doc = realloc(doc, qlen);
      if((nread = read(fin, doc, qlen)) < 0){
        perror("problem in reading ");
        exit(5);
      }
      printf("Document %s\n", doc);
      if((nread = read(fin, &size, sizeof(int))) < 0){
        perror("problem in reading ");
        exit(5);
      }
      for(int k=0; k<size; k++){
        //read number of line
        if((nread = read(fin, &line_no, sizeof(int))) < 0){
          perror("problem in reading ");
          exit(5);
        }
        printf("line %d\n", line_no);
        //read len of line
        if((nread = read(fin, &qlen, sizeof(int))) < 0){
          perror("problem in reading ");
          exit(5);
        }
        line = realloc(line, qlen);
        if((nread = read(fin, line, qlen)) < 0){
          perror("problem in reading ");
          exit(5);
        }
        printf("%.50s\n", line);
      }
    }
    putchar('\n');
  }
  free(doc);
  free(line);
  printf("PARENT EXITING\n");
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
      if(qlen){
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
        if(appears > max_appears || (appears == max_appears && strcmp(doc, maxdoc) < 0)){
          max_appears = appears;
          maxdoc = realloc(maxdoc, qlen);
          strcpy(maxdoc, doc);
        }
      }
    }
    if(max_appears)
      printf("Parent max Appearances %d, path %s\n", max_appears, maxdoc);
    else
      printf("Word wasn't found in any document!\n");
    free(doc);
    free(maxdoc);
  }
  else{
    fprintf(stderr, "Parent: no word was given for maxcount\n");
  }
}

void parent_mincount(int queriesNo, int num_workers, int *fifo_in, int *fifo_out){
  if(queriesNo > 1){
    int min_appears = INT_MAX, appears = 0, qlen=0, nread, j=0;
    char *doc = malloc(1), *mindoc = malloc(1);

    for(int i=0; i<num_workers; i++){
      //read directory string length
      nread = read(fifo_in[i], &qlen, sizeof(int));
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      if(qlen){
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
    }
    if(min_appears != INT_MAX)
      printf("Parent min Appearances %d, path %s\n", min_appears, mindoc);
    else
      printf("Word wasn't found in any document!\n");
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
  //initialize signal handlers
  struct sigaction new_worker, alarmhand;
  new_worker.sa_handler = child_death;
  alarmhand.sa_handler = alarm_off;
  sigemptyset (&(new_worker.sa_mask));
  sigemptyset (&(alarmhand.sa_mask));
  //new_worker.sa_flags = SA_SIGINFO;
  new_worker.sa_flags = 0;
  alarmhand.sa_flags = 0;
  sigaction(SIGCHLD, &new_worker, NULL);
  sigaction(SIGALRM, &alarmhand, NULL);

  fifo_in = malloc(num_workers*sizeof(int));
  fifo_out = malloc(num_workers*sizeof(int));

  //open fifos to write to children
  for(int i=0; i<num_workers; i++){
    if((fifo_out[i] = open(job_to_w[i], O_WRONLY)) < 0){
      perror("fifo out open error ");
      exit(1);
    }
  }

  for(int i=0; i<num_workers; i++){
    if((fifo_in[i] = open(w_to_job[i], O_RDONLY)) < 0){
      perror("fifo in open error ");
      exit(1);
    }
  }
  parse_docfile(docfile, &paths, &total_pathsize);
  pathsize = ceil((double)total_pathsize/(double)num_workers);
  printf("Total pathsize/General pathsize: %d/%d\n", total_pathsize, pathsize);
  for(int i=0; i<num_workers; i++){
    if((nwrite = write(fifo_out[i], &pathsize, sizeof(int))) == -1){
      perror("Error in Writing ");
      exit(2);
    }
    //write paths to the fifo
    for(int j=0; j<pathsize; j++){
      qlen = strlen(paths[j+paths_until_now]) +1;
      if((nwrite = write(fifo_out[i], &qlen, sizeof(int))) == -1){
        perror("Error in Writing ");
        exit(2);
      }
      if((nwrite = write(fifo_out[i], paths[j+paths_until_now], qlen)) == -1){
        perror("Error in Writing ");
        exit(2);
      }
    }
    paths_until_now += pathsize;
    if(total_pathsize-paths_until_now < pathsize){
      pathsize = total_pathsize - paths_until_now;
    }
  }
  struct pollfd* fds = make_fds_array(num_workers, fifo_in);
  //father process
  while(1){
    while(!readQueries(&queries, &queriesNo)) continue;
    printf("Got queries:\n");
    for(int j=0; j<queriesNo; j++)
      printf("%s\n", queries[j]);
    //respawn any processes if nessecary
    if(child_exit){
      printf("Dead children!\n");
      child_spawn(child, num_workers, total_pathsize, paths, job_to_w, w_to_job,
        fifo_in, fifo_out, docfile, &queries, queriesNo, fds);
      child_exit = 0;
    }

    //error checking for search
    if(!strcmp(queries[0], "/search")){
      if(queriesNo < 4 || strcmp(queries[queriesNo-2], "-d") ||
      !atof(queries[queriesNo-1])){
        fprintf(stderr, "No (valid) timeout was given for /search\n");
        deleteQueries(&queries, queriesNo);
        continue;
      }
    }

    for(int i=0; i<num_workers; i++){
      //write
      if((nwrite = write(fifo_out[i], &queriesNo, sizeof(int))) == -1){
        perror("Error in Writing ");
        exit(2);
      }
    }
    for(int j=0; j<queriesNo; j++){
      qlen = strlen(queries[j]) +1; //add space for \0
      //write len of the query
      for(int i=0; i<num_workers; i++){
        if((nwrite = write(fifo_out[i], &qlen, sizeof(int))) == -1){
          perror("Error in Writing ");
          exit(2);
        }
        if((nwrite = write(fifo_out[i], queries[j], qlen)) == -1){
          perror("Error in Writing ");
          exit(2);
        }
      }
    }
    if(!strcmp(queries[0], "/search")){
      //start timeout
      int ctr = 0, ctrdead = 0;
      alarm(atoi(queries[queriesNo-1]));
      while(ctr+ctrdead < num_workers){
        if(poll(fds, num_workers, -1) == -1){
          if(errno == EINTR)  //timeout
            break;
          perror("Error in poll ");
          exit(3);
        }
        printf("Exited POLL\n");
        for(int j=0; j<num_workers; j++){
          if(fds[j].revents == POLLIN){
            //read search results
            print_search_results(fifo_in[j], queries+1, queriesNo-3);
            ctr++;
          }
          else if(fds[j].revents == POLLHUP){
            ctrdead++;
          }
        }
      }
      alarm(0);
      if(send_kill){
        send_kill_children(num_workers, child);
        send_kill = 0;
      }
      //add poll again to synchronize
      if(poll(fds, num_workers, 0) == -1){
        perror("Error in poll ");
        exit(3);
      }
      for(int j=0; j<num_workers; j++){
        if(fds[j].revents == POLLIN){
          //read search results
          print_search_results(fifo_in[j], queries+1, queriesNo-3);
          ctr++;
        }
        else if(fds[j].revents == POLLHUP){
          ctrdead++;
        }
      }
      printf("Got answer back from %d workers and %d workers died\n", ctr, ctrdead);
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
        if ((nread = read(fifo_in[i], &tsum, sizeof(int))) < 0) {
          perror ("problem in reading ");
          exit(5);
        }
        sum += tsum;
      }
      printf("Total number of bytes in all files: %d\n", sum);
    }
    else if(!strcmp(queries[0], "/exit")){
      int total_words = 0;
      for(int i=0; i<num_workers; i++){
        if ((nread = read(fifo_in[i], &total_words, sizeof(int))) < 0) {
          perror ("problem in reading ");
          exit(5);
        }
        printf("Child with id %d found %d total words in its files\n", child[i], total_words);
      }
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
  free(fds);
  free(dead_child);
}
