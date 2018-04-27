#ifndef PLIST_H
#define PLIST_H

typedef struct DocInfo{
  char *doc;          //document path
  int docindex;       //document index
  int *appearance;    //line of appreance
  int no_lines;
  int no_appear;    //no of appearances in total
  struct DocInfo *next;
}DocInfo;

typedef struct Plist{
  DocInfo *info;    //info list
  DocInfo *last;         //last to speed up updates
}Plist;

void delete_doc(DocInfo *frequency);
void search_n_update(Plist *list, char *path, int line, int docindex);
void delete_list(Plist *list);
void find_maxcount(Plist *list, char **doc, int *no_appear);
void find_mincount(Plist *list, char **doc, int *no_appear);

#endif
