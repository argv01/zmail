/*
 * $RCSfile: taskmtr.h,v $
 * $Revision: 2.5 $
 * $Date: 1995/09/20 06:44:32 $
 * $Author: liblit $
 *
 * $Log: taskmtr.h,v $
 * Revision 2.5  1995/09/20 06:44:32  liblit
 * Get rather carried away and prototype a large number of zero-argument
 * functions.  Unlike C++, ANSI C has two extremely different meanings
 * for "()" and "(void)" in function declarations.
 *
 * Also prototype some parameter-taking functions, but not too many,
 * because there are only so many compiler warnings I can take.  :-)
 *
 * In printdialog_activate(), found in printd.c, change the order of some
 * operations so that a "namep" field gets initialized before it is first
 * referenced.  The UMR that this fixes corresponds to PR #6441.
 *
 * Revision 2.4  1994/02/24 19:20:43  bobg
 * Switch over to the new way of doing widget classes.  Previously,
 * hashtable lookups due to widget-class searches accounted for more than
 * 33% of Lite's running time.  The new widget class scheme eliminates
 * nearly all those lookups, replacing them with simple pointer
 * traversals and structure dereferences.  Profiling this version of Lite
 * reveals that we're back to the state where the main thing slowing down
 * Lite is the core itself.  Yay!
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.3  1993/12/01  00:10:53  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef TASKMETER_H
#define TASKMETER_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/textview.h>

#define taskmeter_INTERRUPTABLE (1<<0)
#define taskmeter_EXCEPTION     (1<<1)

struct taskmeter {
    SUPERCLASS(dialog);
    struct spTextview *scale, *submsg;
    int interruptable, percentage;
    unsigned long flags;
};

#define taskmeter_interruptable(t) (((struct taskmeter *) (t))->interruptable)
#define taskmeter_percentage(t) (((struct taskmeter *) (t))->percentage)
#define taskmeter_flags(t) (((struct taskmeter *) (t))->flags)

#define taskmeter_msg(t) (spWrapview_label((t),spWrapview_top))

extern struct spWclass *taskmeter_class;

extern struct spWidgetInfo *spwc_Taskmeter;

#define taskmeter_NEW() \
    ((struct taskmeter *) spoor_NewInstance(taskmeter_class))

extern int m_taskmeter_setMainMsg;
extern int m_taskmeter_setSubMsg;
extern int m_taskmeter_setScale;
extern int m_taskmeter_setInterruptable;

extern void taskmeter_InitializeClass P((void));

#endif /* TASKMETER_H */
