#ifndef INCLUDE_SHELL_MAIN_H
#define INCLUDE_SHELL_MAIN_H


#include <general.h>

void shell_init P((int argc, char *argv[]));
int zm_hostinfo P((void));
int zm_version P((void));
void set_cwd P((void));
int verify_malloc P((int argc, char *argv[]));


#endif /* INCLUDE_SHELL_MAIN_H */
