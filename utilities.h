#ifndef UTILITIES_H
#define UTILITIES_H

int parse_arguments(int argc, char ** argv, char** docfile, int *num_workers);
int parse_docfile(char* doc, char*** paths, int *pathsize);
int make_fifo_arrays(char ***job_to_w, char***w_to_job, int num_workers);

#endif
