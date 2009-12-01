/* m_license.c	Copyright 1990, 1991 Z-Code Software Corp. */

#ifdef SPTX21
#define _XOS_H_
#endif /* SPTX21 */

#include <zcsock.h>
#include "zmail.h"
#include "zmframe.h"
#include "catalog.h"
#include "zm_motif.h"
#include "dismiss.h"

#ifndef LICENSE_FREE

#ifndef lint
static char	m_licens_rcsid[] =
    "$Id: m_licens.c,v 2.31 1996/05/06 22:20:46 schaefer Exp $";
#endif

#include "license/server.h"
#include "license/register.h"
FILE *openlib(/* char *s */); /* open a directory */

#include <Xm/DialogS.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/LabelG.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

typedef enum {
    AddUser = 0,
    DeleteUser = 1,
    UpdateEntries = 2,
    NewPasswd = 3
} LicenseAction;

static void
    action_callback(), add_to_textw(), list_entry(), update_entry_info();
static Widget
    passwd_textw, user_textw, user_label, entry_list, license_list,
    *action_area,
    progname_w, version_w, hostname_w, hostid_w, expire_w, max_users_w;
static char *passwd_file;
static int cannot_write, nentries;
static license_data *entries, *thisentry;

static ActionAreaItem btns[] = {
    { "Add",	 action_callback,	(caddr_t)AddUser },
    { "Delete",	 action_callback,	(caddr_t)DeleteUser },
    { "Update",  action_callback,	(caddr_t)UpdateEntries },
    { "New",	 action_callback,	(caddr_t)NewPasswd },
    { DONE_STR,  PopdownFrameCallback,	NULL },
    { "Help",    DialogHelp, "Registering Users" }
};

/*
typedef struct {
    char *password;
    char *progname;
    char *version;
    char *hostname;
    unsigned long hostid;
    long expiration_date;
    unsigned long max_users;
    long flags;
    char **valid_users;
} license_data;
*/

ZmFrame license_frame;

void
fill_license_list(zserv)
char *zserv;
{
    XmStringTable items;
    sock_t sock;
    FILE *fp = NULL;
    int i, locked = FALSE;

#ifdef NETWORK_LICENSE
    if (!zserv)
	zserv = getenv("ZCNLSERV");
#else /* !NETWORK_LICENSE */
    zserv = 0;
#endif /* !NETWORK_LICENSE */

    init_nointr_msg(zmVaStr(catgets( catalog, CAT_MOTIF, 246, "Getting license information from %s" ),
	zserv && ison(license_flags, NLS_LICENSE)?
	    zserv : zm_gethostname()), 100);
#ifdef NETWORK_LICENSE
    if (zserv && ison(license_flags, NLS_LICENSE)) {
	check_nointr_msg(catgets( catalog, CAT_MOTIF, 247, "Calling network license server..." ));
	if (sock = ls_call_server(zserv, ls_nls_client_connect_timeout(), ls_nls_client_gethello_timeout())) {
	    check_nointr_msg(catgets( catalog, CAT_MOTIF, 248, "Handshake..." ));
	    if (ls_nls_requestfile(sock) < 0) {
		error(UserErrWarning, ls_err);
		end_intr_msg(ls_err);
		return;
	    }
	    /* unportable hack to get this working */
	    fp = fdopen((int) sock, "r+");
	    cannot_write = 3;
	}
    }
#endif /* NETWORK_LICENSE */
    if (!fp && !(fp = openlib(passwd_file = zmlibdir, "r+", locked = TRUE))) {
	cannot_write++;                             /* DON made me do this! */
	if (!(fp = openlib(passwd_file = DEFAULT_LIB, "r+", locked = TRUE)))
	    cannot_write++;
    }
    if (cannot_write == 2 && !(fp =
	    openlib(passwd_file=zmlibdir, "r", locked = FALSE)))
	fp = openlib(passwd_file = DEFAULT_LIB, "r", locked = FALSE);
    if (!fp) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 249, "Cannot open license data file" ));
	end_intr_msg(catgets( catalog, CAT_SHELL, 136, "Failed." ));
	return;
    }
    check_nointr_msg(catgets( catalog, CAT_MOTIF, 251, "Reading license data..." ));
    ls_rdentries(fp, &entries, &nentries);
    if (cannot_write < 3)
	closelib(passwd_file, fp, locked);
    else
	(void) fclose(fp);

    items = (XmStringTable)XtCalloc(nentries+1, sizeof (XmString));

    for (i = 0; i < nentries; ++i)
	items[i] = XmStringCreateSimple(entries[i].password);

    XtVaSetValues(license_list,
	XmNitems, nentries? items : 0,
	XmNitemCount, nentries,
	NULL);

    XmStringFreeTable(items);

    end_intr_msg(catgets( catalog, CAT_SHELL, 119, "Done." ));
}

