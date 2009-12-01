/* FilePrefaceBegin:C:1.0
|============================================================================
| (C) Copyright 1988,1989 Island Graphics Corporation.  All Rights Reserved.
| 
| Title: memintrn.h
| 
| Abstract: Internal declarations for memory library.
|============================================================================
| FilePrefaceEnd */

/* $Revision: 1.1 $ */

#ifndef _MEMINTRN_H
#define _MEMINTRN_H

#include "errlib.h"

#define MEM_DUP		MEM_PRIMARY	/* use non-allocation flag */
#define MEM_ALLOC_FLAGS (MEM_FIXED | MEM_MOVEABLE | MEM_CONSTRAINED | \
			    MEM_DISCARDABLE | MEM_NODISCARD | MEM_ZEROINIT)

typedef unsigned long	MEM_LOC;
typedef short		MEM_COUNT;
typedef MEM_LOC		far *LPMEM_LOC;
typedef MEM_SIZE	far *LPMEM_SIZE;
typedef MEM_FLAGS	far *LPMEM_FLAGS;
typedef MEM_PRIORITY	far *LPMEM_PRIORITY;

#include "memtype.h"

typedef struct mem_info
{
    MEM_SIZE		size;		/* in bytes */
    MEM_FLAGS		flags;
    MEM_PRIORITY	priority;
    MEM_COUNT		ref_count;	/* for mem_dup's */
    MEM_COUNT		access_count;	/* for mem_access'es */

    MEM_PRIORITY	where;		/* memory type where data is stored */
    MEM_LOC		virtual;	/* position in that type of memory */
    MEM_LOC		physical;
    
    MEM_LOC		pred_info, succ_info;
} MEM_INFO;
typedef MEM_INFO	far *LPMEM_INFO;

typedef MEM_USAGE	far *LPMEM_USAGE;

typedef struct simple_handle
{
    MEM_USAGE	usage;
    MEM_LOC	mem_info;
    MEM_LOC	physical;
    
    MEM_COUNT	access_count;
} SIMPLE_HANDLE;
 
typedef struct handle_desc
{
    MEM_USAGE		usage;
    MEM_LOC		mem_info;
    MEM_LOC		physical;/* pointer user gets to accessed memory */

    union
    {
    	MEM_COUNT	access_count;
    	struct
    	{
    	    MEM_LOC		primary;
	    MEM_SIZE		curpos, cursize;	/* position in memory */
	    MEM_DIRECTION	curdir;			/* current access direction */
	    MEM_ACCESS_MODE	access_mode;
	} window;
    } handle_info;
} HANDLE_DESC;
typedef HANDLE_DESC		COMPLEX_HANDLE;
typedef HANDLE_DESC		far *LPHANDLE_DESC;


#define _mem_get_memtype(p)	((p)==DEFAULT_MEM_PRIORITY ? \
					DEFAULT_MEM_PRIORITY : \
					(((p) - HIGHEST_MEM_PRIORITY) / \
					((LOWEST_MEM_PRIORITY - \
					HIGHEST_MEM_PRIORITY + 1) / \
					_mem_types + 1)))
#define _mem_get_last_memtype()	(_mem_types - 1)

/* in alloc.c */
Header(extern MEM_FLAGS _mem_get_alloc_flags, (MEM_FLAGS));
Header(extern MEM_PRIORITY _mem_get_alloc_priority, (MEM_PRIORITY));

/* in dataxfer.c */
Header(extern MEM_HANDLE _mem_alloc_handle,
	(MEM_SIZE, MEM_FLAGS, MEM_PRIORITY, MEM_LOC, MEM_USAGE));
Header(extern void _mem_free_handle, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_satisfy_request,
	(MEM_HANDLE, MEM_SIZE, LPMEM_SIZE_REQUEST, MEM_ACCESS_MODE));

/* in hash.c */
Header(extern void _mem_init_info,
	(MEM_LOC, MEM_LOC, MEM_SIZE, MEM_FLAGS, MEM_PRIORITY, MEM_PRIORITY));
Header(extern void _mem_hash_info, (MEM_LOC, MEM_PRIORITY, MEM_PRIORITY));
Header(extern void _mem_unhash_info, (MEM_LOC, MEM_PRIORITY, MEM_PRIORITY));
Header(extern void _mem_rehash_info, (MEM_LOC, MEM_PRIORITY, MEM_PRIORITY));
Header(extern MEM_BOOL _mem_hash_discard, (LPMEM_HEADER));
Header(extern void _mem_hash_update,
	(LPMEM_HEADER, MEM_LOC, MEM_LOC, MEM_SIZE));
