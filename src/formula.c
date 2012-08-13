#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "list.h"
#include "formula.h"

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

/* returns the parent operator of any atom, or if s refers to an operator,
 * returns s */
int parent(int *formula, int s)
{
    if(formula[s] < ATOMLIM) return s;

    int i;
    for(i = s; i > 0; --i)
        if(formula[i] < ATOMLIM) break;
    return i;
}

int parent_deep(int *formula, int s)
{
    int i;
    for(i = s; i > 0; --i)
        if(formula[i] < ATOMLIM) break;
    return i;
}

int atomcount(int *formula, struct range_t *t)
{
    int tofree_t = 0;
    if(formula == NULL) return 0;
    if(t == NULL) { t = range(formula, 0); tofree_t = 1; }

    int i, count = 0;
    for(i = t->top; i < t->bot; i++) if(formula[i] >= ATOMLIM) count++;
    if(tofree_t) free(t);

    if(t->top == t->bot && formula[t->top] >= ATOMLIM) return 1;
    return count;
}

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
    int spliced = 0;

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

        spliced++;
        free(f);
    }

    va_end(ap);

    if(spliced == 0) {
        free(ret);
        return NULL;
    }
    else if(spliced == 1) {
        memmove(ret, ret+1, (j-1)*sizeof(int));
        ret = realloc(ret, j*sizeof(int));
        j--;
    }
    else
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

    int todec_toadd = 0;
//    int start = parent(formula, t->top);
    // TODO: we get A LOT of valgrind access errors here, to do with t->top
    // sometimes being -1 when shove is called by r_switch... it's 3am and I
    // can't figure out exactly what's going on so for now this will have to
    // do... functionality seems to be fine otherwise
    if(toadd[0] < ATOMLIM && formula[parent(formula, t->top)] == toadd[0]) {
        toadd++;
        n_toadd -= 2; /* no more OP or ) */
        todec_toadd = 1; /* but before we free it, gotta -- */
    }

    int n_new = n+n_toadd;
    int *ret = malloc((n_new+1)*sizeof(int));
    memcpy(ret, formula, (t->top+1)*sizeof(int));
    memcpy(ret+t->top+1, toadd, (n_toadd)*sizeof(int));
    memcpy(ret+t->top+1+n_toadd, formula+t->top+1, (n-t->top)*sizeof(int));

    if(todec_toadd) toadd--;
    free(toadd);
    free(formula);
    return ret;
}

/* this is quite an expensive function to call I think... */
int *sanitize(int *formula, struct range_t *t)
{
    int tofree_t = 0;
    if(t == NULL) { t = range(formula, 0); tofree_t = 1; }

    if(formula[t->top] >= ATOMLIM) {
        if(tofree_t) free(t);
        return formula;
    }

    struct range_t **args = arguments(formula, t);
    int n_args = length_l((void **)args);
    int n_formula = length(formula);
    int i;
    /* we exploit the fact that while sanitize will change and resize formula,
     * it will only fuck the ranges of subsequent arguments, not preceding
     * ones, arguments() gives arguments in ascending index order, and so we
     * approach backwards where this is not a problem */
    for(i = n_args-1; i >= 0; i--)
        formula = sanitize(formula, args[i]);

    free_l((void **)args);
    args = arguments(formula, t);

    n_args = length_l((void **)args);

    if(n_args == 1) {
        memmove(formula+t->top, formula+t->top+1, (n_formula-t->top)*sizeof(int));
        memmove(formula+t->bot-1, formula+t->bot, (n_formula-t->bot)*sizeof(int));
        n_formula -= 2;
    }
    else {
        for(i = n_args-1; i >= 0; i--) {
            if(formula[args[i]->top] == formula[t->top]) {
                memmove(formula+args[i]->top, formula+args[i]->top+1,
                        (n_formula-args[i]->top)*sizeof(int));
                memmove(formula+args[i]->bot-1, formula+args[i]->bot,
                        (n_formula-args[i]->bot)*sizeof(int));
                n_formula -= 2;
            }
        }
    }

    formula = realloc(formula, (n_formula+1)*sizeof(int));

    if(tofree_t) free(t);
    free_l((void **)args);

    return formula;
}
