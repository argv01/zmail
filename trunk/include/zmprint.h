/* zmprint.h	Copyright 1995 Z-Code Software, a Divison of NCD */

#ifndef _ZMPRINT_H_
#define _ZMPRINT_H_

#include "error.h"	/* Prototypes error() */
#include "zccmac.h"	/* Defines max() */

#if !defined(GUI) && !defined(CURSES)

#define wprint		  (void) printf
#define print_more	  (void) printf

#endif /* !GUI && !CURSES */

#ifdef GUI
/* stdout may be closed */
#define printf wprint
#else /* !GUI */
#ifdef CURSES
#define wprint	(void) printf
#endif /* CURSES */
#endif /* GUI */

#if defined(CURSES) || defined(GUI)
#define print_more	  turnon(glob_flags, CONT_PRNT), print

/* Prototypes that used to be here; now moved down into other headers. */
#include "zprint.h"
 
#endif /* CURSES || GUI */

#define MAXPRINTLEN	max(4096,BUFSIZ)

#endif  /* !_ZMPRINT_H_ */
