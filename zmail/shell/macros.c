/* macros.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "c_bind.h"
#include "catalog.h"
#include "macros.h"
#include "pager.h"
#include "zmail.h"
#include "zmsource.h"

#include <general.h>

extern struct cmd_map map_func_names[];

struct cmd_map	*mac_stack, *mac_hide;

/*
 * print current binding to macro mappings if "str" is NULL.
 * else return the string "x_str" which the str is bound to.
 */
char *
c_macro(name, str, opts)
const char *name;
register char *str;
register struct cmd_map *opts;
{
    register int    incurses = iscurses;
    char buf[MAX_MACRO_LEN];
    char buf2[sizeof (buf) * 3];
    ZmPager pager;

    if (!str) {
	for (; opts; opts = opts->m_next)
	    if (opts->m_cmd == C_MACRO)
		break;
	if (!opts) {
	    print(catgets( catalog, CAT_SHELL, 449, "No %s settings.\n" ), name);
	    return (char *)(-1);
	}
	if (incurses)
	    clr_bot_line(), iscurses = FALSE;
	pager = ZmPagerStart(PgInternal);
	sprintf(buf, catgets( catalog, CAT_SHELL, 450,
			      "\nCurrent %s settings:\n\n" ),
		name);
	ZmPagerWrite(pager, buf);
    }

    if (!opts)
	return NULL;

    for (; opts; opts = opts->m_next) {
	if (opts->m_cmd != C_MACRO)
	    continue;
	if (!str) {
	    sprintf(buf, "%-20.20s  ",
		    ctrl_strcpy(buf2, opts->m_str, FALSE));
	    ZmPagerWrite(pager, buf);
	    sprintf(buf, "%s\n",
		    ctrl_strcpy(buf2, opts->x_str, TRUE));
	    ZmPagerWrite(pager, buf);
	} else {
	    if (strcmp(str, opts->m_str))
		continue;
	    else
		return opts->x_str;
	}
    }
    iscurses = incurses;
    if (str)
	ZmPagerStop(pager);
    return NULL;
}

int
mac_push(str)
register const char *str;
{
    register struct cmd_map *tmp;

    /* now make a new macro struct and set fields */
    if (!(tmp = (struct cmd_map *)calloc((unsigned)1,sizeof(struct cmd_map)))) {
	error(SysErrWarning, "calloc");
	return -1;
    }
    tmp->m_next = mac_stack;
    mac_stack = tmp;
    tmp->x_str = savestr(str);	/* x_str is the text of the expansion */
    tmp->m_str = tmp->x_str;	/* m_str is the current read position */

    /*
     * Save the current state of the glob_flags so
     * mac_push() can also serve as unget of stdin
     */
    tmp->m_cmd = glob_flags;
    return 0;
}

int
mac_queue(str)
register char *str;
{
    register struct cmd_map **tmp; /* NOTE pointer to pointer! */

    /* Find the bottom of the macro stack */
    for (tmp = &mac_stack; *tmp; tmp = &((*tmp)->m_next))
	;
    /* now make a new macro struct and set fields */
    if (!(*tmp =
	    (struct cmd_map *)calloc((unsigned)1,sizeof(struct cmd_map)))) {
	error(SysErrWarning, "calloc");
	return -1;
    }
    (*tmp)->m_next = (struct cmd_map *)0; /* calloc should do this .... */
    (*tmp)->x_str = savestr(str); /* x_str is the text of the expansion */
    (*tmp)->m_str = (*tmp)->x_str; /* m_str is the current read position */

    /*
     * Save the current state of the glob_flags, but set
     * LINE_MACRO and IN_MACRO and clear QUOTE_MACRO,
     * so that [commands] will be expanded.
     */
    (*tmp)->m_cmd = glob_flags;
    turnon((*tmp)->m_cmd, IN_MACRO|LINE_MACRO);
    turnon((*tmp)->m_cmd, QUOTE_MACRO);
    return 0;
}

