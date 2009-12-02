/* @(#)commands.c	(c) copyright 10/18/86 (Dan Heller) */

#include "mush.h"

/*
 * Note that all of the routines in here act upon and return 0 or -1.
 * if -1, then the main loop will clear message lists.
 */

struct cmd cmds[] = {
#ifdef SIGSTOP
    { "stop", stop },
#endif /* SIGSTOP */
    { "?", 	  question_mark },{ "sh", sh },
    { "alias", 	  do_alias    },  { "unalias",	do_alias   },
    { "expand",	  do_alias    },  { "cmd", 	do_alias   },
    { "uncmd", 	  do_alias    },  { "from",	do_from    },
    { "un_hdr",	  do_alias    },  { "my_hdr",  	do_alias   },
    { "fkey", 	  do_alias    },  { "unfkey", 	do_alias   },
    { "set", 	  set         },  { "unset", 	set 	   },
    { "ignore",	  set         },  { "unignore", set 	   },
    { "version",  do_version  },  { "help",	print_help },
    { "pick", 	  do_pick     },  { "sort", 	sort 	   },
    { "next",	  readmsg     },  { "previous", readmsg    },
    { "type",     readmsg     },  { "print",	readmsg    },
    { "history",  disp_hist   },  { "top",	readmsg	   },
    { "saveopts", save_opts   },  { "source",   source 	   },
    { "headers",  do_hdrs     },  { "ls",	ls	   },
    { "folder",   folder      },  { "update",   folder     },
    { "cd", 	  cd          },  { "pwd",	cd 	   },
    { "exit",	  mush_quit   },  { "quit", 	mush_quit  },
    { "write", 	  save_msg    },  { "save", 	save_msg   },
    { "copy", 	  save_msg    },  { "folders",  folders    },
    { "merge",	  merge_folders },
    { "mark",	  mark_msg    },  { "unmark",	mark_msg   },
#ifdef CURSES
    { "curses",   curses_init },  { "bind",	bind_it    },
    { "unbind",   bind_it     },  { "bind-macro", bind_it  },
    { "unbind-macro", bind_it  },
#endif /* CURSES */
    { "map",      bind_it     },  { "unmap",       bind_it    },
    { "map!",     bind_it     },  { "unmap!",      bind_it    },
    { "preserve", preserve    },  { "unpreserve",  preserve   },
    { "replyall", respond     },  { "replysender", respond    },
    { "delete",	  delete      },  { "undelete",    delete     },
    { "mail", 	  do_mail     },  { "echo",	   do_echo    },
    { "lpr",      lpr	      },  { "alternates",  alts       },
    { "edit",	  edit_msg    },  { "flags",	   msg_flags  },
    { "pipe",     pipe_msg    },  { "eval",	   eval_cmd   },
    { "undigest", do_undigest },  { "await",	   await      },
#ifdef POP3_SUPPORT
    { "check",    await    },
#endif
    { NULL, mush_quit }
};

struct cmd ucb_cmds[] = {
    { "t",   readmsg   }, { "n",  readmsg  }, { "p", readmsg  },
    { "+",   readmsg   }, { "-",  readmsg  }, { "P", readmsg  },
    { "Print", readmsg }, { "T",  readmsg  }, { "Type", readmsg },
    { "x",   mush_quit }, { "q", mush_quit }, { "xit", mush_quit },
    { ":a",  do_hdrs   }, { ":d", do_hdrs  }, { ":r", do_hdrs },
    { ":o",  do_hdrs   }, { ":u", do_hdrs  }, { ":n", do_hdrs },
    { ":s",  do_hdrs   }, { ":p", do_hdrs  }, { ":m", do_hdrs },
    { "z",   do_hdrs   }, { "z-", do_hdrs  }, { "z+", do_hdrs },
    { "h",   do_hdrs   }, { "H",  do_hdrs  },
    { "f",   do_from   }, { "m",  do_mail  }, { "alts", alts  },
    { "d",   delete    }, { "dt", delete   }, { "dp", delete  },
    { "u",   delete    }, { "fo", folder   },
    { "s",   save_msg  }, { "co", save_msg }, { "w", save_msg },
    { "pre", preserve  }, { "unpre", preserve },
    { "R",   respond   }, { "r",   respond },
    { "reply", respond }, { "respond", respond },
    { "v",   edit_msg  }, { "e",   edit_msg },
    { NULL, mush_quit }
};

