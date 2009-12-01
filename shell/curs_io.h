#ifndef INCLUDE_SHELL_CURS_IO_H
#define INCLUDE_SHELL_CURS_IO_H


#include <general.h>

struct cmd_map;
struct dynstr;

void tty_settings P((void));
int dyn_Getstr P((struct dynstr *dstr, char *pmpt));
int Getstr P((char *pmpt, char *string, int length, int offset));
int check_map P((int c, struct cmd_map *map_list));
int line_wrap P((struct dynstr *dstr));
int errbell P((int ret));
int completion P((struct dynstr *dstr, int showlist, int ignore));
int fignore P((int argc, char ***argvp));


#endif /* !INCLUDE_SHELL_CURS_IO_H */
