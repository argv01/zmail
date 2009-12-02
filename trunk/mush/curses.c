/* @(#)curses.c	(c) copyright 3/18/87 (Dan Heller) */

/* curses.c -- routine to deal with the curses interface */

#include "mush.h"
#ifdef CURSES
#include "bindings.h"

curses_init(argc, argv)
register char **argv;
{
    char buf[80];
    extern char *UP;
#if !defined(M_UNIX) && !defined(__linux__)
    extern char ttytype[];
#endif /* M_UNIX */

    if (argv && *++argv && !strcmp(*argv, "-?"))
	return help(0, "curses", cmd_help);
#ifdef SUNTOOL
    if (istool) {
	print("Sorry, can't change to curses mode from tool.\n");
	return -1;
    } else
#endif /* SUNTOOL */
    if (!is_shell) {
	/*
	 * Can't start curses, but we can prepare to.
	 * Also allow -C switch to be shut off.
	 */
	if (argv && *argv && !lcase_strncmp(*argv, "off", -1))
	    turnoff(glob_flags, PRE_CURSES);
	else
	    turnon(glob_flags, PRE_CURSES);
	return 0;
    } else if (argc && (iscurses || ison(glob_flags, PRE_CURSES))) {
	print("You are already using curses mode.\n");
	return -1;
    } else if (ison(glob_flags, IS_GETTING)) {
	print("Finish your letter first.\n");
	return -1;
    }

#ifndef attrset		/* terminfo version of curses */
    /* you can not start curses in no echo mode.. must be in normal mode */
    echom();
    nocrmode();
#endif /* attrset */
    (void) initscr();
#ifdef SIGCONT
    /* initscr will play with signals -- make sure they're set right. */
    (void) signal(SIGTSTP, stop_start);
    (void) signal(SIGCONT, stop_start);
#endif /* SIGCONT */
#if !defined(SYSV) && !defined(USG)
    if (!UP || !*UP)
#else /* ~SYSV && ~USG */
    if (!stdscr)
#endif /* ~SYSV && ~USG */
		 {
	print("Terminal type %s can not use the curses interface.\n", ttytype);
	return -1;
    }
    iscurses = TRUE;
    noechom(); /* reset tty state -- */
    crmode(); /* do not use "echo_on/off()" */
    scrollok(stdscr, TRUE);
    /* if the user hasn't set his screen explicitly, set it for him */
    set_screen_size();
    if (crt > LINES - 1 || !do_set(set_options, "crt")) {
	crt = LINES;
	(void)cmd_line(sprintf(buf, "\\set screen = %d crt = %d", screen, crt),
	    msg_list);
    } else
	(void)cmd_line(sprintf(buf, "\\set screen = %d", screen), msg_list);
    if (argc) {
	(void) cmd_line(sprintf(buf, "\\headers %d", current_msg+1), msg_list);
	(void) curses_help_msg(TRUE);
    }
    if (!do_set(set_options, "no_reverse"))
	turnon(glob_flags, REV_VIDEO);
    turnoff(glob_flags, CONT_PRNT);
    return 0; /* doesn't affect messages */
}

struct cmd_map *active_cmd;	/* See bindings.h for description */

/*
 * get input in cbreak mode and execute the appropriate command.
 * when the command is done (usually), the user is prompted to
 * hit any key to continue. At this point, the user may enter a
 * new command so no screen refreshing needs to be done. This
 * new command is returned to caller and may be passed back.
 *
 * The flag CNTD_CMD (continued command) is set if
 * this routine is called with the passed parameter (c) != 0. If
 * so, then the character passed is the character input by the
 * user at the last "hit return" prompt indicating that he wants
 * to execute a new command and not draw the screen.
 *
 * CNTD_CMD is also set if the command that the user invokes
 * causes any sort of output that requires a screen refresh.  The
 * variable redo is set to 1 if the header page not only requires
 * redrawing, but updating ... (new call to do_hdrs)
 *
 * calls that say: print("%s", compose_hdr(current_msg)) are constructed
 * that way because if the header has a `%' in it, then print will try to
 * expand it.
 */
