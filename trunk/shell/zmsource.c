/* zmSource.c	Copyright 1990, 1991 Z-Code Software Corp. */

/* This file defines the complete Source library for Z-Mail.
 * It should be broken into individual files as indicated.
 * zmsource.h is self-protecting.
 */

/* Sinit.c	Copyright 1990, 1991 Z-Code Software Corp. */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "zccmac.h"
#include "zcunix.h"
#include "zmail.h"
#include "zmflag.h"
#include "zmsource.h"
#include "zmstring.h"
#ifdef HAVE_MMAP
#include "fmap.h"
#else
#define ss_fp ss.fp   /* Private knowledge about zmsource.h */
#endif /* HAVE_MMAP */

typedef enum {
    SourceFirst,
    SourceLast
} SourceOrder;

/*
 * Create and initialize a Source.
 */
Source *
Sinit(what, who)
SourceControl what;
VPTR who;
{
    Source *tmp = (Source *) calloc((unsigned int)1, (unsigned int)sizeof(Source));

    if (tmp) {
	switch (tmp->sc = what) {
	    case SourceFile :
#ifdef HAVE_MMAP
		if (tmp->ss.sp.string = fmap((FILE *)who)) {
		    tmp->ss.sp.position =
			tmp->ss.sp.string + ftell((FILE *)who);
		    tmp->ss.sp.eof = feof((FILE *)who);
		    tmp->ss.sp.len = fmap_size(tmp->ss.sp.string);
		    tmp->sc = SourceString;
		}
#endif /* HAVE_MMAP */
		tmp->ss_fp = (FILE *)who;
	    when SourceStack :
	    case SourceString :
		if ((tmp->ss.sp.string = savestr((char *)who)) == 0) {
		    free((char *)tmp);
		    return 0;
		}
		tmp->ss.sp.position = tmp->ss.sp.string;
		tmp->ss.sp.eof = 0;
		tmp->ss.sp.len = strlen(tmp->ss.sp.string);
	    when SourceArray :
		if ((tmp->ss.sp.string = joinv(NULL, who, "\n")) == 0) {
		    free((char *)tmp);
		    return 0;
		}
		tmp->ss.sp.position = tmp->ss.sp.string;
		tmp->ss.sp.eof = 0;
		tmp->ss.sp.len = strlen(tmp->ss.sp.string);
		tmp->sc = SourceString;
#ifdef NOT_NOW
		if (vcpy(&tmp->ss.ap.array, who) < 0) {
		    free((char *)tmp);
		    return 0;
		}
		tmp->ss.ap.position = tmp->ss.ap.array[0];
		tmp->ss.ap.indx = 0;
#endif /* NOT_NOW */
	}
	tmp->st = 0;
    }

    return tmp;
}

/* Sopen.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Open a Source.
 *
 * If the type of Source is SourceFile, open the file,
 * otherwise equivalent to Sinit.
 */
Source *
Sopen(what, who)
SourceControl what;
char *who;
{
    FILE *tmp;

    switch (what) {
	case SourceFile:
	    if (!(tmp = fopen(who, "r")))
		return 0;
	    return Sinit(what, tmp);
	default:
	    return Sinit(what, who);
    }
}

/* Sclose.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Close ss and free its contents.
 */
int
Sclose(ss)
Source *ss;
{
    int ret = 0;

    if (!ss)
	return EOF;

    if (ss->st)
	ret = Sclose(ss->st);

    switch ((int) (ss->sc)) {
	case SourceFile :
	    ret = fclose(ss->ss_fp);
	when SourceStack :
	case SourceString :
#ifdef HAVE_MMAP
	    if (ss->ss_fp) {
		funmap(ss->ss.sp.string);
		ret = fclose(ss->ss_fp);
	    } else
#endif /* HAVE_MMAP */
	    xfree(ss->ss.sp.string);
#ifdef NOT_NOW
	when SourceArray :
	    free_vec(ss->ss.ap.array);
#endif /* NOT_NOW */
	otherwise :
	    ret = EOF;
    }
    xfree((char *)ss);
    return ret;
}

/* Sgetc.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Get a character from ss.
 *
 * The SourceOrder determines whether ss will be examined first or its
 * associated stack will be.  Any node actually in a stack is examined
 * SourceFirst, and top-of-stack nodes are examined SourceLast.  The
 * restriction that SourceStack nodes never appear at the top of the
 * stack implies that SourceStack nodes are always accessed SourceFirst.
 *
 * See Spush and Spop for more information on stacking.
 */
