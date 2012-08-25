#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "set.h"
#include "list.h"
#include "formula.h"
#include "rule.h"
// TODO: get rid of stdio when done with printf debugging

int r_medial2(int *formula, int *f2)
{
    // for each OR (1)
    // for each pair (2,3) of ANDs in (1)'s arguments
    // for each of (2)'s partitions of 2
    // for each of (3)'s partitions of 2
    // splice all that shit together, i.e.:
    //      P,OR(X,AND(A,B),Y,AND(C,D),Z),Q
    //  ->  P,AND(X,OR(A,C),Y,OR(B,D),Z),Q

    int n_formula = length(formula);
    int i, j, k;
    //int **ret = NULL;
    //int n_ret = 0;

    for(i = 0; i < n_formula; i++) {
        if(formula[i] != OP_OR) continue;
        struct range_t *or = range(formula, i);
        //or.top = i;
        //or.bot = scope(formula, i); // TODO: make sure this is an ok definition of scope

        /* don't let the name args_or mislead you too much... this is the
         * arguments of the OR that are ANDs themselves ONLY!! */
        struct range_t **args_or = arguments_op(formula, or, OP_AND);
        /* this shouldn't happen unless the formula is fucked */
        if(args_or == NULL) {
            free(or);
            continue;
        }

        // TODO: unify definition of a list of pointers ending, atm length
        // counts -1 as the end, args_or counts NULL...
        int n_args_or = length_l((void **)args_or);
        int *p = NULL;
        
        /* for each pair (2,3) of ANDs in (1)'s arguments */
        while((p = pair(n_args_or, p)) != NULL) {
            /* args_or[p[0]] = first AND index in formula
             * args_or[p[1]] = second AND index
             */

            struct range_t and1, and2;
            memcpy(&and1, args_or[p[0]], sizeof(struct range_t));
            memcpy(&and2, args_or[p[1]], sizeof(struct range_t));
//            and1.top = p[0]; and1.bot = scope(formula, args_or[p[0]]);
//            and2.top = p[1]; and2.bot = scope(formula, args_or[p[1]]);
            struct range_t **args_and1 = arguments(formula, &and1);
            struct range_t **args_and2 = arguments(formula, &and2);
            int n_args_and1 = length_l((void **)args_and1);
            int n_args_and2 = length_l((void **)args_and2);

            int s1[n_args_and1], m1[n_args_and1]; s1[0] = -1;
            int partsize1;
            while((partsize1 = partition(n_args_and1, s1, m1))) {
                /* we're only want partitioning into two disjoint sets */
                if(partsize1 <= 0) break;
                if(partsize1 != 2) continue;

                int s2[n_args_and2], m2[n_args_and2]; s2[0] = -1;
                int partsize2;
                while((partsize2 = partition(n_args_and2, s2, m2))) {
                    if(partsize2 != 2) continue;

                    /* s1[j]==0 => A    s1[j]==1 => B
                     * s2[j]==0 => C    s2[j]==1 => D
                     */

                    /* now we're ready to splice! */
                    struct range_t *arglist[n_args_and1+n_args_and2+4];
                    int delim[4] = {0,0,0,0};

                    /* A */
                    for(j = 0, k = 0; j < n_args_and1; j++)
                        if(s1[j] == 1) arglist[k++] = args_and1[j];
                    arglist[k++] = NULL; delim[1] = k;

                    /* C */
                    for(j = 0; j < n_args_and2; j++)
                        if(s2[j] == 1) arglist[k++] = args_and2[j];
                    arglist[k++] = NULL; delim[2] = k;

                    /* B */
                    for(j = 0; j < n_args_and1; j++)
                        if(s1[j] == 2) arglist[k++] = args_and1[j];
                    arglist[k++] = NULL; delim[3] = k;

                    /* D */
                    for(j = 0; j < n_args_and2; j++)
                        if(s2[j] == 2) arglist[k++] = args_and2[j];
                    arglist[k] = NULL;

                    /* so we have arglist = A,\0,C,\0,B,\0,D,\0 */
                    /*ret = splice(OP_AND, 2,
                            splice(OP_OR, 2,
                                splice_l(formula, OP_AND, arglist+delim[0]),
                                splice_l(formula, OP_AND, arglist+delim[1])),
                            splice(OP_OR, 2,
                                splice_l(formula, OP_AND, arglist+delim[2]),
                                splice_l(formula, OP_AND, arglist+delim[3])));*/

                    int *result = malloc((n_formula+1)*sizeof(int));
                    memcpy(result, formula, (n_formula+1)*sizeof(int));

                    /* the order here is important, otherwise we would have to
                     * compensate (using the return value of the first yank)
                     * to redefine the position of arg2
                     */
                    result = yank(result, &and2);
                    result = yank(result, &and1);
                    result = shove(result, or,
                            splice(OP_AND, 2,
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[0]),  /* A */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[1])), /* C */
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[2]),  /* B */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[3]))));/*D */
                    // TODO: sanitization!!!!

                    result = sanitize(result, NULL);

                    if(implies(result, f2)) {
                        for(j = 0; j < n_args_and1; j++) free(args_and1[j]);
                        free(args_and1);
                        for(j = 0; j < n_args_and2; j++) free(args_and2[j]);
                        free(args_and2);

                        free(or);
                        for(j = 0; j < n_args_or; j++) free(args_or[j]);
                        free(args_or);

                        free(result);

                        //free_l((void **)ret);
                        return 1;
                    }

                    //result = malloc((n_formula+1)*sizeof(int));
                    memcpy(result, formula, (n_formula+1)*sizeof(int));

                    /* the order here is important, otherwise we would have to
                     * compensate (using the return value of the first yank)
                     * to redefine the position of arg2
                     */
                    result = yank(result, &and2);
                    result = yank(result, &and1);
                    result = shove(result, or,
                            splice(OP_AND, 2,
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[0]),  /* A */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[3])), /* D */
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[2]),  /* B */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[1]))));/*C */
                    // TODO: sanitization!!!!

                    result = sanitize(result, NULL);

                    if(implies(result, f2)) {
                        for(j = 0; j < n_args_and1; j++) free(args_and1[j]);
                        free(args_and1);
                        for(j = 0; j < n_args_and2; j++) free(args_and2[j]);
                        free(args_and2);

                        free(or);
                        for(j = 0; j < n_args_or; j++) free(args_or[j]);
                        free(args_or);

                        free(result);

                        //free_l((void **)ret);
                        return 1;
                    }
                }
            }

            /* TODO: put this in a function or some shit... maybe C++ it up
             * into a class eventually? */
            for(j = 0; j < n_args_and1; j++) free(args_and1[j]);
            free(args_and1);
            for(j = 0; j < n_args_and2; j++) free(args_and2[j]);
            free(args_and2);
        }

        free(or);

        for(j = 0; j < n_args_or; j++) free(args_or[j]);
        free(args_or);
    }

    return 0;
}
int **r_medial(int *formula)
{
    // for each OR (1)
    // for each pair (2,3) of ANDs in (1)'s arguments
    // for each of (2)'s partitions of 2
    // for each of (3)'s partitions of 2
    // splice all that shit together, i.e.:
    //      P,OR(X,AND(A,B),Y,AND(C,D),Z),Q
    //  ->  P,AND(X,OR(A,C),Y,OR(B,D),Z),Q

    int n_formula = length(formula);
    int i, j, k;
    int **ret = NULL;
    int n_ret = 0;

    for(i = 0; i < n_formula; i++) {
        if(formula[i] != OP_OR) continue;
        struct range_t *or = range(formula, i);
        //or.top = i;
        //or.bot = scope(formula, i); // TODO: make sure this is an ok definition of scope

        /* don't let the name args_or mislead you too much... this is the
         * arguments of the OR that are ANDs themselves ONLY!! */
        struct range_t **args_or = arguments_op(formula, or, OP_AND);
        /* this shouldn't happen unless the formula is fucked */
        if(args_or == NULL) {
            free(or);
            continue;
        }

        // TODO: unify definition of a list of pointers ending, atm length
        // counts -1 as the end, args_or counts NULL...
        int n_args_or = length_l((void **)args_or);
        int *p = NULL;
        
        /* for each pair (2,3) of ANDs in (1)'s arguments */
        while((p = pair(n_args_or, p)) != NULL) {
            /* args_or[p[0]] = first AND index in formula
             * args_or[p[1]] = second AND index
             */

            struct range_t and1, and2;
            memcpy(&and1, args_or[p[0]], sizeof(struct range_t));
            memcpy(&and2, args_or[p[1]], sizeof(struct range_t));
//            and1.top = p[0]; and1.bot = scope(formula, args_or[p[0]]);
//            and2.top = p[1]; and2.bot = scope(formula, args_or[p[1]]);
            struct range_t **args_and1 = arguments(formula, &and1);
            struct range_t **args_and2 = arguments(formula, &and2);
            int n_args_and1 = length_l((void **)args_and1);
            int n_args_and2 = length_l((void **)args_and2);

            int s1[n_args_and1], m1[n_args_and1]; s1[0] = -1;
            int partsize1;
            while((partsize1 = partition(n_args_and1, s1, m1))) {
                /* we're only want partitioning into two disjoint sets */
                if(partsize1 <= 0) break;
                if(partsize1 != 2) continue;

                int s2[n_args_and2], m2[n_args_and2]; s2[0] = -1;
                int partsize2;
                while((partsize2 = partition(n_args_and2, s2, m2))) {
                    if(partsize2 != 2) continue;

                    /* s1[j]==0 => A    s1[j]==1 => B
                     * s2[j]==0 => C    s2[j]==1 => D
                     */

                    /* now we're ready to splice! */
                    struct range_t *arglist[n_args_and1+n_args_and2+4];
                    int delim[4] = {0,0,0,0};

                    /* A */
                    for(j = 0, k = 0; j < n_args_and1; j++)
                        if(s1[j] == 1) arglist[k++] = args_and1[j];
                    arglist[k++] = NULL; delim[1] = k;

                    /* C */
                    for(j = 0; j < n_args_and2; j++)
                        if(s2[j] == 1) arglist[k++] = args_and2[j];
                    arglist[k++] = NULL; delim[2] = k;

                    /* B */
                    for(j = 0; j < n_args_and1; j++)
                        if(s1[j] == 2) arglist[k++] = args_and1[j];
                    arglist[k++] = NULL; delim[3] = k;

                    /* D */
                    for(j = 0; j < n_args_and2; j++)
                        if(s2[j] == 2) arglist[k++] = args_and2[j];
                    arglist[k] = NULL;

                    /* so we have arglist = A,\0,C,\0,B,\0,D,\0 */
                    /*ret = splice(OP_AND, 2,
                            splice(OP_OR, 2,
                                splice_l(formula, OP_AND, arglist+delim[0]),
                                splice_l(formula, OP_AND, arglist+delim[1])),
                            splice(OP_OR, 2,
                                splice_l(formula, OP_AND, arglist+delim[2]),
                                splice_l(formula, OP_AND, arglist+delim[3])));*/

                    int *result = malloc((n_formula+1)*sizeof(int));
                    memcpy(result, formula, (n_formula+1)*sizeof(int));

                    /* the order here is important, otherwise we would have to
                     * compensate (using the return value of the first yank)
                     * to redefine the position of arg2
                     */
                    result = yank(result, &and2);
                    result = yank(result, &and1);
                    result = shove(result, or,
                            splice(OP_AND, 2,
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[0]),  /* A */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[1])), /* C */
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[2]),  /* B */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[3]))));/*D */
                    // TODO: sanitization!!!!

                    result = sanitize(result, NULL);

                    ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                    ret[n_ret-1] = result;
                    ret[n_ret] = NULL;

                    result = malloc((n_formula+1)*sizeof(int));
                    memcpy(result, formula, (n_formula+1)*sizeof(int));

                    /* the order here is important, otherwise we would have to
                     * compensate (using the return value of the first yank)
                     * to redefine the position of arg2
                     */
                    result = yank(result, &and2);
                    result = yank(result, &and1);
                    result = shove(result, or,
                            splice(OP_AND, 2,
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[0]),  /* A */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[3])), /* D */
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[2]),  /* B */
                                    splice_l(OP_AND, formula,
                                        arglist+delim[1]))));/*C */
                    // TODO: sanitization!!!!

                    result = sanitize(result, NULL);

                    ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                    ret[n_ret-1] = result;
                    ret[n_ret] = NULL;
                }
            }

            /* TODO: put this in a function or some shit... maybe C++ it up
             * into a class eventually? */
            for(j = 0; j < n_args_and1; j++) free(args_and1[j]);
            free(args_and1);
            for(j = 0; j < n_args_and2; j++) free(args_and2[j]);
            free(args_and2);
        }

        free(or);

        for(j = 0; j < n_args_or; j++) free(args_or[j]);
        free(args_or);
    }

    return ret;
}

