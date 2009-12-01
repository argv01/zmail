/*
 * $RCSfile: printd.c,v $
 * $Revision: 2.30 $
 * $Date: 1995/09/20 06:44:25 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <printd.h>

#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/buttonv.h>
#include <spoor/text.h>
#include <spoor/button.h>
#include <spoor/toggle.h>
#include <spoor/popupv.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include <spoor/splitv.h>
#include <spoor/wrapview.h>

#include <zmlite.h>

#include <zmail.h>
#include <zmlutil.h>

#include <dynstr.h>

#include "catalog.h"

#ifndef lint
static const char printdialog_rcsid[] =
    "$Id: printd.c,v 2.30 1995/09/20 06:44:25 liblit Exp $";
#endif /* lint */

struct spWclass *printdialog_class = 0;

enum {
    DONE_B, PRINT_B, HELP_B
};

enum {
    STANDARDHDRS_B, ALLHEADERS_B, BODYONLY_B
};

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
optionsActivate(self, which, clicktype)
    struct spButtonv *self;
    int which;
{
    spButtonv_radioButtonHack(self, which);
}

static void
printersCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct printdialog *p = (struct printdialog *) spView_callbackData(self);

    if (p->namep) {
	struct dynstr d;

	dynstr_Init(&d);
	spSend(spView_observed(p->command), m_spText_clear);
	spSend(spView_observed(self), m_spList_getNthItem, which, &d);
	spSend(spView_observed(p->command), m_spText_insert, 0, -1,
	       dynstr_Str(&d), spText_mBefore);
	dynstr_Destroy(&d);
    }
}

static void
aa_done(b, self)
    struct spButton *b;
    struct printdialog *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_print(b, self)
    struct spButton *b;
    struct printdialog *self;
{
    static struct dynstr d, p;
    static int initialized = 0;
    char *a_ign_val = 0, **pv;

    if (!initialized) {
	dynstr_Init(&d);
	dynstr_Init(&p);
	initialized = 1;
    }
    dynstr_Set(&p, "");
    dynstr_Set(&d, "lpr ");
    if (spToggle_state(spButtonv_button(self->options, STANDARDHDRS_B))) {
	dynstr_Append(&d, "-h ");
    } else if (spToggle_state(spButtonv_button(self->options, BODYONLY_B))) {
	dynstr_Append(&d, "-n ");
    } else if (a_ign_val = get_var_value(VarAlwaysignore)) {
	if (a_ign_val = savestr(a_ign_val))
	    un_set(&set_options, VarAlwaysignore);
    }
    spSend(spView_observed(self->command), m_spText_appendToDynstr,
	   &p, 0, -1);
    if (boolean_val(VarPrintCmd)) {
	if (!dynstr_EmptyP(&p)) {
	    char *setv[4];

	    setv[0] = "print_cmd";
	    setv[1] = "=";
	    setv[2] = dynstr_Str(&p);
	    setv[3] = NULL;
	    (void) add_option(&set_options, setv);
	} else {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 363, "Empty print command."));
	    xfree(a_ign_val);
	    return;
	}
	pv = strvec(get_var_value(VarPrinter), " ,\t", 1);
	if (pv) {
	    int r;

	    if (((r = spListv_lastclick(self->printers)) >= 0)
		&& intset_Contains(spListv_selections(self->printers), r)) {
		dynstr_Append(&d, "-P");
		spSend(spView_observed(self->printers),
		       m_spList_getNthItem, r, &d);
		dynstr_AppendChar(&d, ' ');
	    }
	    free_vec(pv);
	}
    } else if (!dynstr_EmptyP(&p)) {
	dynstr_Append(&d, "-P");
	dynstr_Append(&d, dynstr_Str(&p));
	dynstr_AppendChar(&d, ' ');
    }
    spSend(spView_observed(dialog_messages(self)), m_spText_appendToDynstr,
	   &d, 0, -1);
    LITE_BUSY {
	ZCommand(dynstr_Str(&d), zcmd_ignore);
    } LITE_ENDBUSY;
    if (a_ign_val) {
	char *setv[4];

	setv[0] = VarAlwaysignore;
	setv[1] = "=";
	setv[2] = a_ign_val;
	setv[3] = NULL;
	(void) add_option(&set_options, setv);
	xfree(a_ign_val);
    }
    if (bool_option(VarAutodismiss, "print"))
	spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct printdialog *self;
{
    zmlhelp("Print Dialog");
}

