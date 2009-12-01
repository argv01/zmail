/* FilePrefaceBegin:C:1.0
|============================================================================
| (C) Copyright 1988,1989 Island Graphics Corporation.  All Rights Reserved.
| 
| Title: memtype.h
| 
| Abstract: 
|============================================================================
| FilePrefaceEnd */

/* $Revision: 1.1 $ */

#ifndef _MEMTYPE_H
#define _MEMTYPE_H

#define NUM_MEM_TYPES		2

#define NUM_PRIO_DIVISIONS	10

typedef struct mem_queue
{
    MEM_LOC	head, tail;
} MEM_QUEUE;
typedef MEM_QUEUE	far *LPMEM_QUEUE;

typedef struct mem_header
{
    MEM_BOOL	(far *uninit_func) ();
    MEM_BOOL	(far *clear_func) ();
    MEM_BOOL	(far *move_func) ();
    MEM_LOC	(far *alloc_func) ();
    MEM_BOOL	(far *free_func) ();
    MEM_SIZE	(far *compact_func) ();
    MEM_BOOL	(far *store_func) ();
    MEM_BOOL	(far *restore_func) ();
    MEM_BOOL	(far *swap_func) ();
    MEM_SIZE	(far *biggest_block_func) ();
    MEM_BOOL	(far *less_core_func) ();
    MEM_FLAGS	flags;

    MEM_SIZE	limit, biggest_block, block_size;
    MEM_HANDLE	descriptor;
    MEM_LOC	allocp;
    MEM_SIZE	max_core;
    MEM_QUEUE	prio_tab[NUM_PRIO_DIVISIONS];
} MEM_HEADER;
typedef MEM_HEADER	far *LPMEM_HEADER;

extern LPMEM_HEADER	*_mem_tab, _lpCurHeader;

Header(extern MEM_PRIORITY _mem_type_init, (MEM_SIZE, LPMEM));
Header(extern void _mem_type_uninit, (void));
Header(extern MEM_LOC _mem_type_alloc, (MEM_SIZE, MEM_FLAGS,
	MEM_PRIORITY, MEM_PRIORITY, MEM_PRIORITY, LPMEM_PRIORITY));
Header(extern void _mem_type_free, (MEM_LOC, MEM_SIZE, MEM_PRIORITY));
Header(extern MEM_SIZE _mem_type_compact, (MEM_SIZE, int));
Header(extern MEM_BOOL _mem_type_move, (MEM_LOC, MEM_SIZE, MEM_PRIORITY,
	MEM_LOC, MEM_SIZE, MEM_PRIORITY, MEM_SIZE));
Header(extern MEM_BOOL _mem_type_swap, (MEM_SIZE, MEM_PRIORITY, MEM_PRIORITY));
Header(extern MEM_SIZE _mem_type_get_memavailable,
	(MEM_SIZE, MEM_PRIORITY, MEM_PRIORITY));

#endif /* _MEMTYPE_H */
