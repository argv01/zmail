/* @(#)tool.c	(c) copyright	10/15/86 (Dan Heller) */

/* tool.c --make the mailtool windows, panels, etc... */
#include "mush.h"

#ifndef FONTDIR
#define FONTDIR		  "/usr/lib/fonts/fixedwidthfonts/"
#endif /* FONTDIR */

extern void print_sigwinch(), make_hdr_sw();
extern Notify_value fkey_interposer();

static void init_cursors(), geticon();

extern Panel /* panels.c */
    make_hdr_panel(), make_main_panel();

Panel
    main_panel,		/* the main panel dealing with generic items */
    hdr_panel;		/* panel which contains message header specific items */

Textsw mfprint_sw;
Frame compose_frame;
int compose_destroy = 0;

static char **choice_args, **button_args;

short dat_mail_icon_1[] = {
#include "mail.icon.1"
};

short dat_mail_icon_2[] = {
#include "mail.icon.2"
};

short dat_compose_icon[] = {
#include "compose.icon"
};

short dat_coffee_cup[] = {
    0x0200,0x0100,0x0600,0x0800,0x0600,0x0100,0xFFF8,0x800C,
    0x800A,0x4012,0x401C,0x2020,0x9048,0x7FF0,0x3FE0,0x0000
};

mpr_static(mail_icon_image1, 64, 64, 1, dat_mail_icon_1);
mpr_static(mail_icon_image2, 64, 64, 1, dat_mail_icon_2);
mpr_static(compose_icon_image, 64, 64, 1, dat_compose_icon);

mpr_static(coffee_cup,      16, 16, 1, dat_coffee_cup);

/* public for hdr_sw.c */
Cursor l_cursor, m_cursor, r_cursor;
static Cursor coffee;

/* text and font will be set in mail_status() */
Icon mail_icon, compose_icon;

static Notify_value scroll_hdr();

make_tool()
{
    Rect	mrect;       /* Position and size of icon label. */
    struct stat rootbuf, tmpbuf;
    char	*p;
    struct pixfont *pf_open();

    if (p = do_set(set_options, "font")) {
	char buf[MAXPATHLEN];
	(void) sprintf(buf, "%s%s", FONTDIR, p);
	if (!(mush_font = pf_open(buf)))
	    print("couldn't open font \"%s\"\nUsing default font.\n", buf);
    }
    if (!mush_font)
	mush_font = pf_default();

    geticon();
    check_icons();

    if (p = do_set(set_options, "screen_win"))
	screen = atoi(p);
    else
	screen = 6;

    /* where to place text on mail icon -- how many messages there are */
    mrect.r_left = l_width();
    mrect.r_top = 58-l_height();
    mrect.r_width = 3*l_width();
    mrect.r_height = l_height();
    (void) icon_set(mail_icon, ICON_LABEL_RECT, &mrect, NULL);

    (void) window_set(tool,
	FRAME_ICON,	mail_icon,
	WIN_CONSUME_KBD_EVENTS,
	    WIN_LEFT_KEYS, WIN_TOP_KEYS, WIN_RIGHT_KEYS, NULL,
	NULL);
    (void) notify_interpose_destroy_func(tool, destroy_proc);

    choice_args = panel_make_list(
	PANEL_MENU_TITLE_FONT, mush_font,
	PANEL_DISPLAY_LEVEL,	PANEL_NONE,
	PANEL_SHOW_MENU,	TRUE,
	PANEL_SHOW_MENU_MARK,	FALSE,
	NULL);

    button_args = panel_make_list(
	PANEL_FEEDBACK,		PANEL_INVERTED,
	PANEL_SHOW_MENU,	FALSE,
	NULL);

    hdr_panel = make_hdr_panel(tool, choice_args, button_args);

    make_hdr_sw(tool);

    main_panel = make_main_panel(tool, choice_args, button_args);

    /* main mush frame text subwindow for wprint() */
    mfprint_sw = window_create(tool, TEXTSW,
	WIN_BELOW,                      main_panel,
	WIN_HEIGHT,                     l_height() * 4,
	TEXTSW_READ_ONLY,               TRUE,
	TEXTSW_BLINK_CARET,             FALSE,
	TEXTSW_LINE_BREAK_ACTION,       TEXTSW_WRAP_AT_CHAR,
	NULL);
    /* order is important -- scroll_textwin should be called first! */
    (void) notify_interpose_event_func(mfprint_sw,
	fkey_interposer, NOTIFY_SAFE);
    (void) notify_interpose_event_func(mfprint_sw,
	scroll_textwin, NOTIFY_SAFE);

    /* text subwindow for paging messages */
    p = do_set(set_options, "crt_win");
    pager_textsw = window_create(tool, TEXTSW,
	WIN_HEIGHT,			l_height() * (p? atoi(p) : 12),
	TEXTSW_READ_ONLY,		TRUE,
	TEXTSW_BLINK_CARET,		FALSE,
#ifdef SUN_4_0 /* SunOS 4.0+ */
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_WORD,
#else /* SUN_4_0 */
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_CHAR,
#endif /* SUN_4_0 */
	NULL);
    /* order is important -- scroll_textwin should be called first! */
    (void) notify_interpose_event_func(pager_textsw,
	fkey_interposer, NOTIFY_SAFE);
    (void) notify_interpose_event_func(pager_textsw,
	scroll_textwin, NOTIFY_SAFE);

    (void) sprintf(blank, "%*c", sizeof blank - 1, ' ');
    mrect = *(Rect *)window_get(hdr_sw, WIN_RECT);
    pw_writebackground(hdr_win, 0,0, mrect.r_width, mrect.r_height, PIX_CLR);
    istool = 2;
    (void) fclose(stdin);
    (void) fclose(stdout);
#ifndef SUN_3_5
    /* SunOS 3.5 takes tty modes for ttysws from stderr. */
    (void) fclose(stderr);
#endif /* SUN_3_5 */
    (void) do_version();
    init_cursors();
    timeout_cursors(TRUE);
    window_fit_height(tool);
}

