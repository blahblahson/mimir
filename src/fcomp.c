#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "list.h"
#include "fcomp.h"

/* are the formulae a, b the same? */
int equiv(int *f1, struct range_t *t1, int *f2, struct range_t *t2)
{
    if(f1 == NULL || f2 == NULL) return 0;
    int tofree_t1 = 0;
    int tofree_t2 = 0;
    if(t1 == NULL) { t1 = range(f1, 0); tofree_t1 = 1; }
    if(t2 == NULL) { t2 = range(f2, 0); tofree_t2 = 1; }

    if(f1[t1->top] != f2[t2->top]) {
        if(tofree_t1) free(t1);
        if(tofree_t2) free(t2);
        return 0;
    }

    struct range_t **args1 = arguments(f1, t1);
    struct range_t **args2 = arguments(f2, t2);
    int i, j;
    int n_args = length_l((void **)args1);
    int n_args2 = length_l((void **)args2);

    if(n_args != n_args2) {
        free_l((void **)args1);
        free_l((void **)args2);
        if(tofree_t1) free(t1);
        if(tofree_t2) free(t2);
        return 0;
    }

    /* WARNING: no atoms should be > 15 */
    uint16_t c1, c2; c1 = c2 = 0;
    int ac1, ac2, oc1, oc2; ac1 = ac2 = oc1 = oc2 = 0;

    for(i = 0; i < n_args; i++) {
        if(f1[args1[i]->top] >= ATOMLIM) /* it's an atom */
            c1 |= 0x1<<f1[args1[i]->top];
        else if(f1[args1[i]->top] == OP_AND) ac1++;
        else if(f1[args1[i]->top] == OP_OR) oc1++;
    }

    for(i = 0; i < n_args; i++) {
        if(f2[args2[i]->top] >= ATOMLIM) /* it's an atom */
            c2 |= 0x1<<f2[args2[i]->top];
        else if(f2[args2[i]->top] == OP_AND) ac2++;
        else if(f2[args2[i]->top] == OP_OR) oc2++;
    }

    /* TODO: this should eventually be extended more generally to all types of
     * operator I think, although it might be hard */
    if(c1 != c2 /* are the toplevel atoms the same? */
            || oc1 != oc2 /* are the number of toplevel ORs the same? */
            || ac1 != ac2) { /* or ANDs? */
        /* if any of these three things aren't true, the formulae are not
         * equivalent */
        free_l((void **)args1);
        free_l((void **)args2);
        if(tofree_t1) free(t1);
        if(tofree_t2) free(t2);
        return 0;
    }

    int checked[n_args]; memset(checked, 0, n_args*sizeof(int));
    /* for each ANDOR argument in A, is there an equivalent one in B? if this
     * is the case, since we are dealing with balanced formulae only, we can
     * conclude (along with the above checks) that A and B are equivalent */
    for(i = 0; i < n_args; i++) {
        if(f1[args1[i]->top] >= ATOMLIM) {
            checked[i] = 1;
            continue;
        }

        for(j = 0; j < n_args; j++) {
            if(f2[args2[j]->top] >= ATOMLIM) continue;
            if(equiv(f1, args1[i], f2, args2[j])) {
                checked[i] = 1;
                break;
            }
        }
    }

    for(i = 0; i < n_args; i++) if(!checked[i]) break;

    free_l((void **)args1);
    free_l((void **)args2);
    if(tofree_t1) free(t1);
    if(tofree_t2) free(t2);

    return (i == n_args) ? 1 : 0;
}

int find(int **formulae, int n_formulae, int *formula)
{
    if(n_formulae <= 0) n_formulae = length_l((void **)formulae);
    if(n_formulae <= 0) return -1;
    struct range_t *t = range(formula, 0);

    int i;
    for(i = 0; i < n_formulae; i++) {
        if(equiv(formulae[i], NULL, formula, t)) {
            free(t);
            return i;
        }
    }

    free(t);

    return -1;
}
