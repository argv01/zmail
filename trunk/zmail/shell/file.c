/* file.c    Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "catalog.h"
#include "file.h"
#include "fsfix.h"
#include "maxfiles.h"
#include "zcfctl.h"
#include "zmcomp.h"
#include <ctype.h>

#ifndef MAC_OS
# include <pwd.h>
#else /* MAC_OS */
# include "sys/fcntl.h"
#endif /* !MAC_OS */

#include <general.h>

/*
 * Expand tilde-expression file names from p into buf and return buf for
 * success or NULL for failure.  In case of failure, an error message is
 * written into buf.  It is not an error for p to not begin with "~".
 */
char *
homepath(buf, p)
char *buf, *p;	/* "buf" should usually be MAXPATHLEN+1 large */
{
    char *home;

    if (!p || !buf) {
	if (buf)
	    (void) strcpy(buf, catgets( catalog, CAT_SHELL, 314, "invalid argument (NULL)" ));
	return NULL;
    }
    if (*p != '~')
	return pathcpy(buf, p);

    if (!p[1]) {
	home = value_of(VarHome);
	if (!home || !*home)
	    home = ALTERNATE_HOME;
	(void) strcpy(buf, home);
    }
#ifdef MAC_OS	
    else {
    	home = value_of(VarTmpdir);
	if (home) {
	    if (is_dsep(p[1]) || p[1] == '/')
	    	(void) sprintf(buf, "%s%c%s", home, SLASH, p+2);
	    else strcpy(buf, home);

	} else return NULL;
	return buf;
    }
#else
	else if (!is_dsep(p[1])) {
	/* not our home, but someone else's
	 * look for ~user or ~user/subpath
	 * if '/' exists, separate into tmp="user" p="subpath"
	 */
	struct passwd *ent, *getpwnam();
	char *p2 = p+1;
	if (p = find_dsep(p2))
	    *p++ = 0;
	if (!(ent = getpwnam(p2))) {
	    errno = ENOTDIR;
	    (void) sprintf(buf, catgets(catalog, CAT_SHELL, 315, 
					"no such user: %s"), p2);
	    return NULL;
	}
	/* append subpath to pathname */
	if (p && *p)
	    (void) sprintf(buf, "%s%c%s", ent->pw_dir, SLASH, p);
	/* if *p == NULL, pathname is done (buf) */
	else
	    (void) pathcpy(buf, ent->pw_dir);
    } else {
	home = value_of(VarHome);
	if (!home || !*home)
	    home = ALTERNATE_HOME;
	(void) sprintf(buf, "%s%c%s", home, SLASH, p+2);
    }
#endif /* MAC_OS */
    return buf;
}

/*
 * As homepath(), but for the user's spool file
 */
char *
spoolpath(buf, p)
char *buf, *p;	/* "buf" should usually be MAXPATHLEN+1 large */
{
    if (!p || !buf) {
	if (buf)
	    (void) strcpy(buf, catgets( catalog, CAT_SHELL, 314, "invalid argument (NULL)" ));
	return NULL;
    }
    if (*p != '%')
	return pathcpy(buf, p);

    /* if %user, append user name... else, it's just us */
    if (!*++p || *p == ' ' || *p == '\t')
	(void) pathcpy(buf, spoolfile);
    else {
#ifdef HOMEMAIL
	char tmp[MAXPATHLEN];
	/* If it's NOT us, get the path for ~user/MAILFILE */
	(void) sprintf(tmp, "~%s%c%s", p, SLASH, MAILFILE);
	return homepath(buf, tmp);
#else /* !HOMEMAIL */
	(void) sprintf(buf, "%s%c%s", spooldir, SLASH, p);
#endif /* HOMEMAIL */
    }
    return buf;
}

/*
 * As homepath(), but for files in the folder directory
 */
char *
pluspath(buf, p)
char *buf, *p;	/* "buf" should usually be MAXPATHLEN+1 large */
{
    register char *p2 = value_of(VarFolder);

    if (!p || !buf) {
	if (buf)
	    (void) strcpy(buf, catgets( catalog, CAT_SHELL, 314, "invalid argument (NULL)" ));
	return NULL;
    }
    if (*p != '+')
	return pathcpy(buf, p);

    if (!p2 || !*p2)
	p2 = DEF_FOLDER;
    else if (*p2 == '+') {
	(void) strcpy(buf, catgets( catalog, CAT_SHELL, 318, "cyclic reference (+)" ));
	return NULL;
    }
    while (*++p == SLASH)	/* Catch common user error == +/basename; */
	;			/* eliminate leading slashes in basename. */
    if (*p)
	(void) sprintf(buf, "%s%c%s", p2, SLASH, p);
    else
	(void) pathcpy(buf, p2);

    if (!is_fullpath(buf)) {
	char tmp[MAXPATHLEN];

	if (*buf != '~')
	    (void) sprintf(tmp, "~%c%s", SLASH, buf);
	else
	    (void) pathcpy(tmp, buf);
	return homepath(buf, tmp);
    } else
	return buf;
}

/*
 * As homepath() but for the mbox.
 *
 * As a special case, when both p and buf are NULL, return the value
 * that "&" expands to (not necessarily a full path).
 */
char *
mboxpath(buf, p)
char *buf, *p;
{
    char *mbox;

    mbox = value_of(VarMbox);
    if (!mbox || !*mbox)
	mbox = DEF_MBOX;

    if (!p || !buf) {
	if (buf)
	    (void) strcpy(buf, catgets( catalog, CAT_SHELL, 314, "invalid argument (NULL)" ));
	return buf? (char *) NULL : mbox;
    }
    if (p[0] != '&' || p[1])
	return pathcpy(buf, p);
    else if (*mbox == '&')
	return pathcpy(buf, mbox);	/* No infinite recursions ... */

    /* Have to use pathstat() to expand abbrevs. recursively */
    if (pathstat(mbox, buf, (struct stat *)0) < 0)
	return NULL;
    return buf;
}

struct pathget {
    char *abbrev, *(*pathfunc)(/* char *dst, char *src */);
} pathtab[] = {
    {	"~",	homepath	},
    {	"+",	pluspath	},
    {	"%",	spoolpath	},
    {	"&",	mboxpath	},
    {	0,	0		}
};

/*
 * Get file statistics and path name.  If p uses shorthand to reference
 * a home directory or folder of some sort, then expand it.
 *
 * If s_buf is NULL, ignore the stat and just expand everything.
 *
 * The pathtab should probably be an argument.				XXX
 */
int
pathstat(p, buf, s_buf)
const char *p;
char *buf;
struct stat *s_buf;
{
    int expanded = 0;
    struct pathget *p_ent;

    if (!p || !buf) {
	if (buf)
	    (void) strcpy(buf, catgets( catalog, CAT_SHELL, 314, "invalid argument (NULL)" ));
	return -1;
    }
    if (*p == QNXT)  /* QNXT escapes the special chars */
	(void) pathcpy(buf, ++p);
    else {
	/* check for each path metachar */
	for (p_ent = pathtab; p_ent->abbrev; ++p_ent) {
	    /*
	     * For full generality, should use this:
	    if (strncmp(p, p_ent->abbrev, strlen(p_ent->abbrev)))
	     */
	    if (*p == *p_ent->abbrev) {
		if (!(*p_ent->pathfunc)(buf, p))
		    return -1;
		expanded = 1;
		break;
	    }
	}
	if (!expanded)
	    (void) pathcpy(buf, p);
    }

