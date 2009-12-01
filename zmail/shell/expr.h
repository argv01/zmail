#ifndef INCLUDE_SHELL_EXPR_H
#define INCLUDE_SHELL_EXPR_H


#include <general.h>

/* parse a string converting to a "range" of numbers */
char *zm_range P((register const char *p, struct mgroup *list1));
char *list_to_str P((struct mgroup *list));
char *str_to_list P((struct mgroup *list, const char *str));
char *eval_expr P((register const char *p, struct mgroup *new_list));
/* string/integer comparisons for "if" in init files */
int eq_to P((char *lhs, char *rhs));
int lthan P((char *lhs, char *rhs));
int lt_or_eq P((char *lhs, char *rhs));
int gthan P((char *lhs, char *rhs));
int gt_or_eq P((char *lhs, char *rhs));
int zm_match P((int, char **, struct mgroup *));


#endif /* !INCLUDE_SHELL_EXPR_H */
