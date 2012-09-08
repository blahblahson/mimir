#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "list.h"
#include "formula.h"
#include "fcomp.h"
#include "rule.h"
#include "logic.h"

/* here we are clever and represent up to 16 inputs by a single int */
int eval(int *formula, struct range_t *t, uint16_t input)
{
    int tofree_t = 0;
    if(t == NULL) { t = range(formula, 0); tofree_t = 1; }

    /* top>=ATOMLIM is equivalent to top==bot (under sane conditions) */
    else if(formula[t->top] >= ATOMLIM)
        return (input>>formula[t->top])&0x1;

    struct range_t **args = arguments(formula, t);
    int n_args = length_l((void **)args);
    int i;

    int acc; /* accumulator */
    if(formula[t->top] == OP_AND) {
        acc = 1;
        for(i = 0; i < n_args; i++)
            acc &= eval(formula, args[i], input);
    }
    else if(formula[t->top] == OP_OR) {
        acc = 0;
        for(i = 0; i < n_args; i++)
            acc |= eval(formula, args[i], input);
    }

    if(tofree_t) free(t);
    free_l((void **)args);

    return acc;
}

int eval_tf(int *formula)
{
    int i, skip;

    if(formula[0] != OP_AND && formula[0] != OP_OR) return 0;

    for(i = 1, skip = 0; ; i++) {
        if(formula[i] < ATOMLIM) {
            if(formula[i] == OP_CLOSE) {
                if(--skip < 0) break;
                else continue;
            }
            if(formula[i] == OP_FIN) break;

            if(formula[0] == OP_AND && !eval_tf(formula+i)) return 0;
            if(formula[0] == OP_OR && eval_tf(formula+i))   return 1;
            skip++;
        }
        else if(skip) continue;
        else {
            if(formula[0] == OP_AND && !formula[i]) return 0;
            if(formula[0] == OP_OR && formula[i])   return 1;
        }
    }

    return formula[0] == OP_AND ? 1 : 0;
}

int eval2(int *formula, uint16_t input)
{
    int n_f = length(formula);
    int *f = malloc((n_f+1)*sizeof(int));
    memcpy(f, formula, (n_f+1)*sizeof(int));

    int i;
    for(i = 0; i < n_f; i++) {
        /* initialize the evaluation string */
        if(f[i] >= ATOMLIM)
            f[i] = (input>>formula[i])&0x1;
    }

    int ret = eval_tf(f);

    free(f);
    return ret;
}

/* does f1 => f2? */
int implies(int *f1, int *f2)
{
    struct range_t *t1 = range(f1, 0);
    struct range_t *t2 = range(f2, 0);

    int varmax = 0;
    int i;

    for(i = 0; i < t1->bot+1; i++)
        if(varmax < f1[i]) varmax = f1[i];
    for(i = 0; i < t2->bot+1; i++)
        if(varmax < f2[i]) varmax = f2[i];

    /* this is annoying but not really an issue at this stage */
    if(varmax > 15) {
        fprintf(stderr,
                "ERROR: currently switch can only handle up to 15 "
                "residual arguments; exiting.\n");
        exit(1);
    }

    uint16_t input = (0x1<<(varmax+1))-1;
    
    while(input && (eval(f1, t1, input) <= eval(f2, t2, input))) input--;

    free(t1);
    free(t2);

    return input ? 0 : 1;
}

int trivial(int *f1, int *f2)
{
    struct range_t *t1 = range(f1, 0);
    struct range_t *t2 = range(f2, 0);

    int varmax = 0;
    int i;

    for(i = 0; i < t1->bot+1; i++)
        if(varmax < f1[i]) varmax = f1[i];
    for(i = 0; i < t2->bot+1; i++)
        if(varmax < f2[i]) varmax = f2[i];

    /* this is annoying but not really an issue at this stage */
    if(varmax > 15) {
        fprintf(stderr,
                "ERROR: currently switch can only handle up to 15 "
                "residual arguments; exiting.\n");
        exit(1);
    }

    int triv = 0;
    
    for(i = 0; i <= varmax; i++) {
        uint16_t input = (0x1<<(varmax+1))-1;
        uint16_t control1 = (0x1<<i);
        uint16_t control2 = ~(0x1<<i);

        while(input &&
                //(eval(f1, t1, input|control1) <= eval(f2, t2, input&control2)))
                (eval2(f1, input|control1) <= eval2(f2, input&control2)))
            input--;

        if(!input) {
            triv = 1;
            break;
        }
    }
//        while(input && (eval(f1, t1, input) <= eval(f2, t2, input))) input--;

    free(t1);
    free(t2);

    return triv ? 1 : 0;
}