    return s_buf? stat(buf, s_buf) : 0;
}

/*
 * "Safe" interface to pathstat() that handles NULL and in-place copy
 */
int
getstat(p, buf, s_buf)
const char *p;
char *buf;
struct stat *s_buf;
{
    char tmp[MAXPATHLEN];

    if (!p || !*p)
	p = "~";  /* no arg means home */
    else if (p == buf)
	p = pathcpy(tmp, p);
    if (!buf)
	buf = tmp;	/* Just get the stat?? */

    return pathstat(p, buf, s_buf);
}

/*
 * Do getstat() relative to a directory if file is not a full path.
 * Return the complete path name in buf and the stat info in s_buf.
 */
int
dgetstat(dir, file, buf, s_buf)
const char *dir, *file;
char *buf;
struct stat *s_buf;
{
    char tmp[MAXPATHLEN];

    if (dir && *dir) {
	if (file && *file) {
	    int slash = !is_dsep(dir[strlen(dir)-1]);
	    if (buf == NULL || getstat(file, buf, (struct stat *)0) < 0)
		return -1;
	    if (!is_fullpath(buf)) {
		(void) sprintf(tmp, "%s%s%s", dir, slash? SSLASH : "", buf);
		file = tmp;
	    } else
		file = buf;	/* getstat() handles in-place copy */
	} else
	    file = dir;
    }

    return getstat(file, buf, s_buf);
}

/* Takes string 'p' and address of int (isdir).  Find out what sort of
 * file final path is.  Set isdir to ZmGP_Dir if a directory,
 * ZmGP_File if not, ZmGP_Error on error.  Return final path.  If an
 * error occurs, return string indicating error.  If isdir has a value
 * of ZmGP_IgnoreNoEnt when passed, ignores "No such file or directory"
 * and sets isdir to ZmGP_File.  Set isdir to ZmGP_DontIgnoreNoEnt to
 * avoid this behavior.
 *
 * Always returns a pointer to the static buffer!  This is unfortunately
 * relied upon in some parts of the code ...
 */
char *
getpath(p, isdir)
const char *p;
int *isdir;
{
    static char buf[MAXPATHLEN];
    struct stat stat_buf;

    if (getstat(p, buf, &stat_buf)) {
#if !defined(MSDOS) && !defined(MAC_OS)
	if (errno != ENOTDIR)
	    (void) access(buf, F_OK); /* set errno to the "real" reason */
#endif /* !MSDOS && !MAC_OS*/
	if (errno == ENOENT && *isdir == ZmGP_IgnoreNoEnt) {
	    /* say it's a regular file even tho it doesn't exist */
	    *isdir = ZmGP_File;
	    return buf; /* it may be wanted for creating */
	}
	*isdir = ZmGP_Error;
	if (errno != ENOTDIR)
	    (void) strcpy(buf, strerror(errno));
    } else
	*isdir = ((stat_buf.st_mode & S_IFMT) == S_IFDIR) ?
	    ZmGP_Dir : ZmGP_File;
    return buf;
}

/*
 * Expand variables and then dgetstat().  Return -2 for varexp() failure.
 */
int
dvarstat(dir, file, buf, s_buf)
const char *dir, *file;
char *buf;
struct stat *s_buf;
{
    char buf2[MAXPATHLEN], *b = buf2;
    const char *p = file;
    struct expand expansion;

    *b = 0;
    while (p && (*b = *p++)) {
	if (*b == QNXT) {
	    if ((*++b = *p++) == 0)
		break;
	} else if (*b == '$') {
	    /* XXX casting away const */
	    expansion.orig = (char *) --p;
	    if (varexp(&expansion)) {
		b += Strcpy(b, expansion.exp);	/* XXX Overflow? */
		xfree(expansion.exp);
		p = expansion.rest;
	    } else {
		return -2;
	    }
	} else
	    b++;
    }
    return dgetstat(dir, buf2, buf, s_buf);
}

int
varstat(file, buf, s_buf)
    const char *file;
    char *buf;
    struct stat *s_buf;
{
    return dvarstat(NULL, file, buf, s_buf);
}

/*
 * Expand variables and then getpath()
 */
char *
varpath(path, isdir)
const char *path;
int *isdir;
{
    static char buf[MAXPATHLEN];
    struct stat stat_buf;
    int r;

    if (r = varstat(path, buf, &stat_buf)) {
	if (r > -2) {
	    if (errno != ENOTDIR)
		(void) access(buf, F_OK); /* set errno to the "real" reason */
	    if (errno == ENOENT && *isdir == 1) {
		*isdir = 0; /* say it's a file even tho it doesn't exist */
		return buf; /* it may be wanted for creating */
	    }
	    if (errno != ENOTDIR)
		(void) strcpy(buf, strerror(errno));
	} else
	    (void) strcpy(buf, catgets(catalog, CAT_SHELL, 321, 
				       "Variable expansion failed"));
	*isdir = -1;
    } else
	*isdir = ((stat_buf.st_mode & S_IFMT) == S_IFDIR);
    return buf;
}

/* Given a file path, eliminate multiple directory separators and
 * references to "." and ".." if they occur.
 *
 * Does not check for actual existence of the file!
 *
 * WARNING: assumes all DSEP have been normalized to SLASH on platforms
 * where there is more than one DSEP.  See dos/fsfix.c:dos_copy_path().
 */
char *
pathclean(dst, src)
char *dst;
const char *src;
{
    const char *p = src;
    char *b = dst;
    int slash = 0;

    /* Leave leading consecutive slashes alone because of weird
     * operating systems that refer to NFS mounts with //machine
     */
    while (*p == SLASH) {
	*b++ = *p++;
	slash++;
    }

    while (*p) {
	if (p[0] == '.' && (p[1] == SLASH || !p[1]))
	    p += 2 - !p[1];
	else if (strncmp(p, "..", 2) == 0 && (p[2] == SLASH || !p[2])) {
	    p += 3 - !p[2];
	    while (b > dst+slash && *--b == SLASH)
		;
	    while (b > dst && *--b != SLASH )
		;
	    ++b;
	} else
	    do {
		*b++ = *p;
	    } while (*p++ != SLASH && *p);
	/* Eliminate multiple consecutive slashes */
	while (*p == SLASH)
	    p++;
    }
    *b = 0;
    while (b > dst+slash && *--b == SLASH )
	*b = 0;

    return dst;
}

/* Given a file name, return a full path to that filename, eliminating
 * references to "." and ".." if they occur.  Does not check for actual
 * existence of the file!  If hardpath is nonzero, performs chdir() and
 * GetCwd() to eliminate symbolic links.
 *
 * For MSDOS, also normalizes the path to have only SLASH (no other DSEP).
 *
 * Handles in-place copy (newpath == path), which is used by fullpath().
 */
