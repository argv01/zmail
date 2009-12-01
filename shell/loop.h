#ifndef SHELL_LOOP_H
# define SHELL_LOOP_H

#include <general.h>
#include <dynstr.h>

struct mgroup;

extern void zm_loop P((void));
extern void add_history P((int, char **));
/* build a command vector (argv) */
extern char **make_command P((const char *, register char ***, int *));
extern int zm_command P((int, char **, struct mgroup *));
extern int alias_stuff P((struct dynstr *, int, char **));
extern char *alias_expand P((char *));
extern char *hist_expand P((const char *, register char **, int *));
/* test or evaluate internal variables */
extern char *check_internal P((char *));
extern int varexp P((struct expand *));
/* evaluate a variable, even if it may be internal */
extern char *get_var_value P((char *));
extern int dyn_filename_expand P((struct dynstr *));
extern int dyn_variable_expand P((struct dynstr *));
extern int variable_expand P((char *));
/* given a string, make a vector */
extern char **mk_argv P((const char *, int *, int));
extern char *reference_hist P((const char *, struct dynstr *, char **));
extern char *hist_from_str P((char *, int *));
extern int disp_hist P((int, char **));
extern void init_history P((int));

#endif /* SHELL_LOOP_H */
