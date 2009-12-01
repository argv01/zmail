#ifndef INCLUDE_POPENV_H
#define INCLUDE_POPENV_H
#ifdef __cplusplus
extern "C" {
#endif /* C++ */


#include "osconfig.h"
#include <general.h>
#include <stdio.h>

int popenve	P((FILE **, FILE **, FILE **, const char *, char **, char **));
int popenv	P((FILE **, FILE **, FILE **, const char *, char **));
int popenvp	P((FILE **, FILE **, FILE **, const char *, char **));
int popensh	P((FILE **, FILE **, FILE **, const char *));
int popencsh	P((FILE **, FILE **, FILE **, const char *));
int popencsh_f	P((FILE **, FILE **, FILE **, const char *));
int pclosev	P((int));

int popenle VP((FILE **, FILE **, FILE **, const char *, ...));
int popenl  VP((FILE **, FILE **, FILE **, const char *, ...));
int popenlp VP((FILE **, FILE **, FILE **, const char *, ...));

#ifdef __cplusplus
}
#endif /* C++ */
#endif /* !INCLUDE_POPENV_H */
