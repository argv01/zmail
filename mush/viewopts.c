/* @(#)viewopts.c	(c) copyright	10/18/86 (Dan Heller) */

#include "mush.h"

struct viewopts {
    char *v_opt;
    char *v_prompt;
    char *v_description;
#ifdef SUNTOOL
    Panel_item v_choice;
    Panel_item v_text;
#endif /* SUNTOOL */
};

#ifdef SUNTOOL
short dat_cycle_cursor[] = {
    0x07C0, 0x0FE0, 0x1834, 0x301C, 0x601C, 0x203C, 0x0000, 0x0000,
    0x7808, 0x700C, 0x7018, 0x5830, 0x0FE0, 0x07C0, 0x0000, 0x0000

};
mpr_static(cycle,           16, 16, 1, dat_cycle_cursor);
#endif /* SUNTOOL */

/*
 * struct contains the option, a prompt if it has a string value, whether
 * or not it applies to non suntools, line mode, or both, and a
 * string describing what the option does. If the prompt string starts
 * with a minus sign, then the value can be set without a value. This
 * is there to indicate to option_line to print a toggle (cycle) pixrect
 * and to print TRUE/FALSE telling whether the value is on or off regardless
 * of it's "string" value.
 */
struct viewopts viewopts[] = {
    { "alwaysignore", NULL,
	"Always ignore the message headers on the 'ignored' list." },
    { "alwaysmime", NULL,
        "Always send MIME compliant messages, even if it's not needed."},
    { "ask", NULL,
	"Prompts for a subject on outgoing mail." },
    { "askcc", NULL,
	"Ask for list of Carbon Copy recipients whenever sending mail." },
    { "autodelete", NULL,
	"Automatically delete ALL READ messages whenever you update mail." },
    { "autoedit", NULL,
	"Automatically enter editor for REPLIES only (not toolmode)." },
    { "autoinclude", NULL,
	"Include a copy of author's message each time you reply to mail." },
    { "autoprint", NULL,
	"Display the next message on the list when you delete a message." },
    { "auto_route", "-Host/Path:",
	"Remove redundant uucp addresses when replying to messages." },
    { "autosign", "-Filename:",
	"Add file (~/.signature if set but no value) at end of all letters." },
    { "autosign2", "Addr:File:",
	"Signature to use for specific addresses. \"addr, ... : <signature>\""},
    { "cdpath", "Path:",
	"Path to search for directories when the \"cd\" command is issued." },
    { "cmd_help", "Path:",
	"Location of the general help file for line and curses modes." },
    { "complete", "Character:",
	"The character typed to cause a word completion to occur." },
    { "compose_icon", "-Filename:",
	"Alternate pixmap to use when compose window is closed to an icon." },
    { "crt", "Lines:",
	"The number of lines a message must have for 'pager' to be invoked." },
    { "crt_win", "Lines:",
	"Lines in the tool mode text subwindow for paging messages." },
    { "curses_help", "-Commands:",
	"List of curses commands whose bindings appear in the help display." },
    { "date_received", NULL,
	"Time displayed for message headers shows date received (or sent)." },
    { "delete_empty", NULL,
	"Remove the system folder on updates if it has no undeleted message." },
    { "dead", "Filename:",
	"The name of the file to store dead mail (default = ~/dead.letter)." },
    { "defaultbodytype", "-MIME type:",
        "Default MIME type for message body (if not set text/plain is used)." },
    { "domain_route", "-Host/Path:",
	"Cause short-circuiting of domain addresses when auto-routing." },
    { "dot", NULL,
	"Allow \".\" on a line by itself to send letter." },
    { "edit_hdrs", NULL,
	"Allow headers of messages to be edited using your editor." },
    { "editor", "Editor:",
	"Editor for message editing (default = env EDITOR or \"vi\")." },
    { "eightbits", NULL,
	"Echo untranslated \"control\" characters with the eight bit set." },
    { "escape", "Character:",
	"Escape character for extended editing commands (default = ~)." },
    { "fignore", "Patterns:",
	"Filename extensions or patterns ignored in completions." },
    { "folder", "Pathname:",
	"Full pathname to the directory where personal folders are kept." },
    { "fortune", "-Flag:",
	"Add fortune to end of letters.  Flag to \"fortune\" is optional." },
    { "fortunates", "Users:",
	"Those who will receive fortunes if fortune is set (default: All)." },
    { "hdr_format", "Format:",
	"Formatting string for headers.  \"headers -?\" or help hdr_format." },
    { "history", "Number:",
	"How many commands to remember (like csh)." },
    { "hold", NULL,
	"Read but not deleted messages are saved in spool -- not mbox." },
#ifdef POP3_SUPPORT
    { "holdpop", NULL,
	"Do not delete mail messages from the POP server." },
#endif
    { "home", "Directory:",
	"The user's home directory." },
    { "hostname", "Hostname:",
	"User-definable name for the name of your machine." },
    { "ignore_bang", NULL,
	"Ignore '!' as a history reference.  Otherwise, escape by: \\!" },
    { "ignoreeof", "-Command:",
	"Ignores ^D as exit, or (if set), execute \"command\"." },
    { "indent_str", "String:",
	"String to offset included messages within your letters." },
    { "in_reply_to", "-String:",
	"When responding to mail, add In-Reply-To: to message headers." },
    { "keepsave", NULL,
	"Prevents messages from being marked as `deleted' when you `save'." },
    { "known_hosts", "Host list:",
	"List of hosts that your site is known to uucp mail to." },
    { "logfile", "Filename:",
	"Log outgoing mail headers only.  Message text not logged." },
    { "mail_icon", "Filename:",
	"Alternate pixmap to use when tool is closed to an icon." },
#ifdef POP3_SUPPORT
    { "mailpass", "Passwd: ",
	"Password to give to the POP server." },
#endif
    { "mbox", "Filename:",
	"Filename to use instead of ~/mbox for default mailbox." },
    { "metamail", "-Path:",
        "Program displaing MIME compliant messages." },
    { "metasend", "-Path:",
        "Program providing MIME encoding." },
    { "metoo", NULL,
	"When replying to mail, metoo preserves your name on mailing list." },
    { "mil_time", NULL,
	"24-hour military time format is used whenever a time is printed." },
    { "mimetypes", "-Path:",
        "Path to mime.types file other than ~/.mimetypes and /etc/mime.types."},
    { "msg_win", "Lines:",
	"Number of lines in the message composition window for tool mode." },
    { "newline", "-Command:",
	"Ignore RETURN.  If set to a command, execute that command." },
    { "newmail_icon", "Filename:",
	"Alternate icon shown when new mail is available." },
    { "no_expand", NULL,
	"Prevents expansion of Mush aliases in outgoing mail headers." },
    { "no_hdrs", NULL,
	"If set, personalized headers are NOT inserted to outgoing mail." },
    { "no_reverse", NULL,
	"Disables reverse video in curses mode -- uses \"bold\" in tool mode."},
    { "nonobang", NULL,
	"Suppresses errors from unsuccessful history references." },
    { "nosave", NULL,
	"Prevents aborted mail from being saved in $dead." },
    { "output", NULL,
	"The message list produced as output of the last command." },
    { "pager", "Program:",
	"Program name to be used as a pager for messages longer than crt." },
    { "pre_indent_str", "String:",
	"String to precede message text interpolated into message body." },
#ifdef POP3_SUPPORT
    { "popmail", NULL,
	"Retrieve mail with the POP protocol." },
#endif
    { "post_indent_str", "String:",
	"String to succeed message text interpolated into message body." },
    { "print_cmd", "Program:",
	"Alternate program to use to send messages to the printer." },
    { "printer", "Printer:",
	"Printer to send messages to (default = environment PRINTER)." },
    { "prompt", "String:",
	"Your prompt.  Type \"help prompt\" for more information." },
    { "quiet", "-Conditions:",
	"Turn off verbose messages and error bells in various conditions." },
    { "realname", "Name:",
	"Your real name." },
    { "record", "Filename:",
	"Save all outgoing mail in specified filename." },
    { "reply_to_hdr", "Headers:",
	"List of headers use to construct reply addresses from a message." },
    { "save_empty", NULL,
	"Folders which have all messages deleted are not removed on updates." },
    { "screen", "# of Headers:",
	"Number of headers to print in non-suntools (text) mode." },
    { "screen_win", "# of Headers:",
	"Set the size of the header window for the tool mode only." },
    { "show_deleted", NULL,
	"Show deleted messages in headers listings (unused in curses mode)." },
    { "show_hdrs", "Headers:",
	"When displaying a message, show list of \"headers\" only." },
    { "sendmail", "Program:",
	"Program to use to deliver mail instead of using the default."},
    { "sort", "-Option:",
	"Pre-sorting of messages on mush startup (set to valid sort option)." },
    { "squeeze", NULL,
	"When reading messages, squeeze all blank lines into one." },
    { "status", NULL,
	"The success or failure status of the most recent command." },
    { "thisfolder", "Folder:",
	"This read-only variable gives the current folder name." },
    { "tool_help", "Path:",
	"Location of the help file for tool mode."  },
    { "toplines", "Lines:",
	"Number of lines to print of a message for the 'top' command."  },
    { "tmpdir", "Directory:",
	"Directory to use for temporary files used by Mush." },
    { "unix", NULL,
	"Non-mush commands are considered to be UNIX commands." },
    { "verify", NULL,
	"Verify before acting in various situations, such as sending mail." },
    { "visual", "Visual editor:",
	"Visual editor for messages (default = $editor or env VISUAL)."},
    { "warning", NULL,
	"Print warning messages for non-fatal errors." },
    { "wrap", NULL,
	"After referencing last message, message pointer wraps to start." },
    { "wrapcolumn", "-Column to wrap [78]:",
	"Column at which to wrap lines when composing messages." },
};

