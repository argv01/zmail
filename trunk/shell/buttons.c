/* buttons.c     Copyright 1990, 1991, 1992, 1993 Z-Code Software Corp. */

#ifndef lint
static char	buttons_rcsid[] = "$Id: buttons.c,v 2.80 1996/04/09 21:32:29 schaefer Exp $";
#endif

#include "zmail.h"
#include "zmcomp.h"
#include "buttons.h"
#include "linklist.h"
#include "vars.h"
#include "fsfix.h"
#include "hashtab.h"
#include "catalog.h"
#include "cparse.h"
#include "strcase.h"

char *button_flags[][2] = {
    { "-no-msgs",		"-n" },
    { "-window-context",	"-W" },
    { "-window",		"-w" },
    { "-buttonlist-context",	"-B" },
    { "-buttonlist",    	"-b" },
    { "-focus-condition",       "-f" },
    { "-menulist-context",	"-B" },
    { "-menulist",              "-b" },
    { "-relabel",		"-r" },
    { "-menu",			"-m" },
    { "-position",		"-p" },
    { "-list",			"-l" },
    { "-label",                 "-L" },
    { "-delete",		"-d" },
    { "-sensitivity",     	"-s" },
    { "-mnemonic",      	"-M" },
    { "-accelerator",   	"-a" },
    { "-separator",     	"-S" },
    { "-name",          	"-N" },
    { "-help-menu",     	"-h" },
    { "-right-justify",     	"-h" },
    { "-value",                 "-v" },
    { "-popup-context",         "-U" },
    { "-popup",                 "-u" },
    { "-invisible",		"-i" },
    { "-final",			"-F" },
    { "-icon",			"-I" },
    { "-on-icon",		"-o" },
    { "-script",		"-z" },
    { "-type",		 	"-t" },
    { NULL, NULL }
};

char *window_names[WINDOW_COUNT] = {
    "main", "message", "compose", "zcal"
};

char *button_panels[PANEL_COUNT] = {
    "MainActions", "MessageActions", "ComposeActions", "ZCalActions",
    "MainMenu", "MessageMenu", "ComposeMenu", "ZCalMenu",
    "ToolboxItems",
    "MainToolbar", "MessageToolbar", "ComposeToolbar",
    "ComposeAddressOptions",
    "MainSummariesPopupMenu",
    "OutputStatictextPopupMenu",
    "ReadMessageBodyPopupMenu",
    "CommandlineAFPopupMenu",
    "ComposeBodyPopupMenu"
};

char *popup_widgets[] = {
    "main-summaries",
    "output-statictext",
    "read-message-body",
    "commandline-af",
    "compose-body",
    NULL
};

char *button_types[] = {
    "pushbutton",
    "toggle",
    "separator",
    "submenu",
    "folder-popup",
    "command-line",
    "windows-menu",
    "priority-popup",
    "message-field",
    NULL
};

static int btn_handle_compar(), btn_name_compar();
static ZmButton copy_buttons();
static int relabel_button P((cpData,char*,ZmButtonList,char**));
static int list_remove_buttons P((int,cpData,char*,ZmButtonList,char**));
static int list_all_buttons P((char*,ZmButtonList));
static int is_menubar P((char*));
static void print_context P((char*,char*,int));

#define find_btn_handle(l) \
    (ZmButton)find_link((struct link *)BListButtons(blist), l, btn_handle_compar)
#define find_btn_name(l) \
    (ZmButton)find_link((struct link *)BListButtons(blist), l, btn_name_compar)

/* All buttons are associated with a window somewhere.  The "win_context"
 * variable is static to this file, and is always reset to the "main" window
 * upon each call.  The -window option to zm_button resets the context to a
 * new state for the duration of the call.  -W/-window-context changes the
 * state permanently (or until the next call using -W/-window-context).
 */
#define DEFAULT_BUTTON_CONTEXT "MainActions"
#define DEFAULT_MENU_CONTEXT "MainMenu"

static char *bl_context;
static char global_button_context[64] = DEFAULT_BUTTON_CONTEXT;
static char global_menu_context[64] = DEFAULT_MENU_CONTEXT;

/* cache buttonlist pointer for button/menu context to save time */
static ZmButtonList global_button_context_list,
       		    global_menu_context_list;

#ifdef ZMAIL_BASIC
#include "defmenus.h"
#endif /* ZMAIL_BASIC */

cpDescRec button_descs[] = {
    { 'n', CP_POSITIONAL, 0, },
    { 'w', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'W', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'b', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'B', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'u', CP_STR_ARG, 0, },
    { 'U', CP_STR_ARG, 0, },
    { 'f', CP_STR_ARG, 0, },
    { 't', CP_STR_ARG, 0, },
    { 'm', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'p', CP_INT_ARG, 0, },
    { 'L', CP_STR_ARG, 0, },
    { 's', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'M', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'a', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'N', CP_STR_ARG, 0, },
    { 'v', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'I', CP_STR_ARG, 0, },
    { 'o', CP_STR_ARG, 0, },
    { 'z', CP_STR_ARG|CP_OPT_ARG, 0, },
    { 'l', 0, 0, },
    { 'r', 0, 0, },
    { 'd', 0, 0, },
    { 'q', 0, 0, },
    { 'S', CP_POSITIONAL, 0 },
    { 'h', 0, BT_HELP, },
    { 'i', 0, BT_INVISIBLE, },
    { 'F', 0, BT_FINAL, },
    { 0, 0, 0 }
};