#endif /* !LICENSE_FREE */

#include "bitmaps/license.xbm"
ZcIcon license_icon = {
    "license_icon", 0, license_width, license_height, license_bits
};

#ifndef LICENSE_FREE

ZmFrame
DialogCreateLicense(w)
Widget w;
{
    Arg args[10];
    Widget pane, pane2, rc1, rc2;
    XmString foo;
    int i;

    if (license_frame) {
	/* Bart: Sat Jun 13 17:48:26 PDT 1992
	 * popup_dialog() already avoids making another call to
	 * the frame creation procedure once something non-NULL has
	 * been returned.  If we *want* to come through here each
	 * time, we have to return (ZmFrame)0 ...
	 */
	fill_license_list(NULL);
	FramePopup(license_frame);
	/* return license_frame; */
	return (ZmFrame)0;
    }

    license_frame = FrameCreate("license_dialog", FrameLicense, w,
	FrameFlags,	 FRAME_SHOW_ICON|FRAME_CANNOT_SHRINK,
	FrameIcon,	 &license_icon,
	FrameChild,	 &pane,
#ifdef NOT_NOW
	FrameTitle,	 "License Registration",
	FrameIconTitle,	 "Licenses",
#endif /* NOT_NOW */
	FrameEndArgs);

    /* Left is password list, Right is info about them. */
    rc1 = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	XmNorientation, XmHORIZONTAL,
	NULL);
    pane2 = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, rc1,
	XmNsashWidth, 1,
	XmNsashHeight, 1,
	XmNseparatorOn, False,
	NULL);
    w = XtVaCreateManagedWidget(catgets( catalog, CAT_MOTIF, 255, "Licenses found:" ), xmLabelGadgetClass, pane2,
	XmNalignment, XmALIGNMENT_BEGINNING,
	NULL);
    SetPaneMaxAndMin(w);

    foo = zmXmStr("                ");
    i = XtVaSetArgs(args, XtNumber(args),
	XmNlistSizePolicy, XmCONSTANT,
	XmNselectionPolicy, XmBROWSE_SELECT,
	XmNitems,		&foo,
	XmNitemCount,		1,
	NULL);
    license_list = XmCreateScrolledList(pane2, "license_list", args, i);
    XtManageChild(license_list);

    XtManageChild(pane2);

    XtVaSetValues(license_list,
	XmNitems, NULL,
	XmNitemCount, 0,
	NULL);

    rc2 = XtVaCreateWidget(NULL, xmPanedWindowWidgetClass, rc1,
	XmNseparatorOn,      False,
	XmNsashWidth,        1,
	XmNsashHeight,       1,
	NULL);

    {
	static Widget *ws[] = {
	    &progname_w,
	    &version_w,
	    &hostname_w,
	    &hostid_w,
	    &expire_w,
	    &max_users_w
	};
	static char *names[] = {
	    "progname",
	    "version",
	    "hostname",
	    "hostid",
	    "expiration",
	    "max_users"
	};
	for (i = 0; i < XtNumber(ws); i++) {
	    *ws[i] = CreateRJustLabeledText(names[i], rc2, NULL);
	    XtVaSetValues(*ws[i],
		XmNeditable, False,
		XmNcursorPositionVisible, False,
		XmNsensitive, False,
		NULL);
	}
    }

    XtManageChild(rc2);
    TurnOffSashTraversal(rc2);
    XtManageChild(rc1);

    user_label = XtVaCreateManagedWidget(catgets( catalog, CAT_MOTIF, 256, "Licensed Users:" ),
	xmLabelGadgetClass, pane,
	XmNalignment, XmALIGNMENT_BEGINNING,
	NULL);

    i = XtVaSetArgs(args, XtNumber(args),
	XmNlistSizePolicy, XmCONSTANT,
	XmNselectionPolicy, XmBROWSE_SELECT,
	NULL);
    entry_list = XmCreateScrolledList(pane, "entry_list", args, i);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(entry_list), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    XtAddCallback(license_list, XmNbrowseSelectionCallback,
	list_entry, entry_list);
    XtManageChild(entry_list);

    fill_license_list(NULL);

    if (cannot_write < 2) {
	passwd_textw = CreateLabeledText("passwd_textw", pane, NULL, True);
	XtAddCallback(passwd_textw, XmNactivateCallback, action_callback,
	    (XtPointer) NewPasswd);
	XtVaSetValues(XtParent(passwd_textw), XmNskipAdjust, True, NULL);

	user_textw = CreateLabeledText("user_textw", pane, NULL, True);
	XtAddCallback(user_textw,
	    XmNactivateCallback, action_callback, (XtPointer) AddUser);
	XtAddCallback(entry_list, XmNextendedSelectionCallback, add_to_textw,
	    user_textw);
	XtVaSetValues(XtParent(user_textw), XmNskipAdjust, True, NULL);
    }

    w = CreateActionArea(pane, btns, XtNumber(btns), "Registering Users");
    XtVaGetValues(w, XmNchildren, &action_area, NULL);
    if (cannot_write > 1) {
	XtSetSensitive(action_area[(int)AddUser], False);
	XtSetSensitive(action_area[(int)DeleteUser], False);
	XtSetSensitive(action_area[(int)UpdateEntries], False);
	XtSetSensitive(action_area[(int)NewPasswd], False);
    }
    FrameSet(license_frame,
		FrameDismissButton, action_area[(int)NewPasswd + 1],
		FrameEndArgs);

    FramePopup(license_frame);

    /* return license_frame;  See note above */
    return (ZmFrame)0;	/* Bart: Sat Jun 13 17:50:45 PDT 1992 */
}

