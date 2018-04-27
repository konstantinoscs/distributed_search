#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "registry.h"
#include "plist.h"
#include "trie.h"

//initialize a trie node
void init_trie(TrieNode *node, char ch){
  node->letter = ch;
  node->list = NULL;
  node->next = NULL;
  node->down = NULL;
}

//initialize the posting list for a word
void init_plist(TrieNode *node, char* path, int line, int docindex){
  node->list = malloc(sizeof(Plist));
  //initialize the frequencies list
  node->list->info = malloc(sizeof(DocInfo));
  node->list->info->doc = path;
  node->list->info->docindex = docindex;
  node->list->info->appearance = malloc(sizeof(int)); //add the first appearance
  node->list->info->appearance[0] = line;
  node->list->info->no_lines = 1;
  node->list->info->no_appear = 1;
  node->list->info->next = NULL;
  node->list->last = node->list->info; //last is also the first
}

void insert_in_trie(TrieNode *node, char *word, int pos, int len, char *path,
  int line, int docindex){
  //printf("%c, p:%d, l:%d\n", word[pos], pos, len);
  if(node->letter == word[pos]){
    if(pos+1 == len){  //word has ended with the current letter
      if(node->list ==NULL)
        init_plist(node, path, line, docindex);
      else
        search_n_update(node->list, path, line, docindex);
    }
    else{   //go one level down to next letter (vertical movement)
      if(node->down !=NULL && node->down->letter > word[pos+1]){ //(LAW &) ORDER
        TrieNode* temp = malloc(sizeof(TrieNode));
        init_trie(temp, word[pos+1]);
        temp->next = node->down;
        node->down = temp;
      }
      else{
        if(node->down == NULL){
          node->down = malloc(sizeof(TrieNode));
          init_trie(node->down, word[pos+1]);
        }
      }
      insert_in_trie(node->down, word, ++pos, len, path, line, docindex);
    }
  }
  else { //go to the next letter (horizontal movement)
    if(node->next!=NULL && node->next->letter > word[pos]){ //maintain order
      TrieNode* temp = malloc(sizeof(TrieNode));
      init_trie(temp, word[pos]);
      temp->next = node->next;
      node->next = temp;
    }
    else{
      if(node->next ==NULL){
        node->next = malloc(sizeof(TrieNode));
        init_trie(node->next, word[pos]);
      }
    }
    insert_in_trie(node->next, word, pos, len, path, line, docindex);
  }
}

void swap_root(TrieNode **root, char ch){
  TrieNode* newroot = malloc(sizeof(TrieNode));
  init_trie(newroot, ch);
  newroot->next = *root;
  *root = newroot;
}

void delete_trie(TrieNode *node){
  if(node->down!=NULL){
    delete_trie(node->down);
    free(node->down);
  }
  if(node->next!=NULL){
    delete_trie(node->next);
    free(node->next);
  }
  if(node->list!=NULL){
    delete_list(node->list);
    free(node->list);
  }
}

TrieNode* makeTrie(Registry *documents, int docsize){
  TrieNode * trie = malloc(sizeof(TrieNode));
  char *word = NULL, *subs = NULL, ch= ' ';
  size_t len = 0, j=0;
  printf("process:%d docsize: %d\n", getpid(), docsize);
  init_trie(trie, documents[0].text[0][0]);
  for(int j=0; j<docsize; j++){
    for(int i=0; i<documents[j].lines; i++){
      ch = ' ';
      subs = documents[j].text[i];
      while(isspace(subs[0]))   //set substring to new word
        subs++;
      while(ch!= '\0'){   //start breaking each document in words
        len=0;
        //count the length of each word
        while((ch = subs[len]) != ' ' && ch !='\t' && ch != '\0')
          len++;
        //copy the word in variable
        word = subs;
        if(trie->letter > word[0])     //swap root to maintain order (& law)
          swap_root(&trie, word[0]);
        //insert to trie
        insert_in_trie(trie, word, 0, len, documents[j].path, i, j);
        subs += len;
        if(ch != '\0')           //if it's the end of the document don't search
          while(isspace(ch=subs[0]))   //set substring to new word
            subs++;
      }
    }
  }
  return trie;
}

TrieNode* find_word(TrieNode *node, char* word){
  TrieNode *current = node;
  size_t pos = 0, len = strlen(word);
  while(current!=NULL && pos!=len){   //start traversing the trie
    if(word[pos]==current->letter){
      pos++;
      if(pos!=len)  //word has not ended yet
        current = current->down;
    }
    else
      current = current->next;
  }
  if(current == NULL || current->list == NULL)       //the word is not in the trie
    return NULL;
  return current;
}

void maxcount(TrieNode *node, char *word, char **doc, int *no_appear){
  TrieNode *wordnode = find_word(node, word);
  if(wordnode == NULL || wordnode->list == NULL){
    *doc = NULL;
    *no_appear = -1;
  }
  else{
    find_maxcount(wordnode->list, doc, no_appear);
  }
}

void mincount(TrieNode *node, char *word, char **doc, int *no_appear){
  TrieNode *wordnode = find_word(node, word);
  if(wordnode == NULL || wordnode->list == NULL){
    *doc = NULL;
    *no_appear = -1;
  }
  else{
    find_mincount(wordnode->list, doc, no_appear);
  }
}
