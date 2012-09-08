#include <stdio.h>
#include <stdarg.h>
#include <mpi.h>
#include "parallel.h"

double starttime;
/* external globals */
int mpi_size, mpi_rank;

/* this is all the MPI setup we want to do to make sure things run smoothly */
void parallelize(int *argc, char ***argv)
{
    int ret;
    if((ret = MPI_Init(argc, argv)) != MPI_SUCCESS) {
        printf("fatal error: MPI could not initialize - terminating\n");
        MPI_Abort(MPI_COMM_WORLD, ret);
    }

    starttime = MPI_Wtime();

    /* OpenMPI's shitty man pages inform me that on error MPI will abort for
     * us, so I won't do any error checking here (why is it so difficult to
     * tell me what your API's functions return? fucking useless
     * documentation...) */
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
}

/* this is just for consistency and to avoid MPI bullshit in main()/main.c */
void deparallelize(void)
{
    MPI_Finalize();
}

void pexit(int errcode)
{
    MPI_Abort(MPI_COMM_WORLD, errcode);
}

int outoftime(int walltime)
{
    return (MPI_Wtime()-starttime > walltime-WALLTIME_CLEANUP);
}

/* parallel printf: same as regular printf but also stamp it with the process'
 * ID to make standard output clearer */
int pprintf(char *format, ...)
{
    int ret;
    va_list args;

    va_start(args, format);

    printf("%d-%d:", mpi_size, mpi_rank);
    ret = vprintf(format, args);

    va_end(args);
    return ret;
}

