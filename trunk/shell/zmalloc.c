/* malloc.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * This file contains all memory-management routines used by Z-Mail.
 * These include the zmMem stack-style memory allocation routines,
 * the various forms of debugging mallocs, and the INTERNAL_MALLOC
 * routines borrowed from Perl.  See comments below for information
 * about the originator of the Perl malloc code (it wasn't Larry).
 */

/*
 * zmMem stack-allocation routines
 *
 * Simple front-end to malloc, with simple garbage collection.
 * Items allocated by this package are kept in a stack,
 * implemented by a linked list (each item is preceded by a pointer
 * to the previous item).  Therefore the most-recently-allocated
 * items are the most quickly accessible.
 *
 * Available functions are the following; with the exception of
 * zmMemFreeAfter, the functionality of each is the same as that of
 * the corresponding function in the standard C library (except
 * that the free functions return type void).
 *	void *zmMemMalloc(unsigned n);
 *	void zmMemFree(void *p);
 *	void *zmMemRealloc(void *p, unsigned n);
 *	void *zmMemCalloc(unsigned n, unsigned siz);
 *	void zmMemCfree(void *p);
 *	void zmMemFreeAfter(void *p);
 *
 * zmMemFreeAfter(p) frees everything whose original allocation occured
 * after the original allocation of p.  In the vicinity of a setjmp,
 * the following should be used:
 *
 *	void *mark;
 *	mark = zmMemMalloc(0);
 *	if (setjmp(jmpbuf)) {
 *		/*
 *		 * Just returned from a longjmp in which zmMemMalloc's
 *		 * may have been called but not freed.
 *		 * /
 *		zmMemFreeAfter(mark);
 *	}
 *
 *
 * Notes:
 *	1) 0 is a valid argument for zmMemMalloc; a valid pointer
 *	   will be returned.  NULL is returned iff malloc fails.
 *	2) zmMemFree, zmMemCfree, and zmMemRealloc will accept NULL
 *	   as an argument, an which case they simply return or
 *	   call zmMalloc, as appropriate.
 *	3) If ZM_MEM_BOUNDARY is #defined (as 1,2,4,8, or 16) then all
 *	   pointers returned will be aligned correspondingly.
 *	   The default value is 8.
 */

#include "zmail.h"	/* for ZmErrWarning and type checking */
#include "zmalloc.h"
#include "catalog.h"

#ifndef ZM_MEM_BOUNDARY
#define ZM_MEM_BOUNDARY 8 /* an 8-byte boundary is "suitable for any purpose" */
#endif /* ZM_MEM_BOUNDARY */

#define ALIGN_UP(x) (((long)(x) + (ZM_MEM_BOUNDARY-1)) &~ (ZM_MEM_BOUNDARY-1))

struct zmMemHdr {
    struct zmMemHdr *prev;
#if (ZM_MEM_BOUNDARY >= 8)
    long pad;
#endif
};
static struct zmMemHdr *zmMemTop = 0;

void
zmMemPrintStack()
{
    struct zmMemHdr *p;
    print("zmMem list: (starting with top)\n" );
    for (p = zmMemTop; p; p = p->prev)
	print("\t%#x\n", p);
}

extern VPTR
zmMemMalloc(n)
unsigned n;
{
    struct zmMemHdr *p = (struct zmMemHdr *)malloc(sizeof(struct zmMemHdr) + n);
    if (debug > 4) {
	zmDebug("zmMemMalloc(%d)\n", n);
	zmMemPrintStack();
    }
    if (!p)
	return 0;
    p->prev = zmMemTop;
    zmMemTop = p;
    return (VPTR)(p+1);
}

extern void
zmMemFree(cp)
VPTR cp;
{
    struct zmMemHdr *p, **pp;
    if (debug > 4) {
	zmDebug("zmMemFree(%#x)\n", cp);
	zmMemPrintStack();
    }
    if (!cp)
	return;
    p = ((struct zmMemHdr *)cp)-1;
    for (pp = &zmMemTop; *pp; pp = &(*pp)->prev)
	if (*pp == p) {
	    *pp = p->prev;	/* delete p from linked list */
	    if (debug > 4) {
		zmDebug("zmMemFree: calling free(%#x)\n", p);
		zmMemPrintStack();
	    }
	    free(p);
	    return;
	}
    error(ZmErrWarning, catgets( catalog, CAT_SHELL, 469, "zmMemFree: cannot find %#x!\n" ), cp);

    zmMemPrintStack();
}

extern void 
zmMemFreeAfter(cp)
VPTR cp;
{
    struct zmMemHdr *p, *thing_to_free;
    if (debug > 4) {
	zmDebug("zmMemFreeAfter(%#x)\n", cp);
	zmMemPrintStack();
    }
    if (!cp)
	return;
    p = ((struct zmMemHdr *)cp)-1;
    while (zmMemTop && zmMemTop != p) {
	thing_to_free = zmMemTop;
	zmMemTop = zmMemTop->prev;
	if (debug > 4) {
	    zmDebug("    zmMemFreeAfter: calling free(%#x)\n", thing_to_free);
	    zmMemPrintStack();
	}
	free(thing_to_free);
    }
    if (!zmMemTop)
	error(ZmErrWarning,
		catgets( catalog, CAT_SHELL, 472, "zmMemFreeAfter: Freed everything and didn't find %#x!" ), cp);
}