struct cmd hidden_cmds[] = {
    { "about",	print_help  },
    { "debug", toggle_debug }, { "open", 	nopenfiles },
    { "stty",	my_stty     },
    { "setenv",	Setenv      }, { "unsetenv", 	Unsetenv   },
    { "printenv", Printenv  }, { "Pipe",	pipe_msg   },
    { NULL, mush_quit }
};

toggle_debug(argc, argv)
int argc;
char **argv;
{
    if (argc < 2) /* no value -- toggle "debug" (off/on) */
	debug = !debug;
    else
	debug = atoi(*++argv);
    print("debugging value: %d\n", debug);
    return 0;
}

/* if + was specified, then print messages without headers.
 * n or \n (which will be NULL) will print next unread or undeleted message.
 */
readmsg(x, argv, list)
int x;
char **argv, list[];
{
    register char *p = x? *argv : NULL;
    register long flg = 0;
    extern FILE *ed_fp;
#ifdef SUNTOOL
    SIGRET (*oldint)(), (*oldquit)();
#endif /* SUNTOOL */

    if (x && *++argv && !strcmp(*argv, "-?"))
	return help(0, "readmsg", cmd_help);
    /* View a message as long as user isn't in the editor.
     * If ed_fp is not null, then we've got the
     * file open for typing.  If it's NULL, then an editor is going.
     */
    if (ison(glob_flags, IS_GETTING) && !ed_fp) {
	print("Not while you're in the editor, you don't.\n");
	return -1;
    }
    if (!msg_cnt) {
	print("No messages.\n");
	return -1;
    }
    if (x)
	if (!strcmp(p, "top"))
	    turnon(flg, M_TOP);
	else if (*p == '+') {
	    turnon(flg, NO_PAGE);
	    turnon(flg, NO_HEADER);
	} else if (isupper(*p))
	    turnon(flg, NO_IGNORE);

    if (x && (x = get_msg_list(argv, list)) == -1)
	return -1;
    else if (x == 0) {  /* no arguments were parsed (or given) */
	/* get_msg_list sets current msg on */
	if (isoff(glob_flags, IS_PIPE))
	    unset_msg_bit(list, current_msg);
	/* most commands move to the "next" message. type and print don't */
	if ((!p || !*p || *p == 'n' || *p == '+') && current_msg < msg_cnt &&
				    isoff(msg[current_msg].m_flags, UNREAD))
	    current_msg++;
	if (p && (*p == '-' || !strcmp(p, "previous"))) {
	    while (--current_msg >= 0 &&
		(ison(msg[current_msg].m_flags, DELETE) ||
		 ison(msg[current_msg].m_flags, SAVED)))
		;
	    if (current_msg < 0) {
		print("No previous message.\n");
		current_msg = 0;
		return -1;
	    }
	} else {
	    /*
	     * To be compatible with ucb-mail, find the next available unread
	     * message.  If at the end, only wrap around if "wrap" is set.
	     */
	    if (current_msg == msg_cnt && do_set(set_options, "wrap"))
		current_msg = 0;
	    /* "type" or "print" prints the current only -- "next" goes on.. */
	    if (!p || !*p || *p == 'n')
		while (current_msg < msg_cnt &&
		    (ison(msg[current_msg].m_flags, DELETE) ||
		     ison(msg[current_msg].m_flags, SAVED)))
			current_msg++;
	    if (current_msg >= msg_cnt) {
		print("No more messages.\n");
		current_msg = msg_cnt - 1;
		return -1;
	    }
	}
	if (isoff(glob_flags, IS_PIPE))
	    set_msg_bit(list, current_msg);
    }
#ifdef SUNTOOL
    if (istool > 1)
	on_intr();
#endif /* SUNTOOL */
    current_msg = 0;
    for (x = 0; x < msg_cnt && isoff(glob_flags, WAS_INTR); x++)
	if (msg_bit(list, x)) {
	    if (current_msg > 0 && istool > 1 && isoff(flg, NO_PAGE) &&
		    c_more("Type RETURN for next message, q to quit:") == 'q')
		break;
	    current_msg = x;
#ifdef SUNTOOL
	    if (istool > 1) {
		read_mail(NO_ITEM, 0, NO_EVENT);
		break;
	    }
#endif /* SUNTOOL */
	    display_msg(x, flg);
	}
#ifdef SUNTOOL
    if (istool > 1)
	off_intr();
#endif /* SUNTOOL */
    return 0;
}

