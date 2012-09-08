#ifndef _PARALLEL_H_
#define _PARALLEL_H_

/* wall time is specified in seconds. default: 12 hours */
#define WALLTIME_DEFAULT 43200
/* clean-up time, i.e. time we give ourselves to exit before the scheduler
 * terminates the program */
#define WALLTIME_CLEANUP 60

extern int mpi_size, mpi_rank;

void parallelize(int *, char ***);
void deparalellize(void);
void pexit(int);

int outoftime(int);

int pprintf(char *, ...);

#endif /* _PARALLEL_H_ */
