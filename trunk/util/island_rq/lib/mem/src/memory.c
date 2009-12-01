/* FilePrefaceBegin:C:1.0
|============================================================================
| (C) Copyright 1988,1989 Island Graphics Corporation.  All Rights Reserved.
| 
| Title: memlib.h
| 
| Abstract: Simple memory handle mechanism
|============================================================================
| FilePrefaceEnd */

/* $Revision: 1.1 $ */

#include "memlib.h"
#include "memerr.h"
#include "memintrn.h"

/* #define TRACE */

#ifdef TRACE
#include <stdio.h>
#endif

/*
 * New functions to abstract a layer of error control 
 */

typedef char * (* PFC) ();
typedef int (* PFI) ();

static PFC IslandMallocFunc = MEM_NULL;
static PFC IslandReallocFunc = MEM_NULL;
static PFI IslandFreeFunc = MEM_NULL;

void
IslandRegisterMallocFunc (f)
PFC f;
{
	IslandMallocFunc = f;
}

void
IslandRegisterReallocFunc (f)
PFC f;
{
	IslandReallocFunc = f;
}

void
IslandRegisterFreeFunc (f)
PFI f;
{
	IslandFreeFunc = f;
}

char *
IslandMalloc (size)
int	size;
{
    extern char * malloc ();

    if (IslandMallocFunc)
	return (IslandMallocFunc (size));
    else
	return (malloc (size));
}

char *
IslandCalloc (nitems, size)
int	nitems;
int	size;
{
    extern char * calloc ();

    if (IslandMallocFunc)
    {
	char *m;
	int	s = nitems * size;

	m = IslandMallocFunc (s);
	bzero (m, s);	/* make sure that it's zeroed */
	return (m);
    }
    else
	return (calloc (nitems, size));
}

char *
IslandRealloc (p, size)
char	*p;
int	size;
{
    extern char * realloc ();

    if (!p)
	return (IslandMalloc (size));
 
    if (IslandReallocFunc)
	return (IslandReallocFunc (p, size));
    else
	return (realloc (p, size));
}

IslandFree (p)
char *p;
{
    if (p)
    {
	if (IslandFreeFunc)
	    IslandFreeFunc (p);
	else
	    free (p);
    }
}

/* in access.c */
LPMEM
mem_simple_access (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_simple_access: with handle <0x%x>\n", h);
#endif /* TRACE */
    return ((LPMEM) h);
}

MEM_BOOL
mem_simple_unaccess (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_simple_unaccess: with handle <0x%x>\n", h);
#endif /* TRACE */
    return (MEM_SUCCESS);
}

LPMEM
mem_complex_access (h, s, size, mode)
MEM_HANDLE		h;
MEM_SIZE		s;
LPMEM_SIZE_REQUEST	size;
MEM_ACCESS_MODE		mode;
{
    size->size = (MEM_SIZE) *((unsigned long *) h - 1);
#ifdef TRACE
    fprintf (stderr,
	"mem_complex_access: handle <0x%x> offset %d request %d mode %d return size = %d\n",
			h, s, size, mode, size->size);
#endif /* TRACE */
    return ((LPMEM) ((char *) h + s));	/* add in offset */
}

MEM_BOOL
mem_complex_unaccess (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_complex_unaccess: with handle <0x%x>\n", h);
#endif /* TRACE */
    return (MEM_SUCCESS);
}

LPMEM
mem_complex_seek (h, o, size, mode)
MEM_HANDLE		h;
MEM_OFFS		o;
LPMEM_SIZE_REQUEST	size;
MEM_SEEK_MODE		mode;
{
#ifdef TRACE
    fprintf (stderr, "mem_complex_seek: with handle <0x%x>\n", h);
#endif /* TRACE */
    if (mode == MEM_SEEK_REL)
	return ((LPMEM) MEM_NULL);
    else
	return ((LPMEM) (char *)h + o);
}

MEM_HANDLE
mem_allocate (size, flags, pri, usage)
MEM_SIZE	size;
MEM_FLAGS	flags;
MEM_PRIORITY	pri;
MEM_USAGE	usage;
{
    unsigned long	*p;

    if (!(flags & MEM_ZEROINIT))
	p = (unsigned long *) IslandMalloc (size + sizeof (unsigned long));
    else
	p = (unsigned long *) IslandCalloc (1, size + sizeof (unsigned long));

    if (p)
	*p++ = size;		/* save away the size */

#ifdef TRACE
    fprintf (stderr, "mem_allocate: size %d return handle <0x%x>\n", size, p);
#endif /* TRACE */
    return ((MEM_HANDLE) p);
}

MEM_BOOL
mem_release (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_release: with handle <0x%x>\n", h);
#endif /* TRACE */
    IslandFree((char *)h - sizeof (unsigned long));

    return (MEM_SUCCESS);
}

