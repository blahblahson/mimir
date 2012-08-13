#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "set.h"

/* this function is horrible to use, although the algorithm itself is good */
void permute(int n, int m, int ***perms_p, int **a_p)
{
    int i;
    int **perms = *perms_p;
    int *a = *a_p;

    if(perms == NULL) {
        /* number of permutations will always be n! */
        perms = malloc((factorial((unsigned int)n)+1)*sizeof(int *));
        *perms_p = perms;
        perms[0] = NULL;

        a = malloc(n*sizeof(int));
        for(i = 0; i < n; i++) a[i] = i;
        *a_p = a;

        m = n;
    }

    if(n == 1) {
        /* this is safe assuming that no other function fucks with permute()'s
         * variables */
        for(i = 0; ; i++) {
            if(perms[i] == NULL) {
                perms[i+1] = NULL;
                perms[i] = malloc(m*sizeof(int));
                memcpy(perms[i], a, m*sizeof(int));

                break;
            }
        }
    }
    else {
        for(i = 0; i < n; i++) {
            permute(n-1, m, &perms, &a);

            int swap;
            if(n%2 == 1) {
                swap = a[0];
                a[0] = a[n-1];
                a[n-1] = swap;
            }
            else {
                swap = a[i];
                a[i] = a[n-1];
                a[n-1] = swap;
            }
        }
    }
}

/* n - number of objects to find all pairs of
 * p - the last pair, or NULL if we are starting afresh. this is the same as
 *     the return value
 * return - a pair out of the n objects in the form of a size-2 array (NOT
 *          terminated or anything), successive identical calls then yielding
 *          subsequent unique pairs until all have been exhausted, at which
 *          point NULL is returned
 * NOTE: memory is handled entirely by this function. the correct way to call
 * it initially is as follows:
 *      int *p = NULL;
 *      while(pair(n, p) != NULL) { do stuff with p here }
 *      // no need to free p at any point
 */
// TODO: to replace permute2, which is a dumb name
int *pair(int n, int *p)
{
    /* you clearly can't take two balls out of a bag of only one ball */
    if(n < 2) return NULL;
    
    if(p == NULL) {
        p = malloc(2*sizeof(int));
        p[0] = 0;
        p[1] = 1;
        return p;
    }

    if(++p[1] >= n)
        p[1] = (++p[0])+1;

    if(p[1] >= n) {
        free(p);
        return NULL;
    }

    return p;
}

/* n - size of the set to partition
 * s - array of size n, which will be modified to describe the new partitioning
 * m - array of size n, which is used internally and of no real interest
 * return - the size (number of disjoint subsets) of the new partition (which
 *          has been encoded in s)
 * NOTE: similarly to pair(), this does all its own memory management, so do:
 *      int *s, *m;
 *      s = m = NULL;
 *      while((x = partition(n, s, m)) > 0) { do stuff with s }
 *      // no need to free s, m
 */
// TODO: make it spit out indexes from 0 rather than 1, be consistent!!
int partition(int n, int *s, int *m)
{
    int i;

    if(s[0] < 0) {
        //s = malloc(n*sizeof(int));
        //m = malloc(n*sizeof(int));
        for(i = 0; i < n; i++) s[i] = m[i] = 1;
        //memset(s, 1, n*sizeof(int));
        //memset(m, 1, n*sizeof(int));

        return 1;
    }

    int ret = 1;
    i = 0;
    s[i]++;
    while(i < n-1 && s[i] > m[i]+1) {
        s[i] = 1;
        i++;
        s[i]++;
    }

    /* if i is has reached n-1 th element, then the last unique partitiong
     * has been found */
    if(i == n-1) {
        s[0] = -1;
        return 0;
    }

    /* because all the first i elements are now 1, s[i] (i + 1 th element)
     * is the largest. so we update max by copying it to all the first i
     * positions in m */
    int maximum = s[i];
    if(m[i] > maximum) maximum = m[i];
    for(i = i-1; i >= 0; --i) m[i] = maximum;

    ret = max(m[0],s[0]);

    /*printf("s: ");
    for (i = 0; i < n; ++i)
        printf("%d ", s[i]);
    printf("\nm: ");*/
    /*for (i = 0; i < n; ++i)
        printf("%d ", m[i]);
    printf("\n");*/
    //getchar();*/

    //while(m[0] < 4) if(!next(s, m, n)) return 0;
    return ret;
}