curses_command(c)
register int c;
{
    char	buf[BUFSIZ], file[128], list[128];
    int 	n, curlin;
    static int  redo = 0;  /* set if headers should be redrawn */

    if (c != 0)
	turnon(glob_flags, CNTD_CMD);
    else
	turnoff(glob_flags, CNTD_CMD);
    clear_msg_list(msg_list); /* play it safe */
    if (isoff(glob_flags, CNTD_CMD)) {
	(void) check_new_mail();
	curlin = max(1, current_msg - n_array[0] + 1);
	if (ison(glob_flags, REV_VIDEO) && msg_cnt) {
	    scrn_line(curlin, buf);
	    STANDOUT(curlin, 0, buf);
	}
	mail_status(0);
	move(curlin, 0), refresh();
	/* reprint to remove reverse video from current line (don't refresh) */
	if (ison(glob_flags, REV_VIDEO) && msg_cnt)
	    mvaddstr(curlin, 0, buf);
	c = getcmd(); /* get input AFTER line redrawn without reverse video */
    }
    buf[0] = list[0] = file[0] = '\0';

    if (c == C_WRITE_LIST || c == C_SAVE_LIST || c == C_COPY_LIST
	   || c == C_DELETE_LIST || c == C_UNDEL_LIST) {
	if (msg_cnt < 1) {
	    mac_flush();
	    print("Not enough messages.");
	    c = C_NULL;
	} else if (c == C_DELETE_LIST && ison(glob_flags, READ_ONLY)) {
	    mac_flush();
	    print("Folder is read-only.");
	    c = C_NULL;
	} else if (!curses_msg_list(sprintf(buf, "%s msg list: ",
		(c == C_WRITE_LIST)? "write" : (c == C_SAVE_LIST)?  "save" :
		(c == C_COPY_LIST)? "copy" :
		(c == C_DELETE_LIST)? "delete" : "undelete"), list, msg_list))
	    c = C_NULL;
	if (ison(glob_flags, CNTD_CMD))
	    putchar('\n');
    }

    /* first do non-mail command type stuff */
    switch (c) {
	case C_ERROR :
	    bell();
	    mac_flush();

	when C_NULL :
	    if (isoff(glob_flags, CNTD_CMD))
		bell();

	/* goto a specific message number */
	when C_GOTO_MSG :
	    if (curses_msg_list(strcpy(buf, "goto msg: "), list, msg_list)) {
		/*
		 * Reset the current message in case a 
		 * backquoted command (like `from`) changed it
		 */
		n = current_msg;
		do if (++n >= msg_cnt)
		    n = 0;
		while (n != current_msg && !msg_bit(msg_list, n));
		if (n == current_msg && !msg_bit(msg_list, n)) {
		    mac_flush(); /* bail out if in macro processing */
		    print("Message not found.");
		}
		else if ((current_msg = n) < n_array[0]
			|| n > n_array[screen-1])
		    redo = 1;
	    } else {
		mac_flush();
		bell();
	    }
	    if (ison(glob_flags, CNTD_CMD) && msg_cnt)
		print("%-.*s", COLS-2, compose_hdr(current_msg));
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');

	/* screen optimization stuff */
	when C_REVERSE :
	    if (ison(glob_flags, REV_VIDEO))
		turnoff(glob_flags, REV_VIDEO);
	    else
		turnon(glob_flags, REV_VIDEO);

	when C_REDRAW : redo = 1;

	/*
	 * screen movement
	 */
	when C_NEXT_MSG :
	    if (current_msg + 2 > msg_cnt ||
		isoff(glob_flags, CNTD_CMD) && curlin == LINES-2) {
		mac_flush();	/* Bail out if in macro processing */
		bell();		/* reached the end */
	    } else {
		if (ison(glob_flags, CNTD_CMD)) {
		    if (++current_msg > n_array[screen-1])
			redo = 1;
		    print("%-.*s", COLS-2, compose_hdr(current_msg));
		    putchar('\n');
		} else {
		    if (++current_msg > n_array[screen-1])
			n_array[screen++] = current_msg;
		    move(++curlin, 0);
		    printw("%-.*s", COLS-2, compose_hdr(current_msg));
		    clrtoeol();
		}
	    }

	when C_PREV_MSG :
	    if (isoff(glob_flags, CNTD_CMD) && curlin == 1 ||
		    current_msg == 0 || msg_cnt == 0) {
		mac_flush();	/* Bail out if in macro processing */
		bell();  	/* at the beginning */
	    } else {
		if (--current_msg < n_array[0])
		    redo = 1;
		if (ison(glob_flags, CNTD_CMD)) {
		    print("%-.*s", COLS-2, compose_hdr(current_msg));
		    putchar('\n');
		}
	    }

	when C_FIRST_MSG : case C_LAST_MSG :
	    if (!msg_cnt) {
		mac_flush();
		bell();
		break;
	    }
	    n = current_msg;
	    move(LINES-1, 0), refresh();
	    if (c == C_FIRST_MSG && (current_msg = 0) < n_array[0] ||
		c == C_LAST_MSG && (current_msg = msg_cnt-1)> n_array[screen-1])
		if (isoff(glob_flags, CNTD_CMD))
		    (void) cmd_line(sprintf(buf, "\\headers %d", current_msg+1),
			     msg_list);
		else
		    redo = 1;
	    if (ison(glob_flags, CNTD_CMD) && n != current_msg)
		print("%-.*s", COLS-2, compose_hdr(current_msg)), putchar('\n');

	/* top and bottom of headers screen */
	when C_TOP_PAGE : case C_BOTTOM_PAGE :
	    if (msg_cnt && isoff(glob_flags, CNTD_CMD))
		if (c == C_TOP_PAGE)
		    current_msg = n_array[0];
		else
		    current_msg = min(n_array[screen-1], msg_cnt-1);
	    else {
		mac_flush();
		bell();
	    }

	when C_NEXT_SCREEN : /* next page */
	    move(LINES-1, 0), refresh();
	    if (msg_cnt-1 > n_array[screen-1]) {
		clear();
		set_screen_size();
		(void) cmd_line(strcpy(buf, "\\headers +"), msg_list);
		if (current_msg < n_array[0])
		    current_msg = n_array[0];
		(void) curses_help_msg(TRUE);
		return redo = 0;
	    } else {
		mac_flush();
		bell();
	    }

	when C_PREV_SCREEN : /* previous page */
	    move(LINES-1, 0), refresh();
	    if (n_array[0] > 0) {
		clear();
		set_screen_size();
		(void) cmd_line(strcpy(buf, "\\headers -"), msg_list);
		if (current_msg > n_array[screen-1])
		    current_msg = n_array[screen-1];
		(void) curses_help_msg(TRUE);
		return redo = 0;
	    } else {
		mac_flush();
		bell();
	    }

	/* read from/save to record file (.mushrc) */
	when C_SOURCE : case C_SAVEOPTS : {
	    int argc;
	    char *argv[3];
	    print("%s filename [default]: ",
		(c == C_SOURCE)? "source" : "save options to");
	    argc = Getstr(file, COLS-40, 0);
	    clr_bot_line();
	    if (argc < 0)
		break;
	    if (argc > 0)
		argv[1] = file, argc = 2;
	    else
		argc = 1;
	    argv[argc] = NULL;
	    turnon(glob_flags, PRE_CURSES);
	    if (c == C_SOURCE) {
		(void) source(argc, argv);
		mac_flush(); /* can't change things in mid-macro */
		redo = isoff(glob_flags, CNTD_CMD);
	    } else
		(void) save_opts(argc, argv);
	    turnoff(glob_flags, PRE_CURSES);
	}

	/*
	 * search commands
	 */
	when C_NEXT_SEARCH : case C_PREV_SEARCH : case C_CONT_SEARCH :
	    if (c != C_CONT_SEARCH)
		c = search(0 + (c == C_PREV_SEARCH));
	    else
		c = search(-1);
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');
	    if (c == 0)
		break;
	    if (ison(glob_flags, CNTD_CMD))
		print("%-.*s",COLS-2, compose_hdr(current_msg)), putchar('\n');
	    if (n_array[0] > current_msg || n_array[screen-1] < current_msg) {
		redo = 1;
		if (isoff(glob_flags, CNTD_CMD))
		    (void) cmd_line(sprintf(buf, "\\headers %d",
					    current_msg+1), msg_list);
	    }

	/*
	 * actions on messages
	 */
	/* delete/undelete */
	when C_DELETE_MSG : case C_DELETE_LIST :
	case C_UNDEL_MSG : case C_UNDEL_LIST :
	    if (!msg_cnt) {
		print("No messages.");
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n');
		break;
	    }
	    if (ison(glob_flags, READ_ONLY)) {
		mac_flush();
		print("Folder is read-only.");
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n');
		break;
	    }
	    Debug("current message = %d", current_msg + 1);
	    if (!*list)
		set_msg_bit(msg_list, current_msg);
	    turnon(glob_flags, DO_UPDATE);
	    for (n = 0; n < msg_cnt; n++)
		if (msg_bit(msg_list, n)) {
		    if (c == C_DELETE_MSG || c == C_DELETE_LIST)
			turnon(msg[n].m_flags, DELETE|DO_UPDATE);
		    else
			turnoff(msg[n].m_flags, DELETE);
		    if (isoff(glob_flags, CNTD_CMD) && (msg_cnt < screen ||
			n >= n_array[0] && n <= n_array[screen-1])) {
			move(max(1, n - n_array[0] + 1), 0);
			printw("%-.*s", COLS-1, compose_hdr(n));
		    } else
			redo = 1;
		}
	    if (ison(glob_flags, CNTD_CMD) || *list) {
		/* print(), THEN putchar() -- overwrite line */
		if (ison(glob_flags, CNTD_CMD)) {
		    print("%sdeleted %s",
		    (c == C_DELETE_MSG || c == C_DELETE_LIST)? "":"un", list);
		    putchar('\n');
		}
		if (c == C_DELETE_MSG || c == C_DELETE_LIST) {
		    if (ison(msg[current_msg].m_flags, DELETE) ||
			    ison(msg[current_msg].m_flags, SAVED))
			(void) next_msg();
		    if (isoff(msg[current_msg].m_flags, DELETE) &&
			    do_set(set_options, "autoprint"))
			return C_DISPLAY_MSG;
		}
		if (ison(glob_flags, CNTD_CMD))
		    puts(compose_hdr(current_msg));
		else if (current_msg < n_array[0]
			|| current_msg > n_array[screen-1])
		    redo = 1;
	    }

	/*
	 * write/save messages.  If a list is necessary, the user already
	 * entered it above since he must have used a capital letter. If so,
	 * list will contain good data (already been validated above).
	 * if a list is given, set iscurses to 0 so that print statements
	 * will scroll and the user sees the multiple output. else, one
	 * line can go on the bottom line just fine.
	 */
	when C_WRITE_MSG : case C_SAVE_MSG : case C_COPY_MSG :
	case C_WRITE_LIST : case C_SAVE_LIST : case C_COPY_LIST : {
	    register char *p =
		(c == C_WRITE_MSG || c == C_WRITE_LIST)? "write" :
		(c == C_SAVE_MSG  || c == C_SAVE_LIST)? "save" : "copy";
	    if (!msg_cnt) {
		print("No messages.");
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n');
		break;
	    }
	    if (c != C_WRITE_MSG && c != C_WRITE_LIST) {
		char *f = do_set(set_options, "mbox");
		if (f)
		    (void) sprintf(file, "[%s]", f);
		else
		    (void) sprintf(file, "[%s]", DEF_MBOX);
	    } /* else file is already init'd to "" */
	    print(sprintf(buf, "filename to %s%s: ", p, file));
	    if (Getstr(file, COLS-1-strlen(buf), 0) >= 0) {
		char *argv[3];
		clr_bot_line();
		argv[0] = strcpy(buf, p);
		p = file; skipspaces(0);
		argv[1] = (*p) ? p : NULL;
		argv[2] = NULL;
		if (!*list)
		    set_msg_bit(msg_list, current_msg);
		move(LINES-1, 0), refresh();
		if (*list)
		    iscurses = FALSE;
		/* Turn on piping to make save_msg look at msg_list */
		turnon(glob_flags, IS_PIPE);
		if (save_msg(1 + (*file != '\0'), argv, msg_list) < 0)
		    *list = 0;
		turnoff(glob_flags, IS_PIPE);
		if (ison(glob_flags, CNTD_CMD))
		    redo = 1, putchar('\n'), puts(compose_hdr(current_msg));
		if (*list)
		    iscurses = redo = TRUE, turnon(glob_flags, CNTD_CMD);
		else if (isoff(glob_flags, CNTD_CMD) && msg_cnt) {
		    move(curlin, 0);
		    printw("%-.*s", COLS-1, compose_hdr(current_msg));
		    }
	    } else {
		print("No messages saved.");
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n');
	    }
	}

	/* preserve message or place mark on message */
	when C_PRESERVE : case C_MARK_MSG :
	    if (!msg_cnt) {
		print("No messages.");
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n');
		break;
	    }
	    if (ison(msg[current_msg].m_flags,
			c == C_MARK_MSG ? M_PRIORITY(0) : PRESERVE))
		turnoff(msg[current_msg].m_flags,
			c == C_MARK_MSG ? M_PRIORITY(0) : PRESERVE);
	    else
		turnon(msg[current_msg].m_flags,
			c == C_MARK_MSG ? M_PRIORITY(0) : PRESERVE);
	    if (c != C_MARK_MSG)
		turnon(glob_flags, DO_UPDATE);
	    if (ison(glob_flags, CNTD_CMD)) {
		wprint("%-.*s\n", COLS-1, compose_hdr(current_msg));
		redo = 1;
	    } else {
		move(curlin, 0);
		printw("%-.*s", COLS-1, compose_hdr(current_msg));
	    }

	/* order messages (sort) and redisplay the headers */
	when C_SORT : case C_REV_SORT :
	    (void) strcpy(file, "sort");
	    if (c == C_REV_SORT) {
		print("Reverse "), turnon(glob_flags, CONT_PRNT);
		(void) strcat(file, " -");
	    }
	    print(
	"Order messages by [author, date, length, Status, subject, priority]: "
		);
	    if ((c = m_getchar()) == 'a' || c == 'd' || c == 'l' ||
		    c == 'S' || c == 's' || c == 'R' || c == 'p') {
		print("reordering messages...");
		(void) cmd_line(sprintf(buf, "%s %c", file, c), msg_list);
		print_more("done.");
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n'), puts(compose_hdr(current_msg));
		redo = 1;
	    } else
		clr_bot_line();

	when C_QUIT_HARD :
	    (void) mush_quit(0, DUBL_NULL);
	    redo = 1; /* new mail must have come in */

	/* quit or update -- vrfy_update (returns 1 if updated) */
	when C_QUIT : case C_UPDATE : {
	    clr_bot_line();
	    redo = (c == C_UPDATE);
	    if (!vrfy_update(&redo))
		if (c == C_UPDATE)
		    break;
	    if (isoff(glob_flags, CNTD_CMD))
		(void) cmd_line(sprintf(buf, "\\headers %d", current_msg+1),
				msg_list);
	}

	when C_EXIT : case C_EXIT_HARD :
	    clr_bot_line();
	    iscurses = FALSE;
	    if (c != C_EXIT && c != C_EXIT_HARD)
		putchar('\n');
	    cleanup(0);

	/* change to a new folder */
	when C_FOLDER :
	    for (;;) {
		SIGRET (*oldint)(), (*oldquit)();
		on_intr();
		print("New folder (?=list): ");
		c = Getstr(file, COLS-22, 0);
		off_intr();
		if (c > 0) {
		    int do_up = 0;
		    if (!strcmp(file, "?")) {
			clr_bot_line();
			iscurses = 0;
			puts("folders in your folder directory:");
			(void) cmd_line(strcpy(buf, "\\folders"), msg_list);
	puts("Precede folder names with a +. `%' to specify system mailbox.");
			turnon(glob_flags, CNTD_CMD), iscurses = 1;
			continue;
		    }
		    clearok(stdscr, FALSE);
		    /* if vrfy_update doesn't verify, but folder command fails,
		     * then we need to reset the updatability of current folder
		     */
		    c = (ison(glob_flags, DO_UPDATE))? TRUE : FALSE;
		    if (strcmp(file, "-?") != 0) {
			redo = -1; /* so vrfy_update() won't update or quit */
			do_up = vrfy_update(&redo);
		    }
		    move(LINES-1, 0), refresh();
		    if (cmd_line(sprintf(buf, "folder %s -N %s",
				    do_up? "" : "!", file), msg_list) == -1) {
			if (c) /* remember state of updatability of folder */
			    turnon(glob_flags, DO_UPDATE);
			if (ison(glob_flags, CNTD_CMD))
			    putchar('\n');
		    } else
			redo = 1, turnoff(glob_flags, CNTD_CMD);
		    break;
		} else {
		    print("\"%s\" unchanged.", mailfile);
		    if (ison(glob_flags, CNTD_CMD))
			putchar('\n');
		    break;
		}
	    }

	/* shell escape */
	when C_SHELL_ESC :
	    print("Shell command: ");
	    if (Getstr(file, COLS-24, 0) < 0)
		clr_bot_line();
	    else {
		putchar('\n');
		iscurses = FALSE;
		(void) cmd_line(sprintf(buf, "sh %s", file), msg_list);
		iscurses = TRUE;
		turnon(glob_flags, CNTD_CMD);
	    }

	/* do a line-mode like command */
	when C_CURSES_ESC :
	    print(":");
	    if (Getstr(buf, COLS-2, 0) < 0)
		break;
	    putchar('\n');
	    iscurses = FALSE;
	    if (!*buf) {
		/* return -1 because iscurses = 0 is not enough! */
		redo = 0;
		endwin(); /* this turns echoing back on! */
		echo_off();
		return -1;
	    }
	    /* The "source" and "curses" commands need some indication
	     * that we are in curses mode, so use the PRE_CURSES flag.
	     */
	    turnon(glob_flags, PRE_CURSES);
	    (void) cmd_line(buf, msg_list);
	    /* they may have affected message status or had text output */
	    turnon(glob_flags, CNTD_CMD), redo = 1;
	    turnoff(glob_flags, PRE_CURSES);
	    iscurses = TRUE;
	    if (msg_cnt)
		puts(compose_hdr(current_msg));

	/* send message to printer, redo to display 'p' status */
	when C_PRINT_MSG : redo = (lpr(0, DUBL_NULL, msg_list) == 0);

	/* cd */
	when C_CHDIR :
	    print("chdir to [~]: ");
	    if (Getstr(file, COLS-12, 0) < 0)
		break;
	    clr_bot_line();
	    (void) cmd_line(sprintf(buf, "cd %s", file), msg_list);
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');

	/* variable settings */
	when C_VAR_SET : case C_IGNORE : case C_ALIAS : case C_OWN_HDR :
	    curs_vars(c); /* CNTD_CMD is reset if there's output! */

	when C_VERSION :
	    (void) do_version();
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');

	when C_MAIL_FLAGS :
	    print("flags [-?]: ");
	    if ((c = Getstr(file, COLS-12, 0)) < 0)
		break;
	    putchar('\n');
	    if (c == 0)
		(void) strcpy(file, "-?");
	    else
		redo = 1; /* In case of -f flag, to display the 'f' status */
	/* Fall thru */
	case C_MAIL : {
	    u_long flgs = glob_flags;
	    turnon(glob_flags, IGN_BANG);
	    clr_bot_line();
	    iscurses = FALSE;
	    (void) cmd_line(sprintf(buf, "mail %s", file), msg_list);
	    glob_flags = flgs;
	    iscurses = TRUE, turnon(glob_flags, CNTD_CMD);
	    if (msg_cnt)
		print("%-.*s", COLS-2, compose_hdr(current_msg)), putchar('\n');
	}

	/* reply to mail */
	when C_REPLY_SENDER : case C_REPLY_ALL : {
	    register char *p = (c == C_REPLY_ALL)? "replyall" : "replysender";
	    clr_bot_line();
	    iscurses = FALSE;
	    if (isoff(msg[current_msg].m_flags, REPLIED))
		redo = 1;
	    (void) cmd_line(sprintf(buf, "%s %d", p, current_msg+1),
		msg_list);
	    if (msg_cnt)
		puts(compose_hdr(current_msg));
	    iscurses = TRUE, turnon(glob_flags, CNTD_CMD);
	}

	/* type out a message */
	when C_DISPLAY_MSG : case C_TOP_MSG : case C_DISPLAY_NEXT :
	    if (!msg_cnt ||
		c != C_DISPLAY_NEXT && ison(msg[current_msg].m_flags, DELETE)) {
		if (!msg_cnt)
		    print("No messages.");
		else
		    print("Message %d deleted; type 'u' to undelete.",
				      current_msg+1);
		if (ison(glob_flags, CNTD_CMD))
		    putchar('\n');
		break;
	    }
	    clr_bot_line();
	    iscurses = FALSE;
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');
	    if (c == C_DISPLAY_MSG)
		c = cmd_line(strcpy(buf, "type"), msg_list);
	    else if (c == C_TOP_MSG)
		c = cmd_line(strcpy(buf, "top"), msg_list);
	    else {
		/* "next" screws up the screen whether it displays or not */
		(void) cmd_line(strcpy(buf, "next"), msg_list);
		c = 0;
	    }
	    if (c > -1)
		turnon(glob_flags, CNTD_CMD), redo = 1;
	    iscurses = TRUE;
	    puts(compose_hdr(current_msg));

	/* bind a key or string to a curses-mode command */
	when C_BIND :  case C_UNBIND : case C_MAP : case C_BIND_MACRO :
	case C_MAP_BANG : {
	    char *argv[2];
	    argv[0] = (c == C_BIND) ? "bind" :
		      (c == C_UNBIND) ? "unbind" :
		      (c == C_MAP) ? "map" :
		      (c == C_MAP_BANG) ? "map!" : "bind-macro";
	    argv[1] = NULL;
	    if (bind_it(1, argv) < -1)
		turnon(glob_flags, CNTD_CMD);
	    else if (ison(glob_flags, CNTD_CMD)) /* if it was set anyway */
		putchar('\n');
	    else
		(void) curses_help_msg(TRUE);
	}

	when C_MACRO : 
	    turnon(glob_flags, IN_MACRO);
	    /* Current macro should already be in the mac_stack, so
	     * all we have to do here is look for the next character
	     */

	/* help stuff */
	when C_HELP :
	    move(LINES-1, 0), refresh();
	    (void) help(0, "curses", cmd_help);
	    turnon(glob_flags, CNTD_CMD);
	    if (msg_cnt)
		puts(compose_hdr(current_msg));

	otherwise :
	    mac_flush();
	    bell();
	    if (ison(glob_flags, CNTD_CMD)) {
		/* use print instead of puts to overwrite hit_return msg */
		print("unknown command"), putchar('\n');
		redo = 1;
	    }
    }

    if (ison(glob_flags, CNTD_CMD)) {
	int old_cnt = msg_cnt;
	if (!(c = hit_return()) && !redo && msg_cnt == old_cnt)
	    redraw();
	clr_bot_line();
	if (old_cnt !=  msg_cnt)
	    redo = 1;
	if (c)
	    return c;
    }
    if (redo) {
	set_screen_size(); /* it may have changed */
	n = current_msg;
	clear();
	if (/* msg_cnt < screen || */ n_array[0] < n && n < n_array[screen-1])
	    (void) do_hdrs(0, DUBL_NULL, NULL);
	else
	    (void) cmd_line(sprintf(buf, "\\headers %d", n+1), msg_list);
	(void) curses_help_msg(TRUE);
	redo = 0;
    }
    return 0;
}

