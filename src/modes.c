#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "parallel.h"
#include "formula.h"
#include "version.h"

#define READ_BLOCKSIZE 1024*1024

/* exit variable... TODO: put in handler.c/h? */
extern int letsblowthisjoint;

int checkpoint(int status, int i, int j, int start, int stop)
{
    char checkpointfile[100]; /* who cares about 100 bytes? */
    snprintf(checkpointfile, 100, "mim.%d-%d.checkpoint", mpi_size, mpi_rank);

    pprintf("checkpointing {%d,%d,%d,%d,%d} to file %s\n", status, i, j, start,
            stop, checkpointfile);

    FILE *fh = fopen(checkpointfile, "w+");
    if(fh == NULL) {
        pprintf("error: failed to open checkpoint file (%s)\n",
                strerror(errno));
        return 1;
    }

    int cp[5] = {status, i, j, start, stop};
    if(fwrite(cp, sizeof(int), 5, fh) != 5) {
        pprintf("error: failed to write to file (%s)\n", strerror(errno));
        if(fclose(fh))
            pprintf("error: failed to close file (%s)\n", strerror(errno));
        return 1;
    }

    if(fclose(fh)) {
        pprintf("error: failed to close file (%s)\n", strerror(errno));
        return 1;
    }

    return 0;
}

/* MODE A
 * generate all balanced formulae involving nvars variables, dump it to the
 * file mim<nvars>.bfs and then exit
 */
int mode_a(int nvars)
{
    /* just make clear where we are */
    pprintf("mimir operation mode A\n");

    char dumpfile[100];
    snprintf(dumpfile, 10, "mim%d.bfs", nvars);

    pprintf("generating all balanced formulae of %d variables\n"
            "output file: %s\n", nvars, dumpfile);

    /* generate the balanced formulae */
    int **bfs = genbf(nvars);
    int n_bfs = length_l((void **)bfs);

    pprintf("complete - %d balanced formulae generated. writing to file\n",
            n_bfs);

    FILE *fh = fopen(dumpfile, "w+");
    if(fh == NULL) {
        pprintf("error: failed to open output file (%s)\n", strerror(errno));
        free_l((void **)bfs);
        return 1;
    }

    int i;
    for(i = 0; i < n_bfs; i++) {
        int writesize = length(bfs[i])+1;

        /* just write every BF to file, simple! */
        if(fwrite(bfs[i], sizeof(int), writesize, fh) != writesize) {
            pprintf("error: failed to write to file (%s)\n", strerror(errno));
            if(fclose(fh))
                pprintf("error: failed to close file (%s)\n", strerror(errno));
            free_l((void **)bfs);
            return 1;
        }
    }

    free_l((void **)bfs);

    pprintf("complete - %d balanced formulae successfully written\n", n_bfs);

    if(fclose(fh)) {
        pprintf("error: failed to close file (%s)\n", strerror(errno));
        return 1;
    }

    return 0;
}

