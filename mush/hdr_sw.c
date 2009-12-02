/* @(#)hdr_sw.c	(c) copyright	2/17/90 (Dan Heller) */

/* This file handles all the header subwindow code.  It would be much
 * better if this subwindow, which displays the headers for the current
 * folder, were a textsw.  That way, this file would go away completely.
 * Until then, we have to create the window (canvas), define an event
 * handler for when events happen in this window, create our own scrollbar,
 * figure out when the user scrolls with it, attach our own popup menu to
 * the canvas, handle events for that, let's see... kitchen sink?  Oh,
 * that's over there in the corner.
 */
#include "mush.h"
#ifdef SUN_4_0 /* SunOS 4.0+ */
#include <sunwindow/win_keymap.h>
#endif /* SUN_4_0 */

extern Panel hdr_panel;
extern void hdr_io(), fkey_interposer();

static Notify_value scroll_hdr();
static void msg_menu_func(), do_menu(), msg_menu_notify();
Menu msg_menu;

void
make_hdr_sw(parent)
Frame parent;
{
    Textsw tmpsw;

    if (!(hdr_sw = window_create(parent, CANVAS,
	WIN_HEIGHT,		10 + screen*l_height(),
	WIN_WIDTH,		WIN_EXTEND_TO_EDGE,
	WIN_BELOW,		hdr_panel,
	WIN_EVENT_PROC,		hdr_io,
	CANVAS_AUTO_CLEAR,	TRUE,
	CANVAS_RETAINED,	TRUE,
	WIN_CONSUME_KBD_EVENTS,
	    WIN_ASCII_EVENTS, WIN_LEFT_KEYS, WIN_TOP_KEYS, WIN_RIGHT_KEYS, NULL,
	WIN_IGNORE_KBD_EVENTS,
	    WIN_UP_ASCII_EVENTS, NULL,
	WIN_CONSUME_PICK_EVENTS,
	    LOC_WINENTER, WIN_MOUSE_BUTTONS, LOC_MOVE, NULL,
	WIN_VERTICAL_SCROLLBAR, scrollbar_create(0),
	NULL)))
	perror("hdr_sw"), cleanup(0);
    hdr_win = canvas_pixwin(hdr_sw);
    (void) notify_interpose_event_func(hdr_sw, fkey_interposer, NOTIFY_SAFE);
    (void) notify_interpose_event_func(hdr_sw, scroll_hdr, NOTIFY_SAFE);
    scrollbar_set((Scrollbar)window_get(hdr_sw, WIN_VERTICAL_SCROLLBAR),
	SCROLL_NORMALIZE,	FALSE,
	SCROLL_ADVANCED_MODE,	TRUE,
	SCROLL_LINE_HEIGHT,	l_height(),
	SCROLL_VIEW_LENGTH,	screen,
	NULL);
#ifdef SUN_4_0 /* SunOS 4.0+ */
    /* This is a particularly ugly hack.  If Sun only documented the correct
     * way to set up the key mapping for a window the way that textsw's do
     * then we wouldn't have to do anything this awful.  Maybe in 4.2.....
     *
     * The object here is to get the same translation table for our header
     * canvas as for a textsw (more or less anyway).  This way the arrow
     * keys and such work right.
     */
    tmpsw = window_create(parent, TEXTSW, NULL);
#ifdef SUN_4_1
    keymap_from_fd[(int)window_get(hdr_sw, WIN_FD)].keymap =
	keymap_from_fd[(int)window_get(tmpsw, WIN_FD)].keymap;
    keymap_from_fd[(int)window_get(tmpsw, WIN_FD)].keymap = (Keymap *) 0;
#else /* !SUN_4_1 */
    keymap_from_fd[(int)window_get(hdr_sw, WIN_FD)].kf_keymap =
	keymap_from_fd[(int)window_get(tmpsw, WIN_FD)].kf_keymap;
    keymap_from_fd[(int)window_get(tmpsw, WIN_FD)].kf_keymap = (Keymap *) 0;
#endif /* SUN_4_1 */
    (void) window_destroy(tmpsw);
#endif /* SUN_4_0 */
}

