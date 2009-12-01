/* glob.c    Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef TEST
# include "zmail.h"
#else /* TEST */
# include <stdio.h>
# ifdef BSD
#  define HAVE_READDIR
#  define HAS_SYS_DIR
#  include <string.h>
#  include <sys/types.h>
#  include <sys/file.h>
# else /* !BSD */
#  if defined(MSDOS) || defined(MAC_OS)
#   define HAVE_READDIR
#   include <stddef.h>
#   include <string.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   ifndef MAC_OS
#    include <errno.h>
#   else /* MAC_OS */
#    include <sys/errno.h>
#   endif /* !MAC_OS */
#   ifndef F_OK
#    define F_OK 0
#   endif /* F_OK */
#   define       index   strchr
#   define       rindex  strrchr
#  endif /* MSDOS || MAC_OS */
# endif /* !BSD */

#endif /* TEST */

#include "glob.h"

#include <general.h>

#include <bfuncs.h>

/*
 * Buried somewhere in here is the skeleton of a pattern matcher posted
 * by David Koblas.  It has been hacked almost beyond 
 * recognition to handle more complex patterns, and directory search has
 * been added (patterns are split at '/' characters when file globbing).
 */

#ifdef TEST	/* Define TEST to build a stand-alone file globbing program */

extern char *malloc(), *realloc();

#define getpath(x,y) (*(y) = 0, (x))
#define Access access
#define Strcpy(x,y) (strcpy(x,y), strlen(x))
#define savestr(x)  (strcpy((char *)malloc(strlen(x)+1),x))
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif /* max */
#ifndef min
#define min(x,y) ((x) > (y) ? (y) : (x))
#endif /* min */
#define xfree free
#define free_vec(v) \
    do { int i; for (i = 0; (v) && (v)[i]; free((v)[i++])); } while (0)
#undef wprint
#define wprint printf
#define debug 0

#define DUBL_NULL (char **)0
#define TRPL_NULL (char ***)0
#define TRUE	1
#define FALSE	0
#define MAXPATHLEN 1024

#define TESTGLOB(str1,str2) \
	printf("%s %s = %s\n",str1,str2,zglob(str1,str2)?"TRUE":"FALSE")

main(argc, argv)
int argc;
char **argv;
{
    char **e;
    int f;

    if (argc > 1)
	while (*++argv) {
	    (void) printf("%s -->\n", *argv);
	    if (f = filexp(*argv, &e)) {
		(void) columnate(f, e, 0, (char ***)0);
	    }
	}
    else {
	char buf[1024];

	e = (char **)0;
	f = 0;
	while (fgets(buf, sizeof buf, stdin)) {
	    buf[strlen(buf)-1] = 0;
	    f = catv(f, &e, 1, unitv(buf));
	}
	(void) columnate(f, e, 0, (char ***)0);
    }
    /* Define TEST2 to automatically run these test cases */
#ifdef TEST2
    TESTGLOB("abcdefg", "abcdefg");
    TESTGLOB("abcdefg", "a?cd?fg");
    TESTGLOB("abcdefg", "ab[cde]defg");
    TESTGLOB("abcdefg", "ab[a-z]defg");
    TESTGLOB("abcdefg", "ab[a-z]defg");
    TESTGLOB("ab]defg", "ab[a]c]defg");
    TESTGLOB("ab]defg", "ab[a\\]c]defg");
    TESTGLOB("abcdefg", "ab*fg");
    TESTGLOB("./bc/def/gh/ij", "*de*");
    TESTGLOB("./der/den/deq/der/", "*deq*");
    TESTGLOB("./bc/def/gh/ij", "*ij");
    TESTGLOB("./ij", ".?ij");
    TESTGLOB("./bc/def/gh/ij", "./\052");	/* dot-slash-star */
    TESTGLOB("abcdef", "*def");
    TESTGLOB("abcdef", "*abcdef");
    TESTGLOB("abcdef", "abc*");
    TESTGLOB("abcdef", "abcdef*");
    TESTGLOB("abcdef", "*?*{xxx,,yy}");
    TESTGLOB("abcdef", "abcde{f}");
    TESTGLOB("abcdef", "abcdef{xxx,,yyy}");
    TESTGLOB("abcdef", "abc{def,qwrx}");
    TESTGLOB("abcdef", "abc{ab,def,qwrx}");
    TESTGLOB("abcdef", "{naqrwer,fuwnwer,as,abc,a}{ab,def,qwrx}");
    TESTGLOB("abcdef", "{naqrwer,*,as,abc,a}{ab,def,qwrx}");
    TESTGLOB("abcdef", "{{a*,b*},as,a}{ab,def,qwrx}");
    TESTGLOB("abcdef", "{{c*,b*},as,a}{ab,def,qwrx}");
    TESTGLOB("abcdef", "{{c*,?b*},as,a}{ab,def,qwrx}");
    TESTGLOB("abcdef", "{naqrwer,fuwnwer,as,abc,a}{ab,d*f,qwrx}");
#endif /* TEST2 */
}

