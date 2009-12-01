/* zcard.c -- a "postcard" for zmail.  */

/* NOTE--this has been started, but abandoned in favor of doing more
   important things...
 */

#include "zmail.h"
#include "zm_frame.h"

#include <Xm/List.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/MessageB.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Text.h>

#include "bitmaps/zm_logo.xbm"

static void add_user(), send_it(), add_to_to();

String fallback_resources[] = {
    "*XmText.fontList: -*-courier-medium-r-*--12-*",
    "*XmText.translations: #override \
        Ctrl<Key>D: activate() \n\
        Ctrl<Key>U: kill-to-start-of-line() \n\
        Ctrl<Key>W: delete-previous-word() \n\
        <Key>osfDelete: delete-previous-character()",
    "*msg-text.rows: 15",
    "*msg-text.columns: 60",
    "*XmPushButton.fontList: -*-new century schoolbook-bold-r-*--12-*",
    "*XmPushButtonGadget.fontList: -*-new century schoolbook-bold-r-*--12-*",
    "*XmLabelGadget.fontList: -*-new century schoolbook-bold-r-*--12-*",
    "*XmList.fontList: -*-courier-medium-r-*--12-*",
    "*zcard.labelString: Z-Card",
    "*title.labelString: Quick Message Sender",
    "*actions*leftAttachment: attach_position",
    "*actions*rightAttachment: attach_position",
    "*to-label.labelString: To:",
    "*to-list.visibleItemCount: 6",
    "*subject-label.labelString: Subject:",
    "*add-btn.labelString: Add",
    "*delete-btn.labelString: Delete",
    "*send-btn.labelString: Send",
    "*quit-btn.labelString: Quit",
    "*error.messageString: You must provide at least one message recipient.",
    "*error.cancelLabelString: OK",
    NULL
};

