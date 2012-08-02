#include "mim.h"

unsigned long long int X = 0;

/* formula - any formula that includes the formula end operator
 * return - the literal number of symbols describing formula, up to but not
 *          including the formula end operator
 */
int length(const int *formula)
{
    if(formula == NULL) return 0;
    int i;
    for(i = 0; formula[i] != OP_FIN; i++) ;
    return i;
}

/* list - any NULL-terminated list
 * size - the size of list elements
 * return - the number of elements, up to but not including the terminating
 *          NULL element
 * e.g. for a list: struct range_t **ranges
 *      to count the number of elements, we would call:
 *          length_l(ranges, sizeof(struct range_t *));
 */
int length_l(void **list)
{
    if(list == NULL) return 0;
    int i = -1;
    while(list[++i] != NULL) ;
    return i;
}
// to test:
// struct range_t a,b;
// struct range_t *x[3] = {&a,&b,NULL};
// length_l(x, sizeof(struct range_t *)) == 2

/* formula - any formula, valid within the scope of the operator s refers to
 * s - index within formula pointing to an atom or operator
 * return - s refers to an operator: the index of the complementary closing
 *              operator
 *          s refers to an atom: s
 *          s refers to a terminating operator: s
 */
int scope(int *formula, int s)
{
    /* if it's an atom or a terminating operator, just return s */
    if(formula[s] >= ATOMLIM || formula[s] == OP_CLOSE || formula[s] == OP_FIN)
        return s;

    int i, c;
    for(i = s+1, c = 1; c > 0; i++) {
        if(formula[i] >= ATOMLIM) continue;
        /* consider only operators */
        switch(formula[i]) {
            case OP_CLOSE:
                c--;
                break;
            case OP_FIN:
                return i-1;
            default:
                c++;
        }
    }

    return i-1;

}
// to test:
// int f[] = {OP_AND, OP_OR, 0, 1, 2, OP_END, 3, OP_OR, 4, 5, OP_AND, 6, 7, OP_END, OP_END, OP_END, OP_FIN};
// scope(f,0) == 15 should be true
// scope(f,1) == 5
// scope(f,2) == 2
// scope(f,5) == 5
// scope(f,7) == 14

struct range_t *range(int *formula, int s)
{
    struct range_t *ret = malloc(sizeof(struct range_t));
    ret->top = s;
    ret->bot = scope(formula, s);
    return ret;
}

/* formula - any formula, valid within the scope of *t
 * t - a pointer to a range describing an operator in formula
 * return - a NULL-terminated list of pointers to ranges describing every
 *          argument of the operator referred to by *t
 *          returns NULL if there are exactly no arguments
 */
struct range_t **arguments(int *formula, struct range_t *t)
{
    // iterate over formula from t->top to t->bot, finding every argument
    struct range_t **ret = malloc((t->bot-t->top)*sizeof(struct range_t *));

    int i, j;
    for(i = t->top+1, j = 0; i < t->bot; i++) {
        /*ret[j] = malloc(sizeof(struct range_t));
        ret[j]->top = i;
        ret[j]->bot = scope(formula, i);*/
        ret[j] = range(formula, i);
        i = ret[j]->bot;
        j++;
    }

    if(j == 0) {
        free(ret);
        return NULL;
    }

    ret[j++] = NULL;
    ret = realloc(ret, j*sizeof(struct range_t *));

    return ret;
}

/* formula - any formula, valid within the scope of (*t)
 * t - a pointer to a range describing an operator in formula
 * op - any operator
 * return - a NULL-terminated list of pointers to ranges describing every
 *          instance of an operator op within *t
 */
struct range_t **arguments_op(int *formula, struct range_t *t,
        enum OPERATOR op)
{
    // iterate over formula from t->top to t->bot, finding every argument
    struct range_t **ret = malloc((t->bot-t->top)*sizeof(struct range_t *));

    int i, j;
    for(i = t->top+1, j = 0; i < t->bot; i++) {
        if(formula[i] >= ATOMLIM) continue;
        else if(formula[i] != op) {
            i = scope(formula, i);
            continue;
        }

        /*ret[j] = malloc(sizeof(struct range_t));
        ret[j]->top = i;
        ret[j]->bot = scope(formula, i);*/
        ret[j] = range(formula, i);
        i = ret[j]->bot;
        j++;
    }