void
close_frame()
{
    if (do_set(set_options, "compose_icon")) {
	icon_set(compose_icon, ICON_IMAGE, &compose_icon_image, NULL);
	window_set(compose_frame, FRAME_CLOSED, TRUE, NULL);
    } else
	window_set(compose_frame, WIN_SHOW, FALSE, NULL);
}

void
make_compose_frame()
{
    char *p;
    Textsw msg_sw;
    Panel panel, make_compose_panel();
    Notify_value compose_destroy_proc();

#ifdef SUN_3_5
    if (nopenfiles(0) < 8) {
	ok_box("Too many frames; close one, please.\n");
	compose_frame = 0;
	return;
    }
#endif /* SUN_3_5 */

    if (do_set(set_options, "compose_icon"))
	compose_frame = window_create(NULL, FRAME,
	    FRAME_LABEL,		"Compose Letter",
	    FRAME_SHOW_LABEL,	TRUE,
	    FRAME_NO_CONFIRM,	TRUE,
	    FRAME_ICON,		compose_icon,
	    WIN_SHOW,		TRUE,
	    NULL);
    else
	compose_frame = window_create(tool, FRAME,
	    FRAME_LABEL,		"Compose Letter",
	    FRAME_SHOW_LABEL,	TRUE,
	    FRAME_NO_CONFIRM,	TRUE,
	    FRAME_DONE_PROC,	close_frame,
	    WIN_SHOW,		TRUE,
	    NULL);
    (void) notify_interpose_destroy_func(compose_frame, compose_destroy_proc);

    panel = make_compose_panel(compose_frame, choice_args, button_args);

    /* text subwindow for composing messages */
    p = do_set(set_options, "msg_win");
    msg_sw = window_create(compose_frame, TEXTSW,
	WIN_BELOW,			panel,
	WIN_HEIGHT,			l_height() * (p? atoi(p) : 24),
	TEXTSW_READ_ONLY,		TRUE, /* set to false later */
	TEXTSW_BLINK_CARET,		FALSE,
	TEXTSW_LINE_BREAK_ACTION,	TEXTSW_WRAP_AT_CHAR,
	TEXTSW_IGNORE_LIMIT,		TEXTSW_INFINITY,
	NULL);
    notify_interpose_event_func(msg_sw, fkey_interposer, NOTIFY_SAFE);
    notify_interpose_event_func(msg_sw, edit_msg_textwin, NOTIFY_SAFE);

    /* Assign textsw (msg_sw) to panel for panel items' callbacks */
    panel_set(panel, PANEL_CLIENT_DATA, msg_sw, NULL);

    /* tty subwindow */
    if (!(tty_sw = window_create(compose_frame, TTY,
	TTY_QUIT_ON_CHILD_DEATH, FALSE,
	TTY_ARGV,		TTY_ARGV_DO_NOT_FORK,
	WIN_CLIENT_DATA,	msg_sw,
	WIN_SHOW,		FALSE,
	WIN_WIDTH,		WIN_EXTEND_TO_EDGE,
	WIN_BELOW,		msg_sw,
	NULL)))
	perror("tty_sw"), cleanup(0);

    /* catch SIGTERM when tty_sw dies */
    (void) signal(SIGTERM, catch);
    window_fit_height(compose_frame);
}