void
mac_pop()
{
    register struct cmd_map *tmp;

    if (mac_stack) {
	tmp = mac_stack;
	mac_stack = tmp->m_next;
	xfree(tmp->x_str);
	xfree((char *) tmp);
    }
    /*
     * Restore saved MACRO glob_flags only (see mac_push())
     */
    if (mac_stack) {
	if (ison(mac_stack->m_cmd, IN_MACRO))
	    turnon(glob_flags, IN_MACRO);
	else
	    turnoff(glob_flags, IN_MACRO);
	if (ison(mac_stack->m_cmd, LINE_MACRO))
	    turnon(glob_flags, LINE_MACRO);
	else
	    turnoff(glob_flags, LINE_MACRO);
	if (ison(mac_stack->m_cmd, QUOTE_MACRO))
	    turnon(glob_flags, QUOTE_MACRO);
	else
	    turnoff(glob_flags, QUOTE_MACRO);
    }
}

/* Abandon macro processing */
void
mac_flush()
{
    while (mac_stack)
	mac_pop();
    if (mac_hide) {
	mac_stack = mac_hide;
	mac_hide = 0;
	while (mac_stack)
	    mac_pop();
    }
    turnoff(glob_flags, IN_MACRO);
    turnoff(glob_flags, LINE_MACRO);
    turnoff(glob_flags, QUOTE_MACRO);
}

/* Check for pending input from a macro. */
int
mac_pending()
{
    register struct cmd_map *msp;

    for (msp = mac_stack; msp && !*(msp->m_str); msp = msp->m_next)
	;

    return !!msp;
}

/* Get input and treat it as a macro.  */
int
get_mac_input(newline)
int newline;	/* 1 if newline to be appended, 0 otherwise */
{
    register int len;
    char buf[MAX_MACRO_LEN];

    /* This call cannot be nested */
    if (mac_hide)
	return -1;

    /* Hide the mac_stack so input comes from stdin */
    mac_hide = mac_stack; mac_stack = 0;

    if ((len = Getstr("", buf, MAX_MACRO_LEN - 1, 0)) < 0)
	return len;
    if (newline) {
	buf[len++] = '\n';
	buf[len] = 0;
    }

    /* Restore the mac_stack */
    if (mac_stack) {
	/*
	 * Somehow, a push happened even though mac_hide was
	 * nonzero -- maybe by line wrap?  Fix it as best we can.
	 */
	struct cmd_map *msp;
	for (msp = mac_stack; msp->m_next; msp = msp->m_next)
	    ;
	msp->m_next = mac_hide;
    } else
	mac_stack = mac_hide;
    mac_hide = 0;

    /* Restore saved flags */
    if (mac_stack) {
	if (ison(mac_stack->m_cmd, IN_MACRO))
	    turnon(glob_flags, IN_MACRO);
	else
	    turnoff(glob_flags, IN_MACRO);
	if (ison(mac_stack->m_cmd, LINE_MACRO))
	    turnon(glob_flags, LINE_MACRO);
	else
	    turnoff(glob_flags, LINE_MACRO);
    }
    if (len > 0)
	Ungetstr(buf);

    return 1;
}

/* getchar() substitute -- reads from the current macro if one is active,
 * otherwise does a getchar().
 *
 * NOTE:  In the mac_stack, x_str is the saved text of the current macro,
 *  and m_str is the current read position within the macro.
 */
int
m_getchar()
{
    int c;

    while (mac_stack && (! *(mac_stack->m_str)))
	mac_pop();
    if (mac_stack) {
	c = *((mac_stack->m_str)++);
	return c;
    } else {
	turnoff(glob_flags, IN_MACRO);
	turnoff(glob_flags, LINE_MACRO);
	turnoff(glob_flags, QUOTE_MACRO);
	errno = 0;
	while ((c = Sgetc(stdSource)) == 0 || c == EOF)
	    if (c == EOF && errno != EINTR)
		break;
	return c;
    }
}

void
m_ungetc(c)
int c;
{
    if (mac_stack && (mac_stack->m_str > mac_stack->x_str))
	*(--(mac_stack->m_str)) = (char) c;
    else
	(void) Sungetc(c, stdSource);
}

/*
 * Try to read a long command; assumes MAC_LONG_CMD already seen.
 *  On immediate failure, return 0.
 *  On failure after reading some input, return less than zero.
 *  On success, return greater than 0.
 * The absolute value of the return is the number of chars placed in buf.
 */
