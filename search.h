#ifndef SEARCH_H
#define SEARCH_H

#include "registry.h"
#include "trie.h"

int search(Result **results, int *results_no, TrieNode *trie, char **queries,
  int queriesNo, Registry *documents, int *total_words_found);

void print_results(FILE *logfile, Result **results, int *results_no,
  char **queries, int queriesNo);

#endif
