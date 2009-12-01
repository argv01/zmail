/***
*   Copied directly from setjmp.h in Microsoft Visual C++.
*   Modified to allow setjmp to work with C++. wsw 12/29/94
*
*setjmp.h - definitions/declarations for setjmp/longjmp routines
*
*   Copyright (c) 1985-1993, Microsoft Corporation. All rights reserved.
*
*Purpose:
*   This file defines the machine-dependent buffer used by
*   setjmp/longjmp to save and restore the program state, and
*   declarations for those routines.
*   [ANSI/System V]
*
****/

#ifndef _INC_SETJMP

#ifdef __cplusplus
extern "C" {
#endif

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

/* define the buffer type for holding the state information */

#define _JBLEN  9  /* bp, di, si, sp, ret addr, ds */

#ifndef _JMP_BUF_DEFINED
typedef  int  jmp_buf[_JBLEN];
#define _JMP_BUF_DEFINED
#endif 

/* ANSI requires setjmp be a macro */

#define setjmp  _setjmp

/* function prototypes */

int  __cdecl _setjmp(jmp_buf);
void __cdecl longjmp(jmp_buf, int);

#ifdef __cplusplus
}
#endif 

#define _INC_SETJMP
#endif 
