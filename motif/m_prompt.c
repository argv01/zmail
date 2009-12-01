/* motif_prompt.c     Copyright 1990, 1991 Z-Code Software Corp. */

/* routines that prompt the user.  gui_ask() asks a yes/no question and
 * provides for yes/no/cancel answers.  gui_error() posts a notice and the
 * user clicks Ok.  These routines post dialogs and do not return until
 * the user answered the question.
 * In each case, the dialog is created and destroyed after the question
 * is answered.  It is possible to reuse the dialogs in each case, but
 * there doesn't seem to be a need for this yet.
 */

#include "zmail.h"
#include "zmframe.h"	/* sigh */
#include "catalog.h"
#include "critical.h"
#include "cursors.h"
#include "except.h"
#include "config/features.h"
#include "finder.h"
#include "zm_motif.h"
#include <Xm/Xm.h>
#if XmVersion <= 1001
#include <Xm/VendorE.h>
#endif /* Motif 1.1 or earlier */
#include <Xm/DialogS.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/FileSB.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#ifndef va_dcl
#include <varargs.h>
#endif
#endif /* HAVE_STDARG_H */

static void zmPromptResponse(), blink_item(), blink_symbol(), check_passwd();
void zmModalPopdown();

int blink_quietly;

void
zmButtonAction(button, action)
Widget button;
char *action;
{
    XButtonEvent event;

    bzero((char *) &event, sizeof(event));
    event.button = 1;
    XtCallActionProc(button, action, (XEvent *) &event, DUBL_NULL, 0);
}

void
zmButtonClick(button)
Widget button;
{
    if (XtIsWidget(button))
	zmButtonAction(button, "ArmAndActivate");
#ifdef NOT_NOW
    /* Motif 1.1.3 broke this */
    else
	zmButtonAction(XtParent(button), "ManagerGadgetSelect");
#endif /* NOT_NOW */
    else {
	XmAnyCallbackStruct cbs;

	cbs.reason = XmCR_ACTIVATE;
	cbs.event = 0;
	zmButtonIn(button);
	XtCallCallbacks(button, XmNactivateCallback, &cbs);
	zmButtonOut(button);
    }
}

void
zmButtonIn(button)
Widget button;
{
    if (XtIsWidget(button))
	zmButtonAction(button, "Arm");
    else {
	XmAnyCallbackStruct cbs;

	cbs.reason = XmCR_ARM;
	cbs.event = 0;
	XtCallCallbacks(button, XmNarmCallback, &cbs);
    }
}

void
zmButtonOut(button)
Widget button;
{
    if (XtIsWidget(button))
	zmButtonAction(button, "Disarm");
    else {
	XmAnyCallbackStruct cbs;

	cbs.reason = XmCR_DISARM;
	cbs.event = 0;
	XtCallCallbacks(button, XmNdisarmCallback, &cbs);
    }
}

static void
RaiseThisWindowDammit(display, window)
Display *display;
Window window;
{
    Widget shell = GetTopShell(XtWindowToWidget(display, window));
    static char raising = 0;

    /* Bart: Fri Mar 26 16:41:38 PST 1993
     * This fixes the "battle with xlock" bug; the core dump appears
     * to have resulted from stack overflow by infinite recursion.
     */
    if (raising)	/* Don't do this recursively */
	return;
    raising = 1;

    if (shell != hidden_shell)
	XMapRaised(display, XtWindow(shell));
    XMapRaised(display, window);
    ForceExposes();

    raising = 0;
}

typedef struct {
    XtIntervalId id;
    u_long countdown, firstwarning, lastwarning, checktime;
    int maxrings, bellsrung;
    Widget dialog, response;
    void_proc showwarning;
} PromptCountdown;

static void
zmPromptTimeout(countdown)
PromptCountdown *countdown;
{
    countdown->countdown -= countdown->checktime;
    if (countdown->countdown <= 0) {
	zmButtonClick(countdown->response);
	return;
    }
    if (countdown->firstwarning > 0 &&
	    countdown->countdown <= countdown->firstwarning) {
	countdown->firstwarning -= countdown->checktime;
	if (countdown->bellsrung < countdown->maxrings) {
	    if (countdown->bellsrung == 0 && countdown->dialog)
		RaiseThisWindowDammit(display, XtWindow(countdown->dialog));
	    countdown->bellsrung += 1;
	    if (countdown->showwarning)
		(*(countdown->showwarning))(countdown->dialog,
					    countdown->bellsrung);
	    else
		bell();
	} else {
	    countdown->firstwarning = countdown->lastwarning;
	    countdown->lastwarning = countdown->bellsrung = 0;
	}
    }
    countdown->id =
      XtAppAddTimeOut(app, countdown->checktime,
		      (XtTimerCallbackProc) zmPromptTimeout, countdown);
}

static char keptUp = 0;

