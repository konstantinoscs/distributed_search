#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "registry.h"
#include "trie.h"
#include "utilities.h"

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

int worker_operate(char **paths, int pathsize, char *job_to_w, char *w_to_job){
  int fout, fin;
  int nread = 0, nwrite = 0;
  int docc = 0, docm = 2;
  char *abspath = malloc(1);
  Registry *documents = malloc(docm*sizeof(Registry));
  DIR *dir;
  struct dirent *ent;
  TrieNode *trie = NULL;
  int queriesNo, qlen = 0; //number of queries and temporary length
  char **queries = NULL, *cmd = NULL;

  /*printf("Process:%d, pathsize %d\n", getpid(), pathsize);
  printf("Paths:\n");
  for(int i=0; i<pathsize; i++)
    printf("Process:%d path %s\n", getpid(), paths[i]);
  printf("Process:%d fifo %s\n", getpid(), job_to_w);
  printf("Process:%d fifo %s\n", getpid(), w_to_job);*/
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

        //printf("Before\n");
        //printf ("%s\n", ent->d_name);
        abspath = realloc(abspath, strlen(paths[i])+strlen(ent->d_name)+2);
        strcpy(abspath, paths[i]);
        strcat(abspath, "/");
        strcat(abspath, ent->d_name);
        printf("abspath: %s\n", abspath);
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

  /*for(int i=0; i<docc; i++){
    printf("File: %s\n", documents[i].path);
    for(int j=0; j<documents[i].lines; j++){
      printf("%s\n", documents[i].text[j]);
    }
  }*/

  //here
  if ((fin = open(job_to_w, O_RDONLY)) < 0) {
    perror ("fifo in open problem") ;
    exit(1) ;
  }
  //sleep(3);

  if ((fout = open(w_to_job, O_WRONLY)) < 0){
    perror ( "fifo out open error " ) ;
    exit(1) ;
  }

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

    }
    else if(!strcmp(cmd, "/maxcount")){

    }
    else if(!strcmp(cmd, "/mincount")){

    }
    else if(!strcmp(cmd, "/wc")){
      int sum = byte_sum(documents, docc);
      if((nwrite = write(fout, &sum, sizeof(int))) == -1){
        perror(" Error in Writing ") ;
        exit(2);
      }
    }
    else if(!strcmp(cmd, "/exit")){
      deleteQueries(&queries, queriesNo);
      break;
    }
    else{
      fprintf(stderr, "Unknown command, it will be ignored\n");
    }
    deleteQueries(&queries, queriesNo);
  }

  //close(fin);
  free(abspath);
  free_documents(&documents, docc);
  delete_trie(trie);
  free(trie);
  printf("Will exit\n");
  return 1;
  //free(msgbuf);
}
