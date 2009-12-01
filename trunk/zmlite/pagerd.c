/*
 * $RCSfile: pagerd.c,v $
 * $Revision: 2.32 $
 * $Date: 1996/04/22 23:36:22 $
 * $Author: spencer $
 */

#include <spoor.h>
#include <pagerd.h>

#include <spoor/popupv.h>
#include <spoor/splitv.h>
#include <spoor/text.h>
#include <spoor/textview.h>
#include <spoor/buttonv.h>
#include <spoor/button.h>
#include <spoor/menu.h>
#include <spoor/toggle.h>

#include <tsearch.h>

#include <zmlite.h>
#include <zmlutil.h>
#include <lpr.h>
#include <pager.h>

#include "catalog.h"

#ifndef lint
static const char pagerDialog_rcsid[] =
    "$Id: pagerd.c,v 2.32 1996/04/22 23:36:22 spencer Exp $";
#endif /* lint */

struct spWclass *pagerDialog_class = 0;

int m_pagerDialog_append;
int m_pagerDialog_clear;
int m_pagerDialog_setFile;
int m_pagerDialog_setTitle;

static catalog_ref about_str = catref(CAT_LITE, 680, "About the Text Pager");

static void
do_search(self)
    struct pagerDialog *self;
{
    struct tsearch *ts = tsearch_NEW();

    TRY {
	spSend(ts, m_tsearch_setText, spView_observed(self->text));
	spSend(ts, m_dialog_interactModally);
    } FINALLY {
	spoor_DestroyInstance(ts);
    } ENDTRY;
}

static void
aa_search(b, self)
    struct spButton *b;
    struct pagerDialog *self;
{
    do_search(self);
}

static void
do_write_file(self, file)
    struct pagerDialog *self;
    char *file;
{
    spSend(self->text, m_spTextview_writeFile, file);
    spSend(ZmlIm, m_spIm_showmsg, zmVaStr(catgets(catalog, CAT_LITE, 681, "Saved file %s"), file),
	   15, 2, 0);
    self->modified = 0;
}

#define setfilename(self,name) (dynstr_Set(&((self)->filename),(name)))
#define getfilename(self) (dynstr_Str(&(self)->filename))

static int
do_saveas(self)
    struct pagerDialog *self;
{
    struct dynstr d;
    int retval = 0;