static void
zmKeepItUp(widget, unused, event)
Widget widget, unused;
XVisibilityEvent *event;
{
    if (event->state != VisibilityUnobscured) {
	/* 3 attempts somewhat arbitrarily chosen here:
	 * 1 if the dialog wasn't on top when first exposed;
	 * 1 more in case the dialog is covered up by the user;
	 * 1 for good measure, in case the dialog was covered up
	 *   by something else, like maybe xlock, or in case we
	 *   got two partially-obscured events or something ...
	 */
	if (keptUp > 3)
	    return;
	keptUp++;
	RaiseThisWindowDammit(event->display, event->window);
    }
}

static void
zmKeepDialogUp(dialog)
Widget dialog;
{
    keptUp = 0;
    XtAddEventHandler(dialog, VisibilityChangeMask, FALSE,
		      (XtEventHandler) zmKeepItUp, NULL);
}

#define COUNTDOWN_START	(u_long)60000
#define COUNTDOWN_FIRST	(COUNTDOWN_START/(u_long)2)
#define COUNTDOWN_LAST	(COUNTDOWN_FIRST/(u_long)5)
#define COUNTDOWN_STEP	(u_long)500
#define COUNTDOWN_RINGS	4

void
zmPromptPopdown(dialog, dflt_button, answer)
Widget dialog, dflt_button;
AskAnswer *answer;
{
    PromptCountdown countdown;
#ifdef TIMER_API
    CRITICAL_BEGIN {
#else /* !TIMER_API */
	XtIntervalId do_timer = xt_timer;
	
	if (xt_timer) {
	    XtRemoveTimeOut(xt_timer);
	    xt_timer = 0;
	}
#endif /* TIMER_API */
	
	/* Wait at most 60 seconds for a response, then return default */
	countdown.response = dflt_button;
	countdown.countdown = COUNTDOWN_START;
	countdown.firstwarning = COUNTDOWN_FIRST;
	countdown.lastwarning = COUNTDOWN_LAST;
	countdown.checktime = COUNTDOWN_STEP;
	countdown.maxrings = COUNTDOWN_RINGS;
	countdown.bellsrung = 0;
	countdown.dialog = dialog;
	countdown.showwarning = blink_symbol;
	countdown.id =
	    XtAppAddTimeOut(app, countdown.checktime,
			    (XtTimerCallbackProc) zmPromptTimeout, &countdown);
	
	/* Bart: Thu May 27 18:48:52 PDT 1993 -- AT&T paid for this hack. */
	blink_quietly = chk_option(VarQuiet, "blink");
	
	*answer = AskUnknown;
	
	/* while the user hasn't provided an answer, simulate XtMainLoop.
	 * The answer changes as soon as the user selects one of the
	 * buttons and the callback routine changes its value.  Don't
	 * break loop until XtPending() also returns False to assure
	 * widget destruction.
	 */
	zmKeepDialogUp(dialog);
	while (*answer == AskUnknown)
	    XtAppProcessEvent(app, XtIMAll);

	XtRemoveTimeOut(countdown.id);
	
	XSync(display, 0);
	XmUpdateDisplay(tool);
	blink_symbol(dialog, 0);
	
	/* Serious problems with bugs here.
	 *
	 * X11R5 fixes a bug with Xt that makes DestroyWidget unsafe when
	 * a nested event loop is running.
	 *
	 * Destroying the dialog itself tickles a motif bug that may cause
	 * gadgets to be RemoveGrabbed after they have been destroyed.
	 * Destroying the TopShell may cause an attempt to pop non-existant
	 * ext data.  RemoveGrab is non-fatal, mostly. so live with that.
	 */
#ifdef NOT_NOW
	ZmXtDestroyWidget(dialog);
#endif /* NOT_NOW */
	XtPopdown(GetTopShell(dialog));	/* LEAK */
	
#ifdef TIMER_API
    } CRITICAL_END;
#else /* !TIMER_API */
    if (do_timer)
	xt_timer = XtAppAddTimeOut(app, (u_long)0, gui_check_mail, NULL);
#endif /* TIMER_API */
}

/*
 * At least one of yesStr, noStr, cancelStr must be provided.
 */