extern VPTR
zmMemRealloc(cp, n)
VPTR cp;
unsigned n;
{
    struct zmMemHdr *p, *newp, **pp;

    if (debug > 4) {
	zmDebug("zmMemRealloc(%#x, %d)\n", cp, n);
	zmMemPrintStack();
    }

    if (!cp)
	return zmMemMalloc(n);

    p = ((struct zmMemHdr *)cp)-1;

    for (pp = &zmMemTop; *pp; pp = &(*pp)->prev)
	if (*pp == p)
	    break;

    if (!*pp) {
	error(ZmErrWarning, catgets( catalog, CAT_SHELL, 474, "zmMemRealloc: cannot find %#x!\n" ), cp);
	return 0;
    }

    newp = (struct zmMemHdr *)realloc(p, n + sizeof(struct zmMemHdr));
    if (!newp) {
	error(ZmErrWarning, catgets( catalog, CAT_SHELL, 475, "zmMemRealloc(%#x, %d): realloc(%#x, %d) failed\n" ),
				cp, n, (char *) cp - sizeof(struct zmMemHdr),
				        n + sizeof(struct zmMemHdr));
	return 0;
    }
    *pp = newp;
    return (VPTR)(newp + 1);
}

extern VPTR
zmMemCalloc(n, siz)
unsigned n, siz;
{
    VPTR p = zmMemMalloc(n*siz);
    if (p)
	bzero(p, (int)(n*siz));
    return p;
}

extern void
zmMemCfree(cp)
VPTR cp;
{
    zmMemFree(cp);
}

/*
 * This is a slightly modified version of the malloc.c distributed with
 * Larry Wall's perl 2.0 sources.  RCS and sccs information has been
 * retained, but modified so that it will not actually affect checkin
 * or checkout of this file if revision control is used for Z-Mail.
 *
 * Other changes include:
 *	Removal of the ASSERT macro and other code related to the
 *	preprocessor definition "debug"
 *
 *	Replaced #include "perl.h" with #include "zmail.h" (guess why)
 *
 *	Warning messages are now printed with the Z-Mail Debug macro,
 *	that is, they are normally suppressed.
 *
 *	Added a calloc() function, using Z-Mail's bzero()
 *
 * Also, the Z-Mail xfree() and free_vec() functions have been moved here.
 */

#ifdef MALLOC_UTIL
/*
 * Undefine stuff from zmalloc.h -- this file is at a lower level.
 */
#undef malloc
#undef calloc
#undef realloc
#undef free
#undef xfree
#undef free_elems
#undef free_vec
#endif /* MALLOC_UTIL */

#ifdef MALLOC_TRACE
/*
 * Everyone else calls the functions in malloc_trace.c,
 * which call these functions:
 */
#define malloc malloC
#define free freE
#define realloc realloC
#define calloc calloC
#define cfree cfreE
#endif /* MALLOC_TRACE */

/*
 * Compile this portion only if configured for INTERNAL_MALLOC
 */
#ifdef INTERNAL_MALLOC
#ifdef SYSV
#include <memory.h>
#endif /* SYSV */
#ifndef MALLOC_TRACE
#define free xfree	/* rename free for Z-Mail purposes */
#endif /* MALLOC_TRACE */

/* Begin modified perl malloc.c */

/* Header: malloc.c,v 2.0 88/06/05 00:09:16 root Exp
 *
 * Log:	malloc.c,v
 * Revision 2.0  88/06/05  00:09:16  root
 * Baseline version 2.0.
 * 
 */

#ifndef lint
static char sccsid[] = "malloc.c	4.3 (Berkeley) 9/16/83";
#endif /* !lint */

#define RCHECK
/*
 * malloc.c (Caltech) 2/21/82
 * Chris Kingsley, kingsley@cit-20.
 *
 * This is a very fast storage allocator.  It allocates blocks of a small 
 * number of different sizes, and keeps free lists of each size.  Blocks that
 * don't exactly fit are passed up to the next larger size.  In this 
 * implementation, the available sizes are 2^n-4 (or 2^n-12) bytes long.
 * This is designed for use in a program that uses vast quantities of memory,
 * but bombs when it runs out. 
 */

/* I don't much care whether these are defined in sys/types.h--LAW */

#undef u_char
#define u_char unsigned char
#undef u_int
#define u_int unsigned int
#undef u_short
#define u_short unsigned short

/*
 * The overhead on a block is at least 4 bytes.  When free, this space
 * contains a pointer to the next free block, and the bottom two bits must
 * be zero.  When in use, the first byte is set to MAGIC, and the second
 * byte is the size index.  The remaining bytes are for alignment.
 * If range checking is enabled and the size of the block fits
 * in two bytes, then the top two bytes hold the size of the requested block
 * plus the range checking words, and the header word MINUS ONE.
 */
union	overhead {
	union	overhead *ov_next;	/* when free */
	struct {
#ifdef RCHECK
		u_short	ovu_size;	/* actual block size */
		u_int	ovu_rmagic;	/* range magic number */
#endif /* RCHECK */
		u_char	ovu_magic;	/* magic number */
		u_char	ovu_index;	/* bucket # */
	} ovu;
#define	ov_magic	ovu.ovu_magic
#define	ov_index	ovu.ovu_index
#define	ov_size		ovu.ovu_size
#define	ov_rmagic	ovu.ovu_rmagic
};

#define	MAGIC		0xff		/* magic # on accounting info */
#define OLDMAGIC	0x7f		/* same after a free() */
#define RMAGIC		0x55555555	/* magic # on range info */
#ifdef RCHECK
#define	RSLOP		sizeof (u_int)
#else /* !RCHECK */
#define	RSLOP		0
#endif /* RCHECK */

/*
 * nextf[i] is the pointer to the next free block of size 2^(i+3).  The
 * smallest allocatable block is 8 bytes.  The overhead information
 * precedes the data area returned to the user.
 */
#define	NBUCKETS 30
static	union overhead *nextf[NBUCKETS];
extern	VPTR sbrk();

#ifdef MSTATS
/*
 * nmalloc[i] is the difference between the number of mallocs and frees
 * for a given block size.
 */
static	u_int nmalloc[NBUCKETS];
#endif /* MSTATS */