vrfy_update(redo)
int *redo;
{
    char buf[16];
    int c;

    /* update current folder */
    if (ison(glob_flags, DO_UPDATE)) {
	if (ison(glob_flags, READ_ONLY)) {
	    mac_flush();
	    print("Folder is read-only.");
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');
	    return 0;
	}
	print("Update folder [y]? ");
	if ((c = getchar()) != 'y' && c != 'Y' && c != '\n' && !isspace(c)) {
	    print("Folder unchanged.");
	    if (ison(glob_flags, CNTD_CMD))
		putchar('\n');
	    return 0;
	}
	if (*redo < 0)
	    return 1;
    } else if (*redo)
	return 1;
    if (cmd_line(strcpy(buf, *redo? "update" : "quit"), msg_list) != -1
	    && ison(glob_flags, CNTD_CMD))
	*redo = 1, turnoff(glob_flags, CNTD_CMD);
    turnoff(glob_flags, DO_UPDATE);
    return 1; /* make sure bottom line is clear and no reverse video */
}

scrn_line(line, buf)
char *buf;
{
#ifndef AIX
#ifndef A_CHARTEXT
    (void) strncpy(buf, stdscr->_y[line], COLS-1);
    buf[COLS-1] = 0; /* strncpy does not null terminate */
#else
    int n;

    for (n = 0; n < COLS; n++)
	if ((buf[n] = (mvinch(line, n) & A_CHARTEXT)) == '\0')
	    break;
    buf[n] = '\0';
#endif /* A_CHARTEXT */
#else /* AIX */
    (void) strncpy(buf, compose_hdr(n_array[line-1]), COLS - 1);
#endif /* AIX */
}

