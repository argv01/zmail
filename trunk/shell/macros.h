#ifndef INCLUDE_SHELL_MACROS_H
#define INCLUDE_SHELL_MACROS_H


#include <general.h>

extern struct cmd_map *mac_stack, *mac_hide;

/* set up a string as a macro */
int mac_push P((register const char *str));
int mac_queue P((register char *str));
void mac_pop P((void));
void mac_flush P((void));
int mac_pending P((void));
int get_mac_input P((int newline));
int m_getchar P((void));
void m_ungetc P((int c));
int read_long_cmd P((char *buf));
int reserved_cmd P((char *buf, int do_exec));
int check_mac_bindings P((char *buf));


#endif /* !INCLUDE_SHELL_MACROS_H */
