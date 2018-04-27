#ifndef TRIE_H
#define TRIE_H

#include "plist.h"
#include "registry.h"

typedef struct TrieNode{
  char letter;          //the character of this node
  Plist* list;          //posting list if a word ends here
  struct TrieNode *next;       //nodes in the same level
  struct TrieNode *down;  //next letters
}TrieNode;

//initializes a trienode with character ch
void init_trie(TrieNode *node, char ch);
//inserts a word in the trie with root node
void insert_in_trie(TrieNode *node, char *word, int pos, int len, char *path,
  int line, int docindex);
void print_trie(TrieNode *node);
//deletes trie
void delete_trie(TrieNode *node);
//constructs trie with all words in the documents
TrieNode* makeTrie(Registry *documents, int docsize);
//find specific word in trie
TrieNode* find_word(TrieNode *node, char* word);

void maxcount(TrieNode *node, char *word, char **doc, int *no_appear);
void mincount(TrieNode *node, char *word, char **doc, int *no_appear);


#endif