static Notify_value
scroll_hdr(canvas, event, arg, type)
Canvas	canvas;
Event	*event;
Notify_arg	arg;
Notify_event_type	type;
{
    int amount, count, i;
    int show_deleted = !!do_set(set_options, "show_deleted");
    char *argv[3], msgnum[8];
    Scrollbar sb;
    argv[0] = "headers";
    argv[1] = msgnum;
    argv[2] = NULL;

    switch (decode_scroll((Notify_client) canvas, event, screen, &amount)) {
	case MUSH_SCROLL_PASS_EVENT:
	    switch(ID) {
		case SCROLL_ENTER:
		case SCROLL_EXIT:
		    return NOTIFY_IGNORED;
		case SCROLL_REQUEST:
		    sb = (Scrollbar)arg;
		    switch( (Scroll_motion)
			 scrollbar_get(sb, SCROLL_REQUEST_MOTION)) {
			case SCROLL_LINE_FORWARD:
			    amount = 1;
			    break;
			case SCROLL_LINE_BACKWARD:
			    amount = -1;
			    break;
			case SCROLL_ABSOLUTE:
			    i = (int)scrollbar_get(sb, SCROLL_VIEW_START);
			    if (!show_deleted) {
				count = i;
				for (i = 0; i < msg_cnt-1; i++)
	    			    if (!ison(msg[i].m_flags, DELETE) &&
					    count-- == 0)
					break;
			    }
			    (void) sprintf(msgnum, "%d", i+1);
			    argv[1] = msgnum;
			    (void) do_hdrs(2, argv, NULL);
			    return(NOTIFY_DONE);
			default:
			    amount =
				(int)scrollbar_get(sb, SCROLL_VIEW_START) -
				(int)scrollbar_get(sb, SCROLL_LAST_VIEW_START);
			    break;
		    }
		    break;
		default:
		    return notify_next_event_func(canvas, event, arg, type);
	    }
	    break;
	case MUSH_SCROLL_IGNORE:
	    return NOTIFY_IGNORED;
	case MUSH_SCROLL_TO:
	    if (amount == 1) {
		argv[1] = "1";
		(void) do_hdrs(2, argv, NULL);
		return NOTIFY_DONE;
	    } else {
		(void) sprintf(msgnum, "%d", msg_cnt - screen + 1);
		argv[1] = msgnum;
		(void) do_hdrs(2, argv, NULL);
		return NOTIFY_DONE;
	    }
    }
    if (amount == screen)
	argv[1] = "+";
    else if (amount == -screen)
	argv[1] = "-";
    else if (amount >= 0) {
	if (amount < screen)
	    (void) sprintf(msgnum, "%d", min(n_array[amount]+1, msg_cnt-1));
	else {
	    /* so much for layering */
	    for (i = n_array[0]+1; i < msg_cnt-1 && amount > 0; i++)
		if (show_deleted || !ison(msg[i].m_flags, DELETE))
			amount--;
	    (void) sprintf(msgnum, "%d", i);
	}
    } else {
	/* so much for layering */
	for (i = n_array[0]; i > 0 && amount < 0; i--)
	    if (show_deleted || !ison(msg[i-1].m_flags, DELETE))
		amount++;
	(void) sprintf(msgnum, "%d", i + 1);
    }
    (void) do_hdrs(2, argv, NULL);
    return NOTIFY_DONE;
}

/*
 * Routines to handle io on the hdr_sw (canvas).
 */

/* if MENU button goes down on a hdr, drawbox around hdr and popup menu */
#define draw(x1,y1,x2,y2) (void) pw_vector(hdr_win, x1,y1,x2,y2,PIX_XOR,1)
#define box(x1,y1,x2,y2)  \
	draw(x1,y1, x1,y2), draw(x1,y2, x2,y2), \
	draw(x2,y2, x2,y1), draw(x2,y1, x1,y1)

#define READ_MSG	(char *)'r'
#define DEL_MSG		(char *)'d'
#define UNDEL_MSG	(char *)'u'
#define REPL_MSG	(char *)'R'
#define SAVE_MSG	(char *)'s'
#define PRNT_MSG	(char *)'p'
#define PRE_MSG		(char *)'P'
#define MARK_MSG	(char *)'m'
#define HELP_MSG	(char *)'H'

#define MARK_TOGGLE	(char *)'T'
#define MARK_A		(char *)'A'
#define MARK_B		(char *)'B'
#define MARK_C		(char *)'C'
#define MARK_D		(char *)'D'
#define MARK_E		(char *)'E'
#define MARK_CLEAR	(char *)'c'
#define MARK_HELP	(char *)'h'