void
zmPromptPopup(reason,parent,message,yesStr,noStr,cancelStr,answer)
PromptReason reason;
Widget parent;
const char *message, *yesStr, *noStr, *cancelStr;
AskAnswer *answer;
{
    Widget dialog, button, (*func)();
    char *name, *title = NULL;

    switch((int)reason) {
	case SysErrFatal:
	case UserErrFatal :
	case ZmErrFatal :
	case ZmErrWarning :
	case SysErrWarning :
	    title = catgets( catalog, CAT_MOTIF, 434, " Error " );
	    name = "error_dialog";
	    func = XmCreateWarningDialog;
	    break;

	case QueryWarning :
	    title = catgets( catalog, CAT_MOTIF, 435, " Warning " );
	    name = "warning_dialog";
	    func = XmCreateWarningDialog;
	    break;

	case UserErrWarning :
	    title = catgets( catalog, CAT_MOTIF, 435, " Warning " );
	    name = "warning_dialog";
	    func = XmCreateErrorDialog;
	    break;

	case QueryChoice :
	    title = catgets( catalog, CAT_MOTIF, 437, " Choice " );
	    name = "question_dialog";
	    func = XmCreateQuestionDialog;
	    break;

	case HelpMessage :
	    title = catgets( catalog, CAT_MOTIF, 438, " Help " );
	    name = "help_dialog";
	    func = XmCreateInformationDialog;
	    break;

	default: /* Message and UrgentMessage */
	    title = catgets( catalog, CAT_MOTIF, 439, " Message " );
	    name = "message_dialog";
	    func = XmCreateMessageDialog;
    }

    dialog = (*func)(parent, name, NULL, 0);
    if (yesStr) {
	XtVaSetValues(dialog, XmNokLabelString, zmXmStr(yesStr), NULL);
	XtAddCallback(dialog, XmNokCallback, zmPromptResponse, answer);
    } else
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON));
    if (noStr) {
	XtVaSetValues(dialog, XmNcancelLabelString, zmXmStr(noStr), NULL);
	XtAddCallback(dialog, XmNcancelCallback, zmPromptResponse, answer);
    } else
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_CANCEL_BUTTON));
    if (cancelStr) {
	XtVaSetValues(dialog, XmNhelpLabelString, zmXmStr(cancelStr), NULL);
	XtAddCallback(dialog, XmNhelpCallback, zmPromptResponse, answer);
    } else
	XtUnmanageChild(XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
    SetDeleteWindowCallback(XtParent(dialog), zmPromptResponse, answer);
    button = XmMessageBoxGetChild(dialog,
		*answer == AskYes || *answer == WarnYes || *answer == WarnOk || *answer == SendSplit?
		    XmDIALOG_OK_BUTTON :
		*answer == AskNo || *answer == WarnNo || *answer == SendWhole?
		    XmDIALOG_CANCEL_BUTTON :
	     /* *answer == AskCancel || *answer == WarnCancel */
		    XmDIALOG_HELP_BUTTON),
    XtVaSetValues(dialog,
	XmNdialogStyle,     XmDIALOG_FULL_APPLICATION_MODAL,
	XmNmessageString,   zmXmStr(message),
	XmNdefaultPosition, False,
	XmNdefaultButton,   button,
	NULL);
    XtAddCallback(dialog, XmNmapCallback, (XtCallbackProc) place_dialog, ask_item);
    XtAddCallback(dialog, XmNmapCallback, (XtCallbackProc) fix_olwm_decor,
	(XtPointer)(WMDecorationHeader|WMDecorationPushpin));
    if (title)
	XtVaSetValues(dialog, XmNdialogTitle, zmXmStr(title), NULL);
    XtManageChild(dialog);

#if defined(DVX)
    XMapRaised(display, dialog);
#endif /* DVX */
    zmPromptPopdown(dialog, button, answer);
}

AskAnswer
gui_ask(dflt, question)
AskAnswer dflt;
const char *question;
{
    Widget shell;

    find_good_ask_item();
    shell = GetTopShell(ask_item);

    if (shell != hidden_shell) {
	/* Must make sure it's visible (for aesthetic reasons, not for Xt) */
	(void) NormalizeShell(shell);
#ifdef NOT_NOW
	XtPopup(shell, XtGrabNone);
	XMapRaised(display, XtWindow(shell));
#endif /* NOT_NOW */
    }

    timeout_cursors(True);	/* So we can call it to reset cursors */
    assign_cursor(frame_list, do_not_enter_cursor);

    if (!is_manager(ask_item) && !chk_option(VarQuiet, "blink"))
	blink_item(True);

    switch ((int)dflt) {
	case WarnYes: case WarnNo:
	    zmPromptPopup(QueryWarning, shell, question,
			  catgets( catalog, CAT_MOTIF, 440, "Yes" ),
			  catgets( catalog, CAT_MOTIF, 441, "No" ),
			  NULL,
			  &dflt);

	when WarnOk: case WarnCancel:
	    zmPromptPopup(QueryWarning, shell, question,
			  catgets( catalog, CAT_MOTIF, 442, "Ok" ),
			  NULL,
			  catgets( catalog, CAT_MOTIF, 443, "Cancel" ),
			  &dflt);
	when AskOk:
	    zmPromptPopup(QueryChoice, shell, question,
			  catgets( catalog, CAT_MOTIF, 440, "Yes" ),
			  catgets( catalog, CAT_MOTIF, 441, "No" ),
			  NULL,
			  &dflt);

#ifdef PARTIAL_SEND
	when SendSplit: case SendWhole:
	    zmPromptPopup(QueryChoice, shell, question,
			  catgets(catalog, CAT_MOTIF, 920, "Send Split"),
			  catgets(catalog, CAT_MOTIF, 921, "Send Whole"),
			  catgets( catalog, CAT_MOTIF, 443, "Cancel" ),
			  &dflt);
#endif /* PARTIAL_SEND */

	otherwise:
	    zmPromptPopup(QueryChoice, shell, question,
			  catgets( catalog, CAT_MOTIF, 440, "Yes" ),
			  catgets( catalog, CAT_MOTIF, 441, "No" ),
			  catgets( catalog, CAT_MOTIF, 443, "Cancel" ),
			  &dflt);
    }

    if (!is_manager(ask_item) && !chk_option(VarQuiet, "blink"))
	blink_item(False);

    assign_cursor(frame_list, please_wait_cursor);
    timeout_cursors(False);	/* Reset cursors if they need resetting */

    return dflt;
}