/* zm_button -- front end for button and menu commands.
 * button [-n] label z-script  (any number of args)
 *	creates a new button, or modifies "label" button
 * button -r oldlabel newlabel (newlabel may have spaces; any number of args)
 *
 * button [-window message|compose|main] [-m menu-id] string zscript
 * button -window-context [message|compose|main]
 * button -list [-window windowname] [label] [label2 ... labelN]
 *	lists all buttons in all windows if no -window options given
 * button -window-context windowname
 *	changes default window for all subsequent button commands that
 *	don't specify -window to "windowname".
 *
 * To delete specific buttons by name:
 * button -delete button-name -window message|compose|main|* [button-name ...]
 *    or
 * unbutton button-name -window message|compose|main|* [button-name ...]
 */

int
zm_button(argc, argv, list)
    int argc;
    char **argv;
    struct mgroup *list;
{
    cpDataRec cpdata;
    int opt, iarg;
    char *sarg = NULL;
    char *cmd;
    char newname[256];
    ZmButton button = 0, b;
    zmButton oldb;
    ZmButtonList blist;
    char *global_context,
         *object_type,
	 *name,
	 *label;
    
    int blist_slot = -1;
    int n, i, created = 0, menu_cmd = 0;

    cmd = *argv;
    if (index(cmd, 'm')) menu_cmd = 1;
    if (!cpParseOpts(&cpdata, argv, button_descs, button_flags, 0))
	return -1;
    argv = cpdata.argv;
    if (isupper(*cmd))
	cpAddOptBool(&cpdata, cpGetDesc(&cpdata, 'n'), cpOn);
    else if (*cmd == 'u')
	cpAddOptBool(&cpdata, cpGetDesc(&cpdata, 'd'), cpOn);
    global_context = (menu_cmd) ? global_menu_context : global_button_context;
    object_type = (menu_cmd) ? "menu" : "button";
    bl_context = global_context;
    blist = (menu_cmd) ? global_menu_context_list : global_button_context_list;

#ifdef ZMAIL_BASIC
    if (menu_cmd) {
	print(catgets(catalog, CAT_SHELL, 831, "%s: not supported in this version.\n"), cmd);
	return -1;
    }
    if (opt = cpHasOpts(&cpdata, "wWuUbB")) {
	print(catgets(catalog, CAT_SHELL, 832, "%s: -%c: not supported in this version.\n"), cmd, opt);
	return -1;
    }
#else /* !ZMAIL_BASIC */
    if (opt = cpHasOpts(&cpdata, "wW")) {
	if (!(sarg = cpGetOpt(&cpdata, opt))) {
	    print_context(global_context, object_type, TRUE);
	    return 0;
	}
	for (i = 0; i < WINDOW_COUNT; i++)
	    if (!ci_strcmp(sarg, window_names[i])) break;
	if (i == WINDOW_COUNT) {
	    print(catgets(catalog, CAT_SHELL, 833, "%s: invalid window name: %s\n"), cmd, sarg);
	    return -1;
	}
	blist_slot = i+((menu_cmd) ? WINDOW_MENU_OFFSET :
	    WINDOW_BUTTON_OFFSET);
	bl_context = button_panels[blist_slot];
	blist = (ZmButtonList) 0;
    }
    if (opt = cpHasOpts(&cpdata, "uU")) {
	sarg = cpGetOpt(&cpdata, opt);
	for (i = 0; popup_widgets[i]; i++)
	    if (!strcmp(sarg, popup_widgets[i])) break;
	if (!popup_widgets[i]) {
	    print(catgets( catalog, CAT_SHELL, 68, "%s: -popup: bad item name: %s\n" ), cmd, sarg);
	    return -1;
	}
	blist_slot = i+POPUP_MENU_OFFSET;
	bl_context = button_panels[blist_slot];
	blist = (ZmButtonList) 0;
    }
    if (opt = cpHasOpts(&cpdata, "bB")) {
	bl_context = cpGetOptStr(&cpdata, opt);
	if (!bl_context) {
	    print_context(global_context, object_type, FALSE);
	    return 0;
	}
	blist = (ZmButtonList) 0;
	if (blist_slot != -1) {
	    if (strcmp(bl_context, button_panels[blist_slot])) {
#ifdef GUI
		if (istool > 1)
		    gui_install_all_btns(blist_slot, bl_context, 0);
		else
#endif
		    ZSTRDUP(button_panels[blist_slot], bl_context);
	    }
	}
    }
#endif /* !ZMAIL_BASIC */

