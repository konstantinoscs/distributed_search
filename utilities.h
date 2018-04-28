#ifndef UTILITIES_H
#define UTILITIES_H

#include <sys/types.h>

int parse_arguments(int argc, char ** argv, char** docfile, int *num_workers);
int parse_docfile(char* doc, char*** paths, int *pathsize);
int make_fifo_arrays(char ***job_to_w, char***w_to_job, int num_workers);
int readQueries(char ***queries, int *queriesNo);
void deleteQueries(char ***queries, int queriesNo);
int find_width(int number);

int free_worker(pid_t *child, char *docfile, int num_workers, char **job_to_w,
  char**w_to_job);
int free_executor(pid_t *child, char *docfile, int pathsize, char **paths,
  int num_workers, char **job_to_w, char**w_to_job, int *fifo_in, int *fifo_out);


#endif