#ifdef SUNTOOL

#define OPTIONS_PANEL_WIDTH	550

int set_value(), toggle_value(), help_opt();

Frame opts_frame;
Panel opts_panel;
Panel_item desc_msg;
Panel_item file_text_item;

static void
frame_done()
{
#ifdef SUN_4_0 /* SunOS 4.0+ */
    window_set(opts_frame, WIN_SHOW, FALSE, NULL);
#else /* SUN_4_0 */
    /* not enough fd's to keep it lying around for SunOS 3.X */
    window_destroy(opts_frame);
    opts_frame = (Frame) 0;
#endif /* SUN_4_0 */
}

static void
opts_help()
{
    help(0, "options", tool_help);
}

static void
opts_save_load(item)
Panel_item item;
{
    int (*func)() = (int (*)())panel_get(item, PANEL_CLIENT_DATA);
    int result;
    char buf[MAXPATHLEN];
    char *argv[3], *file = panel_get_value(file_text_item);

    if (!*file) {
	result = (*func)(0, DUBL_NULL);
	file = MAILRC;
    } else {
	argv[1] = file;
	argv[2] = NULL;
	result = (*func)(2, argv);
    }
    switch (result) {
	case 0:
	    sprintf(buf, "%s %s",
		(func == source)? "Loaded options from" : "Saved options to",
		file);
	when -1:
	    sprintf(buf, "%s: %s", file, sys_errlist[errno]);
	when -2:
	    sprintf(buf, "%s is a directory.", file);
	when -3:
	    /* save_opts() returns -3 if user doesn't confirm overwrite */
	    strcpy(buf, "Save operation aborted.");
    }
    panel_set(desc_msg, PANEL_LABEL_STRING, buf, NULL);
}

