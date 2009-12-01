/* m_comp2.c	Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_comp2_rcsid[] =
    "$Id: m_comp2.c,v 2.61 1995/10/05 05:17:06 liblit Exp $";
#endif

/* This file contains the compose options frame.  This thing is set up to
 * allow the user to change most options associated with composing a message
 * and sending mail.  Almost everything here is a toggle button; each toggle
 * has a single callback routine (toggle_callback()).  A data structure has
 * been set up to be passed as the client data to the one callback routine.
 * Nothing in the Compose structure is actually changed till the user is
 * ready to send. (actually does send, that is)
 *
 * The CompChoice data structure is used solely by the callback routines
 * in this file.  The idea is that all the of configurable options for
 * how to send mail are all stored as bitfields and/or text strings in
 * the compose structure.  The user interface has widgets that allow the
 * user to configure these options.  When the user sends mail, do_send()
 * calls us, we query the user interface and set the appropriate bits
 * and fields of the compose structure accordingly.
 */

#include "zmail.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "dynaPrompt.h"
#include "linklist.h"
#include "m_comp.h"
#include "xm/sanew.h"
#include "zm_motif.h"
#include "zmopt.h"

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

typedef struct {
    char *w_name;	/* widget name for app-defaults */
    int idx[2];         /* index into the comp_items array.  1st item is
			 * here in options dialog.  The second item is also
			 * here.  Used to get string value */
    u_long bit;		/* bit set in one of the compose flag words */
    int flagoff;	/* offset in compse struct of the flag word */
    char *var;		/* variable that contains default string value */
} CompChoice;

#undef OffOf
#define OffOf(f) XtOffsetOf(Compose,f)

#define OffToULong(c,o)	(*((u_long *)(((char *)(c))+(o))))

#define ChoiceIsOn(c,ch,ix) \
	    ison(OffToULong((c),(ch)[ix].flagoff), (ch)[ix].bit)
#define ChoiceIsOff(c,ch,ix) \
	    isoff(OffToULong((c),(ch)[ix].flagoff), (ch)[ix].bit)

#define ChoiceTurnOn(c,ch,ix) \
	    turnon(OffToULong((c),(ch)[ix].flagoff), (ch)[ix].bit)
#define ChoiceTurnOff(c,ch,ix) \
	    turnoff(OffToULong((c),(ch)[ix].flagoff), (ch)[ix].bit)


#define ChoiceItemIx(ch,ix) ((ch)[ix].idx[0])
#define ChoiceTextIx(ch,ix) ((ch)[ix].idx[1])

#define ChoiceItem(it,ch,ix) (it)[ChoiceItemIx(ch,ix)]
#define ChoiceText(it,ch,ix) (it)[ChoiceTextIx(ch,ix)]

CompChoice form_items[] = {
    { "record",
	{ COMP_RECORDING2, COMP_RECORDING_TXT },
	RECORDING, OffOf(send_flags), "record" },
    { "logfile",
	{ COMP_LOGGING, COMP_LOGGING_TXT },
	LOGGING, OffOf(send_flags), "logfile" },
};

CompChoice comp_choices[] = {
    { "autosign",
	{ COMP_AUTOSIGN2, -1 },
	SIGN, OffOf(send_flags) },
    { "autoformat",
	{ COMP_AUTOFORMAT2, -1 },
	AUTOFORMAT, OffOf(flags) },
    { "return-receipt",
	{ COMP_RECEIPT2, -1 },
	RETURN_RECEIPT, OffOf(send_flags) },
#ifndef ZMAIL_BASIC
    { "edit-hdrs",
	{ COMP_EDIT_HDRS2, -1 },
	EDIT_HDRS, OffOf(flags) },
#endif /* !ZMAIL_BASIC */
    { "verbose",
	{ COMP_VERBOSE, -1 },
	VERBOSE, OffOf(send_flags) },
    { "synchronous",
	{ COMP_SYNCHRONOUS, -1},
	SYNCH_SEND, OffOf(send_flags) },
    { "autoclear",
	{ COMP_AUTOCLEAR, -1 },
	AUTOCLEAR, OffOf(flags) },
    { "record-user",
	{ COMP_RECORDUSER, -1 },
	RECORDUSER, OffOf(send_flags) },
#ifdef DSERV
    { "address_sort",
	{ COMP_SORTER2, -1 },
	SORT_ADDRESSES, OffOf(flags) },
    { "address_book",
	{ COMP_DIRECTORY2, -1 },
	DIRECTORY_CHECK, OffOf(flags) },
    { "sendtime_check",
	{ COMP_SENDCHECK2, -1 },
	SENDTIME_CHECK, OffOf(flags) },
    { "confirm_send",
	{ COMP_VERIFY, -1 },
	CONFIRM_SEND, OffOf(flags) }
#endif /* DSERV */
};

