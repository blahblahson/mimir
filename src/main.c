#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "set.h"
#include "list.h"
#include "formula.h"
#include "rule.h"
#include "logic.h"

void printformula(int *formula)
{
    int j;
    int n = length(formula);
    for(j = 0; j <= n; j++) {
        switch(formula[j]) {
            case OP_AND:
                printf("AND(");
                break;
            case OP_OR:
                printf("OR(");
                break;
            case OP_CLOSE:
                printf(")");
                break;
            case OP_FIN:
                printf(".");
                break;
            default:
                printf("%d,", formula[j]);
        }
    }
    printf("\n");
}

/* generate ALL balanced formulae of n variables */
int **genbf(int n)
{
    int s[n], m[n]; s[0] = -1;
    int i, j, k, l; /* we do a lot of counting... */
    int **ret = NULL;
    int n_ret = 0;
    int partsize;
    int part[2*n], delim[2*n];

    while((partsize = partition(n, s, m))) {
        /* part = A1,.,A2,.,A3,.,A4,.,...,Apartsize,.
         * Ak = atoms in subset k */
        memset(delim, 0, 2*n*sizeof(int));

        for(i = 0, k = 0; i < partsize; i++) {
            for(j = 0; j < n; j++) {
                if(s[j] == i+1) {
                    part[k++] = j; /* important here: this means we generally
                                        don't use 0 as a variable, even though
                                        it is a valid one */
                }
            }
            part[k++] = OP_FIN;
            delim[i+1] = k;
        }

        /* for every partition, add an AND/OR and a ), plus the n variables,
         * and then the final . */
        int n_formula = partsize*2+n+1;

        /* now we collect all permutations of (1,...,partsize) */
        int **perm = NULL;
        int *pv = NULL;
        permute(partsize, 0, &perm, &pv);
        free(pv); /* TODO: THIS PERMUTATION FUNCTION SUCKS, I HATE IT */

        int n_perms = length_l((void **)perm); /* this will be n! */
        for(i = 0; i < n_perms; i++) {
            /* since we end up with ...,ANDOR(A1),... where A1 is the
             * perm[i][0]th partition, if said partition contains only one atom
             * then we discount it because it is a redundant form of another
             * formula generated by a different partition earlier or later on
             */
            if(length(part+delim[perm[i][0]]) < 2) continue;

            enum OPERATOR op = OP_OR;

            /* make the OR,AND,OR,... */
            int *formula1 = malloc(n_formula*sizeof(int));
            int *formula2 = malloc(n_formula*sizeof(int));

            for(l = 0; l < partsize; l++) {
                formula1[l] = op;
                op = (op == OP_AND) ? OP_OR : OP_AND; /* alternate! */
                formula2[l] = op;
            }

            for(j = 0; j < partsize; j++) {
                /* push part[delim[perm[i][j]]+k] up to OP_FIN */
                for(k = 0; part[delim[perm[i][j]]+k] != OP_FIN; k++, l++)
                    formula1[l] = formula2[l] = part[delim[perm[i][j]]+k];
                /* push ) */
                formula1[l] = formula2[l] = OP_CLOSE;
                l++;
            }

            formula1[l] = formula2[l] = OP_FIN; /* push . */

            n_ret += 2;
            ret = realloc(ret, (n_ret+1)*sizeof(int *));
            ret[n_ret-2] = formula1;
            ret[n_ret-1] = formula2;
            ret[n_ret] = NULL;
        }

        free_l((void **)perm);
    }

    return ret;
}


int **exhaust(int *formula, int **collection)
{
    int **m = r_mix(formula);
    int n_m = length_l((void **)m);
    if(n_m <= 0) {
        free_l((void **)m); // may be redundant
        return collection;
    }

    int i;
    for(i = 0; i < n_m; i++) collection = exhaust(m[i], collection);

    collection = push_uniq(collection, m);

    return collection;
}

int **genbf2(int n)
{
    int conj[n+3], i;
    conj[0] = OP_AND;
    for(i = 0; i < n; i++) conj[i+1] = i;
    conj[n+1] = OP_CLOSE;
    conj[n+2] = OP_FIN;

    return exhaust(conj, NULL);
}

int main(int argc, char *argv[])
{
    int **bfs = genbf2(7);
    int i, j;
    int n_bfs = length_l((void **)bfs);
    int totalinf = 0;
    int totaltriv = 0;

    FILE *fh = fopen("./bfs.7", "w+");
    for(i = 0; i < n_bfs; i++) {
        printf("writing %d/%d\n", i+1, n_bfs);
        fwrite(bfs[i], sizeof(int), length(bfs[i])+1, fh);
    }

    fclose(fh);
    exit(0);
    //for(i = 0; i < n_bfs; i++) printformula(bfs[i]);
    for(i = 0; i < n_bfs; i++) {
        printformula(bfs[i]);
        for(j = 0; j < n_bfs; j++) {
            if(implies(bfs[i], bfs[j])) {
                totalinf++;
//                printformula(bfs[j]);
                int triv = trivial(bfs[i], bfs[j]);


                if(triv) {
                    /*printformula(bfs[i]);
                    printf("-------------\n");
                    printformula(bfs[j]);*/
                    totaltriv++;
                    /*printformula(bfs[i]);
                    printformula(bfs[j]);*/
                    printf("[PASS(trivial)](%d) %d -> %d\n", n_bfs, i+1, j+1);
                    continue;
                }
                int f2vi = validinputs(bfs[j], NULL);
                if(quickprove(bfs[i],bfs[j],f2vi)) {
                    printf("[PASS(exhaust)](%d) %d -> %d\n", n_bfs, i+1, j+1);
                }
                else {
                    int triv = trivial(bfs[i], bfs[j]);
                    if(!triv) {
                        printf("[FAIL] %d -> %d\n", i, j);
                        printf("NOT provable!\nA=");
                        printformula(bfs[i]);
                        printf("B=");
                        printformula(bfs[j]);

                        //quickprove(bfs[i], bfs[j], f2vi);
                        //r_medial(bfs[i]);
                        //printf("and trivial? %d\n", triv);
                        exit(0);
                    }
                    /*else {
                        printf("trivial:\n");
                        printformula(bfs[i]);
                        printformula(bfs[j]);
                        printf("_________\n");
                    }*/
                }
                //printf("%d %s %d\n", i, prove(bfs[i],bfs[j])?"proves":"does not prove", j);
            }
        }
    }
    printf("all inferences have proofs under {mix,switch,medial} and triviality\n"
            "total inferences checked: %d\n"
            "of which trivial: %d\n", totalinf, totaltriv);
    //printf("for the record: we had %d bfs\n", n_bfs);
    /*printf("%d\n", find(bfs, n_bfs, m1));
    printformula(m1);
    for(i = 0; i < n_bfs; i++) printformula(bfs[i]);*/
    free_l((void **)bfs);

    return 0;
}
