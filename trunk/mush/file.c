/* file.c -- Copyright (1988) Dan Heller */

#include "mush.h"
#include <pwd.h>

#ifdef SYSV
#ifdef EWOULDBLOCK
#undef EWOULDBLOCK
#endif /* EWOULDBLOCK */
#define EWOULDBLOCK     EAGAIN
#endif /* SYSV */

/* takes string 'p' and address of int (isdir).  If p uses the ~ to reference
 * a home directory of some sort, then expand it.  find out what sort of
 * file final path is. set isdir to 1 if a directory, 0 if not, -1 on error
 * return final path. If an error occurs, return string indicating error.
 * if isdir has a value of 1 when passed, it ignores "No such file or directory"
 */
char *
getpath(p, isdir)
register char *p;
int *isdir;
{
    static char buf[MAXPATHLEN];
    struct stat stat_buf;

    if (p != buf) { /* Just in case */
	if (!p || !*p || !strcmp(p, "~")) {
	    char *home = do_set(set_options, "home");
	    if (!home || !*home)
		home = ALTERNATE_HOME;
	    (void) strcpy(buf, home);  /* no arg means home */
	} else if (*p == '~') {
	    if (p[1] != '/') {
		/* not our home, but someone else's
		 * look for ~user or ~user/subpath
		 * if '/' exists, separate into tmp="user" p="subpath"
		 */
		struct passwd *ent, *getpwnam();
		char *p2 = p+1;
		if (p = index(p2, '/'))
		    *p++ = 0;
		if (!(ent = getpwnam(p2))) {
		    *isdir = -1;
		    return sprintf(buf, "no such user: %s", p2);
		}
		/* append subpath to pathname */
		if (p && *p)
		    (void) sprintf(buf, "%s/%s", ent->pw_dir, p);
		/* if *p == NULL, pathname is done (buf), set isdir = 1 */
		else {
		    *isdir = 1;
		    return strcpy(buf, ent->pw_dir);
		}
	    } else {
		char *home = do_set(set_options, "home");
		if (!home || !*home)
		    home = ALTERNATE_HOME;
		(void) sprintf(buf, "%s/%s", home, p+2);
	    }
	} else if (*p == '%') {
	    /* if %user, append user name... else, it's just us */
	    if (!*++p || *p == ' ' || *p == '\t')
		(void) strcpy(buf, spoolfile);
	    else
#ifndef HOMEMAIL
		(void) sprintf(buf, "%s/%s", MAILDIR, p);
#else /* HOMEMAIL */
	    {
		/* If it's NOT us, recur to get the path for ~user/MAILFILE */
		int t_isdir = *isdir;
		char *t, tmp[MAXPATHLEN];
		(void) sprintf(tmp, "~%s/%s", p, MAILFILE);
		t = getpath(tmp, &t_isdir);
		if (t_isdir == -1) {
		    *isdir = -1;
		    return t;
		}
		/* strcpy(buf, t); --buf already has info because it's static */
	    }
#endif /* HOMEMAIL */
	} else if (*p == '+') {
	    register char *p2 = do_set(set_options, "folder");
	    if (!p2 || !*p2)
		p2 = DEF_FOLDER;
	    if (*++p)
		(void) sprintf(buf, "%s/%s", p2, p);
	    else
		(void) strcpy(buf, p2);
	    if (*buf != '/') {
		int t_isdir = *isdir;
		char *t, tmp[MAXPATHLEN];
		if (*buf != '~')
		    (void) sprintf(tmp, "~/%s", buf);
		else
		    (void) strcpy(tmp, buf);
		t = getpath(tmp, &t_isdir);
		if (t_isdir == -1) {
		    *isdir = -1;
		    return t;
		}
		/* strcpy(buf, t); --buf already has info because it's static */
	    }
	} else {  /* allow \ to escape the special chars, +, %, ~ */
	    if (*p == '\\')
		p++;
	    (void) strcpy(buf, p);
	}
    }
    if (stat(buf, &stat_buf)) {
	(void) access(buf, F_OK); /* set errno to the "real" reason */
	if (errno == ENOENT && *isdir == 1) {
	    *isdir = 0; /* say it's a regular file even tho it doesn't exist */
	    return buf; /* it may be wanted for creating */
	}
	*isdir = -1;
#ifdef LINUX
        return strerror(errno);
#else
	return sys_errlist[errno];
#endif
    }
    *isdir = ((stat_buf.st_mode & S_IFMT) == S_IFDIR);
    return buf;
}