static int
_Sgetc(ss, so)
Source *ss;
SourceOrder so;
{
    int c = EOF;

    if (!ss)
	return EOF;

    if (so == SourceLast)
	while (ss->st && (c = _Sgetc(ss->st, SourceFirst)) == EOF)
	    Spop(ss);
    if (c != EOF)
	return c;

    switch((int) (ss->sc)) {
	case SourceFile :
	    /* See Sgets() for remarks */
#if defined(MSDOS) && !defined WIN16
	    if (Sfileof (ss) == stdin)
		c = dos_getchar();
	    else
#endif /* MSDOS */
	    c = fgetc(ss->ss_fp);
	when SourceStack :
	case SourceString :
	    if (ss->ss.sp.eof || !ss->ss.sp.position)
		c = EOF;
	    else if (ss->ss.sp.position - ss->ss.sp.string >= ss->ss.sp.len ||
		    !*(ss->ss.sp.position)) {
		ss->ss.sp.eof = 1;
		if (ss->sc == SourceStack)
		    c = EOF;
		else
		    c = '\n';	/* Treat terminator as newline */
	    } else
		c = *((ss->ss.sp.position)++);
#ifdef NOT_NOW
	when SourceArray :
	    if (!ss->ss.ap.position)
		c = EOF;
	    else if (!*(ss->ss.ap.position)) {
		ss->ss.ap.position = ss->ss.ap.array[++(ss->ss.ap.indx)];
		c = '\n';	/* Treat terminator as newline */
	    } else
		c = *((ss->ss.ap.position)++);
#endif /* NOT_NOW */
	otherwise :
	    c = EOF;
    }

    return c;
}

/*
 * Get a character from ss.
 */
int
Sgetc(ss)
Source *ss;
{
    return _Sgetc(ss, SourceLast);
}

/*
 * Push back a character to ss.
 *
 * This is less efficient than it could be, because an entire SourceStack
 * node is created for a one-character pushback if there is already any
 * stack in existence.  This could be revamped to take a hidden third
 * parameter in the manner of Sgetc, so that we call recursively on the
 * stack with instructions on how to optimize space usage.
 *
 * There is currently no way to push back a '\0' character.
 */
int
Sungetc(c, ss)
int c;
Source *ss;
{
    char buf[2];

    if (!ss)
	return EOF;

    /* If ss has a stack, use it */
    if (ss->st) {
	buf[0] = c, buf[1] = 0;
	if (Sungets(buf, ss) < 0)
	    return EOF;
	return c;
    }

    switch((int) (ss->sc)) {
	case SourceFile :
	    return ungetc(c, ss->ss_fp);
	when SourceStack :
	case SourceString :
	    if (!ss->ss.sp.position ||
		    ss->ss.sp.position == ss->ss.sp.string
#ifdef HAVE_MMAP
		    || ss->ss.sp.position[-1] != c
#endif /* HAVE_MMAP */
		    ) {
		buf[0] = c, buf[1] = 0;
		if (Sungets(buf, ss) < 0)
		    return EOF;
	    } else {
#ifdef HAVE_MMAP
		--(ss->ss.sp.position);
#else
		*(--(ss->ss.sp.position)) = c;
#endif /* HAVE_MMAP */
		ss->ss.sp.eof = 0;
	    }
#ifdef NOT_NOW
	when SourceArray :
	    /* Mechanics of backing up the array pointer are too messy */
	    buf[0] = c, buf[1] = 0;
	    if (Sungets(buf, ss) < 0)
		return EOF;
#endif /* NOT_NOW */
	otherwise :
	    c = EOF;
    }
    return c;
}

/* Sgets.c	Copyright 1990, 1991 Z-Code Software Corp. */

static int
_Sbcopyx(src, dst, len, chr)
char *src, *dst;
long len;
int chr;
{
    char *orig = dst;

    while (len-- > 0 && (*dst++ = *src++) != chr)
	;

    return dst - orig;
}

/*
 * Copy len bytes from ss into buf.  If stop is nonzero, it is a character
 * beyond which characters should not be copied; e.g. stop = '\n' for the
 * equivalent of fgets() behavior.
 *
 * Note that buf is not guaranteed to be nul-terminated after _Scopy().
 */
static int
_Scopy(buf, len, ss, stop)
char *buf;
long len;
Source *ss;
int stop;
{
    long clen = 0;
    char *ix;

    buf[0] = 0;
    if (ss->ss.sp.eof || !ss->ss.sp.position)
	return 0;	/* EOF */
    else if (ss->ss.sp.position - ss->ss.sp.string >= ss->ss.sp.len ||
	    !*(ss->ss.sp.position)) {
	ss->ss.sp.eof = 1;
	buf[clen++] = '\n';	/* Treat terminator as newline */
	return clen;
    }
    clen = ss->ss.sp.len - (ss->ss.sp.position - ss->ss.sp.string);
    if (clen > len) {
	if (stop)
	    len = _Sbcopyx(ss->ss.sp.position, buf, len, stop);
	else
	    bcopy(ss->ss.sp.position, buf, len);
	ss->ss.sp.position += len;
	return len;
    } else {
	if (stop)
	    clen = _Sbcopyx(ss->ss.sp.position, buf, clen, stop);
	else
	    bcopy(ss->ss.sp.position, buf, clen);
	ss->ss.sp.position += clen;
	if ((ss->ss.sp.position - ss->ss.sp.string >= ss->ss.sp.len ||
		!*(ss->ss.sp.position)) &&
		len > clen && (!stop || buf[clen-1] != stop)) {
	    ss->ss.sp.eof = 1;
	    buf[clen++] = '\n';
	}
	return clen;
    }
}

