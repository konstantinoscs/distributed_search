#include <stdio.h>



int parse_arguments(int argc, char ** argv, char** docfile, int *num_workers){
  int i=1;
  if(argc !=5 ){
    fprintf(stderr, "Wrong number of arguments given\n");
    exit(-1);
  }
  while(i<argc){
    if(!strcmp(argv[i], "-D")){ //parse docfile
      *doc = malloc(strlen(argv[++i])+1);
      strcpy(*doc, argv[i]);
    }
    else if(!strcmp(argv[i], "-w")){  //parse workers
      *k = atoi(argv[++i]);
    }
    i++;  //move to the next arg one incremation has already been done
  }
  printf("w is %d\n", *k);
  return 1;
}


//parsedocfile reads all the paths in the file specified by doc
int parse_docfile(char* doc, char*** paths, int *pathsize){
  int docm = 8 docc=0; //arbitary start from 8 words
  int wordm = 2, wordc=0; //start from a word with 2 characters
  int ch;
  FILE * docf = fopen(doc, "r");

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
    (*paths)[id] = malloc(wordm); //allocate memory for the word

    while((ch=getc(docf)) != '\n'){
      if(wordc+1 == wordm){  //realloc condition -- save space for '\0'
        wordm *= 2;
        (*paths)[id] = realloc((*paths)[id], wordm);
      }
      (*paths)[id][wordc++] = ch; //save character in paths
    } //document is saved exactly as read --including whitespace

    (*paths)[id][wordc] = '\0';
    (*paths)[id] = realloc((*paths)[id], wordc+1); //shrink to fit
    *pathsize = ++docc;    //necessary update in case of emergency
    wordm = 2;  //re-initialize for next document
    wordc = 0;
  }
  *paths = realloc(*paths, (*pathsize)*sizeof(char*)); //shrink to fit
  //*pathsize = id+1;
  fclose(docf);
  return 1;
}