VPTR
malloc(nbytes)
	register unsigned nbytes;
{
	register union overhead *p;
	register int bucket = 0;
	register unsigned shiftr;

	if (nbytes == 0)
#ifdef GUI
	    nbytes = 1; /* Xt and/or Xm may malloc(0) */
#else
	    return NULL;
#endif /* GUI */
	/*
	 * Convert amount of memory requested into
	 * closest block size stored in hash buckets
	 * which satisfies request.  Account for
	 * space used per block for accounting.
	 */
	nbytes += sizeof (union overhead) + RSLOP;
	nbytes = (nbytes + 3) &~ 3; 
	shiftr = (nbytes - 1) >> 2;
	/* apart from this loop, this is O(1) */
	while (shiftr >>= 1)
		bucket++;
	/*
	 * If nothing in hash bucket right now,
	 * request more memory from the system.
	 */
	if (nextf[bucket] == (union overhead *)0)    
		morecore(bucket);
	if ((p = (union overhead *)nextf[bucket]) == (union overhead *)0)
		return (NULL);
	/* remove from linked list */
#ifdef OLD_STUFF
	if (*((int*)p) > 0x10000000)
	    Debug("Corrupt malloc ptr 0x%x at 0x%x\n",*((int*)p),p);
#else /* new */
#ifdef RCHECK
	if (*((int*)p) & (sizeof(union overhead) - 1))
	    Debug("Corrupt malloc ptr 0x%x at 0x%x\n",*((int*)p),p);
#endif /* RCHECK */
#endif /* OLD_STUFF */
	nextf[bucket] = nextf[bucket]->ov_next;
	p->ov_magic = MAGIC;
	p->ov_index= bucket;
#ifdef MSTATS
	nmalloc[bucket]++;
#endif /* MSTATS */
#ifdef RCHECK
	/*
	 * Record allocated size of block and
	 * bound space with magic numbers.
	 */
	if (nbytes <= 0x10000)
		p->ov_size = nbytes - 1;
	p->ov_rmagic = RMAGIC;
	*((u_int *)((caddr_t)p + nbytes - RSLOP)) = RMAGIC;
#endif /* RCHECK */
	return (VPTR)(p + 1);
}

/*
 * Allocate more memory to the indicated bucket.
 */
static
morecore(bucket)
	register bucket;
{
	register union overhead *op;
	register int rnu;       /* 2^rnu bytes will be requested */
	register int nblks;     /* become nblks blocks of the desired size */
	register int siz;

	if (nextf[bucket])
		return;
	/*
	 * Insure memory is allocated
	 * on a page boundary.  Should
	 * make getpageize call?
	 */
	op = (union overhead *)sbrk(0);
	if ((long)op & 0x3ff)
		sbrk(1024 - ((long)op & 0x3ff));
	/* take 2k unless the block is bigger than that */
	rnu = (bucket <= 8) ? 11 : bucket + 3;
	nblks = 1 << (rnu - (bucket + 3));  /* how many blocks to get */
	if (rnu < bucket)
		rnu = bucket;
	op = (union overhead *)sbrk(1 << rnu);
	/* no more room! */
	if ((long)op == -1)
		return;
	/*
	 * Round up to minimum allocation size boundary
	 * and deduct from block count to reflect.
	 */
	if ((long)op & 7) {
		op = (union overhead *)(((long)op + 8) &~ 7);
		nblks--;
	}
	/*
	 * Add new memory allocated to that on
	 * free list for this hash bucket.
	 */
	nextf[bucket] = op;
	siz = 1 << (bucket + 3);
	while (--nblks > 0) {
		op->ov_next = (union overhead *)((caddr_t)op + siz);
		op = (union overhead *)((caddr_t)op + siz);
	}
}

void
free(cp)
VPTR cp;
{   
	register int size;
	register union overhead *op;
#if defined(DARWIN)
	char *end = (char *) get_end();
#else
	extern char end[];
#endif

	if (cp == NULL || debug > 5)
		return;
	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
	if (op->ov_magic != MAGIC) {
		Debug("%s free() ignored\n",
		    cp < end || cp >= &cp ? "Non-heap" :
		    op->ov_magic == OLDMAGIC ? "Duplicate" : "Bad" );
		return;				/* sanity */
	}
	op->ov_magic = OLDMAGIC;
#ifdef RCHECK
	if (op->ov_rmagic != RMAGIC) {
		Debug("Range check failed, free() ignored\n" );
		return;
	}
	if (op->ov_index <= 13 &&
	    *(u_int *)((caddr_t)op + op->ov_size + 1 - RSLOP) != RMAGIC) {
		Debug("Range check failed, free() ignored\n" );
		return;
	}
#endif /* RCHECK */
	if (op->ov_index >= NBUCKETS)
	    return;
	size = op->ov_index;
	op->ov_next = nextf[size];
	nextf[size] = op;
#ifdef MSTATS
	nmalloc[size]--;
#endif /* MSTATS */
}

/*
 * When a program attempts "storage compaction" as mentioned in the
 * old malloc man page, it realloc's an already freed block.  Usually
 * this is the last block it freed; occasionally it might be farther
 * back.  We have to search all the free lists for the block in order
 * to determine its bucket: 1st we make one pass thru the lists
 * checking only the first block in each; if that fails we search
 * ``reall_srchlen'' blocks in each list for a match (the variable
 * is extern so the caller can modify it).  If that fails we just copy
 * however many bytes was given to realloc() and hope it's not huge.
 */
int reall_srchlen = 4;	/* 4 should be plenty, -1 =>'s whole list */

