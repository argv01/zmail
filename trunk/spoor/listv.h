/*
 * $RCSfile: listv.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/03/30 23:31:21 $
 * $Author: bobg $
 */

#ifndef SPOOR_LISTV_H
#define SPOOR_LISTV_H

#include <spoor.h>
#include "textview.h"
#include "intset.h"

enum spListv_clicktype {
    spListv_click,
    spListv_shiftclick,
    spListv_controlclick,
    spListv_doubleclick,
    spListv_CLICKTYPES
};

#define spListv_ALLCLICKS ((1 << spListv_click) | \
			   (1 << spListv_shiftclick) | \
			   (1 << spListv_controlclick) | \
			   (1 << spListv_doubleclick))

struct spListv {
    SUPERCLASS(spTextview);
    void (*callback) NP((struct spListv *, int, enum spListv_clicktype));
    int lastclick, doselect;
    unsigned long okclicks;
    struct intset selections;
};

#define spListv_selections(l) (&(((struct spListv *) (l))->selections))
#define spListv_callback(l) (((struct spListv *) (l))->callback)
#define spListv_lastclick(l) (((struct spListv *) (l))->lastclick)
#define spListv_okclicks(l) (((struct spListv *) (l))->okclicks)
#define spListv_doselect(l) (((struct spListv *) (l))->doselect)

#define spListv_NEW() \
    ((struct spListv *) spoor_NewInstance(spListv_class))

extern struct spWclass *spListv_class;

extern struct spWidgetInfo *spwc_List;

extern int m_spListv_frameHighlighted;
extern int m_spListv_select;
extern int m_spListv_deselect;
extern int m_spListv_deselectAll;

extern struct spListv *spListv_Create(VA_PROTO(unsigned long));

#endif /* SPOOR_LISTV_H */