static void
unset_opts()
{
    cmd_line("unset *", NULL);
}

static void
reset_opts()
{
    source(0, DUBL_NULL);
}

/*
 * Public routine which creates a subframe which contains two panels.
 * The first contains options for loading and saving options from a
 * file (text item) and so on... the second panel contains all the items
 * which correspond to each mush variable that exists.
 */
void
view_options()
{
    extern Notify_value fkey_interposer();
    register char *p;
    int count;

    if (opts_frame) {
	window_set(opts_frame, WIN_SHOW, TRUE, NULL);
	opts_panel_item(NULL);
	return;
    }
#ifdef SUN_3_5
    if (nopenfiles(0) < 3) {
	ok_box("Too many frames; close one first!\n");
	return;
    }
#endif /* SUN_3_5 */

    opts_frame = window_create(tool, FRAME,
	FRAME_DONE_PROC,	frame_done,
	FRAME_LABEL,		"Mush Options",
	FRAME_NO_CONFIRM,	TRUE,
	FRAME_SHOW_LABEL,	TRUE,
	WIN_WIDTH,		OPTIONS_PANEL_WIDTH,
	NULL);

    opts_panel = window_create(opts_frame, PANEL,
	WIN_WIDTH,		OPTIONS_PANEL_WIDTH,
	NULL);
    (void) notify_interpose_event_func(opts_panel,
	fkey_interposer, NOTIFY_SAFE);
    panel_create_item(opts_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(opts_panel, "Done", 4, mush_font),
	PANEL_NOTIFY_PROC,	frame_done,
	NULL);
    panel_create_item(opts_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(opts_panel, "Help", 4, mush_font),
	PANEL_NOTIFY_PROC,	opts_help,
	NULL);
    panel_create_item(opts_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(opts_panel, "Save", 4, mush_font),
	PANEL_NOTIFY_PROC,	opts_save_load,
	PANEL_CLIENT_DATA,	save_opts,
	NULL);
    panel_create_item(opts_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(opts_panel, "Load", 4, mush_font),
	PANEL_NOTIFY_PROC,	opts_save_load,
	PANEL_CLIENT_DATA,	source,
	NULL);
    panel_create_item(opts_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(opts_panel, "Clear", 5, mush_font),
	PANEL_NOTIFY_PROC,	unset_opts,
	NULL);
    panel_create_item(opts_panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(opts_panel, "Restart", 7, mush_font),
	PANEL_NOTIFY_PROC,	reset_opts,
	NULL);
    file_text_item = panel_create_item(opts_panel, PANEL_TEXT,
	PANEL_LABEL_STRING,	"Save/Load File:",
	PANEL_VALUE_DISPLAY_LENGTH, 30,
	NULL);
    desc_msg = panel_create_item(opts_panel, PANEL_MESSAGE,
	PANEL_LABEL_STRING,	"Help Descriptions -- Click on Variable Name",
	NULL);
    window_fit_height(opts_panel);

    /* reuse opts_panel -- we don't need the other one */
    opts_panel = window_create(opts_frame, PANEL,
	WIN_BELOW,			opts_panel,
	WIN_X,				0,
	WIN_COLUMN_GAP,			120,
	WIN_TOP_MARGIN,			10,
	WIN_LEFT_MARGIN,		10,
	WIN_WIDTH,			OPTIONS_PANEL_WIDTH,
	PANEL_VERTICAL_SCROLLBAR,	scrollbar_create(NULL),
	NULL);
    (void) notify_interpose_event_func(opts_panel,
	fkey_interposer, NOTIFY_SAFE);

    for (count = 0; count < ArraySize(viewopts); count++) {
	panel_create_item(opts_panel, PANEL_MESSAGE,
	    PANEL_ITEM_X,	ATTR_COL(0),
	    PANEL_ITEM_Y,	ATTR_ROW(count),
	    PANEL_LABEL_STRING,	viewopts[count].v_opt,
	    PANEL_NOTIFY_PROC,	help_opt,
	    PANEL_CLIENT_DATA,	count,
	    NULL);

	if (!(p = viewopts[count].v_prompt) || *p == '-') {
	    if (p && *p)
		p++;
	    viewopts[count].v_choice = panel_create_item(opts_panel,
		PANEL_CHOICE,
		PANEL_LABEL_IMAGE,	&cycle,
		PANEL_LAYOUT,		PANEL_HORIZONTAL,
		PANEL_CHOICE_STRINGS,	"False", "True", NULL,
		PANEL_DISPLAY_LEVEL,	PANEL_CURRENT,
		PANEL_ITEM_X,		ATTR_COL(1),
		PANEL_ITEM_Y,		ATTR_ROW(count),
		PANEL_NOTIFY_PROC,	toggle_value,
		PANEL_CLIENT_DATA,	count,
		NULL);
	}
	if (p) {
	    viewopts[count].v_text = panel_create_item(opts_panel, PANEL_TEXT,
		PANEL_VALUE_DISPLAY_LENGTH,	10,
		PANEL_VALUE_UNDERLINED,		TRUE,
		PANEL_LABEL_STRING,		p,
		PANEL_ITEM_X,			ATTR_COL(2),
		PANEL_ITEM_Y,			ATTR_ROW(count),
		PANEL_NOTIFY_PROC,		set_value,
		PANEL_CLIENT_DATA,		count,
		NULL);
	}
    }
    /* set the panel items' values */
    opts_panel_item(NULL);

    window_set(opts_panel,
	WIN_HEIGHT,	400,
	WIN_FIT_HEIGHT,	0,
	NULL);
    window_set(opts_frame, WIN_SHOW, TRUE, NULL);
}

