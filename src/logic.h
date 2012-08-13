#ifndef _LOGIC_H_
#define _LOGIC_H_

int eval(int *, struct range_t *, uint16_t);
int implies(int *, int *);
int trivial(int *, int *);
int validinputs(int *, struct range_t *);

int quickprove(int *, int *, int);

#endif /* _LOGIC_H_ */