do_zcard(parent)
Widget parent;
{
    Widget toplevel, label, left, heading, icon, titles, actions, rc, w, send_w;
    XtAppContext app;
    Arg args[3];
    Pixel fg, bg;
    Pixmap pixmap;
    extern void exit();

    shell = XtVaAppCreateShell("zcard",
	ZM_APP_CLASS, applicationShellWidgetClass, display,
	XmNdeleteResponse, XmDESTROY,
	XmNtitle,          "Compose Message",
	XmNiconName,       "Compose",
	NULL);
    /* SetDeleteWindowCallback(comp_shell, abort_mail, (char *)True); */

    /* The form is the general layout manager for the application.
     * It contains two main widgets: a rowcolumn and a scrolled text.
     */
    rc = XtVaCreateWidget("rc", xmRowColumnWidgetClass, toplevel,
        XmNorientation, XmHORIZONTAL,
        NULL);

    /* left side is a RowColumn -- a child of the bigger RowColumn */
    left = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, rc, NULL);

    /* start the left side with a RowColumn to hold the heading */
    heading = XtVaCreateWidget("heading", xmFormWidgetClass, left, NULL);

    /* create an icon to make things pretty */
    XtVaGetValues(heading, XmNforeground, &fg, XmNbackground, &bg, NULL);
    pixmap = XCreatePixmapFromBitmapData(XtDisplay(heading),
        RootWindowOfScreen(XtScreen(heading)),
        /* these values are defined in "zcard.icon" */
        zm_logo_bits, zm_logo_width, zm_logo_height,
        fg, bg, DefaultDepthOfScreen(XtScreen(heading)));
    icon = XtVaCreateManagedWidget("zcard_icon", xmLabelGadgetClass, heading,
        XmNleftAttachment, XmATTACH_FORM,
        XmNlabelType,       XmPIXMAP,
        XmNlabelPixmap,     pixmap,
        XmNalignment,       XmALIGNMENT_END,
        NULL);

    /* identify the program */
    titles = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, heading,
        XmNrightAttachment,  XmATTACH_FORM,
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       icon,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtVaCreateManagedWidget("zcard", xmLabelGadgetClass, titles, NULL);
    XtVaCreateManagedWidget("title", xmLabelGadgetClass, titles, NULL);
    XtManageChild(titles);
    XtManageChild(heading);

    /* provide the "To:" prompt (see the resources above) */
    to_w = CreateLabeledTextForm(left, "to-label", "to");

    /* prompt for the subject (see the resources above) */
    subj_w = CreateLabeledTextForm(left,
        "subject-label", "subject-text");
    /* when user hits <Return>, advance caret to next input item */
    XtAddCallback(subj_w, XmNactivateCallback,
        XmProcessTraversal, XmTRAVERSE_NEXT_TAB_GROUP);

    /* Create a ScrolledList of all the recipients entered in To: */
    XtSetArg(args[0], XmNscrollingPolicy, XmAUTOMATIC);
    XtSetArg(args[1], XmNselectionPolicy, XmEXTENDED_SELECT);
    XtSetArg(args[2], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
    list_w = XmCreateScrolledList(left, "to-list", args, 3);
    XtAddCallback(list_w, XmNdefaultActionCallback, add_to_to, to_w);
    XtManageChild(list_w);

    /* Any command line args are recipients */
    while (argc-- > 1) {
        XmString str = XmStringCreateSimple(*++argv);
        XmListAddItemUnselected(list_w, str, 0);
        XmStringFree(str);
    }

    /* Add, Delete, Send and Quit buttons -- space equally */
    actions = XtVaCreateWidget("actions", xmFormWidgetClass, left, NULL);

    send_w = XtVaCreateManagedWidget("send-btn",
        xmPushButtonWidgetClass, actions,
        XmNleftPosition, 0,  /* attachment resources in fallbacks! */
        XmNrightPosition, 23,
        NULL);
    XtAddCallback(send_w, XmNactivateCallback, send_it, NULL);

    w = XtVaCreateManagedWidget("add-btn",
        xmPushButtonWidgetClass, actions,
        XmNleftPosition, 26, /* attachment resources in fallbacks! */
        XmNrightPosition, 46,
        NULL);
    /* clicking on Add user adds user to scrolled list */
    XtAddCallback(w, XmNactivateCallback, add_user, (XtPointer)1);

    /* Make it appear as tho hitting return in To: text widget
     * is just like clicking on the Add button.
     */
    XtAddCallback(to_w, XmNactivateCallback, add_user, w);

    w = XtVaCreateManagedWidget("delete-btn",
        xmPushButtonWidgetClass, actions,
        XmNleftPosition, 49,  /* attachment resources in fallbacks! */
        XmNrightPosition, 75,
        NULL);
    /* clicking on delete calls add_user() with a 0 client_data */
    XtAddCallback(w, XmNactivateCallback, add_user, (XtPointer)0);

    w = XtVaCreateManagedWidget("quit-btn",
        xmPushButtonWidgetClass, actions,
        XmNleftPosition, 78,  /* attachment resources in fallbacks! */
        XmNrightPosition, 100,
        NULL);
    XtAddCallback(w, XmNactivateCallback, exit, NULL);
    XtManageChild(actions);

    /* right side is a scrolled text region for letter input. */
    XtSetArg(args[0], XmNeditMode,          XmMULTI_LINE_EDIT);
    XtSetArg(args[1], XmNscrollVertical,    True);
    XtSetArg(args[2], XmNscrollHorizontal,  True);
    text_w = XmCreateScrolledText(rc, "msg-text", args, 3);
    XtManageChild(text_w);

    /* Ctrl-D in text_w causes activate() which calls send_it() */
    XtAddCallback(text_w, XmNactivateCallback, send_it, send_w);

    XtManageChild(left);
    XtManageChild(rc);

    /* add tab groups in the order we'd like tabbing to follow */
    XmAddTabGroup(to_w);
    XmAddTabGroup(subj_w);
    XmAddTabGroup(text_w);
    XmAddTabGroup(actions);
    XmAddTabGroup(list_w);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);
}

/* user clicked on either Add or Delete buttons, or he hit return in
 * the To: text field.  In the latter case, "data" is the add_btn,
 * so call that widget's ArmAndActivate() action proc.
 */
static void
add_user(w, data, cbs)
Widget w;
XtPointer data;
XmAnyCallbackStruct *cbs;
{
    if (w == to_w) {
        /* User hit return... make it look as tho he clicked on Add */
        XtCallActionProc(data, "ArmAndActivate", cbs->event, NULL, 0);
        return;
    }
    /* User clicked on Add if data==1, or delete otherwise */
    if (data) {
        /* get the value of the To: text widget */
        char *text = XmTextGetString(to_w);
        XmString str = XmStringCreateSimple(text);
        if (text && *text) /* if not a null string, add to List */
            XmListAddItemUnselected(list_w, str, 0);
        XmStringFree(str);
        XtFree(text);
        XmTextSetString(to_w, NULL); /* reset so user can add more */
    } else {
        /* user clicked on Delete; delete all selected names */
        int *sel, n;
        if (!XmListGetSelectedPos(list_w, &sel, &n))
            return;
        /* Must delete in reverse order or positions get messed up! */
        while (n--)
            XmListDeletePos(list_w, sel[n]);
        XtFree(sel);
    }
}