preserve(n, argv, list)
int n;		/* no use for argc, so use space for a local variable */
char **argv, list[];
{
    register int unpre;

    unpre = !strncmp(*argv, "un", 2);
    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "preserve", cmd_help);
    if (get_msg_list(argv, list) == -1)
	return -1;
    for (n = 0; n < msg_cnt; n++)
	if (msg_bit(list, n))
	    if (unpre) {
		if (ison(msg[n].m_flags, PRESERVE)) {
		    turnoff(msg[n].m_flags, PRESERVE);
		    turnon(glob_flags, DO_UPDATE);
		}
	    } else {
		if (isoff(msg[n].m_flags, PRESERVE)) {
		    turnon(msg[n].m_flags, PRESERVE);
		    turnon(glob_flags, DO_UPDATE);
		}
	    }
    if (istool)
	(void) do_hdrs(0, DUBL_NULL, NULL);
    return 0;
}

lpr(n, argv, list)
int n;  /* no use for argc, so use its address space for a variable */
char **argv, list[];
{
    register FILE	*pp;
    register long 	flags = 0;
    char		print_cmd[128], *printer, c, *cmd;
    int			total = 0;
    SIGRET		(*oldint)(), (*oldquit)();

#ifdef PRINTER_OPT
    char *opt = PRINTER_OPT;
#else
    char opt[2];
#ifdef SYSV
    opt[0] = 'd';
#else
    opt[0] = 'P';
#endif /* SYSV */
    opt[1] = 0;
#endif /* PRINTER_OPT */

    if (!chk_option("alwaysignore", "printer"))
	turnon(flags, NO_IGNORE);
#ifdef MSG_SEPARATOR
    turnon(flags, NO_SEPARATOR);
#endif /* MMDF */
    if (!(printer = do_set(set_options, "printer")) || !*printer)
	printer = DEF_PRINTER;
    while (argv && *++argv && **argv == '-') {
	n = 1;
	while (c = argv[0][n++])
	    switch(c) {
		case 'n': turnon(flags, NO_HEADER);
		when 'h': turnoff(flags, NO_IGNORE);
		when 'P': case 'd':
#ifndef PRINTER_OPT
		    opt[0] = argv[0][n-1];
#endif /* PRINTER_OPT */
		    if (!argv[0][n] && !(n = 0, *++argv)) {
		        print("specify printer!\n");
		        return -1;
		    }
		    printer = argv[0] + n;
		    n += strlen(printer);
		otherwise: return help(0, "lpr", cmd_help);
	    }
    }
    if (get_msg_list(argv, list) == -1)
	return -1;

    if (cmd = do_set(set_options, "print_cmd"))
	(void) strcpy(print_cmd, cmd);
    else
	(void) sprintf(print_cmd, "%s %s%s", LPR, opt, printer);
    Debug("print command: %s\n", print_cmd);
    if (!(pp = popen(print_cmd, "w"))) {
	error("cannot print");
	return -1;
    }
    on_intr();
    for (n = 0; isoff(glob_flags, WAS_INTR) && n < msg_cnt; n++) {
	if (msg_bit(list, n)) {
	    if (total++)
		(void) fputc('\f', pp); /* send a formfeed for multiple copies */
	    print("printing message %d...", n+1);
	    print_more("(%d lines)\n", copy_msg(n, pp, (u_long) flags, NULL));
	    turnon(msg[n].m_flags, PRINTED|DO_UPDATE);
	    turnon(glob_flags, DO_UPDATE);
	}
    }
    off_intr();
    (void) pclose(pp);
    print_more("%d message%s printed ", total, (total==1)? "": "s");
    if (cmd)
	print_more("through \"%s\".\n", cmd);
    else
	print_more("at \"%s\".\n", printer);
    return 0;
}