/*
 * Generate the help message from the variable curses_help.
 *  If visible is true, the message is displayed,
 *  otherwise its size (in lines) is computed and returned.
 */
curses_help_msg(visible)
int visible;
{
    int count, i, len, siz = 0, mxm = 0;
    static int old_siz = 0;
    register struct cmd_map *list;
    extern struct cmd_map map_func_names[];
    char *curs_help = do_set(set_options, "curses_help"), **format;

    if (!curs_help) {
	if (old_siz && visible) {
	    int bot = min(n_array[screen-1], msg_cnt-1);
	    move(max(0, bot - n_array[0]) + 2, 0), clrtobot();
	    old_siz = 0;
	}
	return 0;
    } else if (!*curs_help)
	curs_help = DEF_CURSES_HELP;
    /* Split the help string into words */
    if (!(format = mk_argv(curs_help, &count, FALSE)) || count <= 0)
	return 0;
    /* Generate a help message for each word */
    for (i = 0; i < count; i++) {
	char buf[MAX_BIND_LEN*2+MAX_LONG_CMD+5], asc[MAX_BIND_LEN*2];

	buf[0] = '\0'; /* default to empty in case of no match */
	for (list = cmd_map; list; list = list->m_next) {
	    if (!strcmp(format[i], map_func_names[list->m_cmd].m_str)) {
		len = strlen(sprintf(buf, "(%s) %s  ",
				ctrl_strcpy(asc, list->m_str, FALSE),
				map_func_names[list->m_cmd].m_str));
		if (len > mxm)
		    mxm = len;
		break;
	    }
	}
	strdup(format[i], buf); /* replace word with its "definition" */
    }
    /* Columnate the output nicely */
    if (mxm > 0) {
	len = (COLS - 1) / mxm;
	if (len == 0) {
	    if (visible)
		print("Curses help message too long!");
	    return 0;
	}
	siz = count / len;
	if (count % len)
	    siz++;
	if (siz > LINES / 3) {
	    if (visible)
		print("Curses help message too long!");
	    return 0;
	}
	if (visible) {
	    int next = LINES - 1 - siz;
	    if (old_siz > siz) {
		int bot = min(n_array[screen-1], msg_cnt-1);
		move(max(0, bot - n_array[0]) + 2, 0), clrtobot();
	    }
	    old_siz = siz;
	    for (i = 0; i < count; i++) {
		if (!(i % len))
		    move(next, 0), clrtoeol(), ++next;
		if (format[i][0])
		    printw("%-*.*s", mxm, mxm, format[i]);
	    }
	    refresh();
	}
    }
    free_vec(format);
    return siz;
}

