/* FilePrefaceBegin:C:1.0
|============================================================================
| (C) Copyright 1988,1989 Island Graphics Corporation.  All Rights Reserved.
| 
| Title: memlib.h
| 
| Abstract: External declarations for memory library
|============================================================================
| FilePrefaceEnd */

/* $Revision: 1.1 $ */

#ifndef _MEMLIB_H
#define _MEMLIB_H

#include "compiler.h"
#include "memerr.h"

#define MEM_NULL	0L

typedef unsigned long	MEM_SIZE;
typedef long		MEM_OFFS;

typedef char		huge *LPMEM;
typedef LPMEM		far *LPMEMPTR;
typedef LPMEMPTR	MEM_HANDLE;

typedef enum
{
    MEM_FAILURE = 0,
    MEM_SUCCESS
} MEM_BOOL;

typedef enum
{
    SAME_MEM_USAGE,
    MEM_SIMPLE,
    MEM_COMPLEX
} MEM_USAGE;

typedef enum
{
    MEM_NO_ACCESS = 0,
    MEM_READ_ONLY,
    MEM_WRITE_ONLY,
    MEM_READ_WRITE
} MEM_ACCESS_MODE;

typedef enum
{
    MEM_SEEK_REL,
    MEM_SEEK_ABS
} MEM_SEEK_MODE;

typedef enum
{
    MEM_FORWARD,
    MEM_BACKWARD
} MEM_DIRECTION;

typedef struct mem_size_request
{
    MEM_SIZE		min_size, size, max_size;
    MEM_DIRECTION	direction;
} MEM_SIZE_REQUEST;

typedef MEM_SIZE_REQUEST	far *LPMEM_SIZE_REQUEST;
typedef short			MEM_PRIORITY;
typedef unsigned short		MEM_FLAGS;

#define DEFAULT_MEM_FLAGS	(MEM_FLAGS)0
#define SAME_MEM_FLAGS		(MEM_FLAGS)0
#define MEM_FIXED		(MEM_FLAGS)(1 << 0)
#define MEM_MOVEABLE		(MEM_FLAGS)(1 << 1)
#define MEM_CONSTRAINED		(MEM_FLAGS)(1 << 2)
#define MEM_DISCARDABLE		(MEM_FLAGS)(1 << 3)
#define MEM_NODISCARD		(MEM_FLAGS)(1 << 4)
#define MEM_DISCARDED		(MEM_FLAGS)(1 << 5)
#define MEM_ACCESSED		(MEM_FLAGS)(1 << 6)
#define MEM_STATIC		(MEM_FLAGS)(1 << 7)
#define MEM_PRIMARY		(MEM_FLAGS)(1 << 8)
#define MEM_ZEROINIT		(MEM_FLAGS)(1 << 9)

#define DEFAULT_MEM_PRIORITY	(MEM_PRIORITY)0
#define SAME_MEM_PRIORITY	(MEM_PRIORITY)0
#define HIGHEST_MEM_PRIORITY	(MEM_PRIORITY)1
#define LOWEST_MEM_PRIORITY	(MEM_PRIORITY)100

#define mem_request(size)	\
	mem_allocate((size), DEFAULT_MEM_FLAGS, \
	    DEFAULT_MEM_PRIORITY, MEM_SIMPLE)
#define mem_resize(h, size)	\
	mem_reallocate(h, size, SAME_MEM_FLAGS, \
		SAME_MEM_PRIORITY, SAME_MEM_USAGE)

#define mem_lock(h)	mem_simple_access(h)
#define mem_unlock(h)	mem_simple_unaccess(h)

/* in access.c */
Header(extern LPMEM mem_simple_access, (MEM_HANDLE));
Header(extern MEM_BOOL mem_simple_unaccess, (MEM_HANDLE));
Header(extern LPMEM mem_complex_access,
	(MEM_HANDLE, MEM_SIZE, LPMEM_SIZE_REQUEST, MEM_ACCESS_MODE));
Header(extern MEM_BOOL mem_complex_unaccess, (MEM_HANDLE));
Header(extern LPMEM mem_complex_seek,
	(MEM_HANDLE, MEM_OFFS, LPMEM_SIZE_REQUEST, MEM_SEEK_MODE));


/* in alloc.c */
Header(extern MEM_HANDLE mem_allocate,
	(MEM_SIZE, MEM_FLAGS, MEM_PRIORITY, MEM_USAGE));
Header(extern MEM_BOOL mem_release, (MEM_HANDLE));
Header(extern MEM_HANDLE mem_reallocate,
	(MEM_HANDLE, MEM_SIZE, MEM_FLAGS, MEM_PRIORITY, MEM_USAGE));
Header(extern MEM_HANDLE mem_static, (LPMEM, MEM_SIZE, MEM_USAGE));
Header(extern MEM_HANDLE mem_dup, (MEM_HANDLE, MEM_USAGE));


/* in datainfo.c */
Header(extern MEM_SIZE mem_get_size, (MEM_HANDLE));
Header(extern MEM_USAGE mem_get_usage, (MEM_HANDLE));
Header(extern MEM_BOOL mem_set_flags, (MEM_HANDLE, MEM_FLAGS));
Header(extern MEM_FLAGS mem_get_flags, (MEM_HANDLE));
Header(extern MEM_BOOL mem_set_priority, (MEM_HANDLE, MEM_PRIORITY));
Header(extern MEM_PRIORITY mem_get_priority, (MEM_HANDLE));
Header(extern MEM_SIZE mem_get_memavailable, (MEM_SIZE, MEM_PRIORITY));

/* in init.c */
Header(extern MEM_BOOL mem_init, (MEM_SIZE, LPMEM));
Header(extern MEM_BOOL mem_uninit, (void));

/* in memerr.c */
Header(extern MEM_ERROR mem_get_error, (void));

#endif /* _MEMLIB_H */
