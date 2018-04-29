#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "registry.h"
#include "search.h"
#include "trie.h"
#include "utilities.h"

volatile sig_atomic_t writeflag = 1;

void ack_deadline(int sig){
  writeflag = 1;
}

int free_documents(Registry **documents, int docsize){
  for(int i=0; i<docsize; i++){
    free((*documents)[i].path);
    for(int j=0; j<(*documents)[i].lines; j++){
      free((*documents)[i].text[j]);
    }
    free((*documents)[i].text);
  }
  free(*documents);
  *documents = NULL;
}

int byte_sum(Registry *documents, int docsize){
  int sum = 0;
  for(int i=0; i<docsize; i++)
    for(int j=0; j<documents[i].lines; j++)
      sum +=strlen(documents[i].text[j]);
  return sum;
}

void print_count_result(FILE *logfile, char *command, char *doc, int no_appear){
  time_t rawtime;
  struct tm *tinfo;
  time(&rawtime);
  tinfo = localtime(&rawtime);
  printf("Current local time and date: %s", asctime(tinfo));
  fprintf(logfile, "%d-%d-%d ", tinfo->tm_mday, tinfo->tm_mon, tinfo->tm_year+1900);
  fprintf(logfile, "%dh %dm %ds ", tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
  fprintf(logfile, ": %s : %s : %d\n", command, doc, no_appear);
}

int worker_operate(char *job_to_w, char *w_to_job){
  char **queries = NULL, *cmd = NULL, *abspath = malloc(1);
  char **paths = NULL, *filename = NULL;
  int fout, fin, nread = 0, nwrite = 0, docc = 0, docm = 2;
  int queriesNo, qlen = 0; //number of queries and temporary length
  int pathsize, pid = getpid(), total_words_found =0;
  Registry *documents = malloc(docm*sizeof(Registry));
  DIR *dir;
  struct dirent *ent;
  TrieNode *trie = NULL;
  FILE *logfile = NULL;
  //initialize signal handlers
  struct sigaction deadline;
  deadline.sa_handler = ack_deadline;
  sigemptyset (&(deadline.sa_mask));
  //new_worker.sa_flags = SA_SIGINFO;
  deadline.sa_flags = 0;
  sigaction(SIGALRM, &deadline, NULL);
  printf("Worker operate starts!\n");

  if ((fin = open(job_to_w, O_RDONLY)) < 0) {
    perror("fifo in child open problem");
    exit(1) ;
  }
  if ((fout = open(w_to_job, O_WRONLY)) < 0){
    perror("fifo out open error ");
    exit(1) ;
  }

  //read paths information
  nread = read(fin, &pathsize, sizeof(int));
  if (nread < 0) {
    perror ("problem in reading ");
    exit(5);
  }
  paths = malloc(pathsize*sizeof(char*));
  for(int i=0; i<pathsize; i++){
    nread = read(fin, &qlen, sizeof(int));
    if (nread < 0) {
      perror("problem in reading ");
      exit(5);
    }
    paths[i] = malloc(qlen);
    nread = read(fin, paths[i], qlen);
    if (nread < 0) {
      perror("problem in reading ");
      exit(5);
    }
    else if(!nread){
      break;
    }
  }

  //here load everything to memory - trie
  for(int i=0; i<pathsize; i++){
    //printf("Testing path:%s\n", paths[i]);
    if ((dir = opendir (paths[i])) != NULL) {
      /* print all the files and directories within directory */
      while ((ent = readdir (dir)) != NULL) {
        if(ent->d_type != DT_REG)
          continue;
        if(docc == docm){
          docm *=2;
          documents = realloc(documents, docm*sizeof(Registry));
        }
        //initialize new registry
        abspath = realloc(abspath, strlen(paths[i])+strlen(ent->d_name)+2);
        strcpy(abspath, paths[i]);
        strcat(abspath, "/");
        strcat(abspath, ent->d_name);
        documents[docc].path = malloc(strlen(abspath)+1);
        strcpy(documents[docc].path, abspath);
        parse_docfile(documents[docc].path, &(documents[docc].text), &(documents[docc].lines));
        docc++;
      }
      closedir (dir);
    }
    else {
    // could not open directory
      perror ("error");
      return EXIT_FAILURE;
    }
  }
  //here make trie and insert
  trie = makeTrie(documents, docc);
  //here
  filename = malloc(strlen("log/Worker_")+find_width(pid)+1);
  sprintf(filename, "log/Worker_%d", pid);
  logfile = fopen(filename, "w");

  while(1){
    nread = read(fin, &queriesNo, sizeof(int));
    if (nread < 0) {
      perror ("problem in reading ");
      exit(5);
    }
    else if(!nread){
      break;
    }
    printf("Read %d bytes/%d queries\n", nread, queriesNo);
    fflush(stdout);
    queries = malloc(queriesNo*sizeof(char*));
    for(int i=0; i<queriesNo; i++){
      //read the length
      nread = read(fin, &qlen, sizeof(int));
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      else if(!nread){
        break;
      }
      printf("Read %d qlen\n", qlen);
      fflush(stdout);
      //allocate space for each query
      queries[i] = malloc(qlen);
      //ready query
      nread = read(fin, queries[i], qlen);
      if (nread < 0) {
        perror ("problem in reading ");
        exit(5);
      }
      else if(!nread){
        break;
      }
      printf("Read query %s\n", queries[i]);
      fflush(stdout);
    }

    cmd = queries[0];
    if(!strcmp(cmd, "/search")){
      writeflag = 1;
      Result **results = malloc((queriesNo-3)*sizeof(Result*));
      int *results_no = malloc((queriesNo-3)*sizeof(int));
      search(results, results_no, trie, queries+1, queriesNo-3, documents, &total_words_found);
      if(write){  //in case we had a timeout
        for(int i=0; i<queriesNo-3; i++){
          //printf("Results for query: %s\n", queries[i]);
          //write number of the documents
          if((nwrite = write(fout, results_no+i, sizeof(int))) == -1){
            perror("Error in Writing ");
            exit(2);
          }
          for(int j=0; j<results_no[i]; j++){
            //write doc path
            qlen = strlen(results[i][j].doc) +1;
            if((nwrite = write(fout, &qlen, sizeof(int))) == -1){
              perror("Error in Writing ");
              exit(2);
            }
            //printf("Doc: %s\n", results[i][j].doc);
            if((nwrite = write(fout, results[i][j].doc, qlen)) == -1){
              perror("Error in Writing ");
              exit(2);
            }
            //write number of lines
            if((nwrite = write(fout, &results[i][j].size, sizeof(int))) == -1){
              perror("Error in Writing ");
              exit(2);
            }
            for(int k=0; k<results[i][j].size; k++){
              //write line no
              //printf("Line %d\n", results[i][j].line_no[k]);
              if((nwrite = write(fout, &results[i][j].line_no[k], sizeof(int))) == -1){
                perror("Error in Writing ");
                exit(2);
              }
              //write line length
              qlen = strlen(results[i][j].lines[k]) + 1;
              if((nwrite = write(fout, &qlen, sizeof(int))) == -1){
                perror("Error in Writing ");
                exit(2);
              }
              //write line
              //printf("%.50s\n", results[i][j].lines[k]);
              if((nwrite = write(fout, results[i][j].lines[k], qlen)) == -1){
                perror("Error in Writing ");
                exit(2);
              }
            }
          }
        }
      }
      print_results(logfile, results, results_no, queries+1, queriesNo-3);
      for(int i=0; i<queriesNo-3; i++){
        for(int j=0; j<results_no[i]; j++)
          free(results[i][j].lines);
        free(results[i]);
      }
      free(results_no);
      free(results);
    }
    else if(!strcmp(cmd, "/maxcount")){
      if(queriesNo > 1){
        char *doc = NULL;   //the path of the doc
        int no_appear;
        maxcount(trie, queries[1], &doc, &no_appear);
        qlen = doc ? strlen(doc) + 1 : 0;
        if((nwrite = write(fout, &qlen, sizeof(int))) == -1){
          perror("Error in Writing ") ;
          exit(2);
        }
        if(qlen){
          if((nwrite = write(fout, doc, qlen)) == -1){
            perror("Error in Writing ") ;
            exit(2);
          }
          if((nwrite = write(fout, &no_appear, sizeof(int))) == -1){
            perror("Error in Writing ") ;
            exit(2);
          }
          print_count_result(logfile, "maxcount", doc, no_appear);
        }
      }
      else
        fprintf(stderr, "No word was given for maxcount\n");
    }
    else if(!strcmp(cmd, "/mincount")){
      if(queriesNo > 1){
        char *doc = NULL;   //the path of the doc
        int no_appear;
        mincount(trie, queries[1], &doc, &no_appear);
        qlen = doc ? strlen(doc) + 1 : 0;
        //qlen = strlen(doc) +1;
        if((nwrite = write(fout, &qlen, sizeof(int))) == -1){
          perror("Error in Writing ") ;
          exit(2);
        }
        //check if word was found
        if(qlen){
          if((nwrite = write(fout, doc, qlen)) == -1){
            perror("Error in Writing ") ;
            exit(2);
          }
          if((nwrite = write(fout, &no_appear, sizeof(int))) == -1){
            perror("Error in Writing ") ;
            exit(2);
          }
          print_count_result(logfile, "mincount", doc, no_appear);
        }
      }
      else
        fprintf(stderr, "No word was given for mincount\n");
    }
    else if(!strcmp(cmd, "/wc")){
      int sum = byte_sum(documents, docc);
      if((nwrite = write(fout, &sum, sizeof(int))) == -1){
        perror("Error in Writing ") ;
        exit(2);
      }
    }
    else if(!strcmp(cmd, "/exit")){
      printf("caught exit \n");
      //send to parent all words that were found
      if((nwrite = write(fout, &total_words_found, sizeof(int))) == -1){
        perror("Error in Writing ");
        exit(2);
      }
      deleteQueries(&queries, queriesNo);
      break;
    }
    else{
      fprintf(stderr, "Unknown command, it will be ignored\n");
    }
    deleteQueries(&queries, queriesNo);
  }

  fclose(logfile);
  close(fin);
  close(fout);
  for(int i=0; i<pathsize; i++)
    free(paths[i]);
  free(paths);
  free(abspath);
  free_documents(&documents, docc);
  delete_trie(trie);
  free(trie);
  free(filename);
  printf("Will exit\n");
  return 1;
}