    if (!blist)
	blist = GetButtonList(bl_context);
    if (cpHasOpts(&cpdata, "BUW")) {
	strcpy(global_context, bl_context);
	if (menu_cmd)
	    global_menu_context_list = blist;
	else
	    global_button_context_list = blist;
    }

    /* if no label or name specified yet, it must be the next argument */
    if (*argv && !cpHasOpts(&cpdata, "LN"))
	cpAddOpt(&cpdata, cpGetDesc(&cpdata, 'L'), *argv++);

    if (cpOptIsOn(&cpdata, 'l') ||
	(!*argv && !cpOtherOpts(&cpdata, "LNwWuUbB")))
	return list_remove_buttons(TRUE, &cpdata, cmd, blist, argv);

    /* if we are changing system state, make a copy first, so we can
     * do saveopts correctly.
     */
    if (ison(BListFlags(blist), BTL_SYSTEM_STATE)) {
	turnoff(BListFlags(blist), BTL_SYSTEM_STATE);
	BListSysButtons(blist) = copy_buttons(BListButtons(blist));
    }

    if (cpOptIsOn(&cpdata, 'r'))
	return relabel_button(&cpdata, cmd, blist, argv);

    if (cpOptIsOn(&cpdata, 'd'))
	return list_remove_buttons(FALSE, &cpdata, cmd, blist, argv);
    
    if (name = cpGetOptStr(&cpdata, 'N'))
	button = find_btn_name(name);
    else {
	if ((label = cpGetOptStr(&cpdata, 'L')) &&
	    (button = find_btn_handle(label)))
	    name = ButtonName(button);
	if (!name) {
	    name = label;
	    if (!name) {
		error(UserErrWarning, catgets(catalog, CAT_SHELL, 847, "Need a name or a label."));
		return -1;
	    }
	}
    }
    if (cpOptIsOn(&cpdata, 'q'))
	return (button) ? 0 : 1;

