/* Based on RootWindoP.h, which is: */
/*
 * Copyright 1990 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  M.I.T. makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 *
 * M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL M.I.T.
 * BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#ifndef _WinWidP_h
#define _WinWidP_h

#include "WinWid.h"
/* include superclass private header file */
#include <X11/CoreP.h>

typedef struct {
    int empty;
} WinWidClassPart;

typedef struct _WinWidClassRec {
    CoreClassPart	core_class;
    WinWidClassPart	winwid_class;
} WinWidClassRec;

extern WinWidClassRec winWidClassRec;

typedef struct {
    /* resources */
    char* resource;
    /* private state */
} WinWidPart;

typedef struct _WinWidRec {
    CorePart	core;
    WinWidPart	winwid;
} WinWidRec;

#endif /* _WinWidP_h */