/*
 * Given a (possibly NULL or empty) string, return the name of a a valid
 * directory.  The string may contain the usual filename metachars (see
 * above).  Returns the current user's home directory if the input string
 * does not refer to a directory, the ALTERNATE_HOME if the user's home
 * directory cannot be found, or NULL if none of the above are accessible.
 *
 * NOTE:  Returns the getpath() static buffer, so the same caveats apply.
 */
char *
getdir(path)
char *path;
{
    int isdir = 0;

    /* getpath() already handles the NULL and empty cases */
    if (!(path = getpath(path, &isdir)) || isdir != 1) {
	isdir = 0;
	path = getpath(ALTERNATE_HOME, &isdir);
	if (isdir != 1)
	    path = NULL;
    }
    return path;
}

/*
 * Given a filename[pointer] (p), a file pointer, and a mode, file_to_fp
 * opens the file with the mode.
 * If the mode is "r" then we read the file into the file pointer at the
 * end (fseek(fp, 2, 0)).  If the file is opened for writing, then read
 * from the beginning of fp and write it into the file.
 * This is usually called to read .signatures into messages (thus,
 * opening .signature with "r" and writing to the end of fp which is probably
 * the sendmail process or the message file pointer) or to write fortunes into
 * the message buffer: reading fp (the popened fortune) and writing into file.
 */
file_to_fp(p, fp, mode)
register char *p;
register FILE *fp;
char *mode;
{
    int 	x = 1;
    char 	*file, buf[BUFSIZ];
    FILE 	*tmp_fp;

    if (!p || !*p) {
	print("specify filename");
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
	    wprint(file);
	    return -1;
	}
	wprint("%s: ", file);
	if (x) {
	    /* if x == 1, then path is a directory */
	    wprint("is a directory.\n");
	    return -1;
	} else if (!(tmp_fp = fopen(file, mode))) {
#ifdef LINUX
            wprint("%s\n", strerror(errno));
#else
	    wprint("%s\n", sys_errlist[errno]);
#endif
	    return -1;
	}
    }
    if (*mode != 'r') {
	rewind(fp);
	for(x = 0; fgets(buf, BUFSIZ, fp); x++)
	    (void) fputs(buf, tmp_fp);
    } else {
	for(x = 0; fgets(buf, BUFSIZ, tmp_fp); x++)
	    (void) fputs(buf, fp);
	(void) fflush(fp);
    }
    wprint("%s%d line%s\n", (*mode == 'a')? "added ": "",
				  x, (x == 1)? "": "s");
    if (file != p || strcmp(file, "-") != 0)
	(void) fclose(tmp_fp);
    return 0;
}

/* clear all contents of the file.  Careful that the file is opened for
 * _writing_ --tempfile is opened for reading, so don't try to empty it
 * if you're using ftruncate.   Return -1 on error, 0 on success.
 */
emptyfile(fp, fname)
register FILE **fp;
register char *fname;
{
    Debug("Emptying \"%s\"\n", fname);
#ifndef SYSV
    return ftruncate(fileno(*fp), 0L);
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
#endif /* SYSV */
}

/*
 * Finds out how many file descriptors are opened.  Useful for making sure
 * no files got opened in subprocedures which were not subsequently closed.
 * If argc is 0, returns the number of available fds.
 */
