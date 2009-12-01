#ifndef INCLUDE_MSGS_SORT_H
#define INCLUDE_MSGS_SORT_H


#include <general.h>

struct mgroup;


/* sort.c 21/12/94 16.20.56 */
int sort P((int argc, char *argv[], struct mgroup *list));
#ifdef MYQSORT
int qsort P((char *base, int len, int siz, int (*compar)()));
#endif /* MYQSORT */


#endif /* !INCLUDE_MSGS_SORT_H */