/* save [msg_list] [file] */
save_msg(n, argv, list)   /* argc isn't used, so use space for variable 'n' */
int n;
char **argv, list[];
{
    register FILE	*mail_fp = NULL_FILE;
    register char 	*file = NULL, *mode, firstchar = **argv, *tmp = ".";
    int 		msg_number, force = 0, by_subj = 0, by_author = 0;
    char		buf[MAXPATHLEN], fbuf[MAXPATHLEN];
    long 		flg = 0;

    while (*++argv)
	if (*argv[0] != '-')
	    break;
	else
	    switch (argv[0][1]) {
		case 'S' :
		    by_subj = 2;
		when 's' :
		    by_subj = 1;
		when 'A' :
		    by_author = 2;
		when 'a' :
		    by_author = 1;
		when 'f' :
		    force = 1;
		otherwise :
		    return help(0, "save", cmd_help);
	    }
    if (!force && (force = (*argv && !strcmp(*argv, "!"))))
	argv++;
    if ((n = get_msg_list(argv, list)) == -1)
	return -1;
    argv += n;
    if (*argv && *(file = *argv) == '\\')
	file++;
    else if (!file && !by_subj && !by_author) {
	/* if no filename specified, save in ~/mbox */
	if (firstchar == 'w') {
	    /* mbox should have headers. If he really wants it, specify it */
	    print("Must specify file name for 'w'\n");
	    return -1;
	}
	if (!(file = do_set(set_options, "mbox")) || !*file)
	    file = DEF_MBOX;
    }
    n = 1; /* tell getpath to ignore no such file or directory */
    if (file)
	tmp = getpath(file, &n);
    if (n < 0) {
	print("%s: %s\n", file, tmp);
	return -1;
    } else if (n && !by_subj && !by_author) {
	print("%s is a directory\n", file);
	return -1;
    }
    file = strcpy(fbuf, tmp); /* getpath() called again later, save result */
    if (force || Access(file, F_OK))
	mode = "w", force = 0;
    else
	mode = "a";
    if (firstchar != 'w' && *mode == 'a' && !by_author && !by_subj &&
	    !test_folder(file, "not a folder, save anyway?"))
	return 0;
    /*
     * open the file for writing (appending) unless we're saving by subject
     * or author name in which case we'll determine the filename later
     */
    if (!by_author && !by_subj && !(mail_fp = lock_fopen(file, mode))) {
	error("cannot save in \"%s\"", file);
	return -1;
    }

#ifdef SUNTOOL
    if (istool)
	timeout_cursors(TRUE);
#endif /* SUNTOOL */
    if (!chk_option("alwaysignore", "save"))
	turnon(flg, NO_IGNORE);	/* presently overridden by UPDATE_STATUS */
    if (firstchar == 'w') {
	turnon(flg, NO_HEADER);
#ifdef MMDF
	turnon(flg, NO_SEPARATOR);
#endif /* MMDF */
    } else
	turnon(flg, UPDATE_STATUS);

    for (n = msg_number = 0; msg_number < msg_cnt; msg_number++)
	if (msg_bit(list, msg_number)) {
	    if ((by_author || by_subj) && !mail_fp) {
		char buf2[256], addr[256];
		register char *p, *p2;
		if (by_subj) {
		    if (p = header_field(msg_number, "subject")) {
			/* convert spaces and non-alpha-numerics to '_' */
			if (!lcase_strncmp(p, "re:", 3)) {
			    p += 3;
			    skipspaces(0);
			}
			for (p2 = p; *p2; p2++)
			    if (!isalnum(*p2) && !index(".,@#$%-+=", *p2))
				*p2 = '_';
		    } else
			p = "mbox";
		} else {
		    (void) reply_to(msg_number, FALSE, buf2);
		    (void) get_name_n_addr(buf2, NULL, addr);
		    if (p = rindex(addr, '!'))
			p++;
		    else
			p = addr;
		    if (p2 = any(p, "@%"))
			*p2 = 0;
		}
		if (!p || !*p)
		    p = "tmp";
		(void) sprintf(buf, "%s/%s", file, p);
		if (force || Access(buf, F_OK))
		    mode = "w";
		else
		    mode = "a";
		if (firstchar != 'w' && *mode == 'a' &&
			!test_folder(buf, "not a folder, save anyway?")) {
		    if (by_author == 2 || by_subj == 2)
			break;
		    continue;
		}
		if (!(mail_fp = lock_fopen(buf, mode))) {
		    error("cannot save in \"%s\"", buf);
		    if (by_author == 2 || by_subj == 2)
			break;
		    continue;
		}
	    }
	    print("%sing msg %d ... ",
		(firstchar == 's')? "Sav" : "Writ", msg_number+1);
	    print_more("(%d lines)",
		copy_msg(msg_number, mail_fp, (u_long) flg, NULL));
	    if (by_author == 1 || by_subj == 1) {
		print_more(" in \"%s\"", buf);
		(void) close_lock(buf, mail_fp), mail_fp = NULL_FILE;
	    }
	    print_more("\n");
	    n++;
	    if (isoff(msg[msg_number].m_flags, SAVED) && firstchar != 'c') {
		turnon(glob_flags, DO_UPDATE);
		turnon(msg[msg_number].m_flags, SAVED|DO_UPDATE);
	    }
	}
    if (mail_fp) {
	(void) close_lock(file, mail_fp);
	if (!file)
	    file = buf;
	print_more("%s %d msg%s to %s\n",
	    (*mode == 'a')? "Appended" : "Saved", n, (n != 1)? "s": "", file);
    }
#ifdef SUNTOOL
    if (istool) {
	extern Panel_item folder_item, save_item;
	timeout_cursors(FALSE);
	if (firstchar != 'c' && n > 0)
	    (void) do_hdrs(0, DUBL_NULL, NULL);
	if (*mode == 'w' && n > 0) {
#ifndef NO_WALK_MENUS
	    create_folder_menus();
#else /* NO_WALK_MENUS */
	    add_folder_to_menu(folder_item, 3);
	    add_folder_to_menu(save_item, 1);
#endif /* NO_WALK_MENUS */
	}
    }
#endif /* SUNTOOL */
    return 0;
}

