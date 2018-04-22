#ifndef REGISTRY_H
#define REGISTRY_H

typedef struct registry{
  char **text;  //text in lines
  char *path; //path of the file that the text is from
  int lines;  //number of lines
}Registry;

#endif
