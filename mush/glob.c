#include "mush.h"
#include "glob.h"

/*
 * Buried somewhere in here is the skeleton of a pattern matcher posted
 * by koblas@mips.COM (David Koblas).  It has been hacked almost beyond
 * recognition to handle more complex patterns, and directory search has
 * been added (patterns are split at '/' characters when file globbing).
 */

static int sglob();

#ifdef TEST	/* Define TEST to build a stand-alone file globbing program */

extern char *malloc(), *realloc();

#define getpath(x,y) (*(y) = 0, (x))
#ifndef Access
#define Access access
#endif
#define Strcpy(x,y) (strcpy(x,y), strlen(x))
#define savestr(x)  (strcpy(malloc(strlen(x)+1),x))
#ifndef max
#define max(x,y) ((x) > (y) ? (x) : (y))
#endif /* max */
#ifndef min
#define min(x,y) ((x) > (y) ? (y) : (x))
#endif /* min */
#define xfree free
#undef wprint
#define wprint printf
#define debug 0
#undef sprintf

#define TESTGLOB(str1,str2) \
	printf("%s %s = %s\n",str1,str2,glob(str1,str2)?"TRUE":"FALSE")

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
		columnate(f, e, 0);
	    }
	}
#ifdef TEST2	/* Define TEST2 to automatically run these test cases */
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
    TESTGLOB("./bc/def/gh/ij", "./*");
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

char *
any(s1, s2)
register char *s1, *s2;
{
    register char *p;
    if (!s1 || !*s1 || !s2 || !*s2)
	return 0;
    for( ; *s1; s1++) {
	for(p = s2; *p; p++)
	    if (*p == *s1)
		return s1;
    }
    return 0;
}

#endif /* TEST */

/*
 * Make a string into a one-element vector
 */
char **
unitv(s)
char *s;
{
    char **v;

    if (v = (char **)malloc((unsigned)(2 * sizeof(char *)))) {
	v[0] = savestr(s);
	v[1] = NULL;
    }
    return v;
}

/*
 * Append one vector to another
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
    else
        *v1 = (char **)malloc((unsigned)((s1+s2+1) * sizeof(char **)));

    if (*v1) {
	for (i = 0; i < s2 && v2[i]; i++)
	    (*v1)[s1 + i] = v2[i]; 
	(*v1)[s1 + i] = NULL;
	xfree(v2);
	return s1 + i;
    }
    return -1;
}

/*
 * A duplicate-eliminating comparison for sorting.  It treats an empty
 * string as greater than any other string, and forces empty one of any
 * pair of of equal strings.  Two passes are sufficient to move the empty
 * strings to the end where they can be deleted by the calling function.
 *
 * This is NOT compatible with the ANSI C qsort(), which requires that the
 * comparison function will not modify its arguments!
 */
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

/*
 * Expand a pattern into a list of file names.  Returns the number of
 * matches.  As in csh, names generated from pattern sets are returned
 * even if there are no actual matches.
 */
