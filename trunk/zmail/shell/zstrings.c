/* strings.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	strings_rcsid[] =
    "$Id: zstrings.c,v 2.61 2005/05/31 07:36:42 syd Exp $";
#endif

#include "zmail.h"
#include "fsfix.h"
#include "catalog.h"
#include "zmstring.h"
#include "zstrings.h"

#include <general.h>
#include <ctype.h>

#if defined(DARWIN)
#include <stdlib.h>
#endif

/*
 * Reverse a string.  Useful for uucp-style address comparisons.
 */
char *
reverse(s)
char s[];
{
  char *p, *q, c;

  p = s;
  q = s+strlen(s);
  while (--q > p)
  {
    c = *q;
    *q = *p;
    *p++ = c;
  }
  return s;
}

/*
 * Lose the newline character, trailing whitespace, and return the end of p.
 * Test for '\n' separately since some _ctype_[] arrays may not have the
 * _S bit set for the newline character.  see <ctype.h> for more info.
 */
char *
no_newln(p)
register char *p;
{
    register char *p2 = p + strlen(p);	/* point it to the null terminator */

    while (p2 > p && *--p2 == '\n' || isascii(*p2) && isspace(*p2))
	*p2 = 0;  /* get rid of newline and trailing spaces */
    return p2;
}

/* Find any character in s1 that's in s2;
 * return pointer to that char in s1.
 */
char *
any(s1, s2)
     register const char *s1, *s2;
{
    register const char *p;

    if (!s1 || !*s1 || !s2 || !*s2)
	return NULL;
    for( ; *s1; s1++) {
	for(p = s2; *p; p++)
	    if (*p == *s1)
		return (char *) s1;
    }
    return NULL;
}

/* Find the last of any character in s1 that's in s2;
 * return pointer to last such char in s1.
 */
char *
rany(s1, s2)
     register const char *s1, *s2;
{
    register const char *p, *p2 = s1;

    if (!s1 || !*s1 || !s2 || !*s2)
	return NULL;
    s1 += strlen(s1);		/* Skip to last character in s1 */
    while (s1-- > p2) {
	for (p = s2; *p; p++)
	    if (*p == *s1)
		return (char *) s1;
    }
    return NULL;
}