char *
expandpath(path, newpath, hardpath)
const char *path;
char *newpath;
int hardpath;
{
    const char *p;
    char *b, buf[MAXPATHLEN];

    if (newpath && newpath != path)
	*newpath = 0;
    if (!path)
	return NULL;

    if (*path == '~' || *path == '+' || *path == '&' || *path == '%') {
	if (getstat(path, buf, 0) >= 0)
	    path = pathcpy(newpath, buf);
	else
	    return NULL;
    }

#if defined(MAC_OS)
    /* Bart: Thu Apr 29 14:12:37 PDT 1993
     * Move this below the check for special characters and #else the unused
     * part of the function.  And, reformat the comment.  I checked with GF.
     */
    if (path != newpath)
	(void) pathcpy(newpath, path);
    return newpath;
#else /* !MAC_OS */

#if !defined(MSDOS)
    /* Paths are always "hard" in MSDOS, so don't muck with chdir() */
    if (!hardpath) {
	p = path;
#else /* MSDOS */
	p = path = dos_copy_path(newpath, path); /* guaranteed safe */
	/* RJL ** 5.19.93 - '/' replaced with SLASH macro throughout */

        /* if path starts with drive id */
        if (*path != SLASH && path[1] == ':') {
	    p = path + 2;
	    if (*p != SLASH)
		buf[0] = path[0];	/* GetCwd uses and returns drive */
	    else {
		path = p;		/* Don't overwrite the drive name */
		buf[0] = 0;		/* Don't give GetCwd a bogus drive */
	    }
	}
#endif /* !MSDOS */
	if (*p != SLASH) {
	    if (!GetCwd(buf, MAXPATHLEN)) {
		error(SysErrWarning, "getcwd: %s", buf);
		return NULL;
	    } else
		b = buf + strlen(buf);
	    if (b[-1] != SLASH)
		*b++ = SLASH;
	} else {
	    b = buf;
	}
	(void) pathclean(b, p);
	(void) pathcpy(newpath, buf);
#ifndef MSDOS
    } else {
	char *oldwd;
	set_cwd();
	if (!(oldwd = value_of(VarCwd)))
	    return NULL;
	(void) pathcpy(buf, path);
	if (b = last_dsep(buf)) {
	    if (b != buf)
		*b++ = 0;
	    else
		b = NULL;
	}
	p = NULL;
	if (chdir(buf) < 0 || !(p = GetCwd(newpath, MAXPATHLEN)))
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), buf);
	if (chdir(oldwd) < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), oldwd);
	    if (chdir(value_of(VarHome)) < 0)
		error(SysErrFatal, catgets( catalog, CAT_SHELL, 324, "Cannot cd to home" ));
	    set_cwd();
	}
	if (!p)
	    return NULL;
	if (b) {
	    char *np = newpath + strlen(newpath);
	    *np++ = SLASH;
	    (void) pathcpy(np, b);
	}
    }
#endif /* !MSDOS */
    return newpath;
#endif /* MAC_OS */
}

/* Given a file name, return a full path to that filename, eliminating
 * references to "." and ".." if they occur.  Does not check for actual
 * existence of the file!  Copies the full path back to the input path,
 * so path should point to at least MAXPATHLEN space.  If hardpath is
 * nonzero, performs chdir() and GetCwd() to eliminate symbolic links.
 *
 * For MSDOS, also normalizes the path to have only SLASH (no other DSEP).
 */
char *
fullpath(path, hardpath)
char *path;
int hardpath;
{
    return expandpath(path, path, hardpath);
}

/*
 * Given a (possibly NULL or empty) string, return the name of a a valid
 * directory.  The string may contain the usual filename metachars (see
 * above).  Returns the current user's home directory if the input string
 * does not refer to a directory, the ALTERNATE_HOME if the user's home
 * directory cannot be found, or NULL if none of the above are accessible.
 *
 * If the makeit parameter is true, getdir() will attempt to create the
 * directory and will fail if unsuccessful, rather than falling back on
 * ALTERNATE_HOME.
 *
 * NOTE:  Returns the getpath() static buffer, so the same caveats apply.
 */
char *
getdir(path, makeit)
const char *path;
int makeit;
{
    char *p;
    int isdir = 0;

    /* getpath() already handles the NULL and empty cases */
    p = getpath(path, &isdir);	/* This never returns NULL */
    if (isdir != 1) {
	if (makeit == 1) {
	    if (isdir < 0 && errno != ENOENT ||
		    getstat(path, p, 0) < 0 ||
		    zmkdir(p, 0700) < 0)
		p = NULL;
	} else {
	    isdir = 0;
	    p = getpath(ALTERNATE_HOME, &isdir);
	    if (isdir != 1)
		p = NULL;
	}
    }
    return p;
}

/*
 * Given a filename[pointer] (p), a file pointer, and a mode, file_to_fp
 * opens the file with the mode.
 * If the mode is "r" then we read the file into the file pointer at the
 * end (fseek(fp, 0, 2)).  If the file is opened for writing, then read
 * from the beginning of fp and write it into the file.
 * This is usually called to read .signatures into messages (thus,
 * opening .signature with "r" and writing to the end of fp which is probably
 * the sendmail process or the message file pointer) or to write fortunes into
 * the message buffer: reading fp (the popened fortune) and writing into file.
 *
 * If add_newline is nonzero, and the last character processed is not a
 * newline, one is added to the input/output as appropriate.
 *
 * NOTE:  file_to_fp() is restricted to ASCII data!  It cannot handle '\0'
 * bytes and maintains a line count under the assumption that no line is
 * more than BUFSIZ characters long.
 */
int
file_to_fp(p, fp, mode, add_newline)
register char *p;
register FILE *fp;
char *mode;
int add_newline;	/* Force the file to end in a newline */
{
    int 	x = 1, got_newline = 1;
    char 	*file, buf[BUFSIZ];
    FILE 	*tmp_fp;

    if (!p || !*p) {
	print(catgets( catalog, CAT_SHELL, 325, "specify filename" ));
	return -1;
    }
    /* Special case for IS_SENDING && !IS_GETTING should eventually go away */
    if (ison(glob_flags, IS_SENDING) && isoff(glob_flags, IS_GETTING) &&
	    strcmp(p, "-") == 0) {
	file = p;
	if (*mode == 'r')
	    tmp_fp = stdin;
	else
	    tmp_fp = stdout;
    } else {
	file = getpath(p, &x);
	if (x == -1) { /* on error, file contains error message */
	    error(UserErrWarning, file);
	    return -1;
	}
	if (x) {
	    /* if x == 1, then path is a directory */
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), file);
	    return -1;
	} else if (!(tmp_fp = fopen(file, mode))) {
	    error(SysErrWarning, "%s", file);
	    return -1;
	}
    }
    if (*mode != 'r') {
	rewind(fp);
	for(x = 0; fgets(buf, BUFSIZ, fp); x++) {
	    if (fputs(buf, tmp_fp) == EOF) {
		error(SysErrWarning, catgets( catalog, CAT_SHELL, 327, "%s: write failed" ), file);
		return -1;
	    }
	    if (add_newline)
		got_newline = (buf[strlen(buf)-1] == '\n');
	}
	if (add_newline && !got_newline)
	    (void) fputc('\n', tmp_fp);
	if (fflush(tmp_fp) < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 327, "%s: write failed" ), file);
	    return -1;
	}
    } else {
	for(x = 0; fgets(buf, BUFSIZ, tmp_fp); x++) {
	    if (fputs(buf, fp) == EOF) {
		error(SysErrWarning, catgets( catalog, CAT_SHELL, 329, "Write failed" )); /* File name not known */
		return -1;
	    }
	    if (add_newline)
		got_newline = (buf[strlen(buf)-1] == '\n');
	}
	if (add_newline && !got_newline)
	    (void) fputc('\n', fp);
	if (fflush(fp) == EOF) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 329, "Write failed" )); /* File name not known */
	    return -1;
	}
    }
    if (!istool)
	wprint("%s%d line%s\n", (*mode == 'a')? "added ": "",
				  x, (x == 1)? "": "s");
    if (file != p || strcmp(file, "-") != 0)
	if (fclose(tmp_fp) == EOF)
	    return -1;
    return 0;
}

