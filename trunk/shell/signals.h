#ifndef INCLUDE_SHELL_SIGNALS_H
#define INCLUDE_SHELL_SIGNALS_H


#include <general.h>
#include <osconfig.h>

/* handle interrupts when we don't want to */
RETSIGTYPE intrpt P((int sig));
int handle_intrpt P((unsigned long flags, char *str, long percentage));
int set_user_handler P((int sig, char *handler));
#ifndef __cplusplus
/* catch user (keyboard) generated signals */
RETSIGTYPE catch P((int sig));
#endif /* !C++ */
/* handle job control signals */
RETSIGTYPE stop_start P((int));
void cleanup P((int sig));
/* handle runtime segfaults -- exit gracefully */
RETSIGTYPE bus_n_seg P((int sig));

/* 
The standard signal.h in MS's msvc 1.51 has these definitions 
defined only for non-windows applications. This could be a propblem!!
*/
#ifdef WIN16
#define SIGINT      2   /* Ctrl-C sequence */
#define SIGILL      4   /* illegal instruction - invalid function image */
#define SIGFPE      8   /* floating point exception */
#define SIGSEGV     11  /* segment violation */
#define SIGTERM     15  /* Software termination signal from kill */
#define SIGABRT     22  /* abnormal termination triggered by abort call */
#endif


#endif /* !INCLUDE_SHELL_SIGNALS_H */
