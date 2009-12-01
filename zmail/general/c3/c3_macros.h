#ifndef __macros_include__
#define __macros_include__
#include "c3_string.h"

#ifdef THINK_C

#include <Memory.h>

#define MEMALLOC(bytes_count) malloc((size_t) bytes_count)

#endif

#ifdef UNIX

#define MEMALLOC(bytes_count) malloc((size_t) bytes_count)
#define MEMREALLOC(point, bytes_count) realloc(point, (size_t) bytes_count)

#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef MAX
#define MAX(A,B) (A) > (B) ? (A) : (B)
#endif

#ifndef BYTEMASK
#define BYTEMASK(ch) ((ch) & 0xff)
#endif

#define SAFE_ALLOCATE(POINT, PTYPE, AMOUNT, ERRSTEP, FUNCNAME) \
    do { \
	POINT = PTYPE MEMALLOC(AMOUNT); \
	if (POINT == NULL) \
	{ \
    	    _c3_register_outcome(FUNCNAME, C3E_NOMEM); \
		ERRSTEP; \
	} \
    } while(0) 

#define SAFE_ZALLOCATE(POINT, PTYPE, AMOUNT, ERRSTEP, FUNCNAME) \
    do { \
	POINT = PTYPE MEMALLOC(AMOUNT); \
	if (POINT == NULL) \
	{ \
	    _c3_register_outcome(FUNCNAME, C3E_NOMEM); \
		ERRSTEP; \
	} \
	_c3_zero_memory(POINT, (size_t) AMOUNT); \
    } while(0) 

#define SAFE_REALLOCATE(POINT, PTYPE, AMOUNT, ERRSTEP, FUNCNAME) \
    POINT = PTYPE MEMREALLOC(POINT, AMOUNT); \
    if (POINT == NULL) \
    { \
        _c3_register_outcome(FUNCNAME, C3E_NOMEM); \
	ERRSTEP; \
    }

#define VERB_NONE  0
#define VERB_MINI  1
#define VERB_NORM  2
#define VERB_DEB1  3
#define VERB_DEB2  4
#define VERB_DEB3  5


#define C3VERBOSE(level, format, id, value) \
    if (opt_verbosity >= (level)) \
    { \
	 fprintf(stderr, "%s: ", (id)); \
	 fprintf(stderr, format, (value)); \
	 fprintf(stderr, "\n"); \
    }

#endif /* !__macros_include__*/