/* Same as fp_to_fp(), below, except we don't futz with file modes and
 * we don't fflush() when we're done, so this can be called many times
 * in succesion on the same pair of file pointers.
 */
long
fp2fp(infp, start, len, outfp)
FILE *infp, *outfp;
long start, len;
{
    long n, i, tot = 0;
    char buf[BUFSIZ];

    if (start >= 0 && fseek(infp, start, 0) < 0)
	return 0;
    if (len < 0)
	while (i = fread(buf, sizeof(char), sizeof buf, infp)) {
	    tot += (n = fwrite(buf, sizeof(char), i, outfp));
	    if (i != n)
		break;
	}
    else
	while (len > 0 &&
		(i = fread(buf, sizeof(char), min(sizeof buf, len), infp))) {
	    tot += (n = fwrite(buf, sizeof(char), i, outfp));
	    if (i != n)
		break;
	    len -= n;
	}

    return tot;
}

/*
 * Copy len bytes from infp to outfp, starting at start.  Return the
 * number of bytes successfully copied, or -1 on error.
 *
 * When start == -1, start at the current seek position.
 * When len == -1, copy through end-of-file.
 *
 * This isn't done with fioxlate() because fp_to_fp() does mode magic
 * for DOS and other systems that differentiate binary/text files.
 */
long
fp_to_fp(infp, start, len, outfp)
FILE *infp, *outfp;
long start, len;
{
    long tot;

#ifdef MSDOS
    int old_inmode, old_outmode;
    
    fflush (infp);
    old_inmode = _setmode (fileno(infp), _O_BINARY);
    fflush (outfp);
    old_outmode = _setmode (fileno(outfp), _O_BINARY);
#endif /* MSDOS */

    if ((tot = fp2fp(infp, start, len, outfp)) > 0)
	if (fflush(outfp) == EOF)
	    tot = -1;

#ifdef MSDOS
    /* flushing these fp's was causing the seek pointer to become
     * screwed up, causing indexed loads to fail.
     */
    if (old_inmode != _O_BINARY) {
	fflush (infp);
	_setmode (fileno(infp), old_inmode);
    }
    if (old_outmode != _O_BINARY) {
	fflush (outfp);
	_setmode (fileno(outfp), old_outmode);
    }
#endif /* MSDOS */

    return tot;
}

/*
 * Translate len bytes of ASCII input from in to out, starting at start.
 * Return the number of bytes read from in and successfully copied out.
 * On complete failure, return -1.
 *
 * The function passed as xlate should take four parameters:
 *	char *input;	The ASCII line to translate
 *	long len;	Length of ASCII input line
 *	char **output;	Pointer to translated output (may be non-ASCII)
 *	char *state;	Pointer to arbitrary state information
 * and should return the length to be written from output, or -1 on error.
 *
 * The output string passed to xlate() is guaranteed to be NULL on the first
 * call and to contain its previous value on each succeeding call.
 */
long
fioxlate(in, start, len, out, xlate, state)
FILE *in;		/* File to read from */
FILE *out;		/* File to write to */
long start;		/* Starting seek pos (-1 == current pos) */
long len;		/* Length to copy (-1 == through EOF) */
long (*xlate)();		/* Translation function */
VPTR state;		/* Other data for xlate() */
{
    char input[BUFSIZ], *output = NULL;
    long tot = 0, count = 0;

    if (in == 0)
	return -1;
    if (start >= 0 && fseek(in, start, 0) < 0)
	return -1;
    if (len == 0)
	return 0;

    while (fgets(input, sizeof input, in)) {
	long copied = tot;

	count = strlen(input);
	tot += count;
	if (len > 0 && tot > len) {
	    count -= (tot - len);
	    input[count] = 0;
	    if (fseek(in, len-tot, 1) == 0)
		tot = len;
	    else
		return -1;
	}
	if (xlate) {
	    if ((count = (*xlate)(input, count, &output, state)) < 0)
		return copied;
	} else
	    output = input;
	if (count && out) {
	    long n;
	    /* Bart: Thu Jan 21 19:24:18 PST 1993
	     * Introduce while() in case fwrite() doesn't write everything
	     * in one shot.  (Is this even possible without error?)
	     */
	    while ((n = fwrite(output, sizeof(char), count, out)) < count) {
		if (n < 0)
		    return copied;
		count -= n;
		copied += n;
	    }
	}

	if (len > 0 && tot >= len)
	    break;
    }

    return tot;
}

/*
 * Count lines in a section of a file using fioxlate()
 */
long
xlcount(input, len, output, state)
char *input;
long len;
char **output;
char *state;	/* Actually (int *) for line count */
{
    int *lines = (int *)state;

    *lines += 1;
    return 0;	/* Suppress output */
}

#ifndef HAVE_FTRUNCATE
# ifdef F_FREESP
/*
 * an implementation of ftruncate for systems that don't have ftruncate or
 * chsize, but do have the F_FREESP fcntl flag -- spencer
 */

int
ftruncate(fd, offset)
  int fd;
  off_t offset;
{
  struct flock fl;

  fl.l_whence = SEEK_SET;
  fl.l_start = offset;
  fl.l_len = 0; /* 0 means "until EOF" */

  return fcntl(fd, F_FREESP, &fl);
}
# endif /* F_FREESP */
#endif /* !HAVE_FTRUNCATE */

/* clear all contents of the file.  Careful that the file is opened for
 * _writing_ --tempfile is opened for reading, so don't try to empty it
 * if you're using ftruncate.   Return -1 on error, 0 on success.
 */
