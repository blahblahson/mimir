#ifndef _MIM_H_
#define _MIM_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

/* util */
#define max(a,b) (a>b?a:b)
#define min(a,b) (a<b?a:b)

//#define partition next

/* available operators
 * NOTE: the type enum F_OPERATOR is rarely if ever used, and int is used
 * instead to allow for inclusion of atoms (all nonzero values) */
enum OPERATOR {
    OP_FIN    = -1, /* end of formula */
    OP_CLOSE  = -2, /* end of an operator's scope */
    OP_AND    = -3, /* AND, & */
    OP_OR     = -4, /* OR, | */
    OP_NOT    = -5  /* NOT, ~ -- NOTE: not currently used */
};

/* values greater equal to ATOMLIM are considered atoms */
#define ATOMLIM 0

/* inclusive range of indices where top <= bot.
 * if top == bot then we are looking at an atom or terminating operator (or
 * some bastard abuse of the functionality of range_t in functions like yank,
 * where top/bot don't really mean anything tangible in a general sense)
 */
struct range_t {
    int top;
    int bot;
};

int length(const int *);
int length_l(void **);

int scope(int *, int);
int parent(int *, int);
struct range_t *range(int *, int);
struct range_t **arguments(int *, struct range_t *);
struct range_t **arguments_op(int *, struct range_t *, enum OPERATOR);
struct range_t **arguments_ex(int *, struct range_t *, struct range_t *);

int *pair(int, int *);
int partition(int, int *, int *);

int *splice(enum OPERATOR, int, ...);
int *splice_l(enum OPERATOR, const int *, struct range_t **);
int *yank(int *, struct range_t *);
int *paste(int *, struct range_t *, int *);

int **r_medial(int *);
int **r_switch(int *);
int **r_mix(int *);

int equiv(int *, struct range_t *, int *, struct range_t *);

int **genbf(int);

#endif /* _MIM_H_ */
