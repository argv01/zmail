/* @(#)strings.h	(c) copyright 9/3/86 (Dan Heller) */

/*
 * If you *know* your sprintf() returns char*, you can remove the follow
 * define.  Careful, "new" 4.3BSD's sprintf returns int.  See README
 */
#define sprintf Sprintf
#ifdef SYSV
#define	index	strchr
#define	rindex	strrchr
#endif /* SYSV */

#if !defined(__linux__)
/* External function definitions for routines described in string(3).  */
extern char
    *strcat(), *strncat(), *strcpy(), *strncpy(),
    *index(), *rindex(), *getenv();
extern int
    strcmp(), strncmp(), strlen();
# endif

extern char
    *Sprintf(char *buf, char *fmt, ...),		/* See comments above function in strings.c */
    *argv_to_string(),	/* convert a vector of strings into one string */
    *any(), 		/* return first char in str2 that exists in str1 */
    *basename(),	/* return the last component of a file path */
    *ctrl_strcpy(),	/* string copy converting control chars to ascii */
    *itoa(),		/* return a string representation of a number */
    *lcase_strcpy(),	/* just like strcpy, but convert all chars to lower */
    *m_xlate(),		/* translate string from ascii to ctrl-char format */
    *my_atoi(), 	/* do an atoi, but return the last char parsed */
    *no_newln(),	/* remove newline and extra whitespace - return end */
    *savestr();		/* strcpy arg into malloc-ed memory; return address */

extern void
    print_argv(),	/* prints an argv as one string */
    putstring();	/* put a string */
