#include "main.h"

int F_OPERATOR_ARITY[] = {
    -1, /* N/A */
    0,  /* F_OP_END */
    2,  /* F_OP_AND */
    2,  /* F_OP_OR */
    1   /* F_OP_NOT */
,3
};

/* length of formula, i.e. number of operators + number of atoms */
unsigned int flength(int formula[])
{
    int i;
    for(i = 0; ; i++) /* TODO: implement limit, just like strnlen */
        if(formula[i] == F_OP_END) return i;
    return 0; /* should never get here */
}

/* number of atoms in formula */
unsigned int atomcount(int formula[])
{
    int i,j;
    for(i = 0, j = 0; ; i++) { /* TODO: implement limit, just like strnlen */
        if(formula[i] >= 0) j++; /* positive element indicates atom */
        if(formula[i] == F_OP_END) return j; /* end */
    }
    return 0; /* should never get here */
}

// WARNING: make sure to free output at some point
int **opinfluence(int formula[])
{
    int n_f = flength(formula);
    int depth[n_f]; memset(depth, 0, n_f*sizeof(int));
    int **influence = calloc(sizeof(int *), n_f);
    int i, j, k, d;
    for(i = 0, d = -1; i < n_f; i++) {
        if(formula[i] >= 0) { /* atom */
            influence[i] = NULL; /* an atom has no influence */
            depth[i] = ++d;
            //printf("depth[%d] = %d | %d\n", i, depth[i], d);
        }
        else { /* operator */
            d -= F_OPERATOR_ARITY[-1*formula[i]]-1;
            depth[i] = d;
            //printf("depth[%d] = %d | %d\n", i, depth[i], d);
 
            //for(j = i-1; j >= 0 && depth[j] >= depth[i]; j--); j++;
            int arity = F_OPERATOR_ARITY[-1*formula[i]];
            influence[i] = malloc(arity*sizeof(int));
            for(j = i-1, k = arity-1; j >= 0 && k >= 0; j--) { // k <= arity
                //printf("op=%d: j=%d, k=%d, check, is %d < %d?\n", formula[i], j, k, depth[j], depth[i]+k);
                if(depth[j] < depth[i]+k) {
                    //printf("yup\n");
                    influence[i][k--] = j+1;
                }
            }
            //influence[i][0] = j+1;
        }
    }

    return influence;
}

int evalformula_i(int formula[], int input[])
{
    unsigned int flen = flength(formula);
    //printf("length = %d\n",flen);
    unsigned int n = atomcount(formula);
    //printf("%d - %d\n", n, flength(input));
    if(n != flength(input)) return -1; /* input MUST correspond to formula */

    int stack[n];
    memset(stack, F_OP_END, n);

    int i, j, eval;
    for(i = 0, j = 0; i < flen+1; i++) { /* begin evaluation */
        if(formula[i] >= 0) { /* atom - push onto the stack */
            stack[j++] = input[formula[i]] ; /* TODO: sanity check!! */
            //printf("throwing %d on the stack\n", input[formula[i]]);
        }
        else { /* operator - work it out! */
            switch(formula[i]) {
                case F_OP_END:
                    return stack[j-1]; /* end of formula */
                case F_OP_AND:
                    //printf("doing %d & %d\n", stack[j-2] , stack[j-1]);
                    //printf("doing %d & %d {debug: stack[%d]|stack[%d]}\n", stack[j-2] , stack[j-1], j-2, j-1);
                    eval = stack[j-1] & stack[j-2];
                    stack[j-1] = F_OP_END;
                    stack[j-2] = eval;
                    j--;
                    break;
                case F_OP_OR:
                    //printf("doing %d | %d {debug: stack[%d]|stack[%d]}\n", stack[j-2] , stack[j-1], j-2, j-1);
                    eval = stack[j-1] | stack[j-2];
                    stack[j-1] = F_OP_END;
                    stack[j-2] = eval;
                    j--;
                    break;
                    /*printf("doing %d | %d {debug: stack[%d]|stack[%d]}\n", stack[j-2] , stack[j-1], j-2, j-1);
                    eval = stack[j-1] & stack[j-2];
                    printf(" --> eval=%d\n", eval);
                    stack[j-1] = F_OP_END;
                    stack[j-2] = eval;
                    j--;
                    break;*/
                case F_OP_NOT:
                    stack[j-1] = (~stack[j-1])&0x1; /* simple negation */
                    break;
                default:
                    fprintf(stderr, "warning: unknown operator %d ignored\n",
                            formula[i]);
            }
        }
    }

    return -1;
}

