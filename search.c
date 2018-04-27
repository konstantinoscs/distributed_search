#include <stdio.h>
#include <stdlib.h>

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
  int queriesNo, Registry *documents){
  TrieNode *tempTrie = NULL;
  DocInfo *tempInfo = NULL;
  for(int i=0; i<queriesNo; i++){
    results[i] = NULL;
    results_no[i] = 0;
    tempTrie = find_word(trie, queries[i]);
    if(tempTrie == NULL)  //avoid bad things
      continue;
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
