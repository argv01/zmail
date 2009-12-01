/* m_actions.c	Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * This file contains routines that allow user actions to be recorded
 * and played back.  Only certain objects (those that make sense) have
 * their actions recorded and played back.  The intent is to be able
 * to define macros for yourself, record common event tasks, or even
 * generate a rolling "demo".
 */

#include "zmail.h"
#include "catalog.h"

#ifdef RECORD_ACTIONS
#include "gui_def.h"

#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/Text.h>
#include <Xm/ToggleB.h>

#define PLAY_MODE	ULBIT(0)
#define RECORD_MODE	ULBIT(1)
static u_long rec_or_play;

/* abstract actions that can be done with user interface objects */
typedef enum {
     ActionActivate,/* generic select: buttons, toggles, lists, text, etc */
     ActionSelect,   /* lists, ??? */
     ActionDeselect, /* lists, toggles, etc */
     ActionSelectAndActivate,
     ActionNext,    /* process traversal */
     ActionPrev,    /* process traversal */
     ActionTab,     /* process traversal */
     ActionLast
} ZmAction;

/* table used to translate strings into actions */
typedef struct {
    char *string;
    ZmAction action;
} ZmActionEventTable;

static ZmActionEventTable zme_table[] = {
    { "ActionActivate", ActionActivate },
    { "ActionSelect", ActionSelect },
    { "ActionDeselect", ActionDeselect },
    { "ActionSelectAndActivate", ActionSelectAndActivate },
    { "ActionNext", ActionNext },
    { "ActionPrev", ActionPrev },
    { "ActionTab", ActionTab },
    { NULL, ActionLast }
};

/* Action events taken from Motif ButtonWidget */
static ZmActionEventTable btn_table[] = {
    { "Activate", ActionActivate },
    { "Arm", ActionSelect },
    { "Disarm", ActionDeselect },
    { "ArmAndActivate", ActionSelectAndActivate },
    { NULL, ActionLast }
};

/* Action events taken from Motif ButtonWidget */
static ZmActionEventTable list_table[] = {
    { "ListKbdActivate", ActionActivate },
    { "ListKbdActivate", ActionSelectAndActivate },
    { "ListEndSelect", ActionSelect },
    { NULL, ActionLast }
};

/* when an action is found, look up the widget class of the widget
 * specified and call the function that operates on the event in a
 * manner appropriate for that widget type.
 */
typedef struct {
    WidgetClass *class;
    void (*play_func)();
    void (*rec_func)();
} WidgetFunc;

static void
    play_list_action(), play_button_action(), play_toggle_action(),
    play_text_action(),
    rec_list_action(), rec_button_action(), rec_toggle_action(),
    rec_text_action();

static WidgetFunc widget_funcs[] = {
    &xmListWidgetClass, play_list_action, rec_list_action,
    &xmPushButtonWidgetClass, play_button_action, rec_button_action,
    &xmToggleButtonWidgetClass, play_toggle_action, rec_toggle_action,
    &xmTextWidgetClass, play_text_action, rec_text_action,
};

/* Misc support functions ... */
static ZmAction
lookup_action(action, table)
char *action;
ZmActionEventTable table[];
{
    int i;

    for (i = 0; table[i].string; i++)
	if (!strcmp(action, table[i].string))
	    return table[i].action;

    return ActionLast;
}

static char *
action_string(action, table)
ZmAction action;
ZmActionEventTable table[];
{
    int i;

    for (i = 0; table[i].string; i++)
	if (action == table[i].action)
	    return table[i].string;

    return NULL;
}

static void
(*lookup_func(widget, play_or_record))()
Widget widget;
{
    int i;
    WidgetClass class = XtClass(widget);

    for (i = 0; i < XtNumber(widget_funcs); i++)
	if (class == *widget_funcs[i].class)
	    return play_or_record == PLAY_MODE?
		widget_funcs[i].play_func : widget_funcs[i].rec_func;

    return 0;
}

static void
call_xt_action(widget, action, sleeptime, button_event)
Widget widget;
char *action;
int sleeptime;
int button_event; /* boolean */
{
    XEvent xevent;

    bzero(&xevent, sizeof(XEvent));
    if (button_event)
	xevent.xbutton.button = 1;
    XtCallActionProc(widget, action, &xevent, DUBL_NULL, 0);
    ForceUpdate(widget);
    if (sleeptime)
	sleep(sleeptime);
}

/* Z-Script: play_action */
gui_play_action(argc, argv, list)
int argc;
char *argv[];
msg_group *list;
{
    Widget widget;
    int sleeptime = 1;
    ZmAction act;
    void (*func)();
    char *myname = argv[0];

    if (argc < 3) {
usage:
	print("usage: %s [-sleep N] widget-path action [options]\n", myname);
	return -1;
    }

    if (ison(rec_or_play, RECORD_MODE)) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 19, "Cannot play actions in record mode." ));
	return -1;
    }
    if (!strcmp(argv[1], "-sleep")) {
	if (argv += 2, (argc < 5))
	    goto usage;
	else
	    sleeptime = atoi(*argv);
    }

#ifdef NOT_NOW
    /* noop for now */
    if (!strncmp(argv[1], "-st", 3)) { /* "stop" or "start" */
	timeout_cursors(!strcmp(argv[1], "-start"));
	return 0;
    }