int
emptyfile(fp, fname)
register FILE **fp;
register char *fname;
{
    Debug("Emptying \"%s\"\n", fname);

#if defined(HAVE_FTRUNCATE) || defined(F_FREESP)
    (void) rewind(*fp);
    return ftruncate(fileno(*fp), 0L);
#else /* !(HAVE_FTRUNCATE || F_FREESP) */
#ifdef HAVE_CHSIZE
    (void) rewind(*fp);
    return chsize(fileno(*fp), 0L);
#else
    {
	int omask = umask(077), ret;
	(void) fclose(*fp);
	if (!(*fp = fopen(fname, "w")))
	    ret = -1;
	else
	    ret = 0;
	(void) umask(omask);
	return ret;
    }
#endif /* HAVE_CHSIZE */
#endif /* !(HAVE_FTRUNCATE || F_FREESP) */
}

/*
 * Finds out how many file descriptors are opened.  Useful for making sure
 * no files got opened in subprocedures which were not subsequently closed.
 * If argc is 0, returns the number of available fds.
 */
int
nopenfiles(argc)
    int argc;
{
    register int size = maxfiles();
    register int nfiles = 0, totalfiles = size;

    if (argc > 1)
	return -1;

    if (argc == 1)
	wprint(catgets( catalog, CAT_SHELL, 332, "open file descriptors:" ));
    while (--size >= 0)
	if (fcntl(size, F_GETFL, 0) != -1) {
	    if (argc == 1)
		wprint(" %d", size);
	    ++nfiles;
	}
    if (argc == 1) {
	wprint("\n");
	return 0;
    }
    return totalfiles - nfiles;
}

/*
 * Close all "extraneous" file descriptors; return the number closed
 */
int
closefileds_above(n)
    int n;
{
    register int nfiles = 0;
    register int size = maxfiles();

    while (--size > n)
	if (fcntl(size, F_GETFL, 0) != -1) {
	    (void) close(size);
	    ++nfiles;
	}
    return nfiles;
}

#ifdef UNIX
/*
 * Safer version of closefileds() when file descriptors aren't consecutive.
 */
void
closefileds_except(fd)
int fd;
{
    register int size = maxfiles();

    while (--size >= 0) {
	if (size == fileno(stdin) ||
	    size == fileno(stdout) ||
	    size == fileno(stderr) ||
	    size == fd ||
	    fcntl(size, F_GETFL, 0) != 0)
	    continue;
	(void) close(size);
    }
}
#endif /* UNIX */

/*
 * Open a path for writing or appending -- return a FILE pointer.
 * If program is TRUE, then use popen, not fopen and don't check
 * to see if the file is writable.  If program is FALSE and lockit
 * is TRUE, then lock on open.
 */
FILE *
open_file(p, program, lockit)
register char *p;
int program, lockit;
{
    register FILE *newfile = NULL_FILE;
    register char *tmp;
    int x = 1;

    if (program) {
#ifndef MAC_OS
	tmp = p, x = 0;
#else /* MAC_OS */
		/* 5/15/94 !GF -- change to a debug print statement for FCS */
	error(SysErrWarning, catgets(catalog, CAT_SHELL, 889, "Can't open another program from Mac Z-Mail"));
	return NULL_FILE;
#endif /* !MAC_OS */
    } else
	tmp = getpath(p, &x);
    if (x == 1)
	print(catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), tmp);
    else if (x == -1)
	print("%s: %s\n", p, tmp);
    else {
	register char *mode = NULL;
	/* if it doesn't exist open for "w" */
	if (program || Access(tmp, F_OK))
	    mode = FLDR_WRITE_MODE;
	/* if we can't write to it, forget it */
	else if (Access(tmp, W_OK))
	    error(SysErrWarning, tmp);
	else
	    mode = FLDR_APPEND_MODE;
	if (mode) {
#ifndef MAC_OS
	    if (program) {
		if (!(newfile = popen(tmp, mode)))
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 334, "Cannot execute %s" ), tmp);
	    } else 
#endif /* !MAC_OS */
	    if (lockit) {
		/* Lock on open */
		if (!(newfile = lock_fopen(tmp, mode)) && errno != EAGAIN
#ifdef EWOULDBLOCK
		    && errno != EWOULDBLOCK
#endif
		      )
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 335, "Cannot write to \"%s\"" ), tmp);
#if defined(MAC_OS) && defined(USE_SETVBUF)
		else (void) setvbuf(newfile, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
	    } else {
		/* Ordinary open */
		if (!(newfile = mask_fopen(tmp, mode)))
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 335, "Cannot write to \"%s\"" ), tmp);
	    }
	}
	if (newfile != NULL_FILE) {
	    Debug("Successfully opened %s\n", tmp);
	}
    }
    return newfile;
}

/*
 * Open each file in the vector names[] and place the corresponding
 * file descriptor in files[].  If the file is really a program (pipe),
 * delete the name after opening; otherwise lock the file.
 * Tokens beginning with a /, ~, or + are files; tokens beginning
 * with a | are programs.
 */
int
open_list(names, files, size)
char *names[];
FILE *files[];
int  size;
{
    register int total = 0, prog;
    register char *fpath;

    if (size)
	Debug("opening "),
	    print_argv(names);
    for (total = 0; total < size; total++) {
	fpath = names[total] + (prog = (names[total][0] == '|'));
	/* open_file() locks the file here only if prog is false */
	if ((files[total] = open_file(fpath, prog, TRUE))) {
	    if (prog) {
		xfree(names[total]);
		names[total] = NULL;
	    } else {
		/* Seek to end of file AFTER locking */
		(void) fseek(files[total], 0L, 2);
	    }
	} else {
	    Debug("Failed to open %s\n", names[total]);
	}
    }
    return size;
}

/*
 * find_files gets a set of addresses and an array of
 * char pointers and the maximum size that array can be.
 * The object is to find the files or programs listed in "s".  If the
 * size is 0, then just extract the file names and give error messages
 * for each one since they will not be opened. Return the number of
 * files found and delete all files from the list in "s".
 * The string "s" is modified to be a list of address -- all names AND
 * files are stripped out of the list.
 * The force parameter causes names to be interpreted as files even if
 * they would normally appear to be addresses.
 */
