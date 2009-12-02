/* strings.c Copyright(1988) Dan Heller */

#undef va_start
#include <stdarg.h>
#define _STDARG_H
#include "mush.h"

/*
 * reverse a string.  Useful for uucp-style address comparisons.
 */
char *
reverse(s)
char s[];
{
    int n = strlen(s), m;
    char c;

    if (n < 1)
	return 0;
    if (n & 1)
	n = n/2 + 1, m = n - 2;
    else
	n /= 2, m = n - 1;
    for ( ; m >= 0; m--, n++)
	c = s[n], s[n] = s[m], s[m] = c;
    return s;
}

/*
 * lose the newline character, trailing whitespace, and return the end of p
 * test for '\n' separately since some _ctype_[] arrays may not have the
 * _S bit set for the newline character.  see <ctype.h> for more info.
 */
char *
no_newln(p)
register char *p;
{
    register char *p2 = p + strlen(p);	/* point it to the null terminator */

    while (p2 > p && *--p2 == '\n' || isspace(*p2))
	*p2 = 0;  /* get rid of newline and trailing spaces */
    return p2;
}

/* find any character in s1 that's in s2; return pointer to char in s1. */
char *
any(s1, s2)
register char *s1, *s2;
{
    register char *p;
    if (!s1 || !*s1 || !s2 || !*s2)
	return NULL;
    for( ; *s1; s1++) {
	for(p = s2; *p; p++)
	    if (*p == *s1)
		return s1;
    }
    return NULL;
}

/* check two lists of strings each of which contain substrings.
 * Each substring is delimited by any char in "delimiters"
 * return true if any elements in list1 are on list2.
 * thus:
 * string1 = "foo, bar, baz"
 * string2 = "foobar, baz, etc"
 * delimiters = ", \t"
 * example returns 1 because "baz" exists in both lists
 * NOTE: case is ignored.
 */
chk_two_lists(list1, list2, delimiters)
register char *list1, *list2, *delimiters;
{
    register char *p, c;
    register int found = 0;

    if (!list1 || !list2)
	return 0;

    if (p = any(list1, delimiters)) {
	if (p > list1) {
	    c = *p; *p = 0;
	    /* Check list2 against the first word of list1.
	     * Swap places of list2 and list1 to step through list2.
	     */
	    found = chk_two_lists(list2, list1, delimiters);
	    *p = c;
	}
	if (found)
	    return 1;
	for (p++; *p && index(delimiters, *p); p++)
	    ;
	if (!*p)
	    return 0;
    } else if (!any(list2, delimiters))
	/* Do the trivial case of single words */
	return !lcase_strncmp(list1, list2, -1);
    else
	p = list1;

    /* Either only list2 has delims or the first word of list1
     * did not match anything in list2.  Check list2 against the
     * rest of list1.  This could be more efficient by using a
     * different function to avoid repeating the any() calls.
     */
    return chk_two_lists(list2, p, delimiters);
}

#ifndef __linux__
bzero(addr, size)
register char *addr;
register int size;
{
    while (size-- > 0)
	addr[size] = 0;
}
#endif

/* do an atoi() on the string passed and return in "val" the decimal value.
 * the function returns a pointer to the location in the string that is not
 * a digit.
 */
char *
my_atoi(p, val)
register char *p;
register int *val;
{
    int positive = 1;

    if (!p)
	return NULL;
    *val = 0;
    if (*p == '-')
	positive = -1, p++;
    while (isdigit(*p))
	*val = (*val) * 10 + *p++ - '0';
    *val *= positive;
    return p;
}

/* strcmp ignoring case */
lcase_strncmp(str1, str2, n)
register char *str1, *str2;
{
    while (*str1 && *str2 && --n != 0)
	if (lower(*str1) != lower(*str2))
	    break;
	else
	    str1++, str2++;
    return lower(*str1) - lower(*str2);
}

/* strcpy converting everything to lower case (arbitrary) to ignore cases */
char *
lcase_strcpy(dst, src)
register char *dst, *src;
{
    register char *s = dst;

    /* "lower" is a macro, don't increment its argument! */
    while (*dst++ = lower(*src))
	src++;
    return s;
}

/* this strcpy returns number of bytes copied */
Strcpy(dst, src)
register char *dst, *src;
{
    register int n = 0;
    if (!dst || !src)
	return 0;
    while (*dst++ = *src++)
	n++;
    return n;
}

/* copy a vector of strings into one string -- return the end of the string */
char *
argv_to_string(p, argv)
register char *p, **argv;
{
    register int i;
    register char *ptr = p;

    *p = 0;
    if (!argv[0])
	return "";
    for (i = 0; argv[i]; i++)
	ptr += strlen(sprintf(ptr, "%s ", argv[i]));
    *--ptr = 0;   /* get rid of the last space */
    return ptr;
}

char *
itoa(n)
{
    static char buf[10];
    return sprintf(buf, "%d", n);
}

/*
 * There are two different kinds of sprintf() --those that return char * and
 * those that return int.  System-V returns int (the length of the resulting
 * string).  BSD has historically returned a pointer to the resulting string
 * instead. Mush was originally written under BSD, so the usage has always
 * been to assume the char * method.  Because the system-v method is far more
 * useful, mush should some day change to use that method, but until then,
 * this routine was written to allow all the unix'es to appear the same to
 * the programmer regardless of which sprintf is actually used.  The "latest"
 * version of 4.3BSD (as of Fall 1988) has changed its format to go from the
 * historical BSD method to the sys-v method.  It is no longer possible to
 * simply #ifdef this routine for sys-v --it is now required to use this
 * routine regardless of which sprintf is notice to your machine.  However,
 * if you know your system's sprintf returns a char *, you can remove the
 * define in strings.h
 */
