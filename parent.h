#ifndef PARENT_H
#define PARENT_H

void parent_maxcount(int queriesNo, int num_workers, int *fifo_in, int *fifo_out);
void parent_mincount(int queriesNo, int num_workers, int *fifo_in, int *fifo_out);

int parent_operate(int num_workers, pid_t *child, char *docfile, char **job_to_w,
  char **w_to_job);

#endif
