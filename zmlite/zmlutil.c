/* 
 * $RCSfile: zmlutil.c,v $
 * $Revision: 2.21 $
 * $Date: 1995/07/25 22:02:49 $
 * $Author: bobg $
 */

#include <spoor.h>

#include <spoor/toggle.h>
#include <spoor/splitv.h>
#include <spoor/menu.h>
#include <spoor/event.h>
#include <spoor/im.h>

#include <msgf.h>
#include <helpindx.h>

#include <zmlite.h>

#include <zmail.h>
#include <pager.h>

#include <dynstr.h>

#include "catalog.h"

#ifndef lint
static const char zmlutil_rcsid[] =
    "$Id: zmlutil.c,v 2.21 1995/07/25 22:02:49 bobg Exp $";
#endif /* lint */

extern char *getenv();

struct spSplitview *
SplitAdd(split, child, size, which, percentp, type, style, borders)
    struct spSplitview *split;
    struct spView *child;
    int size, which, percentp;
    enum spSplitview_splitType type;
    enum spSplitview_style style;
    int borders;
{
    struct spSplitview *new = spSplitview_NEW();

    spSend(split, m_spSplitview_setup,
	   (which ? (struct spView *) new : child),
	   (which ? child : (struct spView *) new),
	   size, which, percentp, type, style, borders);
    return (new);
}

void
KillSplitviews(tree)
    struct spView *tree;
{
    if (spoor_IsClassMember(tree, spSplitview_class)) {
	struct spView *tmp;

	if (tmp = spSplitview_child(tree, 0)) {
	    spSend(tree, m_spSplitview_setChild, (struct spView *) 0, 0);
	    KillSplitviews(tmp);
	}
	if (tmp = spSplitview_child(tree, 1)) {
	    spSend(tree, m_spSplitview_setChild, (struct spView *) 0, 1);
	    KillSplitviews(tmp);
	}
	spoor_DestroyInstance(tree);
    }
}

void
KillAllButOneSplitview(tree, saveme)
    struct spView *tree;
    struct spSplitview *saveme;
{
    if (spoor_IsClassMember(tree, spSplitview_class)
	&& (tree != (struct spView *) saveme)) {
	struct spView *tmp;

	if (tmp = spSplitview_child(tree, 0)) {
	    spSend(tree, m_spSplitview_setChild, (struct spView *) 0, 0);
	    KillAllButOneSplitview(tmp, saveme);
	}
	if (tmp = spSplitview_child(tree, 1)) {
	    spSend(tree, m_spSplitview_setChild, (struct spView *) 0, 1);
	    KillAllButOneSplitview(tmp, saveme);
	}
	spoor_DestroyInstance(tree);
    }
}

void
KillSplitviewsAndWrapviews(view)
    struct spView *view;
{
    struct spView *v;

    if (spoor_Class(view) == spSplitview_class) {
	if (v = spSplitview_child(view, 0)) {
	    spSend(view, m_spSplitview_setChild, (struct spView *) 0, 0);
	    KillSplitviewsAndWrapviews(v);
	}
	if (v = spSplitview_child(view, 1)) {
	    spSend(view, m_spSplitview_setChild, (struct spView *) 0, 1);
	    KillSplitviewsAndWrapviews(v);
	}
	spoor_DestroyInstance(view);
    } else if (spoor_Class(view) == spWrapview_class) {
	if (v = spWrapview_view(view)) {
	    spSend(view, m_spWrapview_setView, (struct spView *) 0);
	    KillSplitviewsAndWrapviews(v);
	}
	spoor_DestroyInstance(view);
    }
}

char *
rvl(a)
    int *a;
{
    static char buf[128];
    int i;

    for (i = 0; a[i]; ++i)
	buf[i] = (char) a[i];
    return (buf);
}

void
doSaveState()
{
    struct dynstr d;

    dynstr_Init(&d);
    TRY {
	char *dflt;

	if (!(dflt = getenv("ZMAILRC")))
	    if (!(dflt = getenv("MAILRC")))
#if defined( MSDOS ) /* RJL ** 5.25.93 - default for MSDOS */
		dflt = "~\\zmail.rc";
#else /* !MSDOS */
		dflt = "~/.zmailrc";
#endif /* !MSDOS */
	if (!dyn_choose_one(&d, catgets(catalog, CAT_LITE, 523, "Save application state to:"), dflt,
			   NULL, 0, PB_FILE_BOX | PB_NOT_A_DIR)
	    && !dynstr_EmptyP(&d)) {
	    dynstr_Replace(&d, 0, 0, "\\saveopts -g ");
	    ZCommand(dynstr_Str(&d), zcmd_commandline);
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

typedef void (*bfn_t) P((struct spButton *,
			 GENERIC_POINTER_TYPE *));

struct spButtonv *
ActionArea(VA_ALIST(GENERIC_POINTER_TYPE *callbackData))
    VA_DCL
{
    VA_LIST ap;
    VA_ZLIST(GENERIC_POINTER_TYPE *callbackData);
    struct spButtonv *result = spButtonv_NEW();
    char *label;
    bfn_t fn;

    VA_START(ap, GENERIC_POINTER_TYPE *, callbackData);
    spView_callbackData(result) = callbackData;
    while (label = VA_ARG(ap, char *)) {
	fn = VA_ARG(ap, bfn_t);
	spSend(result, m_spButtonv_insert,
	       spButton_Create(label, fn, callbackData), -1);
    }
    VA_END(ap);
    return (result);
}

void
zmlhelp(str)
    const char *str;
{
    help(HelpInterface, (VPTR) str, tool_help);
}