/*ARGSUSED*/
void
open_compose()
{
    if (compose_frame) {
	if (do_set(set_options, "compose_icon"))
	    window_set(compose_frame, FRAME_CLOSED, FALSE, NULL);
	else
	    window_set(compose_frame, WIN_SHOW, TRUE, NULL);
    } else
	make_compose_frame();
}

void
destroy_compose()
{
    if (!compose_destroy && compose_frame)
	window_destroy(compose_frame);
}

/*ARGSUSED*/
Notify_value
compose_destroy_proc(frame, status)
Frame frame;
Destroy_status status;
{
    if (!compose_destroy && compose_frame) {
	compose_destroy++;
	if (ison(glob_flags, IS_GETTING))
	    rm_edfile(-1);
    }
    return notify_next_destroy_func(frame, status);
}

Panel
get_compose_panel()
{
    Panel panel;

#ifdef SUN_4_0
    if (do_set(set_options, "compose_icon"))
	panel = (Panel)window_get(compose_frame, FRAME_NTH_WINDOW, 0);
    else
	panel = (Panel)window_get(compose_frame, FRAME_NTH_WINDOW, 1);
#else
    panel = (Panel)window_get(compose_frame, FRAME_NTH_WINDOW, 0);
#endif /* SUN_4_0 */
    return panel;
}

void
close_compose(item, value, event)
Panel_item item;
int value;
Event *event;
{
    if (event_id(event) == MS_LEFT) {
	close_frame();
	return;
    }
    switch (value) {
	case 0:
	    close_frame();
	when 1:
	    destroy_compose();
	when 2:
	    (void) help(0, "close", tool_help);
    }
}

parse_tool_opts(argcp, argv)
int *argcp;
char **argv;
{
    if (!(tool = window_create((Window) 0, FRAME,
	FRAME_ARGC_PTR_ARGV, argcp, argv,
	NULL)))
	cleanup(0);
}

/*
 * used by both the hdr_sw (to scroll canvas) and by textsw's.
 * Return values:
 *  MUSH_SCROLL_TO (to scroll _to_ a particular location)
 *  MUSH_SCROLL_RELATIVE (scroll relative our current location)
 *  MUSH_SCROLL_IGNORE (not a scroll event and ignore it entirely (up events))
 *  MUSH_SCROLL_PASS_EVENT (not a scroll event; pass it to next event handler)
 * If absolute scrolling (scroll_to) then "amount" is set to 1 for beginning
 * of textsw or 2 for end of textsw.
 * User can precede a keyboard scrolling request by a "count" -- the
 * count is typed by the user in digits: 5j = move 5 lines down.
 * It is assumed that if the user moves the mouse after a digit is pressed,
 * then he doesn't really want to adjust the scrolling amount and the 
 * count is reset to the default (1).
 */