/*VARARGS*/
/*ARGSUSED*/
char *
Sprintf(char *buf, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    
#ifdef VPRINTF
    (void) vsprintf(buf, fmt, ap);
#else
    {
	FILE foo;
	foo._cnt = BUFSIZ;
	foo._base = foo._ptr = buf; /* may have to be cast (unsigned char *) */
	foo._flag = _IOWRT+_IOSTRG;
	(void) _doprnt(fmt, ap, &foo);
	*foo._ptr = '\0'; /* plant terminating null character */
    }
#endif /* VPRINTF */
    va_end(ap);
    return buf;
}

void
print_argv(argv)
char **argv;
{
    while (*argv)
	if (debug)
	    wprint("(%s) ", *argv++);
	else
	    wprint("%s ", *argv++);
    wprint("\n");
}

/*
 * putstring -- put a string into a file.  Expand \t's into tabs and \n's
 * into newlines.  Append a \n and fflush(fp);
 */
void
putstring(p, fp)
register char *p;
register FILE *fp;
{
    for ( ; *p; ++p)
	if (*p != '\\')
	    (void) fputc(*p, fp);
	else
	    switch(*++p) {
		case 'n': (void) fputc('\n', fp);
		when 't': (void) fputc('\t', fp);
		otherwise: (void) fputc(*p, fp);
	    }
    (void) fputc('\n', fp);
    (void) fflush(fp);
}

#define chtoi(c)	((int)(c) - (int)'0')

/* m_xlate(str) converts strings of chars which contain ascii representations
 *  of control characters appearing in str into the literal characters they
 *  represent.  The usual curses-mode character expansions (\Cx -> control-x)
 *  are honored, as are most C escapes.  Unrecognized portions are unchanged.
 */
char *
m_xlate (str)
register char *str;
{
    register char *r, *s, *t;
    int dv, nd;

    /*
     * r will receive the new string, s will track the old one,
     *  and t will step through escape sequences
     * This allows the translation to be done in place
     */
    r = s = str;
    while (s && *s) {
	if (*s == '\\') {
	    t = s + 1;
	    /*
	     * After each case below, t should point to the character
	     *  following the escape sequence
	     */
	    switch(*t) {
		case '\0' :
		    /*
		     * Hmmm ... a backslash followed by the string
		     *  terminator.  Copy the backslash ONLY.
		     */
		    *r++ = *s++;
		    break;
		case '0' :
		case '1' :
		case '2' :
		case '3' :
		case '4' :
		case '5' :
		case '6' :
		case '7' :
		    /*
		     * Convert up to 3 octal digits to their ascii value
		     */
		    dv = chtoi(*t++);
		    for (nd = 0; (isdigit(*t) && (nd < 2)); nd++)
			if (chtoi(*t) < 8)
			    dv = (8 * dv) + chtoi(*t++);
			else
			    break;
		    if (dv < 256 && dv > 0)
			/* Valid octal number escaped */
			*r++ = (char)dv;
		    else
			/* Invalid octal number, so copy unchanged */
			while (s < t)
			    *r++ = *s++;
		    break;
		case 'b' :
		    *r++ = '\b';
		    t++;
		    break;
		case 'C' :
		    t++;
		    if (*t == '?')
			*r++ = '\177';
		    else if (*t == '~')
			*r++ = '\036';
		    else if (*t == '/')
			*r++ = '\037';
		    else if (isalpha(*t) || *t > '\132' && *t < '\140')
			*r++ = *t & 037;
		    else
			while (s <= t) *r++ = *s++;
		    t++;
		    break;
		case 'E' :
		    *r++ = '\033';
		    t++;
		    break;
		case 'f' :
		    *r++ = '\f';
		    t++;
		    break;
		case 'n' :
		    *r++ = '\n';
		    t++;
		    break;
		case 'r' :
		    *r++ = '\r';
		    t++;
		    break;
		case 't' :
		    *r++ = '\t';
		    t++;
		    break;
		case '\\' :
		    *r++ = *t++;
		    break;
		default :
		    /*
		     * Not recognized, so copy both characters
		     */
		    *r++ = *s++;
		    *r++ = *s++;
		    break;
	    }
	    /*
	     * Now make sure s also points to the character after the
	     *  escape sequence, by comparing to t
	     */
	    if (t > s)
		s = t;
	} else
	    *r++ = *s++;
    }
    *r = '\0';
    return str;
}

/*
 * Convert control characters to ascii format (reverse effect of m_xlate()).
 */
char *
ctrl_strcpy(s_out, s_in, bind_format)
register char *s_out, *s_in;
{
#if !defined(M_XENIX) || (defined(M_XENIX) && !defined(CURSES))
    extern char *_unctrl[];
#endif /* !M_XENIX || M_XENIX && !CURSES */
    char *start = s_out;

    for (; *s_in; s_in++)
	if (*s_in == '\n')
	    *s_out++ = '\\', *s_out++ = 'n';
	else if (*s_in == '\r')
	    *s_out++ = '\\', *s_out++ = 'r';
	else if (*s_in == '\t')
	    *s_out++ = '\\', *s_out++ = 't';
	else if (*s_in == ESC)
	    *s_out++ = '\\', *s_out++ = 'E';
	else if (iscntrl(*s_in)) {
	    if (bind_format)
		*s_out++ = '\\', *s_out++ = 'C';
	    else
		*s_out++ = '^';
	    *s_out++ = _unctrl[*s_in][1];
	} else
	    *s_out++ = *s_in;
    *s_out = 0;
    return start;
}

/*
 * This routine returns a pointer to the file portion of a path/file name.
 */
char *
basename(path)
register char *path;
{
    char *file;

    if (file = rindex(path, '/'))
	return ++file;
    return path;
}