int r_switch2(int *formula, int *f2)
{
    // for each AND (1)
    // for each pair (2,3) of *,OR in (1)'s args
    // for each of (3)'s partitions of 2
    // splice:
    //      P,AND(X,A,Y,OR(B,C),Z),Q
    //  ->  P,AND(X,A,B,OR(C),Z),Q

    int n_formula = length(formula);
    int i, j, k, l; // TODO: this is too many!

    for(i = 0; i < n_formula; i++) {
        if(formula[i] != OP_AND) continue;
        struct range_t *and = range(formula, i);
        int andcorr = 1;//and->top != 0 ? 1 : 0;

        struct range_t **and_ors = arguments_op(formula, and, OP_OR);

        if(and_ors == NULL) {
            free(and);
            continue;
        }

        int n_and_ors = length_l((void **)and_ors);

        /* for each OR in (1)'s arguments */
        for(j = 0; j < n_and_ors; j++) {
            struct range_t *and_or = and_ors[j];
            struct range_t **args_and_rest = arguments_ex(formula, and, and_or);

            if(args_and_rest == NULL) continue;
            int n_args_and_rest = length_l((void *)args_and_rest);
            uint16_t counter; /* by this, we support only up to 16 args :< */
            if(n_args_and_rest > 15) {
                fprintf(stderr,
                        "ERROR: currently switch can only handle up to 15 "
                        "residual arguments; exiting.\n");
                exit(1);
            }

            struct range_t *args_and1[n_args_and_rest+1];
            struct range_t *args_and2[n_args_and_rest+1];
            int n_args_and1, n_args_and2;

            /* for each combination of the remaining arguments */
            for(counter = 1; counter < 0x1<<n_args_and_rest; counter++) {
                for(k = 0, n_args_and1 = n_args_and2 = 0;
                        k < n_args_and_rest; k++) {
                    /* this is a very inspired way of dealing with this... */
                    if((counter>>k)&0x1)
                        args_and1[n_args_and1++] = args_and_rest[k];
                    else
                        args_and2[n_args_and2++] = args_and_rest[k];
                }

                args_and1[n_args_and1] = NULL;
                args_and2[n_args_and2] = NULL;

                // make an args_and2 which is the ones we don't use, then reconstruct the whole thing-- thisll be easier!

                struct range_t **args_and_or = arguments(formula, and_or);
                if(args_and_or == NULL) continue;
                int n_args_and_or = length_l((void *)args_and_or);
                int s[n_args_and_or], m[n_args_and_or]; s[0] = -1;
                int partsize;

                while((partsize = partition(n_args_and_or, s, m))) {
                    if(partsize <= 0) break;
                    if(partsize != 2) continue;

                    struct range_t
                        *arglist[n_args_and_or+2];
                    int delim[2] = {0,0};

                    /* B */
                    for(k = 0, l = 0; k < n_args_and_or; k++)
                        if(s[k] == 1) arglist[l++] = args_and_or[k];
                    arglist[l++] = NULL; delim[1] = l;

                    /* C */
                    for(k = 0; k < n_args_and_or; k++)
                        if(s[k] == 2) arglist[l++] = args_and_or[k];
                    arglist[l++] = NULL;

                    int *result1 = malloc((n_formula+1)*sizeof(int));
                    int *result2 = malloc((n_formula+1)*sizeof(int));
                    memcpy(result1, formula, (n_formula+1)*sizeof(int));

                    /* here the principle is a bit more straightforward than in
                     * r_medial - we simply remove the entire toplevel AND
                     * chunk that we found and rebuild it out of its
                     * constituent aruments, applying the switch in the process
                     */
                    result1 = yank(result1, and);
                    memcpy(result2, result1, (length(result1)+1)*sizeof(int));
                    /*int *a1 = splice_l(OP_OR, formula, arglist+delim[0]);
                    int *a2 = splice_l(OP_OR, formula, arglist+delim[1]);
                    int *a3 = splice_l(OP_AND, formula, args_and1);
                    int *a4 = splice_l(OP_AND, formula, args_and2);

                    int *b1 = splice(OP_AND, 2, a3, a2);
                    int *b2 = splice(OP_OR, 2, a1, b1);

                    int *c1 = splice(OP_AND, 2, a4, b2);*/

                    if(andcorr) and->top--;
                    result1 = shove(result1, and, splice(OP_AND, 2,
                            splice_l(OP_AND, formula, args_and2),
                            splice(OP_OR, 2,
                                splice_l(OP_OR, formula, arglist+delim[0]),
                                splice(OP_AND, 2,
                                    splice_l(OP_AND, formula, args_and1),
                                    splice_l(OP_OR, formula,
                                        arglist+delim[1])))));
                    //result1 = shove(result1, and, c1);
                    if(andcorr) and->top++;
                    // TODO: splice is limited in that it can only take int *s,
                    // and likewise for splice_l. consider writing a splice_x
                    // that can take 
                    // TODO: sanitization? I think the splices sort this out

                    /*ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                    ret[n_ret-1] = result;
                    ret[n_ret] = NULL;*/


                    //memcpy(result2, formula, (n_formula+1)*sizeof(int));

                    //result2 = yank(result2, and);

                    if(andcorr) and->top--;
                    result2 = shove(result2, and, splice(OP_AND, 2,
                            splice_l(OP_AND, formula, args_and2),
                            splice(OP_OR, 2,
                                splice_l(OP_OR, formula, arglist+delim[1]),
                                splice(OP_AND, 2,
                                    splice_l(OP_AND, formula, args_and1),
                                    splice_l(OP_OR, formula,
                                        arglist+delim[0])))));

                    //result2 = shove(result2, and, c1);
                    if(andcorr) and->top++;

                    result1 = sanitize(result1, NULL);
                    result2 = sanitize(result2, NULL);

                    /* what the fuck is going on here? well, since the
                     * partition function works on an unordered set basis, we
                     * must take both possible combinations for each partition
                     * into two chunks */

                    if(implies(result1,f2) || implies(result2,f2)) {
                        for(k = 0; k < n_args_and_or; k++) free(args_and_or[k]);
                        free(args_and_or);

                        for(k = 0; k < n_args_and_rest; k++) free(args_and_rest[k]);
                        free(args_and_rest);

                        free(and);

                        for(k = 0; k < n_and_ors; k++) free(and_ors[k]);
                        free(and_ors);

                        free(result1);
                        free(result2);

                        return 1;
                    }
                }

                for(k = 0; k < n_args_and_or; k++) free(args_and_or[k]);
                free(args_and_or);
            }

            for(k = 0; k < n_args_and_rest; k++) free(args_and_rest[k]);
            free(args_and_rest);
        }

        free(and);

        for(k = 0; k < n_and_ors; k++) free(and_ors[k]);
        free(and_ors);
    }

    return 0;
}
int **r_switch(int *formula)
{
    // for each AND (1)
    // for each pair (2,3) of *,OR in (1)'s args
    // for each of (3)'s partitions of 2
    // splice:
    //      P,AND(X,A,Y,OR(B,C),Z),Q
    //  ->  P,AND(X,A,B,OR(C),Z),Q

    int n_formula = length(formula);
    int i, j, k, l; // TODO: this is too many!
    int **ret = NULL;
    int n_ret = 0;

    for(i = 0; i < n_formula; i++) {
        if(formula[i] != OP_AND) continue;
        struct range_t *and = range(formula, i);
        int andcorr = 1;//and->top != 0 ? 1 : 0;

        struct range_t **and_ors = arguments_op(formula, and, OP_OR);

        if(and_ors == NULL) {
            free(and);
            continue;
        }

        int n_and_ors = length_l((void **)and_ors);

        /* for each OR in (1)'s arguments */
        for(j = 0; j < n_and_ors; j++) {
            struct range_t *and_or = and_ors[j];
            struct range_t **args_and_rest = arguments_ex(formula, and, and_or);

            if(args_and_rest == NULL) continue;
            int n_args_and_rest = length_l((void *)args_and_rest);
            uint16_t counter; /* by this, we support only up to 16 args :< */
            if(n_args_and_rest > 15) {
                fprintf(stderr,
                        "ERROR: currently switch can only handle up to 15 "
                        "residual arguments; exiting.\n");
                exit(1);
            }

            struct range_t *args_and1[n_args_and_rest+1];
            struct range_t *args_and2[n_args_and_rest+1];
            int n_args_and1, n_args_and2;

            /* for each combination of the remaining arguments */
            for(counter = 1; counter < 0x1<<n_args_and_rest; counter++) {
                for(k = 0, n_args_and1 = n_args_and2 = 0;
                        k < n_args_and_rest; k++) {
                    /* this is a very inspired way of dealing with this... */
                    if((counter>>k)&0x1)
                        args_and1[n_args_and1++] = args_and_rest[k];
                    else
                        args_and2[n_args_and2++] = args_and_rest[k];
                }

                args_and1[n_args_and1] = NULL;
                args_and2[n_args_and2] = NULL;

                // make an args_and2 which is the ones we don't use, then reconstruct the whole thing-- thisll be easier!

                struct range_t **args_and_or = arguments(formula, and_or);
                if(args_and_or == NULL) continue;
                int n_args_and_or = length_l((void *)args_and_or);
                int s[n_args_and_or], m[n_args_and_or]; s[0] = -1;
                int partsize;

                while((partsize = partition(n_args_and_or, s, m))) {
                    if(partsize <= 0) break;
                    if(partsize != 2) continue;

                    struct range_t
                        *arglist[n_args_and_or+2];
                    int delim[2] = {0,0};

                    /* B */
                    for(k = 0, l = 0; k < n_args_and_or; k++)
                        if(s[k] == 1) arglist[l++] = args_and_or[k];
                    arglist[l++] = NULL; delim[1] = l;

                    /* C */
                    for(k = 0; k < n_args_and_or; k++)
                        if(s[k] == 2) arglist[l++] = args_and_or[k];
                    arglist[l++] = NULL;

                    int *result1 = malloc((n_formula+1)*sizeof(int));
                    int *result2 = malloc((n_formula+1)*sizeof(int));
                    memcpy(result1, formula, (n_formula+1)*sizeof(int));

                    /* here the principle is a bit more straightforward than in
                     * r_medial - we simply remove the entire toplevel AND
                     * chunk that we found and rebuild it out of its
                     * constituent aruments, applying the switch in the process
                     */
                    result1 = yank(result1, and);
                    memcpy(result2, result1, (length(result1)+1)*sizeof(int));
                    /*int *a1 = splice_l(OP_OR, formula, arglist+delim[0]);
                    int *a2 = splice_l(OP_OR, formula, arglist+delim[1]);
                    int *a3 = splice_l(OP_AND, formula, args_and1);
                    int *a4 = splice_l(OP_AND, formula, args_and2);

                    int *b1 = splice(OP_AND, 2, a3, a2);
                    int *b2 = splice(OP_OR, 2, a1, b1);

                    int *c1 = splice(OP_AND, 2, a4, b2);*/

                    if(andcorr) and->top--;
                    result1 = shove(result1, and, splice(OP_AND, 2,
                            splice_l(OP_AND, formula, args_and2),
                            splice(OP_OR, 2,
                                splice_l(OP_OR, formula, arglist+delim[0]),
                                splice(OP_AND, 2,
                                    splice_l(OP_AND, formula, args_and1),
                                    splice_l(OP_OR, formula,
                                        arglist+delim[1])))));
                    //result1 = shove(result1, and, c1);
                    if(andcorr) and->top++;
                    // TODO: splice is limited in that it can only take int *s,
                    // and likewise for splice_l. consider writing a splice_x
                    // that can take 
                    // TODO: sanitization? I think the splices sort this out

                    /*ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                    ret[n_ret-1] = result;
                    ret[n_ret] = NULL;*/


                    //memcpy(result2, formula, (n_formula+1)*sizeof(int));

                    //result2 = yank(result2, and);

                    if(andcorr) and->top--;
                    result2 = shove(result2, and, splice(OP_AND, 2,
                            splice_l(OP_AND, formula, args_and2),
                            splice(OP_OR, 2,
                                splice_l(OP_OR, formula, arglist+delim[1]),
                                splice(OP_AND, 2,
                                    splice_l(OP_AND, formula, args_and1),
                                    splice_l(OP_OR, formula,
                                        arglist+delim[0])))));

                    //result2 = shove(result2, and, c1);
                    if(andcorr) and->top++;

                    result1 = sanitize(result1, NULL);
                    result2 = sanitize(result2, NULL);

                    /* what the fuck is going on here? well, since the
                     * partition function works on an unordered set basis, we
                     * must take both possible combinations for each partition
                     * into two chunks */
                    n_ret += 2;
                    ret = realloc(ret, (n_ret+1)*sizeof(int *));
                    ret[n_ret-2] = result1;
                    ret[n_ret-1] = result2;
                    ret[n_ret] = NULL;
                }

                for(k = 0; k < n_args_and_or; k++) free(args_and_or[k]);
                free(args_and_or);
            }

            for(k = 0; k < n_args_and_rest; k++) free(args_and_rest[k]);
            free(args_and_rest);
        }

        free(and);

        for(k = 0; k < n_and_ors; k++) free(and_ors[k]);
        free(and_ors);
    }

    return ret;
}

