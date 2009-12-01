/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = 
"Copyright (c) 1990 Regents of the University of California.\n\
All rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_msg.c	2.1  2.1 3/18/91";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STDARG_H
pop_msg(POP *p, int stat, char *format, ...)
#else
pop_msg(va_alist)
    va_dcl
#endif
{
    va_list ap;
#ifndef HAVE_STDARG_H
    POP *p;
    int stat;
    char *format;
#endif
    FILE *output_fp;
    char msgbuf[MAXLINELEN];	/* This is bogus but what can you do? */
    FILE *trace_fp;
    register char *mp;

#ifdef HAVE_STDARG_H
    va_start(ap, format);
#else
    va_start(ap);
    p = va_arg(ap, POP *);
    stat = va_arg(ap, int);
    format = va_arg(ap, char *);
#endif /* HAVE_STDARG_H */

    /* Figure the status code and send it to the client. */
    output_fp = p->output;
    fputs(stat == POP_SUCCESS ? POP_OK : POP_ERR, output_fp);
    putc(' ', output_fp);

    /* Format the message. */
    if (format) {
#ifdef HAVE_VSPRINTF
	vsprintf(msgbuf, format, ap);
#else
  {
    int ap0, ap1, ap2, ap3, ap4;
    ap0 = va_arg(ap, int);
    ap1 = va_arg(ap, int);
    ap2 = va_arg(ap, int);
    ap3 = va_arg(ap, int);
    ap4 = va_arg(ap, int);
    sprintf(msgbuf, format, ap0, ap1, ap2, ap3, ap4);
  }
#endif /* HAVE_VSPRINTF */

	/* Send the message to the client. */
	fputs(msgbuf, output_fp);
    }

    /* Send a CRLF and flush. */
    putc('\r', output_fp);
    putc('\n', output_fp);
    fflush(output_fp);

    /* Log the message if it's an error or debugging is turned on. */
    trace_fp = p->trace;
    if (trace_fp != NULL &&
	(stat != POP_SUCCESS || (p->debug & DEBUG_COMMANDS) != 0))
    {
	fputs(p->trace_prefix, trace_fp);
	if (format) {
	    fputs(stat == POP_SUCCESS ? "Sent OK: \"" : "Sent error: \"",
		  trace_fp);
	    fputs(msgbuf, trace_fp);
	    fputs("\"\n", trace_fp);
	} else {
	    fputs(stat == POP_SUCCESS ? "Sent OK\n" : "Sent error\n",
		  trace_fp);
	}
	fflush(trace_fp);
    }

    va_end(ap);
    return stat;
}
