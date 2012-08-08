#ifndef _MAIN_H_
#define _MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

/* util */
#define max(a,b) (a>b?a:b)
#define min(a,b) (a<b?a:b)

/* available operators
 * NOTE: the type enum F_OPERATOR is rarely if ever used, and int is used
 * instead to allow for inclusion of atoms (all nonzero values) */
enum F_OPERATOR {
    F_OP_END    = -1, /* end of formula */
    F_OP_AND    = -2, /* AND, & */
    F_OP_OR     = -3, /* OR, | */
    F_OP_NOT    = -4  /* NOT, ~ -- NOTE: not currently used */
,    F_OP_NEW    = -5
};

/* number of operands */
/*enum F_OPERATOR_ARITY {
    F_ARITY_END  = 0,
    F_ARITY_AND  = 2,
    F_ARITY_OR   = 2,
    F_ARITY_NOT  = 1
};*/

struct t_range {
    int start;
    int stop;
};

#endif /* _MAIN_H_ */