/*
 * Get a string no longer than size-1 from ss.
 */
char *
Sgets(buf, size, ss)
char *buf;
long size;
Source *ss;
{
    int c = 0, len = 0;

    if (!ss)
	return NULL;
    buf[0] = 0;
    if (size <= 1)
	return buf;

    if (ss->sc == SourceFile &&
	    ss->ss_fp == stdin && isoff(glob_flags, REDIRECT)) {
	/*
	 * This fragment is Z-Mail specific.  If the current Source
	 * is standard input, and redirection isn't specified, go
	 * through Getstr() to perform the usual input translations.
	 * Note that Getstr() eventually comes back through Sgetc(),
	 * so in any other application this code could be omitted.
	 */
	if (istool > 1)
	    return NULL;
	/* I wish this part were more modular ... */
	len = Getstr(prompt2, buf, (int)size, 0);
    } else {
	/* Big optimization using block copy unless ss->st */
	while (ss->st &&
		len < (size - 1) && (c = _Sgetc(ss, SourceLast)) != EOF) {
	    buf[len++] = c;
	    if (c == '\n' || c == '\r')
		goto Sgots;
	}
	if (!ss->st && len < (size - 1)) {
	    if (ss->sc == SourceFile) {
		if (fgets(buf + len, size - len, Sfileof(ss)))
		    return buf;
	    } else if (ss->sc == SourceString) {
		len += _Scopy(buf + len, size - len - 1, ss, '\n');
	    } else	/* This branch should never be taken */
		while (len < (size - 1) &&
			(c = _Sgetc(ss, SourceLast)) != EOF) {
		    buf[len++] = c;
		    if (c == '\n' || c == '\r')
			goto Sgots;
		}
	}
Sgots:
	buf[len] = 0;
    }
    return len > 0? buf : (char *) 0;
}

/* Sread.c	Copyright 1990, 1991 Z-Code Software Corp. */

int
Sread(buf, elsize, nelems, ss)
char *buf;
unsigned elsize, nelems;
Source *ss;
{
    int c, len = 0, clen;

    if (!ss)
	return -1;
    buf[0] = 0;
    if (nelems == 0)
	return 0;

    /* Big optimization here */
    if (!ss->st) {
	if (ss->sc == SourceFile)
	    return fread(buf, elsize, nelems, Sfileof(ss));
	else if (ss->sc == SourceString)
	    return _Scopy(buf, elsize * nelems, ss, 0);
    }	

    clen = elsize * nelems;
    while (len < clen && (c = _Sgetc(ss, SourceLast)) != EOF)
	buf[len++] = c;
    return len;
}

/* Sseek.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * This simply does not work -- you can't seek backwards into the stack,
 * and even within the actual Source seeking is limited.  The price you
 * pay for unlimited pushback is limited mobility ...
 */

long
Sseek(ss, off, whence)
Source *ss;
long off;
int whence;
{
    if (!ss)
	return -1;

    switch((int) (ss->sc)) {
	case SourceFile :
	    return fseek(ss->ss_fp, off, whence);
	when SourceStack :
	case SourceString : {
	    long len = ss->ss.sp.len;

	    if (whence == 1)
		off += ss->ss.sp.position - ss->ss.sp.string;
	    if (off > len)
		off = len;
	    else if (off < 0)
		off = 0;
	    switch (whence) {
		case 0: case 1:
		    ss->ss.sp.position = &(ss->ss.sp.string[off]);
		    ss->ss.sp.eof = (off == len);
		when 2:
		    ss->ss.sp.position = &(ss->ss.sp.string[len - off]);
		    ss->ss.sp.eof = (off == 0);
		otherwise :
		    return -1;
	    }
	}
	when SourceArray :
#ifdef NOT_NOW
	    if (off == 0 && whence == 0) {
		ss->ss.ap.indx = 0;
		ss->ss.ap.position = ss->ss.ap.array[0];
	    } else if (off == 0 && whence == 2) {
		while (ss->ss.ap.array[ss->ss.ap.indx])
		    (ss->ss.ap.indx)++;
		ss->ss.ap.position = NULL;
	    } else
#endif /* NOT_NOW */
		return -1; /* Not yet supported */
	otherwise :
	    return 0;
    }
    return 0;
}