static void
toggle_callback(toggle, choice, cbs)
Widget toggle;
CompChoice *choice;
XmToggleButtonCallbackStruct *cbs;
{
    Compose *compose = (Compose *) FrameGetClientData(FrameGetData(toggle));
    
    /* Grody special case */
    if (ChoiceItemIx(choice, 0) == COMP_RECEIPT2) {
	gui_request_receipt(compose, cbs->set, "");
	return;
    }

    if (cbs->set)
	ChoiceTurnOn(compose, choice, 0);
    else
	ChoiceTurnOff(compose, choice, 0);
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
}

void
reset_comp_opts(compose, notify)
Compose *compose;
int notify;
{
    int i;
    Widget w;

    /* if this frame never popped up, just bail.... */
    if (!compose->interface->options)
	return;

    for (i = 0; i < XtNumber(comp_choices); i++)
	XmToggleButtonSetState(ChoiceItem(compose->interface->comp_items, comp_choices, i),
	    ChoiceIsOn(compose, comp_choices, i), notify);

    for (i = 0; i < XtNumber(form_items); i++) {
	if (form_items[i].idx[1] != -1) {
	    /* XXX -- HACK! */
	    switch (ChoiceItemIx(form_items, i)) {
		case COMP_LOGGING:
		    w = ChoiceText(compose->interface->comp_items,
				    form_items, i);
		    SetTextString(w, compose->logfile);
		    XtSetSensitive(XtParent(w),
				    ChoiceIsOn(compose, form_items, i));
		when COMP_RECORDING2:
		    w = ChoiceText(compose->interface->comp_items,
				    form_items, i);
		    SetTextString(w, compose->record);
		    XtSetSensitive(XtParent(w),
				    ChoiceIsOn(compose, form_items, i));
#ifdef NOT_NOW
		when COMP_AUTOSIGN:
		    w = ChoiceText(compose->interface->comp_items,
				    form_items, i);
		    SetTextString(w, compose->signature);
		    XtSetSensitive(XtParent(w),
				    ChoiceIsOn(compose, form_items, i));
#endif /* NOT_NOW */
		otherwise: ; /* ?? */
	    }
	}
	XmToggleButtonSetState(ChoiceItem(compose->interface->comp_items,
					    form_items, i),
	    ChoiceIsOn(compose, form_items, i), notify);
    }
}

void
update_comp_struct(compose)
Compose *compose;
{
    int i;

    /* if this frame never popped up, just bail.... */
    if (!compose->interface->options)
        return;

    for (i = 0; i < XtNumber(form_items); i++) {
        if (ChoiceIsOn(compose, form_items, i)) {
            if (form_items[i].idx[1] != -1) {
		char **compval;
		char *text = XmTextGetString(ChoiceText(compose->interface->comp_items, form_items, i));
                /* XXX -- HACK! */
		if (text && *text) {
		    switch (ChoiceItemIx(form_items, i)) {
			case COMP_LOGGING: compval = &(compose->logfile);
			when COMP_RECORDING2: compval = &(compose->record);
#ifdef NOT_NOW
			when COMP_AUTOSIGN: compval = &(compose->signature);
#endif /* NOT_NOW */
			otherwise: compval = 0;	/* Impossible? */
		    }
		    if (compval) {
			xfree(*compval);
			*compval = savestr(text);
		    }
		}
		XtFree(text);
            }
        }
    }
}