/*
 * Sets the items in the panels to reflect that variable's value.
 * If "var" is NULL, do it for all the items.
 */
void
opts_panel_item(var)
char *var;
{
    int count;
    char *value;

    if (!opts_frame)
	return;

    for (count = 0; count < ArraySize(viewopts); count++) {
	if (var && strcmp(var, viewopts[count].v_opt))
	    continue;
	value = do_set(set_options, viewopts[count].v_opt);

	if (!viewopts[count].v_prompt || *viewopts[count].v_prompt == '-')
	    panel_set_value(viewopts[count].v_choice, value != NULL);
	if (viewopts[count].v_prompt)
	    panel_set_value(viewopts[count].v_text, value? value : "");
	if (var)
	    break;
    }
}

/*
 * Callback for choice items -- for variables that have boolean settings.
 * CLIENT_DATA is the index in the viewopts array.
 */
static
toggle_value(item, value)
Panel_item item;
int value;
{
    int count = (int) panel_get(item, PANEL_CLIENT_DATA);
    char *p, *argv[4];
    char *text_value = NULL;

    if (check_internal(viewopts[count].v_opt)) {
	panel_set(desc_msg, PANEL_LABEL_STRING,
	    "This is an internal variable which cannot be changed.",
	    NULL);
	/* can't change it - restore previous setting */
	panel_set_value(viewopts[count].v_choice, !value);
	return -1;
    }

    if (p = viewopts[count].v_prompt) /* set equal */
	text_value = panel_get_value(viewopts[count].v_text);

    if (!value) {
	if (un_set(&set_options, viewopts[count].v_opt) == -1) {
	    /* can't change it - restore previous setting */
	    panel_set_value(viewopts[count].v_choice, !value);
	    return -1;
	}
    } else {
	/* Turn it on if it's entirely boolean or bool/str, but no str value */
	if (!p || text_value && !*text_value) {
	    argv[0] = viewopts[count].v_opt; /* it's a boolean */
	    argv[1] = NULL;
	} else {
	    /* string value -- determine the text from the typed in value */
	    argv[0] = viewopts[count].v_opt;
	    argv[1] = "=";
	    argv[2] = text_value;
	    argv[3] = NULL;
	}
	if (add_option(&set_options, argv) != 1) {
	    /* can't change it - restore previous setting */
	    panel_set_value(viewopts[count].v_choice, !value);
	    return -1;
	}
    }

    if (!strcmp(viewopts[count].v_opt, "no_reverse") ||
	!strcmp(viewopts[count].v_opt, "show_deleted"))
	do_hdrs(0, DUBL_NULL, NULL);

    return 0;
}