static void
msgsItem(self, str)
    struct spCmdline *self;
    char *str;
{
    char *argv[2];
    msg_group mgroup;

    init_msg_group(&mgroup, 1, 0);
    argv[0] = str;
    argv[1] = NULL;
    if (get_msg_list(argv, &mgroup))
	spSend(CurrentDialog, m_dialog_setmgroup, &mgroup);
    destroy_msg_group(&mgroup);
}

static void
printdialog_initialize(self)
    struct printdialog *self;
{
    struct spWrapview *wrap, *wrap2;
    int minh = 0, minw = 0, maxh = 0, maxw = 0, besth = 0, bestw = 0;
    int alwaysignore = chk_option(VarAlwaysignore, "printer");

    ZmlSetInstanceName(self, "print", self);

    init_msg_group(&(self->mg), 1, 0);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setopts, dialog_ShowFolder | dialog_ShowMessages);
	ZmlSetInstanceName(dialog_messages(self), "print-messages-field", self);
	spCmdline_fn(dialog_messages(self)) = msgsItem;

	self->options = spButtonv_NEW();
	ZmlSetInstanceName(self->options, "print-headers-rg", self);
	spSend(self->options, m_spView_setWclass, spwc_Radiogroup);

	spSend(self->command = spCmdline_NEW(), m_spView_setObserved,
	       spText_NEW());
	ZmlSetInstanceName(self->command, "print-command-field", self);

	spSend(self->printers = spListv_NEW(), m_spView_setObserved,
	       spList_NEW());
	spView_callbackData(self->printers) = (struct spoor *) self;
	spListv_callback(self->printers) = printersCallback;
	spListv_okclicks(self->printers) = (1 << spListv_click);
	ZmlSetInstanceName(self->printers, "printer-list", self);

	spSend(self->options, m_spButtonv_insert,
	       spToggle_Create(catgets(catalog, CAT_LITE, 364, "Standard Message Headers"), 0, 0, !!alwaysignore),
	       STANDARDHDRS_B);
	spSend(self->options, m_spButtonv_insert,
	       spToggle_Create(catgets(catalog, CAT_LITE, 365, "All Message Headers"), 0, 0, !alwaysignore),
	       ALLHEADERS_B);
	spSend(self->options, m_spButtonv_insert,
	       spToggle_Create(catgets(catalog, CAT_LITE, 366, "Message Body Only"), 0, 0, 0),
	       BODYONLY_B);

	spButtonv_style(self->options) = spButtonv_vertical;
	spButtonv_toggleStyle(self->options) = spButtonv_checkbox;
	spButtonv_callback(self->options) = optionsActivate;

	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 368, "Print"), aa_print,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	ZmlSetInstanceName(dialog_actionArea(self), "print-aa", self);

	self->cmdwrap = spWrapview_NEW();
	spSend(self->cmdwrap, m_spWrapview_setView, self->command);

	wrap = spWrapview_NEW();
	spWrapview_boxed(wrap) = 1;
	spSend(wrap, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 370, "Print Message"), spWrapview_top);
	spSend(wrap, m_spWrapview_setView, self->options);

	wrap2 = spWrapview_NEW();
	spWrapview_boxed(wrap2) = 1;
	spSend(wrap2, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 371, "Printers"), spWrapview_top);
	spSend(wrap2, m_spWrapview_setView, self->printers);

	spSend(wrap, m_spView_desiredSize, &minh, &minw, &maxh, &maxw,
	       &besth, &bestw);

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 372, "Printer"), spWrapview_top);
	spSend(self, m_dialog_setView,
	       Split(Split(wrap,
			   wrap2,
			   bestw, 0, 0,
			   spSplitview_leftRight,
			   spSplitview_plain, 0),
		     self->cmdwrap,
		     1, 1, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, dialog_messages(self));
    spSend(self, m_dialog_addFocusView, self->options);
    spSend(self, m_dialog_addFocusView, self->printers);
    spSend(self, m_dialog_addFocusView, self->command);
}

