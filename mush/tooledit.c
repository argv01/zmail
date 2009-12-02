/* @(#)tooledit.c	(c) copyright	2/14/90 (Dan Heller) */

/*
 * intercept events in the compose window for auto-
 *	positioning and tilde command recognition.
 */
#include "mush.h"

static short dat_bentarrow[] = {
    0x007F, 0x007F, 0x007F, 0x0007, 0x0407, 0x0C07, 0x1C07, 0x3807, 
    0x7FFF, 0xFFFF, 0x7FFF, 0x3800, 0x1C00, 0x0C00, 0x0400, 0x0000
};
mpr_static(bent_arrow, 16, 16, 1, dat_bentarrow);
Cursor bentarrow;

extern void do_send(), do_edit();

/* Return the byte position in the textsw of the header specified */
Textsw_index
header_position(textsw, str)
Textsw textsw;
char *str;
{
    char buf[256];
    register char *p = buf, *p2;
    int contd_hdr = 0, add_newline = 0;
    Textsw_index pos = 0L, ret_pos = 0L;

    buf[0] = 0;
    for (;;) {
	/* get a line at a time from the textsw */
	(void) window_get(textsw, TEXTSW_CONTENTS, pos, buf, 256);
	if (p = index(buf, '\n'))
	    *p = 0;
	else
	    add_newline++;
	p = buf;
	skipspaces(0);
	if (!*p) /* newline alone -- end of headers */
	    break;
	pos += strlen(buf) + 1; /* advance position to next line */
	if (*p != ' ' && *p != '\t') {
	    contd_hdr = 0;
	    /* strcmp ignoring case */
	    for (p2 = str; *p && *p2 && lower(*p2) == lower(*p); ++p, ++p2)
		;
	    /* MATCH is true if p2 is at the end of str and *p is ':' */
	    if (*p2 || *p != ':') {
		if (!*p2 && isspace(*any(p, ": \t"))) {
		    /* Not a legal or continued header */
		    pos -= strlen(buf) + 1; /* go back to beginning of line */
		    break;
		}
		continue;
	    } else {
		contd_hdr = 1;
		ret_pos = pos - 1;
	    }
	} else if (!contd_hdr)
	    continue;
    }
    if (!ret_pos) {
	/* coudn't find the header -- add it */
	window_set(textsw, TEXTSW_INSERTION_POINT, pos, NULL);
	p = buf;
	if (add_newline)
	    *p++ = '\n', pos--;
	for (p2 = str; *p2; ++p2) {
	    if (p2 == str || p2[-1] == '-')
		*p++ = upper(*p2);
	    else
		*p++ = *p2;
	}
	*p++ = ':', *p++ = ' ', *p++ = '\n', *p = 0;
	textsw_insert(textsw, buf, strlen(buf));
	ret_pos = pos + strlen(buf) - 1;
    }
    return ret_pos;
}

/* position_flags indicates which header to go to when uses tilde commands */
static u_long position_flags;
static char *tilde_hdrs[] = {
#define POSITION_TO	ULBIT(0)
    "to",
#define POSITION_SUBJ	ULBIT(1)
    "subject",
#define POSITION_CC	ULBIT(2)
    "cc",
#define POSITION_BCC	ULBIT(3)
    "bcc",
#define POSITION_FCC	ULBIT(4)
    "fcc"
};
#define POSITION_ALL \
    ((POSITION_TO) | (POSITION_SUBJ) | (POSITION_CC) | (POSITION_BCC))
#define POSITION_END	ULBIT(5)
#define TOTAL_POSITIONS	6

/*
 * position_flags identifies which header is requested by the calling func.
 * use header_position to find the position of the header associated with
 * with the flags.
 */