    if(j == 0) {
        free(ret);
        return NULL;
    }

    ret[j++] = NULL;
    ret = realloc(ret, j*sizeof(struct range_t *));

    return ret;
}

/* same as arguments, except exclude any argument that's the same top as ex */
struct range_t **arguments_ex(int *formula, struct range_t *t,
        struct range_t *ex)
{
    // iterate over formula from t->top to t->bot, finding every argument
    struct range_t **ret = malloc((t->bot-t->top)*sizeof(struct range_t *));

    int i, j;
    for(i = t->top+1, j = 0; i < t->bot; i++) {
        /*ret[j] = malloc(sizeof(struct range_t));
        ret[j]->top = i;
        ret[j]->bot = scope(formula, i);*/
        ret[j] = range(formula, i);
        i = ret[j]->bot;
        if(ex->top != ret[j]->top) j++;
        else free(ret[j]);
    }

    if(j == 0) {
        free(ret);
        return NULL;
    }

    ret[j++] = NULL;
    ret = realloc(ret, j*sizeof(struct range_t *));

    return ret;
}

/* n - number of objects to find all pairs of
 * p - the last pair, or NULL if we are starting afresh. this is the same as
 *     the return value
 * return - a pair out of the n objects in the form of a size-2 array (NOT
 *          terminated or anything), successive identical calls then yielding
 *          subsequent unique pairs until all have been exhausted, at which
 *          point NULL is returned
 * NOTE: memory is handled entirely by this function. the correct way to call
 * it initially is as follows:
 *      int *p = NULL;
 *      while(pair(n, p) != NULL) { do stuff with p here }
 *      // no need to free p at any point
 */
// TODO: to replace permute2, which is a dumb name
int *pair(int n, int *p)
{
    /* you clearly can't take two balls out of a bag of only one ball */
    if(n < 2) return NULL;
    
    if(p == NULL) {
        p = malloc(2*sizeof(int));
        p[0] = 0;
        p[1] = 1;
        return p;
    }

    if(++p[1] >= n)
        p[1] = (++p[0])+1;

    if(p[1] >= n) {
        free(p);
        return NULL;
    }

    return p;
}

/* n - size of the set to partition
 * s - array of size n, which will be modified to describe the new partitioning
 * m - array of size n, which is used internally and of no real interest
 * return - the size (number of disjoint subsets) of the new partition (which
 *          has been encoded in s)
 * NOTE: similarly to pair(), this does all its own memory management, so do:
 *      int *s, *m;
 *      s = m = NULL;
 *      while((x = partition(n, s, m)) > 0) { do stuff with s }
 *      // no need to free s, m
 */
// TODO: make it spit out indexes from 0 rather than 1, be consistent!!
int partition(int n, int *s, int *m)
{
    int i;

    if(s[0] < 0) {
        //s = malloc(n*sizeof(int));
        //m = malloc(n*sizeof(int));
        for(i = 0; i < n; i++) s[i] = m[i] = 1;
        //memset(s, 1, n*sizeof(int));
        //memset(m, 1, n*sizeof(int));

        return 1;
    }

    int ret = 1;
    i = 0;
    s[i]++;
    while(i < n-1 && s[i] > m[i]+1) {
        s[i] = 1;
        i++;
        s[i]++;
    }

    /* if i is has reached n-1 th element, then the last unique partitiong
     * has been found
     */
    if(i == n-1) {
        s[0] = -1;
        return 0;
    }

    /* because all the first i elements are now 1, s[i] (i + 1 th element)
     * is the largest. so we update max by copying it to all the first i
     * positions in m
     */
    int maximum = s[i];
    if(m[i] > maximum) maximum = m[i];
    for(i = i-1; i >= 0; --i) m[i] = maximum;

    ret = max(m[0],s[0]);

    /*printf("s: ");
    for (i = 0; i < n; ++i)
        printf("%d ", s[i]);
    printf("\nm: ");*/
    /*for (i = 0; i < n; ++i)
        printf("%d ", m[i]);
    printf("\n");*/
    //getchar();*/

    //while(m[0] < 4) if(!next(s, m, n)) return 0;
    return ret;
}

