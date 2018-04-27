#ifndef SEARCH_H
#define SEARCH_H

#include "registry.h"
#include "trie.h"

int search(Result **results, int *results_no, TrieNode *trie, char **queries,
  int queriesNo, Registry *documents);

#endif