int evalformula(int formula[])
{
    //unsigned int flen = flength(formula);
    unsigned int n_atoms = atomcount(formula);
    int n_inputs = pow(2,n_atoms); // TODO: error checking
    printf("atomcount = %d, n inputs = %d\n", n_atoms, n_inputs);
    int inputs[n_inputs][n_atoms+1];
    int outputs[n_inputs];
    memset(inputs, 0, n_inputs*(n_atoms+1)*sizeof(int));

    int i, j, k;
    /* terminate inputs */
    for(i = 0; i < n_inputs; i++) inputs[i][n_atoms] = F_OP_END;

    /* populate 1s */
    for(j = 0; j < n_atoms; j++) { /* for every atom... */
        double inc = pow(2, j+1);
        for(i = inc; i <= n_inputs; i+=inc) {
            for(k = 1; k <= inc/2; k++) { /* for every valid input */
                printf("[%d][%d] = 1\n", i-k, n_atoms-1-j);
                inputs[i-k][n_atoms-1-j] = 1;
                //getchar();
            }
        }
        printf("finished j=%d, starting %d\n", j, j+1);
    }

    for(i = 0; i < n_inputs; i++) {
        outputs[i] = evalformula_i(formula, inputs[i]);
    }

    printf("A B C D E F -> ANS | ENC\n");
    for(i = 0; i < n_inputs; i++) {
        int encoding_l = 0;
        int encoding_b = 0;
        for(j = 0; j < n_atoms; j++) {
            printf("%d ", inputs[i][j]);
            encoding_l |= inputs[i][j]<<j;
            encoding_b |= inputs[i][j]<<(n_atoms-1-j);
        }
        printf("-> %d   | %d - %d\n", outputs[i], encoding_l, encoding_b);
    }

    return 0;
}

// TODO: nowhere near finished... implement a repeating thing through static
// vars or a struct to pass that keeps a tally on where to stop... or just pass
// an arg for that, although that reuqires more effort on the calliung function
// and less abstraction
// TODO: maybe change *c to a static var?
int *tf_medial(int formula[], int *c)
{
    int **influence = opinfluence(formula); /* this is very useful! */
    int n_f = flength(formula);
    int i;

    //printformula(formula);
    
    /* start over if index is out of bounds */
    if(*c < 0 || *c >= n_f) *c = n_f-1;
    for(i = *c; i >= 0; i--) {
        /*index formula
         * 0    A
         * 1      B
         * 2    &
         * 3      C
         * 4        D
         * 5      &
         * 6    |
         */
        int indices[7];
        indices[6] = i; /* location of | */
        if(formula[i] != F_OP_OR) continue; /* | */
        indices[5] = i-1; /* location of 'first' & */
        if(formula[indices[5]] != F_OP_AND) continue; /* &1 */
        indices[2] = influence[indices[5]][0]-1; /* location of 'second' & */
        if(formula[indices[2]] != F_OP_AND) continue; /* &2 */
        indices[4] = influence[indices[5]][1]; /* D */
        indices[3] = influence[indices[5]][0]; /* C */
        indices[1] = influence[indices[2]][1]; /* B */
        indices[0] = influence[indices[2]][0]; /* A */

        formula[indices[5]] = F_OP_OR;
        formula[indices[2]] = F_OP_OR;
        formula[indices[6]] = F_OP_AND;

        /* rearrange/transform the formula! */
        int *newformula = malloc((n_f+1)*sizeof(int));
        int *n = newformula;
        /*printf("%d, %d, %d, %d, %d, %d, %d\n",
                indices[0],
                indices[1],
                indices[2],
                indices[3],
                indices[4],
                indices[5],
                indices[6]);*/
        memcpy(n, formula, sizeof(int)*indices[1]); // A
        n += indices[1];
        memcpy(n, &formula[indices[3]], sizeof(int)*(indices[4]-indices[3])); // C
        n += indices[4]-indices[3];
        memcpy(n, &formula[indices[2]], sizeof(int)*(indices[3]-indices[2])); // &
        n += indices[3]-indices[2];
        memcpy(n, &formula[indices[1]], sizeof(int)*(indices[2]-indices[1])); // B
        n += indices[2]-indices[1];
        memcpy(n, &formula[indices[4]], sizeof(int)*(n_f-indices[4])); // the rest
        newformula[n_f] = -1;

        formula[indices[5]] = F_OP_AND;
        formula[indices[2]] = F_OP_AND;
        formula[indices[6]] = F_OP_OR;

        /* on the next search, continue from one above this */
        *c = i-1;
        int j;
        for(j = 0; j < n_f; j++) free(influence[i]);
        free(influence);
        return newformula;
    }

    /* did not find any */
    *c = -1;
    return NULL;
}

