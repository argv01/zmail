/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = 
"Copyright (c) 1990 Regents of the University of California.\n\
All rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_log.c	2.1  2.1 3/18/91";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <sys/types.h>

#ifdef HAVE_STDARG_H
pop_log(POP *p, int stat, char *format, ...)
#else
pop_log(va_alist)
    va_dcl
#endif
{
    va_list ap;
#ifndef HAVE_STDARG_H
    POP *p;
    int stat;
    char *format;
#endif
    FILE *trace_fp;
    char msgbuf[MAXLINELEN];

#ifdef HAVE_STDARG_H
    va_start(ap, format);
#else
    va_start(ap);
    p = va_arg(ap, POP *);
    stat = va_arg(ap, int);
    format = va_arg(ap, char *);
#endif

#ifdef HAVE_VSPRINTF
    vsprintf(msgbuf, format, ap);
#else
  {
    int ap0, ap1, ap2, ap3, ap4, ap5;
    ap0 = va_arg(ap, int);
    ap1 = va_arg(ap, int);
    ap2 = va_arg(ap, int);
    ap3 = va_arg(ap, int);
    ap4 = va_arg(ap, int);
    ap5 = va_arg(ap, int);
    sprintf(msgbuf, format, ap0, ap1, ap2, ap3, ap4, ap5);
  }
#endif /* HAVE_VSPRINTF */

    va_end(ap);
    
    trace_fp = p->trace;
    if (trace_fp == NULL)
	syslog(stat, "%s", msgbuf);
    else
    {
	fputs(p->trace_prefix, trace_fp);
	fputs(msgbuf, trace_fp);
	putc('\n', trace_fp);
	fflush(trace_fp);
    }

    return stat;
}