/* callback for text items -- set vars to the string typed. */
static
set_value(item, event)
Panel_item item;
Event *event;
{
    int count = (int)panel_get(item, PANEL_CLIENT_DATA);
    char *p, *argv[4], *value;

    if (event_id(event) == '\t')
	return (int) PANEL_NEXT;

    p = viewopts[count].v_prompt;
    value = panel_get_value(item);

    if (check_internal(viewopts[count].v_opt)) {
	panel_set(desc_msg, PANEL_LABEL_STRING,
	    "This is an internal variable which cannot be changed.",
	    NULL);
	return (int) PANEL_NONE;
    }

    /*
     * You can "unset" string-only values by entering a blank string.
     * If the "prompt" starts with a -, then you can only "unset" the
     * variable by setting the associated choice item to false.
     */
    if (*p != '-' && !*value) {
	(void) un_set(&set_options, viewopts[count].v_opt);
	return (int) PANEL_NONE; /* do not advance caret */
    }
    /* Turn it on, but not to a value */
    if (!*value) {
	argv[0] = viewopts[count].v_opt; /* it's a boolean */
	argv[1] = NULL;
    } else {
	/* string value -- determine the text from the typed in value */
	argv[0] = viewopts[count].v_opt;
	argv[1] = "=";
	argv[2] = value;
	argv[3] = NULL;
    }

    if (add_option(&set_options, argv) == 1 && p && *p == '-')
	panel_set_value(viewopts[count].v_choice, TRUE);

    return (int) PANEL_NONE;
}

/* when user clicks on variable label itself */
static
help_opt(item, event)
Panel_item item;
Event *event;
{
    int count = (int)panel_get(item, PANEL_CLIENT_DATA);

    panel_set(desc_msg,
	PANEL_LABEL_STRING, viewopts[count].v_description,
	NULL);
    return 0;
}

#endif /* SUNTOOL */

/*
 * return a string describing a variable.
 * parameters: count, str, buf.
 * If str != NULL, check str against ALL variables
 * in viewopts array.  The one that matches, set count to it and 
 * print up all the stuff from the viewopts[count] into the buffer
 * space in "buf" and return it.
 */
char *
variable_stuff(count, str, buf)
register char *str, *buf;
{
    if (str)
	for (count = 0; count < ArraySize(viewopts); count++)
	    if (!strcmp(str, viewopts[count].v_opt))
		break;
    if (count >= ArraySize(viewopts)) {
	if (str && !*str) {
	(void) sprintf(buf, "You must give a default %s variable name (?all lists them all).",
	    prog_name);
	} else {
	(void) sprintf(buf, "%s: Not a default %s variable.",
			   str? str : itoa(count), prog_name);
	}
	return NULL;
    }
    return sprintf(buf, "%s: %s",
	viewopts[count].v_opt,
	viewopts[count].v_description);
}