int *tf_mix(int formula[], int *c)
{
    int n_f = flength(formula);
    int i;
    
    /* start over if index is out of bounds */
    if(*c < 0 || *c >= n_f) *c = n_f-1;
    for(i = *c; i >= 0; i--) {
        if(formula[i] != F_OP_AND) continue;

        int *newformula = malloc((n_f+1)*sizeof(int));
        memcpy(newformula, formula, (n_f+1)*sizeof(int));
        newformula[i] = F_OP_OR; /* just do this! */

        /* on the next search, continue from one above this */
        *c = i-1;
        return newformula;
    }

    /* did not find any */
    *c = -1;
    return NULL;
}

int *tf_shiftL(int formula[], int *c, int *t)
{
    int n_f = flength(formula);
    int i;
    int top = -1;

    if(*c < 0 || *c >= n_f) *c = n_f-1;
    if(*t < 0) *t = n_f; /* just so that this has no effect for now... */
    for(i = *c-1; i >= 0 && i < *t; i--) {
        // TODO: Check depth!
        if(formula[i] < 0 && formula[i] != formula[*c]) break;
        top = i;
    }

    if(top == -1) {
        /* we found nothing! */
        *c = -1;
        return NULL;
    }

    for(i = top; i < *c-1; i++) {
        if(formula[i-1] < 0) continue;
        swap(i,i-1); // TODO: this ismeaningless
        top = i;
    }

}

void printformula(int formula[])
{
    if(formula == NULL) { printf("nope\n"); return; }
    //printf("printing... \n");
    int i;
    for(i = 0; i < flength(formula); i++) printf("%d,", formula[i]);
    printf("\b\n");
}

int everything(int formula[], int counter)
{
    if(formula == NULL) return 0;

    //printformula(formula);
    /* apply every possible transformation every step of the way */
    int i;

    //for(i=0;i<counter;i++) printf("  ");

    int *newformula = NULL;
    int c = -1;

    do {
        everything(newformula, counter+1);
        free(newformula);
        newformula = tf_medial(formula, &c);
        if(newformula != NULL) printf("non-NULL medial result!\n");
    } while(newformula != NULL);

    //for(i=0;i<counter;i++) printf("  ");
    c = -1;
    do {
        everything(newformula, counter+1);
        free(newformula);
        newformula = tf_mix(formula, &c);
    } while(newformula != NULL);

    return 0;
}

unsigned long int genall(int n_v, int depth)
{
    //int i;
    //int varbag[n_v];
    //for(i = 0; i < n_v; i++) varbag[i] = i; /* populate it */

    unsigned long int n = 0;
    int bagsize = n_v;
    if(bagsize == 0 && depth == 1) return 1;
    // add a variable
    if(bagsize >= 1) n += bagsize*genall(bagsize-1, depth+1);
    // add an and/or
    if(depth > 1) n += 2*genall(bagsize, depth-1);

    //printf("bagsize=%d -- n=%d -- depth=%d\n", bagsize, n, depth);
    return n;
}


