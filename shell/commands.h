#ifndef INCLUDE_SHELL_COMMANDS_H
#define INCLUDE_SHELL_COMMANDS_H


#include <config.h>
#include <config/features.h>
#include <general.h>

struct dynstr;
struct mgroup;

/* commands.c 22/12/94 14.12.48 */
extern int toggle_debug P((int, char **));
extern int read_msg P((int, char **, struct mgroup *));
extern unsigned long display_flags P((void));
extern int zm_view P((int, char **, struct mgroup *));
extern int preserve P((int, char **, struct mgroup *));
extern int save_msg P((int, char **, struct mgroup *));
extern int respond P((int, char **, struct mgroup *));
extern int has_root_prefix P((char *));
extern int cd P((int, char **));
extern int zm_chroot P((int, char **));
extern int zm_quit P((int, char **));
extern int zm_delete P((int, char **, struct mgroup *));
extern int zm_undelete P((int, char **, struct mgroup *));
extern int zm_from P((int, char **, struct mgroup *));
extern int ls P((int, char **));
extern int sh P((int, char **));
extern void GetWineditor P((struct dynstr *));
extern void GetWindowShell P((struct dynstr *));
extern int stop P((int, char **));
extern int Setenv P((int, char **));
extern int Unsetenv P((int, char **));
extern int Printenv P((int, char **));
extern int my_stty P((int, char **));
extern int edit_msg P((int, char **, struct mgroup *));
extern int pipe_msg P((int, char **, struct mgroup *));
extern int zm_echo P((int, char **));
extern int eval_cmd P((int, char **, struct mgroup *));
extern int await P((int, char **, struct mgroup *));
extern int mark_msg P((int, char **, struct mgroup *));
extern int zm_priority P((int, char **, struct mgroup *));
extern int zm_ask P((int, char **, struct mgroup *));
extern int dyn_choose_one P((struct dynstr *, const char *, const char *,
			     char **, int, unsigned long));
extern int choose_one P((char *, char *, const char *, char **,
			 int, unsigned long));
extern int zm_trap P((int, char **, struct mgroup *));
extern int zm_dialog P((int, char **, struct mgroup *));
extern int zm_rm P((int, char **, struct mgroup *));
extern int zm_multikey P((int, char **, struct mgroup *));
extern int zm_unmultikey P((int, char **, struct mgroup *));
extern int zm_bindkey P((int, char **, struct mgroup *));
extern int screencmd P((int, char **, struct mgroup *));

#ifdef ZM_CHILD_MANAGER
extern int child_debug P((int, char **));
#endif /* ZM_CHILD_MANAGER */

#ifdef MALLOC_UTIL
extern int call_mutil_info P((int, char **))
#endif /* MALLOC_UTIL */

#ifdef MALLOC_TRACE
extern int call_malloc_trace_info P((int, char **));
#endif /* MALLOC_TRACE */


#endif /* !INCLUDE_SHELL_COMMANDS_H */