static int
chk_one_item(item, len, list, delimiters)
    const char *item;
    char *list, *delimiters;
    int len;
{
    char *p;

    if (!item || !list || !len)
	return 0;

    /* Find first delimiter, skipping leading delimiters */
    while (*list && (p = any(list, delimiters)) == list)
	list++;
    if (!*list)
	return 0;

    if (p) {
	if (len != p - list || ci_strncmp(item, list, len) != 0)
	    return chk_one_item(item, len, p + 1, delimiters);
	return 1;
    }
    return (ci_strncmp(item, list, len) == 0 && strlen(list) == len);
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
int
chk_two_lists(list1, list2, delimiters)
const char *list1, *list2, *delimiters;
{
    register char *p1, *p2;

    if (!list1 || !list2)
	return 0;

    /* Find first delimiter, skipping leading delimiters */
    while (*list1 && (p1 = any(list1, delimiters)) == list1)
	list1++;
    if (!*list1)
	return 0;
    while (*list2 && (p2 = any(list2, delimiters)) == list2)
	list2++;
    if (!*list2)
	return 0;

    /*
     * p1 as boolean	represents the presence of a delimiter in list1
     * p1 as pointer	represents the end of the first item in list1
     * p1 - list1	gives the length of the first item in list1
     * p1 + 1		gives the "rest" of list1
     *
     * same for p2 and list2
     */
    
    /* Check all cases of booleans for p1 and p2 */
    if (p1 && p2) {
	if (p1 - list1 != p2 - list2) {
	    /* Lengths different, check first item against rest of list2,
	     * then if that fails, rest of list1 against all of list2
	     */
	    return (chk_one_item(list1, p1 - list1, p2 + 1, delimiters) ||
		    chk_two_lists(p1 + 1, list2, delimiters));
	}
	/* Lengths of items same, compare strings */
	if (ci_strncmp(list1, list2, p1 - list1) == 0)
	    return 1;
	/* Check this item of list2 against rest of list1,
	 * then check all of list1 against rest of list2
	 */
	return (chk_one_item(list2, p2 - list2, p1 + 1, delimiters) ||
		chk_two_lists(list1, p2 + 1, delimiters));
    }
    if (p1 /* && !p2 */) {
	/* Do strncmp() before strlen(), it fails quickly */
	if (ci_strncmp(list1, list2, p1 - list1) == 0 &&
		strlen(list2) == p1 - list1)
	    return 1;
	/* Only one item in list2, check it against rest of list1 */
	return chk_one_item(list2, strlen(list2), p1 + 1, delimiters);
    }
    if (p2 /* && !p1 */) {
	/* Do strncmp() before strlen(), it fails quickly */
	if (ci_strncmp(list1, list2, p2 - list2) == 0 &&
		strlen(list1) == p2 - list2)
	    return 1;
	/* Only one item in list1, check it against rest of list2 */
	return chk_one_item(list1, strlen(list1), p2 + 1, delimiters);
    }

    /* Only one item in each list */
    return (ci_strcmp(list1, list2) == 0);
    
#ifdef NOT_NOW
    register const char *p;
    register int found = 0;

    if (!list1 || !list2)
	return 0;

    if (p = any(list1, delimiters)) {
	if (p > list1) {
	    char *q = savestrn(list1, p-list1);
	    /* Check list2 against the first word of list1.
	     * Swap places of list2 and list1 to step through list2.
	     */
	    found = chk_two_lists(list2, q, delimiters);
	    xfree(q);
	}
	if (found)
	    return 1;
	for (p++; *p && index(delimiters, *p); p++)
	    ;
	if (!*p)
	    return 0;
    } else if (!any(list2, delimiters))
	/* Do the trivial case of single words */
	return !ci_strcmp(list1, list2);
    else
	p = list1;

    /* Either only list2 has delims or the first word of list1
     * did not match anything in list2.  Check list2 against the
     * rest of list1.  This could be more efficient by using a
     * different function to avoid repeating the any() calls.
     */
    return chk_two_lists(list2, p, delimiters);
#endif /* NOT_NOW */
}

#if !defined(M_UNIX) && !defined(BSD)
    /* The X libraries define bzero and bcopy, except in Solaris ... */
#if !defined(GUI) || defined(MAC_OS) || defined(sun) && defined(SYSV)

#ifndef HAVE_B_MEMFUNCS
#ifndef bzero
bzero(addr, size)
register char *addr;
register int size;
{
    while (--size >= 0)
	addr[size] = 0;
}
#endif /* bzero */

#ifndef bcopy
bcopy(from, to, n)
register char *from, *to;
int n;
{
    register int i;
    /*
     * This is correct enough, and if it ever blows up,
     * then we will sell to those people.
     */
    if ((unsigned long)to <= (unsigned long)from)
	for (i = 0; i < n; ++i)
	    to[i] = from[i];
    else
	for (i = n-1; i >= 0; --i)
	    to[i] = from[i];
}
#endif /* bcopy */
#endif /* !HAVE_B_MEMFUNCS */

#endif /* !GUI || MAC_OS || sun && SYSV */
#endif /* !M_UNIX && !BSD */

/* do an atoi() on the string passed and return in "val" the decimal value.
 * the function returns a pointer to the location in the string that is not
 * a digit.
 */
char *
my_atoi(p, val)
register const char *p;
register int *val;
{
    int positive = 1;

    *val = 0;
    if (!p)
	return NULL;
    if (*p == '-')
	positive = -1, p++;
    while (isdigit(*p))
	*val = (*val) * 10 + *p++ - '0';
    *val *= positive;
    /* XXX casting away const */
    return (char *) p;
}

/* copy a vector of strings into one string -- return the end of the string */
char *
argv_to_string(p, argv)
    register char *p;
    char **argv;
{
    register char *ptr = p;

    if (argv[0]) {
	/* argv_to_string(NULL, argv) returns malloc'd space,
	 * else we behave exactly as before, returning end.
	 * One extra strlen traded against all those sprintfs.
	 */
        ptr = joinv(p, argv, " ");
	if (ptr && p)
	    ptr += strlen(p);
    }
    else if (ptr)
      *ptr = '\0';
    return ptr;
}

/* copy a vector of strings into one string --
 * return the end of the string if it was non-NULL to begin with,
 * otherwise return the beginning of a newly malloced string.
 * The characters specified by chars_to_escape are preceded
 * by a backslash in the result.
 */
char *
smart_argv_to_string(buf, argv, chars_to_escape)
    char *buf;
    char **argv;
    const char *chars_to_escape;
{
    char *start, *ptr;
    int i, j, do_backslash;

    if (!chars_to_escape) {
	/* The characters dollar, tab, newline, doublequote, quote, space,
	 * and backslash are preceded by a backslash in the result.
	 */
	chars_to_escape = "$ \t\n\"\'\\";
	do_backslash = TRUE;
    } else
	do_backslash = !!index(chars_to_escape, '\\');

    if (!buf) {
	/*
	 * Calculate the desired length
	 */
	int len = 0;
	for (i = 0; argv[i]; ++i) {
	    for (j = 0; argv[i][j]; ++j) {
		if (index(chars_to_escape, argv[i][j]))
		    len++;	/* room for the backslash preceding the char */
		len++;		/* room for the char */
	    }
	    len++;		/* room for the space or null after the arg */
	}
	if (len == 0)
	    len = 1;
	start = (char *) malloc(len);
	if (!start)
	    return NULL;
    } else
	start = buf;

    ptr = start;
    for (i = 0; argv[i]; ++i) {
	for (j = 0; argv[i][j]; ++j) {
	    /* If backslash is not one of the chars_to_escape, don't
	     * escape any char that already has a backslash before it.
	     */
	    if (index(chars_to_escape, argv[i][j]) &&
		    (do_backslash || j == 0 || argv[i][j-1] != '\\'))
		*ptr++ = '\\';
	    *ptr++ = argv[i][j];
	}
	*ptr++ = (argv[i+1] ? ' ' : '\0');
    }
    if (ptr == start)
	*ptr++ = '\0';

    return buf ? ptr : start;
}

#ifdef HAVE_STDARG_H
#include <stdarg.h>	/* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

/*
 * There are two different kinds of sprintf() --those that return char * and
 * those that return int.  System-V returns int (the length of the resulting
 * string).  BSD has historically returned a pointer to the resulting string
 * instead. Z-Mail was originally written under BSD, so the usage has always
 * been to assume the char * method.  Because the system-v method is far more
 * useful, Z-Mail should some day change to use that method, but until then,
 * this routine was written to allow all the unix'es to appear the same to
 * the programmer regardless of which sprintf is actually used.  The "latest"
 * version of 4.3BSD (as of Fall 1988) has changed its format to go from the
 * historical BSD method to the sys-v method.  It is no longer possible to
 * simply #ifdef this routine for sys-v --it is now required to use this
 * routine regardless of which sprintf is notice to your machine.  However,
 * if you know your system's sprintf returns a char *, you can remove the
 * define in strings.h
 */
char *
#ifdef HAVE_STDARG_H
Sprintf(char *buf, const char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS2*/
/*ARGSUSED*/
Sprintf(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    va_list args;
#ifndef HAVE_STDARG_H
    char *buf, *fmt;

    va_start(args);
    buf = va_arg(args, char *);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
#ifdef HAVE_VPRINTF
    (void) vsprintf(buf, fmt, args);
#else
    {
	FILE foo;
	foo._cnt = (int)((~(unsigned int)0)>>1); /* buf overflows if it's gonna */
	foo._base = foo._ptr = buf; /* may have to be cast (unsigned char *) */
	foo._flag = _IOWRT+_IOSTRG;
	(void) _doprnt(fmt, args, &foo);
	*foo._ptr = '\0'; /* plant terminating null character */
    }
#endif /* HAVE_VPRINTF */
    va_end(args);
    return buf;
}

char *
itoa(n)
int n;
{
    static char buf[10];

    sprintf(buf, "%d", n);
    return (buf);
}

/* print_argv() has effectively degenerated into a front-end to Debug().
 * It should not be used for any real printing.  Rename to debug_argv()?
 */
void
print_argv(argv)
char **argv;
{
    while (*argv)
	if (debug)
	    Debug("(%s) ", *argv++);
	else
	    wprint("%s ", *argv++);
    if (debug)
	Debug("\n");
    else
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
		when '\0': (void) fputc('\\', fp);
		otherwise: (void) fputc(*p, fp);
	    }
    (void) fputc('\n', fp);
    (void) fflush(fp);
}

#define chtoi(c)	((int)(c) - (int)'0')

/* m_xlate(str) converts strings of chars which contain ascii representations
 *  of control characters appearing in str into the literal characters they
 *  represent.  The usual fullscreen-mode char expansions (\Cx -> control-x)
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
		case '^' :
		    *r++ = *t++;
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
	} else if (*s == '^' && s[1]) {
	    t = s + 1;
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
	    if (++t > s)
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
int bind_format;
{
#if !defined(M_XENIX) || (defined(M_XENIX) && !defined(CURSES))
    extern char *_unctrl[];
#endif /* !M_XENIX || M_XENIX && !CURSES */
    char *start = s_out;

    for (; *s_in; s_in++)
	if (*s_in == '\n')
	    if (bind_format < 0)
		*s_out++ = 'R', *s_out++ = 'E', *s_out++ = 'T';
	    else
		*s_out++ = '\\', *s_out++ = 'n';
	else if (*s_in == '\r')
	    if (bind_format < 0)
		*s_out++ = 'R', *s_out++ = 'E', *s_out++ = 'T';
	    else
		*s_out++ = '\\', *s_out++ = 'r';
	else if (*s_in == '\t')
	    if (bind_format < 0)
		*s_out++ = 'T', *s_out++ = 'A', *s_out++ = 'B';
	    else
		*s_out++ = '\\', *s_out++ = 't';
	else if (*s_in == '^' || *s_in == '\\')
	    if (bind_format <= 0)
		*s_out++ = *s_in;
	    else
		*s_out++ = '\\', *s_out++ = *s_in;
	else if (*s_in == ESC)
	    if (bind_format < 0)
		*s_out++ = 'E', *s_out++ = 'S', *s_out++ = 'C';
	    else
		*s_out++ = '\\', *s_out++ = 'E';
	else if (iscntrl(*s_in)) {
	    if (bind_format > 0)
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
#if !defined(__STDC__) && !defined(__cplusplus) && !defined(_OSF_SOURCE)
const char *
basename(path)
register const char *path;
{
    const char *file;

    if (path && (file = last_dsep(path)) && file > path)
	return ++file;
    return path;
}
#endif /* !__STDC__ && !__cplusplus && !_OSF_SOURCE */

#define wrapspc(x)	index(" \t\n\r\f", (x))

void
fmt_string(in, out, length, wrap, tabstop, join)
char *in, *out;
int length, wrap, tabstop, join;
{
    char *p, *p2, *last;
    int punct, indent = TRUE, trail, c;

    if (wrap <= 0 || length < 2) {
	(void) strcpy(out, in);
	return;
    } else
	trail = in[length-1];

    for (last = out; length > 0 && *in; length--, in++) {
	if ((p = any(in, "\t\n")) && *p == '\t') {   /* I hate tabs */
	    int n = p - in, t = 0, s = 0;
	    /* Accumulate in s the number of spaces that all tabs
	     * represent and in t the number of tabs involved.  The
	     * adjustment to the wrap position is then (s - t).
	     * Accumulate in n the total number of "hidden spaces"
	     * and other characters that we've skipped over, so we
	     * know when to stop skipping ...
	     */
	    while (n < wrap && *p == '\t') {
		t++;
		if (n % tabstop == 0)
		    n++, s++;
		while (n % tabstop && n < wrap)
		    n++, s++;
		while (*++p && *p != '\t' && *p != '\n' && n < wrap)
		    n++;
	    }
	    p = p2 = in + min(length, wrap - (s - t));
	} else
	    p = p2 = in + min(length, wrap);
	if (*p)					/* If not at a '\0' */
	    while (p > in && !wrapspc(*p))	/* back up to space */
		--p;
	else					/* Else, start with */
	    --p;				/* whatever we have */
	while (*p && p > in && wrapspc(*p))	/* Back past spaces */
	    --p;
	if (p > in) {				/* If one was found */
	    c = *++p;				/* save first space */
	} else {				/* Else, look ahead */
	    for (p = p2; *p && !wrapspc(*p); p++)
		;				/* and if one found */
	    c = *p;				/* use that instead */
	}
	*p = 0;
	p2 = in;
	for (punct = FALSE; *in; ++in) {
	    if (*in == '\n' && (!join || wrapspc(in[1]))) {
		*p = c;
		c = '\n';
		break;
	    }
	    if (wrapspc(*in) && (join || *in != '\n')) {
		*last++ = (join && *in == '\n')? ' ' : *in;
#if 0				/* This used to double a space after .!?: */
		if (punct && last[-1] == ' ')
		    *last++ = ' ';
#endif
	    } else {
		*last++ = *in;
		indent = !join;
	    }
#if 0				/* We no longer care about punctuation */
	    if (join)
		punct = !!index(".!?:", *in);
#endif
	    if (!indent)
		while (wrapspc(*in) && (in[1] == ' ' || in[1] == '\t'))
		    in++;
	}
	/* Trim off any trailing spaces we may have copied */
	while (last > out && wrapspc(last[-1]) && last[-1] != '\n')
	    --last;
	*last++ = '\n';
	if (c == '\n' && join && wrapspc(in[1])) {
	    *last++ = *++in;
	    indent = TRUE;
	} else if (c != '\n' && in >= p) {
	    *p = c;
	    p = in;
	    /* Skip over any whitespace up to a newline */
	    while (wrapspc(*p) && *p != '\n')
		++p;
	    if (*p == '\n')
		in = p;
	}
	length -= in - p2;
    }
    if (!wrapspc(trail))
	while (last > out && wrapspc(last[-1]))
	    *--last = '\0';
    *last = '\0';
}

/*
 * Count the number of elements in a vector
 */
int
vlen(v)
char **v;
{
    int n;

    if (!v) return 0;
    for (n = 0; v[n]; n++)
	;
    return n;
}

/*
 * Search a vector (where are hash tables when you need them?)
 */
char **
vindex(vec, str)
char **vec, *str;
{
    int i;

    if (!vec || !*vec || !str)
	return 0;

    for (i = 0; vec[i]; i++)
	if (strcmp(vec[i], str) == 0)
	    return &vec[i];

    return 0;
}

/*
 * Compare a vector -- sort of
 */
int
vcmp(v1, v2)
char **v1, **v2;
{
    int i;

    /* First do fast cmps and avoid segmentation faults */
    if (v1 == v2)
	return 0;
    if (!v1)
	return -1;
    if (!v2 || *v1 && !*v2)
	return 1;
    if (*v2 && !*v1)
	return -1;
    if (!*v1 && !*v2)
	return 0;

    /* Now compare strings within the vectors */
    while (*v1 && *v2)
	if ((i = strcmp(*v1++, *v2++)) != 0)
	    return i;

    /* Re-use the block of "if"s above for the final cmp */
    return vcmp(v1, v2);
}

/*
 * Copy a vector -- returns number of elements
 */
int
vcpy(v1, v2)
    char ***v1;
    char **v2;
{
    int i, s2;

    if (!v1 || !v2)
	return -1;
    s2 = vlen(v2);
    *v1 = (char **)malloc((unsigned)((s2 + 1) * sizeof(char *)));
    if (*v1) {
	for (i = 0; i < s2; i++) {
	    (*v1)[i] = savestr(v2[i]); 
	    if (!(*v1)[i]) {
		free_vec(*v1);
		*v1 = DUBL_NULL;
		return -1;
	    }
	}
	(*v1)[i] = NULL;
	return i;
    }
    return -1;
}

/*
 * Insert one vector into another -- frees v2
 */
int
vins(v1, v2, pos)
char ***v1, **v2;
int pos;
{
    int s1 = 0, s2 = 0;
    char **va, **vb, **orig_v2 = v2;
    
    if (!v1)
	return -1;
    if (*v1)
	for (s1 = 0; (*v1)[s1]; s1++);
    if (v2)
	for (s2 = 0; v2[s2]; s2++);
    if (*v1)
	*v1 = (char **) realloc(*v1, (unsigned)((s1+s2+1)*sizeof **v1));
    else {
	*v1 = (char **) malloc((unsigned)((s2+1)*sizeof **v1));
	**v1 = NULL;
    }
    va = *v1;
    while (*va && pos)
	va++, pos--;
    vb = va;
    while (*va) va++;
    for (; va >= vb; va--)
	va[s2] = va[0];
    while (*v2) *vb++ = *v2++;
    xfree((char *)orig_v2);
    return s1+s2;
}

/*
 * Duplicate a vector -- returns the new vector
 */
char **
vdup(v)
char **v;
{
    char **x = DUBL_NULL;

    (void) vcpy(&x, v);
    return x;
}

/*
 * Speedy vector append when sizes are known -- frees v2
 */
int
catv(s1, v1, s2, v2)
int s1, s2;
char ***v1, **v2;
{
    int i;

    if (s1 < 0 || !v1)
	return -1;
    if (s2 < 0 || !v2)
	return s1;

    /* realloc(NULL, size) should be legal, but Sun doesn't support it. */
    if (*v1)
        *v1 = (char **)realloc(*v1,(unsigned)((s1+s2+1) * sizeof(char **)));
    else {
        *v1 = v2;	/* Do the obvious optimization */
	return s2;
    }

    if (*v1) {
	for (i = 0; i < s2 && v2[i]; i++)
	    (*v1)[s1 + i] = v2[i]; 
	(*v1)[s1 + i] = NULL;
	xfree((char *)v2);
	return s1 + i;
    }
    return -1;
}

/*
 * Append one vector to another -- frees v2
 */
int
vcat(v1, v2)
char ***v1, **v2;
{
    if (!v1)
	return -1;
    return catv(vlen(*v1), v1,
		vlen( v2), v2);
}

/*
 * Append one string onto a vector
 */
int
vcatstr(v1, s)
char ***v1;
const char *s;
{
    return vcat(v1, unitv(s));
}

/*
 * Make a string into a one-element vector
 */
char **
unitv(s)
const char *s;
{
    char **v;

    if (v = (char **)malloc((unsigned)(2 * sizeof(char *)))) {
	v[0] = savestr(s);
	v[1] = NULL;
    }
    return v;
}

/*
 * Make a pointer into a one-element vector
 */
char **
unitp(p)
const char *p;
{
    char **v;

    if (v = (char **)malloc((unsigned)(2 * sizeof(char *)))) {
	/* XXX casting away const */
	v[0] = (char *) p;
	v[1] = NULL;
    }
    return v;
}

/*
 * Join a vector into a string, separating each pair of elements with sep.
 * If str is NULL, allocates and returns space, else copies into str.
 */
char *
joinv(str, vec, sep)
    char *str;
    char **vec;
    const char *sep;
{
    char *s1;
    const char *s3;
    unsigned int count = 0, len1 = 0, len2 = sep? strlen(sep) : 0;

    if (!vec)
	return NULL;

    for (s1 = str; s3 = *vec; ++vec) {
	unsigned int len3 = strlen(s3);
	if (!str) {
	    if (s1)
		s1 = (char *) realloc(s1, len1 + len2 + len3 + 1);
	    else
		s1 = (char *) malloc(len2 + len3 + 1);
	}
	if (s1) {
	    if (sep && count++) {
		(void) strcpy(s1 + len1, sep);
		len1 += len2;
	    }
	    (void) strcpy(s1 + len1, s3);
	    len1 += len3;
	} else
	    return NULL;
    }

    return s1;
}

/*
 * Split a string into a vector using the indicated separator characters.
 * If crunch is true, multiple consecutive separators are discarded,
 * otherwise an empty element will be returned "between" separators.
 */
char **
strvec(str, sep, crunch)
const char *str, *sep;
int crunch;
{
    char **vec = DUBL_NULL, *nxt;
    int cnt = 0;

    if (!str)
	return DUBL_NULL;

    if (crunch)
	while (*str && index(sep, *str) && *++str)
	    ;
    while (nxt = any(str, sep)) {
	char **uv = (char **)malloc(2 * (unsigned)sizeof(char **));
	if (uv) {
	    uv[0] = (char *) malloc((unsigned)((nxt - str) + 1));
	    uv[1] = NULL;
	}
	if (!uv || !*uv) {
	    free_vec(vec);
	    return DUBL_NULL;
	}
	**uv = 0;
	strncat(*uv, str, nxt - str);
	cnt = catv(cnt, &vec, 1, uv); /* Frees uv */
	if (cnt < 0) {
#ifdef SAFE_REALLOC
	    free_vec(vec);
#endif /* SAFE_REALLOC */
	    return DUBL_NULL;
	}
	while (index(sep, *nxt) && *++nxt && crunch)
	    ;
	str = nxt;
    }
    if (*str) {
	cnt = catv(cnt, &vec, 1, unitv(str));
	if (cnt < 0) {
#ifdef SAFE_REALLOC
	    free_vec(vec);
#endif /* SAFE_REALLOC */
	    return DUBL_NULL;
	}
    }
    return vec;
}

char **
vvavec(args)
va_list args;
{
    char **vec = DUBL_NULL, *nxt;
    int cnt = 0;

    while (nxt = va_arg(args, char *)) {
	cnt = catv(cnt, &vec, 1, unitv(nxt));
	if (cnt < 0) {
#ifdef SAFE_REALLOC
	    free_vec(vec);
#endif /* SAFE_REALLOC */
	    return DUBL_NULL;
	}
    }

    return vec;
}

char **
#ifdef HAVE_STDARG_H
vavec(char *start, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS2*/
/*ARGSUSED*/
vavec(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    va_list args;
    char **vec;
#ifndef HAVE_STDARG_H
    char *start;

    va_start(args);
    start = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, start);
#endif /* !HAVE_STDARG_H */

    if (start) {
	vec = unitv(start);
	if (vcat(&vec, vvavec(args)) < 0) {
#ifdef SAFE_REALLOC
	    free_vec(vec);
#endif /* SAFE_REALLOC */
	    vec = DUBL_NULL;
	}
    }
    va_end(args);
    return vec;
}

char **
vvaptr(args)
va_list args;
{
    char **ptr = DUBL_NULL, *nxt;
    int cnt = 0;

    while (nxt = va_arg(args, char *)) {
	cnt = catv(cnt, &ptr, 1, unitp(nxt));
	if (cnt < 0) {
#ifdef SAFE_REALLOC
	    free(ptr);
#endif /* SAFE_REALLOC */
	    return DUBL_NULL;
	}
    }

    return ptr;
}

char **
#ifdef HAVE_STDARG_H
vaptr(char *start, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS2*/
/*ARGSUSED*/
vaptr(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    va_list args;
    char **ptr;
#ifndef HAVE_STDARG_H
    char *start;

    va_start(args);
    start = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, start);
#endif /* !HAVE_STDARG_H */

    if (start) {
	ptr = unitp(start);
	if (vcat(&ptr, vvaptr(args)) < 0) {
#ifdef SAFE_REALLOC
	    free(ptr);
#endif /* SAFE_REALLOC */
	    ptr = DUBL_NULL;
	}
    }
    va_end(args);
    return ptr;
}

int
stripq(argv)
char **argv;
{
    int i;
    char *p, buf[BUFSIZ];

    for (i = 0; argv && argv[i]; i++)
	if (argv[i][0] == '"' || argv[i][0] == '\'') {
	    (void) strcpy(buf, &argv[i][1]);
	    p = buf + strlen(buf);
	    if (*--p == argv[i][0])
		*p = 0;
	    (void) strcpy(argv[i], buf);
	}
    return i;
}

/*
 * Reveals hidden constant strings #defined as 's','t','r','i','n','g',0.
 */
char *
#ifdef HAVE_STDARG_H
unhidestr(int start, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS2*/
/*ARGSUSED*/
unhidestr(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    static int nstrings;
    static char **strings;
    int i;
    char buf[1000];

    va_list args;
#ifndef HAVE_STDARG_H
    int start;

    va_start(args);
    start = va_arg(args, int);
#else /* HAVE_STDARG_H */
    va_start(args, start);
#endif /* !HAVE_STDARG_H */

    buf[0] = start;
    for (i = 1; i < sizeof(buf); ++i) {
	buf[i] = va_arg(args, int);
	if (!buf[i])
	    break;
    }
    buf[sizeof(buf)-1] = '\0';

    va_end(args);

    /*
     * Now we got the string in buf.  Can't just return it,
     * cause then we'd have to use a static buffer that gets overwritten
     * with each call,
     * so instead we maintain a table of strings.
     */
    for (i = 0; i < nstrings; ++i)
	if (strcmp(buf, strings[i]) == 0)
	    return strings[i];

    strings = (char **) (strings ? realloc(strings, (nstrings+1)*sizeof(char *))
				 : malloc((nstrings+1) * sizeof(char *)));
    if (!strings) {
	nstrings = 0;
	return NULL;
    }
    if (!(strings[nstrings] = savestr(buf)))
	return NULL;
    return strings[nstrings++];
}

/* this strcpy returns number of bytes copied */
int
Strcpy(dst, src)
     register char *dst;
     register const char *src;
{
    register char *d = dst;

    if (!dst || !src)
	return 0;
    while (*dst++ = *src++)
	;
    return dst - d - 1;
}

/*
 * String comparison function for qsort.  An empty string is greater
 * than any other, otherwise strcmp is used.
 */
int
strptrcmp(p1, p2)
     const char **p1, **p2;
{
    if (!**p1 || !**p2)
	return **p2 - **p1;
    return strcmp(*p1, *p2);
}

/*
 * Numeric string comparison for qsort.  Doesn't work for doubles.
 */
int
strnumcmp(p1, p2)
     char **p1, **p2;
{
    if (!**p1 || !**p2)
	return **p2 - **p1;
    return atoi(*p1) - atoi(*p2);
}

#ifndef savestr
char *
savestr(s)
register const char *s;
{
    register char *p;

    if (!s)
	s = "";
    if (!(p = (char *) malloc((unsigned) (strlen(s) + 1)))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 763, "out of memory saving %s" ), s);
	return NULL;
    }
    return strcpy(p, s);
}
#endif /* savestr */

char *
savestrn(s, n)
const char *s;
int n;
{
    register char *p;

    if (!s)
	s = "";
    if (!(p = (char *) malloc((unsigned) (n+1)))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 763, "out of memory saving %s" ), s);
	return NULL;
    }
    p = strncpy(p, s, n);
    p[n] = '\0';
    return p;
}

