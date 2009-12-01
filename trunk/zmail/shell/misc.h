#ifndef INCLUDE_SHELL_MISC_H
#define INCLUDE_SHELL_MISC_H


#include <general.h>

struct mgroup;
struct zmCallback;

void make_widget_name P((register char *label, register char *ret));
int print_help P((int argc, char **argv));
FILE *seek_help P((const char *str, const char *file, int complain, char *first_str));
int help P((int flags, GENERIC_POINTER_TYPE *str, GENERIC_POINTER_TYPE *file));
int cmd_line P((char buf[], struct mgroup *list));
int edit_file P((const char *file, const char *editor, int noisy));
void paint_title P((char *buf));
int check_flags P((unsigned long flags));
struct zmCallback *ZmCallbackAdd P((const char *name, int type,
				    void (*callback)(), VPTR data));
void ZmCallbackCallAll P((const char *name, int type, int event, VPTR xdata));
void ZmCallbackRemove P((struct zmCallback *));
void bell P((void));


#endif /* !INCLUDE_SHELL_MISC_H */