    if (!*argv && !button && !cpHasOpts(&cpdata, "Smvt")) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 75, "%s: not enough arguments" ), cmd);
	return -1;
    }

    make_widget_name(name, newname);
    if (strcmp(name, newname)) name = newname;
    
    if (!button) {
	/* Create a new one */
	if (!(button = new_button())) {
	    print(catgets( catalog, CAT_SHELL, 77, "%s: cannot create new %s.\n" ), cmd, object_type);
	    return -1;
	}
	ButtonName(button) = savestr(name);
	if (menu_cmd) turnon(ButtonFlags(button), BT_MENU_CMD);
	turnon(ButtonFlags(button), BT_REQUIRES_SELECTED_MSGS);
	ButtonParent(button) = BListName(blist);
	ButtonType(button) = BtypePushbutton;
	created = TRUE;
    } else {
#define xsavestr(string)  ((string) ? savestr(string) : 0)
	/* save some flags for gui_update_button */
	ButtonFlags(&oldb) = ButtonFlags(button);
	ButtonMnemonic(&oldb) = ButtonMnemonic(button);
	ButtonAccelText(&oldb) = xsavestr(ButtonAccelText(button));
	ButtonSubmenu(&oldb) = xsavestr(ButtonSubmenu(button));
	ButtonType(&oldb) = ButtonType(button);
    }

    while (cpNextOpt(&cpdata, &opt, &sarg, &iarg)) {
	switch (opt) {
	case 's':
	    if (ButtonSenseCond(button)) {
		DestroyDynCondition(ButtonSenseCond(button));
		ButtonSenseCond(button) = 0;
	    }
#ifdef GUI
	    if (sarg && !(ButtonSenseCond(button) =
		  CreateDynCondition(sarg, gui_update_button, (char *) button)))
		return -1;
#endif /* GUI */
	when 'v':
	    if (ButtonValueCond(button)) {
		DestroyDynCondition(ButtonValueCond(button));
		ButtonValueCond(button) = 0;
	    }
	    if (sarg) {
		ButtonType(button) = BtypeToggle;
#ifdef GUI
		if (!(ButtonValueCond(button) =
		  CreateDynCondition(sarg, gui_update_button, (char *) button)))
		    return -1;
#endif /* GUI */
	    } else {
		if (ButtonType(button) == BtypeToggle)
		    ButtonType(button) = BtypePushbutton;
	    }
	when 'f':
	    if (ButtonFocusCond(button))
		DestroyDynCondition(ButtonFocusCond(button));
#ifdef GUI
	    if (!(ButtonFocusCond(button) =
		  CreateDynCondition(sarg, gui_update_button, button)))
		return -1;
#endif /* GUI */
	when 'p':
	    if (!created && iarg != ButtonPosition(button)) {
		error(UserErrWarning,
		    catgets( catalog, CAT_SHELL, 78, "Moving %ss among positions is not implemented yet!" ),
		    object_type);
		return -1;
	    }
	when 'L': ZSTRDUP(ButtonLabel(button), sarg);
	when 'a':
	    xfree(ButtonAccelText(button));
	    ButtonAccelText(button) = xsavestr(sarg);
	when 'M': ButtonMnemonic(button) = (sarg) ? *sarg : 0;
	when 'I': ZSTRDUP(ButtonIcon(button), sarg);
	when 'o': ZSTRDUP(ButtonOnIcon(button), sarg);
	when 't': {
	    for (i = 0; button_types[i]; i++)
		if (!ci_strcmp(sarg, button_types[i]))
		    break;
	    if (!button_types[i]) {
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 884, "%s: unrecognized button type: %s"),
		    cmd, button_types[i]);
		return -1;
	    }
	    ButtonType(button) = (ZmButtonType) i;
	}
	when 'm':
	    xfree(ButtonSubmenu(button));
	    ButtonSubmenu(button) = xsavestr(sarg);
	    if (sarg) {
		ButtonType(button) = BtypeSubmenu;
	    } else if (ButtonType(button) == BtypeSubmenu) {
		ButtonType(button) = BtypePushbutton;
	    }
	when 'S':
	    if (iarg == cpOn) {
		ButtonType(button) = BtypeSeparator;
	    } else if (ButtonType(button) == BtypeSeparator) {
		ButtonType(button) = BtypePushbutton;
	    }
	when 'z':
	    if (sarg)
		ZSTRDUP(ButtonScript(button), sarg);
	    else {
		xfree(ButtonScript(button));
		ButtonScript(button) = NULL;
	    }
	when 'n':
	    if (iarg == cpOn)
		turnoff(ButtonFlags(button), BT_REQUIRES_SELECTED_MSGS);
	    else
		turnon(ButtonFlags(button), BT_REQUIRES_SELECTED_MSGS);
	}
    }
    turnon(ButtonFlags(button), cpGetFlagsOn(&cpdata));
    turnoff(ButtonFlags(button), cpGetFlagsOff(&cpdata));
    if (*argv) {
#ifdef NOT_NOW
	/*
	 * Note that this condition only catches an *explcit*
	 * request for a submenu in this same command.  If there
	 * is simply a submenu left over from an earlier command,
	 * it seems reasonable to discard that submenu silently.
	 */
	if (cpOptIsOn(&cpdata, 'm'))
	    /* warn about discarding submenu request */ ;
#endif /* NOT_NOW */
	xfree(ButtonSubmenu(button));
	xfree(ButtonScript(button));
	ButtonScript(button) = joinv(NULL, argv, " ");
	if (ButtonType(button) == BtypeSubmenu)
	    ButtonType(button) = BtypePushbutton;
    }
    
    if (created) {
	int position = cpGetOptInt(&cpdata, 'p');
	
#ifndef _WINDOWS
	if (!ButtonSubmenu(button) && is_menubar(bl_context)) {
	    error(UserErrWarning,
		catgets( catalog, CAT_SHELL, 76, "Any %s installed in a menubar must have a submenu." ),
		object_type);
	    return -1;
	}
#endif /* !_WINDOWS */
	/* insert in sorted order */
	n = number_of_links(BListButtons(blist)) + 1;
	if (ison(ButtonFlags(button), BT_FINAL))
	    position = n;
	else if (position <= 0) {
	    ZmButton last;
	    last = BListButtons(blist);
	    position = n;
	    while (position > 1) {
		last = prev_button(last);
		if (isoff(ButtonFlags(last), BT_FINAL)) break;
		position--;
	    }
	} else if (position > n)
	    position = n;
	ButtonPosition(button) = position;
	if (position == n) {
	    insert_link(&BListButtons(blist), button);
	} else {
	    if (position == 1)
		push_link(&BListButtons(blist), button);
	    else {
		b = (ZmButton)retrieve_nth_link(BListButtons(blist), position);
		push_link(&b, button);
	    }
	    b = next_button(button);
	    for (position++; position <= n; b = next_button(b))
		ButtonPosition(b) = position++;
	}
#ifdef GUI
	if (istool > 1) gui_install_button(button, blist);
#endif /* GUI */
	    
    } else {
#ifdef GUI
	if (istool > 1) gui_update_button(button, (ZmCallbackData) 0, &oldb);
#endif /* GUI */
	xfree(ButtonAccelText(&oldb));
	xfree(ButtonSubmenu(&oldb));
    }
    return 0;
}

static int
relabel_button(cpdata, cmd, blist, argv)
cpData cpdata;
char *cmd;
ZmButtonList blist;
char **argv;
{
    const char *label;
    char *relabel;
    int opt;
    ZmButton button;

    if (opt = cpOtherOpts(cpdata, "wWuUbBrL"))
	print(catgets(catalog, CAT_SHELL, 834, "%s: warning: -%c option ignored\n"), cmd, opt);
    label = cpGetOptStr(cpdata, 'L');
    if (!label && !(label = *argv++)) {
	print(catgets( catalog, CAT_SHELL, 61, "%s: -r: missing old label argument.\n" ), cmd);
	return -1;
    }
    if (!*argv) {
	print(catgets( catalog, CAT_SHELL, 62, "%s: -r: missing new label argument.\n" ), cmd);
	return -1;
    }
    relabel = joinv(NULL, argv, " ");
    if (!(button = find_btn_handle(label))) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 74, "%s: -r: cannot find \"%s\"" ), cmd, label);
	return -1;
    }
    if (ButtonLabel(button) && !strcmp(relabel, ButtonLabel(button))) {
	xfree(relabel);
	return 0;
    }
    xfree(ButtonLabel(button));
    ButtonLabel(button) = relabel;