int main(int argc, char *argv[])
{
    //printf("int = %lu, int32_t = %ld, uint32_t = %d\n", sizeof(int), sizeof(int32_t), sizeof(uint32_t));
    //int f[] = {0,1,F_OP_OR,F_OP_END};
    //int f1[] = {0, 1, F_OP_AND, F_OP_END};
    //int f2[] = {0, F_OP_NOT, 1, F_OP_NOT, F_OP_OR, F_OP_NOT, -10, F_OP_END};

    //int inputs[4][3] = { {1,0,-1}, {1,1,-1}, {0,1,-1}, {0,0,-1}};
    //int inputs2[16][5];

    //int f[] = {0, F_OP_NOT, 1, F_OP_AND, 2, 3, 4, 5, F_OP_AND, F_OP_AND, F_OP_OR, F_OP_OR, F_OP_END};
    int f[] = {0, F_OP_NOT, 1, F_OP_AND, 2, 3, 4, F_OP_NEW, 5, F_OP_AND, 6, 7, 8, F_OP_AND, F_OP_AND, F_OP_OR, F_OP_OR, F_OP_END};
    int f2[] = {0, 1, F_OP_AND, 2, 3, F_OP_AND, F_OP_OR, 4, 5, F_OP_AND, 6, 7, F_OP_AND, F_OP_OR, F_OP_AND, F_OP_END};
    int f3[] = {0,1,F_OP_AND,2,3,F_OP_AND,F_OP_OR,F_OP_END};
    int f4[] = {0,1,2,3,4,5,6,7,8,9,F_OP_AND,F_OP_AND,F_OP_AND,F_OP_AND,F_OP_AND,F_OP_AND,F_OP_OR,F_OP_OR,F_OP_OR,F_OP_END};
    int c = -1;
    int c2 = -1;
    int c3 = -1;
    /*printformula(f2);
    printformula(tf_medial(f2, &c));
    printformula(tf_medial(tf_medial(f2, &c2), &c));*/
    everything(f4, 1);
    /*printf("2: %lu\n", genall(2,0));
    printf("3: %lu\n", genall(3,0));
    printf("4: %lu\n", genall(4,0));
    printf("5: %lu\n", genall(5,0));
    printf("6: %lu\n", genall(6,0));
    printf("7: %lu\n", genall(7,0));
    printf("8: %lu\n", genall(8,0));*/
    //int **influence = opinfluence(f);
    int i, j;
    /*for(i = 0; i < flength(f); i++) {
        if(influence[i] == NULL) { printf("operator %d: NULL\n", f[i]); }
        else if(f[i] < 0) {
            printf("operator %d (index %d): ranges: ", f[i], i);
            for(j = 0; j < F_OPERATOR_ARITY[f[i]*-1]; j++)
                printf("%d, ", influence[i][j]);
            printf("\n");
        }
        else {
            printf("what\n");
        }
    }*/
//    printformula(f);

//    int c = -1;
//    printformula(tf_medial(f, &c));
    //evalformula(f);

    //int i, j, k;
    //memset(inputs, 0, 4*3*sizeof(int));
    //for(i = 0; i < 4; i++) inputs[i][2] = F_OP_END; /* terminate inputs */
    //for(j = 0; j < 4; j++) { /* for every atom */
        // [1][0] = 1
        // [3][0] = 1
        // [5][0] = 1
        // [7][0] = 1
        // ----------
        // [3][1] = 1
        // [2][1] = 1
        // [7][1] = 1
        // [6][1] = 1
        // ----------
        // [7][2] = 1
        // [6][2] = 1
        // [5][2] = 1
        // [4][2] = 1
    /*    double inc = pow(2,j+1);
        for(i = inc; i <= 4; i+=inc) {
            for(k = 1; k <= inc/2; k++) {
                inputs[i-k][1-j] = 1;
                printf("[%d][%d] = 1\n", i-k, 1-j);
            }
        }
    }

    int output1[4];
    int output2[4];
    for(i = 0; i < 4; i++) {
        output1[i] = evalformula_i(f1, inputs[i]);
        output2[i] = evalformula_i(f2, inputs[i]);
    }

    printf("f1:\n");
    for(j = 0; j < 4; j++) printf("%d ", output1[j]);
    printf("\n");

    printf("f2:\n");
    for(j = 0; j < 4; j++) printf("%d ", output2[j]);
    printf("\n");
    printf("-------------\n");

    memset(inputs2, 0, 16*5*sizeof(int));*/
    //for(i = 0; i < 16; i++) inputs[i][4] = F_OP_END; /* terminate inputs */
    //for(j = 0; j < 4; j++) { /* for every atom */
        // [1][0] = 1
        // [3][0] = 1
        // [5][0] = 1
        // [7][0] = 1
        // ----------
        // [3][1] = 1
        // [2][1] = 1
        // [7][1] = 1
        // [6][1] = 1
        // ----------
        // [7][2] = 1
        // [6][2] = 1
        // [5][2] = 1
        // [4][2] = 1
       /* double inc = pow(2,j+1);
        for(i = inc; i <= 16; i+=inc) {
            for(k = 1; k <= inc/2; k++) {
                inputs2[i-k][3-j] = 1;
                printf("[%d][%d] = 1\n", i-k, 3-j);
            }
        }
    }

    for(i = 0; i < 16; i++) {
        for(j = 0; j < 4; j++) printf("%d ", inputs2[i][j]);
        printf("\n");
    }

    printf("%d\n", evalformula_i(f,inputs[0]));
    printf("%d\n", evalformula_i(f,inputs[1]));
    printf("%d\n", evalformula_i(f,inputs[2]));
    printf("%d\n", evalformula_i(f,inputs[3]));*/
    return 0;
}