void
gui_error(reason, message)
PromptReason reason;
const char *message;
{
    Widget shell;
    AskAnswer answer = AskNo;	/* Use No postion as the button */
    char *button =
	    reason == SysErrFatal ||
	    reason == ZmErrFatal ||
	    reason == UserErrFatal?    catgets( catalog, CAT_MOTIF, 449, "Bye" ) :
	    reason == SysErrWarning ||
	    reason == ZmErrWarning ||
	    reason == UserErrWarning?  catgets( catalog, CAT_MOTIF, 442, "Ok" ) :
	 /* reason == QueryChoice ||
	    reason == QueryWarning ||
	    reason == HelpMessage ||
	    reason == UrgentMessage ||
	    reason == Message */       catgets( catalog, CAT_MOTIF, 442, "Ok" );
    Boolean hidden = False;

    find_good_ask_item();
    shell = GetTopShell(ask_item);
    if (shell == tool && reason != UrgentMessage &&
	    (hidden = !window_is_visible(shell)) &&
	    (reason == UserErrWarning ||
	    reason == HelpMessage ||
	    reason == Message))
	shell = hidden_shell;

    /* If the ask_item is iconified or withdrawn, don't bother popping up unless
     * the message is urgent.  Just print to the display window ... (it may
     * not be visible (toggleable the "Windows" menu), but so it goes...
     * If reason is ForcedMessage then we both wprint() and, if the output
     * area is not managed, pop up a dialog.
     *
    if ((!XtIsRealized(shell) || !XtIsManaged(ask_item)) && ask_item != tool &&
     */
    if (reason == ForcedMessage || shell != tool && shell != hidden_shell &&
	    (reason == SysErrWarning ||
	    reason == ZmErrWarning || reason == UserErrWarning ||
	    reason == Message || reason == HelpMessage) &&
	    (hidden || (hidden = !window_is_visible(ask_item)))) {
	wprint("%s\n", message);
	if (reason != ForcedMessage || chk_option(VarMainPanes, "output"))
	    return;
	else
	    ask_item = shell = hidden_shell;
    }

    if (shell != hidden_shell) {
	/* Must make sure it's visible (for aesthetic reasons, not for Xt) */
	(void) NormalizeShell(shell);
#ifdef NOT_NOW
	XtPopup(shell, XtGrabNone);
	XMapRaised(display, XtWindow(shell));
#endif /* NOT_NOW */
    }

    timeout_cursors(True);	/* So we can call it to reset cursors */
    assign_cursor(frame_list, do_not_enter_cursor);

    if (!is_manager(ask_item) && !chk_option(VarQuiet, "blink"))
	blink_item(True);

    zmPromptPopup(reason, shell, message, NULL, button, NULL, &answer);

    if (!is_manager(ask_item) && !chk_option(VarQuiet, "blink"))
	blink_item(False);

    assign_cursor(frame_list, please_wait_cursor);
    timeout_cursors(False);	/* Reset cursors if they need resetting */
}

/* zmPromptResponse() --The user made some sort of response to the
 * question posed in gui_ask() or gui_error().  Set the answer (client_data)
 * accordingly and remove the dialog.
 */
static void
zmPromptResponse(w, answer, cbs)
Widget w;
AskAnswer *answer;
XmAnyCallbackStruct *cbs;
{
    /* check if called from catch_del() from DeleteWindowCallback... */
    if (!cbs)
	*answer = AskCancel;
    else switch (cbs->reason) {
        case XmCR_OK:
            *answer = AskYes;
        when XmCR_CANCEL:
            *answer = AskNo;
        when XmCR_HELP:
            *answer = AskCancel;
        otherwise:
            return;
    }
    XtUnmanageChild(w);
}

#include "bitmaps/bang0.xbm"
#include "bitmaps/bang1.xbm"

ZcIcon explosions[] = {
    { "bang0_icon", 0, bang0_width, bang0_height, bang0_bits },
    { "bang1_icon", 0, bang1_width, bang1_height, bang1_bits }
};