/* op - the operator under whose basis we are unifying
 * n - number of lists to splice together
 * ... - n instances of end-operator-terminated lists of type int *, describing
 *       total, terminated formulae
 * return - if ... is some sequence of formulae A1,A2,...,An, and op is some
 *          arbitrary operator OP, then the returned value is a NULL-terminated
 *          list of type int * describing the formula OP(A1,A2,...,An)
 *          TODO: having undergone weak (or strong?) sanitization?
 */
int *splice(enum OPERATOR op, int n, ...)
{
    // for each ... (x)
    // 
    va_list ap;
    int i, j;
    int *f, *ret;

    if(n < 1) return NULL;

    int n_ret = 3; /* start operator op, plus terminating operators */
    ret = malloc(n_ret*sizeof(int));

    j = 0;
    ret[j++] = op;
    va_start(ap, n);
    volatile int n_f;

    for(i = 0; i < n; i++) {
        f = va_arg(ap, int *);

        if(!(n_f = length(f))) continue;
        
        if(f[0] == op) {
            /* OP,[...],CLOSE */
            n_ret += n_f-2;
            ret = realloc(ret, n_ret*sizeof(int));
            memcpy(ret+j, f+1, (n_f-2)*sizeof(int));
            j += n_f-2;
        }
        else {
            /* [OP,...,CLOSE] */
            n_ret += n_f;
            ret = realloc(ret, n_ret*sizeof(int));
            memcpy(ret+j, f, n_f*sizeof(int));
            j += n_f;
        }

        free(f);
    }

    va_end(ap);

    ret[j++] = OP_CLOSE;
    ret[j] = OP_FIN;
    //printf("split: ");
    //printformula(ret);
    return ret;
}

/* op - the operator under whose basis we are unifying
 * formula - any formula, valid within the scope of *t
 * t - a NULL-terminated list of pointers to ranges describing arguments in formula
 * return - if t describes some arguments within formula A1,A2,...,An, and op
 *          is some arbitrary operator OP, then the returned value is a
 *          NULL-terminated list of type int * describing the formula
 *          OP(A1,A2,...,An)
 *          TODO: having undergone weak (or strong?) sanitization?
 */
int *splice_l(enum OPERATOR op, const int *formula, struct range_t **t)
{
    int n_t, n_formula;
    if(!(n_t = length_l((void **)t))) return NULL;
    if(!(n_formula = length(formula))) return NULL;
    int i, j;

    if(n_t == 1) { /* OP(A) = A */
        int *ret = malloc((t[0]->bot-t[0]->top+2)*sizeof(int));
        memcpy(ret, formula+t[0]->top, (t[0]->bot-t[0]->top+1)*sizeof(int));
        ret[t[0]->bot-t[0]->top+1] = OP_FIN;
        return ret;
    }

    /* 2 = the extra operator plus its terminator
     * 1 = end terminator
     */
    int *ret = malloc((n_formula+1+2)*sizeof(int));

    j = 0;
    ret[j++] = op;
    volatile int n_f;

    for(i = 0; i < n_t; i++) {
        if(formula[t[i]->top] == op) {
            /* OP,[...],CLOSE */
            n_f = (t[i]->bot-t[i]->top+1)-2;
            memcpy(ret+j, formula+t[i]->top+1, n_f*sizeof(int));
            j += n_f;
        }
        else {
            /* [OP,...,CLOSE] */
            n_f = t[i]->bot-t[i]->top+1;
            memcpy(ret+j, formula+t[i]->top, n_f*sizeof(int));
            j += n_f;
        }
    }

    ret[j++] = OP_CLOSE;
    ret[j++] = OP_FIN;
    ret = realloc(ret, j*sizeof(int));
    //printf("split_l: ");
    //printformula(ret);
    return ret;
}
// TODO: const some shit?

/* formula - any formula containing the formula end operator after t->bot
 * t - a pointer to a range describing some range in formula, ideally a full
 *     argument
 * return - the number of elements removed (#bytes removed/sizeof(int)).
 *          formula is modified to have the range *t removed, and the remaining
 *          stuff shifted up to avoid some huge void of undefinedness within
 *          formula. formula is resized accordingly.
 */