nopenfiles(argc)
{
#ifdef MAXFILES
    register int size = MAXFILES;
#else
    register int size = getdtablesize();
#endif /* MAXFILES */
    register int nfiles = 0, totalfiles = size;

    if (argc > 1)
	return -1;

    if (argc == 1)
	wprint("open file descriptors:");
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
closefileds (n)
{
    register int nfiles = 0;
#ifdef MAXFILES
    register int size = MAXFILES;
#else
    register int size = getdtablesize();
#endif /* MAXFILES */

    while (--size >= n)
	if (fcntl(size, F_GETFL, 0) != -1) {
	    (void) close(size);
	    ++nfiles;
	}
    return nfiles;
}

/*
 * Open a path for writing or appending -- return a FILE pointer.
 * If program is TRUE, then use popen, not fopen and don't check 
 * to see if the file is writable.  If program is FALSE and lockit
 * is TRUE, then lock on open.
 */
FILE *
open_file(p, program, lockit)
register char *p;
{
    register FILE *newfile = NULL_FILE;
    register char *tmp;
    int x = 1;

    if (program)
	tmp = p, x = 0;
    else
	tmp = getpath(p, &x);
    if (x == 1)
	print("%s is a directory.\n", tmp);
    else if (x == -1)
	print("%s: %s\n", p, tmp);
    else {
	register char *mode = NULL;
	/* if it doesn't exist open for "w" */
	if (program || Access(tmp, F_OK))
	    mode = "w";
	/* if we can't write to it, forget it */
	else if (Access(tmp, W_OK))
	    error(tmp);
	else
	    mode = "a";
	if (mode)
	    if (program) {
		if (!(newfile = popen(tmp, mode)))
		    error("Can't execute %s\n", tmp);
	    } else if (lockit) {
		/* Lock on open */
		if (!(newfile = lock_fopen(tmp, mode)) && errno != EWOULDBLOCK)
		    error("Can't write to %s", tmp);
	    } else {
		/* Ordinary open */
		if (!(newfile = mask_fopen(tmp, mode)))
		    error("Can't write to %s", tmp);
	    }
	    if (newfile != NULL_FILE)
		Debug("Successfully opened %s\n", tmp);
    }
    return newfile;
}

/*
 * Open each file in the vector names[] and place the corresponding
 * file descriptor in files[].  If the file is really a program (pipe),
 * delete the name after opening; otherwise lock the file.
 * Tokens beginning with a "/, ~, or + are files; tokens beginning
 * with a | are programs.
 */
open_list(names, files, size)
char *names[];
FILE *files[];
{
    register int total = 0, prog;
    register char *fpath;

    Debug("opening "), print_argv(names);
    for (total = 0; size && total < size; ) {
	fpath = names[total] + (prog = (names[total][0] == '|'));
	/* open_file() locks the file here only if prog is false */
	if ((files[total] = open_file(fpath, prog, TRUE))) {
	    if (prog) {
		xfree(names[total]);
		names[total++] = NULL;
	    } else {
		/* Seek to end of file AFTER locking */
		(void) fseek(files[total++], 0L, 2);
	    }
	} else {
	    Debug("Failed to open %s\n", names[total]);
	    /* Swap the failed file with the last in the list */
	    if (size--) {
		xfree(names[total]);
		names[total] = names[size];
		names[size] = NULL;
	    }
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
 * files found and delete all files from the list in * "s".
 * The string "s" is modified to be a list of address -- all names AND
 * files are stripped out of the list.
 * The force parameter causes names to be interpreted as files even if
 * they would normally appear to be addresses.
 */
find_files(s, names, size, force)
register char *s;
char *names[];
{
    register int     total = 0;
    char 	     file[MAXPATHLEN], buf[HDRSIZ], *start = s, c;
    register char    *p, *b = buf, *fpath;

    do  {
	if (!(p = get_name_n_addr(s, NULL, file)))
	    break;
	c = *p, *p = 0;
	/* See if it's a file.  This doesn't get written back
	 * onto "buf" since it is supposed to be extracted anyway.
	 * The check for '=' in names beginning with '/' is to
	 * avoid mis-identifying X.400 addresses as file names.
	 *
	 * \052 is a * for broken compilers that would do a comment.
	 */
	if (force || *file == '+' || *file == '~' ||
		*file == '|' || *file == '/' && !glob(file, "/?*=*?/\052")) {
	    int isdir;
	    /* open either "file" or &file[1] */
	    if (*file == '|') {
		isdir = 0;
		fpath = file;
	    } else {
		isdir = 1;
		/* if successful, getpath will reset isdir to 0 */
		fpath = getpath(file, &isdir);
	    }
	    if (!isdir) {
		if (size && total < size)
		    names[total++] = savestr(fpath);
		else
		    print("No open space for %s\n", file);
	    } else if (isdir == 1)
		print("%s: is a directory\n", file);
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
 * access(2) has an undocumented feature which ignores suid.  If you are
 * su'ed and try to read your mail, you will be unable to because access()
 * will give the illusion that you cannot read/write to your mbox.  Solve
 * the problem by using stat() instead.
 */

#ifndef Access

Access(file, mode)
register char *file;
{
    struct stat buf;

    if (stat(file, &buf) == -1)
	return -1;
    if (mode == R_OK)
	return (buf.st_mode & 0400)? 0 : -1;
    if (mode == W_OK)
	return (buf.st_mode & 0200)? 0 : -1;
    return 0;
}

#endif

/*
 * Open a file for read/write/whatever but make sure umask is rw by user only.
 */
FILE *
mask_fopen(file, mode)
char *file, *mode;
{
    int omask = umask(077);
    FILE *fp;
#ifdef SYSV
    /* XENIX and other older sytems can't handle "a+".	Even newer
     * SysV systems define a+ such that all writes go at end-of-file,
     * not at whatever the current seek position is.  Good grief.
     */
    if (strcmp(mode, "a+") == 0) {
	if (Access(file, F_OK) == 0)
	    mode = "r+";
	else
	    mode = "w+";
	if (fp = fopen(file, mode))
	    (void) fseek(fp, 0L, 2); /* assure we're at the end of the file */
    } else
#endif /* SYSV */
    fp = fopen(file, mode);
    (void) umask(omask);
    return fp;
}

/*
 * Shorten a file name, replacing its full path name with one using an
 *  accepted mush abbreviation:
 *	~	home directory
 *	+	folder directory
 *  For files in the current directory, the path is simply skipped.
 * Returns a pointer into a static buffer holding the trimmed path.
 */
char *
trim_filename(name)
char *name;
{
    static char buf[MAXPATHLEN];
    char *fldr = do_set(set_options, "folder"),
	 *home = do_set(set_options, "home");
    int len;

    /* Handling $folder is tough, because if it is not set then we should
     * trim DEF_FOLDER; but DEF_FOLDER may not be a full path, and we can't
     * call getpath() because the "name" parameter may point to gepath()'s
     * static buffer.  So we handle the special case of DEF_FOLDER starting
     * with a tilde ($home), and forget about it otherwise.  Yuck.
     */
    if ((!fldr || !*fldr) && (fldr = DEF_FOLDER) && *fldr == '~' && home) {
	(void) sprintf(buf, "%s%s", home, fldr + 1);
	fldr = buf;  /* buf will get overwritten again below */
    }
    /* One more special case: if $folder and $home are the same, then we
     * trim as $home, otherwise we trim as $folder.  This prevents strange
     * contractions like "+.cshrc" for "~/.cshrc".
     */
    if ((!home || strcmp(home, fldr)) && (len = strlen(fldr)) &&
	    !strncmp(fldr, name, len) && (name[len] == '/' || !name[len])) {
	buf[0] = '+';
	if (name[len] && name[len + 1])
	    (void) strcpy(buf + 1, name + len + 1);
	else
	    buf[1] = 0;
	return buf;
    } else if (home && (len = strlen(home)) && !strncmp(home, name, len) &&
	    (name[len] == '/' || !name[len])) {
	buf[0] = '~';
	(void) strcpy(buf + 1, name + len);
	return buf;
    } else if ((fldr = do_set(set_options, "cwd")) &&
	    (len = strlen(fldr)) && !strncmp(fldr, name, len) &&
	    name[len] == '/')
	return strcpy(buf, name + len + 1);
    return strcpy(buf, name);
}