static void
toggle_text(w, text_w, cbs)
Widget w, text_w;
XmToggleButtonCallbackStruct *cbs;
{
    XtSetSensitive(XtParent(text_w), cbs->set);
}

static ActionAreaItem comp_opts[] = {
    { "Close",   PopdownFrameCallback,	NULL },
    { NULL,	 (void_proc)0,		NULL },
    { NULL,	 (void_proc)0,		NULL },
    { "Help",    DialogHelp,    "Compose Options" }
};

static void
comp_opts_cb(frame, data)
ZmFrame frame;
ZmCallbackData data;
{
    Compose *compose = FrameComposeGetComp(frame);

    if (compose)
	reset_comp_opts(compose, False);
}

ZmFrame
DialogCreateCompOptions(junk)
Widget junk;
{
    ZmFrame newframe, dad;
    extern ZcIcon options_icon;
    Widget shell, pane, rc = 0, form, w, xmframe, *items;
    Compose *compose;
    int i;

    shell = GetTopShell(ask_item);
    dad = FrameGetData(shell);
    if (FrameGetType(dad) != FrameCompose) {
	error(UserErrWarning,
	    catgets(
		catalog, CAT_MOTIF, 894, "CompOptions dialog must be created from a Compose window!"));
	return (ZmFrame)NULL;
    }
    compose = (Compose *)FrameGetClientData(dad);

    if (compose->interface->options) {
	newframe = compose->interface->options;
	FrameSet(newframe, FrameClientData, compose, FrameEndArgs);
	FramePopup(newframe);
	return (ZmFrame) 0;
    }

    newframe = FrameCreate("comp_opts_dialog",
	FrameCompOpts,	shell,
	FrameIcon,	&options_icon,
#ifdef NOT_NOW
	FrameTitle,	"Compose Options",
#endif /* NOT_NOW */
	FrameChild,	&pane,
	FrameRefreshProc,generic_frame_refresh,
	FrameFlags,	FRAME_SHOW_ICON | FRAME_CANNOT_RESIZE |
			FRAME_DIRECTIONS,
	FrameClientData,compose, /* client data...JUST LIKE DAD */
	FrameEndArgs);
    compose->interface->options = newframe;
    items = compose->interface->comp_items;

    xmframe = XtCreateManagedWidget(NULL, xmFrameWidgetClass, pane, NULL, 0);

    form = XtCreateWidget(NULL, xmFormWidgetClass, xmframe, NULL, 0);
    for (i = 0; i < XtNumber(form_items); i++) {

	rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, form,
	    XmNorientation, XmHORIZONTAL,
	    XmNtopAttachment, i == 0? XmATTACH_FORM : XmATTACH_WIDGET,
	    XmNtopWidget, rc,
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);
	w = XtVaCreateManagedWidget(NULL,
	    xmToggleButtonWidgetClass, rc,
	    XmNlabelString,	zmXmStr(" "),
	    XmNset,		ChoiceIsOn(compose, form_items, i),
	    NULL);
	ChoiceItem(items, form_items, i) = w;
	XtAddCallback(w, XmNvalueChangedCallback,
		      (XtCallbackProc) toggle_callback, &form_items[i]);

	ChoiceText(items, form_items, i) =
	    CreateLabeledText(form_items[i].w_name, rc, NULL, True);
	if (ChoiceIsOff(compose, form_items, i))
	    XtSetSensitive(XtParent(ChoiceText(items, form_items, i)), False);
	XtAddCallback(w, XmNvalueChangedCallback, (XtCallbackProc) toggle_text,
	    ChoiceText(items, form_items, i));

#ifdef NOT_NOW /* Initialized by reset_comp_opts(), called below */
	w = ChoiceText(items, form_items, i);
	if (form_items[i].var) {
	    SetTextString(w, value_of(form_items[i].var));
	    /* These are all copied from the compose structure at the
	     * moment, and don't have anything to do with the value of
	     * the variable after this initialization step.
	     */
	    XtAddCallback(w, XmNdestroyCallback, remove_callback_cb,
			  ZmCallbackAdd(form_items[i].var, ZCBTYPE_VAR,
					comp_opts_cb, compose));
	}
#endif /* NOT_NOW */
	XtManageChild(rc);
    }
    XtManageChild(form);

    rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	XmNnumColumns,  XtNumber(comp_choices)/3,
	XmNpacking,	XmPACK_COLUMN,
	NULL);

    for (i = 0; i < XtNumber(comp_choices); i++) {

	w = XtVaCreateManagedWidget(comp_choices[i].w_name,
		xmToggleButtonWidgetClass,	rc,
		XmNset,	ChoiceIsOn(compose, comp_choices, i),
		NULL);
	ChoiceItem(items, comp_choices, i) = w;
	XtAddCallback(w, XmNvalueChangedCallback,
		      (XtCallbackProc) toggle_callback, &comp_choices[i]);
    }
    XtAddCallback(rc, XmNdestroyCallback, remove_callback_cb,
		  ZmCallbackAdd("compose_state", ZCBTYPE_VAR,
				comp_opts_cb, newframe));
    XtManageChild(rc);

    CreateActionArea(pane, comp_opts, XtNumber(comp_opts), "Compose Options");

    XtManageChild(pane);
    FrameCopyContext(dad, newframe);
    reset_comp_opts(compose, False);
    FramePopup(newframe);
    return (ZmFrame) 0;
}