respond(n, argv, list)
int n;  /* no use for argc, so use its address space for a variable */
char **argv, *list;
{
    register char *cmd = *argv;
    char list1[MAXMSGS_BITS];
    int cur_msg = current_msg, save_cnt = msg_cnt;

    if (*++argv && !strcmp(*argv, "-?"))
	return help(0, "respond", cmd_help);
    if ((n = get_msg_list(argv, list)) == -1)
	return -1;

    /* make into our own list so ~: commands don't overwrite this list */
    bitput(list, list1, MAXMSGS, =);

    /* back up one arg to replace "cmd" in the new argv[0] */
    argv += (n-1);
    if (!strcmp(cmd, "replyall"))
	Upper(*cmd);
    if (n > 0)
	strdup(argv[0], cmd);

    /* make sure the *current* message is the one being replied to */
    for (current_msg = -1, n = 0; n < msg_cnt && current_msg == -1; n++)
	if (msg_bit(list1, n) && current_msg == -1)
	    current_msg = n;
    if (current_msg == -1) { /* "reply -" can cause this to happen */
	current_msg = cur_msg;
	return -1;
    }
    if (do_mail(1 /* ignored */, argv, list) == -1)
	return -1;
    /* New mail may have arrived during do_mail(), which will change
     * the msg_cnt.  Use the old count when examining the list of bits
     * to set the replied flag, or the wrong messages can be marked.
     */
    for (n = 0; n < save_cnt; n++)
	if (msg_bit(list1, n)) {
	    /* set_isread(n); */
	    set_replied(n); /* only if mail got delivered */
	}
    if (istool)
	(void) do_hdrs(0, DUBL_NULL, NULL);
    /* copy the specified list back into msg_list */
    bitput(list1, list, MAXMSGS, =);
    return 0;
}