set_screen_size()
{
    int hlp_siz = LINES - 2 - curses_help_msg(FALSE); 

    if (!do_set(set_options, "screen"))
#ifdef USG
	switch (_tty.sg_ospeed & CBAUD)
#else /* USG */
	switch (_tty.sg_ospeed)
#endif /* USG */
	{
	    case B300 :  screen = min(hlp_siz, 7);
	    when B1200 : screen = min(hlp_siz, 14);
	    when B2400 : screen = min(hlp_siz, 22);
	    otherwise :  screen = hlp_siz;
	}
    else
	screen = min(screen, hlp_siz);
}

/*
 * prompt for a carriage return, but return whatever user types unless
 * it's a character which he might regret (like 'q' or 'x'). Ignore
 * interrupts (kind of) because we have nowhere to longjmp to.  When we
 * return, we'll setjmp again (top of loop.c)
 */
hit_return()
{
    int c;

    turnon(glob_flags, IGN_SIGS);
    iscurses = FALSE;
    (void) check_new_mail();
    iscurses = TRUE;
    mail_status(1), addstr("...continue... "), refresh();
    c = getcmd();
    turnoff(glob_flags, IGN_SIGS);

    /* don't let the user type something he might regret */
    if (c == C_QUIT || c == C_EXIT)
	return C_NULL;
    return c;
}