int **r_mix(int *formula)
{
    int n_formula = length(formula);
    int i, j, k;
    int **ret = NULL;
    int n_ret = 0;

    for(i = 0; i < n_formula; i++) {
        if(formula[i] != OP_AND) continue;
        struct range_t *and = range(formula, i);
        int andcorr = 1;//and->top != 0 ? 1 : 0;

        struct range_t **args_and = arguments(formula, and);

        if(args_and == NULL) {
            free(and);
            continue;
        }

        int n_args_and = length_l((void *)args_and);
        int s[n_args_and], m[n_args_and]; s[0] = -1;
        int partsize;

        /* for each partition of AND's arguments of size 2 or 3 */
        while((partsize = partition(n_args_and, s, m))) {
            if(partsize <= 0) break;
            if(partsize != 3 && partsize != 2) continue;

            struct range_t *arglist[n_args_and+3];
            int delim[3] = {0,0,0};

            /* A */
            for(j = 0, k = 0; j < n_args_and; j++)
                if(s[j] == 1) arglist[k++] = args_and[j];
            arglist[k++] = NULL; delim[1] = k;

            /* B */
            for(j = 0; j < n_args_and; j++)
                if(s[j] == 2) arglist[k++] = args_and[j];
            arglist[k++] = NULL; delim[2] = k;

            /* C */
            for(j = 0; j < n_args_and; j++)
                if(s[j] == 3) arglist[k++] = args_and[j];
            arglist[k++] = NULL;

            int *result;

            if(partsize == 2) {
                result = malloc((n_formula+1)*sizeof(int));
                memcpy(result, formula, (n_formula+1)*sizeof(int));

                result = yank(result, and);
                if(andcorr) and->top--;
                result = shove(result, and,
                        splice(OP_OR, 2,
                            splice_l(OP_AND, formula, arglist+delim[0]),
                            splice_l(OP_AND, formula, arglist+delim[1])));
                if(andcorr) and->top++;

                result = sanitize(result, NULL);

                ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                ret[n_ret-1] = result;
                ret[n_ret] = NULL;
            }
            else {
                /* here we consider every pair of the 3 partitions, since they
                 * are unique up to order */
                int *p = NULL;

                while((p = pair(3, p)) != NULL) {
                    /* arglist+delim[p[0]] = A
                     * arglist+delim[p[1]] = B
                     * this will run 3 times, excluding one delim as C each
                     * time (and so the labels above when we build arglist are
                     * actually kinda misleading)
                     */
                    // TODO: is it easier to just count 1,2,3?

                    result = malloc((n_formula+1)*sizeof(int));
                    memcpy(result, formula, (n_formula+1)*sizeof(int));

                    for(j = n_args_and-1; j >= 0; j--) {
                        if(s[j] == p[0]+1 || s[j] == p[1]+1)
                            result = yank(result, args_and[j]);
                    }

                    result = shove(result, and,
                            splice(OP_OR, 2,
                                splice_l(OP_AND, formula, arglist+delim[p[0]]),
                                splice_l(OP_AND, formula, arglist+delim[p[1]])));

                    result = sanitize(result, NULL);

                    ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                    ret[n_ret-1] = result;
                    ret[n_ret] = NULL;
                }
            }
        }

        free(and);

        for(j = 0; j < n_args_and; j++) free(args_and[j]);
        free(args_and);
    }

    return ret;
}