    dynstr_Init(&d);
    TRY {
	struct stat statbuf;

	if (!dyn_choose_one(&d, catgets(catalog, CAT_LITE, 682, "Save as file:"), 0, 0, 0,
			    PB_FILE_BOX | PB_NOT_A_DIR)
	    && strcmp(dynstr_Str(&d), getfilename(self))
	    && (stat(dynstr_Str(&d), &statbuf)
		|| (ask(AskNo, catgets(catalog, CAT_LITE, 683, "%s exists.  Overwrite?"),
			dynstr_Str(&d)) == AskYes))) {
	    do_write_file(self, dynstr_Str(&d));
	    setfilename(self, dynstr_Str(&d));
	    retval = 1;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (retval);
}

static void
do_save(self)
    struct pagerDialog *self;
{
    char *name = getfilename(self);

    if (name && *name) {
	do_write_file(self, name);
    } else {
	do_saveas(self);
    }
}

static int
save_if_changed(self)
    struct pagerDialog *self;
{
    if (self->modified) {
	AskAnswer a;

	if ((a = ask(AskYes, catgets(catalog, CAT_LITE, 684, "Save changes?"))) == AskYes) {
	    char *name = getfilename(self);

	    if (name && *name) {
		do_write_file(self, name);
		return (1);
	    } else {
		return (do_saveas(self));
	    }
	}
	if (a == AskNo) {
	    return (1);
	}
	return (0);		/* canceled */
    }
    return (1);
}

static void
do_close(self)
    struct pagerDialog *self;
{
    if (save_if_changed(self))
	spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_save(b, self)
    struct spButton *b;
    struct pagerDialog *self;
{
    do_save(self);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct pagerDialog *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
do_replace_from_file(self, name)
    struct pagerDialog *self;
    char *name;
{
    FILE *fp;

    if (fp = fopen(name, "r")) {
	char buf[BUFSIZ];
	int numbytes;

	spSend(spView_observed(self->text), m_spText_clear);
	while ((numbytes = fread(buf, (sizeof (char)),
				 (sizeof (buf)), fp)) > 0) {
	    spSend(spView_observed(self->text), m_spText_insert,
		   -1, numbytes, buf, spText_mNeutral);
	}
	fclose(fp);
	setfilename(self, name);
    } else {
	error(SysErrWarning, catgets(catalog, CAT_LITE, 685, "Could not open %s"), name);
    }
}

static void
open_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    struct dynstr d;

    if (!save_if_changed(self))
	return;
    dynstr_Init(&d);
    TRY {
	if (!dyn_choose_one(&d, catgets(catalog, CAT_LITE, 686, "Select a file to insert"), 0, 0, 0,
			    PB_FILE_BOX | PB_NOT_A_DIR | PB_MUST_EXIST)) {
	    do_replace_from_file(self, dynstr_Str(&d));
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
save_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    do_save(self);
}

static void
saveas_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    do_saveas(self);
}

static void
insert_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    struct dynstr d;

    if (spText_readOnly(spView_observed(self->text))) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 687, "Text is read only"));
	return;
    }
    dynstr_Init(&d);
    TRY {
	if (!dyn_choose_one(&d, catgets(catalog, CAT_LITE, 688, "Select a file to open"), 0, 0, 0,
			    PB_FILE_BOX | PB_NOT_A_DIR)) {
	    FILE *fp;

	    if (fp = fopen(dynstr_Str(&d), "r")) {
		char buf[BUFSIZ];
		int numbytes;
		int pos;

		pos = spText_markPos((struct spText *)
				     spView_observed(self->text),
				     spTextview_textPosMark(self->text));
		while ((numbytes = fread(buf, (sizeof (char)),
					 (sizeof (buf)), fp)) > 0) {
		    spSend(spView_observed(self->text), m_spText_insert,
			   pos, numbytes, buf, spText_mNeutral);
		    pos += numbytes;
		}
		fclose(fp);
	    } else {
		error(SysErrWarning, catgets(catalog, CAT_LITE, 685, "Could not open %s"), dynstr_Str(&d));
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
print_fn(menu, b, self, data)	/* largely copied from motif/m_paging.c */
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    char *text, *prog;
    ZmPager pager;
    struct printdata pdata;

    bzero(&pdata, sizeof pdata);
    if (!(pager = printer_setup(&pdata, 0)))
	return;
    LITE_BUSY {
	struct dynstr d;

	dynstr_Init(&d);
	TRY {
	    spSend(spView_observed(self->text), m_spText_appendToDynstr,
		   &d, 0, -1);
	    ZmPagerWrite(pager, dynstr_Str(&d));
	} FINALLY {
	    dynstr_Destroy(&d);
	} ENDTRY;
	prog = savestr(ZmPagerGetProgram(pager));
	ZmPagerStop(pager);
    } LITE_ENDBUSY;
    print(catgets(catalog, CAT_LITE, 690, "Printed through \"%s\".\n"), prog);
    xfree(prog);
}

static void
close_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    do_close(self);
}

static void
cut_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    spSend(self->text, m_spView_invokeInteraction, "text-cut-selection",
	   0, 0, 0);
}

static void
copy_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    spSend(self->text, m_spView_invokeInteraction, "text-copy-selection",
	   0, 0, 0);
}

static void
selectall_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    spSend(self->text, m_spView_invokeInteraction, "text-select-all",
	   0, 0, 0);
}

static void
clear_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    spSend(self->text, m_spView_invokeInteraction, "text-clear-selection",
	   0, 0, 0);
}


static void
search_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    do_search(self);
}

static void
editor_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    char buf[2 * MAXPATHLEN];
    char *filename = getfilename(self);

    if (!filename || !*filename) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 691, "Use \"Save As ...\" first"));
	return;
    }
    if (save_if_changed(self)) {
	EnterScreencmd(1);
	TRY {
	    spIm_LOCKSCREEN {
		edit_file(filename, 0, 1);
		do_replace_from_file(self, filename);
	    } spIm_ENDLOCKSCREEN;
	} FINALLY {
	    ExitScreencmd(0);
	} ENDTRY;
    }
}