/* for testing glob.c only */
char *
any(s1, s2)
register const char *s1, *s2;
{
    register const char *p;
    if (!s1 || !*s1 || !s2 || !*s2)
	return 0;
    for( ; *s1; s1++) {
	for(p = s2; *p; p++)
	    if (*p == *s1)
		return (char *) s1;
    }
    return 0;
}

/* for testing glob.c only */
char *
rany(s1, s2)
register const char *s1, *s2;
{
    register char *p, *p2 = s1;

    if (!s1 || !*s1 || !s2 || !*s2)
	return NULL;
    s1 += strlen(s1);		/* Skip to last character in s1 */
    while (s1-- > p2) {
	for (p = s2; *p; p++)
	    if (*p == *s1)
		return (const char *) s1;
    }
    return NULL;
}

/*
 * Copy a vector -- returns number of elements
 * for testing glob.c only
 */
vcpy(v1, v2)
    char ***v1;
    char **v2;
{
    int i, s2;

    if (!v1 || !v2)
	return -1;
    for (s2 = 0; v2[s2]; s2++)
	;
    *v1 = (char **)malloc((unsigned)((s2 + 1) * sizeof(char **)));
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
 * Append one vector to another -- frees v2
 * for testing glob.c only
 */
vcat(v1, v2)
char ***v1, **v2;
{
    int s1 = 0, s2 = 0;

    if (!v1)
	return -1;
    if (*v1)
	for (s1 = 0; (*v1)[s1]; s1++)
	    ;
    if (v2)
	for (s2 = 0; v2[s2]; s2++)
	    ;
    return catv(s1, v1, s2, v2);
}

/*
 * Speedier vector append when sizes are known
 * for testing glob.c only
 */
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
        *v1 = v2;	/* Do the obvious optimzation */
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
 * Make a string into a one-element vector
 * for testing glob.c only
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

#ifdef NOT_NOW
/*
 * A duplicate-eliminating comparison for sorting.  It treats an empty
 * string as greater than any other string, and forces empty one of any
 * pair of of equal strings.  Two passes are sufficient to move the empty
 * strings to the end where they can be deleted by the calling function.
 *
 * This is NOT compatible with the ANSI C qsort(), which requires that the
 * comparison function will not modify its arguments!
 *
 * for testing glob.c only
 */
int
uniqcmp(p1, p2)
char **p1, **p2;
{
    int cmp;

    if (**p1 && !**p2)
	return -1;
    if (**p2 && !**p1)
	return 1;
    if (cmp = strcmp(*p1, *p2))
	return cmp;
    **p2 = 0;
    return -1;
}
#endif /* NOT_NOW */

/*
 * String comparison function for qsort.  An empty string is greater
 * than any other, otherwise strcmp is used.
 *
 * for testing glob.c only
 */
int
strptrcmp(p1, p2)
     const char **p1, **p2;
{
    if (!**p1 || !**p2)
	return **p2 - **p1;
    return strcmp(*p1, *p2);
}

#endif /* TEST */

/*
 * Remove duplicate entries in a sorted array, usually the result of qsort.
 * Returns the number of unique entries, or -1 on error.
 * Moves the redundant stuff to the end, in case it needs to be deallocated
 * or something.
 */
int
crunch(base, nel, width, cmp)
char *base;
int nel, width, (*cmp)();
{
    int i, j;
    char *temp = (char *) malloc(width);

    if (!temp)
	return -1;

    j = 0;
    for (i = 0; i < nel; ++i) {
	if (i == 0 || (*cmp)(base+i*width, base+(j-1)*width) != 0) {
	    if (i != j) {
		bcopy(base+j*width, temp, width);
		bcopy(base+i*width, base+j*width, width);
		bcopy(temp, base+i*width, width);
	    }
	    j++;
	}
    }

    xfree(temp);

    return j;
}

/*
 * Qsort and remove duplicates.  Returns the final number of entries.
 */
int
qsort_and_crunch(base, nel, width, cmp)
char *base;
int nel, width, (*cmp)();
{
    qsort(base, nel, width, cmp);
    return crunch(base, nel, width, cmp);
}

/*
 * Expand a pattern into a list of file names.  Returns the number of
 * matches.  As in csh, names generated from pattern sets are returned
 * even if there are no actual matches.
 */
int
filexp(pat, exp)
const char *pat;
char ***exp;
{
    char **t1, **t2;
    int n, new, crunched_new, cnt;

    if (!exp)
	return -1;
    if (!pat || !*pat)
	return 0;

    /* Note that sxp() returns its list in reverse order, so we then
     * step through it in reverse order to complete the expansion.
     */
    if ((n = sxp(pat, &t1)) > 0)
	cnt = 0;
    else
	return n;
    *exp = DUBL_NULL;
    while (n--)
	if ((new = fxp(t1[n], &t2)) > 0 || new++ == 0 && t2) {
	    if (new > 1) {
		crunched_new =
		qsort_and_crunch((char *)t2, new, sizeof(char *), strptrcmp);
		while (new > crunched_new) {
		    xfree(t2[--new]);
		    t2[new] = NULL;
		}
	    }
	    cnt = catv(cnt, exp, new, t2);
	}
    free_vec(t1);
    return cnt;
}

/*
 * Expand a filename with globbing chars into a list of matching filenames.
 * Pattern set notatation which crosses directories is not handled, e.g.
 * "fi{le/exp,nger/h}and" will NOT expand to "file/expand finger/hand".
 * Such patterns must be pre-expanded by sxp() before calling fxp().
 *
 * The list of expansions is placed in *exp, and the number of matches
 * is returned, or -1 on an error.
 */
int
fxp(name, exp)
const char *name;
char ***exp;
{
    const char *p;
    int isdir;

    if (!exp)
	return -1;

#ifndef MAC_OS
    isdir = 1; /* ignore no such file */
    p = getpath(name, &isdir);
    if (isdir < 0)
	return -1;
    else if (isdir)
	return ((*exp = unitv(p)) ? 1 : -1);
    /* XXX casting away const */
#else /* MAC_OS */
    p = name;
#endif /* !MAC_OS */
    return pglob((char *)((*name == QNXT) ? name : p), 0, exp);
}

/*
 * Match all globbings in a path.  Mutually recursive with dglob(), below.
 * The first "skip" characters of the path are not globbed, see dglob().
 *
 * Returns the number of matches, or -1 on an error.  *exp is set to the
 * list of matches.
 *
 * If the path has no metachars, it is returned in *exp whether it matches
 * a real file or not.  This allows patterns built by sxp() to be recognized
 * and returned even when there are no matches (ala csh generation of names
 * from pattern sets).  pglob() still returns zero in this case.
 */
int
pglob(path, skip, exp)
char *path;
char ***exp;
int skip;
{
    char *t, *t2;
    int ret = 0;

    if (!path || !exp || skip < 0)
	return -1;
    *exp = DUBL_NULL; /* Must be null in case of zero matches and no sets */

    for (t = t2 = path + skip; (t2 = any(t2, META)); t = t2++) {
#ifdef NOT_NOW
	/* Bart: Wed Mar  2 14:22:46 PST 1994
	 * This was intended to allow quoting of the DSEP character,
	 * but since it isn't possible to include the DSEP in a file
	 * name on most systems, it only causes problems deeper in
	 * the glob package by failing to glob other metacharacters
	 * before calling down to dglob().
	 */
	if (*t2 == QNXT)
	    t2 += (t2[1]? 1 : 0);
	else	/* Remove "else" to allow quoted DSEPs without breaking
		 * the rest of the world, if for some reason we need to
		 * unwrap this #ifdef section.
		 */
#endif /* NOT_NOW */
	if (!is_dsep(*t2))
	    break;
    }
    if (!t2) {
	ret = ((*exp = unitv(path)) ? 1 : -1);
	if (ret > 0 && Access(path, F_OK) < 0)
	    ret = 0;
    } else {
	if (t2 = find_dsep(t + 1))
	    *t2++ = 0;
	if (is_dsep(*t)) {
	    *t++ = 0;
	    if (!*path)
		ret = dglob(SSLASH, t, t2, exp);
#if defined(MSDOS)
/* RJL ** 5.19.93 * DRIVE_SEP is in fsfix.h */
	    else if (path[1] == DRIVE_SEP && !path[2]) {
		char dos_path[4];
		strcpy(dos_path,path);
		strcat(dos_path, SSLASH);
		ret = dglob(dos_path, t, t2, exp);
	    }
#endif /* MSDOS */
	    else
		ret = dglob(path, t, t2, exp);
	} else {
	    ret = dglob("", t, t2, exp);
	}
    }
    /*
     * If we're going to return zero with something in exp, then we're
     * returning for an sxp() pattern as described above.  Strip the
     * QNXT character as if we'd actually found a file that matches.
     * Note that in this case we know there's only one element in exp.
     */
    if (ret == 0 && *exp && **exp) {
	for (t = t2 = exp[0][0]; t && *t; *t2++ = *t++) {
	    if (*t == QNXT && t[1])
		t++;
	}
	*t2 = 0;
    }
    return ret;
}

/*
 * Terminate a failing dglob() by gluing a path name back together out
 * of the several components.  If the reassembled path doesn't contain
 * any globbing metacharacters, add it to the exp list.  This deals with
 * the lowest level {foo,bar} pattern notation as described for pglob().
 *
 * Note that we know from the circumstances in which this is called by
 * dglob() that the exp array is empty within xglob()'s context.  This
 * function should be considered part of dglob() and should not be used
 * from other parts of the code.  If C could do nested function decls,
 * this would be a perfect candidate.
 */
static int
xglob(buf, b, pat1, pat2, exp)
char *buf, *b;
const char *pat1, *pat2;
char ***exp;
{
    while (*pat1) {
	if (index(FMETA, *pat1))
	    return 0;
	else if (*pat1 == QNXT && pat1[1])
	    pat1++;
	*b++ = *pat1++;
    }
    if (pat2) {
	*b++ = SLASH;
	while (*pat2) {
	    if (index(FMETA, *pat2))
		return 0;
	    else if (*pat2 == QNXT && pat2[1])
		pat2++;
	    *b++ = *pat2++;
	}
    }
    *b = 0;
    return catv(0, exp, 1, unitv(buf));
}

/*
 * Search a directory (possibly recursively) for glob matches.
 * Argument pat1 is a pattern to be matched in this directory,
 * and pat2 is a pattern to be matched in matched subdirectories.
 *
 * Matches are returned through *exp.
 */
int
dglob(dir, pat1, pat2, exp)
    const char *dir;
    const char *pat1, *pat2;
    char ***exp;
{
    DIR *dirp;
    struct dirent *dp;
    char *b, buf[MAXPATHLEN], **tmp;
    const char *d;
    int n, ret = 0, skip, hits = 0;

    if (!dir || !exp)
	return -1;

    b = buf + Strcpy(buf, dir);
    if (b > buf && !is_dsep(*(b - 1)))
	*b++ = SLASH;

#ifndef MAC_OS
    d = (*dir ? dir : ".");
#else /* MAC_OS */
    *b = 0;
    d = buf;
#endif /* !MAC_OS */

    errno = 0;
    if (!(dirp = opendir(d))) {
	if ((errno == ENOENT || errno == ENOTDIR) &&
		xglob(buf, b, pat1, pat2, exp) > 0)
	    return 0;
	return -1;
    }
    skip = b - buf; /* We know this much matches, don't glob it again */
    while (ret >= 0 && (dp = readdir(dirp))) {
	/* XXX casting away const */
	if (fglob(dp->d_name, (char *) pat1)) {
	    hits++;
	    if (pat2) {
		(void) sprintf(b, "%s%c%s", dp->d_name, SLASH, pat2);
		n = pglob(buf, skip + strlen(dp->d_name), &tmp);
		ret = catv(ret, exp, n, tmp);
	    } else {
		(void) strcpy(b, dp->d_name);
		ret = catv(ret, exp, 1, unitv(buf));
	    }
	}
    }
    closedir(dirp);
    if (hits == 0 && *pat1)
	(void) xglob(buf, b, pat1, pat2, exp);
    return ret;
}

/*
 * Match file names.  This means that metachars do not match leading ".".
 */
int
fglob(str, pat)
char *str, *pat;
{
#ifdef MSDOS	/* Case-insensitive globbing required; no leading "." */
    char cistr[MAXPATHLEN], cipat[MAXPATHLEN];
    if (!str || !pat || *str == '.' && *pat != '.')
	return FALSE;
    else
	return zglob(ci_strcpy(cistr, str), ci_strcpy(cipat, pat));
#else /* !MSDOS */
    if (!str || !pat || *str == '.' && *pat != '.')
	return FALSE;
    else
	return zglob(str, pat);
#endif /* !MSDOS */
}

/*
 * Match two concatenated patterns.  Mainly for use by sglob().
 */
static int
glob2(str, pat1, pat2)
char *str, *pat1, *pat2;
{
    char buf[MAXPATHLEN];

    if (!str || !pat1 && !pat2)
	return FALSE;
    (void) sprintf(buf, "%s%s", pat1? pat1 : "", pat2? pat2 : "");
    return zglob(str, buf);
}

/*
 * Match a pattern set {s1,s2,...} followed by any other pattern.
 * Pattern sets and other patterns may nest arbitrarily.
 *
 * If "mat" is not a null pointer, a vector of possible expansions
 * is generated and placed in *mat; otherwise, the expansions are
 * matched against str and a truth value is returned (SLASH is NOT
 * treated as a directory separator in this case).  NOTE: The vector
 * of expansions may still contain nested pattern sets, which must
 * be expanded separately.  See sxp().
 *
 * Currently allows at most 256 alternatives per set.  Enough? :-)
 */
static int
sglob(str, pat, mat)
    const char *str;
    char *pat, ***mat;
{
    char *p, *newpat[256], *oldpat[256], buf[MAXPATHLEN], *b = buf;
    int copy = 1, had_nest = 0, nest = 0, i = 0, ret = 0;

    if (!pat)
	return FALSE;

    while (*pat) {
	if (copy)
	    if (*pat != '{') /*}*/ {
		if (*pat == QNXT && pat[1])
		    *b++ = *pat++;
		*b++ = *pat++;
		continue;
	    } else {
		copy = 0;
		pat++;
	    }
	p = pat;
	while (*pat && (nest || *pat != ',' && /*{*/ *pat != '}')) {
	    if (*pat == QNXT)
		pat++;
	    else if (*pat == '{')
		had_nest = nest++;
	    else if (*pat == '}')
		nest--;
	    if (*pat)
		pat++;
	}
	if (*pat) {
	    oldpat[i] = pat;
	    newpat[i++] = p;
	    if (*pat != ',') {
		*pat++ = 0;
		break;
	    } else
		*pat++ = 0;
	}
    }
    oldpat[i] = NULL;
    if (i > 0 && mat) {
	*mat = (char **)malloc((unsigned)((i + 1) * sizeof(char *)));
	if (*mat)
	    (*mat)[i] = NULL;
	else
	    return -1;
	ret = i;
    }
    while (!mat && i-- > 0)
	if (ret = glob2(str, newpat[i], pat))
	    break;
    for (i = 0; oldpat[i]; i++) {
	if (mat && *mat) {
	    (void) sprintf(b, "%s%s", newpat[i], pat);
	    (*mat)[i] = savestr(buf);
	}
	if (oldpat[i + 1])
	    oldpat[i][0] = ',';
	else
	    oldpat[i][0] = /*{*/ '}';
    }
    if (ret == 0 && b > buf && mat) {
	*b = 0;
	ret = ((*mat = unitv(buf)) ? 1 : -1);
    }
    return ret;
}

/*
 * The basic globbing matcher.
 *
 * "*"           = match 0 or more occurances of anything
 * "[abc]"       = match any of "abc" (ranges supported)
 * "{xx,yy,...}" = match any of "xx", "yy", ... where
 *                 "xx", "yy" can be any pattern or empty
 * "?"           = match any character
 */
int
zglob(str, pat)
    const char *str, *pat;
{
    int done = FALSE, ret = FALSE;

    if (!str || !pat)
	return FALSE;

    while (*pat && !done && (*str || (*pat == '{' || *pat == '*'))) /*}*/ {
	char c;
	/*
	 * First look for a literal match, stepping over backslashes
	 * in the pattern to match against the "protected" character.
	 */
	if (*pat == QNXT && *str == *++pat) {
	    str++;
	    pat++;
	} else switch (c = *pat++) {
	    case '*':	/* Match any string */
		/* Any number of consecutive '*' are equivalent */
		while (*pat == '*')
		    pat++;
		if (!*pat)
		    return TRUE;
		/*
		 * Try the rest of the glob against every
		 * possible suffix of the string.  A bit
		 * inefficient in cases that eventually fail.
		 */
		while (*str && !(ret = zglob(str++, pat)))
		    ;
		return ret;
		break;
	    case '[':	/* Match a set */
	    repeat:
		/* If we've hit the end of the set, give up. */
		if (!*pat || *pat == ']' || *pat == QNXT && !*++pat) {
		    done = TRUE;
		    break;
		}
		/* Check for a range. */
		if (pat[1] == '-') {
		    c = *pat++;
		    /* We don't handle open-ended ranges. */
		    if (*++pat == ']' || *pat == QNXT && !*++pat) {
			done = TRUE;
			break;
		    }
		    if (*str < c || *str > *pat) {
			pat++;
			goto repeat;
		    }
		} else if (*pat != *str) {
		    pat++;
		    goto repeat;
		}
		/*
		 * We matched either the range or a literal member of
		 * the set.  Skip to the end of the set.
		 */
		pat++;
		while (*pat && *pat != ']')
		    if (*pat++ == QNXT && *pat)
			pat++;
		/*
		 * If no pattern remains, the set was never closed,
		 * so don't increment.  This will cause a FALSE return.
		 */
		if (*pat) {
		    pat++;
		    str++;
		}
		break;
	    case '?':	/* Match any one character */
		str++;
		break;
	    case '{':	/* } Match any of a set of patterns */
		return sglob(str, pat - 1, TRPL_NULL);
		break;
	    default:
		if (*str == c)
		    str++;
		else
		    done = TRUE;
	}
    }
    /* Any number of trailing '*' are equivalent */
    while (*pat == '*')
	pat++;
    return ((*str == '\0') && (*pat == '\0'));
}

/*
 * Pre-expand pattern set notations so sets containing DSEP separators
 * can be globbed successfully.  Returns the number of expansions.
 */
int
sxp(pat, exp)
    const char *pat;
    char ***exp;
{
    char **t1 = DUBL_NULL, **t2;
    int n, new, cnt = 0;

    if ((n = sglob(NULL, pat, &t1)) < 2) {
	char *b = NULL;

	/*
	 * Determine whether we need to do a recursive expansion.
	 */
	if (t1 && t1[0]) {
	    for (pat = t1[0]; *pat && (b = index(pat, '{' /*}*/)); pat++) {
		if (b == t1[0] || b[-1] != QNXT)
		    break;
		else
		    pat = b;
	    }
	}
	if (!b) {
	    *exp = t1;
	    return n;
	}
    }
    *exp = DUBL_NULL;
    while (n-- && cnt >= 0) {
	new = sxp(t1[n], &t2);
	cnt = catv(cnt, exp, new, t2);
    }
    free_vec(t1);
    return cnt;
}

/*
 * Generate the "glob difference" of two vectors (*argvp and patv).
 * The "glob difference" means to remove all strings from argv that
 * match any of the glob patterns in patv.
 *
 * Consecutive slashes in the pattern match literally!  This is not a true
 * file path "glob".  DOS drive specifiers are also matched literally, and
 * metacharacters may match '/' characters in some circumstances.
 *
 * Returns the number of strings remaining in *argvp.  The strings "removed"
 * from argv are actually left at the end of *argvp, so they can still be
 * accessed; their number will of course be argc - (returned value).
 */
int
gdiffv(argc, argvp, patc, patv)
int argc, patc;
char ***argvp, **patv;
{
    char **argv, *t, *h;
    int ac, pc, oldac = argc;

    if (argc < 1 || patc < 1 || !patv || !*patv)
	return argc;
    if (!argvp || !(argv = *argvp) || !*argv)
	return -1;
    for (ac = 0; ac < argc && argv[ac]; ac++) {
	for (pc = 0; ac < argc && pc < patc && patv[pc]; pc++) {
	    int l = strlen(argv[ac]);
	    if (l == 0)
		continue;
	    /*
	     * We shouldn't cross DSEP characters unless they appear
	     * in the pattern, so test only as much of the "tail" of
	     * each element of argv as necessary.  If the first char
	     * of the pattern is a DSEP, match from the beginning.
	     */
	    h = patv[pc];
	    if (!is_dsep(*h)) {
		for (t = &argv[ac][--l]; t > argv[ac]; t--) {
		    if (is_dsep(*t) && !((h = find_dsep(h)) && h++)) {
			t++;
			break;
		    }
		}
	    } else
		t = argv[ac];
	    if (zglob(t, patv[pc])) {
		/* Move matches to the end and reduce argc */
		t = argv[ac];
		argv[ac] = argv[--argc];
		argv[argc] = t;
		/* Start patterns over on the new string */
		pc = -1; /* It'll be incremented to 0 */
	    }
	}
    }
    /*
     * Sort the two parts of the argv.
     */
    if (argc)
	qsort((char *)argv, argc, sizeof(char *),
	      (int (*) NP((CVPTR, CVPTR))) strptrcmp);
    if (oldac > argc)
	qsort((char *)&argv[argc], oldac - argc, sizeof(char *),
	      (int (*) NP((CVPTR, CVPTR))) strptrcmp);
    return argc;
}

/*
 * Generate the longest common prefix from all strings in a vector
 * If "skip" is nonzero, that many chars are assumed to be in common
 * and are not tested.  WARNING: skip must be <= than the length of
 * the shortest string in the vector!  Safest to call with skip = 0.
 *
 * Returns the length of the longest common prefix.
 */
int
lcprefix(vec, skip)
char **vec;
int skip;
{
    char c, **v;
    int done = FALSE;

    if (!vec || !*vec || !vec[1] || skip < 0)
	return 0;
    do {
	for (v = vec + 1, c = vec[0][skip]; c && *v; v++)
	    if (v[0][skip] != c) {
		done = TRUE;
		break;
	    }
    } while (!done && c && ++skip);
    return skip;
}

#define MAXCOLS 8	/* Max number of columns of words to make */
#define MINWIDTH 10	/* Minimum width of each column of words */
#ifdef CURSES
#define MAXWIDTH (iscurses? COLS : 80)
#else /* CURSES */
#define MAXWIDTH 80	/* Maximum width of all columns */
#endif /* CURSES */

/*
 * Print a vector in columns
 *
 * If "outv" is non-null, it will be loaded with an allocated vector
 * where each element is one line of the columnated text to be output.
 * Strings in outv do NOT include a trailing newline!  Returns the number
 * of elements in outv (or the number of lines printed if outv is null).
 *
 * If "skip" is nonzero, that many chars are assumed to be in common
 * and are not printed.  WARNING: skip must be <= than the length of
 * the shortest string in the vector!  Safest to call with skip = 0.
 */
int
columnate(argc, argv, skip, outv)
int argc;
char **argv, ***outv;
int skip;
{
    int colstep, colwidth[MAXCOLS + 1];
    int maxcols = min(argc, MAXCOLS);
    int minwidth, maxwidth, *widths;
    int maxword = 0, n, c, lines = 0;
    char *out = NULL;

    if (argc <= 0 || !argv || !*argv)
	return -1;
    if (!(widths = (int *)malloc((unsigned)((argc + 1) * sizeof(int)))))
	return -1;

    /*
     * Compute the widths of all words in the vector, and
     * remember the maximum width and which word had it.
     * Also remember the minimum width.
     */
    for (minwidth = MAXWIDTH, maxwidth = n = 0; n < argc; n++) {
	widths[n] = max(strlen(argv[n] + skip) + 2, MINWIDTH);
	if (widths[n] > MAXWIDTH - MINWIDTH)
	    break;
	if (widths[n] > maxwidth) {
	    maxwidth = widths[n];
	    maxword = n;
	}
	if (widths[n] < minwidth)
	    minwidth = widths[n];
    }

    for (; maxcols > 0; maxcols--) {
	if (argc % maxcols)
	    colstep = argc / maxcols + 1;
	else
	    colstep = argc / maxcols;
	colwidth[MAXCOLS] = 0;
	for (c = 0; c < maxcols; c++) {
	    colwidth[c] = 0;
	    for (n = c * colstep; n < (c + 1) * colstep && n < argc; n++)
		colwidth[c] = max(colwidth[c], widths[n]);
	    colwidth[MAXCOLS] += colwidth[c];
	}
	if (colwidth[MAXCOLS] <= MAXWIDTH)
	    break;
    }
    xfree((char *)widths);

    if (outv)
	*outv = DUBL_NULL; /* for catv() */

    if (maxcols < 2 && minwidth <= MAXWIDTH / 2) {
	char **tmpv = DUBL_NULL;
	/*
	 * If the maxword fills too much screen, redo everything
	 * above it, print maxword, then do everything below it.
	 */
	if (maxword > 0 && (lines = columnate(maxword, argv, skip, outv)) < 0)
	    return -1;
	if (outv) {
	    if ((n = catv(lines, outv, 1, unitv(argv[maxword] + skip))) < 0) {
		if (*outv)
		    free_vec(*outv);
		*outv = DUBL_NULL;
		return -1;
	    } else
	        lines = n;
	} else
	    wprint("%s\n", argv[maxword] + skip);
	if (argc - maxword < 2)
	    return lines;
	else if (outv)
	    tmpv = *outv;
	n = columnate(argc - maxword - 1, &argv[maxword + 1], skip, outv);
	if (tmpv) {
	    if (n < 0 || (n = catv(lines, &tmpv, n, *outv)) < 0) {
		free_vec(tmpv);
		*outv = DUBL_NULL;
	    } else
		*outv = tmpv;
	}
	return n;
    }

    if (!(out = (char *) malloc((unsigned)(MAXWIDTH + 1)))) {
	if (outv && *outv)
	    free_vec(*outv);
	if (outv)
	    *outv = DUBL_NULL;
	return -1;
    }
    for (n = 0; n < colstep; n++) {
	char *p = out;
	for (c = 0; c < maxcols && n + c * colstep < argc - colstep; c++) {
	    (void) sprintf(p, "%-*.*s", colwidth[c], colwidth[c],
					    argv[n + c * colstep] + skip);
	    p += strlen(p);
	}
	sprintf(p, "%s", argv[n + c * colstep] + skip);
	if (outv) {
	    if ((lines = catv(lines, outv, 1, unitv(out))) < 0) {
		if (*outv)
		    free_vec(*outv);
		*outv = DUBL_NULL;
		return -1;
	    }
	} else
	    wprint("%s\n", out);
    }
    xfree(out);

    return lines;
}

#ifndef HAVE_READDIR

#undef NULL
#define NULL 0

/*
 *  4.2BSD directory access emulation for non-4.2 systems.
 *  Based upon routines in appendix D of Portable C and Unix System
 *  Programming by J. E. Lapin (Rabbit Software).
 *
 *  No responsibility is taken for any error in accuracies inherent
 *  either to the comments or the code of this program, but if
 *  reported to me then an attempt will be made to fix them.
 */

/*  Support for Berkeley directory reading routines on a V7/SysV file
 *  system.
 */

/*  Open a directory. */

DIR *
opendir(name)
const char *name ;
{
  register DIR *dirp ;
  register int fd ;

  if ((fd = open(name, 0)) == -1) return NULL ;
  if ((dirp = (DIR *) malloc(sizeof(DIR))) == NULL)
    {
      close(fd) ;
      return NULL ;
    }
  dirp->dd_fd = fd ;
  dirp->dd_loc = 0 ;
  return dirp ;
}


/*  Read an old style directory entry and present it as a new one. */

#define  ODIRSIZ  14

struct olddirent
{
  short  od_ino ;
  char   od_name[ODIRSIZ] ;
} ;


/*  Get next entry in a directory. */

struct dirent *
readdir(dirp)
register DIR *dirp ;
{
  register struct olddirent *dp ;
  static struct dirent dir ;

  for (;;)
    {
      if (dirp->dd_loc == 0)
        {
          dirp->dd_size = read(dirp->dd_fd, dirp->dd_buf, DIRBLKSIZ) ;
          if (dirp->dd_size <= 0) return NULL ;
        }
      if (dirp->dd_loc >= dirp->dd_size)
        {
          dirp->dd_loc = 0 ;
          continue ;
        }

      dp = (struct olddirent *)(dirp->dd_buf + dirp->dd_loc) ;
      dirp->dd_loc += sizeof(struct olddirent) ;

      if (dp->od_ino == 0) continue ;

      dir.d_fileno = dp->od_ino ;
      strncpy(dir.d_name, dp->od_name, ODIRSIZ) ;
      dir.d_name[ODIRSIZ] = '\0' ;       /* Ensure termination. */
      dir.d_namlen = strlen(dir.d_name) + 1;
      dir.d_reclen = DIRSIZ(&dir) ;
      return(&dir) ;
    }
}


/*  Close a directory. */

void
closedir(dirp)
register DIR *dirp ;
{
  close(dirp->dd_fd) ;
  dirp->dd_fd = -1 ;
  dirp->dd_loc = 0 ;
  xfree(dirp) ;
}

#endif /* HAVE_READDIR */
