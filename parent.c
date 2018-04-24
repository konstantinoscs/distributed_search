#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
