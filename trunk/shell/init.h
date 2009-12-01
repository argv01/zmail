#ifndef _INIT_H_
#define _INIT_H_

#include <general.h>
#include "zmsource.h"

extern Source *stdSource, *inSource;

void init_user P((char *));
void init_host P((char *));
int init_lib P((const char *));
int init P((void));
char **get_ourname P((void));
char *zmRcFileName P((int, int, int));
int source P((int, char **, struct mgroup *));
int src_Source P((char *, Source *));
int src_parse P((char *, Source *, int, int, int *, struct mgroup *));

#endif