VPTR
realloc(cp, nbytes)
	VPTR cp; 
	unsigned nbytes;
{   
	register u_int onb;
	union overhead *op;
	VPTR res;
	register int i;
	int was_alloced = 0;

	if (cp == NULL)
		return (malloc(nbytes));
	op = (union overhead *)((caddr_t)cp - sizeof (union overhead));
	if (op->ov_magic == MAGIC) {
		was_alloced++;
		i = op->ov_index;
	} else {
		/*
		 * Already free, doing "compaction".
		 *
		 * Search for the old block of memory on the
		 * free list.  First, check the most common
		 * case (last element free'd), then (this failing)
		 * the last ``reall_srchlen'' items free'd.
		 * If all lookups fail, then assume the size of
		 * the memory block being realloc'd is the
		 * smallest possible.
		 */
		if ((i = findbucket(op, 1)) < 0 &&
		    (i = findbucket(op, reall_srchlen)) < 0)
			i = 0;
	}
	onb = (1 << (i + 3)) - sizeof (*op) - RSLOP;
#ifdef RCHECK
#ifdef NOT_NOW
	if (was_alloced) {
	    if (op->ov_rmagic != RMAGIC) {
		    Debug("Range check failed, realloc() failed\n" );
		    return NULL;
	    }
	    if (op->ov_index <= 13 &&
		*(u_int *)((caddr_t)op + op->ov_size + 1 - RSLOP) != RMAGIC) {
		    Debug("Range check failed, realloc() failed\n" );
		    return NULL;
	    }
	}
#endif /* NOT_NOW */
	/* There's something wrong with the "onb" size computation when
	 * RCHECK is defined, but I can't figure out what it is.  Bart.
	 */
	if (was_alloced) {
		free(cp);	/* hack to have a chance of res == cp */
		was_alloced = 0;
	}
#else /* RCHECK */
	/* avoid the copy if same size block */
	if (was_alloced &&
	    nbytes <= onb && nbytes > (onb >> 1) - sizeof(*op) - RSLOP) {
#ifdef RCHECK
		/*
		 * Record new allocated size of block and
		 * bound space with magic numbers.
		 */
		if (op->ov_index <= 13) {
			/*
			 * Convert amount of memory requested into
			 * closest block size stored in hash buckets
			 * which satisfies request.  Account for
			 * space used per block for accounting.
			 */
			nbytes += sizeof (union overhead) + RSLOP;
			nbytes = (nbytes + 3) &~ 3; 
			op->ov_size = nbytes - 1;
			*((u_int *)((caddr_t)op + nbytes - RSLOP)) = RMAGIC;
		}
#endif /* RCHECK */
		return(cp);
	}
#endif /* RCHECK */
	if ((res = malloc(nbytes)) == NULL)
		return (NULL);
	if (cp != res)			/* common optimization */
		bcopy(cp, res, (nbytes < onb) ? nbytes : onb);
	if (was_alloced)
		free(cp);
	return (res);
}

/*
 * Search ``srchlen'' elements of each free list for a block whose
 * header starts at ``freep''.  If srchlen is -1 search the whole list.
 * Return bucket number, or -1 if not found.
 */
static
findbucket(freep, srchlen)
	union overhead *freep;
	int srchlen;
{
	register union overhead *p;
	register int i, j;

	for (i = 0; i < NBUCKETS; i++) {
		j = 0;
		for (p = nextf[i]; p && j != srchlen; p = p->ov_next) {
			if (p == freep)
				return (i);
			j++;
		}
	}
	return (-1);
}

#ifdef MSTATS
/*
 * mstats - print out statistics about malloc
 * 
 * Prints two lines of numbers, one showing the length of the free list
 * for each size category, the second showing the number of mallocs -
 * frees for each size category.
 */
void 
mstats(s)
char *s;
{
	register int i, j;
	register union overhead *p;
	int totfree = 0,
	totused = 0;

	Debug("Memory allocation statistics %s\nfree:\t" , s);
	for (i = 0; i < NBUCKETS; i++) {
		for (j = 0, p = nextf[i]; p; p = p->ov_next, j++)
			;
		Debug(" %d", j);
		totfree += j * (1 << (i + 3));
	}
	Debug("\nused:\t" );
	for (i = 0; i < NBUCKETS; i++) {
		Debug( " %d", nmalloc[i]);
		totused += nmalloc[i] * (1 << (i + 3));
	}
	Debug("\n\tTotal in use: %d, total free: %d\n" ,
	    totused, totfree);
}
#endif /* MSTATS */

/* End of modified perl malloc.c */

VPTR
calloc(nitems, itemsz)
u_int nitems, itemsz;
{
    VPTR cp;

    if (cp = malloc(nitems * itemsz))
	bzero(cp, nitems * itemsz);
    return cp;
}

#ifdef MALLOC_TRACE

void
cfree(p)
VPTR p;
{
    free(p);
}

#undef free

void
xfree(p)
     VPTR p;
{
#if defined(DARWIN)
	char *end = (char *) get_end();
#else
	extern char end[];
#endif
    if (p && p >= end && p < &p && debug < 5)
	free(p);
}

#else /* !MALLOC_TRACE */

/* These are needed for curses and other external linkage */

#ifndef MAC_OS
#undef free
#endif /* !MAC_OS */

VPTR
cfree(p, n, s)
VPTR p;
u_int n, s;
{
    xfree(p);
    return NULL;
}

#ifdef HAVE_VOID_FREE
void
#endif /* HAVE_VOID_FREE */
#ifdef HAVE_CHAR_FREE
VPTR
#endif /* HAVE_CHAR_FREE */
free(p)
VPTR p;
{
    xfree(p);
#ifndef HAVE_VOID_FREE
    return 0;
#endif /* HAVE_VOID_FREE */
}
#endif /* !MALLOC_TRACE */

#else /* !INTERNAL_MALLOC */ /* RJL 3.2.93 - ! */

VPTR stackbottom;		/* set first thing in main() */

