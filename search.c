#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "registry.h"
#include "trie.h"

void update_results(Result *result, DocInfo *info, Registry *documents){
  result->doc = info->doc;
  result->size = info->no_lines;
  //result->line_no = malloc(info->no_lines*sizeof(int));
  result->line_no = info->appearance;
  result->lines = malloc(result->size*sizeof(char*));
  //printf("%s\n", documents[0].text[0]);
  for(int i=0; i<result->size; i++)
    result->lines[i] = documents[info->docindex].text[result->line_no[i]];
}

int search(Result **results, int *results_no, TrieNode *trie, char **queries,
  int queriesNo, Registry *documents, int *total_words_found){
  TrieNode *tempTrie = NULL;
  DocInfo *tempInfo = NULL;
  for(int i=0; i<queriesNo; i++){
    results[i] = NULL;
    results_no[i] = 0;
    tempTrie = find_word(trie, queries[i]);
    if(tempTrie == NULL)  //avoid bad things
      continue;
    (*total_words_found)++;
    tempInfo = tempTrie->list->info;
    //printf("Searching in dir %s\n", tempInfo->doc);
    int resctr = 0;
    while(tempInfo != NULL){
      results[i] = realloc(results[i], (resctr+1)*sizeof(Result));
      update_results(&(results[i][resctr]), tempInfo, documents);
      resctr++;
      tempInfo = tempInfo->next;
    }
    results_no[i] = resctr;
  }
}

void print_results(FILE *logfile, Result **results, int *results_no,
  char **queries, int queriesNo){

  time_t rtime;
  struct tm *tinfo;
  time(&rtime);
  for(int i=0; i<queriesNo; i++){
    if(!results_no[i]) continue;
    tinfo = localtime(&rtime);
    fprintf(logfile, "%d-%d-%d ", tinfo->tm_mday, tinfo->tm_mon, tinfo->tm_year+1900);
    fprintf(logfile, "%dh %dm %ds : ", tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
    fprintf(logfile, "search : %s", queries[i]);
    for(int j=0; j<results_no[i]; j++){
      fprintf(logfile, " : %s", results[i][j].doc);
    }
    fprintf(logfile, "\n");
  }
}
