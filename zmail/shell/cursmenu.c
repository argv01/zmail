/* cursmenu.c   	Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "cursmenu.h"
#include "glob.h"
#include "linklist.h"

#include <general.h>

#ifdef CURSES

typedef struct {
    struct link link;
#define label link.l_name	/* nasty but quick */
    char *response;
    int x;
} CursesMenu;

#define Y_LINE (LINES-1)

void
display_menu_bar(query, menu, start)
char *query;
CursesMenu *menu, *start;
{
    int x = 0, offset = start->x, indent = strlen(query) + 1;

    mvaddstr(Y_LINE, 0, query), clrtoeol();
    do {
	x = start->x - offset + indent;
	if (x + strlen(start->label) >= COLS - 4) {
	    mvaddstr(Y_LINE, x, "...");
	    break;
	} else
	    mvaddstr(Y_LINE, x, start->label);
    } while ((start = (CursesMenu *)start->link.l_next) != menu);
    refresh();
}

char *
curses_menu_bar(query, choices, responses, do_exec)
char *query, **choices, **responses;
int do_exec;
{
    char *p = query, *q = NULL;
    CursesMenu *menu = 0, *entry, *old, *tmp;
    int indent, offset, c, n, x, y = Y_LINE;

    if (!query || !choices)
	return NULL;

    /* Special handling for multi-line queries */
    while (*p && (p = index(p, '\n'))) {
	if (y > 0) {
	    --y;
	    move(y, 0), clrtoeol();
	} else {
	    /* Can't fit the whole query on the screen,
	     * chop off the top line.  Bad news.
	     */
	    query = index(query, '\n') + 1;
	}
	q = ++p;
    }
    if (q) {
	/* Paint the whole query, then skip all but the last line */
	mvaddstr(y, 0, query), clrtoeol();
	query = q;
    }

    indent = strlen(query) + 1;
    p = NULL;

    for (x = 0, n = 0; choices && choices[n]; n++) {
	if (!(entry = zmNew(CursesMenu)))
	    goto done;
	entry->label = choices[n];
	entry->response = responses? responses[n] : choices[n];
	entry->x = x;
	x += strlen(choices[n]) + 1;
	insert_link(&menu, entry);
    }
    display_menu_bar(query, menu, menu);
    old = entry = menu;
    n = offset = 0;
    while (p == NULL && n == 0) {
	x = entry->x - offset + indent;
	if (x < 0) {
	    offset = 0;
	    x = entry->x + indent;
	    tmp = menu;
	    while (x + strlen(entry->label) >= COLS-4) {
		tmp = (CursesMenu *)tmp->link.l_next;
		offset = tmp->x;
		x = entry->x - offset + indent;
	    }
	    display_menu_bar(query, menu, tmp);
	} else if (x + strlen(entry->label) > COLS-4) {
	    offset = entry->x;
	    x = indent;
	    display_menu_bar(query, menu, entry);
	}
	if (ison(glob_flags, REV_VIDEO))
	    STANDOUT(Y_LINE, x, entry->label);
	else
	    move(Y_LINE, x);
	refresh();
	switch (c = m_getchar()) {
	    case ' ' : case '\t' : case '>' : case '+' :
		entry = (CursesMenu *)entry->link.l_next;
	    when Ctrl('H'):
		if (entry == menu)
		    n = -1;
		else {
		    /* Fall through */
	    case '<' : case '-' :
		    entry = (CursesMenu *)entry->link.l_prev;
		}
	    when '[' : case '{' : case '^' :
		entry = menu;
	    when ']' : case '}' : case '$' :
		entry = (CursesMenu *)menu->link.l_prev;
	    when '\177' : case ESC :
		n = -1;
	    when '\n' :
		p = entry->response;
	    otherwise :
		tmp = do_exec? entry : (CursesMenu *)entry->link.l_next;
		do {
		    if (tmp->label[0] == c) {
			if (do_exec) {
			    p = tmp->response;
			    n = 1;
			} else
			    entry = tmp;
			break;
		    }
		} while ((tmp = (CursesMenu *)tmp->link.l_next) != entry);
	}
	mvaddstr(Y_LINE, x, old->label);
	old = entry;
    }
done:
    while (entry = menu) {
	remove_link(&menu, entry);
	xfree((char *)entry);
    }

    if (y < Y_LINE) {
	/* Multiple lines were displayed. */
	turnon(glob_flags, IGN_SIGS);
	indent = current_msg;
	current_msg = -1;
	/* This repaint should be handled more generically. */
	while (y < Y_LINE) {
	    move(y, 0);
	    if (isoff(glob_flags, CNTD_CMD) && y <= screen &&
		    n_array[y-1] < msg_cnt)
		printw("%-.*s", COLS-2, compose_hdr(n_array[y-1]));
	    clrtoeol();
	    ++y;
	}
	current_msg = indent;
	if (ison(glob_flags, CNTD_CMD))
	    mvprintw(LINES-2, 0, "%-.*s", COLS-2, compose_hdr(current_msg));
	else if (screen < LINES - 2 - curses_help_msg(FALSE))
	    (void) curses_help_msg(TRUE);
	turnoff(glob_flags, IGN_SIGS);
    }

#if Y_LINE == 0
#if defined( IMAP )
    zmail_mail_status(0), refresh();
#else
    mail_status(0), refresh();
#endif
#else
    clr_bot_line(); /* does a refresh() */
#endif /* Y_LINE */
    if (n < 0)
	return NULL;
    return p;
}

#endif /* CURSES */

int
option_to_menu(list, menu, responses)
struct options **list;
char ***menu, ***responses;
{
    struct options *tmp;
    int n = 0, r = 0;

    if (!menu)
	return -1;
    else
	*menu = DUBL_NULL;
    if (responses)
	*responses = DUBL_NULL;

    if (list && *list) {
	optlist_sort(list);
	tmp = *list;
	do {
	    if ((n = catv(n, menu, 1, unitv(tmp->option))) < 1) {
		free_vec(*menu);
		if (responses)
		    free_vec(*responses);
		break;
	    }
	    if (responses) {
		if ((r = catv(r, responses, 1, unitv(tmp->value))) < 1) {
		    free_vec(*menu);
		    free_vec(*responses);
		    break;
		}
	    }
	} while (tmp = tmp->next);
    }
    return n;
}

char **
link_to_argv(list)
struct link *list;
{
    struct link *tmp = list;
    char **argv = DUBL_NULL;
    int n = 0;

    if (tmp)
	do {
	    if ((n = catv(n, &argv, 1, unitv(tmp->l_name))) < 1) {
		free_vec(argv);
		return DUBL_NULL;
	    }
	} while ((tmp = tmp->l_next) != list);
    return argv;
}
