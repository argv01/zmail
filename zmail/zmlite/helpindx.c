/*
 * $RCSfile: helpindx.c,v $
 * $Revision: 2.31 $
 * $Date: 1995/10/24 23:52:35 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <helpindx.h>

#include <zmail.h>
#include <zmlutil.h>
#include <zmlite.h>

#include <dynstr.h>

#include <spoor/popupv.h>
#include <spoor/buttonv.h>
#include <spoor/button.h>
#include <spoor/toggle.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include "catalog.h"

#ifndef lint
static const char helpIndex_rcsid[] =
    "$Id: helpindx.c,v 2.31 1995/10/24 23:52:35 bobg Exp $";
#endif /* lint */

struct spWclass *helpIndex_class = 0;

int m_helpIndex_clear;
int m_helpIndex_append;
int m_helpIndex_setTopic;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static int
compareButtons(b1, b2)
    char **b1, **b2;
{
    return (strcmp(*b1, *b2));
}

static void
fillInList(self)
    struct helpIndex *self;
{
    FILE *fp = (FILE *) 0;
    char **bpp;
    char buf[256], *p;
    struct glist blist;
    struct dynstr d;
    int i;

    glist_Init(&blist, (sizeof (char *)), 16);
    dynstr_Init(&d);
    TRY {
	if (spToggle_state(spButtonv_button(self->category, 0))) {
	    /* User Interface */
	    fp = efopen(tool_help, "r", "helpIndex/fillInList");
	} else {
	    /* Z-Script Commands */
	    fp = efopen(cmd_help, "r", "helpIndex/fillInList");
	}
	while (fgets(buf, (sizeof (buf)), fp)) {
	    if ((buf[0] == '%') && (buf[1] != '%')) {
		if ((buf[1] != '-') && zglob(buf, "%?*%\n")) {
		    p = index(buf + 1, '%');
		    dynstr_Set(&d, "");
		    dynstr_AppendN(&d, buf + 1, p - (buf + 1));
		    p = savestr(dynstr_Str(&d));
		    glist_Add(&blist, &p);
		}
		while (fgets(buf, (sizeof (buf)), fp)
		       && strncmp(buf, "%%", 2))
		    ;
	    }
	}
	glist_Sort(&blist, compareButtons);
	glist_FOREACH(&blist, char *, bpp, i) {
	    spSend(spView_observed(self->list), m_spList_append, *bpp);
	    free(*bpp);
	}
    } FINALLY {
	if (fp)
	    fclose(fp);
	glist_Destroy(&blist);
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
listCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct helpIndex *hi = (struct helpIndex *) spView_callbackData(self);
    struct dynstr d;

    dynstr_Init(&d);
    TRY {
	spSend(spView_observed(self), m_spList_getNthItem, which, &d);
	help(HelpInterface, dynstr_Str(&d),
	     (spToggle_state(spButtonv_button(hi->category, 0)) ?
	      tool_help : cmd_help));
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
categoryActivate(self, which, clicktype)
    struct spButtonv *self;
    int which;
{
    struct helpIndex *hi = (struct helpIndex *) spView_callbackData(self);
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;
    struct spWrapview *w1, *w2;

    spButtonv_radioButtonHack(self, which);
    spSend(spView_observed(hi->list), m_spText_clear);
    fillInList(hi);
    w1 = (struct spWrapview *) spSplitview_child(hi->split, 0);
    w2 = (struct spWrapview *) spSplitview_child(hi->split, 1);
    spSend(w1, m_spView_desiredSize, &minh, &minw, &maxh, &maxw,
	   &besth, &bestw);
    spSend(hi->split, m_spSplitview_setup, w1, w2,
	   (bestw ? bestw : 25), 0, !bestw,
	   spSplitview_leftRight, spSplitview_plain, 0);
    spSend(hi->list, m_spView_invokeInteraction, "list-click",
	   self, NULL, NULL);
    spSend(hi->list, m_spView_wantFocus, hi->list);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct helpIndex *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct helpIndex *self;
{
    zmlhelp("Help Index");
}

static void
helpIndex_initialize(self)
    struct helpIndex *self;
{
    struct spText *t;
    struct spSplitview *split;
    struct spWrapview *wrap, *wrap2;
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

    ZmlSetInstanceName(self, "helpindex", self);

    spSend(self->list = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->list) = listCallback;
    spView_callbackData(self->list) = (struct spoor *) self;
    ZmlSetInstanceName(self->list, "helpindex-topic-list", self);

    self->category = spButtonv_NEW();
    ZmlSetInstanceName(self->category, "helpindex-topic-rg", self);
    spSend(self->category, m_spView_setWclass, spwc_Radiogroup);
    spButtonv_style(self->category) = spButtonv_horizontal;
    spButtonv_toggleStyle(self->category) = spButtonv_checkbox;
    spButtonv_callback(self->category) = categoryActivate;
    spView_callbackData(self->category) = (struct spoor *) self;
    spSend(self->category, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 277, "User Interface"), 0, 0, 1), 0);
    spSend(self->category, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 278, "Z-Script Commands"), 0, 0, 0), 1);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	ZmlSetInstanceName(dialog_actionArea(self), "helpindex-aa", self);

