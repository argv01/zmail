/* @(#) misc_frame.c	(c) copyright	9/29/89 (Dan Heller) */

/*
 * This file contains several functions which create dialog box frames
 * for (currently) mail aliases and ignored headers.  Each dialog box
 * has a list of some kind and a way to add or delete items from the
 * list.  The list is a textsw which is updated (currently) by do_set().
 * Public routines:
 *    update_list_textsw(struct options **) updates the textsw list.
 *    do_alias()	creates the alias dialog frame box
 *    do_ignore()	creates the ignored headers dialog frame box
 */

#include "mush.h"

extern Notify_value fkey_interposer();

/****************** Mail Aliases ********************/

Frame	alias_frame;
Panel_item alias_msg, alias_name, alias_value, alias_list_textsw;
static void set_alias();

Frame	ignore_frame;
Panel_item ignore_msg, ignore_name, ignore_list_textsw;
static Panel_setting set_ignore();

#define MY_FRAME_WIDTH	600

static void
frame_help(item)
Panel_item item;
{
    (void) help(0, panel_get(item, PANEL_CLIENT_DATA), tool_help);
}

void
update_list_textsw(list)
struct options **list;
{
    Textsw save = pager_textsw;

    if (list == &aliases)
	pager_textsw = alias_list_textsw;
    else if (list == &ignore_hdr)
	pager_textsw = ignore_list_textsw;
    else
	/* no textsw for this guy yet */
	return;

    if (pager_textsw && !!window_get(pager_textsw, WIN_SHOW))
	(void) do_set(*list, NULL);
    pager_textsw = save;
}

static void
alias_done()
{
    window_destroy(alias_frame);
    alias_frame = (Frame) 0;
}

void
do_aliases()
{
    Panel	panel;

    if (alias_frame) {
	window_set(alias_frame, WIN_SHOW, TRUE, NULL);
	return;
    }
#ifdef SUN_3_5
    if (nopenfiles(0) < 5) {
	print("Too many frames; close one first!\n");
	return;
    }
#endif /* SUN_3_5 */

    alias_frame = window_create(tool, FRAME,
	FRAME_SHOW_LABEL,	TRUE,
	FRAME_LABEL,		"Mail Aliases",
	FRAME_NO_CONFIRM,	TRUE,
	FRAME_DONE_PROC,	alias_done,
	WIN_SHOW,		TRUE,
	WIN_WIDTH,		MY_FRAME_WIDTH,
	NULL);

    panel = window_create(alias_frame, PANEL,
	PANEL_WIDTH,		MY_FRAME_WIDTH,
	NULL);
    notify_interpose_event_func(panel, fkey_interposer, NOTIFY_SAFE);

    panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Help", 4, mush_font),
	PANEL_CLIENT_DATA,	"aliases",
	PANEL_NOTIFY_PROC,	frame_help,
	NULL);
    panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Set", 3, mush_font),
	PANEL_NOTIFY_PROC,	set_alias,
	PANEL_CLIENT_DATA,	TRUE,
	NULL);
    panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Unset", 5, mush_font),
	PANEL_NOTIFY_PROC,	set_alias,
	PANEL_CLIENT_DATA,	FALSE,
	NULL);

    alias_msg = panel_create_item(panel, PANEL_MESSAGE,
	PANEL_LABEL_STRING,
	    "Type name of alias and address list and select <set> or <unset>",
	NULL);

    alias_name = panel_create_item(panel, PANEL_TEXT,
	PANEL_LABEL_STRING,	"Alias Name:",
	PANEL_VALUE_DISPLAY_LENGTH, 60,
	NULL);
    alias_value = panel_create_item(panel, PANEL_TEXT,
	PANEL_LABEL_STRING,	"Alias Address(es):",
	PANEL_VALUE_DISPLAY_LENGTH, 60,
	NULL);
    window_fit_height(panel);

    alias_list_textsw = window_create(alias_frame, TEXTSW,
	WIN_BELOW,			panel,
	WIN_WIDTH,			MY_FRAME_WIDTH,
	WIN_HEIGHT,			15 * l_height(),
#ifdef SUN_4_0 /* SunOS 4.0+ */
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_WORD,
#else /* SUN_4_0 */
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_CHAR,
#endif /* SUN_4_0 */
	NULL);
    (void) notify_interpose_event_func(alias_list_textsw,
	fkey_interposer, NOTIFY_SAFE);

    window_fit_height(alias_frame);
    update_list_textsw(&aliases);
}