int
find_files(s, names, size, force, recorduser)
register char *s;
char *names[];
int size, force, recorduser;
{
    register int     total = 0;
    char 	     file[MAXPATHLEN], buf[HDRSIZ], *start = s, c;
    register char    *p, *b = buf, *fpath;

    do  {
	int isdir, frc = FALSE;

	if (!(p = get_name_n_addr(s, NULL, file)))
	    break;
	c = *p, *p = 0;
	/* See if it's a file.  This doesn't get written back
	 * onto "buf" since it is supposed to be extracted anyway.
	 * The check for '=' in names beginning with '/' is to
	 * avoid mis-identifying X.400 addresses as file names.
	 *
	 * \052 is for broken compilers that would see a comment.
	 */
	if (force
#ifndef MAC_OS
		|| *file == '+' || *file == '~' && find_dsep(file) ||
		    *file == '|' ||
#ifdef MSDOS /* Bloody DOS backslashes!  '=' is the QNXT! */
		is_fullpath(file) && !zglob(file, "/?*==*?/\052")
#else /* MSDOS */
		is_fullpath(file) && !zglob(file, "/?*=*?/\052")
#endif /* MSDOS */
#endif /* !MAC_OS */
								) {
	    if (*file == '|') {
		isdir = 0;
		fpath = file;
	    } else {
		isdir = 1;
		/* if successful, getpath will reset isdir to 0 */
		fpath = getpath(file, &isdir);
	    }
	    frc = TRUE;
	} else if (recorduser) {
	    char copy[256];

	    bang_form(copy, file);
	    if (fpath = rindex(copy, '!')) {
		*fpath = '+';
		(void) strcpy(file, fpath);
	    } else
		(void) sprintf(file, "+%s", copy);
	    isdir = 0;
	    fpath = getpath(file, &isdir);
	    frc = (!isdir || (isdir != 1 && recorduser == 2));
	    if (frc) {
		b += Strcpy(b, s);
		*b++ = ',', *b++ = ' ';
	    }
	}
	if (frc) {
	    if (!isdir) {
		if (size && total < size)
		    names[total++] = savestr(fpath);
		else
		  print(catgets(catalog, CAT_SHELL, 340, "No open space for \"%s\"\n"),
			file);
	    } else if (isdir == 1)
		print(catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), file);
	    else
		print("%s: %s\n", file, fpath);
	} else {
	    b += Strcpy(b, s);
	    *b++ = ',', *b++ = ' ';
	}
	for (*p = c, s = p; *s == ',' || isspace(*s); s++)
	    ;
    } while (*s);
    for (*b-- = 0; b > buf && (*b == ',' || isspace(*b)); b--)
	*b = 0;
    (void) strcpy(start, buf);
    names[total] = NULL; /* for free_vec() */

    return total;
}

/*
 * access(2) goes by your real uid rather than your effective uid. If you are
 * su'ed and try to read your mail, you will be unable to because access()
 * will give the illusion that you cannot read/write to your mbox.  Solve
 * the problem by using stat() instead.  This allows us to determine the
 * "effective access".
 * Access(filename, mode) tells whether the effective user (i.e. this
 * program running right now) can access the given file in the given way.
 * StatAccess(statbuf, mode) does the same thing with a stat buffer (the result
 * of a call to stat()).
 */

#ifndef NGROUPS_MAX
# ifdef NGROUPS
#  define NGROUPS_MAX NGROUPS
# else /* NGROUPS */
#  define NGROUPS_MAX 100
# endif /* NGROUPS */
#endif /* !NGROUPS_MAX */

/* RJL ** 6.15.93     - no groups of any kind in MSDOS */
/* GF 6/20/93		- or on Mac */
#if !defined(MSDOS) && !defined(MAC_OS)

static int
is_suppl_group(gid)
GETGROUPS_T gid;
{
    int i, n;
    GETGROUPS_T gidset[NGROUPS_MAX];

    if ((n = getgroups(NGROUPS_MAX, gidset)) < 0)
	return 0;
    for (i = 0; i < n; ++i)
	if (gid == gidset[i])
	    return 1;
    return 0;
}

#else  /* MSDOS || MAC_OS */
#define is_suppl_group(gid) 0
#endif /* MSDOS || MAC_OS */

int
StatAccess(buf, mode)
struct stat *buf;
int mode;
{
    int shift, euid;

    if (!buf) {
	errno = EINVAL;
	return -1;
    }
    /*
     * euid=root is a special case; the only way we get EACCESS
     * is if we want to execute and *nobody* has execute permission.
     */
    if ((euid = geteuid()) == 0) {
	if ((mode & 1) && !(buf->st_mode & 0111)) {
	    errno = EACCES;
	    return -1;
	} else
	    return 0;
    }

    if (buf->st_uid == euid)
	shift = 6;
    else if (buf->st_gid == getegid() || is_suppl_group(buf->st_gid))
	shift = 3;
    else
	shift = 0;

    /* failure means some bit that's on in mode is off in shifted buf->st_mode*/
    if (mode & ~(buf->st_mode >> shift)) {
	errno = EACCES;
	return -1;
    } else
	return 0;
}

#ifndef WIN16
int
Access(file, mode)
const char *file;
int mode;
{
#if defined(MSDOS) || defined(MAC_OS)
    return (access (file, mode));
#else /* !(MSDOS || MAC_OS) */
#ifdef HAVE_SETREGID
    /*
     * If not BSD, setreuid and setregid probably don't exist,
     * so unless the real or effective id is 0, there is no
     * way to swap real and effective uids, so the following trick won't work.
     */
     int r;
     int uid = getuid();
     int gid = getgid();
     int euid = geteuid();
     int egid = getegid();

     setreuid(euid, uid);
     setregid(egid, gid);
     r = access(file, mode);
     setreuid(uid, euid);
     setregid(gid, egid);
     return r;
#else /* !HAVE_SETREGID */
    /*
     * Because a process can have supplementary groupids under BSD,
     * the following method would give an incorrect answer under
     * BSD if the file is group-only accessible with group foo,
     * and foo is one of the supplementary groupids of the process,
     * but not the "primary" one. We could solve this
     * by stepping through the list returned by getgroups(),
     * but since we've gotta use #ifdef BSD anyway, we might as well
     * use the above method.
     */
    struct stat buf;

    if (stat(file, &buf) == -1)
	return -1;

    return StatAccess(&buf, mode);
#endif /* HAVE_SETREGID */
#endif /* MSDOS || MAC_OS */
}
#endif /* !WIN16 */

/*
 * Open a file for read/write/whatever but make sure umask is rw by user only.
 */