static void
editable_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    spSend(spView_observed(self->text), m_spText_setReadOnly,
	   !spText_readOnly(spView_observed(self->text)));
}

static void
paste_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    spSend(self->text, m_spView_invokeInteraction, "text-paste",
	   0, 0, 0);
}

static void
help_fn(menu, b, self, data)
    struct spMenu *menu;
    struct spButton *b;
    struct pagerDialog *self;
    GENERIC_POINTER_TYPE *data;
{
    if (strcmp(spButton_label(b), catgetref(about_str)))
	zmlhelp(spButton_label(b));
    else
	zmlhelp("Text Pager");
}

static void
pagerDialog_initialize(self)
    struct pagerDialog *self;
{
    struct spText *t;
    struct spMenu *menu;

    ZmlSetInstanceName(self, "pager", self);

    self->modified = 0;

    dynstr_Init(&(self->filename));

    spSend(self->text = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spSend(t, m_spObservable_addObserver, self);
    ZmlSetInstanceName(self->text, "pager-text", self);
    spTextview_showpos(self->text) = 1;
    spTextview_wrapmode(self->text) = spTextview_nowrap;

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			  catgets(catalog, CAT_LITE, 25, "Search"), aa_search,
			  0));
	spSend(self, m_dialog_setMenu,
	       spMenu_Create((struct spoor *) self, 0, spButtonv_horizontal,
			     catgets(catalog, CAT_LITE, 695, "File"), spMenu_menu,
			     spMenu_Create((struct spoor *) self,
					   0, spButtonv_vertical,
					   catgets(catalog, CAT_LITE, 696, "Open ..."), spMenu_function, open_fn,
					   catgets(catalog, CAT_LITE, 697, "Insert ..."), spMenu_function, insert_fn,
					   catgets(catalog, CAT_LITE, 62, "Save"), spMenu_function, save_fn,
					   catgets(catalog, CAT_LITE, 699, "Save As ..."), spMenu_function, saveas_fn,
					   catgets(catalog, CAT_LITE, 368, "Print"), spMenu_function, print_fn,
					   catgets(catalog, CAT_LITE, 701, "Close"), spMenu_function, close_fn,
					   0),
			     catgets(catalog, CAT_LITE, 11, "Edit"), spMenu_menu,
			     menu = spMenu_Create((struct spoor *) self,
						  0, spButtonv_vertical,
						  catgets(catalog, CAT_LITE, 703, "Cut"), spMenu_function, cut_fn,
						  catgets(catalog, CAT_LITE, 704, "Copy"), spMenu_function, copy_fn,
						  catgets(catalog, CAT_LITE, 705, "Paste"), spMenu_function, paste_fn,
						  catgets(catalog, CAT_LITE, 706, "Select All"), spMenu_function, selectall_fn,
						  catgets(catalog, CAT_LITE, 26, "Clear"), spMenu_function, clear_fn,
						  catgets(catalog, CAT_LITE, 708, "Search/Replace/Spell ..."), spMenu_function, search_fn,
						  catgets(catalog, CAT_LITE, 709, "Editor ..."), spMenu_function, editor_fn,
						  0),
			     catgets(catalog, CAT_LITE, 17, "Help"), spMenu_menu,
			     spMenu_Create((struct spoor *) self,
					   0, spButtonv_vertical,
					   catgetref(about_str), spMenu_function, help_fn,
					   catgets(catalog, CAT_LITE, 711, "Creating a Signature"), spMenu_function, help_fn,
					   catgets(catalog, CAT_LITE, 712, "Displaying text attachments"), spMenu_function, help_fn,
					   0),
			     0));
	spSend(menu, m_spMenu_addFunction,
	       self->editable = spToggle_Create(catgets(catalog, CAT_LITE, 713, "Editable"), 0, 0, 0),
	       editable_fn, -1, self, 0);
	spButtonv_toggleStyle(menu) = spButtonv_checkbox;
	ZmlSetInstanceName(dialog_actionArea(self), "pager-aa", self);

	spSend(self, m_dialog_setView, self->text);
    } dialog_ENDMUNGE;
    spWrapview_highlightp(self) = 1;
    spWrapview_boxed(self) = 1;
    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 714, "Text Pager"), spWrapview_top);

    spSend(self, m_dialog_addFocusView, self->text);
}