MEM_HANDLE
mem_reallocate (h, size, flags, pri, usage)
MEM_HANDLE	h;
MEM_SIZE	size;
MEM_FLAGS	flags;
MEM_PRIORITY	pri;
MEM_USAGE	usage;
{
    unsigned long	*p;
    unsigned long	old_size;

/*
    if  (size == *((unsigned long *) ((char *)h - sizeof(unsigned long))))
	return;
*/

    old_size = *((unsigned long *)h - 1);
    p = (unsigned long *) realloc ((char *)h - sizeof (unsigned long), size + sizeof (unsigned long));
    if (p)
    {
	*p++ = size;
	if ((flags & MEM_ZEROINIT) && (size > old_size))
	    bzero ((char *)p + old_size, size - old_size);
    }
#ifdef TRACE
    fprintf (stderr, "mem_reallocate: with handle <0x%x> size %d\n", h, size);
#endif /* TRACE */
    return ((MEM_HANDLE) p);
}

MEM_HANDLE
mem_static (m, size, usage)
LPMEM		m;
MEM_SIZE	size;
MEM_USAGE	usage;
{
    unsigned long	*p;

    p = (unsigned long *) IslandMalloc (size + sizeof (unsigned long));
    if (p)
    {
	*p++ = size;
	bcopy((char *)m, (char *)p, size);
    }
#ifdef TRACE
    fprintf (stderr, "mem_static: with memory <0x%x> size %d\n", m, size);
#endif /* TRACE */
    return ((MEM_HANDLE) p);
}

MEM_HANDLE
mem_dup (h, u)
MEM_HANDLE	h;
MEM_USAGE	u;
{
    int	size;
    unsigned long	*d, *s;

    size = mem_get_size (h);

    d = (unsigned long *) IslandMalloc (size + sizeof (unsigned long));
    if (d)
    {
	*d++ = size;
	bcopy((char *)h, (char *)d , size);
    }

#ifdef TRACE
    fprintf (stderr, "mem_dup: with handle <0x%x> return <0x%x>\n", h, d);
#endif /* TRACE */

    return ((MEM_HANDLE) d);
}

MEM_SIZE
mem_get_size (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_get_size: with handle <0x%x> return size %d\n",
			h, (MEM_SIZE) *((unsigned long *) ((char *)h - sizeof(unsigned long))));
#endif /* TRACE */
    return ((MEM_SIZE) *((unsigned long *) ((char *)h - sizeof(unsigned long))));
}

MEM_USAGE
mem_get_usage (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_get_usage: with handle <0x%x>\n", h);
#endif /* TRACE */
    return ((MEM_USAGE) MEM_SIMPLE);
}

MEM_BOOL
mem_set_flags (h, f)
MEM_HANDLE	h;
MEM_FLAGS	f;
{
#ifdef TRACE
    fprintf (stderr, "mem_set_flags: with handle <0x%x>\n", h);
#endif /* TRACE */
    return ((MEM_BOOL) MEM_FAILURE);
}

MEM_FLAGS
mem_get_flags (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_get_flags: with handle <0x%x>\n", h);
#endif /* TRACE */
    return ((MEM_FLAGS) DEFAULT_MEM_FLAGS);
}

MEM_BOOL
mem_set_priority (h, p)
MEM_HANDLE	h;
MEM_PRIORITY	p;
{
#ifdef TRACE
    fprintf (stderr, "mem_set_priority: with handle <0x%x>\n", h);
#endif /* TRACE */
    return ((MEM_BOOL) MEM_FAILURE);
}

MEM_PRIORITY
mem_get_priority (h)
MEM_HANDLE	h;
{
#ifdef TRACE
    fprintf (stderr, "mem_get_priority: with handle <0x%x>\n", h);
#endif /* TRACE */
    return ((MEM_PRIORITY) DEFAULT_MEM_PRIORITY);
}

MEM_SIZE
mem_get_memavailable (s, p)
MEM_SIZE	s;
MEM_PRIORITY	p;
{
#ifdef TRACE
    fprintf (stderr, "mem_get_memavailable: with size <%d>\n", s);
#endif /* TRACE */
    return ((MEM_SIZE) 0x0fffffff);
}

MEM_BOOL
mem_init (s, m)
MEM_SIZE	s;
LPMEM		m;
{
#ifdef TRACE
    fprintf (stderr, "mem_init\n");
#endif /* TRACE */
    return (MEM_SUCCESS);
}

MEM_BOOL
mem_uninit ()
{
#ifdef TRACE
    fprintf (stderr, "mem_uninit\n");
#endif /* TRACE */
    return (MEM_SUCCESS);
}

MEM_ERROR
mem_get_error ()
{
#ifdef TRACE
    fprintf (stderr, "mem_get_error\n");
#endif /* TRACE */
    return ((MEM_ERROR) MEM_NO_ERROR);
}

/* stubs for compatibility with debug library */
mem_report () {}
mem_set_feedback () {}
mem_stat_reset () {}
mem_set_feedback_proc () {}