static void
update_entry_info(entry)
license_data *entry;
{
    zmXmTextSetString(hostname_w, entry->hostname);
    zmXmTextSetString(hostid_w, zmVaStr("%x", entry->hostid));
    zmXmTextSetString(version_w, entry->version);
    zmXmTextSetString(progname_w, entry->progname ? entry->progname : "Z-Mail");
    if (entry->expiration_date == LICENSE_UNLIMITED)
	zmXmTextSetString(expire_w, catgets( catalog, CAT_MOTIF, 258, "Never Expires" ));
    else {
	long days_until_expiration =
			(entry->expiration_date - time(0)) / (60 * 60 * 24);
	zmXmTextSetString(expire_w, zmVaStr(days_until_expiration < 0
					    ? catgets( catalog, CAT_MOTIF, 259, "%d days ago" )
					    : catgets( catalog, CAT_MOTIF, 260, " ago" ),
					    abs(days_until_expiration)));
    }
    zmXmTextSetString(max_users_w, entry->max_users?
	zmVaStr("%d", entry->max_users) : catgets( catalog, CAT_MOTIF, 261, "Unlimited" ));
}

static void
list_entry(license_list, entry_list, cbs)
Widget license_list, entry_list;
XmListCallbackStruct *cbs;
{
    int i, nusers = 0;
    XmStringTable users = 0;

    thisentry = &entries[cbs->item_position-1];
    if (entries[cbs->item_position-1].max_users) {
	nusers = vlen( entries[cbs->item_position-1].valid_users);
	if (nusers)
	    users = ArgvToXmStringTable(nusers, entries[cbs->item_position-1].valid_users);
	if (cannot_write < 2) {
	    XtSetSensitive(user_textw, True);
	    XtSetSensitive(action_area[AddUser], True);
	    XtSetSensitive(action_area[DeleteUser], True);
	}
	for (i = 0; i < thisentry->max_users && thisentry->valid_users[i]; ++i)
	    ;
	XtVaSetValues(user_label, XmNlabelString,
	    zmXmStr(zmVaStr(catgets( catalog, CAT_MOTIF, 262, "%d users registered on %s" ),
		i, entries[cbs->item_position-1].hostname)),
	    NULL);
    } else {
	XtVaSetValues(user_label, XmNlabelString, zmXmStr(""), NULL);
	if (user_textw)
	    XtSetSensitive(user_textw, False);
	XtSetSensitive(action_area[(int)AddUser], False);
	XtSetSensitive(action_area[(int)DeleteUser], False);
    }

    if (user_textw)
	zmXmTextSetString(user_textw, NULL);
    XtVaSetValues(entry_list,
	XmNitems, users,
	XmNitemCount, nusers,
	NULL);
    if (nusers)
	XmStringFreeTable(users);
    update_entry_info(thisentry);
}

/* callback for all the items in the action area.  If the user can't
 * update the license database, then he's in browse-only mode and
 * this function will never be called.
 */
