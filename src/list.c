#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fcomp.h"
#include "list.h"
// TODO: stdio only needed for printf debugs, remove eventually!

/* list - any NULL-terminated list
 * size - the size of list elements
 * return - the number of elements, up to but not including the terminating
 *          NULL element
 * e.g. for a list: struct range_t **ranges
 *      to count the number of elements, we would call:
 *          length_l(ranges, sizeof(struct range_t *));
 */
int length_l(void **list)
{
    if(list == NULL) return 0;
    int i = -1;
    while(list[++i] != NULL) ;
    return i;
}
// to test:
// struct range_t a,b;
// struct range_t *x[3] = {&a,&b,NULL};
// length_l(x, sizeof(struct range_t *)) == 2

void free_l(void **list)
{
    /* this would happen anyway, but this makes it clear */
    if(list == NULL) return;

    int i, n = length_l(list);
    for(i = 0; i < n; i++) free(list[i]);
    free(list);
}

int **push_uniq(int **collection, int **list)
{
    int n_list = length_l((void **)list);
    if(n_list <= 0) return collection;
    int n_collection = length_l((void **)collection);
    if(n_collection <= 0) {
        /* this could cause issues but I think it's ok since this is quite an
         * internal function */
        collection = NULL;
        n_collection = 0;
    }

    collection = realloc(collection, (n_collection+n_list+1)*sizeof(int *));
    collection[n_collection] = NULL;

    int i;
    for(i = 0; i < n_list; i++) {
        if(find(collection, n_collection, list[i]) < 0)
            collection[n_collection++] = list[i];
        else
            free(list[i]);
    }

    collection[n_collection] = NULL;

    if(n_collection == 0) {
        free(collection);
        free(list);
        return NULL;
    }

    collection = realloc(collection, (n_collection+1)*sizeof(int *));
    fprintf(stderr, "%d\n", n_collection);

    return collection;
}