Scroll_action
decode_scroll(client, event, len, amount)
Notify_client client;
Event	*event;
int len, *amount;
{
    static int count;

    if (event_id(event) == LOC_MOVE) {
	count = 0;
	return MUSH_SCROLL_PASS_EVENT;
    }

    *amount = 0;  /* Assume relative scroll */
    if (ID == SCROLL_REQUEST)
	return MUSH_SCROLL_PASS_EVENT;

    if (event_is_up(event))
	return MUSH_SCROLL_PASS_EVENT;
    if (event_is_ascii(event) && isdigit(event_id(event))) {
	count = (count * 10) + event_id(event) - '0';
	return MUSH_SCROLL_IGNORE;
    }
#ifdef SUN_4_0 /* SunOS 4.0+ */
    /* returns sunview events for some ctl chars */
    switch (event_action(event)) {
	case ACTION_GO_LINE_FORWARD:
	case ACTION_GO_COLUMN_FORWARD:
	    *amount = count ? count : 1;
	    count = 0;
	    return MUSH_SCROLL_RELATIVE;
	case ACTION_GO_LINE_BACKWARD:
	case ACTION_GO_COLUMN_BACKWARD:
	    *amount = count ? -count : -1;
	    count = 0;
	    return MUSH_SCROLL_RELATIVE;
	case ACTION_GO_DOCUMENT_START:
	    *amount = 1;
	    count = 0;
	    return MUSH_SCROLL_TO;
	case ACTION_GO_DOCUMENT_END:
	    *amount = 2;
	    count = 0;
	    return MUSH_SCROLL_TO;
    }
#endif /* SUN_4_0 */
    /* for SunOS 3.5, assume default SunView key mapping */
    /* a little redundancy for 4.0, but it's ok */
    if (!event_is_ascii(event) && ID != KEY_RIGHT(14) && ID != KEY_RIGHT(8)
	&& ID != KEY_RIGHT(7) && ID != KEY_RIGHT(13))
	/* may have to reset "count" here */
	return MUSH_SCROLL_PASS_EVENT; /* Not a scroll event */
    switch (event_id(event)) {
	case KEY_RIGHT(7):	/* Home on new keyboards */
	    *amount = 1, count = 0;
	    return MUSH_SCROLL_TO;
	case KEY_RIGHT(13):	/* End on new keyboards */
	    *amount = 2, count = 0;
	    return MUSH_SCROLL_TO;
	case 'j': case KEY_RIGHT(14):	/* downarrow */
	    *amount = count ? count : 1;
	when 'k': case KEY_RIGHT(8):	/* uparrow */
	    *amount = count ? -count : -1;
	when 'F'-'@':	/* ^f */
	    *amount = count ? count * (len-1) : len - 1;
	when 'D'-'@':	/* ^d */
	    *amount = len/2;
	when 'B'-'@':	/* ^b */
	    *amount = -(count ? count * (1-len) : len - 1);
	when 'U'-'@':	/* ^u */
	    *amount = -len/2;
	when '\033':	/* Escape */
	    /* For SunOS 3.5 check to see if this is a cursor
	     * move key, i.e. ESC[A or ESC[B.
	     */
	    if (!(int)window_read_event(client, event) &&
		event_id(event) == '[' &&
		!(int)window_read_event(client, event))
		    if (event_id(event) == 'A') {
			*amount = -1;
			break;
		    } else if (event_id(event) == 'B') {
			*amount = 1;
			break;
		    }
	default:
	    count = 0;
	    return event_is_ascii(event) ?
		MUSH_SCROLL_IGNORE : MUSH_SCROLL_PASS_EVENT;
    }
    count = 0;
    /* Scroll indicated amount if event is down, ignore up events */
    return MUSH_SCROLL_RELATIVE;
}

Notify_value
scroll_textwin(textsw, event, arg, type)
Textsw	textsw;
Event	*event;
Notify_arg	arg;
Notify_event_type	type;
{
    int scroll_amount;

    switch (decode_scroll(textsw, event, textsw_screen_line_count(textsw),
	&scroll_amount)) {
	case MUSH_SCROLL_PASS_EVENT :
	    return notify_next_event_func(textsw, event, arg, type);
	case MUSH_SCROLL_IGNORE:
	    return NOTIFY_IGNORED;
	case MUSH_SCROLL_TO:
	    if (scroll_amount == 1)
		window_set(textsw, TEXTSW_FIRST, 0, NULL);
	    else if (scroll_amount == 2)
		window_set(textsw, TEXTSW_FIRST, TEXTSW_INFINITY, NULL);
		textsw_scroll_lines(textsw, -textsw_screen_line_count(textsw));
	    break;
	case MUSH_SCROLL_RELATIVE :
	    textsw_scroll_lines(textsw, scroll_amount);
    }
    window_set(textsw, TEXTSW_UPDATE_SCROLLBAR, NULL);
    return NOTIFY_DONE;
}

/*ARGSUSED*/
Notify_value
destroy_proc(frame, status)
Frame frame;
Destroy_status status;
{
    if (ison(glob_flags, IS_GETTING))
	rm_edfile(-1);
    /* status is ignored -- maybe post notice asking to confirm quit? */
    if (status == DESTROY_CHECKING && ison(glob_flags, DO_UPDATE) &&
	!ask("Your folder has been modified.  Quit anyway?"))
	(void) notify_veto_destroy(frame);
    else
	cleanup(0); /* doesn't return */
    return NOTIFY_DONE;
}

/* Initialise the Mush mail icon. */
static void
geticon()
{
    static Rect lrect = { 5, 5, 26, 12 };

    mail_icon = icon_create(
	ICON_WIDTH,		64,
	ICON_HEIGHT,		64,
	ICON_FONT,		mush_font,
	ICON_IMAGE,		&mail_icon_image1,
	ICON_LABEL,		"",
	ICON_LABEL_RECT,	&lrect,
	0);
    compose_icon = icon_create(
	ICON_WIDTH,		64,
	ICON_HEIGHT,		64,
	ICON_IMAGE,		&compose_icon_image,
	ICON_LABEL,		"",
	0);
}

