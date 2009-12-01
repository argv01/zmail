/*
 * Copyright 1994 by Z-Code Software Corp., an NCD company.
 */

#include "zyncd.h"
#include <sys/types.h>
#include <stdio.h>

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else
# include <varargs.h>
#endif /* HAVE_STDARG_H */


extern int zync_debug;
extern FILE *zync_trace_file;
extern pid_t zync_pid;

static char msgbuf[MAXLINELEN];


/* Callback for c-client debug telemetry */
void mm_dlog(string)
     char *string;
{
  if (zync_debug & DEBUG_CCLIENT) 
    zync_log(ZYNC_DEBUG, string);
}
     

/* Log a message to the trace file or via syslog() */
#ifdef HAVE_STDARG_H
void zync_log(int severity, char *format, ...)
{
  va_list ap;
  va_start(ap, format);
#else /* !HAVE_STDARG_H */
void mm_dlog(va_alist)
     va_dcl
{
  va_list ap;
  int severity;
  char *format;

  va_start(ap);
  severity = va_arg(ap, int);
  format = va_arg(ap, char *);
#endif /* !HAVE STDARG_H */

  va_end(ap);

#ifdef HAVE_VSPRINTF
  vsprintf(msgbuf, format, ap);
#else /* !HAVE VSPRINTF */
  sprintf(msgbuf, format, ((int *)ap)[0], ((int *)ap)[1], ((int *)ap)[2],
	  ((int *)ap)[3], ((int *)ap)[4], ((int *)ap)[5]);
#endif /* !HAVE_VSPRINTF */

  if (zync_trace_file) {
    fprintf(zync_trace_file, "[%d] %s%s\n", zyncd_pid, 
	    (severity == ZYNC_WARNING) ? "Warning: " : ""), msgbuf);
    fflush(zync_trace_file);
  } else 
    syslog(severity, "%s", msgbuf);
}