FILE *
mask_fopen(file, mode)
const char *file, *mode;
{
#ifdef apollo
    int needs_chmod = (Access(file, F_OK) != 0);
#else
    int omask = umask(077);
#endif
    FILE *fp;
#ifdef MSDOS
    char fbuf[MAXPATHLEN];
    
    dos_copy_path (fbuf, file);	/* remove extra path seperators */
    file = fbuf;
#endif /* MSDOS */

#if defined(NO_FOPEN_A_PLUS) || defined(MAC_OS) || defined(MSDOS)
    /* XENIX and other older sytems can't handle "a+".  Even newer
     * SysV systems define a+ such that all writes go at end-of-file,
     * not at whatever the current seek position is.  Good grief.
     */
	 /* RJL ** ANSI/ISO C specifies this fopen behaviour */
    if (strcmp(mode, FLDR_APLUS_MODE) == 0) {
	if (Access(file, F_OK) == 0)
	    mode = FLDR_RPLUS_MODE;
	else
	    mode = FLDR_WPLUS_MODE;
	if (fp = fopen(file, mode))
	    (void) fseek(fp, 0L, 2); /* assure we're at the end of the file */
    } else
#endif /* NO_FOPEN_A_PLUS || AIX || MAC_OS */
    fp = fopen(file, mode);
#if defined(MAC_OS) && defined(USE_SETVBUF)
    if (fp)
	(void) setvbuf(fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
#ifdef apollo
    /* Initial file ACL's override umask so make sure mode is right */
    if (fp && needs_chmod)
	(void) chmod(file, 0600);
#else
    (void) umask(omask);
#endif
    return fp;
}

/*
 * Shorten a file name, replacing its full path name with one using an
 *  accepted zmail abbreviation:
 *	~	home directory
 *	+	folder directory
 *  For files in the current directory, the path is simply skipped.
 * Returns a pointer into a static buffer holding the trimmed path.
 *
 * if "name" is NULL, return previous value.
 */
char *
trim_filename(name)
const char *name;
{
    static char buf[MAXPATHLEN];
    char *fldr = value_of(VarFolder),
	 *home = value_of(VarHome);
    int len;

    if (!name)
	return buf;

    /* Handling $folder is tough, because if it is not set then we should
     * trim DEF_FOLDER; but DEF_FOLDER may not be a full path, and we can't
     * call getpath() because the "name" parameter may point to gepath()'s
     * static buffer.  So we handle the special case of DEF_FOLDER starting
     * with a tilde-slash ($home), and forget about it otherwise.  Yuck.
     */
    if ((!fldr || !*fldr) && (fldr = DEF_FOLDER) &&
	    fldr[0] == '~' && is_dsep(fldr[1]) && home) {
	(void) sprintf(buf, "%s%s", home, fldr + 1);
	fldr = buf;  /* buf will get overwritten again below */
    }
    /* One more special case: if $folder and $home are the same, then we
     * trim as $home, otherwise we trim as $folder.  This prevents strange
     * contractions like "+.cshrc" for "~/.cshrc".
     */
    if ((!home || pathcmp(home, fldr)) && (len = strlen(fldr)) &&
	    !pathncmp(fldr, name, len) && (is_dsep(name[len]) || !name[len])) {
	buf[0] = '+';
	if (name[len] && name[len + 1])
	    (void) pathcpy(buf + 1, name + len + 1);
	else
	    buf[1] = 0;
	return buf;
    }
#ifndef MSDOS
    /* don't say "~/foo" on MS-DOS.  I'm not 100% sure we should say
     * "+foo", either.  This probably applies to Mac as well.
     */
    else if (home && (len = strlen(home)) && !pathncmp(home, name, len) &&
	    (is_dsep(name[len]) || !name[len])) {
	buf[0] = '~';
	(void) pathcpy(buf + 1, name + len);
	return buf;
    }
#endif /* !MSDOS */
    else if ((fldr = value_of(VarCwd)) &&
	    (len = strlen(fldr)) && !pathncmp(fldr, name, len) &&
	    is_dsep(name[len]) && name[len+1])
	return pathcpy(buf, name + len + 1);
    return pathcpy(buf, name);
}

extern char *alloc_tempfname_dir P((const char *, const char *));

/*
 * find a free tmpfile using seedname, and return a dynamically
 * allocated copy of the file's name.  does not create the file.
 */
char *
alloc_tempfname(seedname)
const char *seedname;
{
    const char *dir_prefix;
    
    if (!(dir_prefix = getdir(value_of(VarTmpdir), FALSE)))
	dir_prefix = ALTERNATE_HOME;
    return alloc_tempfname_dir(seedname, dir_prefix);
}

#ifdef UNIX
const char at_valid_letters[] = "bcdfghjklmnpqrstvxwxzBCDFGHJKLMNPQRSTVXWXZ-:@.,_";
#else /* !UNIX */
const char at_valid_letters[] = "bcdfghjklmnpqrstvxwxz0123456789-_@";
#endif /* !UNIX */

/*
 * find a free tmpfile using seedname, in the specified directory, and
 * return a dynamically allocated copy of the file's name.  does not
 * create the file.
 *
 * filename format is ".SSSNNNNNNNYYY" on UNIX, "XYYYYYYY.SSS" on
 * non-UNIX.  SSS is the seedname, NNNNNNN is the PID, and YY is a
 * seed number represented in base AT_VALID_COUNT.  On Unix, the pid
 * is truncated to 8 digits, and the seed number takes up the remainder
 * of the filename, for a total of 14 characters.
 */
char *
alloc_tempfname_dir(seedname, dir_prefix)
const char *seedname, *dir_prefix;
{
#define AT_SEEDLEN 7
#define AT_PID_LEN 8
#define AT_TOTAL_LEN 14
    char *p, newname[MAXPATHLEN], seedstr[AT_SEEDLEN+1];
    unsigned int pid = getpid();
    static unsigned int intseed = 0;
    unsigned int ct, num;
#define AT_VALID_COUNT (ArraySize(at_valid_letters)-1)

    do {
	intseed++;
	p = seedstr;
#ifdef UNIX
	num = intseed;
#else /* !UNIX */
	num = pid+(intseed << 15);
#endif /* !UNIX */
	for (ct = AT_SEEDLEN; ct; ct--) {
	    *p++ = at_valid_letters[num % AT_VALID_COUNT];
	    num /= AT_VALID_COUNT;
	}
	*p = '\0';
#ifdef UNIX
	sprintf(newname, "%s%c.%.3s", dir_prefix, SLASH, seedname);
	p = newname + strlen(newname);
	sprintf(p, "%d", pid);
	p[AT_PID_LEN] = '\0';
	strcpy(p+strlen(p), seedstr);
	newname[strlen(dir_prefix)+1+AT_TOTAL_LEN] = '\0';
#else /* !UNIX */
	sprintf(newname, "%s%cx%s.%.3s", dir_prefix, SLASH, seedstr, seedname);
#endif /* !UNIX */
    } while (!Access(newname, F_OK));
    return(savestr(newname));
}

/*
 * find a free tmpfile using seedname, open it, return a file pointer, and
 * return the file's name through tmpname
 */
FILE *
open_tempfile(seedname, tmpname)
const char *seedname;
char **tmpname;
{
    int alted = False;
    char *newname, *p;
    FILE *ftmp;

    if (!(p = getdir(value_of(VarTmpdir), FALSE))) {
	alted = True;
	p = ALTERNATE_HOME;
    }
    for (;;) {
	newname = alloc_tempfname_dir(seedname, p);
	/* create the file, make sure it's empty */
	if (ftmp = mask_fopen(newname, "w+"))
	    break;
	if (alted) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 342, "Cannot create tempfile %s" ), newname);
	    xfree(newname);
	    return NULL_FILE;
	}
	alted = True;
	p = ALTERNATE_HOME;
	xfree(newname);
    }
    xfree(*tmpname);
    *tmpname = newname;
#if defined(MAC_OS) && defined(USE_SETVBUF)
    if (ftmp)
    	(void) setvbuf(ftmp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
    return ftmp;
}

void
touch(path)
const char *path;
{
    FILE *fp;

    if (fp = fopen(path, "a"))
	fclose(fp);
}

/*
 * "Wipe" a file by overwriting it with garbage.
 */
int
zwipe(path)
const char *path;
{
    FILE *fp = fopen(path, "r+");
    unsigned char garbage[BUFSIZ], *g = garbage;
    static Int32 trash = 0x5abacada;

    if (fp) {
	long siz;

	(void) fseek(fp, 0L, 2);
	siz = (long)ftell(fp);
	(void) fseek(fp, 0L, 0);
	while ((long)ftell(fp) < siz) {
	    while (g < garbage + BUFSIZ) {
		bcopy((void *)&trash, (void *)g, (int)sizeof(trash));
		trash = (trash << 1) | ((trash >> 31) & 0x1);
		g += sizeof(trash);
	    }
	    if (fwrite((char *)garbage, sizeof(unsigned char), BUFSIZ, fp) < 0)
		break;
	}
	(void) fclose(fp);
    }
    return unlink(path);
}