static void
go_to_next_pos(textsw)
Textsw textsw;
{
    Textsw_index pos;
    int i = 0;

    while (i < TOTAL_POSITIONS && isoff(position_flags, ULBIT(i)))
	i++;
    if (i == TOTAL_POSITIONS)
	return;
    if (i < ArraySize(tilde_hdrs))
	pos = header_position(textsw, tilde_hdrs[i]);
    else
	pos = (Textsw_index)window_get(textsw, TEXTSW_LENGTH);
    turnoff(position_flags, ULBIT(i));
    if (!position_flags)
	/* restore old cursor */
	window_set(textsw,WIN_CURSOR, window_get(mfprint_sw, WIN_CURSOR), NULL);
    else
	window_set(textsw, WIN_CURSOR, bentarrow, NULL);
    window_set(textsw, TEXTSW_INSERTION_POINT, pos, NULL);
    textsw_normalize_view(textsw, (Textsw_index)0);
}

tilde_from_menu(item, value, event)
Panel_item item;
int value;
Event	*event;
{
    Textsw textsw = (Textsw)panel_get(panel_get(item, PANEL_PARENT_PANEL),
	PANEL_CLIENT_DATA);
    if (value == 0 || event_id(event) == MS_LEFT)
	position_flags = POSITION_ALL;
    else
	turnon(position_flags, ULBIT(value - 1));
    panel_set_value(item, 0);
    go_to_next_pos(textsw);
}

/*
 * This interpose function is here to parse for tilde escapes.
 * Note: this is a (currently) undocumented feature and is intended
 * as an accelerator for advanced users.  Supported tilde escapes
 * are: t,s,c,b,x,e and v.
 */
Notify_value
edit_msg_textwin(textsw, event, arg, type)
Textsw	textsw;
Event	*event;
Notify_arg	arg;
Notify_event_type	type;
{
    char buf[2];
    static char do_tilde;
    Textsw_index pos;

    if (do_tilde == 1 && event_is_ascii(event) &&
	    /* make sure we are going to catch this switch */
	    index("bschetv", event_id(event))) {
	textsw_erase(textsw,
	    (unsigned)window_get(textsw, TEXTSW_INSERTION_POINT)-1,
	    (unsigned)window_get(textsw, TEXTSW_INSERTION_POINT));
	switch (event_id(event)) {
	    case 'h':
		turnon(position_flags, POSITION_ALL);
	    when 't':
		turnon(position_flags, POSITION_TO);
	    when 's':
		turnon(position_flags, POSITION_SUBJ);
	    when 'c':
		turnon(position_flags, POSITION_CC);
	    when 'b':
		turnon(position_flags, POSITION_BCC);
	    when 'e' : case 'v' : {
		/* shouldn't use global -- hack for now */
		extern Panel_item edit_item;
		do_edit(edit_item);
		return NOTIFY_DONE;
	    }
	}
	do_tilde = 0;
	go_to_next_pos(textsw);
	return NOTIFY_DONE;
    }
    do_tilde = 0;
    /* check to see if this is a potential tilde escape */
    if (event_id(event) == *escape) {
	/* get previous character entered */
	pos = (Textsw_index)window_get(textsw, TEXTSW_INSERTION_POINT);
	if (pos > 0)
	    (void) window_get(textsw, TEXTSW_CONTENTS, pos-1, buf, 1);
	/* test to see if ~ came at the beginning of a line */
	if (pos < 1 || buf[0] == '\n')
	    do_tilde = 1;
    }
    /* check for auto-next-header .. e.g. when you hit CR on To: go to Subj:
     * special case backspace keys since textsw_start_of_display_line() has
     * a bug where it gets the line # wrong when backspacing.
     */
    if (position_flags != 0L && ID != CTRL('H') && ID != 127) {
	Notify_value val;
	if (ID == '\n' || ID == '\r') {
	    go_to_next_pos(textsw);
	    return NOTIFY_DONE; /* don't process event */
	}
	/* we're still processing this header -- continue to do so unless
	 * the event in question changes the line# of the insertion point.
	 * first get current position...
	 */
	pos = (Textsw_index)window_get(textsw, TEXTSW_INSERTION_POINT);
	/* now let the event be processed... */
	val = notify_next_event_func(textsw, event, arg, type);
	/* see if the line # for the new insertion point has changed. */
	if (textsw_start_of_display_line(textsw, pos) !=
	    textsw_start_of_display_line(textsw,
		(Textsw_index)window_get(textsw, TEXTSW_INSERTION_POINT))) {
	    /* the event (mouse button, ACTION_??), changed the line # */
	    position_flags = 0L; /* disable auto-next-header */
	    /* restore cursor */
	    window_set(textsw,
		WIN_CURSOR, window_get(mfprint_sw, WIN_CURSOR),
		NULL);
	}
	return val;
    }
    return notify_next_event_func(textsw, event, arg, type);
}

