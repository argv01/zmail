#ifndef _UIVSRCH_H_
#define _UIVSRCH_H_

/*
 * $RCSfile: uivsrch.h,v $
 * $Revision: 1.3 $
 * $Date: 1995/10/05 05:28:05 $
 * $Author: liblit $
 */

#include "uivars.h"
#include "zmflag.h"

struct _uivarsearch {
    char *search_str;
    int offset, varno, first_var, end_offset;
    uivarlist_t *vl;
    long flags;
    char *desc;
};
typedef struct _uivarsearch uivarsearch_t;

#define uivarsearch_XREF ULBIT(0)

#define uivarsearch_GetOffset(X) ((X)->offset)
#define uivarsearch_GetEndOffset(X) ((X)->end_offset)
#define uivarsearch_GetVarNum(X) ((X)->varno)
#define uivarsearch_IsXref(X)	 (ison((X)->flags, uivarsearch_XREF))

#define uivarsearch_NO_MATCHES catgets(catalog, CAT_UISUPP, 5, "No matches found for \"%s\".")
#define uivarsearch_ENTER_STR  catgets(catalog, CAT_UISUPP, 6, "Please enter a search string.")

extern void uivarsearch_SetXref P ((uivarsearch_t *));
extern void uivarsearch_EndSearch P ((uivarsearch_t *));
extern void uivarsearch_ReportNoMatch P ((uivarsearch_t *));
extern zmBool uivarsearch_Search P ((uivarsearch_t *, const char *, int, int));
extern void uivarsearch_Destroy P ((uivarsearch_t *));
extern void uivarsearch_Init P ((uivarsearch_t *, uivarlist_t *));
#ifdef MSDOS
extern int uivarsearch_AdjustOffset P ((uivarsearch_t *, int));
#endif /* MSDOS */

#endif /* _UIVSRCH_H_ */