static void
add_alias(w, which)
Widget w;
int which;
{
    Compose *compose =
	(Compose *)FrameGetClientData(FrameGetFrameOfParent(FrameGetData(w)));
    Widget *comp_items = compose->interface->comp_items;
    Widget list_w = compose->interface->alias_list;
    char *p;
    int i = 0, j;
    XmStringTable sel;

    XtVaGetValues(list_w,
	XmNselectedItems,     &sel,
	XmNselectedItemCount, &i,
	NULL);
    if (i == 0)
	return;

    p = savestr(zmVaStr("builtin compcmd %%%s %s ",
		    compose->link.l_name, address_headers[which]));
    for (j = 0; j < i; j++) {
	char *q, *t;
	XmStringGetLtoR(sel[j], xmcharset, &t);
	if (q = any(t, " \t"))
	    *q = 0;
	if (j)
	    p = strapp(&p, ", ");
	p = strapp(&p, quotezs(t, 0));
	XtFree(t);
    }
    ask_item = w;
    if (cmd_line(p, NULL_GRP) == 0) {
	Autodismiss(w, "alias");
	DismissSetWidget(w, DismissClose);
    }
}

static ActionAreaItem btns[] = {
    { "To",     add_alias,  (caddr_t) TO_ADDR  },
    { "Cc",     add_alias,  (caddr_t) CC_ADDR  },
    { "Bcc",    add_alias,  (caddr_t) BCC_ADDR },
    { DONE_STR, do_cmd_line, "dialog -close"   },
    { "Help",   DialogHelp,  "Using Aliases"   },
};

extern ZcIcon alias_icon;

ZmFrame
DialogCreateCompAliases(unused, dad_w)
Widget unused;
Widget dad_w;
{
    Arg args[10];
    Widget shell, pane, list_w;
    Compose *compose;
    ZmFrame dad, frame;

    shell = GetTopShell(dad_w);
    dad = FrameGetData(shell);
    if (FrameGetType(dad) != FrameCompose) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 101, 
		    "CompAliases dialog must be created from a Compose dialog!" ));
	return (ZmFrame)NULL;
    }
    compose = FrameComposeGetComp(dad);
    if (!compose) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MOTIF, 905, 
		    "No active composition in this window"));
	return (ZmFrame) 0;
    }

    if (compose->interface->alias_list) {
	update_list(&aliases);
	frame = FrameGetData(compose->interface->alias_list);
	FramePopup(frame);
	return (ZmFrame) 0;
    }

    frame = FrameCreate("comp_alias_dialog", FrameCompAliases, dad_w,
	FrameFlags,	 FRAME_CANNOT_SHRINK|FRAME_DIRECTIONS|FRAME_SHOW_ICON,
	FrameIcon,	 &alias_icon,
	FrameChild,	 &pane,
#ifdef NOT_NOW
	FrameTitle,	 "Aliases",
#endif /* NOT_NOW */
	/* FrameIconTitle,"Aliases",	/* It isn't iconifiable */
	FrameEndArgs);