/*ARGSUSED*/
void
hdr_io(canvas, event, arg)
Canvas canvas;
Event *event;
caddr_t arg;
{
    static int	which_cursor;
    int 	line;

    if (ID == WIN_REPAINT) {
	if (is_iconic != (int) window_get(tool, FRAME_CLOSED)) {
	    check_new_mail();

	    /*  Reload time with value of timeout upon timer expiration. */
	    mail_timer.it_interval.tv_sec = time_out;

	    mail_timer.it_value.tv_sec = time_out;
	    (void) notify_set_itimer_func(tool, do_check,
		ITIMER_REAL, &mail_timer, (struct itimerval *) 0);
	    is_iconic = 0;
	}
    }

    /* make cursor change which button is lit */
    switch (which_cursor) {
	case 0 : (void) window_set(canvas, WIN_CURSOR, l_cursor, NULL);
	when 1 : (void) window_set(canvas, WIN_CURSOR, m_cursor, NULL);
	when 2 : (void) window_set(canvas, WIN_CURSOR, r_cursor, NULL);
    }

    which_cursor = (which_cursor+1) % 3;

    /* just return -- we just wanted to make the cursor flicker */
    if (ID == LOC_STILL || ID == LOC_MOVE || ID == LOC_WINENTER ||
	ID == LOC_RGNENTER || ID == KBD_USE || ID == KBD_DONE)
	return;

    if (event_is_button(event) && event_is_down(event)) {
	line = (event_y(event) - 5) / l_height();
	if (line < 0)
	    line = 0;
	else if (line >= screen)
	    line = screen - 1;
	if (!msg_cnt || n_array[line] > msg_cnt)
	    return;
	if (ID == MS_RIGHT)
	    do_menu(hdr_sw, event, window_get(hdr_sw, WIN_FD), n_array[line]);
	else if (ID == MS_MIDDLE) {
	    set_isread(n_array[line]);
	    msg_menu_func((int)DEL_MSG, n_array[line]);
	} else {
	    int do_do_hdrs = 0;
	    if (current_msg != n_array[line]) {
		current_msg = n_array[line];
		do_do_hdrs++;
	    }
	    if (ison(msg[current_msg].m_flags, UNREAD))
		do_do_hdrs++;
	    (void) display_msg(n_array[line], (u_long)0);
	    if (do_do_hdrs)
		(void) do_hdrs(0, DUBL_NULL, NULL);
	}
    } else
	window_default_event_proc(canvas, event, NULL);
}

static struct menu_rec {
    char *str;	/* Menu item label. */
    char *data;	/* Menu item client data. */
};

void
get_msg_menu()
{
    int i;
    Menu_item mi = NULL, sub_mi;

    static struct menu_rec msg_items[] = {
	{ "Read",            READ_MSG  },
	{ "Delete",          DEL_MSG   },
	{ "Undelete",        UNDEL_MSG },
	{ "Reply",           REPL_MSG  },
	{ "Save",            SAVE_MSG  },
	{ "Preserve",        PRE_MSG   },
	{ "Mark",	     MARK_MSG  },
	{ "Print",           PRNT_MSG  },
	{ "Help",            HELP_MSG  },
    };
    static struct menu_rec mark_msg_items[] = {
	{ "Toggle Mark",    MARK_TOGGLE},
	{ "Priority A",     MARK_A     },
	{ "Priority B",     MARK_B     },
	{ "Priority C",     MARK_C     },
	{ "Priority D",     MARK_D     },
	{ "Priority E",     MARK_E     },
	{ "Clear Priority", MARK_CLEAR },
	{ "Help",           MARK_HELP  },
    };

    msg_menu = menu_create(MENU_NOTIFY_PROC, menu_return_item, NULL);
    for (i = 0; i < ArraySize(msg_items); i++) {
	mi = menu_create_item(MENU_STRING,	msg_items[i].str,
			      MENU_CLIENT_DATA,	msg_items[i].data,
			      NULL);
	if (msg_items[i].data == MARK_MSG) {
	    int j;
	    /* get the menu from <Mark> and set as this item's pullright */
	    Menu the_menu = menu_create(
		MENU_NOTIFY_PROC, menu_return_item, NULL);
	    for (j = 0; j < ArraySize(mark_msg_items); j++) {
		sub_mi = menu_create_item(
		    MENU_STRING,	mark_msg_items[j].str,
		    MENU_CLIENT_DATA,	mark_msg_items[j].data,
		    NULL);
		(void) menu_set(the_menu, MENU_APPEND_ITEM, sub_mi, NULL);
	    }
	    menu_set(mi, MENU_PULLRIGHT, the_menu, NULL);
	}
	(void) menu_set(msg_menu, MENU_APPEND_ITEM, mi, NULL);
    }
}