int *yank(int *formula, struct range_t *t)
{
    // TODO: check these indices, could easily go wrong
    int n = length(formula);
    int n_new = n-(t->bot-t->top);
    memmove(formula+t->top, formula+t->bot+1, (n-t->bot)*sizeof(int));
    formula = realloc(formula, n_new*sizeof(int));
    return formula;
}

/* formula - any formula
 * t - a pointer to a range describing some operator within formula
 * toadd - a formula that includes a formula end operator
 * return - the number of elements inserted (#bytes added/sizeof(int)).
 *          formula is modified such that the operator described by *t has the
 *          new argument described by toadd (formulae and arguments are
 *          interchangeable notions)
 *          TODO: minor sanitizing? it could be quite easy to do here
 */
int *shove(int *formula, struct range_t *t, int *toadd)
{
    int n = length(formula);
    int n_toadd = length(toadd);
    int n_new = n+n_toadd;
    int *ret = malloc((n_new+1)*sizeof(int));
    memcpy(ret, formula, (t->top+1)*sizeof(int));
    memcpy(ret+t->top+1, toadd, (n_toadd)*sizeof(int));
    memcpy(ret+t->top+1+n_toadd, formula+t->top+1, (n-t->top)*sizeof(int));
    free(toadd);
    free(formula);
    return ret;
}

/* formula - any bastardized formula
 * t - pointer to a range within formula, in which we shall perform some
 *     sanitizing to make sure the formula is as succinct as possible
 * return - TODO
 * TODO: this whole idea here
 */
//int sanitize(int *formula, struct range_t *t);

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
                                        arglist+delim[0]),
                                    splice_l(OP_AND, formula,
                                        arglist+delim[1])),
                                splice(OP_OR, 2,
                                    splice_l(OP_AND, formula,
                                        arglist+delim[2]),
                                    splice_l(OP_AND, formula,
                                        arglist+delim[3]))));
                    // TODO: sanitization!!!!

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
                    if((counter>>k)&0x1)
                        args_and1[n_args_and1++] = args_and_rest[k];
                    else
                        args_and2[n_args_and2++] = args_and_rest[k];
                }

                args_and1[n_args_and1] = NULL;
                args_and2[n_args_and2] = NULL;

                // make an args_and2 which is the ones we don't use, then reconstruct the whoele thing-- thisll be easier!

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
                    arglist[l++] = NULL; delim[1] = k;

                    /* C */
                    for(k = 0; k < n_args_and_or; k++)
                        if(s[k] == 2) arglist[l++] = args_and_or[k];
                    arglist[l++] = NULL;

                    int *result = malloc((n_formula+1)*sizeof(int));
                    memcpy(result, formula, (n_formula+1)*sizeof(int));

                    /* here the principle is a bit more straightforward than in
                     * r_medial - we simply remove the entire toplevel AND
                     * chunk that we found and rebuild it out of its
                     * constituent aruments, applying the switch in the process
                     */
                    result = yank(result, and);
                    int *a1 = splice_l(OP_OR, formula, arglist+delim[0]);
                    int *a2 = splice_l(OP_OR, formula, arglist+delim[1]);
                    int *a3 = splice_l(OP_AND, formula, args_and1);
                    int *a4 = splice_l(OP_AND, formula, args_and2);

                    int *b1 = splice(OP_AND, 2, a3, a2);
                    int *b2 = splice(OP_OR, 2, a1, b1);

                    int *c1 = splice(OP_AND, 2, a4, b2);
                    /*
                    result = shove(result, and, splice(OP_AND, 2,
                            splice_l(OP_AND, formula, args_and2),
                            splice(OP_OR, 2,
                                splice_l(OP_OR, formula, arglist+delim[0]),
                                splice(OP_AND, 2,
                                    splice_l(OP_AND, formula, args_and1),
                                    splice_l(OP_OR, formula,
                                        arglist+delim[1])))));
                                        */

                    and->top--;
                    result = shove(result, and, c1);
                    and->top++;
                    // TODO: splice is limited in that it can only take int *s,
                    // and likewise for splice_l. consider writing a splice_x
                    // that can take 
                    // TODO: sanitization? I think the splices sort this out

                    ret = realloc(ret, (++n_ret+1)*sizeof(int *));
                    ret[n_ret-1] = result;
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
                and->top--;
                result = shove(result, and,
                        splice(OP_OR, 2,
                            splice_l(OP_AND, formula, arglist+delim[0]),
                            splice_l(OP_AND, formula, arglist+delim[1])));
                and->top++;

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