/* Initialise all the cursors used. */
static void
init_cursors()
{
    extern Pixrect mouse_left, mouse_middle, mouse_right, bent_arrow;
    extern Cursor bentarrow;

    l_cursor = cursor_create(
	CURSOR_XHOT,	3,
	CURSOR_YHOT,	3,
	CURSOR_OP,	PIX_SRC,
	CURSOR_IMAGE,	&mouse_left,
	NULL);
    m_cursor = cursor_create(
	CURSOR_XHOT,	3,
	CURSOR_YHOT,	3,
	CURSOR_OP,	PIX_SRC,
	CURSOR_IMAGE,	&mouse_middle,
	NULL);
    r_cursor = cursor_create(
	CURSOR_XHOT,	3,
	CURSOR_YHOT,	3,
	CURSOR_OP,	PIX_SRC,
	CURSOR_IMAGE,	&mouse_right,
	NULL);
    bentarrow = cursor_create(
	CURSOR_XHOT,	8,
	CURSOR_YHOT,	8,
	CURSOR_OP,	PIX_SRC|PIX_DST,
	CURSOR_IMAGE,	&bent_arrow,
	NULL);
    coffee = cursor_create(
	CURSOR_XHOT,	8,
	CURSOR_YHOT,	8,
	CURSOR_OP,	PIX_SRC,
	CURSOR_IMAGE,	&coffee_cup,
	NULL);
}

/* show the timeout cursor (coffee cup) when "on" is TRUE.  This routine
 * may be called many times in layers of locking mechanisms, so be careful
 * not to unlock cursors until "on" has been FALSE as many times as it has
 * been TRUE.
 */
void
timeout_cursors(on)
int on;
{
    Window win;
    Frame subframe;
    static int locked, numwins;
    int i = 0, j;
    static struct {
	Cursor cursor;
	Window win;
    } win_curs[64];

    on? locked++ : locked--;
    if (istool < 2 || locked > 1 || locked == 1 && on == 0)
	return;
    if (on) {
	for (numwins = 0; win = window_get(tool, FRAME_NTH_SUBWINDOW, numwins);
		numwins++) {
	    win_curs[numwins].cursor =
			    cursor_copy((Cursor) window_get(win, WIN_CURSOR));
	    win_curs[numwins].win = win;
	    window_set(win, WIN_CURSOR, coffee, NULL);
	}
	while (subframe = window_get(tool, FRAME_NTH_SUBFRAME, i++))
	    for (j = 0; win = window_get(subframe, FRAME_NTH_SUBWINDOW, j);
		    j++,numwins++) {
		win_curs[numwins].cursor =
			    cursor_copy((Cursor) window_get(win, WIN_CURSOR));
		win_curs[numwins].win = win;
		window_set(win, WIN_CURSOR, coffee, NULL);
	    }
    } else {
	for (j = 0; j < numwins; j++) {
	    window_set(win_curs[j].win, WIN_CURSOR, win_curs[j].cursor, NULL);
	    cursor_destroy(win_curs[j].cursor);
	}
    }
}

/*
 * If the user has specified an alternate icon or set of icons in
 * his .mushrc file, copy the image(s) over the defaults.
 */
void
check_icons()
{
    Pixrect *icon_mpr;
    char errbuf[256], *icon_file, *icon_path;
    int isdir;

    if ((icon_file = do_set(set_options, "mail_icon")) && *icon_file) {
	isdir = 0;
	icon_path = getpath(icon_file, &isdir);
	if (isdir == 0) {
	    if (!(icon_mpr = icon_load_mpr(icon_path, errbuf)))
		error("Error loading mail icon file:\n%s",errbuf);
	    else
		pr_rop(&mail_icon_image1, 0,0,64,64, PIX_SRC, icon_mpr, 0, 0);
	}
    }
    if ((icon_file = do_set(set_options, "newmail_icon")) && *icon_file) {
	isdir = 0;
	icon_path = getpath(icon_file, &isdir);
	if (isdir == 0) {
	    if (!(icon_mpr = icon_load_mpr(icon_path, errbuf)))
		error("Error loading newmail icon file:\n%s",errbuf);
	    else
		pr_rop(&mail_icon_image2, 0,0,64,64, PIX_SRC, icon_mpr, 0, 0);
	}
    }
    if ((icon_file = do_set(set_options, "compose_icon")) && *icon_file) {
	isdir = 0;
	icon_path = getpath(icon_file, &isdir);
	if (isdir == 0) {
	    if (!(icon_mpr = icon_load_mpr(icon_path, errbuf)))
		error("Error loading newmail icon file:\n%s",errbuf);
	    else
		pr_rop(&compose_icon_image, 0,0,64,64, PIX_SRC, icon_mpr, 0, 0);
	}
    }
}
