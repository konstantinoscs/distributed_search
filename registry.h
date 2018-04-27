#ifndef REGISTRY_H
#define REGISTRY_H

typedef struct registry{
  char **text;  //text in lines
  char *path; //path of the file that the text is from
  int lines;  //number of lines
}Registry;

typedef struct Result{
  char *doc;
  int *line_no;
  char **lines;
  int size;
}Result;

#endif