#ifdef GUI
    gui_update_button(button, (ZmCallbackData) 0, button);
#endif /* GUI */
    return 0;
}

static int
list_remove_buttons(listing, cpdata, cmd, blist, argv)
int listing;
cpData cpdata;
char *cmd;
ZmButtonList blist;
char **argv;
{
    char *name, *label;
    ZmButton b;
    int opt;
    
    if (opt = cpOtherOpts(cpdata, "wWuUbBNLld"))
	print(catgets(catalog, CAT_SHELL, 835, "%s: warning: -%c option ignored\n"), cmd, opt);
    name = cpGetOptStr(cpdata, 'N');
    label = cpGetOptStr(cpdata, 'L');
    if (!name && !label && !*argv) {
	/* if all user did was change context or set a window
	 * buttonlist, that's ok
	 */
	if (cpHasOpts(cpdata, "WUB")) return 0;
	if (cpHasOpts(cpdata, "bB") && cpHasOpts(cpdata, "wWuU")) return 0;
	if (!listing) {
	    print(catgets(catalog, CAT_SHELL, 836, "%s: not enough arguments\n"), cmd);
	    return -1;
	}
	return list_all_buttons(cmd, blist);
    }
    if (label && !strcmp(label, "*")) {
	while (b = BListButtons(blist))
	    free_button(&BListButtons(blist), &b);
	return 0;
    }
    while (name || label) {
	b = (name) ? find_btn_name(name) : find_btn_handle(label);
	if (!b) {
	    if (ison(glob_flags, WARNINGS))
		print(catgets(catalog, CAT_SHELL, 837, "%s: cannot find \"%s\"\n"), cmd, name ? name :label);
	} else {
	    if (listing)
		print_button_info(b);
	    else
		free_button(&BListButtons(blist), &b);
	}
	if (!*argv) break;
	name = NULL;
	label = *argv++;
    }
    return 0;
}

static int
list_all_buttons(cmd, blist)
char *cmd;
ZmButtonList blist;
{
    int n;
    int ct = number_of_links(BListButtons(blist));

    for (n = 1; n <= ct; n++)
	print_button_info(
	    (ZmButton) retrieve_nth_link(BListButtons(blist), n));
    return 0;
}

static void
print_context(ctxt, obj, win)
char *ctxt, *obj;
int win;
{
    int i;

    if (win)
	for (i = 0; i != WINDOW_COUNT*2; i++) {
	    if (!strcmp(ctxt, button_panels[i])) {
		print(catgets(catalog, CAT_SHELL, 838, "Current window context for %ss: %s\n"), obj,
		    window_names[i % WINDOW_COUNT]);
		return;
	    }
	}
    print(catgets(catalog, CAT_SHELL, 839, "Current context for %ss: %s\n"), obj, ctxt);
    return;
}

static int
is_menubar(s)
char *s;
{
    int i;
    
    for (i = 0; i != WINDOW_COUNT; i++)
	if (!strcmp(button_panels[i+WINDOW_MENU_OFFSET], s))
	    return 1;
    return 0;
}

void
free_button(list, button)
ZmButton *list, *button;
{
    int n = ButtonPosition(*button);
    ZmButton b, old_list;

    b = next_button(*button); /* get the "next" button */
    old_list = *list;
    remove_link(list, *button);
    if (b != old_list)
	do
	    ButtonPosition(b) = n++; /* renumber the positions */
	while ((b = next_button(b)) != *list);
#ifdef GUI
    /* button should be out of the list before calling gui_remove_button,
     * so menu dialog list is updated correctly.  pf Wed Sep  1 15:26:43 1993
     */
    if (istool > 1)
	gui_remove_button(*button);
#endif /* GUI */
    xfree(ButtonName(*button));
    xfree(ButtonLabel(*button));
    xfree(ButtonScript(*button));
    xfree(ButtonAccel(*button));
    xfree(ButtonAccelText(*button));
    xfree(ButtonSubmenu(*button));
    DestroyDynCondition(ButtonSenseCond(*button));
    DestroyDynCondition(ButtonValueCond(*button));
    DestroyDynCondition(ButtonFocusCond(*button));
    xfree((char *)*button);
    *button = 0;
}

/* return 0 on success, non-zero on failure */
static int
btn_handle_compar(a, b)
char *a;
ZmButton b;
{
    if (!a || !GetButtonHandle(b)) return 1;
    return strcmp(a, GetButtonHandle(b));
}

