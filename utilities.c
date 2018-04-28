#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>


int parse_arguments(int argc, char ** argv, char** docfile, int *num_workers){
  int i=1;
  if(argc !=5 ){
    fprintf(stderr, "Wrong number of arguments given\n");
    exit(-1);
  }
  while(i<argc){
    if(!strcmp(argv[i], "-d")){ //parse docfile
      *docfile = malloc(strlen(argv[++i])+1);
      strcpy(*docfile, argv[i]);
    }
    else if(!strcmp(argv[i], "-w")){  //parse workers
      *num_workers = atoi(argv[++i]);
    }
    i++;  //move to the next arg one incremation has already been done
  }
  printf("w is %d\n", *num_workers);
  return 1;
}


//parsedocfile reads all the paths in the file specified by doc
int parse_docfile(char* doc, char*** paths, int *pathsize){
  int docm = 8, docc=0; //arbitary start from 8 words
  int wordm = 2, wordc=0; //start from a word with 2 characters
  int ch;
  FILE * docf = fopen(doc, "r");
  *pathsize = 0;

  *paths = malloc(docm*sizeof(char*));

  while((ch=getc(docf)) != EOF){
    ungetc(ch, docf);
    // fscanf(docf, "%d", &id);
    // //error checking
    // if(id != ++pid){
    //   fprintf(stderr, "ids %d, %d in docfile are not in the correct order\n", id, pid);
    //   fclose(docf);
    //   return 0;
    // }

    if(docc == docm){     //allocate space for more paths
      docm *= 2;
      *paths = realloc(*paths, docm*sizeof(char*));
    }
    (*paths)[docc] = malloc(wordm); //allocate memory for the word

    while((ch=getc(docf)) != '\n'){
      if(wordc+1 == wordm){  //realloc condition -- save space for '\0'
        wordm *= 2;
        (*paths)[docc] = realloc((*paths)[docc], wordm);
      }
      (*paths)[docc][wordc++] = ch; //save character in paths
    } //document is saved exactly as read --including whitespace

    (*paths)[docc][wordc] = '\0';
    (*paths)[docc] = realloc((*paths)[docc], wordc+1); //shrink to fit
    *pathsize = ++docc;    //necessary update in case of emergency
    wordm = 2;  //re-initialize for next document
    wordc = 0;
  }
  *paths = realloc(*paths, (*pathsize)*sizeof(char*)); //shrink to fit
  //*pathsize = id+1;
  fclose(docf);
  return 1;
}

int make_fifo_arrays(char ***job_to_w, char***w_to_job, int num_workers){
  int len = strlen("jtw") + 2;
  *job_to_w = malloc(num_workers*sizeof(char*));
  *w_to_job = malloc(num_workers*sizeof(char*));
  for(int i=0; i<num_workers; i++){
    (*job_to_w)[i] = malloc(len);
    (*w_to_job)[i] = malloc(len);
    sprintf((*job_to_w)[i], "jtw%d", i);
    sprintf((*w_to_job)[i], "wtj%d", i);
  }
  for(int i=0; i<num_workers; i++){
    if (mkfifo((*job_to_w)[i], 0666) == -1 ) {
      if (errno != EEXIST ) {
        perror ( " receiver : mkfifo " ) ;
        exit (6) ;
      }
    }
    if (mkfifo((*w_to_job)[i], 0666) == -1 ) {
      if (errno != EEXIST ) {
        perror ( "receiver : mkfifo " ) ;
        exit (6) ;
      }
    }
  }
}

int readQueries(char ***queries, int *queriesNo){
  int ch = ' ';
  int wordm=0, wordc=0;   //current query memory and query counter
  int queriesc = 0;

  if((ch =getchar()) == '\n')
    return 0;
  else
    ungetc(ch, stdin);

  *queriesNo = 2;
  *queries = malloc((*queriesNo)*sizeof(char*));
  while(ch!='\n'){
    wordm = 2;    //start from 1 character (plus '\0')
    wordc = 0;
    if(queriesc == 12){
      while((ch=getchar()) !='\n') continue;
      break;
    }
    if(queriesc == *queriesNo){
      (*queriesNo) *=2;
      *queries = realloc(*queries, (*queriesNo)*sizeof(char*));
    }
    (*queries)[queriesc] = malloc(wordm);
    while((ch=getchar())!= ' ' && ch!='\n'){
      if(wordc+1 == wordm){     //chech if reallocation needed
        wordm *= 2;
        (*queries)[queriesc] = realloc((*queries)[queriesc], wordm);
      }
      (*queries)[queriesc][wordc++] = ch;
    }
    (*queries)[queriesc][wordc] ='\0';
    (*queries)[queriesc] = realloc((*queries)[queriesc], wordc+1); //shrink tf
    queriesc++;
  }
  *queries = realloc(*queries, queriesc*sizeof(char*));
  *queriesNo = queriesc;
  return 1;
}

void deleteQueries(char ***queries, int queriesNo){
  if(*queries == NULL){
    fprintf(stderr, "NULL pointer given to free\n");
    return;
  }
  for(int i=0; i<queriesNo; i++)
    free((*queries)[i]);
  free(*queries);
  *queries = NULL;
}

int find_width(int number){
  int digits = 1;
  while (number/=10) digits++;
  return digits;
}

int free_worker(pid_t *child, char *docfile, int num_workers, char **job_to_w,
  char**w_to_job){

  free(child);  //copy on write, should be lightweight
  free(docfile);
  for (int i=0; i<num_workers; i++){
    free(job_to_w[i]);
    free(w_to_job[i]);
  }
  free(job_to_w);
  free(w_to_job);
}

int free_executor(pid_t *child, char *docfile, int pathsize, char **paths,
  int num_workers, char **job_to_w, char**w_to_job, int *fifo_in, int *fifo_out){

  free_worker(child, docfile, num_workers, job_to_w, w_to_job);
  for(int i=0; i<pathsize; i++)
    free(paths[i]);
  free(paths);
  free(fifo_in);
  free(fifo_out);
}