/* double-clicking a list item causes the selected item to To: text */
static void
add_to_to(list_w, to_w, cbs)
Widget list_w, to_w;
XmListCallbackStruct *cbs;
{
    char *text;

    XmStringGetLtoR(cbs->item, "", &text);
    XmTextSetString(to_w, text);
    XmTextSetInsertionPosition(to_w, strlen(text));
    XtFree(text);
    XmListDeletePos(list_w, cbs->item_position);
    /* it's a long way, but traverse to To: text field */
    XmProcessTraversal(list_w, XmTRAVERSE_NEXT_TAB_GROUP);
    XmProcessTraversal(list_w, XmTRAVERSE_NEXT_TAB_GROUP);
    XmProcessTraversal(list_w, XmTRAVERSE_NEXT_TAB_GROUP);
}

/* user clicked on "Send" -- build a command line, use popen() to
 * open pipe to mail command, send text data to it, then exit.
 */
static void
send_it(w, send_w, cbs)
Widget w, send_w; /* send_w -only- when w == text_w !!! */
XmAnyCallbackStruct *cbs;
{
    char *text, *subj, cmd[BUFSIZ], *p, *dummy, *getenv();
    int n, i, status;
    XmString *list;
    FILE *pp, *popen();

    if (w == text_w) {
        XtCallActionProc(send_w, "ArmAndActivate", cbs->event, NULL, 0);
        return;
    }
    
    /* if something was left in the To: field, grab it */
    text = XmTextGetString(to_w);
    if (text != 0 && *text != 0) {
        XmString str = XmStringCreateSimple(text);
        XmListAddItemUnselected(list_w, str, 0);
        XmTextSetString(to_w, "");
        XmStringFree(str);
        XtFree(text);
    }

    /* Get the list of users entered */
    XtVaGetValues(list_w,
        XmNitems, &list,
        XmNitemCount, &n,
        NULL);
    if (n == 0) {
        static Widget dialog;
        /* user goofed -- must provide at least one recipient */
        if (!dialog) {
            Arg args[1];
            XtSetArg(args[0], XmNdialogStyle, XmDIALOG_APPLICATION_MODAL);
            dialog = XmCreateErrorDialog(to_w, "error", args, 1);
            XtUnmanageChild(
                XmMessageBoxGetChild(dialog, XmDIALOG_HELP_BUTTON));
            XtUnmanageChild(
                XmMessageBoxGetChild(dialog, XmDIALOG_OK_BUTTON));
        }
        XtManageChild(dialog);
        return;
    }

    /* get the subject (may be empty) */
    subj = XmTextGetString(subj_w);

    /* build command line */
    if (!(p = getenv("MAIL_CMD")))
        p = MAIL_CMD;
    p = strcpy(cmd, p);
    p += strlen(cmd);
    *p++ = ' ';
    if (subj && *subj) {
        /* if subject not empty, add to mail command */
        sprintf(p, "-s \"%s\" ", subj);
        p += strlen(p);
    }

    /* Add each user in the List to the command line */
    for (i = 0; i < n; i++) {
        XmStringGetLtoR(list[i], "", &dummy);
        p += strlen(strcpy(p, dummy));
        if (i < n-1) /* more to come yet... */
            *p++ = ',', *p++ = ' '; /* separate addresses w/commas */
    }

    /* open pipe to mail command */
    if (!(pp = popen(cmd, "w"))) {
        fprintf(stderr, CATGETS("Cannot execute"));
        perror(cmd);
        return;
    }
    /* give it the text user typed (may be empty) */
    text = XmTextGetString(text_w);
    fputs(text, pp);
    fputc('\n', pp); /* make sure there's a terminating newline */
    status = pclose(pp); /* close mail program */

    XtFree(text);
    XtFree(subj);
    if (status == 0) {
        XmTextSetString(to_w, NULL);
        XmTextSetString(text_w, NULL);
        XmTextSetString(subj_w, NULL);
        XmListDeleteAllItems(list_w);
    }
    /* send complete -- start back at beginning */
    XmProcessTraversal(w, XmTRAVERSE_HOME);
}

/* Create a Form widget that has a label on the left and a Text
 * widget to the right.  Attach perimeter edges to form.
 * We use it twice in the program, so make a function out of it.
 */
Widget
CreateLabeledTextForm(parent, label_name, text_name)
Widget parent;
char *label_name, *text_name;
{
    Widget form, label, ret;

    form = XtVaCreateWidget(NULL, xmFormWidgetClass, parent,
        XmNorientation,      XmHORIZONTAL,
        NULL);
    label = XtVaCreateManagedWidget(label_name, xmLabelGadgetClass, form,
        XmNleftAttachment,   XmATTACH_FORM,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    ret = XtVaCreateManagedWidget(text_name, xmTextWidgetClass, form,
        XmNleftAttachment,   XmATTACH_WIDGET,
        XmNleftWidget,       label,
        XmNtopAttachment,    XmATTACH_FORM,
        XmNrightAttachment,  XmATTACH_FORM,
        XmNbottomAttachment, XmATTACH_FORM,
        NULL);
    XtManageChild(form);

    return ret;
}