#if !defined(HAVE_RENAME)

rename(from, to)
const char *from, *to;
{
    int realerrno = errno = 0;

    if (link(from, to) < 0) {
#ifdef CLOBBER_ON_RENAME
	/* DANGER!  We cannot provide all the safeguards the OS might.
	 * This code most closely duplicates the real rename(2), but
	 * we can't guarantee that the link will succeed just because
	 * the unlink succeeded, and we can't link before unlinking!
	 */
	if (errno != EEXIST || unlink(to) < 0 || link(from, to) < 0)
#endif /* CLOBBER_ON_RENAME */
	    return -1;
    }
    if (unlink(from) < 0) {	/* This is nasty when CLOBBER_ON_RENAME */
	realerrno = errno;
	(void) unlink(to);	/* Because we've lost the original "to" */
	errno = realerrno;
	return -1;
    }
    return 0;
}

#endif /* HAVE_RENAME */

#if !defined(HAVE_MKDIR) || defined(MKDIR_PROG)

#ifndef MKDIR_PROG
#define MKDIR_PROG "/bin/mkdir"
#endif /* MKDIR_PROG */

mkdir(path, mode)
const char *path;
int mode;
{
    char buf[MAXPATHLEN*2];
    int ok = -1;

    if (Access(path, F_OK) != 0) {
	(void) sprintf(buf, "exec %s %s", MKDIR_PROG, path);
	(void) system(buf);
	ok = Access(path, F_OK);
    } else
	errno = EEXIST;
    if (ok == 0 && chmod(path, mode) < 0)
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 343, "mkdir: cannot set mode of %s" ), path);
    return ok;
}

#endif /* !HAVE_MKDIR || MKDIR_PROG */

int
exists_dir(path, chklast)
const char *path;
int chklast;
{
    char *p, buf[MAXPATHLEN];
    struct stat sbuf;

    if (!fullpath(pathcpy(buf, path), FALSE))
	return 0;
    if (!chklast && (p = last_dsep(buf)))
	*p = 0;
    if (stat(buf, &sbuf) || (sbuf.st_mode & S_IFMT) != S_IFDIR)
	return 0;
    return 1;
}

int
zmkdir(path, mode)
const char *path;
int mode;
{
    AskAnswer answer = AskYes;
    int save_errno;
    struct stat sbuf;

    if (stat(path, &sbuf) == 0) {
	if ((sbuf.st_mode & S_IFMT) == S_IFDIR)
	    return 0;
	errno = EEXIST;
	return -1;
    }
    save_errno = errno;
    if (none_p(glob_flags, REDIRECT|NO_INTERACT) &&
	    chk_option(VarVerify, "mkdir"))
	answer = ask(AskOk,
	    catgets(catalog, CAT_SHELL, 900, "Create new directory \"%s\"?"),
		    trim_filename(path));
    if (answer != AskYes) {
	errno = save_errno;
	return -1;
    }
/* RJL** 12.15.92 mkdir under MS C only takes a single parameter */
#if defined(_WINDOWS) || defined(MSC) || defined(_INTELC32_) || defined(MAC_OS)
    return mkdir(path);
#else   /* !(_WINDOWS || MSC || _INTELC32_ || MAC_OS) */
    return mkdir(path, mode);
#endif /* !(_WINDOWS || MSC || _INTELC32_ || MAC_OS) */
}

int
zmkpath(path, mode, mklast)
const char *path;
int mode, mklast;
{
    char buf[MAXPATHLEN], **elems, *p = fullpath(pathcpy(buf, path), FALSE);
    struct stat sbuf;
    int n;

    if (!p)
	return -1;
#ifdef MSDOS
    if (isalpha(*p) && p[1] == ':')
	p += 2;
#endif /* MSDOS */
    while (is_dsep(*p)) p++;	/* Skip root directory */
    n = vlen(elems = strvec(p, DSEP, TRUE));
    if (n < 1)
	return -1;
    if (mklast == 0) {
	xfree(elems[--n]);
	elems[n] = 0;
    }
	
    *p = 0;	/* Now buf contains only the root directory */
    for (n = 0; elems[n]; n++) {
	if (n > 0)
	    *p++ = SLASH;
	p += Strcpy(p, elems[n]);	/* Append next elem to buf */
	if (stat(buf, &sbuf) < 0) {
#if defined(MSDOS) || defined (MAC_OS)
	    if (errno == ENOENT && mkdir(buf) == 0)
#else  /* !MSDOS && !MAC_OS */
	    if (errno == ENOENT && mkdir(buf, mode) == 0)
#endif /* MSDOS || MAC_OS */
		continue;
	    n = -1;
	    break;
	} else if ((sbuf.st_mode & S_IFMT) != S_IFDIR) {
	    errno = ENOTDIR;
	    n = -1;
	    break;
	}
    }
    free_vec(elems);

    return n < 0 ? -1 : 0;
}

long
file_size(fp)
FILE *fp;
{
    struct stat statbuf;

    if (fstat(fileno(fp), &statbuf) < 0) {
	return 0;
    }
    return statbuf.st_size;
}

#if defined (MSDOS)
/*
This function takes a long pointer to a constant 'C' string, and determines
if the string is a valid 8.3 file name for DOS. Criteria are:

    1. Total length <= 12 (includes the '.') AND
    2. if '.': (length before '.' <= 8 AND length after '.' <= 3)
    3. else (total length <= 8)

to evaluate to TRUE (is a valid file name). Otherwise, FALSE.
*/
    
int is_valid_dos_fname (char *pName)
{
char    *ptr = NULL;    
int     i, ret = 0, len = 0, len2 = 0;
char    valid_ascii[] = "abcdefghijklmnopqrstuvwxyz\
ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789$#&@!%()-{}`_'^~.";

    /* make sure we didn't get passed a NULL */
    if (pName)
    {
        len = strlen (pName);

		/* valid ascii check */
		for ( i=0, ptr=pName; i<len; i++,ptr++ )
			if ( !strchr( valid_ascii, *ptr ) ) return 0;

        /* boundary check */
        if ((len) && (len < 13))
        {
            /* '.' check */
            ptr = strchr (pName, '.');
            if (ptr)
            {
                /* a . is not allowed as the first character in the name */
                if (ptr == pName)
                {
                    ret = 0;
                }
                else
                {
                    /* len2 includes the '.', thus the check for < 5, and not < 4 */
                    len2 = strlen (ptr);
                    if ((len2 < 5) && ((len - len2) < 9))
                        ret = 1;
                }
            }
            else
            {
                if (len <= 8)
                    ret = 1;
            }
            /* finally, look for embedded spaces; they are not valid! */
            ptr = strchr (pName, ' ');
            if (ptr)
            	ret = 0;
        }
    }
    
    return (ret);

}

LPCSTR SubjectToFileName(LPCSTR szSubject)
{
}

#endif /* MSDOS */