Pixmap bang_pixmaps[2];

static void
blink_symbol(dialog, state)
Widget dialog;
int state;
{
    if (state == 0) {
	if (bang_pixmaps[0]) {
	    XFreePixmap(display, bang_pixmaps[0]);
	    bang_pixmaps[0] = 0;
	}
	if (bang_pixmaps[1]) {
	    XFreePixmap(display, bang_pixmaps[1]);
	    bang_pixmaps[1] = 0;
	}
	return;
    }

    if (!bang_pixmaps[0]) {
	Debug("loading icons ...\n" );
	load_icons(dialog, explosions, XtNumber(explosions), bang_pixmaps);
	XFreePixmap(display, explosions[0].pixmap);
	XFreePixmap(display, explosions[1].pixmap);
	explosions[0].pixmap = explosions[1].pixmap = 0;
    }

    if (blink_quietly)
	return;

    if (bang_pixmaps[state & 1])
	XtVaSetValues(dialog,
	    XmNsymbolPixmap, bang_pixmaps[state & 1],
	    NULL);
    bell();
}

static void
blink_item(repeat)
int repeat;
{
    static int on = -1;
    static Pixel old_top, old_bottom;
    static XtIntervalId id;
    static u_long blink_time;

    if (on == -1) {
	char *bt = value_of(VarBlinkTime);
	if (!bt || (blink_time = atoi(bt)) <= 100 || blink_time > 1000)
	    blink_time = (u_long)500;
	XtVaGetValues(ask_item,
	    XmNtopShadowColor,  &old_top,
	    XmNbottomShadowColor,  &old_bottom,
	    NULL);
    }
    if (repeat) {
	on = !on;
	XtVaSetValues(ask_item,
	    XmNtopShadowColor,  on? old_top : old_bottom,
	    XmNbottomShadowColor, on? old_bottom : old_top, 
	    NULL);
	id = XtAppAddTimeOut(app, blink_time,
			     (XtTimerCallbackProc) blink_item,
			     (XtPointer) True);
    } else {
	XtVaSetValues(ask_item,
	    XmNtopShadowColor,  old_top,
	    XmNbottomShadowColor, old_bottom, 
	    NULL);
	XtRemoveTimeOut(id);
	on = -1;
    }
}

static void
prompt_done(dialog, file, cbs)
Widget dialog;
char **file;
XmSelectionBoxCallbackStruct *cbs;
{
    if (!cbs) {
	*file = (char *)1;
	return;
    }
    if (cbs->reason == XmCR_NO_MATCH) {
	Widget old_ask = ask_item;
	ask_item = XmSelectionBoxGetChild(dialog, XmDIALOG_TEXT);
	error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 453, "You must select one of the choices in the list." ));
	ask_item = old_ask;
	return;
    }
    if (cbs->reason == XmCR_CANCEL || cbs->reason == XmCR_HELP)
	*file = (char *)1;
    else
	XmStringGetLtoR(cbs->value, xmcharset, file);
}

/* user has selected a message -- extract the number from the header
 * summary and stuff into "sel_list" (the return value for PromptBox).
 * This function allocated data for sel_list, but PromptBox expects that.
 */
static void
list_done(dialog, sel_list, cbs)
Widget dialog;
char **sel_list; /* the selected items converted to a msg_list */
XmSelectionBoxCallbackStruct *cbs;
{
    char *p;
    int i;

    if (!cbs || cbs->reason == XmCR_CANCEL ||
	    !XmStringGetLtoR(cbs->value, xmcharset, sel_list)) {
	*sel_list = (char *)1;
	return;
    }
    p = *sel_list;
    while (*p && !isdigit(*p)) /* skip the spaces; get to the msg number */
	p++;
    (void) my_atoi(p, &i); /* get the number into i,  */
    sprintf(*sel_list, "%d", i); /* overwrite data with msg number and null */
}

static void
TryAgainUsed(dialog, calldata, cbs)
Widget dialog;
void **calldata;
XmSelectionBoxCallbackStruct *cbs;
{
    char **file = (char **)(calldata[0]);
    void (*done_func)() = (void (*)())(calldata[1]);
    AskAnswer *retval = (AskAnswer *)(calldata[2]);

    xfree(calldata);	/* XXX Should this really be done here?? */
    *retval = AskNo;
    (*done_func)(dialog, file, cbs);
}

static void
SkipItUsed(dialog, calldata, cbs)
Widget dialog;
char **calldata;
XmSelectionBoxCallbackStruct *cbs;
{
    char **file = (char **)(calldata[0]);
    AskAnswer *retval = (AskAnswer *)(calldata[2]);

    xfree(calldata);	/* XXX Should this really be done here?? */
    *file = Xt_savestr("");
}

