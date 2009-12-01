/* memextrn.h */

#include <island/memlib.h>

#define		LAST_OBJ_INDEX		8

#define		KEY_ALLOCATED		0
#define		KEY_IN_MEMORY		(MEM_FLAGS)MEM_PRIMARY
#define		KEY_ABOVE_BOARD		0
#define		KEY_ON_TEMPFILE		0
#define		KEY_ON_PERMFILE		0
#define		KEY_DISCARDABLE		(MEM_FLAGS)MEM_DISCARDABLE
#define		KEY_DISCARDED		(MEM_FLAGS)MEM_DISCARDED

#define KEY_SOMEWHERE (KEY_IN_MEMORY|KEY_ABOVE_BOARD|KEY_ON_TEMPFILE|KEY_ON_PERMFILE)

#define GMEM_ANY            0x0001

#ifndef MEMORY_H
#define MEMORY_H

/* Interface to global memory manager */
#define GMEM_FIXED          (MEM_FLAGS)MEM_FIXED
#define GMEM_MOVEABLE       (MEM_FLAGS)MEM_MOVEABLE
#define GMEM_NOCOMPACT      (MEM_FLAGS)0x0000
#define GMEM_NODISCARD      (MEM_FLAGS)MEM_NODISCARD
#define GMEM_ZEROINIT       (MEM_FLAGS)MEM_ZEROINIT
#define GMEM_MODIFY         (MEM_FLAGS)0x0000
#define GMEM_DISCARDABLE    (MEM_FLAGS)MEM_DISCARDABLE
#define GHND    	    (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR    	    (GMEM_FIXED    | GMEM_ZEROINIT)
#define GLOBAL_HANDLE unsigned long

#define GlobalAlloc(flags,size)	(GLOBAL_HANDLE)mem_allocate ((MEM_SIZE)(size), \
				    flags, DEFAULT_MEM_PRIORITY, MEM_SIMPLE)
#define GlobalFree(h)		mem_release ((MEM_HANDLE)(h))
#define GlobalData(data,size)	(GLOBAL_HANDLE)mem_static ((LPMEM)(data), \
				    (MEM_SIZE)(size), MEM_SIMPLE)
#define GlobalReAlloc(h,size,flags)	(GLOBAL_HANDLE)mem_reallocate((MEM_HANDLE)(h),\
				    (MEM_SIZE)size, flags, SAME_MEM_PRIORITY, \
				    SAME_MEM_USAGE)
#define GlobalSize(h)		mem_get_size ((MEM_HANDLE)(h))
#define GlobalFlags(h)		mem_get_flags ((MEM_HANDLE)(h))
#define GlobalLock(h)		(LPSTR)mem_lock ((MEM_HANDLE)(h))
#define GlobalUnlock(h)		mem_unlock ((MEM_HANDLE)(h)) ? TRUE : FALSE
#define GlobalDiscard(h)	GlobalReAlloc (h,0L,GMEM_MOVEABLE)

#define KeywordAlloc(flags,size,type)	GlobalAlloc(flags,size)
#define KeywordFree(h)			GlobalFree(h)
#define KeywordLock(h)			GlobalLock(h)
#define KeywordUnlock(h)		GlobalUnlock(h)
#define KeywordReAlloc(h,size,flags)	GlobalReAlloc(h,size,flags)
#define KeywordDiscard(h)		GlobalDiscard(h)
#define KeywordSize(h)			GlobalSize(h)
#define KeywordMemFlags(h)		KEY_IN_MEMORY
#define KeywordKeyFlags(h)		KEY_IN_MEMORY

#define LoadResource(h)			(h)
#define FreeResource(h)
#define LockResource(h)			GlobalLock(h)
#define UnlockResource(h)		GlobalUnlock(h)
#define SizeofResource(inst, h)		GlobalSize(h)

#define CloneStruct(h)		(GLOBAL_HANDLE)mem_resize (mem_dup ((MEM_HANDLE)(h), \
				    SAME_MEM_USAGE), \
				    mem_get_size ((MEM_HANDLE)(h)))
#define AllowGlobalFree(bytes) (DWORD)mem_get_memavailable((MEM_SIZE)bytes, \
			    DEFAULT_MEM_PRIORITY)

#define MemInit()	mem_init ((MEM_SIZE)0, MEM_NULL)
#define MemUninit()	mem_uninit ()

/* Flags returned by GlobalFlags (in addition to GMEM_DISCARDABLE) */
#define GMEM_DISCARDED      (MEM_FLAGS)MEM_DISCARDED
#define GMEM_SWAPPED        (MEM_FLAGS)0x0000
#define GMEM_LOCKCOUNT      (MEM_FLAGS)MEM_ACCESSED

#endif /* MEMORY_H */