int mode_b(char *bffile, int start, int stop, int walltime, int ii, int jj)
{
    pprintf("mimir operation mode B\n");

    FILE *fh = fopen(bffile, "r");
    if(fh == NULL) {
        pprintf("error: failed to open input file (%s)\n", strerror(errno));
        return 1;
    }

    int n_bfs = stop-start+1; /* number of BFs we're dealing with */
    int *bfs[n_bfs]; /* BFs to check, the B in A->B */
    int **bfs_lo = NULL; /* lexicographically ordered BFs, the A in A->B */
    int complete = 0; /* loop control */
    int j = 0; /* number of formulae we have read from file, i.e. index of
                  current formula */
    int k = 0; /* number of formulae dumped into bfs, i.e. index of next */
    int n_bfs_lo = 0; /* likewise for bfs_lo */
    int bufpos = 0; /* where we scan into buf from */
    int i; /* counter for various crap */
    int n_checked = 0; /* number of sound inferences checked */
    int n_trivial = 0; /* number of which are trivial */

    pprintf("loading balanaced formulae %d~%d (inclusive) and all "
            "lexicographically ordered balanced formulae from input file "
            "%s\n", start, stop, bffile);

    while(!complete) {
        int buf[READ_BLOCKSIZE];
        int read = fread(buf+bufpos, sizeof(int), READ_BLOCKSIZE-bufpos, fh);

        if(feof(fh)) { /* end of file, so things are actually OK */
            buf[read+bufpos] = OP_FIN; /* double -1 implies that we are done */
        }
        else if(read != READ_BLOCKSIZE-bufpos) { /* error */
            pprintf("error: failed to read file (%s)\n", strerror(errno));
            if(fclose(fh))
                pprintf("error: failed to close file (%s)\n", strerror(errno));
            return 1;
        }

        bufpos = 0; /* next read we assume we can fill the whole of buf again
                       unless we find otherwise (which is probably the case but
                       not necessarily) */

        int i, n_f;
        for(i = 0, n_f = 0; i < READ_BLOCKSIZE; i += n_f+1, j++) {
            if(buf[i] == OP_FIN) {
                complete = 1;
                break;
            }

            /* here we find the length of the formula */
            for(n_f = 0; i+n_f < READ_BLOCKSIZE; n_f++)
                if(buf[i+n_f] == OP_FIN) break;

            /* in this scenario we've reached the limit of the buffer but have
             * an incomplete formula, so we have to make sure we get the rest
             * and don't miss this guy! */
            if(i+n_f == READ_BLOCKSIZE) {
                /* shift it back to the front of the buffer */
                memmove(buf, buf+i, (n_f-1)*sizeof(int));
                bufpos = n_f-1; /* at next point dump into buf from here */
                break;
            }

            if(j >= start && j <= stop) { /* needs adding for checking */
                bfs[k] = malloc((n_f+1)*sizeof(int));
                memcpy(bfs[k], &buf[i], (n_f+1)*sizeof(int));
                k++;
            }

            /* I don't like how much dynamic allocation we have to do here so
             * I'd like to find a formula for the number of lexicographically
             * ordered BFs, i.e. the number of unique "strcutures"... for now
             * this'll do though */
            if(lexorder(&buf[i])) {
                bfs_lo = realloc(bfs_lo, (++n_bfs_lo)*sizeof(int *));
                bfs_lo[n_bfs_lo-1] = malloc((n_f+1)*sizeof(int));
                memcpy(bfs_lo[n_bfs_lo-1], &buf[i], (n_f+1)*sizeof(int));
            }

            //i += n_f+1; /* onto the next formula */
            //j++; /* we've read one more balanced formula */
        }
    }

    /* we didn't read the right number of balanced formulae */
    if(k != n_bfs) {
        pprintf("error: start-stop range invalid for input file\n");
        for(i = 0; i < k; i++) free(bfs[i]);
        return 1;
    }

    pprintf("complete - %d+%d balanced formulae loaded\n"
            "checking provability of all possible inferences\n", k,
            n_bfs_lo);

    /* ok, finally we've read them */

    for(i = ii; i < n_bfs; i++) { /* for each B in A->B */
        for(j = jj; j < n_bfs_lo; j++) { /* for each A in A->B */
            /* if we're out of time or get a SIGINT */
            if(outoftime(walltime) || letsblowthisjoint) {
                if(letsblowthisjoint) pprintf("wall time exhausted\n");
                else pprintf("received SIGINT\n");

                /* checkpoint, clean up, exit! */
                int ret = checkpoint(0, i, j, start, stop);
                free_l((void **)bfs_lo);
                for(i = 0; i < n_bfs; i++) free(bfs[i]);
                pprintf("exiting\n");
                return ret;
            }

            /* else, FINALLY, we get to the actual logic */
            if(implies(bfs_lo[j], bfs[i])) { /* sound inference, check! */
                n_checked++;

                if(trivial(bfs_lo[j], bfs[i])) { /* trivial, so move on */
                    n_trivial++;
                    continue;
                }
                
                int f2vi = validinputs(bfs[i], NULL);
                if(quickprove2(bfs_lo[j], bfs[i], f2vi)) {
                    /* provable, boring... NEXT! */
                    continue;
                }
                else { /* oh shit new inference! */
                    /* I think there's a possibility these could get broken up
                     * in stdout with many parallel things, but probably only
                     * in theory... no big deal anyway */
                    pprintf("A:");
                    printformula(bfs_lo[j]);
                    pprintf("B:");
                    printformula(bfs[i]);
                    pprintf("new inference found! (see above)\n");

                    int ret = checkpoint(1, i, j, start, stop);

                    free_l((void **)bfs_lo);
                    for(i = 0; i < n_bfs; i++) free(bfs[i]);

                    pprintf("terminating siblings\n");
                    /* now we are ready to exit cleanly, BUT we also want
                     * the other parallel processes to exit as well because
                     * we have the information we want. we use MPI for
                     * this... */
                    pexit(0);
                    /* this'll send a SIGINT to all other instances in this
                     * computation and they will catch it and subsequently
                     * checkpoint as soon as possible */

                    pprintf("exiting\n");

                    return 0;
                }
            }
        }
    }

    pprintf("complete - no new linear inferences (checked %d sound "
            "inferences, of which %d trivial)\n", n_checked, n_trivial);
    checkpoint(0, i, j, start, stop);

    free_l((void **)bfs_lo);
    for(i = 0; i < n_bfs; i++) free(bfs[i]);

    return 0;
}

int mode_c(char *bffile, int walltime)
{
    char checkpointfile[100];
    snprintf(checkpointfile, 100, "mim.%d-%d.checkpoint", mpi_size,
            mpi_rank);

    FILE *fh = fopen(checkpointfile, "r");
    if(fh == NULL) {
        pprintf("error: failed to open checkpoint file (%s)\n",
                strerror(errno));
        return 1;
    }

    int checkpoint[5];
    if(fread(checkpoint, sizeof(int), 5, fh) != 5) {
        pprintf("error: failed to read file (%s)\n", strerror(errno));
        if(fclose(fh))
            pprintf("error: failed to close file (%s)\n", strerror(errno));
        return 1;
    }

    if(fclose(fh)) {
        pprintf("error: failed to close file (%s)\n", strerror(errno));
        return 1;
    }

    if(checkpoint[0] == 1) {
        pprintf("notice: checkpoint refers to a finished task\n");
        return 0;
    }

    pprintf("checkpoint loaded, dropping to mode B\n");
    return mode_b(bffile, checkpoint[3], checkpoint[4], walltime,
            checkpoint[1], checkpoint[2]);
}

int mode_v(void)
{
    pprintf("%s\n%s", MIMIR_VERSION, MIMIR_COPYING);
    return 0;
}
