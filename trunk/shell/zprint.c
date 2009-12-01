/* print.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "catalog.h"
#include "zprint.h"

#ifdef HAVE_STDARG_H
# include <stdarg.h>
#else /* HAVE_STDARG_H */
# ifndef va_dcl
#  include <varargs.h>
# endif
#endif

#ifdef AUDIO
#include "au.h"
#endif /* AUDIO */

#ifndef lint
static char	print_rcsid[] =
    "$Id: zprint.c,v 2.15 1998/12/07 23:50:10 schaefer Exp $";
#endif

#include <general.h>

/* Print an error or other warning message.  First argument is the reason
 * for the message, second is printf formatting, rest are to be formatted.
 * Bart: Fri Feb 19 14:52:46 PST 1993 -- never suppress '\n' -- gui_error()
 * is explicitly *appending* a '\n' when reason==Message, but this function
 * was suppressing it in that case.  Somebody got confused somewhere.
 */
void
#ifdef HAVE_STDARG_H
error (PromptReason reason, const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
error(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    char buf[MAXPRINTLEN];
    va_list args;
#ifndef HAVE_STDARG_H
    PromptReason reason;
    char *fmt;

    va_start(args);
    reason = (PromptReason) va_arg(args, int);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
    VSprintf(buf, fmt, args);
    va_end(args);

    if (reason == SysErrWarning || reason == SysErrFatal)
      if (errno != 0)
	sprintf(buf+strlen(buf), ": %s.", strerror(errno));
      else
	strcat(buf+strlen(buf), ".");

#ifdef AUDIO
    retrieve_and_play_sound(AuAction,
	reason == SysErrFatal || reason == ZmErrFatal ||
	    reason == UserErrFatal? "fatal" :
	reason == SysErrWarning || reason == ZmErrWarning ||
	    reason == UserErrWarning? "warning" :
	reason == QueryChoice || reason == QueryWarning?
	    "query" : "message");
#endif /* AUDIO */

#ifdef GUI
# ifndef _WINDOWS
    if (istool == 2)
	gui_error(reason, buf);
# else /* _WINDOWS */
    if (1)
	gui_error(reason, buf, NULL);
# endif /* _WINDOWS */
    else if (istool == 3) {
	fprintf(stderr, "%s%s\n",
	    (reason==ZmErrFatal || reason==ZmErrWarning)?
	    catgets( catalog, CAT_MSGS, 483, "Internal Error: " ) : "", buf);
    } else
#endif /* GUI */
    print("%s%s\n",
	(reason==ZmErrFatal || reason==ZmErrWarning)? catgets( catalog, CAT_MSGS, 483, "Internal Error: " ) : "",
	    buf);
    if (reason == SysErrFatal || reason == UserErrFatal)
	cleanup(-1);
    else if (reason == ZmErrFatal)
	cleanup(debug - 1); /* cause abort() if debugging is on */
}


void
#ifdef HAVE_STDARG_H
wprint_status(const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
wprint_status(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
#ifdef GUI
#ifdef MSDOS
    char msgbuf[BUFSIZ*2]; /* we're not getting huge strings */
#else /* MSDOS */
    char msgbuf[BUFSIZ]; /* we're not getting huge strings */
#endif /* MSDOS */
    va_list args;
#ifndef HAVE_STDARG_H
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
#ifdef HAVE_VPRINTF
    (void) vsprintf(msgbuf, fmt, args);
#else /* !HAVE_VPRINTF */
    {
	FILE foo;
	foo._cnt = BUFSIZ;
        foo._base = foo._ptr = msgbuf; /* may have to cast(unsigned char *) */
        foo._flag = _IOWRT+_IOSTRG;
        (void) _doprnt(fmt, args, &foo);
        *foo._ptr = '\0'; /* plant terminating null character */
    }
#endif /* HAVE_VPRINTF */
    va_end(args);
    gui_print_status(msgbuf);
#endif /* GUI */
}

#if defined(GUI) || defined(CURSES)
/*
 * print just like printf -- to a window, to curses, or to stdout.  Use vprintf
 * if available.  msgbuf is the buffer used to print into if necessary.
 * If you're running SUN3.2 or higher, the typecast (unsigned char *)msgbuf
 * (where indicated) otherwise, msgbuf is not typecast at all.
 */
void
#ifdef HAVE_STDARG_H
print (const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
print(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    char msgbuf[BUFSIZ * 2];
    va_list args;
    static int x; /* position on line saved for continued prints */
    char *p; /* same type as struct file _ptr,_buf in stdio.h */
#ifndef HAVE_STDARG_H
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */

#ifdef CURSES
    if (iscurses) {
	if (isoff(glob_flags, CONT_PRNT))
	    move(LINES-1, x = 0), refresh();
    }
#endif /* CURSES */
#if defined(MAC_OS) || defined(_WINDOWS)
    VSprintf(msgbuf, fmt, args);
    va_end(args);
    wprint("%s", msgbuf);
    return;
#endif /* MAC_OS || _WINDOWS */

    if (!iscurses && istool != 2) {
	VPrintf(fmt, args);
	(void) fflush(stdout);
    } else
	VSprintf(msgbuf, fmt, args);
    va_end(args);
    if (!iscurses && istool != 2)
	return;
    if (istool) {
	wprint("%s", msgbuf);
	return;
    }
#ifdef CURSES
    p = msgbuf;
    while (p = index(p, '\n'))
	*p = ' ';
    p = msgbuf;
    for (;;) {
	int len = COLS-1-x; /* space remaining at till the eol */
	/* don't wrap the line! Just print it and refresh() */
	printw("%-.*s", len, p), clrtoeol(), refresh();
	/* if length(p) (remainder of msgbuf) doesn't wrap, break loop */
	if ((x += strlen(p)) < COLS-1)
	    break;
	/* the next print will overwrite bottom line, so \n first */
	putchar('\n'), move(LINES-1, x = 0); /* reset x */
	/* move p forward the number of chars we were able to display */
	p += len;
	turnon(glob_flags, CNTD_CMD); /* display ...continue... prompt */
    }
    turnoff(glob_flags, CONT_PRNT);
    (void) fflush(stdout); /* some sys-v's aren't fflushing \n's */
    return;
#endif /* CURSES */
}

#if defined(CURSES) && !defined(PRINTW_WORKS) && !(defined(MSDOS) || defined(MAC_OS))

int
#ifdef HAVE_STDARG_H
printw (char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
printw(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    char msgbuf[MAXPRINTLEN];
    va_list args;
#ifndef HAVE_STDARG_H
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */

    VSprintf(msgbuf, fmt, args);
    return addstr(msgbuf);
}

#endif /* CURSES && !PRINTW_WORKS && !(MSDOS || MAC_OS) */

#endif /* GUI || CURSES */

#ifdef ZMVADEBUG
char **last_zmVaCaller;
char *zmVaCaller_file;
int zmVaCaller_line;

#undef zmVaStr
#define zmVaStr real_zmVaStr

void
real_zmVaReset()
{
    last_zmVaCaller = DUBL_NULL;
}
#endif /* ZMVADEBUG */

/* Generate a string from a format and arguments.  Intended for one-shot
 * usages, as in arguments of ZSTRDUP() or savestr().  As a convenience,
 * passing a NULL format will return the most recently generated string.
 */
char *
#ifdef HAVE_STDARG_H
zmVaStr(const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
zmVaStr(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    /* VSprintf uses sizeof(), so this can't be malloc'd */
    static char vaStr[max(MAXPATHLEN+BUFSIZ,MAXPRINTLEN)];
    va_list args;
#ifndef HAVE_STDARG_H
    const char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
#ifdef ZMVADEBUG
    if (fmt && last_zmVaCaller && &fmt > last_zmVaCaller)
	fprintf(stderr, catgets( catalog, CAT_SHELL, 601, "%s\nNoticed at %s: line %d\n" ),
	    catgets( catalog, CAT_SHELL, 602, "WARNING!  zmVaStr() called while previous caller still active!" ),
	    zmVaCaller_file, zmVaCaller_line);
    if (!last_zmVaCaller)
	zmVaReset();
    last_zmVaCaller = &fmt;
#endif /* ZMVADEBUG */

    if (fmt)
	VSprintf(vaStr, fmt, args);	
    va_end(args);

    return vaStr;
}

/* This is a nasty hack for debugging to the screen or to a file.
 * Set the environment (or local) variable DBFILE and use either
 * the -d command line option or the "debug 2" command and debug
 * output goes to the file.  Otherwise we do an incredibly wasteful
 * tangle of print calls to finally get the output where its going
 * (zmDebug calls print, print calls wprint/printw, they call ...).
 */
void
#ifdef HAVE_STDARG_H
zmDebug (char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
zmDebug(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    static FILE *dbFile;
    static char *dbName;
    char dbStr[MAXPRINTLEN];
    va_list args;
#ifndef HAVE_STDARG_H
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */

    if (fmt)
	VSprintf(dbStr, fmt, args);
    va_end(args);

    if (fmt = value_of("DBFILE")) {
	if (!dbFile || !dbName || strcmp(fmt, dbName)) {
	    if (dbFile) {
		(void) fclose(dbFile);
		dbFile = NULL_FILE;
	    }
	    ZSTRDUP(dbName, fmt);
	}
	if (dbFile || (dbFile = fopen(dbName, "a"))) {
	    (void) fputs(dbStr, dbFile);
	    (void) fflush(dbFile);
#ifdef MSDOS
	    _commit(fileno(dbFile));  /* fflush() isn't good enough on DOS */
#endif /* MSDOS */
	    return;
	}
    } else if (dbFile) {
	(void) fclose(dbFile);
	dbFile = NULL_FILE;
    }
    print("%s", dbStr);
}

/*
 * ask() -- a generalized routine that asks the user a question
 * and returns the Yes/No response.
 *
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
AskAnswer
#ifdef HAVE_STDARG_H
ask (AskAnswer dflt, const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS2*/
ask(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    char question[BUFSIZ];
    va_list args;
#ifdef CURSES
    char *menu[4];
    extern char *curses_menu_bar();
#endif /* CURSES */
#ifndef HAVE_STDARG_H
    const char *fmt;
    AskAnswer dflt;

    va_start(args);
    dflt = (AskAnswer)va_arg(args, int);
    fmt = va_arg(args, const char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
    VSprintf(question, fmt, args);
    va_end(args);

    turnoff(glob_flags, NOT_ASKED);
#ifdef GUI
    if (istool)
	return gui_ask(dflt, question);
#endif /* GUI */
    
#ifdef CURSES
    if (iscurses) {
	menu[0] = "yes";
	menu[2] = menu[3] = NULL;
	switch ((int)dflt) {
	    case AskYes:  case AskNo: case AskCancel:
#ifdef PARTIAL_SEND
	    case SendSplit:  case SendWhole: case SendCancel:
#endif /* PARTIAL_SEND */
		menu[2] = "cancel";
		/* Fall through */
	    case AskOk: case WarnYes: case WarnNo:
		menu[1] = "no";
	    when WarnOk: case WarnCancel:
		menu[1] = "cancel";
	}
	/* Move the cursor to the default choice */
	if (dflt == AskCancel)
	    Ungetstr("  ");
	else if (dflt != AskYes && dflt != WarnYes && dflt != WarnOk)
	    Ungetstr(" ");
    } else
#endif /* CURSES */
    switch ((int)dflt) {
	case AskYes:  case AskNo: case AskCancel:
#ifdef PARTIAL_SEND
	case SendSplit:  case SendWhole: case SendCancel:
#endif /* PARTIAL_SEND */
	    fmt = catgets( catalog, CAT_SHELL, 607, "%s (y/n/c) [%s] " );
	when AskOk: case WarnYes: case WarnNo:
	    fmt = catgets( catalog, CAT_SHELL, 608, "%s (y/n) [%s] " );
	when WarnOk: case WarnCancel:
	    fmt = catgets( catalog, CAT_SHELL, 609, "Warning: %s (y/c) [%s] " );
	otherwise:
	    fmt = catgets( catalog, CAT_SHELL, 610, "%s (y/n/c) " );
    }
    switch ((int)dflt) {
	case AskOk: case WarnYes: case WarnOk:
#ifdef PARTIAL_SEND
	case SendSplit:
#endif /* PARTIAL_SEND */
	    dflt = AskYes;
	when WarnNo:
#ifdef PARTIAL_SEND
	case SendWhole:
#endif /* PARTIAL_SEND */
	    dflt = AskNo;
	when WarnCancel:
#ifdef PARTIAL_SEND
	case SendCancel:
#endif /* PARTIAL_SEND */
	    dflt = AskCancel;
    }
    for (;;) {
	char buf[16];
	int ret;
#ifdef CURSES
	if (iscurses) {
	    if (fmt = curses_menu_bar(question, menu, DUBL_NULL, TRUE))
		strcpy(buf, fmt);
	    else
		return AskCancel;
	} else
#endif /* CURSES */
	{
	    zmVaStr(fmt, question,
		      dflt == AskYes ? catgets( catalog, CAT_SHELL, 611, "y" )
		    : dflt == AskNo  ? catgets( catalog, CAT_SHELL, 612, "n" )
		                     : catgets( catalog, CAT_SHELL, 613, "cancel" ));
	    if ((ret = Getstr(zmVaStr(NULL), buf, sizeof (buf), 0)) == 0)
		return dflt;
	    else if (ret == -1)
		return AskCancel;
	}
	/* Interpret the result in a manner that can be localized for multibyte encodings */
	if (zglob(buf, catgets(catalog, CAT_SHELL, 824, "[Yy]*")))
	    return AskYes;
	if (zglob(buf, catgets(catalog, CAT_SHELL, 825, "[Nn]*")))
	    return AskNo;
	if (zglob(buf, catgets(catalog, CAT_SHELL, 826, "[CcQqXx]*")))
	    return AskCancel;
	return dflt;
    }
}

/* for fullscreen mode */
void
clr_bot_line()
{
    print("");
}

#ifdef BSD
#undef fputs

/*
 * The standard 4.x BSD fputs() does not return any useful value.
 * For compatibility with Sun and SysV fputs(), we use this wrapper.
 */

int
Fputs(line, fp)
const char *line;
FILE *fp;
{
    clearerr(fp);
    (void) fputs(line, fp);
    if (ferror(fp))
	return EOF;
    return 0;
}

#endif /* BSD */
