#ifndef INCLUDE_SHELL_ARITH_H
#define INCLUDE_SHELL_ARITH_H


#include <general.h>


/*
 * Uncomment one of the following pairs of lines (either way will work)
 */
#define EXPR_TYPE long
#define EXPR_FMT "%ld"

/* #define EXPR_TYPE double */
/* #define EXPR_FMT "%lg" */


#define EXPR_ERRBUFSIZ 80


int zm_expr_evaluate P((int argc, char **argv, EXPR_TYPE *val, char errbuf[EXPR_ERRBUFSIZ]));
int zm_calc P((int argc, char **argv));
int zm_set_arith P((int argc, char **argv));


#endif /* INCLUDE_SHELL_ARITH_H */
