/*
 * $RCSfile: cmdline.h,v $
 * $Revision: 2.11 $
 * $Date: 1994/04/21 19:11:31 $
 * $Author: bobg $
 *
 * $Log: cmdline.h,v $
 * Revision 2.11  1994/04/21 19:11:31  bobg
 * Finish spCmdline_jump feature.  Rebind scrolling keys in Text and
 * EditText.
 *
 * Revision 2.10  1994/03/03  07:31:09  bobg
 * Add "jump" field to cmdline.
 *
 * Revision 2.9  1994/02/24  19:15:49  bobg
 * Change everything.
 *
 * But seriously, change the way "widget classes" are handled.  Rather
 * than requiring a bazillion hashtable lookups (because of everything
 * being string-keyword based), use actual data structures to associate
 * e.g. keymaps and interaction lists with widget classes.  A spoor
 * class's "widget class" is now stored in the spoor class descriptor
 * itself, rather than requiring yet two more hashtable lookups.  Of
 * course, spoor class descriptors don't have room for a "widget class"
 * pointer field, but fortunately, class descriptors are instances of the
 * class class!  So, subclass the class class to get a subclass of
 * "class" called spWclass (re-read that sentence until your eyes don't
 * cross).  Any class that wants to point to a widget class now creates
 * its class descriptor as an instance of spWclass, not spClass.
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.8  1994/02/02  18:31:32  bobg
 * Create "protect" method to protect a cmdline's present contents from
 * reverting when focus is lost.
 *
 * Revision 2.7  1994/02/02  03:43:50  bobg
 * By default, cmdline objects now revert to their previous contents when
 * focus leaves them, unless the user has committed any edits by pressing
 * RETURN.  Also remove some dead code.
 */

#ifndef SPOOR_CMDLINE_H
#define SPOOR_CMDLINE_H

#include <spoor.h>
#include "textview.h"

#include <dynstr.h>

struct spCmdline {
    SUPERCLASS(spTextview);
    void (*fn) NP((struct spCmdline *, char *));
    struct spoor *obj;
    GENERIC_POINTER_TYPE *data;
    struct dynstr oldstr;
    int revert:1, saved:1, jump:1;
};

#define spCmdline_revert(x) (((struct spCmdline *) (x))->revert)
#define spCmdline_jump(x)   (((struct spCmdline *) (x))->jump)
#define spCmdline_fn(c)	    (((struct spCmdline *) (c))->fn)
#define spCmdline_obj(c)    (((struct spCmdline *) (c))->obj)
#define spCmdline_data(c)   (((struct spCmdline *) (c))->data)

extern struct spWclass *spCmdline_class;

extern struct spWidgetInfo *spwc_Inputfield;

#define spCmdline_NEW() \
    ((struct spCmdline *) spoor_NewInstance(spCmdline_class))

extern int m_spCmdline_protect;

extern void spCmdline_InitializeClass();

extern struct spCmdline *spCmdline_Create P((void (*) NP((struct spCmdline *,
							  char *))));

#endif /* SPOOR_CMDLINE_H */