static void
FilePromptBox(dialog, file, cbs)
Widget dialog;
char **file;
XmSelectionBoxCallbackStruct *cbs;
{
    char *query, *dflt;

    XmStringGetLtoR(cbs->value, xmcharset, &dflt);

    XtVaGetValues(dialog, XmNuserData, &query, NULL);
    XtUnmapWidget(GetTopShell(dialog));
    *file = PromptBox(dialog, query, dflt, NULL, 0, PB_FILE_BOX, 0); 
    if (!*file)
	*file = (char *)1; /* Let the previous dialog die */
    XtFree(dflt);
}

/* Prompt the user and allow him to choose from a selection of answers or
 * allow him to type a new one.  If controls include PB_MUST_MATCH, the
 * response must match one of the choices.  Return an _allocated_ string
 * that must be freed using XtFree().
 * "controls" may be one of:
 *	PB_MUST_MATCH PB_FILE_BOX PB_FILE_OPTION PB_NO_TEXT PB_NO_ECHO
 *	PB_TRY_AGAIN PB_MSG_LIST
 * When PB_FILE_*, "controls" may also include either or both of:
 *	PB_MUST_EXIST PB_NOT_A_DIR
 */
char *
PromptBox(parent, query, dflt, choices, n_choices, controls, retval)
Widget parent;
char *query;
const char *dflt;
const char **choices;
int n_choices;
u_long controls;
AskAnswer *retval;	/* Ignored unless PB_TRY_AGAIN */
{
    Widget dialog, text_w, (*dialog_func)(), (*get_child)();
    char *answer = NULL, *dialog_name = "prompt_dialog";
    XmString *xm_choices = 0, title, label, txt_label, dflt_choice;
    Arg args[16];
    int n_args, must_match = ison(controls, PB_MUST_MATCH);
    int show_choices = FALSE;
    void (*done_func)();

    if (!parent) /* if "ask" is called before tool is initialized */
	parent = tool;

    if (n_choices > 1 || ison(controls, PB_MSG_LIST) ||
	    n_choices == 1 && dflt && strcmp(choices[0], dflt) != 0)
	show_choices = TRUE;
    if (show_choices) {
	if (ison(controls, PB_FILE_BOX))
	    turnon(controls, PB_FILE_OPTION);
	turnoff(controls, PB_FILE_BOX);
	if (isoff(controls, PB_MSG_LIST)) {
	    /* XXX casting away const */
	    xm_choices = ArgvToXmStringTable(n_choices, (char **) choices);
	    /* XXX casting away const -- Motif has poor prototypes */
	    dflt_choice = dflt? XmStr((char *) dflt) : XmStringCopy(xm_choices[0]);
	} else {
	    msg_group list;
	    int i, j;
	    init_msg_group(&list, msg_cnt, 1);
	    clear_msg_group(&list);
	    /* XXX casting away const */
	    if (get_msg_list((char **) choices, &list) == -1) {
		destroy_msg_group(&list);
		return NULL;
	    }
	    for (i = j = 0; i < list.mg_max; i++)
		if (msg_is_in_group(&list, i))
		    j++;
	    if (!(xm_choices = (XmStringTable)
		    XtMalloc((j+1) * sizeof (XmString)))) {
		destroy_msg_group(&list);
		return NULL;
	    }
	    for (i = j = 0; i < list.mg_max; i++)
		if (msg_is_in_group(&list, i))
		    xm_choices[j++] = XmStr(compose_hdr(i));
	    n_choices = j;
	    xm_choices[j] = 0;
	    dflt_choice = dflt && chk_msg(dflt)?
		XmStr(compose_hdr(chk_msg(dflt)-1)) : 0;
	    destroy_msg_group(&list);
	}
	dialog_func = XmCreateSelectionDialog;
	get_child = XmSelectionBoxGetChild;
    } else if (ison(controls, PB_FILE_BOX)) {
	return FileFinderPromptBox(parent, query, dflt, controls);
    } else {
	dialog_func = XmCreatePromptDialog;
	get_child = XmSelectionBoxGetChild;
    }

    if (ison(controls, PB_FILE_BOX|PB_FILE_OPTION)) {
	title = XmStr(catgets( catalog, CAT_MOTIF, 454, "File Selection" ));
	dialog_name = "file_prompt_dialog";
    } else if (ison(controls, PB_MSG_LIST)) {
	title = XmStr(catgets( catalog, CAT_MOTIF, 455, "Message List Selection" ));
	dialog_name = "message_prompt_dialog";
    } else if (ison(controls, PB_TRY_AGAIN)) {
	title = XmStr(catgets( catalog, CAT_MOTIF, 456, "Selection" ));
	dialog_name = "lookup_prompt_dialog";
    } else
	title = XmStr(catgets( catalog, CAT_MOTIF, 457, "Input" ));

    if (query && *query && ison(controls, PB_NO_TEXT|PB_MSG_LIST)) {
	label = XmStr(query);
	txt_label = 0;
    } else {
	label = XmStr(catgets( catalog, CAT_MOTIF, 458, "Choices" ));
	txt_label = zmXmStr(query);
    }

    n_args = XtVaSetArgs(args, XtNumber(args),
	XmNuserData,             query,	/* Sneakiness */
	XmNautoUnmanage,	 False,
	XmNdialogStyle,		 XmDIALOG_FULL_APPLICATION_MODAL,
	XmNdialogTitle,          title,
	/* unused by FileSelectionDialog and PromptDialog, but it's ok */
	XmNlistLabelString,      label,
#if 0
        /* inserting the items is now deferred until after XtManage */
	XmNlistItems,            xm_choices,
	XmNlistItemCount,        n_choices,
#endif
        XmNlistSizePolicy,       XmCONSTANT, /* XXX XmRESIZE_IF_POSSIBLE? */
	XmNmustMatch,            must_match,
	XmNnoResize,             True,
	/* this won't take effect if file_selection (n_choices < 0) */
	XmNlistVisibleItemCount, min(5, n_choices),
	/* don't use XmNselectionLabelString if no txt_label */
	txt_label? XmNselectionLabelString : SNGL_NULL, txt_label,
	NULL);
    dialog = (*dialog_func)(GetTopShell(parent), dialog_name,
	args, n_args - ison(controls, PB_FILE_BOX|PB_MSG_LIST));
    {
      XtWidgetGeometry geom;
      geom.request_mode = CWWidth;
      XtQueryGeometry(dialog, NULL, &geom);
      XtVaSetValues(dialog, XmNwidth, geom.width, NULL);
    }

    if (ison(controls, PB_NO_TEXT|PB_MSG_LIST)) {
	XtUnmanageChild((*get_child)(dialog, XmDIALOG_TEXT));
	XtUnmanageChild((*get_child)(dialog, XmDIALOG_SELECTION_LABEL));
    }
    if (isoff(controls, PB_TRY_AGAIN))
	XtUnmanageChild((*get_child)(dialog, XmDIALOG_HELP_BUTTON));

    XmStringFree(title);
    XmStringFree(label);

    if (ison(controls, PB_MSG_LIST))
	done_func = list_done;
    else
	done_func = prompt_done;

    if (isoff(controls, PB_FILE_BOX+PB_FILE_OPTION+PB_TRY_AGAIN))
	XtUnmanageChild((*get_child)(dialog, XmDIALOG_APPLY_BUTTON));
    if (ison(controls, PB_FILE_OPTION)) {
	XtVaSetValues(dialog, XmNapplyLabelString, zmXmStr(catgets( catalog, CAT_MOTIF, 459, "Search" )), NULL);
	XtAddCallback(dialog, XmNapplyCallback,
		      (XtCallbackProc) FilePromptBox, &answer);
	XtManageChild((*get_child)(dialog, XmDIALOG_APPLY_BUTTON));
	turnoff(controls, PB_TRY_AGAIN);
    } else if (ison(controls, PB_TRY_AGAIN)) {
	XtVaSetValues(dialog, XmNapplyLabelString, zmXmStr(catgets( catalog, CAT_MOTIF, 460, "Retry" )), NULL);
	XtVaSetValues(dialog, XmNcancelLabelString, zmXmStr(catgets( catalog, CAT_MOTIF, 461, "Omit" )), NULL);
	XtVaSetValues(dialog, XmNhelpLabelString, zmXmStr(catgets( catalog, CAT_MOTIF, 443, "Cancel" )), NULL);
	*retval = AskYes;
	XtAddCallback(dialog, XmNapplyCallback, (XtCallbackProc) TryAgainUsed,
	    vaptr((char *) &answer, done_func, retval, NULL));
	XtManageChild((*get_child)(dialog, XmDIALOG_APPLY_BUTTON));
	XtAddCallback(dialog, XmNcancelCallback, (XtCallbackProc) SkipItUsed,
	    vaptr((char *) &answer, done_func, retval, NULL));
	XtAddCallback(dialog, XmNhelpCallback, done_func, &answer);
    }

    if (isoff(controls, PB_TRY_AGAIN))
	XtAddCallback(dialog, XmNcancelCallback, done_func, &answer);

    if (must_match)
	XtAddCallback(dialog, XmNnoMatchCallback, done_func, &answer);
    SetDeleteWindowCallback(XtParent(dialog), done_func, &answer);

    text_w = (*get_child)(dialog, XmDIALOG_TEXT);
    if (ison(controls, PB_NO_ECHO)) {
	Pixel bg;
        if (!getenv("ZMAIL_NO_ASTERISK"))
          {
	    XtAddCallback(text_w, XmNmodifyVerifyCallback, check_passwd, NULL);
	    XtAddCallback(dialog, XmNokCallback, check_passwd, &answer);
          }
        else
          {
	    XtAddCallback(dialog, XmNokCallback, done_func, &answer);
	    XtVaGetValues(text_w, XmNbackground, &bg, NULL);
	    XtVaSetValues(text_w, XmNforeground, bg, NULL);
          }
    } else {
	XtAddCallback(dialog, XmNokCallback, done_func, &answer);
	if (isoff(controls, PB_NO_TEXT+PB_MSG_LIST)) {
	    XtAddCallback(text_w, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, NULL);
	    XtAddCallback(text_w, XmNmodifyVerifyCallback, (XtCallbackProc) filec_cb, NULL);
	    XtAddCallback(text_w, XmNmotionVerifyCallback, (XtCallbackProc) filec_motion, NULL);
	}
    }
    if (dflt)
	SetTextString(text_w, dflt);

    XtManageChild(dialog);

    if (show_choices)
    {
      Widget list_w = (*get_child)(dialog, XmDIALOG_LIST); 
      XmListAddItems(list_w, xm_choices, n_choices, 0);
      XmStringFreeTable(xm_choices);
      if (n_choices > 1)
        {
	  LIST_SELECT_ITEM(list_w, dflt_choice, False);
	  LIST_VIEW_POS(list_w, LIST_ITEM_POS(list_w, dflt_choice));
        }
      XmStringFree(dflt_choice);
    }

    SetTextInput(text_w);

    timeout_cursors(TRUE);
    assign_cursor(frame_list, do_not_enter_cursor);
    zmModalPopdown(dialog, &answer);
    assign_cursor(frame_list, please_wait_cursor);
    timeout_cursors(FALSE);

    if (answer == (char *)1)
	answer = 0;
    return answer;
}

