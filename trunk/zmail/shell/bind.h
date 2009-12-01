#ifndef INCLUDE_SHELL_BIND_H
#define INCLUDE_SHELL_BIND_H


#include <general.h>
#include "c_bind.h"

struct mgroup;

#ifdef WIN16
extern struct cmd_map __far map_func_names[];
#else
extern struct cmd_map map_func_names[];
#endif

extern void init_bindings P((void));
/* bind strings to functions or macros */
extern int bind_it P((int, char **, struct mgroup *));

#endif /* !INCLUDE_SHELL_BIND_H */