/* cd to a particular directory specified by "p" */
cd(x, argv) /* argc, unused -- use space for a non-register variable */
int x;
char **argv;
{
    char *cwd, buf[MAXPATHLEN];
    register char *path, *p = argv[1], *cdpath = NULL, *p2;
    int err = 0;

    if (argv && argv[1] && !strcmp(argv[1], "-?"))
	return help(0, argv[0], cmd_help);

    if (!strcmp(*argv, "pwd")) {
	set_cwd(); /* reset in case some dummy changed $cwd */
        if ((p = do_set(set_options, "cwd")) && *p) {
	    print("%s\n", p);
	    return 0;
	}
	return -1;
    }
    if (!p || !*p) /* if no args, pwd = ".", cd = ~ */
	p = (**argv == 'p')? "." : "~";
    /* if a full path was not specified, loop through cdpath */
    if (**argv != 'p' && *p != '/' && *p != '~' && *p != '+')
	cdpath = do_set(set_options, "cdpath");
    (void) strcpy(buf, p);
    do  {
	err = x = 0;
	path = getpath(buf, &x);
	if (x != 1 || chdir(path) == -1) {
	    err = errno;
	    if (cdpath && *cdpath) {
		char c;
		if (p2 = any(cdpath, " \t:"))
		    c = *p2, *p2 = 0;
		(void) sprintf(buf, "%s/%s", cdpath, p);
		if (cdpath = p2) /* assign and compare to NULL */
		    *p2 = c;
		while (cdpath && (isspace(*cdpath) || *cdpath == ':'))
		    cdpath++;
	    } else
		break;
	}
    } while (err);
    if (err)
	error(p);
    set_cwd();
    if ((istool || iscurses || err) && (cwd = do_set(set_options, "cwd"))) {
	if (err)
	    turnon(glob_flags, CONT_PRNT);
	if (iscurses || istool || ison(glob_flags, WARNING))
	    print("Working dir: %s\n", cwd);
    }
    return 0;
}

mush_quit(argc, argv)
int argc;
char **argv;
{
    u_long updated = ison(glob_flags, DO_UPDATE);

    if (argc > 1) {
	if (!strcmp(argv[1], "-?"))
	    return help(0, "quit", cmd_help);
	else {
	    print("%s: too many arguments\n", argv[0]);
	    return -1;
	}
    }
    if ((!argc || (*argv && **argv == 'q')) && !copyback("Really Quit? ",TRUE))
	return -1;
#ifdef CURSES
    if (iscurses) {
	/* we may already be on the bottom line; some cases won't be */
	move(LINES-1, 0), refresh();
	if (updated)
	    putchar('\n');
    }
#endif /* CURSES */
    cleanup(0);
#ifdef lint
    return 0;
#endif /* lint */
}

delete(argc, argv, list)
int argc;
char **argv, list[];
{
    register int prnt_next, undel = argc && **argv == 'u';
    int old_msg = current_msg;

    prnt_next = (argv && (!strcmp(*argv, "dt") || !strcmp(*argv, "dp")));

    if (argc && *++argv && !strcmp(*argv, "-?"))
	return help(0, "delete", cmd_help);

    if (ison(glob_flags, READ_ONLY)) {
	print("Folder is read-only\n");
	return -1;
    }

    if (get_msg_list(argv, list) == -1)
	return -1;
    for (argc = 0; argc < msg_cnt; argc++)
	if (msg_bit(list, argc))
	    if (undel)
		turnoff(msg[argc].m_flags, DELETE);
	    else
		turnon(msg[argc].m_flags, DELETE|DO_UPDATE);

    /* only if current_msg has been affected && not in curses mode */
    if (prnt_next == 0 && !iscurses && msg_bit(list, current_msg))
	prnt_next = !!do_set(set_options, "autoprint"); /* change to boolean */

    turnon(glob_flags, DO_UPDATE);

    /* goto next available message if current was just deleted.
     * If there are no more messages, turnoff prnt_next.
     */
    if (!iscurses && !undel && msg_bit(list, current_msg))
	(void) next_msg();
    else
	prnt_next = 0;

    if (prnt_next && !undel && !iscurses && isoff(glob_flags, DO_PIPE))
	if (old_msg != current_msg && isoff(msg[current_msg].m_flags, DELETE))
	    display_msg(current_msg, (long)0);
	else {
	    if (ison(msg[current_msg].m_flags, DELETE))
		print("No more messages.\n");
	    current_msg = old_msg;
	}
#ifdef SUNTOOL
    if (istool && isoff(glob_flags, IS_PIPE)) {
	/* If deleted messages are to be shown and the current message
	 * has moved off the screen, or if all messages are deleted,
	 * redraw the whole display.
	 * Otherwise, redraw the display starting at the current
	 * topmost message, to bring the new current message on.
	 */
	if (current_msg != old_msg && do_set(set_options, "show_deleted") ||
		n_array[0] > msg_cnt)
	    (void) do_hdrs(0, DUBL_NULL, NULL);
	else {
	    char *av[3], buf[8];
	    /* do_hdrs(0, ...) repositions the display, so pass an arg */
	    av[0] = "h";
	    av[1] = sprintf(buf, "%d", n_array[0] + 1);
	    av[2] = NULL;
	    (void) do_hdrs(2, av, NULL);
	}
    }
#endif /* SUNTOOL */
    return 0;
}