/* Append to a malloc'd string */
char *
strapp(s1, s2)
    char **s1;
    const char *s2;
{
    unsigned len1 = 0;
    unsigned len2 = strlen(s2);

    if (*s1) {
	len1 = strlen(*s1);
	*s1 = (char *) realloc(*s1, len1 + len2 + 1);
    } else
	*s1 = (char *) malloc(len2 + 1);
    if (*s1)
	(void) strcpy(*s1 + len1, s2);
    return *s1;
}


#if !defined(POP3_SUPPORT) && !defined(HAVE_STRSTR)
/*
 * strstr - find first occurrence of ct in cs
 */
char *				/* found string, or NULL if none */
strstr(cs, ct)
     const char *cs;
     const char *ct;
{
  const char *scan;
  long len;
  char firstc;
  
  /*
   * The odd placement of the two tests is so "" is findable.
   * Also, we inline the first char for speed.
   * The ++ on scan has been moved down for optimization.
   */
  firstc = *ct;
  len = strlen(ct);
  for (scan = cs; *scan != firstc || strncmp(scan, ct, len) != 0; )
    if (*scan++ == '\0')
      return NULL;
  return (char *)scan;
}
#endif /* !POP3_SUPPORT && !HAVE_STRSTR */

char *
str_replace(ptr, s)
char **ptr;
const char *s;
{
    xfree(*ptr);
    if (!s)
	return NULL;
    return *ptr = savestr(s);
}