static void
action_callback(w, action)
Widget w;
LicenseAction action;
{
    char buf[64], *user, *passwd;
    long date, hostid;
    license_data *new;
    u_long nusers;
    int n, status;

    ask_item = w;
    if (action == NewPasswd) {
	char *first_err;
	license_data desired_entry;
	passwd = XmTextGetString(passwd_textw);
	if (!passwd || !*passwd) {
	    XtFree(passwd);
	    return;
	}
	ask_item = w;
	ls_err = NULL;
	if (ls_license(passwd, zmVersion(2),
		hostid = ls_gethostid(0), &nusers, &date) == -1) {
	    first_err = ls_err;
	    ls_err = NULL;
	    if (ls_license(passwd, zmVersion(2),
		    LICENSE_MAGIC, &nusers, &date) == -1)
		error(UserErrWarning, first_err);
	}
	if (!ls_err) {
	    desired_entry.password = passwd;
	    desired_entry.hostid = hostid;
	    desired_entry.progname = zmMainName();
	    desired_entry.version = zmVersion(2);
	    desired_entry.max_users = nusers;
	    desired_entry.expiration_date = date;
	    new = ls_find_or_make_entry(&desired_entry,
		    &entries, &nentries, FALSE);
	    if (!new)
		error(SysErrWarning, catgets( catalog, CAT_MOTIF, 263, "Cannot register new licenses" ));
	    else {
		XmListAddItemUnselected(license_list, zmXmStr(new->password),0);
		XmListSelectPos(license_list, 0, True);
	    }
	    DismissSetWidget(w, DismissClose);
	}
	XtFree(passwd);
	return;
    }
    if (action == UpdateEntries) {
	FILE *fp;
	if (ison(license_flags, NLS_LICENSE)) {
	    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 264, "Cannot update licenses over the network." ));
	    return;
	}
	if (!(fp = openlib(passwd_file, "w", TRUE))) {
	    error(SysErrWarning, catgets( catalog, CAT_MOTIF, 265, "Cannot update licenses" ));
	    return;
	}
	ls_wrentries(fp, entries, nentries);
	closelib(passwd_file, fp, TRUE);
	DismissSetWidget(w, DismissClose);
	return;
    }
    if (!(user = XmTextGetString(user_textw)) || !*user) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 266, "You must enter a name to %s" ),
	    action == AddUser? catgets( catalog, CAT_MOTIF, 267, "Add" ) : catgets( catalog, CAT_MOTIF, 268, "Delete" ));
	XtFree(user);
	return;
    }
    if (action == AddUser) {
	if (status = add_user_to_entry(user, thisentry, buf))
	    XmListAddItemUnselected(entry_list, zmXmStr(user), 0);
    } else if (action == DeleteUser) {
	XmStringTable list;
	int m, *pos;
	XtVaGetValues(entry_list, XmNselectedItems, &list, NULL);
	XmListGetSelectedPos(entry_list, &pos, &n);
	m = n;
	if (n == 1) {
	    /* take it from the text field */
	    if (status = delete_user_from_entry(user, thisentry, buf))
		XmListDeleteItem(entry_list, zmXmStr(user));
	    XtFree((char *)pos);
	} else if (n > 1) {
	    /* loop through all the selections */
	    while (n--) {
		XtFree(user);
		XmStringGetLtoR(list[n], xmcharset, &user);
		if (!delete_user_from_entry(user, thisentry, buf))
		    error(UserErrWarning, buf);
	    }
	    for (n = 0; n < m; n++)
		XmListDeletePos(entry_list, pos[n]);
	    XtFree((char *)pos);
	    status = 1;
	}
    }
    if (!status)
	error(UserErrWarning, buf);
    else {
	zmXmTextSetString(user_textw, NULL);
	DismissSetWidget(w, DismissClose);
    }

    if (action == AddUser || action == DeleteUser) {
	for (n = 0; n < thisentry->max_users && thisentry->valid_users[n]; ++n)
	    ;
	XtVaSetValues(user_label, XmNlabelString,
	    zmXmStr(zmVaStr(catgets( catalog, CAT_MOTIF, 262, "%d users registered on %s" ),
		n, thisentry->hostname)),
	    NULL);
    }
    XtFree(user);
}

static void
add_to_textw(list_w, text_w, cbs)
Widget list_w, text_w;
XmListCallbackStruct *cbs;
{
    char *text;
    if (XmStringGetLtoR(cbs->item, xmcharset, &text)) {
	SetTextString(text_w, text);
	XtFree(text);
    }
}

#else /* LICENSE_FREE */

ZmFrame
DialogCreateLicense(w)
Widget w;
{
    error(Message, catgets( catalog, CAT_MOTIF, 270, "This version of Z-Mail does not have licensing." ));
    return 0;
}

#endif /* LICENSE_FREE */
