/* zcsyssel.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _ZCSYSSEL_H_
#define _ZCSYSSEL_H_

/*
 * SCO defines struct timeval in both sys/time.h and sys/select.h.  Sigh.
 * Since sys/select.h is significantly smaller, pull it in here.
 */
#ifdef HAVE_SYS_SELECT_H
# ifndef ZC_INCLUDED_SYS_SELECT_H
#  define ZC_INCLUDED_SYS_SELECT_H
#  ifdef SCO
#   include <zctime.h> /* for struct timeval */
    /*
     * Code used internally for select system call.
     */
#   define IOC_SELECT	0xffff

    /*
     * Selectable conditions for select system call
     */
#   define	SELREAD		0x1
#   define	SELWRITE	0x2
#   define	SELEXCEPT	0x4
#  else
#   include <sys/select.h>
#  endif
# endif /* ZC_INCLUDED_SYS_SELECT_H */
#endif /* HAVE_SYS_SELECT_H */

#endif /* _ZCSYSSEL_H_ */
