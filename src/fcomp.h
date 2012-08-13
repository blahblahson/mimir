#ifndef _FCOMP_H_
#define _FCOMP_H_

#include "formula.h"

struct range_t;

int equiv(int *, struct range_t *, int *, struct range_t *);
int find(int **, int, int *);

#endif /* _FCOMP_H_ */