filexp(pat, exp)
char *pat, ***exp;
{
    char **t1, **t2;
    int n, new, cnt;

    if (!exp)
	return -1;
    if (!pat || !*pat)
	return 0;

    if ((n = sxp(pat, &t1)) > 0)
	cnt = 0;
    else
	return n;
    *exp = DUBL_NULL;
    while (n--)
	if ((new = fxp(t1[n], &t2)) > 0 || new++ == 0 && t2)
	    cnt = catv(cnt, exp, new, t2);
    if (cnt > 1) {
	/* Two sort passes to eliminate duplicates -- see uniqcmp() */
	qsort((char *)*exp, cnt, sizeof(char *), uniqcmp);
	qsort((char *)*exp, cnt, sizeof(char *), uniqcmp);
	while (!(*exp)[cnt - 1][0]) {
	    xfree((*exp)[--cnt]);
	    (*exp)[cnt] = NULL;
	}
    }
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
fxp(name, exp)
char *name, ***exp;
{
    char *p;
    int isdir;

    if (!exp)
	return -1;

    isdir = 1; /* ignore no such file */
    p = getpath(name, &isdir);
    if (isdir < 0)
	return -1;
    else if (isdir)
	return ((*exp = unitv(p)) ? 1 : -1);
    return pglob(p, 0, exp);
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
pglob(path, skip, exp)
char *path, ***exp;
int skip;
{
    char *t, *t2;
    int ret = 0;

    if (!path || !exp || skip < 0)
	return -1;
    *exp = DUBL_NULL; /* Must be null in case of zero matches and no sets */

    for (t = t2 = path + skip; (t2 = any(t2, META)) && *t2 == '/'; t = t2++)
	;
    if (!t2) {
	ret = ((*exp = unitv(path)) ? 1 : -1);
	if (ret > 0 && Access(path, F_OK) < 0)
	    ret = 0;
    } else {
	if (t2 = index(t + 1, '/'))
	    *t2++ = 0;
	if (*t == '/') {
	    *t++ = 0;
	    if (!*path)
		ret = dglob("/", t, t2, exp);
	    else
		ret = dglob(path, t, t2, exp);
	} else {
	    ret = dglob("", t, t2, exp);
	}
    }
    return ret;
}

/*
 * Search a directory (possibly recursively) for glob matches.
 * Argument pat1 is a pattern to be matched in this directory,
 * and pat2 is a pattern to be matched in matched subdirectories.
 *
 * Matches are returned through *exp.
 */
dglob(dir, pat1, pat2, exp)
char *dir, *pat1, *pat2, ***exp;
{
    DIR *dirp;
    struct dirent *dp;
    char *b, *d, buf[MAXPATHLEN], **tmp;
    int n, ret = 0, skip;

    if (!dir || !exp)
	return -1;
    d = (*dir ? dir : ".");
    if (!(dirp = opendir(d)))
	return -1;
    b = buf + Strcpy(buf, dir);
    if (b > buf && *(b - 1) != '/')
	*b++ = '/';
    skip = b - buf; /* We know this much matches, don't glob it again */
    while (ret >= 0 && (dp = readdir(dirp))) {
	if (fglob(dp->d_name, pat1)) {
	    if (pat2) {
		(void) sprintf(b, "%s/%s", dp->d_name, pat2);
		n = pglob(buf, skip, &tmp);
		ret = catv(ret, exp, n, tmp);
	    } else {
		(void) strcpy(b, dp->d_name);
		ret = catv(ret, exp, 1, unitv(buf));
	    }
	}
    }
    closedir(dirp);
    return ret;
}

/*
 * Match file names.  This means that metachars do not match leading ".".
 */
fglob(str, pat)
char *str, *pat;
{
    if (!str || !pat || *str == '.' && *pat != '.')
	return FALSE;
    else
	return glob(str, pat);
}

/*
 * Match two concatenated patterns.  Mainly for use by sglob().
 */
static
glob2(str, pat1, pat2)
char *str, *pat1, *pat2;
{
    char buf[MAXPATHLEN];

    if (!str || !pat1 && !pat2)
	return FALSE;
    (void) sprintf(buf, "%s%s", pat1? pat1 : "", pat2? pat2 : "");
    return glob(str, buf);
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
glob(str, pat)
char *str, *pat;
{
    int done = FALSE, ret = FALSE;

    if (!str || !pat)
	return FALSE;

    while (*pat && !done && (*str || (*pat == '{' || *pat == '*'))) /*}*/ {
	/*
	 * First look for a literal match, stepping over backslashes
	 * in the pattern to match against the "protected" character.
	 * Ordering and precendence are important in this expression!
	 */
	if (*pat == '\\' && *str == *++pat || *str == *pat) {
	    str++;
	    pat++;
	} else switch (*pat++) {
	    case '*':	/* Match any string */
		if (!*pat) {
		    while (*str)
			str++;
		    break;
		}
		/*
		 * Try the rest of the glob against every
		 * possible suffix of the string.  A bit
		 * inefficient in cases that eventually fail.
		 */
		while (*str && !(ret = glob(str++, pat)))
		    ;
		return ret;
		break;
	    case '[':	/* Match a set */
	    repeat:
		/* If we've hit the end of the set, give up. */
		if (!*pat || *pat == ']' || *pat == '\\' && !*++pat) {
		    done = TRUE;
		    break;
		}
		/* Check for a range. */
		if (*(pat + 1) == '-') {
		    char c = *pat++;
		    /* We don't handle open-ended ranges. */
		    if (*++pat == ']' || *pat == '\\' && !*++pat) {
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
		    if (*pat++ == '\\' && *pat)
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
		done = TRUE;
	}
    }
    while (*pat == '*')
	pat++;
    return ((*str == '\0') && (*pat == '\0'));
}

/*
 * Match a pattern set {s1,s2,...} followed by any other pattern.
 * Pattern sets and other patterns may nest arbitrarily.
 *
 * If "mat" is not a null pointer, a vector of possible expansions
 * is generated and placed in *mat; otherwise, the expansions are
 * matched against str and a truth value is returned ("/" is NOT
 * treated as a directory separator in this case).  NOTE: The vector
 * of expansions may still contain nested pattern sets, which must
 * be expanded separately.  See sxp().
 *
 * Currently allows at most 256 alternatives per set.  Enough? :-)
 */
static
sglob(str, pat, mat)
char *str, *pat, ***mat;
{
    char *p, *newpat[256], *oldpat[256], buf[MAXPATHLEN], *b = buf;
    int copy = 1, nest = 0, i = 0, ret = 0;

    if (!pat)
	return FALSE;

    while (*pat) {
	if (copy)
	    if (*pat != '{') /*}*/ {
		if (*pat == '\\' && pat[1])
		    *b++ = *pat++;
		*b++ = *pat++;
		continue;
	    } else {
		copy = 0;
		pat++;
	    }
	p = pat;
	while (*pat && (nest || *pat != ',' && /*{*/ *pat != '}')) {
	    if (*pat == '\\')
		pat++;
	    else if (*pat == '{')
		nest++;
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
 * Pre-expand pattern set notations so sets containing "/" separators
 * can be globbed successfully.  Returns the number of expansions.
 */
sxp(pat, exp)
char *pat, ***exp;
{
    char **t1 = DUBL_NULL, **t2;
    int n, new, cnt = 0;

    if ((n = sglob(NULL, pat, &t1)) < 2) {
	*exp = t1;
	return n;
    }
    *exp = DUBL_NULL;
    while (n-- && cnt >= 0) {
	new = sxp(t1[n], &t2);
	cnt = catv(cnt, exp, new, t2);
    }
    xfree(t1);
    return cnt;
}

/*
 * Generate the "glob difference" of two vectors (*argvp and patv).
 * The "glob difference" means to remove all strings from argv that
 * match any of the glob patterns in patv.
 *
 * Returns the number of strings remaining in *argvp.  The strings "removed"
 * from argv are actually left at the end of *argvp, so they can still be
 * accessed; their number will of course be argc - (returned value).
 */
gdiffv(argc, argvp, patc, patv)
int argc, patc;
char ***argvp, **patv;
{
    char **argv, *t;
    int ac, pc, oldac = argc;

    if (argc < 1 || patc < 1 || !patv || !*patv)
	return argc;
    if (!argvp || !(argv = *argvp) || !*argv)
	return -1;
    for (ac = 0; ac < argc && argv[ac]; ac++) {
	for (pc = 0; ac < argc && pc < patc && patv[pc]; pc++) {
	    /*
	     * We shouldn't cross '/' characters, so test
	     * only the "tail" of each element of argv.
	     */
	    if (!(t = rindex(argv[ac], '/')))
	        t = argv[ac];
	    if (glob(t, patv[pc])) {
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
     * Sort the two parts of the argv.  uniqcmp() works here only if
     * there already are no duplicates, but we assume that for now.
     */
    if (argc)
	qsort((char *)argv, argc, sizeof(char *), uniqcmp);
    if (oldac > argc)
	qsort((char *)&argv[argc], oldac - argc, sizeof(char *), uniqcmp);
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
lcprefix(vec, skip)
char **vec;
int skip;
{
    char c, **v;
    int done = FALSE;

    if (!vec || !*vec || skip < 0)
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
 * If "skip" is nonzero, that many chars are assumed to be in common
 * and are not printed.  WARNING: skip must be <= than the length of
 * the shortest string in the vector!  Safest to call with skip = 0.
 */
columnate(argc, argv, skip)
int argc;
char **argv;
int skip;
{
    int colstep, colwidth[MAXCOLS + 1];
    int maxcols = min(argc, MAXCOLS);
    int minwidth, maxwidth, *widths;
    int maxword = 0, n, c;

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
    xfree(widths);

    if (maxcols < 2 && minwidth <= MAXWIDTH / 2) {
	/*
	 * The maxword fills too much screen, so redo everything
	 * above it, print maxword, then do everything below it.
	 */
	if (maxword > 0 && columnate(maxword, argv, skip) < 0)
	    return -1;
	wprint("%s\n", argv[maxword] + skip);
	if (argc - maxword < 2)
	    return 0;
	return columnate(argc - maxword - 1, &argv[maxword + 1], skip);
    }

    for (n = 0; n < colstep; n++) {
	for (c = 0; c < maxcols && n + c * colstep < argc - colstep; c++)
	    wprint("%-*.*s", colwidth[c], colwidth[c],
					    argv[n + c * colstep] + skip);
	wprint("%s\n", argv[n + c * colstep] + skip);
    }

    return 0;
}

#ifndef DIRECTORY

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
char *name ;
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
      dir.d_namlen = strlen(dir.d_name) ;
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

#endif /* DIRECTORY */