static void
do_menu(can_sw, event, fd, message)
Canvas can_sw;
Event *event;
int fd, message;
{
    char *action;
    char *save_place;
    Menu_item cur_msg_item;
    static char buf[16];

    if (!msg_cnt) {
	wprint("No Messages.\n");
	return;
    }
    if (fd) {
	int line;
	Rect *hdr_rect;
	extern Menu hdr_save_menu;

	if (!msg_menu)
	    get_msg_menu();
	(void) sprintf(buf, "Message #%d", message+1);
	/* provide feedback about what message the menu references */
	for (line = 0; line <= n_array[screen-1]; line++)
	    if (n_array[line] == message)
		break;
	hdr_rect = (Rect *)window_get(hdr_sw, WIN_RECT);
	box(0, 5 + line * l_height(),
	    hdr_rect->r_width, 5 + (line+1) * l_height());
	/* show menu */
	cur_msg_item = menu_show(msg_menu, can_sw, event, NULL);
	/* remove feedback */
	box(0, 5 + line * l_height(),
	    hdr_rect->r_width, 5 + (line+1) * l_height());
	/* if user selected something, figure out what was selected. */
	if (!cur_msg_item)
	    return;
	else {
#ifndef NO_WALK_MENUS
	    Menu item = (Menu)cur_msg_item;
	    while (item = (Menu)menu_get(item, MENU_PARENT))
		if ((Menu)item == hdr_save_menu)
		    break;
		/* May also need to test the type of item and
		 * break if it is not a Menu_item -- but how??
		 * My sunview isn't that good ... I got the
		 * fragment above from Dan's XView book.
		 */
	    if ((Menu)item == hdr_save_menu) {
		save_place = (char *)menu_get(cur_msg_item, MENU_CLIENT_DATA);
		action = SAVE_MSG;
	    } else
#endif /* NO_WALK_MENUS */
		action = (char *) menu_get(cur_msg_item, MENU_CLIENT_DATA);
	}
    } else
	action = (char *) event;

    set_isread(message);
    switch ((int) action) {
	case SAVE_MSG : {
	    extern Panel_item msg_num_item, save_item;
	    (void) panel_set(msg_num_item, PANEL_VALUE,
					sprintf(buf, "%d", message+1), NULL);
#ifndef NO_WALK_MENUS
		if (*save_place == '\0') /* magic to mean "use Filename:" */
		    do_file_dir(save_item, event);
		else
		    xx_file_dir(save_item, save_place);
#else /* NO_WALK_MENUS */
		event_id(event) = MS_LEFT;
		do_file_dir(save_item, 0, event);
#endif /* NO_WALK_MENUS */
	    (void) panel_set(msg_num_item, PANEL_VALUE, NO_STRING, NULL);
	}
	when HELP_MSG :
	    help(0, "headers", tool_help);
	when REPL_MSG : {
	    extern Panel_item reply_item;
	    open_compose();
	    if (!compose_frame)
		break;	/* open failed */
	    /* reply_item shouldn't be here */
	    respond_mail(reply_item, message, NO_EVENT);
	}
	when READ_MSG :
	    if (current_msg != message) {
		current_msg = message;
		(void) do_hdrs(0, DUBL_NULL, NULL);
	    }
#ifdef SUN_3_5
	    /* Test for a shortage of file descriptors */
	    if (nopenfiles(0) > 3)
#endif /* SUN_3_5 */
	    turnon(glob_flags, NEW_FRAME);
	    more_prompt = compose_hdr(message);
	    display_msg(message, (u_long)0);

	otherwise :
	    msg_menu_func((int)action, message);
    }
}

/* msg_menu_func() is a function called to perform message menu actions
 * that are either selected from the popup menu in the header window or
 * from mouse actions that function as accelerators.
 */
static void
msg_menu_func(action, message)
int action;
{
    int argc;
    register char **argv;
    char buf[32];

    switch (action) {
        case PRNT_MSG :
	    wprint("Message #%d sent to printer.\n", message+1);
	    (void) strcpy(buf, "lpr");
	when UNDEL_MSG : case DEL_MSG :
	    (void) sprintf(buf, "%selete", (action == (int)DEL_MSG)?"d":"und");
        when PRE_MSG :
	    (void) strcpy(buf, "preserve");
        when MARK_MSG : case MARK_TOGGLE :
	    (void) sprintf(buf, "%smark",
		ison(msg[message].m_flags, M_PRIORITY(0))? "un" : "");
	when MARK_A : case MARK_B : case MARK_C : case MARK_D : case MARK_E :
	    (void) sprintf(buf, "mark -%c", action);
	when MARK_CLEAR	:
	    (void) strcpy(buf, "mark -");
	when MARK_HELP :
	    (void) help(0, "mark", tool_help);
	    return;
	otherwise :
	    print("unknown switch: %c\n", action);
	    return;
    }
    (void) sprintf(&buf[strlen(buf)], " %d", message+1);

    if (argv = make_command(buf, (char ***) DUBL_NULL, &argc))
	(void) do_command(argc, argv, msg_list);
}
