/* Source.h     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef SOURCE_H
#define SOURCE_H

#include <general.h>
#include <stdio.h>

typedef enum {
    SourceFile,		/* Source from a file pointer */
    SourceString,	/* Source from a string */
    SourceArray,	/* Source from an array of strings */
    SourceStack		/* Pushback string on a Source */
} SourceControl;

typedef struct Source {
    SourceControl sc;
#ifdef HAVE_MMAP
    FILE *ss_fp;
#endif /* HAVE_MMAP */
    union {
#ifndef HAVE_MMAP
	FILE *fp;
#endif /* HAVE_MMAP */
	struct {
	    char **array, *position;
	    int indx;
	} ap;
	struct {
	    char *string, *position;
	    int eof;
	    int len;
	} sp;
    } ss;
    struct Source *st;
} Source;

extern Source
    *Sopen(), *stdSource, *inSource;

extern Source *Sinit P((SourceControl, VPTR));

extern char
    *Sgets();

extern long
    Sseek(), Stell();

extern int
    Sclose(), Sgetc(), Sungetc(), Sread(), Spush(), Squeue();

extern void
    Spop();

#define Sungets(STR, SS)	Spush(Sinit(SourceStack, STR), SS)

#ifdef HAVE_MMAP
#define Sfileof(SS) (SS->ss_fp)
#else /* HAVE_MMAP */
#define Sfileof(SS) (SS->sc == SourceFile? SS->ss.fp : NULL_FILE)
#endif /* HAVE_MMAP */

extern long Source_to_fp();

#endif /* SOURCE_H */