/* return 0 on success, non-zero on failure */
static int
btn_name_compar(a, b)
char *a;
ZmButton b;
{
    if (!a || !ButtonName(b)) return 1;
    return strcmp(a, ButtonName(b));
}

void
print_button_info(b)
ZmButton b;
{
    int ismenu;

    ismenu = ison(ButtonFlags(b), BT_MENU_CMD);
    print(catgets( catalog, CAT_SHELL, 80, "%s:\n    Label: %s\n    Name: %s\n    Window: %s\n    Position: %d\n" ),
	  ismenu ? catgets( catalog, CAT_SHELL, 81, "Menu" ) : catgets( catalog, CAT_SHELL, 82, "Button" ),
	  ButtonLabel(b) ? ButtonLabel(b) : catgets(catalog, CAT_SHELL, 822, "(none)"),
	  ButtonName(b)  ? ButtonName(b)  : catgets(catalog, CAT_SHELL, 822, "(none)"),
	  ButtonParent(b), ButtonPosition(b));
    if (ButtonSubmenu(b))
	print(catgets( catalog, CAT_SHELL, 83, "    Menu: %s\n" ), ButtonSubmenu(b));
    if (ButtonScript(b))
	printf("    Z-Script: %s\n", ButtonScript(b));
}

char *
blist_spec(slot, name, butcmd, use_def)
int slot, butcmd, use_def;
char *name;
{
    static char buf[200];
    int offset, win;
    char *object_type = (butcmd) ? "button" : "menu";

    if (name)
	for (slot = 0; slot < PANEL_COUNT; slot++)
	    if (!strcmp(button_panels[slot], name)) break;
    if (slot >= PANEL_COUNT || slot < 0)
	return ((!name) ? "" :
		(sprintf(buf, " -%slist %s", object_type, name), buf));
    offset = (butcmd) ? WINDOW_BUTTON_OFFSET : WINDOW_MENU_OFFSET;
    if (use_def && slot == MAIN_WINDOW+offset) return "";
    win = slot-offset;
    if (win >= 0 && win < WINDOW_COUNT) {
	sprintf(buf, " -window %s", window_names[win]);
	return buf;
    }
    if (slot >= POPUP_MENU_OFFSET) {
	sprintf(buf, " -popup %s",
		popup_widgets[slot-POPUP_MENU_OFFSET]);
	return buf;
    }
    sprintf(buf, " -%slist %s", object_type, button_panels[slot]);
    return buf;
}

void
dump_button_info(fp, b)
FILE *fp;
ZmButton b;
{
    int ismenu;
    char *listname;
    char wname[200];

    ismenu = ison(ButtonFlags(b), BT_MENU_CMD);
    fprintf(fp, (ismenu) ? "menu" : "button");
    if (isoff(ButtonFlags(b), BT_REQUIRES_SELECTED_MSGS))
	fprintf(fp, " -no-msgs");
    listname = ButtonParent(b);
    fprintf(fp, "%s", blist_spec(0, listname, !ismenu, TRUE));
    if (ButtonMnemonic(b))
	fprintf(fp, " -mnemonic %c", ButtonMnemonic(b));
    if (ButtonAccelText(b))
	fprintf(fp, " -accelerator \"%s\"", quotezs(ButtonAccelText(b), '"'));
    if (ButtonIcon(b))
	fprintf(fp, " -icon \"%s\"", quotezs(ButtonIcon(b), '"'));
    if (ButtonOnIcon(b))
	fprintf(fp, " -on-icon \"%s\"", quotezs(ButtonOnIcon(b), '"'));
    switch (ButtonType(b)) {
    case BtypePushbutton:
    case BtypeToggle:
	break;
    case BtypeSeparator:
	fprintf(fp, " -separator");
	break;
    case BtypeSubmenu:
	fprintf(fp, " -menu %s", ButtonSubmenu(b));
	break;
    default:
	fprintf(fp, " -type %s", button_types[(int) ButtonType(b)]);
	break;
    }
    if (ison(ButtonFlags(b), BT_HELP))
	fprintf(fp, " -help-menu");
    if (ButtonSenseCond(b))
	fprintf(fp, " -sensitivity '%s'",
		quotezs(DynCondExpression(ButtonSenseCond(b)), '\''));
    if (ButtonValueCond(b))
	fprintf(fp, " -value '%s'",
		quotezs(DynCondExpression(ButtonValueCond(b)), '\''));
    if (ButtonLabel(b)) {
	int wrote_dash_label = False;
	if (ButtonName(b)) {
	    make_widget_name(ButtonLabel(b), wname);
	    if (strcmp(ButtonName(b), wname)) {
		fprintf(fp, " -name %s -label", ButtonName(b));
		wrote_dash_label = True;
	    }
	}
	if (!wrote_dash_label && *ButtonLabel(b) == '-')
	    fprintf(fp, " -label");
        fprintf(fp, " \"%s\"", quotezs(ButtonLabel(b), '"'));
    } else
	fprintf(fp, " -name %s", ButtonName(b));
    if (ButtonScript(b))
	fprintf(fp, " \"%s\"", quotezs(ButtonScript(b), '"'));
    fprintf(fp, "\n");
}

