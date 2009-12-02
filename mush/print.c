/* @(#)print.c	2.4	(c) copyright 10/15/86 (Dan Heller) */

#undef va_start
#include <stdarg.h>
#define _STDARG_H
#include "mush.h"

/*ARGSUSED*/
/*VARARGS1*/
void
error(fmt, arg1, arg2, arg3, arg4)
register char *fmt;
char *arg1, *arg2, *arg3, *arg4;
{
    char buf[BUFSIZ];

    (void) sprintf(buf, fmt, arg1, arg2, arg3, arg4);
    if (buf[strlen(buf) - 1] != '\n') {
        if (errno)
#ifdef LINUX
            strcat(buf, strerror(errno));
#else
	    sprintf(buf+strlen(buf), ": %s\n", sys_errlist[errno]);
#endif
	else
	    strcat(buf, "\n");
    } 
#ifdef SUNTOOL
    if (istool > 1)
	ok_box(buf);
    else
#endif /* SUNTOOL */
    print(buf);
}

#if defined(SUNTOOL) || defined(CURSES)
/*
 * print just like printf -- to a window, to curses, or to stdout.  Use vprintf
 * if available.  msgbuf is the buffer used to print into if necessary.
 * If you're running SUN3.2 or higher, the typecast (unsigned char *)msgbuf
 * (where indicated) otherwise, msgbuf is not typecast at all.
 */
/*VARARGS*/
void
print(char *fmt, ...)
{
    static char msgbuf[BUFSIZ];
    va_list args;
#ifndef VPRINTF
    FILE foo;
#endif /* VPRINTF */
    static int x; /* position on line saved for continued prints */
    char *p; /* same type as struct file _ptr,_buf in stdio.h */

    va_start(args, fmt);

#ifdef CURSES
    if (iscurses) {
	if (isoff(glob_flags, CONT_PRNT))
	    move(LINES-1, x = 0), refresh();
    } else
#endif /* CURSES */
	if (istool < 2) {
#ifdef VPRINTF
	    (void) vprintf(fmt, args);
#else /* VPRINTF */
	    _doprnt(fmt, args, stdout);
#endif /* VPRINTF */
	    va_end(args);
	    (void) fflush(stdout);
	    return;
	}
    if (fmt) {
#ifdef VPRINTF
	(void) vsprintf(msgbuf, fmt, args); /* NULL in fmt reprints last msg */
#else /* VPRINTF */
	foo._cnt = BUFSIZ;
	foo._base = foo._ptr = msgbuf; /* may have to cast(unsigned char *) */
	foo._flag = _IOWRT+_IOSTRG;
	(void) _doprnt(fmt, args, &foo);
	*foo._ptr = '\0'; /* plant terminating null character */
#endif /* VPRINTF */
    }
    va_end(args);
    if (istool) {
	wprint("%s", msgbuf);
	return;
    }
    p = msgbuf;
    if (iscurses)
	while (p = index(p, '\n'))
	    *p = ' ';
#ifdef CURSES
    if (iscurses) {
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
    }
#endif /* CURSES */
}

#endif /* SUNTOOL || CURSES */

/* for curses mode */
clr_bot_line()
{
    print("");
}

#ifdef SUNTOOL

/*
 * wprint prints stuff to a scrollable textsw.  This is intended for
 * print statements that need to be recalled since print() overwrites
 * its last message.
 * For non-suntool mode, wprint() is just like printf().  For curses mode,
 * wprint() does -not- act like print() -- lines length is not counted
 * and newlines are not stripped.
 */
/*VARARGS*/
void
wprint(va_alist)
va_dcl;
{
#ifndef VPRINTF
    FILE foo;
#endif /* VPRINTF */
    char msgbuf[BUFSIZ]; /* we're not getting huge strings */
    char *fmt;
    va_list args;

    if (istool < 2) {
       va_start(args);
       fmt = vaarg(args, char *);
#ifdef VPRINTF
	(void) vprintf(fmt, args);
#else /* VPRINTF */
	_doprnt(fmt, args, stdout);
#endif /* VPRINTF */
	va_end(args);
	(void) fflush(stdout);
	return;
    }

    if (!mfprint_sw)
        return;
    va_start(args);
    fmt = va_arg(args, char *);
    if (fmt) {
#ifdef VPRINTF
	(void) vsprintf(msgbuf, fmt, args); /* NULL in fmt reprints last msg */
#else /* VPRINTF */
	foo._cnt = BUFSIZ;
	foo._base = foo._ptr = msgbuf; /* may have to cast (unsigned char *) */
	foo._flag = _IOWRT+_IOSTRG;
	_doprnt(fmt, args, &foo); /* format like printf into msgbuf via foo */
	*foo._ptr = '\0'; /* plant terminating null character */
#endif /* VPRINTF */
	textsw_insert(mfprint_sw, msgbuf, strlen(msgbuf));
    }
    va_end(args);
}

#include <sundev/kbio.h>
#include <sundev/kbd.h>

bell()
{
#ifdef KIOCCMD
    if (istool) {
	int kbd = open("/dev/kbd", O_WRONLY, 0);
	struct timeval timer;
	timer.tv_sec = 0;
	timer.tv_usec = 100000;
	if (kbd != -1) {
	    int cmd = KBD_CMD_BELL;
	    (void) ioctl(kbd, KIOCCMD, &cmd);
	    (void) select(32, (fd_set *) 0,(fd_set *) 0,(fd_set *) 0, &timer);
	    cmd = KBD_CMD_NOBELL;
	    (void) ioctl(kbd, KIOCCMD, &cmd);
	    (void) close(kbd);
	}
    } else
#endif /* KIOCCMD */
	(void) fputc('\007', stderr), (void) fflush(stderr);
    return 0;
}

#endif /* SUNTOOL */

#ifdef BSD
#undef fputs

/*
 * The standard 4.x BSD fputs() does not return any useful value.
 * For compatibility with Sun and SysV fputs(), we use this wrapper.
 */

Fputs(line, fp)
char *line;
FILE *fp;
{
    clearerr(fp);
    (void) fputs(line, fp);
    if (ferror(fp))
	return EOF;
    return 0;
}

#endif /* BSD */
