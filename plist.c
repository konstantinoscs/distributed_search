#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plist.h"

void delete_doc(DocInfo *info){
  if(info->next!=NULL){
    delete_doc(info->next);
    free(info->next);
  }
  free(info->appearance);
}

void delete_list(Plist *list){
  if(list->info!=NULL){
    delete_doc(list->info);
    free(list->info);
  }
}

//update the posting list for a word
void search_n_update(Plist *list, char *path, int line){
  DocInfo *temp = NULL;
  if(strcmp(list->last->doc, path)){
    temp = malloc(sizeof(DocInfo));
    temp->doc = path;
    temp->appearance = malloc(sizeof(int));
    temp->appearance[0] = line;
    temp->no_lines = 1;
    temp->next = NULL;
    list->last->next = temp;
    list->last = temp;
  }
  else{
    if(list->last->appearance[list->last->no_lines-1] != line){
      list->last->no_lines++;
      list->last->appearance = realloc(list->last->appearance,
        list->last->no_lines*sizeof(int));
      list->last->appearance[list->last->no_lines-1] = line;
    }
    list->last->no_appear++;
  }
}
