#ifndef _UIPICK_H_
#define _UIPICK_H_

/*
 * $RCSfile: uipick.h,v $
 * $Revision: 1.8 $
 * $Date: 1994/06/16 19:47:19 $
 * $Author: pf $
 */

/* UI search/pick */

#include "uisupp.h"
#include "gptrlist.h"

#define uipick_FirstN		ULBIT(0)
#define uipick_LastN		ULBIT(1)

struct uipick {
    struct gptrlist patlist;
    zmFlags flags;
    int first, last;
};
typedef struct uipick uipick_t;
#define uipick_GetFlags(UP, F)  ((UP)->flags & (F))
#define uipick_GetFirstNCount(UP) ((UP)->first)
#define uipick_GetLastNCount(UP)  ((UP)->last)
#define uipick_SetFirstNCount(UP, N) ((UP)->first = (N))
#define uipick_SetLastNCount(UP, N)  ((UP)->last = (N))

#define uipickpat_Invert	ULBIT(0)
#define uipickpat_IgnoreCase	ULBIT(1)
#define uipickpat_ExpFixed	ULBIT(2)
#define uipickpat_ExpExtended	ULBIT(3)
#define uipickpat_DateRelative	ULBIT(4)
#define uipickpat_DateAbsolute	ULBIT(5)
#define uipickpat_SearchFrom	ULBIT(6)
#define uipickpat_SearchSubject	ULBIT(7)
#define uipickpat_SearchTo	ULBIT(8)
#define uipickpat_SearchHdr	ULBIT(9)
#define uipickpat_SearchBody	ULBIT(10)
#define uipickpat_SearchEntire	ULBIT(11)
#define uipickpat_DateBefore	ULBIT(12)
#define uipickpat_DateAfter	ULBIT(13)
#define uipickpat_DateOn	ULBIT(14)

enum uipickpat_date_units_enum {
    uipickpat_DateYears = 0,
    uipickpat_DateMonths,
    uipickpat_DateWeeks,
    uipickpat_DateDays,
    uipickpat_DateUnknown,
    uipickpat_DateCount = uipickpat_DateUnknown
};
typedef enum uipickpat_date_units_enum uipickpat_date_units_t;

#define uipickpat_SetPattern(UP, P)   	(str_replace(&(UP)->pattern, (P)))
#define uipickpat_SetHeader(UP, P)    	(str_replace(&(UP)->header, (P)))
#define uipickpat_GetHeader(UP)		((UP)->header)
#define uipickpat_GetPattern(UP)	((UP)->pattern)
#define uipickpat_SetFlags(UP, F)	(turnon((UP)->flags, (F)))
#define uipickpat_ClearFlags(UP, F)	(turnoff((UP)->flags, (F)))
#define uipickpat_GetFlags(UP, F)	((UP)->flags & (F))
#define uipickpat_SetDate(UP, U, D)	((UP)->date[(int) U] = (D))
#define uipickpat_GetDate(UP, U)	((UP)->date[(int) U])

struct uipickpat {
    zmFlags flags;
    char *pattern, *header;
    int date[(int) uipickpat_DateCount];
};
typedef struct uipickpat uipickpat_t;

#define uipick_FOREACH(uip, f, i) gptrlist_FOREACH(&(uip)->patlist, uipickpat_t, f, i)

extern void uipick_Init P ((uipick_t *));
extern void uipick_Destroy P ((uipick_t *));
extern zmBool uipick_Parse P ((uipick_t *, char **));
extern uipickpat_date_units_t uipickpat_GetNextDate
       P ((uipickpat_t *, uipickpat_date_units_t, int *));
extern uipickpat_t *uipick_AddPattern P ((uipick_t *));
extern char **uipick_MakeCmd P ((uipick_t *, zmBool));
extern char **uipickpat_GetDateUnitDescs P ((void));

/* this is really for the filters dialog... */
#define uipickpat_Date_Older 	0
#define uipickpat_Date_Newer	1
#define uipickpat_Date_Count	2
extern void uipickpat_GetDateInfo P ((uipickpat_t *, int *, uipickpat_date_units_t *));

#endif /* _UIPICK_H_ */