/*
 * start the compose textsw.  This is here because we need position_flags
 * and the tilde-bits to set the insertion point at the To: line if
 * do_position is true.
 */
void
start_textsw_edit(textsw, do_position)
Textsw textsw;
{
    extern char *hfile;
    char *file = (char *)window_get(textsw, TEXTSW_CLIENT_DATA);
    Textsw_index first, last, to_index;
    int		i;

    strdup(file, hfile);
#ifdef SUN_4_0 /* SunOS 4.0+ */
    window_set(textsw,
	TEXTSW_CLIENT_DATA,		file,
	TEXTSW_FILE_CONTENTS,		hfile,
	TEXTSW_READ_ONLY,		FALSE,
	TEXTSW_STORE_CHANGES_FILE,	FALSE,
	NULL);
#else /* SUN_4_0 */
    textsw_load_file(textsw, hfile, 1, 0, 0);
    window_set(textsw,
	TEXTSW_CLIENT_DATA,		file,
	TEXTSW_READ_ONLY,		FALSE,
	TEXTSW_STORE_CHANGES_FILE,	FALSE,
	NULL);
#endif /* SUN_4_0 */
    position_flags = 0L;
    if (do_position) {
	turnon(position_flags, POSITION_TO);
	if (do_set(set_options, "ask") || do_set(set_options, "asksub"))
	    turnon(position_flags, POSITION_SUBJ);
	if (do_set(set_options, "askcc"))
	    turnon(position_flags, POSITION_CC);
    }
    turnon(position_flags, POSITION_END);
    go_to_next_pos(textsw);
    (void) unlink(hfile);
    xfree(hfile), hfile = NULL;
}

/*ARGSUSED*/
void
do_edit(item, value, event)
Panel_item item;
int value;
register Event *event;
{
    int argc;
    char *file, **argv, *edit, cmd[MAXPATHLEN];
    Panel_item next;
    Panel panel = (Panel)panel_get(item, PANEL_PARENT_PANEL);
    Textsw textsw = (Textsw)panel_get(panel, PANEL_CLIENT_DATA);

    file = (char *)window_get(textsw, TEXTSW_CLIENT_DATA);
    if (textsw_store_file(textsw, file, 0, 0)) {
	error("Can't start editor");
	return;
    }
    if ((!(edit = do_set(set_options, "visual")) || !*edit) &&
	(!(edit = do_set(set_options, "editor")) || !*edit))
	edit = DEF_EDITOR;
    (void) sprintf(cmd, "%s %s", edit, file);
    argc = 0;
    if (!(argv = mk_argv(cmd, &argc, FALSE))) {
	unlink(file);
	return;
    }
    if (tool_edit_letter(textsw, argv) > -1) {
	/* skip first panel item */
	item = (Panel_item) panel_get(panel, PANEL_FIRST_ITEM);
	for (item = (Panel_item) panel_get(item, PANEL_NEXT_ITEM);
	     item; item = next) {
	     next = (Panel_item) panel_get(item, PANEL_NEXT_ITEM);
	     (void) panel_set(item, PANEL_SHOW_ITEM, FALSE, NULL);
	}
	position_flags = 0L;
	window_set(textsw,WIN_CURSOR, window_get(mfprint_sw,WIN_CURSOR), NULL);
    }
    free_vec(argv);
}