static int
check_sys_strings(s1, s2)
char *s1, *s2;
{
    if (!s1 && !s2) return TRUE;
    if (s1 && s2) return strcmp(s1, s2) == 0;
    return FALSE;
}

static int
check_sys_cond(dc, ds)
DynCondition dc, ds;
{
    char *s = (char *) ds;

    if (!dc && !s) return TRUE;
    if (dc && s) return strcmp(DynCondExpression(dc), s) == 0;
    return FALSE;
}

static int
check_sys_button(b, sb)
ZmButton b, sb;
{
    if (!b && !sb) return TRUE;
    if (!b || !sb) return FALSE;
    if ((ButtonFlags(b) & BT_ATTRIBUTE_FLAGS) !=
	(ButtonFlags(sb) & BT_ATTRIBUTE_FLAGS))
	return FALSE;
    if (ButtonType(b) != ButtonType(sb)) return False;
    if (ButtonParent(b) != ButtonParent(sb)) return FALSE;
    if (ButtonMnemonic(b) != ButtonMnemonic(sb)) return FALSE;
    if (!check_sys_strings(ButtonName(b), ButtonName(sb)))
	return FALSE;
    if (!check_sys_strings(ButtonIcon(b), ButtonIcon(sb)))
	return FALSE;
    if (!check_sys_strings(ButtonOnIcon(b), ButtonOnIcon(sb)))
	return FALSE;
    if (!check_sys_strings(ButtonLabel(b), ButtonLabel(sb)))
	return FALSE;
    if (!check_sys_strings(ButtonAccelText(b), ButtonAccelText(sb)))
	return FALSE;
    if (!check_sys_strings(ButtonSubmenu(b), ButtonSubmenu(sb)))
	return FALSE;
    if (!check_sys_strings(ButtonScript(b), ButtonScript(sb)))
	return FALSE;
    if (!check_sys_cond(ButtonSenseCond(b), ButtonSenseCond(sb)))
	return FALSE;
    if (!check_sys_cond(ButtonValueCond(b), ButtonValueCond(sb)))
	return FALSE;
    return TRUE;
}

static char **system_button_panels;

void
print_all_button_info(fp, button_flag, all)
FILE *fp;
int button_flag, all;
{
    struct hashtab_iterator hti;
    ZmButtonList bl;
    ZmButton b, sb;
    int i;
    char *object_type;

    object_type = (button_flag) ? "button" : "menu";
    for (i = 0; i != PANEL_COUNT; i++) {
	if (!all && (system_button_panels &&
		!strcmp(button_panels[i], system_button_panels[i])))
	    continue;
	if (button_flag && i >= WINDOW_MENU_OFFSET) continue;
	if (!button_flag && i < WINDOW_MENU_OFFSET) continue;
	fprintf(fp, "%s%s -%slist %s\n", object_type,
		blist_spec(i, NULL, button_flag, FALSE), object_type,
		button_panels[i]);
    }
    hashtab_InitIterator(&hti);

#define null_next_button(X, Y) \
    ((next_button(X)==(Y))?(ZmButton)0:next_button(X))

    while (bl = (ZmButtonList) hashtab_Iterate(&ButtonTable, &hti)) {
	if (!all && ison(BListFlags(bl), BTL_SYSTEM_STATE)) continue;
	b = BListButtons(bl);
	sb = all ? (zmButton *)0 : BListSysButtons(bl);
	if (b && !ison(ButtonFlags(b), BT_MENU_CMD) != button_flag) continue;
	while (b || sb) {
	    if (check_sys_button(b, sb)) {
		b = null_next_button(b, BListButtons(bl));
		sb = null_next_button(sb, BListSysButtons(bl));
	    } else if (!sb) {
		dump_button_info(fp, b);
		b = null_next_button(b, BListButtons(bl));
	    } else {
		fprintf(fp, "un%s%s -name \"%s\"\n", object_type,
			blist_spec(0, BListName(bl), button_flag, TRUE),
			quotezs(ButtonName(sb), '"'));
		sb = null_next_button(sb, BListSysButtons(bl));
	    }
	}
    }
}

void
stow_system_buttons()
{
    struct hashtab_iterator hti;
    ZmButtonList bl;
    int i;

    hashtab_InitIterator(&hti);
    while (bl = (ZmButtonList) hashtab_Iterate(&ButtonTable, &hti))
	turnon(BListFlags(bl), BTL_SYSTEM_STATE);
    system_button_panels = (char **) calloc(sizeof *system_button_panels, PANEL_COUNT);
    for (i = 0; i < PANEL_COUNT; i++)
	ZSTRDUP(system_button_panels[i], button_panels[i]);
}