static void
set_alias(item)
Panel_item item;
{
    int argc, set_it = (int)panel_get(item, PANEL_CLIENT_DATA);
    char buf[BUFSIZ], **argv, *name, *value;

    name = panel_get_value(alias_name);
    if (!*name) {
	panel_set(alias_msg, PANEL_LABEL_STRING, "Need an alias name.", NULL);
	return;
    }
    if (any(name, " \t")) {
	panel_set(alias_msg,
	    PANEL_LABEL_STRING, "Alias name may not contain spaces.",
	    NULL);
	return;
    }
    if (set_it) {
	value = panel_get_value(alias_value);
	if (!*value) {
	    panel_set(alias_msg,
		PANEL_LABEL_STRING, "Specify alias address(es).",
		NULL);
	    return;
	}
	sprintf(buf, "alias %s %s", name, value);
    } else
	sprintf(buf, "unalias %s", name);
    if (!(argv = mk_argv(buf, &argc, TRUE)) || do_alias(argc, argv) == -1)
	panel_set(alias_msg,
	    PANEL_LABEL_STRING, "Couldn't set alias.",
	    NULL);
    else
	panel_set(alias_msg,
	    PANEL_LABEL_STRING, "",
	    NULL);
    panel_set_value(alias_name, "");
    panel_set_value(alias_value, "");
    free_vec(argv);
}

/* int cuz it's also the callback for the text item */
static Panel_setting
set_ignore(item)
Panel_item item;
{
    int argc, set_it = (int)panel_get(item, PANEL_CLIENT_DATA);
    char buf[BUFSIZ], *name, **argv;

    name = panel_get_value(ignore_name);
    if (!*name) {
	panel_set(ignore_msg, PANEL_LABEL_STRING, "Missing header name.", NULL);
	return PANEL_NONE;
    }
    if (set_it)
	sprintf(buf, "ignore %s", name);
    else
	sprintf(buf, "unignore %s", name);
    /* set() will call update_list_textsw() */
    if (!(argv = mk_argv(buf, &argc, TRUE)) || set(argc, argv, NULL) == -1)
	panel_set(ignore_msg,
	    PANEL_LABEL_STRING, "Internal Error!?",
	    NULL);
    else
	panel_set(ignore_msg,
	    PANEL_LABEL_STRING, "",
	    NULL);
    free_vec(argv);
    panel_set_value(ignore_name, "");
    return PANEL_NONE;
}

static void
ignore_done()
{
    window_destroy(ignore_frame);
    ignore_frame = (Frame) 0;
}

void
do_ignore()
{
    Panel	panel;

    if (ignore_frame) {
	window_set(ignore_frame, WIN_SHOW, TRUE, NULL);
	return;
    }
#ifdef SUN_3_5
    if (nopenfiles(0) < 5) {
	print("Too many frames; close one first!\n");
	return;
    }
#endif /* SUN_3_5 */

    ignore_frame = window_create(tool, FRAME,
	FRAME_SHOW_LABEL,	TRUE,
	FRAME_LABEL,		"Ignored Headers",
	FRAME_NO_CONFIRM,	TRUE,
	FRAME_DONE_PROC,	ignore_done,
	WIN_SHOW,		TRUE,
	WIN_WIDTH,		MY_FRAME_WIDTH,
	NULL);

    panel = window_create(ignore_frame, PANEL,
	PANEL_WIDTH,		MY_FRAME_WIDTH,
	NULL);
    (void) notify_interpose_event_func(panel, fkey_interposer, NOTIFY_SAFE);
    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	
	    panel_button_image(panel, "Help", 4, mush_font),
	PANEL_NOTIFY_PROC,	frame_help,
	PANEL_CLIENT_DATA,	"ignore",
	NULL);
    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	
	    panel_button_image(panel, "Set", 3, mush_font),
	PANEL_NOTIFY_PROC,	set_ignore,
	PANEL_CLIENT_DATA,	TRUE,
	NULL);
    panel_create_item(panel, PANEL_BUTTON,
	PANEL_LABEL_IMAGE,	
	    panel_button_image(panel, "Unset", 5, mush_font),
	PANEL_NOTIFY_PROC,	set_ignore,
	PANEL_CLIENT_DATA,	FALSE,
	NULL);

    ignore_msg = panel_create_item(panel, PANEL_MESSAGE,
	PANEL_LABEL_STRING,
	    "Type name of header to ignore and then <set> or <unset>",
	NULL);

    ignore_name = panel_create_item(panel, PANEL_TEXT,
	PANEL_LABEL_STRING,	"Ignored Header:",
	PANEL_NOTIFY_PROC,	set_ignore,
	PANEL_CLIENT_DATA,	1,
	PANEL_VALUE_DISPLAY_LENGTH, 60,
	NULL);
    window_fit_height(panel);

    ignore_list_textsw = window_create(ignore_frame, TEXTSW,
	WIN_BELOW,		panel,
	WIN_WIDTH,		MY_FRAME_WIDTH,
	WIN_HEIGHT,		15 * l_height(),
#ifdef SUN_4_0 /* SunOS 4.0+ */
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_WORD,
#else /* SUN_4_0 */
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_CHAR,
#endif /* SUN_4_0 */
	NULL);
    (void) notify_interpose_event_func(ignore_list_textsw,
	fkey_interposer, NOTIFY_SAFE);

    window_fit_height(ignore_frame);
    update_list_textsw(&ignore_hdr);
}
