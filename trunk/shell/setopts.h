#include <general.h>
#include <stdio.h>

struct Variable;
struct mgroup;
struct options;
struct thread_hashtab;
struct zmFunction;

struct thread_hashtab *optlist2tht P((struct options **list));
struct options *optlist_fetch P((struct options **list, const char *str));
void tht_thread P((struct thread_hashtab *tht, struct options **list));
void optlist_sort P((struct options **list));
int reset_state P((struct options *tmp, struct options **list));
int add_option P((struct options **list, const char *const *argv));
int set_var P((const char *var, const char *eq, const char *val));
int set_int_var P((char *var, char *eq, long val));
int user_set P ((struct options **, const char * const *));
int user_unset P((struct options **list, char *varname));
int user_settable P((struct options **list, const char *varname, struct Variable **varptr));
int set_env P((char *var, char *val));
int unset_env P((char *var));
char *zm_set P((register struct options **list, register const char *str));
int un_set P((struct options **list, const char *p));
int set P((int argc, char **argv, struct mgroup *list));
int zm_readonly P((int argc, char **argv, struct mgroup *list));
int var_mark_readonly P((const char *name));
int alts P((int argc, VPTR arg));
int save_opts P((int cnt, char **argv));
struct options **name2optlist P((char ***argvp));
int zm_alias P((int argc, char **argv));
int zm_unalias P((int argc, char **argv));
int bool_option P((const char *value, const char *field));
void stow_state P((void));
void cache_funct P((struct zmFunction **list, struct zmFunction *fun));
int funct_modified P((struct zmFunction *fun, struct zmFunction **list));
void clean_funct P((struct zmFunction **funlist, char *command, FILE *fp));
void add_var_callbacks P((void));