/* Stell.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Report the offset within the current Source.
 *
 * This works only for SourceFile and SourceString at the moment, and
 * can be badly wrong if there is a stack ....
 */
long
Stell(ss)
Source *ss;
{
    if (!ss)
	return -1;

    switch((int) (ss->sc)) {
	case SourceFile :
	    return ftell(ss->ss_fp);
	when SourceStack :
	case SourceString :
	    return ss->ss.sp.position - ss->ss.sp.string;
	when SourceArray :
	    return -1; /* Not yet supported */
	otherwise :
	    return -1;
    }
    return 0;
}

/* Spush.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Push ss onto st.
 *
 * It is an error for ss to have or be in a stack, and also an error
 * for st to be a SourceStack node.  Stack nodes may be inserted into
 * a stack but must not be the top of the stack.  See Sgetc for stack
 * reading semantics.
 */
int
Spush(ss, st)
Source *ss, *st;
{
    if (!ss)
	return 0;
    if (ss->st || !st || st->sc == SourceStack)
	return -1;

    ss->st = st->st;
    st->st = ss;

    return 0;
}

/* Squeue.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Queue ss onto st.
 *
 * If st currently has no stack, then this is equivalent to Spush.  Queuing
 * is only relative to other pushbacks, not to the original Source (st).
 *
 * In contrast to Spush, ss is allowed to have an existing stack, which is
 * queued along with it.
 */
int
Squeue(ss, st)
Source *ss, *st;
{
    Source **tmp;

    if (!ss || !st || st->sc == SourceStack)
	return -1;

    /* Find the bottom of the Source stack */
    for (tmp = &ss->st; *tmp; tmp = &((*tmp)->st))
	;
    *tmp = ss;

    return 0;
}

/* Spop.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Pop the stack associated with ss.
 *
 * SourceStack nodes are handled specially.  A stack node must never be
 * the top node in a stack (see Spush and Sgetc).  If we pop a stack node,
 * make it appear that we've read all data from that node.  This should
 * never happen and is really a hack for Sgetc error-handling.
 */
void
Spop(ss)
Source *ss;
{
    Source *tmp;

    if (!ss)
	return;

    if (tmp = ss->st) {
	ss->st = tmp->st;
	tmp->st = 0;
	(void) Sclose(tmp);
    } else if (ss->sc == SourceStack)
        ss->ss.sp.eof = 1;
}

/* This stuff should be somewhere else ... requires shell/file.c */

/*
 * Copy len bytes from ss to outfp, starting at start.  Return the
 * number of bytes successfully copied, or -1 on error.
 *
 * When start == -1, start at the current seek position.
 * When len == -1, copy through end-of-Source.
 *
 * This isn't done with fioxlate() because of file mode magic
 * for DOS and other systems that differentiate binary/text files.
 */
long
Source_to_fp(ss, start, len, outfp)
Source *ss;
FILE *outfp;
long start, len;
{
#ifdef MSDOS
    int old_outmode;
#endif /* MSDOS */

    if (!ss)
	return -1;

    switch((int) (ss->sc)) {
	case SourceFile :
	    return fp_to_fp(Sfileof(ss), start, len, outfp);
	when SourceStack :
	case SourceString :
	    if (start >= 0)
		Sseek(ss, start, 0);
	    if (len < 0)
		len = ss->ss.sp.len - Stell(ss);
#ifdef MSDOS
	    (void) fflush(outfp);
	    old_outmode = _setmode(fileno(outfp), _O_BINARY);
#endif /* MSDOS */
	    start = fwrite(ss->ss.sp.position, sizeof(char), len, outfp);
	    if (fflush(outfp) == EOF)
		start = -1;
#ifdef MSDOS
	    if (old_outmode != _O_BINARY)
		_setmode(fileno(outfp), old_outmode);
#endif /* MSDOS */
	    if (start > 0)
		Sseek(ss, start, 1);
	    return start;
	when SourceArray :
	    return -1; /* Not yet supported */
	otherwise :
	    return -1;
    }
}

#ifdef NOT_NOW
/* Poor man's mmap() */
Source *
fp_to_Source(infp, start, len)
FILE *infp;
long start, len;
{
    long i;
    char *p;
    Source *tmp = (Source *) calloc((unsigned int)1, (unsigned int)sizeof(Source));

    if (!tmp || start >= 0 && fseek(infp, start, 0) < 0)
	return 0;
    if (len <= 0)
	return 0;
    else {
	if (!(p = (char *) malloc(len +1))) {
	    xfree((char  *)tmp);
	    return 0;
	}
	while (len > 0 &&
		(i = fread(p, sizeof(char), len, infp))) {
	    len -= i;
	    ss->ss.sp.len += i;
	}
    }
    tmp->ss.sp.string = tmp->ss.sp.position = p;
    return tmp;
}
#endif /* NOT_NOW */
