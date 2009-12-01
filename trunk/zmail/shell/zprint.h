#ifndef _PRINT_H_
#define _PRINT_H_

#include "general.h"
#include "osconfig.h"

void clr_bot_line P((void));

#if defined(GUI) || defined(CURSES)
extern void  print	  VP((const char *, ...));
#else /* !GUI && !CURSES */
#include <stdio.h>
#define print (void) printf
#endif /* !GUI && !CURSES */

extern void wprint_status VP((const char *, ...));
extern char *zmVaStr	  VP((const char *, ...));

#endif