	spSend(self->text = spTextview_NEW(), m_spView_setObserved,
	       t = spText_NEW());
	spSend(t, m_spText_setReadOnly, 1);
	ZmlSetInstanceName(self->text, "helpindex-text", self);
	spTextview_showpos(self->text) = 1;

	fillInList(self);

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 281, "Help Index"), spWrapview_top);

	spSend(self, m_dialog_setView, split = spSplitview_NEW());
    } dialog_ENDMUNGE;

    wrap = spWrapview_NEW();
    spSend(wrap, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 282, "Help Category: "), spWrapview_left);
    spSend(wrap, m_spWrapview_setView, self->category);
    self->split = SplitAdd(split, wrap, 1, 1, 0, spSplitview_topBottom,
			   spSplitview_plain, 0);

    wrap = spWrapview_NEW();
    spWrapview_boxed(wrap) = 1;
    spSend(wrap, m_spWrapview_setView, self->list);

    wrap2 = spWrapview_NEW();
    spWrapview_boxed(wrap2) = 1;
    spSend(wrap2, m_spWrapview_setView, self->text);

    spSend(wrap, m_spView_desiredSize, &minh, &minw, &maxh, &maxw,
	   &besth, &bestw);

    spSend(self->split, m_spSplitview_setup, wrap, wrap2,
	   (bestw ? bestw : 25), 0, !bestw,
	   spSplitview_leftRight, spSplitview_plain, 0);

    spSend(self, m_dialog_addFocusView, self->list);
    spSend(self, m_dialog_addFocusView, self->text);
    spSend(self, m_dialog_addFocusView, self->category);
}

static void
helpIndex_finalize(self)
    struct helpIndex *self;
{
    /* To do: undo everything from above */
}

static void
helpIndex_clear(self, arg)
    struct helpIndex *self;
    spArgList_t arg;
{
    spSend(spView_observed(self->text), m_spText_clear);
}

static void
helpIndex_append(self, arg)
    struct helpIndex *self;
    spArgList_t arg;
{
    char *p;

    p = spArg(arg, char *);
    spSend(spView_observed(self->text), m_spText_insert, -1,
	   strlen(p), p, spText_mAfter);
}

static void
helpIndex_desiredSize(self, arg)
    struct helpIndex *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int screenw = 80, screenh = 24;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(helpIndex_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    if (*bestw < (screenw - 4)) {
	*bestw = screenw - 4;
    }
    if (*besth < (screenh - 6)) {
	*besth = screenh - 6;
    }
}

static void
helpIndex_setTopic(self, arg)
    struct helpIndex *self;
    spArgList_t arg;
{
    char *topic = spArg(arg, char *);
    int i, len = spSend_i(spView_observed(self->list), m_spList_length);
    struct dynstr d;

    if (!topic)
	return;
    dynstr_Init(&d);
    TRY {
	for (i = 0; i < len; ++i) {
	    dynstr_Set(&d, "");
	    spSend(spView_observed(self->list), m_spList_getNthItem,
		   i, &d);
	    if (!ci_identcmp(dynstr_Str(&d), topic)) {
		spSend(self->list, m_spListv_deselectAll);
		spSend(self->list, m_spListv_select, i);
		break;
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
helpIndex_activate(self, arg)
    struct helpIndex *self;
    spArgList_t arg;
{
    spSuper(helpIndex_class, self, m_dialog_activate);
    spSend(self->list, m_spListv_frameHighlighted, 1);
}

struct spWidgetInfo *spwc_Helpindex = 0;

void
helpIndex_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (helpIndex_class)
	return;
    helpIndex_class =
	spWclass_Create("helpIndex", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct helpIndex)),
			helpIndex_initialize,
			helpIndex_finalize,
			spwc_Helpindex = spWidget_Create("Helpindex",
							 spwc_Popup));

    m_helpIndex_clear =
	spoor_AddMethod(helpIndex_class, "clear",
			NULL,
			helpIndex_clear);
    m_helpIndex_append =
	spoor_AddMethod(helpIndex_class, "append",
			NULL,
			helpIndex_append);
    m_helpIndex_setTopic =
	spoor_AddMethod(helpIndex_class, "setTopic",
			NULL,
			helpIndex_setTopic);

    spoor_AddOverride(helpIndex_class, m_dialog_activate, 0,
		      helpIndex_activate);
    spoor_AddOverride(helpIndex_class, m_spView_desiredSize, NULL,
		      helpIndex_desiredSize);

    spPopupView_InitializeClass();
    spButtonv_InitializeClass();
    spButton_InitializeClass();
    spToggle_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