void exhaust(int *formula)
{
    if(formula == NULL)
        return;
    //printformula(formula);
    X++;
    int **stuff = r_mix(formula);
    int n = length_l((void **) stuff);
    //if(n == 0) printf("FINISHED!!\n");
    int i;
    for(i = 0; i < n; i++) {
        exhaust(stuff[i]);
        free(stuff[i]);
    }
    free(stuff);
}

int main(int argc, char *argv[])
{
    /*
    printf("testing length():\n");
    int f1[] = {OP_OR, OP_AND, 2, 3, OP_CLOSE, 1, OP_FIN};
    printf("%d == 6? | %d == 1? | %d == 0?\n", length(f1), length(f1+5), length(f1+6));
    printf("\n");
    printf("testing length_l():\n");
    struct range_t a,b;
    struct range_t *x[] = {&a,&b,NULL};
    struct range_t *y[] = {NULL,&a,&a,&b,&b,&a,&b,&b,NULL};
    printf("%d == 2 | %d == 0? | %d == 7?\n", length_l((void **)x), length_l((void **)y), length_l((void **)y+1));
    printf("\n");
    printf("testing scope():\n");
    int f3[] = {OP_AND, 1, 2, OP_OR, 3, 4, OP_CLOSE, 5, OP_FIN};
    int f2[] = {OP_AND, 1, 2, OP_OR, 3, 4, OP_CLOSE, 5, OP_CLOSE, OP_AND, OP_FIN};
    printf("%d == 8? | %d == 1? | %d == 7?\n", scope(f2, 0), scope(f2,1),scope(f3,0));
    printf("\n");
    printf("testing r_medial():\n");*/
    int f[] = {OP_AND,
                    1,
                    OP_OR, // 2
                        2,
                        OP_AND, // 4
                            3, 4, 5,
                        OP_CLOSE, // 7
                        OP_AND, // 8
                            6, 7, 8,
                        OP_CLOSE,
                        OP_AND,
                            9, 10, 11, 12,
                        OP_CLOSE,
                        OP_AND,
                            13,14,15,16,17,
                        OP_CLOSE,
                        OP_AND,
                            18,19,20,21,
                        OP_CLOSE,
                    OP_CLOSE, // 13
                OP_CLOSE,
                OP_FIN};
    int ff[] = {OP_AND, 1,
            OP_OR,
                OP_AND,
                    OP_OR,
                        OP_AND, 2, 3, OP_CLOSE,
                        OP_AND, 4, 5, OP_CLOSE,
                    OP_CLOSE,
                    OP_OR, 6, 7, OP_CLOSE,
                OP_CLOSE,
                8,
                OP_AND, 9, 10, 11, 12, OP_CLOSE,
                OP_AND, 13, 14, 15, 16, 17, OP_CLOSE,
                OP_AND, 18, 19, 20, 21, OP_CLOSE,
            OP_CLOSE,
        OP_CLOSE,
        OP_FIN};
    int fff[] = {OP_AND, 1, 2, OP_CLOSE, OP_FIN};
    int g[] = {OP_AND, 1, 4, OP_OR, 2, 3, 5, 6, 7, OP_CLOSE, OP_CLOSE, OP_FIN};
    int h[] = {OP_AND, 1, 2, 3, 4, 5, 6, 7, 8, OP_CLOSE, OP_FIN};
    //exhaust(f);

    exhaust(h);
    printf("%llu\n", X);
    //exhaust(ff);

//    int **stuff;
   
//    stuff = r_mix(h);
//    stuff = r_switch(g);
//    if(stuff == NULL) printf("null\n");
//    int n = length_l((void **) stuff);
//    int i, j, k;
//    for(i = 0; i < n; i++) {
        //printf("---");
//        printformula(stuff[i]);

        /*int **stuff2 = r_medial(stuff[i]);
        int n2 = length_l((void **) stuff2);
        for(k = 0; k < n2; k++) {
            printformula(stuff2[k]);
            free(stuff2[k]);
        }
        free(stuff2);*/
//        free(stuff[i]);
//    }
//    free(stuff);
                 


    return 0;
}