#ifdef NOT_NOW
    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);
    FrameGet(frame, FrameIconPix, &pix, FrameEndArgs);
    label = XtVaCreateManagedWidget("directions",
		xmLabelGadgetClass, form,
		XmNleftAttachment,  XmATTACH_FORM,
		XmNtopAttachment,   XmATTACH_FORM,
		XmNalignment,	    XmALIGNMENT_BEGINNING,
		NULL);
    widget =
	XtVaCreateManagedWidget(alias_icon.var, xmLabelGadgetClass, form,
	    XmNlabelType,       XmPIXMAP,
	    XmNlabelPixmap,     pix,
	    XmNuserData,        &alias_icon,
	    XmNalignment,       XmALIGNMENT_END,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNtopAttachment,   XmATTACH_FORM,
	    NULL);
    XtVaSetValues(label,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		widget,
	NULL);
    FrameSet(frame,
	FrameFlagOn,	FRAME_SHOW_ICON,
	FrameIconItem,	widget,
	FrameEndArgs);
    XtManageChild(form);
#endif /* NOT_NOW */
    
    XtSetArg(args[0], XmNlistSizePolicy, XmCONSTANT);
    XtSetArg(args[1], XmNselectionPolicy, XmEXTENDED_SELECT);
    list_w = XmCreateScrolledList(pane, "alias_list", args, 2);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    XtAddCallback(list_w, XmNdefaultActionCallback,
		  (XtCallbackProc) add_alias, (caddr_t) TO_ADDR);
    ListInstallNavigator(list_w);
    XtManageChild(list_w);
    compose->interface->alias_list = list_w;

    update_list(&aliases);

    {
	Widget actionArea = CreateActionArea(pane, btns, XtNumber(btns), "Using Aliases");
	FrameSet(frame, FrameDismissButton, GetNthChild(actionArea, 3), FrameEndArgs);
    }

    XtManageChild(pane);
    FramePopup(frame);
    return (ZmFrame) 0;
}

void
gui_request_priority(compose, p)
Compose *compose;
const char *p;
{

    if (ison(compose->flags, EDIT_HDRS)) {
	timeout_cursors(TRUE);
	if (!SaveComposition(compose, False)) {
	    error(SysErrWarning, catgets( catalog, CAT_MOTIF, 81, "Cannot save to \"%s\"" ), compose->edfile);
	    timeout_cursors(FALSE);
	    return;
	}
	resume_compose(compose);
	if (reload_edfile() != 0) {
	    suspend_compose(compose);
	    timeout_cursors(FALSE);
	    return;
	}
    }
    request_priority(compose, p);
    if (ison(compose->flags, EDIT_HDRS)) {
	(void) prepare_edfile();	 /* If it fails, we use the old one */
	suspend_compose(compose);
	LoadComposition(compose);
	timeout_cursors(FALSE);
    }
}

void
gui_request_receipt(compose, on, p)
Compose *compose;
int on;
char *p;
{
    if (ison(compose->flags, EDIT_HDRS)) {
	timeout_cursors(TRUE);
	if (!SaveComposition(compose, False)) {
	    error(SysErrWarning, catgets( catalog, CAT_MOTIF, 81, "Cannot save to \"%s\"" ), compose->edfile);
	    timeout_cursors(FALSE);
	    return;
	}
	resume_compose(compose);
	if (reload_edfile() != 0) {
	    suspend_compose(compose);
	
    timeout_cursors(FALSE);
	    return;
	}
    }
    request_receipt(compose, on, p);
    if (ison(compose->flags, EDIT_HDRS)) {
	(void) prepare_edfile();	 /* If it fails, we use the old one */
	suspend_compose(compose);
	LoadComposition(compose);
	timeout_cursors(FALSE);
    }
}