void
xfree(cp)
     VPTR cp;
{
#if defined(DARWIN)
	char *end = (char *) get_end();
#else
	extern char end[];
#endif

    if (cp && cp >= (VPTR) end && cp < stackbottom
	&& cp < (VPTR) &cp && debug < 5)
        free(cp);
}

#endif /* INTERNAL_MALLOC */

#ifdef MALLOC_TRACE
/*
 * The remainder of this section consists of utility functions that
 * should go through the malloc tracing routines below.
 * (They should probably not even be in this file.)
 */
#undef malloc
#undef free
#undef realloc
#undef calloc
#undef cfree
#undef free_elems
#endif /* MALLOC_TRACE */

void
free_elems(argv)
char **argv;
{
    register int n;

    if (!argv)
	return;
    for (n = 0; argv[n]; n++)
	xfree(argv[n]);
}

void
free_vec(argv)
char **argv;
{
    free_elems(argv);
    xfree(argv);
}

/*
 * malloc tracing routines
 *
 * A front-end to malloc/free/realloc/calloc/cfree that keeps track of
 * numbers of calls and stuff.
 * There are two versions, which you get by compiling with
 * -DMALLOC_UTIL or -DMALLOC_TRACE respectively.
 *
 * The -DMALLOC_UTIL version is Brandyn Webb's mutil.c.
 * It is portable to any system, but requires all modules using
 * it to be compiled #including "mutil.h", and mallocs
 * done with this module must be freed by this module.
 *
 * The following descriptions refer to the -DMALLOC_TRACE version.
 *
 *     The -DMALLOC_TRACE is done by tracing the stack.
 *     This depends on the functions get_stacktrace() and symbolize_stacktrace()
 *     from stacktrace.c, which is supported on sun-4's only (but will
 *     soon be on sun-3's and toshibas).
 *
 *     This file contains the functions malloc,free,realloc,calloc, and cfree.
 *     It depends on the existence of the "real" functions,
 *     which should be named malloC,freE,realloC,calloC,cfreE.
 *     This can be done either by editing malloc.c or by changing
 *     the symbol names in malloc.o.
 *
 *     Additional functions in this file are:
 *    	   malloc_trace_info(verbose,show_by_default,files_and_functions)
 *    	   malloc_trace_reset()
 *
 */

#if defined(MALLOC_TRACE) || defined(MALLOC_UTIL)

/* Bart: Wed Sep 25 18:07:04 PDT 1991 */
#undef NULL
#define NULL 0

extern int get_stacktrace(), cleanup_symbolize_stacktrace();
extern char *symbolize_stacktrace();

#ifdef MALLOC_UTIL
#define real_malloc malloc
#define real_free free
#define real_realloc realloc
#define real_calloc calloc
#define real_cfree cfree
#else	/* !MALLOC_UTIL */
#define real_malloc malloC
#define real_free freE
#define real_realloc realloC
#define real_calloc calloC
#define real_cfree cfreE
#endif	/* !MALLOC_UTIL */

#ifndef MAX_MALLOC_TRACE_DEPTH
#define MAX_MALLOC_TRACE_DEPTH 4
#endif
#ifndef DEFAULT_MALLOC_TRACE_DEPTH
#define DEFAULT_MALLOC_TRACE_DEPTH 2
#endif

static int malloc_trace_depth = DEFAULT_MALLOC_TRACE_DEPTH;

#include <stdio.h>

/*
 * ALL source files using malloc and free must #include "zmalloc.h"
 */

#ifndef MUTIL_SHOW_FULL_NAME
#define MUTIL_SHOW_FULL_NAME 1
#endif

#define MAXMALLOCS 2000	/* max number of occurrances of malloc&free in source
			 * code */

typedef char *string;
typedef unsigned char *pointer;

static int numb_malloc_really = 0;	/* Not touched by reset */
static int numb_free_really = 0;	/* Not touched by reset */
static int max_diff_bytes = 0;	/* Not touched by reset */

static int num_malloc = 0;
static int num_free = 0;
static int numb_malloc = 0;
static int numb_free = 0;

typedef struct HistStruct *Hist;

struct HistStruct {
    int type;	/* mutil_FREE, mutil_MALLOC */
#ifdef MALLOC_UTIL
    string file;
    int lineno;
#else /* !MALLOC_UTIL */
    char *trace[MAX_MALLOC_TRACE_DEPTH];	/* return addresses */
#endif /* !MALLOC_UTIL */
    int calls;
    int bytes;

    int fcalls;
    int fbytes;
};

typedef struct {
    int numbytes;
    Hist hist;
} vomit;

static struct HistStruct HistList[MAXMALLOCS];
static int num_hist = 0;

#define mutil_FREE 0
#define mutil_MALLOC 1

void
#ifdef MALLOC_UTIL
mutil_reset()
#else	/* !MALLOC_UTIL */
malloc_trace_reset()
#endif	/* !MALLOC_UTIL */
{
    int i;

    for (i = 0; i < num_hist; i++) {
	HistList[i].calls = 0;
	HistList[i].bytes = 0;
	HistList[i].fcalls = 0;
	HistList[i].fbytes = 0;
    }

    num_malloc = 0;
    num_free = 0;
    numb_malloc = 0;
    numb_free = 0;
}