#endif /* NOT_NOW */

    if (!(widget = XtNameToWidget(tool, argv[1]))) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 20, "%s: widget not found." ), argv[1]);
	return -1;
    }

    act = lookup_action(argv[2], zme_table);
    switch ((int)act) {
	case ActionNext:    /* process traversal */
	    XmProcessTraversal(widget, XmTRAVERSE_NEXT);
	when ActionPrev:    /* process traversal */
	    XmProcessTraversal(widget, XmTRAVERSE_PREV);
	when ActionTab:    /* process traversal */
	    XmProcessTraversal(widget, XmTRAVERSE_NEXT_TAB_GROUP);
    }

    if (func = lookup_func(widget, PLAY_MODE)) {
	turnon(rec_or_play, PLAY_MODE);
	(*func)(widget, act, argv+3, (XEvent *)NULL);
	turnoff(rec_or_play, PLAY_MODE);
    }

    return 0;
}

void
record_actions(widget, client_data, action_name, event, args, num_args)
Widget widget;
XtPointer client_data;
String action_name;
XEvent *event;
String *args;
int *num_args;
{
    int n;
    static FILE *file;
    static char *filename;
    char *str;
    void (*func)();

    if (!widget) {
	fclose(file);
	return;
    }
    if (filename != client_data || !file) {
	if (file && file != stdout) {
	    fclose(file);
	    file = 0;
	}
	if ((filename = client_data) && !(file = fopen(filename, "w"))) {
	    ask_item = widget;
	    error(SysErrWarning, filename);
	}
	if (!file)
	    file = stdout;
	else
	    setbuf(file, NULL);
    }
    if (!(str = WidgetString(widget)))
	return;

    if (func = lookup_func(widget, RECORD_MODE)) {
	turnon(rec_or_play, RECORD_MODE);
	(*func)(widget, str, action_name, event, file);
	turnoff(rec_or_play, RECORD_MODE);
    }
}

/*
 * per-widget action functions come in pairs...
 * play_function(Widget widget, ZmAction action, char **argv);
 * record_function(Widget widget, char *widget_name, char *action_name,
 *		   XEvent *event, FILE *fp);
 */

static void
play_list_action(widget, action, argv)
Widget widget;
ZmAction action;
char *argv[];
{
    int i, *pos_list, pos_count;

    switch ((int)action) {
	case ActionActivate :
	    if (!XmListGetSelectedPos(widget, &pos_list, &pos_count))
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 21, "%s:\nNo items selected." ));
	    else {
		XmListSelectPositions(widget, pos_list, pos_count, True);
		XtFree(pos_list);
	    }

	when ActionSelectAndActivate:
	case ActionSelect: {
	    i = atoi(*argv);
	    XmListSelectPos(widget, i, action == ActionSelectAndActivate);
	}

	when ActionNext:
	case ActionPrev:
	    if (!XmListGetSelectedPos(widget, &pos_list, &pos_count))
		i = 1;
	    else
		i = pos_list[0];
	    XmListSelectPos(widget, i, False);

	otherwise: ;
	    /*
	    error(UserErrWarning, "%s: cannot execute %s.",
		XtName(widget), action_string(action));
	    */
    }
}

static void
rec_list_action(widget, w_name, action_name, event, fp)
Widget widget;
char *w_name, *action_name;
XEvent *event;
FILE *fp;
{
    fprintf(fp, "%s %s\n", w_name, action_name);
}

static void
play_button_action(widget, action, argv)
Widget widget;
ZmAction action;
char *argv[];
{
    switch ((int)action) {
	case ActionSelectAndActivate:
	    call_xt_action(widget, "Arm", 1, True);
	    call_xt_action(widget, "Activate", 0, True);
	    call_xt_action(widget, "DisArm", 1, True);

	when ActionSelect:
	    call_xt_action(widget, "Arm", 1, True);
	when ActionDeselect:
	    call_xt_action(widget, "Disarm", 1, True);
	when ActionActivate :
	    call_xt_action(widget, "Activate", 0, True);

	otherwise: ;
	    /*
	    error(UserErrWarning, "%s: cannot execute %s.",
		XtName(widget), action_string(action));
	     */
    }
}

static void
rec_button_action(widget, w_name, action_name, event, fp)
Widget widget;
char *w_name, *action_name;
XEvent *event;
FILE *fp;
{
    fprintf(fp, "%s %s\n", w_name, action_name);
}

static void
play_toggle_action(widget, action, argv)
Widget widget;
ZmAction action;
char *argv[];
{
    switch ((int)action) {
	case ActionActivate :
	    call_xt_action(widget, "Activate", 1, True);

	when ActionSelectAndActivate:
	    call_xt_action(widget, "ArmAndActivate", 1, True);

	when ActionSelect:
	    call_xt_action(widget, "Activate", 1, True);
	when ActionDeselect:
	    call_xt_action(widget, "DeSelect", 1, True);

	otherwise: ;
	    /*
	    error(UserErrWarning, "%s: cannot execute %s.",
		XtName(widget), action_string(action));
	    */
    }
}

static void
rec_toggle_action(widget, w_name, action_name, event, fp)
Widget widget;
char *w_name, *action_name;
XEvent *event;
FILE *fp;
{
    fprintf(fp, "%s %s\n", w_name, action_name);
}

static void
play_text_action(widget, action, argv)
Widget widget;
ZmAction action;
char *argv[];
{
}

static void
rec_text_action(widget, w_name, action_name, event, fp)
Widget widget;
char *w_name, *action_name;
XEvent *event;
FILE *fp;
{
    fprintf(fp, "%s %s\n", w_name, action_name);
}

#endif /* RECORD_ACTIONS */
