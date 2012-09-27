#ifndef _LOGIC_H_
#define _LOGIC_H_

#include <stdint.h>

int eval(int *, struct range_t *, uint16_t);
int sound(int *, int *);
int trivial(int *, int *);
int validinputs(int *, struct range_t *);

int prove(int *, int *, int);

#endif /* _LOGIC_H_ */