#ifdef MALLOC_UTIL
static Hist 
findhist(file, lineno, type)
string file;
int lineno;
int type;
#else	/* !MALLOC_UTIL */
static Hist 
findhist(trace, type)
char *trace[];
int type;
#endif	/* !MALLOC_UTIL */
{
    int i, j;
    Hist h;

#if (!MUTIL_SHOW_FULL_NAME)
    string rindex(), p = rindex(file, '/');

    if (p)
	file = p + 1;
#endif

    for (i = 0, h = HistList; i < num_hist; i++, h++) {
#ifdef MALLOC_UTIL
	if (lineno != h->lineno)
	    continue;
	if (type != h->type)
	    continue;
	if (strcmp(file, h->file))
	    continue;
#else	/* !MALLOC_UTIL */
	for (j = 0; j < malloc_trace_depth; ++j)
	    if (trace[j] != h->trace[j])
		break;
	if (j == malloc_trace_depth)
#endif	/* !MALLOC_UTIL */
	    return (h);
    }

    if (num_hist >= MAXMALLOCS) {
#ifdef MALLOC_UTIL
	fprintf(stderr,
	    catgets( catalog, CAT_SHELL, 489, "Mutil %s Error in %s at %d: too many unique mallocs and frees\n" ),
		((type == mutil_FREE) ? "Free" : "Malloc" ),
		file, lineno);
#else	/* !MALLOC_UTIL */
	fprintf(stderr,
		catgets( catalog, CAT_SHELL, 492, "Malloc_trace %s Error: too many unique mallocs and frees\n" ),
		((type == mutil_FREE) ? "Free" : "Malloc" ));
#endif	/* !MALLOC_UTIL */
	/*
	 * Return an arbitrary entry so we don't get a segmentation
	 * violation-- the info will be messed up, but who cares
	 */
	return HistList + MAXMALLOCS - 1;
    }
    num_hist++;

    h->type = type;
#ifdef MALLOC_UTIL
    h->file = file;
    h->lineno = lineno;
#else	/* !MALLOC_UTIL */
    for (j = 0; j < malloc_trace_depth; ++j)
	h->trace[j] = trace[j];
#endif	/* !MALLOC_UTIL */
    h->calls = 0;
    h->bytes = 0;
    h->fcalls = 0;
    h->fbytes = 0;

    return (h);
}

/*
 * Do not attempt to reset this after the first call to malloc!
 */
static int env_malloc_trace = -1;

#ifdef MALLOC_UTIL
VPTR
mutil_malloc(n, file, lineno)
int n;
string file;
int lineno;
#else	/* !MALLOC_UTIL */
VPTR
malloc(n)
int n;
#endif	/* !MALLOC_UTIL */
{
    vomit *vom;
    Hist h;

#ifdef MALLOC_UTIL
    h = findhist(file, lineno, mutil_MALLOC);
#else	/* !MALLOC_UTIL */
    int i;
    char *trace[MAX_MALLOC_TRACE_DEPTH];

    if (env_malloc_trace == -1) {
	char *p, *getenv();

	if (p = getenv("MALLOC_TRACE")) {
	    env_malloc_trace = 1;
	    if (*p)
		malloc_trace_depth = atoi(p);
	} else
	    env_malloc_trace = 0;
    }
    if (!env_malloc_trace)
	return real_malloc(n);

    for (i = 0; i < malloc_trace_depth; ++i)
	trace[i] = 0;
    get_stacktrace(1, malloc_trace_depth, trace);
    h = findhist(trace, mutil_MALLOC);
#endif	/* !MALLOC_UTIL */

    if ((vom = (vomit *) real_malloc(n + sizeof(vomit))) == NULL) {
#ifdef MALLOC_TOUCHY
#ifdef MALLOC_UTIL
	fprintf(stderr,
		catgets( catalog, CAT_SHELL, 495, "Error: Bad malloc (malloc(%d)) in %s at line %d\n" ),
		n, file, lineno);
	exit(1);
#endif	/* MALLOC_UTIL */
#ifdef MALLOC_TRACE
	fprintf(stderr,
		catgets( catalog, CAT_SHELL, 496, "Error: Bad malloc (malloc(%d)) -- trace:" ), n);
	for (i = 0; i < malloc_trace_depth; ++i)
	    fprintf(stderr, " %#x", trace[i]);
#endif	/* MALLOC_TRACE */
#endif	/* MALLOC_TOUCHY */
	return 0;
    }
    h->calls++;
    h->bytes += n;

    vom->numbytes = n;
    vom->hist = h;

    num_malloc++;
    numb_malloc += n;
    numb_malloc_really += n;

    if (numb_malloc_really - numb_free_really > max_diff_bytes)
	max_diff_bytes = numb_malloc_really - numb_free_really;

    return (vom + 1);
}