Header(extern void _mem_hash_newloc,
	(MEM_LOC, MEM_LOC, MEM_SIZE, MEM_FLAGS,
	MEM_PRIORITY, MEM_PRIORITY, MEM_PRIORITY));
Header(extern MEM_BOOL _mem_hash_swap,
	(MEM_SIZE, MEM_PRIORITY, MEM_PRIORITY, MEM_PRIORITY,
	MEM_SIZE, void (*)()));

/* in hndlinfo.c */
Header(extern MEM_BOOL _mem_get_attrs,
	(MEM_HANDLE, LPMEM_LOC, LPMEM_SIZE, LPMEM_FLAGS,
	LPMEM_PRIORITY, LPMEM_PRIORITY));
Header(extern MEM_SIZE _mem_get_size, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_set_flags, (MEM_HANDLE, MEM_FLAGS));
Header(extern MEM_FLAGS _mem_get_flags, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_set_priority, (MEM_HANDLE, MEM_PRIORITY));
Header(extern MEM_PRIORITY _mem_get_priority, (MEM_HANDLE));
Header(extern MEM_USAGE _mem_get_usage, (MEM_HANDLE));
Header(extern MEM_LOC _mem_get_virtual_loc, (MEM_HANDLE));
Header(extern MEM_PRIORITY _mem_get_where, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_set_phys_size, (MEM_HANDLE, MEM_SIZE));
Header(extern MEM_SIZE _mem_get_phys_size, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_set_primary, (MEM_HANDLE, MEM_LOC, MEM_SIZE));
Header(extern MEM_LOC _mem_get_primary, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_set_phys_pos,
	(MEM_HANDLE, MEM_SIZE, MEM_DIRECTION));
Header(extern MEM_SIZE _mem_get_phys_pos, (MEM_HANDLE, MEM_BOOL));
Header(extern MEM_BOOL _mem_set_ref, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_clr_ref, (MEM_HANDLE));
Header(extern MEM_ACCESS_MODE _mem_get_access, (MEM_HANDLE));
Header(extern LPMEM _mem_set_access, (MEM_HANDLE, MEM_ACCESS_MODE));
Header(extern MEM_BOOL _mem_clr_access, (MEM_HANDLE));
Header(extern MEM_BOOL _mem_set_info, (MEM_HANDLE, MEM_LOC, MEM_USAGE));
Header(extern MEM_LOC _mem_get_info, (MEM_HANDLE, LPMEM_USAGE));

/* in memerr.c */
extern ERR_ID			_mem_err_id;
Header(extern MEM_BOOL _mem_err_init, (void));
Header(extern void _mem_err_fatal, (char *));

/* in memtype.c */
extern MEM_PRIORITY     _mem_types;

/* in disk.c */
Header(extern MEM_BOOL _mem_disk_init, (MEM_SIZE, LPMEM_HEADER *, LPMEM));
Header(extern MEM_BOOL _mem_disk_uninit, (void));
Header(extern MEM_BOOL _mem_disk_clear, (MEM_LOC, MEM_SIZE));
Header(extern MEM_BOOL _mem_disk_store, (LPMEM, MEM_LOC, MEM_SIZE));
Header(extern MEM_BOOL _mem_disk_restore, (MEM_LOC, LPMEM, MEM_SIZE));
Header(extern MEM_BOOL _mem_disk_move,
	(MEM_LOC, MEM_LOC, MEM_SIZE, MEM_SIZE, MEM_SIZE));
Header(extern MEM_BOOL _mem_disk_less_core, (void));

/* in primary.c */
Header(extern MEM_BOOL _mem_primary_init, (MEM_SIZE, LPMEM_HEADER *));
Header(extern MEM_BOOL _mem_primary_uninit, (void));
Header(extern MEM_BOOL _mem_primary_clear, (MEM_LOC, MEM_SIZE));
Header(extern MEM_BOOL _mem_primary_move,
	(MEM_LOC, MEM_LOC, MEM_SIZE, MEM_SIZE, MEM_SIZE));


#endif /* _MEMINTRN_H */
