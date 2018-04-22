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

/*void print_freq(FreqInfo *frequency){
  printf("[%d, %d]", frequency->doc, frequency->appreance);
  if(frequency->next!=NULL){
    printf(",");
    print_freq(frequency->next);
  }
}

//returns the appearances of a word in a specific doc
int search_n_get(FreqInfo *frequency, int docno){
  FreqInfo *current = frequency;
  while(current->doc != docno){
    if(current->next == NULL)
      return 0;
    current = current->next;
  }
  return current->appreance;
}*/

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

/*void print_list(Plist *list){
  printf("[");
  print_freq(list->frequencies);
  printf("]");
}

int calculate_frequency(Plist *list){
  return list->appearances;
}

int caclulate_specific_frequency(Plist *list, int docno){
  return search_n_get(list->frequencies, docno);
}

int calculate_doc_appearances(Plist *list){
  return list->docs;
}

//store all document IDs that a word appears in in array ids
void get_all_docid(Plist *list, int **ids, int*idsize){
  FreqInfo *current = list->frequencies;
  int currentId = 0;
  *idsize = list->docs;
  *ids = malloc(list->docs*sizeof(int));
  while(current!=NULL){
    (*ids)[currentId++] = current->doc;
    current = current->next;
  }
}*/