#ifdef HAVE_VOID_FREE
void
#endif /* HAVE_VOID_FREE */
#ifdef MALLOC_UTIL
mutil_free(p, file, lineno)
VPTR p;
string file;
int lineno;
#else	/* !MALLOC_UTIL */
free(p)
VPTR p;
#endif	/* !MALLOC_UTIL */
{
    vomit *vom;
    Hist h;

#ifndef MALLOC_UTIL
    int i;
    char *trace[MAX_MALLOC_TRACE_DEPTH];

    if (!env_malloc_trace) {
#ifdef HAVE_VOID_FREE
	real_free(p);
	return;
#else /* !HAVE_VOID_FREE */
	return real_free(p);
#endif /* HAVE_VOID_FREE */
    }

    for (i = 0; i < malloc_trace_depth; ++i)
	trace[i] = 0;
    get_stacktrace(1, malloc_trace_depth, trace);
#endif	/* !MALLOC_UTIL */

    if (p == NULL) {
#ifdef MALLOC_UTIL
	fprintf(stderr,
		catgets( catalog, CAT_SHELL, 497, "Error: Null pointer passed to free in %s at line %d\n" ),
		file, lineno);
#else	/* !MALLOC_UTIL */
	fprintf(stderr, catgets( catalog, CAT_SHELL, 498, "Error: Null pointer passed to free -- trace:" ));
	for (i = 0; i < malloc_trace_depth; ++i)
	    fprintf(stderr, " %#x", trace[i]);
	printf("\n");
#endif	/* !MALLOC_UTIL */
#ifdef HAVE_VOID_FREE
	return;
#else /* HAVE_VOID_FREE */
	return 0;
#endif /* HAVE_VOID_FREE */
    }
    vom = ((vomit *) p) - 1;
    if (vom->hist < HistList || vom->hist >= HistList + MAXMALLOCS) {
#ifdef MALLOC_UTIL
	fprintf(stderr,
		catgets( catalog, CAT_SHELL, 499, "Error: non-malloced pointer passed to free in %s at line %d\n" ),
		file, lineno);
#else	/* !MALLOC_UTIL */
	fprintf(stderr,
		catgets( catalog, CAT_SHELL, 500, "Error: non-malloced pointer passed to free -- trace:" ));
	for (i = 0; i < malloc_trace_depth; ++i)
	    fprintf(stderr, " %#x", trace[i]);
#endif	/* !MALLOC_UTIL */
#ifdef HAVE_VOID_FREE
	return;
#else /* HAVE_VOID_FREE */
	return 0;
#endif /* HAVE_VOID_FREE */
    }
    num_free++;
    numb_free += vom->numbytes;
    numb_free_really += vom->numbytes;

    vom->hist->fcalls++;
    vom->hist->fbytes += vom->numbytes;

#ifdef MALLOC_UTIL
    h = findhist(file, lineno, mutil_FREE);
#else	/* !MALLOC_UTIL */
    h = findhist(trace, mutil_FREE);
#endif	/* !MALLOC_UTIL */

    h->calls++;
    h->bytes += vom->numbytes;

#ifdef HAVE_VOID_FREE
    real_free(vom);
#else /* !HAS_VOID_FRE */
    return real_free(vom);
#endif /* HAVE_VOID_FREE */
}

#ifdef MALLOC_UTIL
/*
 * The next two functions were added for z-mail; stackbottom and debug
 * are zmail variables.  I don't know what the purpose of stackbottom is;
 * I took the logic from malloc.c. --DFH
 */
mutil_xfree(vom, file, lineno)
vomit *vom;
string file;
int lineno;
{
    Hist h;

    VPTR cp = (VPTR) vom;
#if defined(DARWIN)
	char *end = (char *) get_end();
#else
	extern char end[];
#endif
    extern int debug;

#ifndef INTERNAL_MALLOC
    extern VPTR stackbottom;

    if (cp >= stackbottom)
	return;
#endif	/* INTERNAL_MALLOC */

    if (!cp || cp < end || cp >= (VPTR) &vom)
	return;
    mutil_free(vom, file, lineno);
}

mutil_free_elems(vom, file, lineno)
vomit **vom;
string file;
int lineno;
{
    register int n;

    if (!vom)
	return;
    for (n = 0; vom[n]; n++)
	mutil_xfree(vom[n], file, lineno);
}

VPTR
mutil_realloc(p, n, file, lineno)
VPTR p;
int n;
string file;
int lineno;
{
    VPTR newp;
    int i, old_n = ((vomit *) p)[-1].numbytes;

    newp = mutil_malloc(n, file, lineno);
    for (i = 0; i < n && i < old_n; ++i)
	newp[i] = p[i];
    mutil_free(p, file, lineno);

    return newp;
}

VPTR
mutil_calloc(n, s, file, lineno)
int n, s;
string file;
int lineno;
{
    int i;
    VPTR p = mutil_malloc(n * s, file, lineno);

    n *= s;
    if (p)
	for (i = 0; i < n; ++i)
	    ((char *)p)[i] = 0;
    return p;
}

#else	/* !MALLOC_UTIL */

VPTR
realloc(p, n)
VPTR p;
unsigned int n;
{
    VPTR newp;
    int i, old_n;

    if (!env_malloc_trace)
	return real_realloc(p, n);

    old_n = ((vomit *) p)[-1].numbytes;
    newp = malloc(n);
    for (i = 0; i < n && i < old_n; ++i)
	newp[i] = p[i];
    free(p);

    return newp;
}

VPTR
calloc(n, s)
unsigned int n, s;
{
    VPTR p;

    if (!(p = malloc(n * s)))
	return 0;
    bzero(p, n * s);
    return p;
}

int 
cfree(p)
VPTR p;
{
#ifdef HAVE_VOID_FREE
    free(p);
    return 0;
#else /* HAVE_VOID_FREE */
    return free(p);
#endif /* HAVE_VOID_FREE */
}

#endif	/* !MALLOC_UTIL */

static void
multiqsort(base, n, elsiz, cmps)
VPTR base;
int n;
int elsiz;
int (*cmps[]) ();	/* null-terminated array of comparison functions */
{
    int i, (*cmp) ();

    if (!(cmp = cmps[0]))
	return;

    qsort(base, n, elsiz, cmp);

    if (cmps[1])
	for (; n > 0; n -= i, base += i * elsiz) {
	    for (i = 0; i < n; ++i)
		if ((*cmp) (base, base + i * elsiz))
		    break;
	    if (i > 1)
		multiqsort(base, i, elsiz, cmps + 1);
	}
}

static 
cmp_diffbytes(a, b)
Hist *a, *b;
{
    return ((*a)->bytes - (*a)->fbytes) - ((*b)->bytes - (*b)->fbytes);
}

static 
cmp_malloc_then_free(a, b)
Hist *a, *b;
{
    return ((*a)->type == mutil_FREE) - ((*b)->type == mutil_FREE);
}

#ifdef MALLOC_UTIL
static 
cmp_lineno(a, b)
Hist *a, *b;
{
    return (*a)->lineno - (*b)->lineno;
}

static 
cmp_filename(a, b)
Hist *a, *b;
{
    return strcmp((*a)->file, (*b)->file);
}

int (*cmps[]) () = {
    cmp_filename,
    cmp_lineno,
    NULL
};

#else	/* !MALLOC_UTIL */

