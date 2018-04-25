#ifndef WORKER_H
#define WORKER_H

int worker_operate(char *job_to_w, char *w_to_job, int openfifo, int fifo_in,
  int fifo_out);

#endif