static void
check_passwd(text_w, ret_passwd, cbs)
Widget        text_w;
char **ret_passwd; /* return */
XmTextVerifyCallbackStruct *cbs;
{
    static char *passwd;
    static char *returned;
    char *new;
    int len;
    static int first_time = 0;
    char str[2];

    if (first_time == 0)
      {
        passwd = XtMalloc(1); 
        passwd[0] = 0;
        first_time = 1;
      }

    if (cbs->reason == XmCR_OK) {
        first_time = 0;
        returned = XtMalloc(strlen(passwd)+1);
        strcpy(returned,passwd);
        XtFree(passwd);
	*ret_passwd = returned;
	return;
    }

    if (cbs->text->ptr == NULL && passwd) { /* backspace */
        cbs->endPos = strlen(passwd); /* delete from here to end */
        passwd[cbs->startPos] = 0; /* backspace--terminate */
        return;
    }

    if (cbs->text->length > 1) {
        cbs->doit = False; /* don't allow "paste" operations */
        return; /* make the user *type* the password! */
    }

/* At this point we are dealing with one character to append or insert */

    new = XtMalloc(strlen(passwd)+1);
    str[0] = cbs->text->ptr[0];
    str[1] = 0;
    if (cbs->startPos >= strlen(passwd))
      {
        (void) strcpy(new, passwd); /* Append it */
        (void) strcat(new, str);
      }
    else if (cbs->startPos <= 0)
      {
        (void) strcpy(new, str); /* Insert it at the beginning */
        (void) strcat(new, passwd);
      }
    else
      {
        (void) strncpy(new,passwd,cbs->startPos); /* Insert it in the middle */
        new[cbs->startPos] = 0;
        (void) strcat(new, str);
        (void) strcat(new, passwd+cbs->startPos);
      }
    XtFree(passwd);
    passwd = new;

    for (len = 0; len < cbs->text->length; len++)
        cbs->text->ptr[len] = '*'; 
}