static void
pagerDialog_finalize(self)
    struct pagerDialog *self;
{
    spSend(self, m_dialog_setView, 0);
    spSend(self->text, m_spView_destroyObserved);
    dynstr_Destroy(&(self->filename));
    spoor_DestroyInstance(self->text);
}

static void
pagerDialog_append(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    char *buf;

    buf = spArg(arg, char *);
    spSend(spView_observed(pagerDialog_textview(self)),
	   m_spText_insert, -1, strlen(buf), buf, spText_mAfter);
}

static void
pagerDialog_clear(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    spSend(spView_observed(pagerDialog_textview(self)), m_spText_clear);
}

static void
pagerDialog_desiredSize(self, arg)
    struct pagerDialog *self;
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

    spSuper(pagerDialog_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    if (*bestw < (screenw - 2))
	*bestw = screenw - 2;
    if (*besth < screenh - 2)
	*besth = screenh - 2;
}

static void
pagerDialog_deactivate(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    int val = spArg(arg, int);

    if (save_if_changed(self))
	spSuper(pagerDialog_class, self, m_dialog_deactivate, val);
}

static void
pagerDialog_setFile(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    char *filename = spArg(arg, char *);

    setfilename(self, filename);
}

static void
pagerDialog_receiveNotification(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int), istoggle;
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(pagerDialog_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (o == spView_observed(self->text)) {
	if (event == spText_readOnlynessChanged) {
	    spSend(self->editable, m_spToggle_set, !spText_readOnly(o));
	} else {
	    self->modified = 1;
	}
    }
}

static int
pagerDialog_interactModally(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    self->modified = 0;
    return (spSuper_i(pagerDialog_class, self, m_dialog_interactModally));
}

static void
pagerDialog_setTitle(self, arg)
    struct pagerDialog *self;
    spArgList_t arg;
{
    char *title = spArg(arg, char *);

    if (title)
	spSend(self, m_spWrapview_setLabel, title, spWrapview_top);
}

struct spWidgetInfo *spwc_Pager = 0;

void
pagerDialog_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (pagerDialog_class)
	return;
    pagerDialog_class =
	spWclass_Create("pagerDialog", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct pagerDialog)),
			pagerDialog_initialize,
			pagerDialog_finalize,
			spwc_Pager = spWidget_Create("Pager",
						     spwc_MenuPopup));

    m_pagerDialog_setTitle =
	spoor_AddMethod(pagerDialog_class, "setTitle",
			0, pagerDialog_setTitle);
    m_pagerDialog_setFile =
	spoor_AddMethod(pagerDialog_class, "setFile",
			0, pagerDialog_setFile);
    m_pagerDialog_append =
	spoor_AddMethod(pagerDialog_class, "append",
			NULL,
			pagerDialog_append);
    m_pagerDialog_clear =
	spoor_AddMethod(pagerDialog_class, "clear",
			NULL,
			pagerDialog_clear);
    spoor_AddOverride(pagerDialog_class, m_dialog_interactModally, 0,
		      pagerDialog_interactModally);
    spoor_AddOverride(pagerDialog_class, m_spObservable_receiveNotification,
		      0, pagerDialog_receiveNotification);
    spoor_AddOverride(pagerDialog_class, m_spView_desiredSize, NULL,
		      pagerDialog_desiredSize);
    spoor_AddOverride(pagerDialog_class, m_dialog_deactivate, 0,
		      pagerDialog_deactivate);

    spToggle_InitializeClass();
    spText_InitializeClass();
    spTextview_InitializeClass();
    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spButton_InitializeClass();
    spPopupView_InitializeClass();
    spMenu_InitializeClass();
    tsearch_InitializeClass();
}