static 
cmp(a, b)
Hist *a, *b;
{
    int i;

    for (i = 0; i < malloc_trace_depth; ++i)
	if ((*a)->trace[i] != (*b)->trace[i])
	    return (*a)->trace[i] - (*b)->trace[i];
    return 0;
}

int (*cmps[]) () = {
    cmp,
    NULL
};

#endif	/* !MALLOC_UTIL */

#ifdef MALLOC_UTIL
void
mutil_info1(verbose, file, lineno)
int verbose;
string file;
int lineno;
{
    string files[2];

    if (file == NULL) {
	mutil_info(verbose, NULL, lineno);
	return;
    }
    files[0] = file;
    files[1] = NULL;

    mutil_info(verbose, files, lineno);
}

void
mutil_info(verbose, files, lineno)
int verbose;
string files[];
int lineno;
{
    int i;
    Hist ptrs[MAXMALLOCS], h;

#else	/* !MALLOC_UTIL */

struct symbolic_trace {
    char *funs[MAX_MALLOC_TRACE_DEPTH];
    char *files[MAX_MALLOC_TRACE_DEPTH];
    int lines[MAX_MALLOC_TRACE_DEPTH];
};

void
malloc_trace_info(verbose, show_by_default, argv)
int verbose, show_by_default;
char *argv[];
{
    int i, j, is_in_list;
    Hist ptrs[MAXMALLOCS], h;
    char *p;
    char *tracefuns[MAX_MALLOC_TRACE_DEPTH];
    char *tracefiles[MAX_MALLOC_TRACE_DEPTH];
    int tracelines[MAX_MALLOC_TRACE_DEPTH];
    char buf[MAXMALLOCS * 80], *bufp = buf, *bufend = buf + sizeof(buf);

#endif	/* !MALLOC_UTIL */
    /*
     * Allowing our idea of num_hist to change during this function causes a
     * bus error...
     */
    int local_num_hist = num_hist;


    printf("\n");
    printf("%6s %10s %10s %10s %10s %10s\n",
	    " ",
	    "Mallocs",
	    "Frees",
	    "Diff",
	    "Mem Used",
	    "Max Used");
    printf("-------------------------------------------------------------\n");
    printf("%6s %10d %10d %10d %10d %10d\n",
	    "Bytes:",
	    numb_malloc,
	    numb_free,
	    numb_malloc - numb_free,
	    numb_malloc_really - numb_free_really,
	    max_diff_bytes);
    printf("%6s %10d %10d %10d\n",
	    "Calls:",
	    num_malloc,
	    num_free,
	    num_malloc - num_free);

    if (!verbose)
	return;

    printf("\n");

    printf("%17s[%4s]: %3s %6s %8s %7s %8s %7s %9s\n",
	    "Filename",
	    "Line",
	    "Wha",
	    "Calls",
	    "Bytes",
	    "FCalls",
	    "FBytes",
	    "Diff",
	    "DiffBytes");
    printf("-------------------------------------------------------------------------------\n");

    for (i = 0; i < local_num_hist; ++i)
	ptrs[i] = HistList + i;

    multiqsort(ptrs, local_num_hist, sizeof(Hist), cmps);

    for (i = 0; i < local_num_hist; i++) {

	h = ptrs[i];

#ifdef MALLOC_UTIL
	if (lineno && h->lineno != lineno)
	    continue;
	if (files) {
	    int j;

	    for (j = 0; files[j]; j++)
		if (strcmp(files[j], h->file) == 0)
		    break;

	    if (!files[j])
		continue;
	}
#endif	/* MALLOC_UTIL */

	if (h->calls == 0 && (h->type == mutil_FREE || h->fcalls == 0))
	    continue;

#ifdef MALLOC_UTIL
	printf("%17s[%4d]", h->file, h->lineno);
#else	/* !MALLOC_UTIL */
	bufp = symbolize_stacktrace(malloc_trace_depth,
		h->trace,
		tracefuns,
		tracefiles,
		tracelines,
		bufp, bufend);
	if (!bufp)
	    break;
	is_in_list = 0;
	if (argv) {
	    for (j = 0; argv[j]; j++) {
		if (tracefiles[0] && strcmp(argv[j], tracefiles[0]) == 0 ||
		    tracefuns[0] && strcmp(argv[j], tracefuns[0] + 1) == 0) {
		    is_in_list = 1;
		    break;
		}
	    }
	}
	if (is_in_list == show_by_default)
	    continue;


	for (j = malloc_trace_depth - 1; j >= 0; --j) {
	    /*
	     * (functions are more interesting than files anyway)
	     */
	    if (0 && tracefiles[j] && *tracefiles[j])
		printf("%17.17s[%4d]",
			p = rindex(tracefiles[j], '/') ? p + 1 : tracefiles[j],
			tracelines[j]);
	    else {
		if (tracefuns[j] && *tracefuns[j])
		    printf("%13.13s()", tracefuns[j] + 1);
		else
		    printf("%13.13s()", "???");
		if (tracelines[j])
		    printf("[%6d]", tracelines[j]);
		else
		    printf("[%06x]", h->trace[j]);
	    }
	    if (j > 0)
		printf("\n");
	}
#endif	/* !MALLOC_UTIL */

	if (h->type == mutil_MALLOC)
	    printf(": %3s %6d %8d %7d %8d %7d %9d\n",
		    "mal",
		    h->calls,
		    h->bytes,
		    h->fcalls,
		    h->fbytes,
		    h->calls - h->fcalls,
		    h->bytes - h->fbytes);
	else
	    printf(": %3s %6d %8d\n",
		    "fre",
		    h->calls,
		    h->bytes);
    }
    cleanup_symbolize_stacktrace();
}

#endif	/* MALLOC_TRACE || MALLOC_UTIL */