curses_msg_list(str, list, m_list)
register char *str, *list;
char m_list[];
{
    register char *p = NULL;
    int c, sv_cur_msg = current_msg;

    print(str);
    c = Getstr(list, COLS-13, 0);
    move(LINES-1, 0), refresh();
    if (c <= 0 || !(p = do_range(list, m_list)) ||
	(p == list && *p && *p != '$' && *p != '^')) {
	if (p)
	    print("Invalid message list: %s", p);
	current_msg = sv_cur_msg;
	return 0;
    }
    current_msg = sv_cur_msg;
    return 1;
}

curs_vars(which)
int which;  /* really, a char */
{
    char c, buf[128], buf2[128], *string;
    struct options **list;

    switch(which) {
	case C_OWN_HDR : string = "my_hdr", list = &own_hdrs;
	when C_ALIAS : string = "alias", list = &aliases;
	when C_IGNORE : string = "ignore", list = &ignore_hdr;
	when C_VAR_SET : string = "set", list = &set_options;
	otherwise : clr_bot_line(); return;
    }

    print("%s [? Set Unset All]: ", string);
    c = m_getchar();
    clr_bot_line();
    switch (Lower(c)) {
	/* if help, print help -- if "all", show all settings. */
	case '?' : case 'a' :
	    if (c == '?') {
		if (!strcmp(string, "set")) {
		    print("which variable? [all <var>]: ");
		    if ((c = Getstr(buf+1, COLS-40, 0)) < 0)
			return;
		    clr_bot_line();
		    buf[0] = '?';
		    if (c > 0) {
			char *argv[3];
			argv[0] = string;
			argv[1] = buf;
			argv[2] = NULL;
			Lower(buf[1]);
			if (!strcmp(buf+1, "a"))
			    (void) strcpy(buf+1, "all");
			if (!strcmp(buf+1, "all"))
			    turnon(glob_flags, CNTD_CMD);
			(void) set(2, argv, (char *) 0);
			break;
		    }
		}
		/* help returns next command (hit_return) */
		(void) help(0, string, cmd_help);
		turnon(glob_flags, CNTD_CMD);
		return;
	    }
	    turnon(glob_flags, CNTD_CMD);
	    (void) do_set(*list, NULL);

	/* if set, prompt for string and let user type */
	when 's' :
	    print("set: ");
	    c = Getstr(buf, COLS-18, 0);
	    clr_bot_line();
	    if (c > 0)
		(void) cmd_line(sprintf(buf2, "%s %s", string, buf), msg_list);

	/* if unset, just as easy as set! */
	when 'u' :
	    print("unset: ", string);
	    if (Getstr(buf, COLS-18, 0) > 0 && !un_set(list, buf))
		print("%s isn't set", buf);
    }
    if (ison(glob_flags, CNTD_CMD))
	putchar('\n');
    else
	(void) curses_help_msg(TRUE);
}
#endif /* CURSES */
