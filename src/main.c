#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "util.h"
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
int **genbfold(int n)
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

void genbf_i(int n, int m, int *maxima, int *choices, int *choice, int j)
{
    static int i;

    if(j == n) {
        //choices[i] = malloc(n*sizeof(int));
        memcpy(&choices[i*n], choice, n*sizeof(int));
        i++;
        return;
    }
    else if(j < 0) { /* initial call */
        /* initialize choice 2d array */
        /*int m = n;
        for(i = 0; i < n; i++) m *= maxima[i];*/

        /* and counting variables... */
        i = 0;
        j = 0;
    }
    else if(i == m) return; // TODO: I think this is unnecessary


    int k;
    for(k = 0; k < maxima[j]; k++) {
        choice[j] = k;
        genbf_i(n, m, maxima, choices, choice, j+1);
    }

    return;
}

int **genbf_(int *atoms, int n, enum OPERATOR op, int *n_ret)
{
    /* base case, which is probably horribly inefficient... maybe I could cut
     * down on so much dynamic allocation, but we'll see... */
    if(n == 1) {
        int **ret = malloc(2*sizeof(int *));
        ret[0] = malloc(2);
        // TODO: again, better way to do this?
        ret[0][0] = atoms[0]; ret[0][1] = OP_FIN;
        ret[1] = NULL;
        *n_ret = 1;

        return ret;
    }

    int i, j, k;
    int **ret = NULL;
    *n_ret = 0;
    enum OPERATOR op2 = op == OP_OR ? OP_AND : OP_OR;

    int partsize, s[n], m[n]; s[0] = -1;
    while((partsize = partition(n, s, m))) {
        /* this causes an infinite loop, and the case of the operator acting
         * directly on the passed atoms is covered instead by the case when
         * partsize is n instead */
        if(partsize == 1) continue;

        int **subbfs[partsize];
        int maxima[partsize], atoms2[n];
        int n_choices = 1;
        for(i = 0; i < partsize; i++) {
            for(j = 0, k = 0; j < n; j++)
                if(s[j] == i+1) atoms2[k++] = atoms[j];
            subbfs[i] = genbf_(atoms2, k, op2, &maxima[i]);
            //maxima[i] = length_l(subbfs[i]);
            n_choices *= maxima[i];
        }

        /* a 2d array would be nice but C sucks at such things when passing
         * between functions and I don't really want to malloc all that shit
         * and then have to free it */
        int choices[n_choices*partsize];
        int choice[partsize];
        genbf_i(partsize, n_choices, maxima, choices, choice, -1);


        struct range_t t; t.top = 0;
        for(i = 0; i < n_choices; i++) {
            int *result = malloc(3*sizeof(int));
            /* TODO: neater way to do this? */
            result[0] = op; result[1] = OP_CLOSE; result[2] = OP_FIN;

            /* we put them in reverse to preserve lexicographical order, since
             * shove() is a quite primitive */
            for(j = partsize-1; j >= 0; j--)
                shove(result, &t, subbfs[j][choices[i*partsize+j]]);

            /* now shove it onto our list to return */
            ret = realloc(ret, (++(*n_ret)+1)*sizeof(int *));
            ret[(*n_ret)-1] = result;
            ret[*n_ret] = NULL;

            /* and now we're done with coice i, so free it */
            //free(choices[i]);
        }

        /* free up the rest of the stuff... */
        //for(i = 0; i < partsize; i++) free(subbfs[i]);
    }

    return ret;
}

int **genbf(int n)
{
    int i, j;
    int atoms[n];
    for(i = 0; i < n; i++) atoms[i] = i;

    int n_ands, n_ors;
    int **ands = genbf_(atoms, n, OP_AND, &n_ands);
    int **ors = genbf_(atoms, n, OP_OR, &n_ors);

    int **ret = malloc((n_ands+n_ors+1)*sizeof(int *));

    /* this looks stupid but this way we alternate AND and OR thingies, which I
     * think is probably more sensical... code to just shove in all the ANDs
     * and then all the ORs is commented out after this stuff though */
    for(i = 0, j = 0; i < min(n_ands, n_ors); i++) {
        ret[j++] = ands[i];
        ret[j++] = ors[i];
    }

    if(n_ands > n_ors)
        for( ; i < n_ands; i++) ret[j++] = ands[i];
    else if(n_ors > n_ands)
        for( ; i < n_ors; i++) ret[j++] = ors[i];

    /*
    for(i = 0, k = 0; i < n_ands; i++) ret[j++] = ands[i];
    for(i = 0; i < n_ors; i++) ret[j++] = ors[i];
    */

    free(ands);
    free(ors);

    return ret;
}

int main(int argc, char *argv[])
{
    int **bfs = genbf(3);
    int i, j;
    int n_bfs = length_l((void **)bfs);
    int totalinf = 0;
    int totaltriv = 0;

    //FILE *fh = fopen("./bfs.7", "w+");
    for(i = 0; i < n_bfs; i++) {
        printf("writing %d/%d\n", i+1, n_bfs);
        //fwrite(bfs[i], sizeof(int), length(bfs[i])+1, fh);
    }

    //fclose(fh);
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
