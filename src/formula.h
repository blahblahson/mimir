#ifndef _FORMULA_H_
#define _FORMULA_H_

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
int scope(int *, int);
int parent(int *, int);
int parent_deep(int *, int);
int atomcount(int *, struct range_t *);
struct range_t *range(int *, int);
struct range_t **arguments(int *, struct range_t *);
struct range_t **arguments_op(int *, struct range_t *, enum OPERATOR);
struct range_t **arguments_ex(int *, struct range_t *, struct range_t *);

int *splice(enum OPERATOR, int, ...);
int *splice_l(enum OPERATOR, const int *, struct range_t **);
int *yank(int *, struct range_t *);
int *shove(int *, struct range_t *, int *);
int *shove_nf(int *, struct range_t *, int *);

int *sanitize(int *, struct range_t *);

int **genbf(int nvar);

#endif /* _FORMULA_H_ */