int validinputs(int *formula, struct range_t *t)
{
    int tofree_t = 0;
    if(formula == NULL) return 0;
    if(t == NULL) { t = range(formula, 0); tofree_t = 1; }

    if(formula[t->top] >= ATOMLIM) {
        if(tofree_t) free(t);
        return 1;
    }

    struct range_t **args = arguments(formula, t);
    int n_args = length_l((void **)args);
    /*if(n_args <= 0) {
        if(tofree_t) free(t);
        free_l((void **)args);
        return 0;
    }*/

    if(formula[t->top] == OP_AND) {
        int i, ret = 1;
        for(i = 0; i < n_args; i++)
            ret *= validinputs(formula, args[i]);

        if(tofree_t) free(t);
        free_l((void **)args);
        return ret;
    }
    else if(formula[t->top] == OP_OR) {
        if(n_args == 1) {
            if(tofree_t) free(t);
            int ret = validinputs(formula, args[0]);
            free_l((void **)args);
            return ret;
        }
        int ia = validinputs(formula, args[n_args-1]); /* #A */
        int na = (int)pow(2, (double)atomcount(formula, args[n_args-1]));

        struct range_t t2;
        t2.top = t->top;
        t2.bot = args[n_args-1]->top;

        int ib = validinputs(formula, &t2); /* #B */
        int nb = (int)pow(2, (double)atomcount(formula, &t2)); /* 2^|B| */

        if(tofree_t) free(t);
        free_l((void **)args);

        /* #(A v B) = 2^|A|.#B + 2^|B|.#A - #A.#B */
        return na*ib + nb*ia-ia*ib;
    }
    else {
        if(tofree_t) free(t);
        free_l((void **)args);
        return 0;
    }
}

int quickprove2(int *f1, int *f2, int f2vi)
{
    /* nonsense */
    if(f1 == NULL || f2 == NULL)
        return 0;

    //if(equiv(f1, NULL, f2, NULL)) return 1;

    if(validinputs(f1, NULL)+2 > f2vi) return 0;

    //printformula(f1);

    int i;

    /* can we prove it with one step of mix, switch or medial? */
    /*int **next_mix = r_mix(f1);
    int n_next_mix = length_l((void **)next_mix);
    for(i = 0; i < n_next_mix; i++) {
        if(implies(next_mix[i], f2)) {
            free_l((void **)next_mix);
            return 1;
        }
    }*/
    /*if(find(next_mix, n_next_mix, f2) >= 0) {
        free_l((void **)next_mix);
        return 1;
    }*/

    if(r_switch2(f1, f2)) return 1;
    if(r_medial2(f1, f2)) return 1;
    /*if(find(next_switch, n_next_switch, f2) >= 0) {
        free_l((void **)next_mix);
        free_l((void **)next_switch);
        return 1;
    }*/

    return 0;
}

int quickprove(int *f1, int *f2, int f2vi)
{
    /* nonsense */
    if(f1 == NULL || f2 == NULL)
        return 0;

    if(equiv(f1, NULL, f2, NULL)) return 1;

    if(validinputs(f1, NULL)+2 > f2vi) return 0;

    //printformula(f1);

    /* can we prove it with one step of mix, switch or medial? */
    int **next_mix = r_mix(f1);
    int n_next_mix = length_l((void **)next_mix);
    if(find(next_mix, n_next_mix, f2) >= 0) {
        free_l((void **)next_mix);
        return 1;
    }

    int **next_switch = r_switch(f1);
    int n_next_switch = length_l((void **)next_switch);
    if(find(next_switch, n_next_switch, f2) >= 0) {
        free_l((void **)next_mix);
        free_l((void **)next_switch);
        return 1;
    }

    int **next_medial = r_medial(f1);
    int n_next_medial = length_l((void **)next_medial);
    if(find(next_medial, n_next_medial, f2) >= 0) {
        free_l((void **)next_mix);
        free_l((void **)next_switch);
        free_l((void **)next_medial);
        return 1;
    }

    /* if not, can we prove it from any of the results we got from mix, switch,
     * medial with further steps? */
    int i;

    // TODO: put all 3 results from each rule into one list and just plug that
    // into prove, rather than doing it three times

    /* mix */
    for(i = 0; i < n_next_mix; i++) {
//        printf("prove from mix\n");
        if(quickprove(next_mix[i], f2, f2vi)) {
            free_l((void **)next_mix);
            free_l((void **)next_switch);
            free_l((void **)next_medial);
            return 1;
        }
    }

    /* switch */
    for(i = 0; i < n_next_switch; i++) {
//        printf("prove from switch\n");
//        printformula(next_switch[i]);
        if(quickprove(next_switch[i], f2, f2vi)) {
            free_l((void **)next_mix);
            free_l((void **)next_switch);
            free_l((void **)next_medial);
            return 1;
        }
    }

    /* medial */
    for(i = 0; i < n_next_medial; i++) {
//        printf("prove from medial\n");
        if(quickprove(next_medial[i], f2, f2vi)) {
            free_l((void **)next_mix);
            free_l((void **)next_switch);
            /* ok, I think this function could do with some gotos... */
            free_l((void **)next_medial);
            return 1;
        }
    }

    /* and assuming we can't do THAT, there's no way we can prove it */
    free_l((void **)next_mix);
    free_l((void **)next_switch);
    free_l((void **)next_medial);
    return 0;
}