void
zmModalPopdown(dialog, answer)
Widget dialog;
char **answer;
{
#ifdef TIMER_API
    CRITICAL_BEGIN {
#else /* !TIMER_API */
	XtIntervalId do_timer = xt_timer;
	
	if (xt_timer) {
	    XtRemoveTimeOut(xt_timer);
	    xt_timer = 0;
	}
#endif /* TIMER_API */
	
	zmKeepDialogUp(dialog);
	while (*answer == NULL /* && isoff(glob_flags, WAS_INTR) */)
	    XtAppProcessEvent(app, XtIMAll);
	XmUpdateDisplay(tool);
	
#ifdef NOT_NOW
	/* Destroying the dialog itself tickles a motif bug that may cause
	 * gadgets to be RemoveGrabbed after they have been destroyed.
	 */
	if (XtIsManaged(dialog))
	    ZmXtDestroyWidget(GetTopShell(dialog));
	else
	    ZmXtDestroyWidget(dialog);
#endif /* NOT_NOW */
	XtPopdown(GetTopShell(dialog));	/* LEAK */
	
#ifdef TIMER_API
    } CRITICAL_END;
#else /* !TIMER_API */
    if (do_timer)
	xt_timer = XtAppAddTimeOut(app, (u_long)0, gui_check_mail, NULL);
#endif /* TIMER_API */
}
