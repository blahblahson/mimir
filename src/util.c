#include "util.h"

int factorial(unsigned int n)
{
    return n < 2 ? 1 : n*factorial(n-1);
}