/*
 * historically from the "from" command in ucb-mail, this just prints
 * the composed header of the messages set in list or in pipe.
 */
do_from(n, argv, list)
int n;
char **argv, list[];
{
    int inc_cur_msg = 0;

    if (argv && *++argv && !strcmp(*argv, "-?"))
	return help(0, "from", cmd_help);
    if (argv && *argv && (!strcmp(*argv, "+") || !strcmp(*argv, "-")))
	if (!strcmp(*argv, "+")) {
	    if (!*++argv && current_msg < msg_cnt-1)
		current_msg++;
	    inc_cur_msg = 1;
	} else if (!strcmp(*argv, "-")) {
	    if (!*++argv && current_msg > 0)
		current_msg--;
	    inc_cur_msg = -1;
	}
    if ((n = get_msg_list(argv, list)) == -1)
	return -1;
    else if (argv && argv[n]) {
	u_long save_flags = glob_flags;
	char *newargv[6], buf[BUFSIZ];
	(void) argv_to_string(buf, &argv[n]);
	newargv[0] = "pick";
	if (n == 0) {
	    newargv[++n] = "-r";
	    newargv[++n] = "*";
	    turnoff(glob_flags, IS_PIPE);
	} else {
	    n = 0;
	    turnon(glob_flags, IS_PIPE);
	}
	newargv[++n] = "-f";
	newargv[++n] = buf;
	newargv[++n] = NULL;
	Debug("calling: "), print_argv(newargv);
	turnon(glob_flags, DO_PIPE);
	(void) do_pick(n, newargv, list);
	glob_flags = save_flags;
    }
    for (n = 0; n < msg_cnt; n++)
	if (msg_bit(list, n)) {
	    wprint("%s\n", compose_hdr(n));
	    /* if -/+ given, set current message pointer to this message */
	    if (inc_cur_msg) {
		current_msg = n;
		/* if - was given, then set to first listed message.
		 * otherwise, + means last listed message -- let it go...
		 */
		if (inc_cur_msg < 0)
		    inc_cur_msg = 0;
	    }
	}
    return 0;
}

static
sorter(cmd1, cmd2)
register struct cmd *cmd1, *cmd2;
{
    return strcmp(cmd1->command, cmd2->command);
}

question_mark(x, argv)
int x;
char **argv;
{
    int n = 0, N = sizeof cmds / sizeof (struct cmd);
    char *Cmds[sizeof cmds/sizeof(struct cmd)], *p, buf[30];

    if (!*++argv) {
	if (N % 5)
	    N = N / 5 + 1;
	else
	    N = N / 5;

	qsort((char *)cmds, sizeof(cmds)/sizeof(struct cmd)-1,
			    sizeof(struct cmd), sorter);

	for (x = 0; x < N * 5; x++) {
	    if (!(x % 5))
		if (!(p = Cmds[n++] = malloc(80))) {
		    error("malloc in question_mark()");
		    free_vec(Cmds);
		    return -1;
		}
	    if (x%5*N+n < sizeof cmds / sizeof (struct cmd))
		p += strlen(sprintf(p, "%-14.14s ", cmds[x%5*N+n-1].command));
	}
	Cmds[n++] = savestr("Type: `command -?' for help with most commands.");
	Cmds[n] = NULL;
	(void) help(0, (char *) Cmds, NULL);
	free_vec(Cmds);
    } else if (!strcmp(*argv, "-?"))
	return help(0, "?", cmd_help);
    else {
	for (x = 0; cmds[x].command; x++)
	    if (!strcmp(*argv, cmds[x].command))
		return cmd_line(sprintf(buf, "\\%s -?", *argv), msg_list);
	print("Unknown command: %s\n", *argv);
    }
    return 0 - in_pipe();
}