static void
printdialog_finalize(self)
    struct printdialog *self;
{
    /* Undo everything! */
}

static void
printdialog_activate(self, arg)
    struct printdialog *self;
    spArgList_t arg;
{
    int i;
    char *tmp, **printers;

    spSend(self, m_dialog_setmgroup,
	   (msg_group *) spSend_p(spIm_view(ZmlIm), m_dialog_mgroup));
    spButtonv_selection(dialog_actionArea(self)) = 1; /* the Print button */
    spSuper(printdialog_class, self, m_dialog_activate);
    spSend(dialog_actionArea(self), m_spView_wantFocus,
	   dialog_actionArea(self));

    if (!(tmp = get_var_value(VarPrinter))) {
	tmp = DEF_PRINTER;
    }
    spSend(spView_observed(self->printers), m_spText_clear);
    if (!tmp || !*tmp) {
	printers = unitv("");
    } else if (!(printers = strvec(tmp, ", \t", 1))) {
	RAISE(strerror(ENOMEM), "printdialog_activate");
    }
    for (i = 0; printers[i] && printers[i][0]; ++i) {
	spSend(spView_observed(self->printers), m_spList_append,
	       printers[i]);
    }
    free_vec(printers);
    if (tmp = get_var_value(VarPrintCmd)) {
	self->namep = 0;
	spSend(self->cmdwrap, m_spWrapview_setLabel,
	       catgets(catalog, CAT_LITE, 373, "Print Command: "), spWrapview_left);
	spSend(spView_observed(self->command), m_spText_clear);
	spSend(spView_observed(self->command), m_spText_insert,
	       0, strlen(tmp), tmp, spText_mBefore);
    } else {
	int r = spListv_lastclick(self->printers);

	self->namep = 1;
	spSend(self->cmdwrap, m_spWrapview_setLabel,
	       catgets(catalog, CAT_LITE, 374, "Printer Name: "), spWrapview_left);
	spSend(spView_observed(self->command), m_spText_clear);
	if ((r >= 0)
	    && intset_Contains(spListv_selections(self->printers), r)) {
	    struct dynstr d;

	    dynstr_Init(&d);
	    spSend(spView_observed(self->printers),
		   m_spList_getNthItem, r, &d);
	    spSend(spView_observed(self->command), m_spText_insert,
		   0, dynstr_Length(&d), dynstr_Str(&d), spText_mBefore);
	    dynstr_Destroy(&d);
	}
    }
    if (i > 0)
	spSend(self->printers, m_spView_invokeInteraction,
	       "list-click-line", 0, "1", 0);
}

static void
printdialog_setmgroup(self, arg)
    struct printdialog *self;
    spArgList_t arg;
{
    msg_group *new = spArg(arg, msg_group *);

    msg_group_combine(&(self->mg), MG_SET, new);
    spSuper(printdialog_class, self, m_dialog_setmgroup, new);
}

static msg_group *
printdialog_mgroup(self, arg)
    struct printdialog *self;
    spArgList_t arg;
{
    return (&(self->mg));
}

static void
printdialog_desiredSize(self, arg)
    struct printdialog *self;
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

    spSuper(printdialog_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    *bestw = screenw - 10;
}

struct spWidgetInfo *spwc_Print = 0;

void
printdialog_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (printdialog_class)
	return;
    printdialog_class =
	spWclass_Create("printdialog", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct printdialog)),
			printdialog_initialize,
			printdialog_finalize,
			spwc_Print = spWidget_Create("Print",
						     spwc_Popup));

    spoor_AddOverride(printdialog_class,
		      m_spView_desiredSize, NULL,
		      printdialog_desiredSize);
    spoor_AddOverride(printdialog_class,
		      m_dialog_mgroup, NULL,
		      printdialog_mgroup);
    spoor_AddOverride(printdialog_class,
		      m_dialog_setmgroup, NULL,
		      printdialog_setmgroup);
    spoor_AddOverride(printdialog_class, m_dialog_activate, NULL,
		      printdialog_activate);

    spTextview_InitializeClass();
    spCmdline_InitializeClass();
    spButtonv_InitializeClass();
    spText_InitializeClass();
    spButton_InitializeClass();
    spToggle_InitializeClass();
    spPopupView_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