int
read_long_cmd(buf)
char *buf;
{
    register char c, *p = buf;
    register int count = 0;

    /*
     * Test in_macro() in this loop because the _entire_
     * long command _must_ be in the macro -- if we run
     * out of macro in mid-long-command, it is an error.
     */
    while (in_macro() && (count < MAX_LONG_CMD - 1)
	    && ((c = m_getchar()) != MAC_LONG_END)) {
	*p++ = c; ++count;
    }
    *p = '\0';
    if (c != MAC_LONG_END)
	return (-count);
    return count;
}

/*
 * Identify and possibly execute a reserved long macro command
 * Executes if do_exec is true.  Otherwise, just parse.
 */
int
reserved_cmd(buf, do_exec)
char *buf;
{
    int ret = 1;

    if (!strcmp(buf, MAC_GET_STR)) {
	if (do_exec)
	    ret = get_mac_input(0);
    } else if (!strcmp(buf, MAC_GET_LINE)) {
	if (do_exec)
	    ret = get_mac_input(1);
    } else
	ret = 0;
    return ret;
}

#ifdef CURSES

/*
 * Identify (and possibly execute, if reserved) curses mode commands
 *  that appear in macro strings enclosed by MAC_LONG_CMD and
 *  MAC_LONG_END.  Return the binding of the command.
 */
int
long_mac_cmd(c, do_exec)
int c;
{
    char buf[MAX_LONG_CMD];
    register int count, binding;
    int y, x;

    if (c != MAC_LONG_CMD)
	return C_ERROR;

    if ((count = read_long_cmd(buf)) <= 0) {
	print("Invalid long macro command");
	if (ison(glob_flags, CNTD_CMD))
	    putchar('\n');
	if (do_exec)
	    mac_flush();
	return C_ERROR;
    }

    if (do_exec) {
	if (ison(glob_flags, CNTD_CMD))
	    clr_bot_line();
	getyx(stdscr, y, x);
	move(LINES - 1, 0);
    }
    if (reserved_cmd(buf, do_exec)) {
	if (do_exec) {
	    if (isoff(glob_flags, CNTD_CMD))
		move(y, x);
	    return getcmd();
	} else
	    return C_NULL;
    } else if (do_exec)
	move(y, x);

    /* Can start at C_NULL because of "no-op" command */
    for (count = 0; count <= C_HELP; count++) {
	if (!strcmp(buf, map_func_names[count].m_str)) {
	    binding = (int)map_func_names[count].m_cmd;
	    break;
	}
    }
    if (count > C_HELP) {
	if (do_exec) {
	    (void) cmd_line(zmVaStr("%s %d", buf, current_msg+1), NULL_GRP);
	    turnon(glob_flags, CNTD_CMD);
	}
	return C_NULL;
    }
    /* Don't allow C_MACRO to be called directly */
    if (count > C_HELP || binding == C_MACRO) {
	print("Invalid long macro command");
	if (ison(glob_flags, CNTD_CMD))
	    putchar('\n');
	return C_ERROR;
    } else
	return binding;
}

#endif /* CURSES */

/*
 * Check the validity of a macro binding as far as possible
 */
int
check_mac_bindings(buf)
char *buf;
{
    int ok = TRUE;

    while (ok && buf && *buf) {
	if (*buf == MAC_LONG_CMD) {
	    char *i;
#ifdef CURSES
	    int count;
#endif /* CURSES */

	    if (ok)
		ok = ((i = index(++buf, MAC_LONG_END)) != NULL);
	    if (i)
	        *i = '\0';      /* Don't worry, we'll fix it */
	    else
	        return ok;
#ifdef CURSES
	    /* OK to start at C_NULL because of "no-op" command */
	    for (count = 0; count <= C_HELP; count++)
	        if (! strcmp(buf, map_func_names[count].m_str))
	            break;
	    /* Don't allow C_MACRO to be called directly */
	    if (count == C_MACRO)
	        ok = FALSE;
	    else if (count > C_HELP)
	    if (ok && !(ok = reserved_cmd(buf, FALSE)))
		wprint(catgets( catalog, CAT_SHELL, 453, "Warning: unrecognized fullscreen command: \"%s\"\n" ),
		    buf);
#endif /* CURSES */
	    buf = i;
	    *buf++ = MAC_LONG_END;
	} else if (*buf++ == '\\' && *buf)
	    ++buf;
    }
    return ok;
}