ZmButtonList
GetButtonList(name)
char *name;
{
    zmButtonList p;
    ZmButtonList ret;

    if (!name) {
	Debug("GetButtonList called with null name\n");
	return (ZmButtonList) 0;
    }
    p.name = name;
    if ((ret = hashtab_Find(&ButtonTable, &p)) != 0) return ret;
    bzero((char *) &p, sizeof p);
    p.name = savestr(name);
    p.resource = "menu_bar"; /*menu_resource(name);*/

#ifdef VUI
    dlist_Init(&(p.copylist), (sizeof (struct blistCopyElt)), 4);
#endif /* VUI */

    hashtab_Add(&ButtonTable, &p);
    return GetButtonList(name);
}

struct hashtab ButtonTable;

static int
buttonlist_hash(elt)
ZmButtonList elt;
{
    return hashtab_StringHash(elt->name);
}

static int
buttonlist_cmp(a, b)
ZmButtonList a, b;
{
    return ci_strcmp(a->name, b->name);
}

static ZmButton
copy_buttons(b)
ZmButton b;
{
    ZmButton new, list = (ZmButton) 0;
    ZmButton first = b;

    if (!b) return (ZmButton) 0;
    do {
	new = (ZmButton) calloc(sizeof *new, 1);
	if (ButtonLabel(b)) ZSTRDUP(ButtonLabel(new), ButtonLabel(b));
	if (ButtonName(b)) ZSTRDUP(ButtonName(new), ButtonName(b));
	ButtonFlags(new) = ButtonFlags(b);
	ButtonParent(new) = ButtonParent(b);
	ButtonType(new) = ButtonType(b);
	if (ButtonScript(b))
	    ZSTRDUP(ButtonScript(new), ButtonScript(b));
	ButtonMnemonic(new) = ButtonMnemonic(b);
	if (ButtonAccelText(b))
	    ZSTRDUP(ButtonAccelText(new), ButtonAccelText(b));
	if (ButtonIcon(b))
	    ZSTRDUP(ButtonIcon(new), ButtonIcon(b));
	if (ButtonOnIcon(b))
	    ZSTRDUP(ButtonOnIcon(new), ButtonOnIcon(b));
	if (ButtonSubmenu(b))
	    ZSTRDUP(ButtonSubmenu(new), ButtonSubmenu(b));
	/* all we need here is the expression text, not all the other stuff
	 * that comes with a DynCond, so we just use the expression.
	 */
	if (ButtonSenseCond(b))
	    ButtonSenseCond(new) = (DynCondition)
		   savestr(DynCondExpression(ButtonSenseCond(b)));
	if (ButtonValueCond(b))
	    ButtonValueCond(new) = (DynCondition)
		   savestr(DynCondExpression(ButtonValueCond(b)));
	insert_link(&list, new);
    } while ((b = next_button(b)) != first);
    return list;
}

char *
GetButtonHandle(b)
ZmButton b;
{
    return ButtonLabel(b) ? ButtonLabel(b) : ButtonName(b);
}


#ifdef MAC_OS
# include "zminit.seg"
#endif /* MAC_OS */
void
InitButtonLists()
{
#ifdef ZMAIL_BASIC
    ZmButton b, first, last;
    ZmButtonList bl;
#endif /* ZMAIL_BASIC */
    int i;

    /* move all the button panel strings to the heap, so we can
     * xfree() safely later.
     */
    for (i = 0; i != PANEL_COUNT; i++)
	button_panels[i] = savestr(button_panels[i]);
    hashtab_Init(&ButtonTable,
		 (unsigned int (*) P((CVPTR))) buttonlist_hash,
		 (int (*) P((CVPTR, CVPTR))) buttonlist_cmp,
		 sizeof(zmButtonList), 37);
#ifdef ZMAIL_BASIC
    /* prepare the hardcoded buttons for actual use.
     * set up the link structures, and create all the dynconditions.
     */
    b = def_buttons;
    for (; ButtonName(b); b++) {
	first = b;
	bl = GetButtonList(ButtonParent(first));
	BListButtons(bl) = first;
	for (; ButtonName(b); b++) {
	    ButtonParent(b) = ButtonParent(first);
#ifdef GUI
	    if (ButtonSenseCond(b))
		ButtonSenseCond(b) =
		    CreateDynCondition((char *) ButtonSenseCond(b),
			gui_update_button, b);
	    if (ButtonValueCond(b))
		ButtonValueCond(b) =
		    CreateDynCondition((char *) ButtonValueCond(b),
			gui_update_button, b);
	    if (ButtonFocusCond(b))
		ButtonFocusCond(b) =
		    CreateDynCondition((char *) ButtonFocusCond(b),
			gui_update_button, b);
#else /* !GUI */
	    ButtonSenseCond(b) = ButtonValueCond(b) =
		ButtonFocusCond(b) = NULL;
#endif /* !GUI */
	    b->b_link.l_next = (struct link *) (b+1);
	    b->b_link.l_prev = (struct link *) (b-1);
	}
	last = b-1;
	last->b_link.l_next = (struct link *) first;
	first->b_link.l_prev = (struct link *) last;
    }
#endif /* ZMAIL_BASIC */
}
