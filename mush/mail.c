/* @(#)mail.c 	(c) copyright 1986 (Dan Heller) */

#include "mush.h"

/*
 * mail.c --
 *    do_mail() 	external interface to these functions; parses options.
 *    mail_someone()    called from do_mail() to begin composing the message.
 *    start_file()      creates the editing file and reset signal catching.
 *    add_to_letter()	adds the next line to letter --determine ~ escapes.
 *    finish_up_letter()  prompts for Cc:, verifies send, adds signatures.
 *    send_it() 	expands aliases, invokes mailer, sends to record file.
 *    add_headers()	adds all headers to a FILE *, reads draft files
 *    rm_edfile()	signals are directed here. remove letter, longjmp
 *    dead_letter()     make a dead.letter if mail failed.
 */

static char Subject[BUFSIZ],To[HDRSIZ],Cc[HDRSIZ],Bcc[HDRSIZ],in_reply_to[256];
static int killme;
static u_long flags;
static SIGRET (*oldterm)(), (*oldint)(), (*oldquit)();
static int finish_up_letter(), send_it(), start_file();
static long add_headers();
static jmp_buf cntrl_c_buf;
static char *Hfile, *edfile;
FILE *ed_fp;
char *hfile;

static int mail_someone();

/* argc and argv could be null if coming from tool mode compose */
do_mail(n, argv, list)
register int n;   /* no need for "argc", so use the space for a variable */
register char **argv, *list;
{
    char firstchar = (argv)? **argv: 'm';
    char *to = NULL, *cc = NULL, *addcc = NULL, *bcc = NULL, *subj = NULL;
    char *route = NULL;
    char inc_list[MAXMSGS_BITS], buf[HDRSIZ];
    u_long flgs = 0;

    if (ison(glob_flags, IS_GETTING)) {
	wprint("You must finish the letter you are editing first.\n");
	return -1;
    }
    if (ison(glob_flags, DO_PIPE)) {
	wprint("You can't pipe out of the %s command.\n", argv? *argv : "mail");
	return -1;
    }
    clear_msg_list(inc_list);
    hfile = Hfile = NULL;

    /* If piped to mail, include the messages piped */
    if (ison(glob_flags, IS_PIPE) ||
	(lower(firstchar) == 'r' && do_set(set_options, "autoinclude"))) {
	turnon(flgs, INCLUDE);
	bitput(list, inc_list, msg_cnt, =);
    }
    /* Set SIGN and DO_FORTUNE now so we can turn them off later */
    if (do_set(set_options, "autosign"))
	turnon(flgs, SIGN);
    if (do_set(set_options, "fortune"))
	turnon(flgs, DO_FORTUNE);
    while (argv && *argv && *++argv && **argv == '-') {
	n = 1;
	while (n && argv[0][n])
	    switch (argv[0][n]) {
#ifdef VERBOSE_ARG
		case 'v': turnon(flgs, VERBOSE); n++; break;
#endif /* VERBOSE_ARG */
		case 'H':
		    if (argv[1]) {
			n = 0;
			Hfile = *++argv;
		    } else {
			wprint("Must specify a file\n");
			return -1;
		    }
		when 'h':
		    if (argv[1]) {
			n = -1; /* it gets incremented below */
			hfile = savestr(*++argv);
			if (ison(glob_flags, REDIRECT)) {
			    turnoff(glob_flags, REDIRECT);
			    turnon(flgs, SEND_NOW);
			}
		    } else {
			wprint("Must specify a file containing headers\n");
			return -1;
		    }
		    /* Fall through */
		case 'E': turnon(flgs, EDIT_HDRS); n++;
		when 'e': turnon(flgs, EDIT); n++;
		when 'F': turnon(flgs, DO_FORTUNE); n++;
		when 'b':
		    if (argv[1]) {
			n = 0, bcc = *++argv;
			fix_up_addr(bcc);
		    } else {
			wprint("Must specify blind-carbon list\n");
			return -1;
		    }
		when 'c':
		    if (argv[1]) {
			n = 0, addcc = *++argv;
			fix_up_addr(addcc);
		    } else {
			wprint("Must specify carbon-copy list\n");
			return -1;
		    }
		when 's':
		    if (argv[1])
			n = 0, subj = *++argv;
		    else
			n++, turnon(flgs, NEW_SUBJECT);
		when 'i': case 'I': case 'f': {
		    int m;
		    if (!msg_cnt) {
			wprint("No message to include!\n");
			return -1;
		    }
		    if (argv[0][n] == 'i') {
			turnon(flgs, INCLUDE);
			turnoff(flgs, INCLUDE_H);
			turnoff(flgs, FORWARD);
		    } else if (argv[0][n] == 'I') {
			turnon(flgs, INCLUDE_H);
			turnoff(flgs, INCLUDE);
			turnoff(flgs, FORWARD);
		    } else if (argv[0][n] == 'f') {
			turnon(flgs, FORWARD);
			turnon(flgs, SEND_NOW);
			turnoff(flgs, INCLUDE_H);
			turnoff(flgs, INCLUDE);
		    }
		    /* "-i 3-5" or "-i3-5"  Consider the latter case first */
		    if (!argv[0][++n])
			argv++, n = 0;
		    (*argv) += n;
		    m = get_msg_list(argv, inc_list);
		    (*argv) -= n;
		    if (m == -1)
			return -1;
		    /* if there were args, then go back to the first char
		     * in the next argv
		     */
		    if (m)
			n = 0;
		    if (!n) /* n may be 0 from above! */
			argv += (m-1);
		}
		when 'U':
		    turnon(flgs, SEND_NOW);
		    n++;
		when 'u':
		    turnoff(flgs, SIGN);
		    turnoff(flgs, DO_FORTUNE);
		    n++;
		when 'r':
		    if (lower(firstchar) == 'r') {
			route = *++argv;
			n = 0;
			break;
		    }
		    /* fall thru */
		default:
		    if (argv[0][n] != '?') {
			wprint("%c: unknown option\n\n", argv[0][n]);
			return -1;
		    } else
			return help(0, "mail", cmd_help);
	    }
    }
    if (isoff(flgs, FORWARD)) {
	if (ison(flgs, SEND_NOW)) {
	    if (!hfile && !Hfile) {
		wprint("Can't send immediately without draft file.\n");
		return -1;
	    }
	    turnoff(flgs, EDIT); /* -U overrides -e */
	} else if (do_set(set_options, "autoedit"))
	    turnon(flgs, EDIT);
    } else if (ison(flgs, EDIT)) /* -e modifies -f */
	turnoff(flgs, SEND_NOW);
#ifdef VERBOSE_ARG
    if (do_set(set_options, "verbose"))
	turnon(flgs, VERBOSE);
#endif /* VERBOSE_ARG */
    *in_reply_to = *To = *Subject = *Cc = *Bcc = 0;
    if (lower(firstchar) == 'r') {
	char *in_reply_fmt, *pcc = NULL;
	to = To, cc = Cc;
	/*
	 * Generate a reply to all the messages passed to respond().  This
	 * list is different than the include-msg list above.  Get info about
	 * whom the messages were sent to for reply-all.
	 * BUG: currently, redundant addresses aren't pruned from Bcc list!
	 */
	for (n = 0; n < msg_cnt; n++)
	    if (msg_bit(list, n)) {
		if (to != To)
		    *to++ = ',', *to++ = ' ';
		(void) reply_to(n, (firstchar == 'R'), buf);
		if (strlen(buf) + (to - To) > sizeof(To) - 1) {
		    wprint("# recipients exceeded at msg %d\n", n);
		    break;
		}
		to += Strcpy(to, buf);
		if (firstchar == 'R') {
		    if (pcc = cc_to(n, buf)) {
			/* if there was a previous cc, append ", " */
			if (cc != Cc)
			    *cc++ = ',', *cc++ = ' ';
			if (strlen(pcc) + (cc - Cc) > sizeof(Cc) - 1)
			    wprint("# Cc's exceeded at msg %d\n", n);
			else
			    cc += Strcpy(cc, pcc);
		    }
		}
		/* remove redundant addresses now, or headers could get too
		 * long before the list runs out (it still might)
		 */
		rm_redundant_addrs(To, Cc);
		to = To + strlen(To);
		cc = Cc + strlen(Cc);
	    }
	/* clean up end of Cc line for replyall's */
	while (*cc == ' ' || *cc == ',')
	    *cc-- = '\0';
	if (firstchar == 'R' && !do_set(set_options, "metoo")) {
	    /* Each reply_to() call above will leave at least
	     * one person in To.  If that one person was us,
	     * we need to get removed from the complete list.
	     */
	    (void) take_me_off(to);
	}
	to = To, cc = Cc;
	if (route || (route = do_set(set_options, "auto_route")))
	    /* careful! This routine could add lots-o-bytes and lose addresses
	     * to avoid writing out of segment.
	     */
	    route_addresses(To, Cc, route);
	if (in_reply_fmt = do_set(set_options, "in_reply_to"))
	    /* "9" here is a magic # --see compose_hdr() */
	    (void) strcpy(in_reply_to,
			    format_hdr(current_msg, in_reply_fmt, FALSE)+9);
    }
    if (ison(flgs, FORWARD) && ison(flgs, EDIT) ||
	    lower(firstchar) == 'r' && isoff(flgs, NEW_SUBJECT)) {
	turnoff(flgs, NEW_SUBJECT);
	if (subj && *subj && (isoff(flgs, FORWARD) || ison(flgs, EDIT)))
	    subj = strcpy(Subject, subj);
	else if (subj = subject_to(current_msg, buf))
	    subj = strcpy(Subject, buf + 4*(lower(firstchar) != 'r'));
    } else if (isoff(flgs, NEW_SUBJECT) && isoff(flgs, FORWARD) &&
	(do_set(set_options, "ask") || do_set(set_options, "asksub")))
	turnon(flgs, NEW_SUBJECT);
    if (argv && *argv) {
	char buf[HDRSIZ];
	(void) argv_to_string(buf, argv);
	fix_up_addr(buf);
	to = &To[strlen(To)];
	if (*To)
	    *to++ = ',', *to++ = ' ';
	(void) strcpy(to, buf);
	to = To;
    }
    if (addcc && *addcc) {
	cc = &Cc[strlen(Cc)];
	if (*Cc)
	    *cc++ = ',', *cc++ = ' ';
	(void) strcpy(cc, addcc); /* addcc has already been fixed up */
	cc = Cc;
    }
    /* remove any redundant addresses that just got added */
    rm_redundant_addrs(To, Cc);
    if (bcc && *bcc)
	(void) strncpy(Bcc, bcc, sizeof(Bcc)); /* bcc already fixed up */
    bcc = Bcc;

    return mail_someone(to, subj, cc, bcc, flgs, inc_list);
}

static
mail_someone(to, subject, cc, bcc, flgs, list)
register char *to, *subject, *cc, *bcc, *list;
u_long flgs;
{
    register char *p;

    flags = flgs;
    if (to && *to) {
	if (!*To)
	    (void) strncpy(To, to, sizeof(To));
    } else
	to = NO_STRING;
    if (subject && *subject) {
	if (!*Subject)
	    (void) strncpy(Subject, subject, sizeof(Subject));
    } else
	subject = NO_STRING;
    if (cc && *cc) {
	if (!*Cc)
	    (void) strncpy(Cc, cc, sizeof(Cc));
    } else
	Cc[0] = '\0';
    if (bcc && *bcc) {
	if (!*Bcc)
	    (void) strncpy(Bcc, bcc, sizeof(Bcc));
    } else
	Bcc[0] = '\0';

    /* Initialize MIME support variables */
    mime_prepare();

    if (ison(glob_flags, REDIRECT)) {
	/*
	 * NOTE: Could change this to finish_up_letter() to allow
	 * signatures to be appended.  The -U! option to mush would
	 * be extended to suppress signing when redirection is on.
	 */
	int sent = send_it();
	if (sent == -1) {
	    wprint("Message not sent!\n");
	    rm_edfile(-1);
	}
	return sent;
    }
    /* if (!*to) then prompting will be done */
    if (!istool && !hfile) {
	if (p = set_header("To: ", to, !*to)) {
	    if (!*to) /* if user typed To-line here, fix up the addresses */
		fix_up_addr(p);
	    (void) strcpy(To, p);
	}
	if (!*To && ison(flags, FORWARD) && ison(flags, SEND_NOW)) {
	    turnoff(flags, SEND_NOW); /* user must edit To: line or do again */
	    print("(You must add a To: address.)\n");
	}
	/* don't prompt for subject if forwarding mail */
	if (isoff(flags, FORWARD) && (*subject || ison(flags, NEW_SUBJECT)) &&
		(p = set_header("Subject: ", subject,
			!*subject && ison(flags, NEW_SUBJECT))))
	    (void) strcpy(Subject, p);
	if (*Cc || ison(flags, EDIT_HDRS) && do_set(set_options, "askcc")) {
	    if ((p = set_header("Cc: ", cc, !*Cc)) && *p) {
		fix_up_addr(p);
		(void) strcpy(Cc, p);
	    }
	}
	if (*Bcc)
	    print("Bcc: %s\n", Bcc);
	putchar('\n');
    }

    /* If forwarding w/o editing, start a new file for each. */
    if (ison(flags, FORWARD) && ison(flags, SEND_NOW)) {
	char fwd[MAXMSGS_BITS];
	register int i;
	clear_msg_list(fwd);
	for (i = 0; i < msg_cnt; i++)
	    if (msg_bit(list, i)) {
		set_msg_bit(fwd, i);
		if (start_file(fwd) < 0)
		    return -1;
		turnon(msg[i].m_flags, FORWARD);
		if (isoff(glob_flags, READ_ONLY))
		    turnon(glob_flags, DO_UPDATE);
		clear_msg_list(fwd);
	    }
    } else
	return start_file(list);
    return 0;
}

static
start_file(list)
char *list;
{
    register char  *dir;
    register int   i;
    char           line[MAXPATHLEN];
    int		   had_hfile = FALSE;

    /* getdir() uses the home directory if no tmpdir */
    if (!(dir = getdir(do_set(set_options, "tmpdir"))))
alted:
	dir = ALTERNATE_HOME;
    (void) mkstemp(sprintf(line, "%s/%s", dir, EDFILE));
    strdup(edfile, line);
    if (!(ed_fp = mask_fopen(edfile, "w+"))) {
	if (strcmp(dir, ALTERNATE_HOME))
	    goto alted;
	error("can't create %s", edfile);
	return -1;
    }
    if (!istool) {
	oldint = signal(SIGINT, rm_edfile);
	oldquit = signal(SIGQUIT, rm_edfile);
	oldterm = signal(SIGTERM, rm_edfile);
    }

    if (istool && isoff(flags, SEND_NOW) ||
	    (isoff(flags, FORWARD) || isoff(flags, SEND_NOW)) &&
	    (ison(flags, EDIT_HDRS) || do_set(set_options, "edit_hdrs"))) {
	turnon(flags, EDIT_HDRS);
	if (hfile)
	    had_hfile = TRUE;
	if (add_headers(NULL_FILE, &ed_fp, 1, flags) == (long) -1)
	    return -1;
    }
    if (Hfile) {
	(void) file_to_fp(Hfile, ed_fp, "r");
	Hfile = NULL;
	had_hfile = TRUE;
    }
    if (istool && isoff(flags, SEND_NOW))
	strdup(hfile, edfile);

    /* if flags call for it, include current message (with header?) */
    if (ison(flags, INCLUDE|FORWARD|INCLUDE_H)) {
	long copy_flgs = 0, is_forw = ison(flags, FORWARD);
	char buf[sizeof(To)];
	if (!is_forw) {
	    turnon(copy_flgs, INDENT);
	    if (ison(flags, INCLUDE_H) &&
		    !chk_option("alwaysignore", "include"))
		turnon(copy_flgs, NO_IGNORE);
	    else if (ison(flags, INCLUDE))
		turnon(copy_flgs, NO_HEADER);
	} else if (ison(flags, SEND_NOW) ||
		!chk_option("alwaysignore", "forward"))
	    turnon(copy_flgs, FORWARD);	/* FORWARD implies NO_IGNORE */
#ifdef MSG_SEPARATOR
	turnon(copy_flgs, NO_SEPARATOR);
#endif /* MSG_SEPARATOR */
	for (i = 0; i < msg_cnt; i++)
	    if (msg_bit(list, i)) {
		if (is_forw && isoff(flags, SEND_NOW)) {
		    (void) reply_to(i, FALSE, buf);
		    (void) fprintf(ed_fp,"--- Forwarded mail from %s\n\n",buf);
		}
		wprint("%sing message %d ...",
		    is_forw? "forward" : "includ", i+1);
		wprint("(%d lines)\n",
		    copy_msg(i, ed_fp, (u_long) copy_flgs, NULL));
		set_isread(i); /* if we included it, we read it, right? */
		if (is_forw && isoff(flags, SEND_NOW))
		    (void) fprintf(ed_fp,
			"\n--- End of forwarded message from %s\n", buf);
		if (!is_forw || isoff(flags, SEND_NOW))
		    (void) fputc('\n', ed_fp);
	    }
	(void) fflush(ed_fp);
    }
    if (!istool && ison(glob_flags, WARNING)) {
	if (escape && strncmp(escape, DEF_ESCAPE, 1))
	    print("(escape character is set to `%c')\n", *escape);
	if (wrapcolumn && wrapcolumn < 20)
	    print("(warning: wrapping only %d columns from the left!)\n",
		    wrapcolumn);
    }

    /* do an "if" again in case editor not found and EDIT turned off */
    if (!istool && ison(flags, EDIT)) {
	char **argv, *edit;
	int argc;
	if ((!(edit = do_set(set_options, "visual")) || !*edit) &&
		(!(edit = do_set(set_options, "editor")) || !*edit))
	    edit = DEF_EDITOR;
	(void) sprintf(line, "%s %s", edit, edfile);
	if ((argv = mk_argv(line, &argc, FALSE)) && argc > 0) {
	    print("Starting \"%s\"...\n", argv[0]);
	    (void) fclose(ed_fp);
	    ed_fp = NULL_FILE;
	    execute(argv);
	    free_vec(argv);
	    turnoff(flags, EDIT);
	    turnoff(flags, FORWARD); /* forwarded messages must be unedited */
	    /* upon exit of editor, user must now type eofc or "." to send */
	    if (!(ed_fp = fopen(edfile, "r+"))) {
		error("can't reopen %s", edfile);
		return -1;
	    }
	    (void) fseek(ed_fp, 0L, 2);
	} else
	    print("Unable to start \"%s\"\n", edit);
	wprint("(continue editing letter or ^%c to send)\n", eofc + '@');
    } else if (ison(flags, SEND_NOW)) {
	/* if finish_up_letter() was successful, file was successfully sent. */
	if (!setjmp(cntrl_c_buf) && finish_up_letter() == 0) {
	    rm_edfile(0);
	    return 0;
	}
    } else if (had_hfile) {
	/* it's not obvious what's going on -- enlighten user */
	wprint("(continue editing or ^%c to send)\n", eofc + '@');
    }

#ifdef SUNTOOL
    if (istool) {
	/* toolmode doesn't care if SEND_NOW -- user continues to edit file.
	 * if SEND_NOW is not set, then the editor file has just been started,
	 * so again, just return so user can edit file.
	 */
	if (ed_fp)
	    fclose(ed_fp), ed_fp = NULL_FILE;
	turnon(glob_flags, IS_GETTING);
	return 0;
    }
#endif /* SUNTOOL */
    if (ison(flags, SEND_NOW)) {
	/* editing couldn't have been on -- finish_up_letter() failed */
	rm_edfile(0 - ison(flags, FORWARD));
	return -1;
    }

    i = 0;
    turnon(glob_flags, IS_GETTING);
    do  {
	/* If the user hits ^C in cbreak mode, mush will return to
	 * Getstr and not clear the buffer. whatever is typed next will
	 * be appended to the line.  jumping here will force the line to
	 * be cleared cuz it's a new call.
	 */
	(void) setjmp(cntrl_c_buf);
	while (Getstr(line, sizeof(line), 0) > -1) {
	    if (!istool) /* toolmode checks on a timer -- don't do it here */
		(void) check_new_mail(); /* if new mail comes in, get it */
	    if ((i = add_to_letter(line)) <= 0)
		break;
	}
    } while (i >= 0 && finish_up_letter() == -1);
    turnoff(glob_flags, IS_GETTING);
    return i; /* return -1 if ~x or ~q to terminate letter */
}

char *tilde_commands[] = {
    "commands: [OPTIONAL argument]",
    "t [list]\tChange list of recipients",
    "s [subject]\tModify [set] subject header",
    "c [cc list]\tModify [set] carbon copy recipients",
    "b [bcc list]\tModify [set] blind carbon recipients",
    "h\t\tModify all message headers",
    "m\t\tSet MIME headers for message body",           /* Coven */
    "*\t\tAdd an attachment",                           /* Coven */
    "e [editor]\tEnter editor. Editor used: \"set editor\", env EDITOR, vi",
    "v [editor]\tEnter visual editor. \"set visual\", env VISUAL, vi",
    "u\t\tEdit previous (last) line in file.",
    "p [pager]\tPage message; pager used: \"set pager\", env. PAGER, more",
    "i [msg#'s]\tInclude current msg body [msg#'s] indented by \"indent_str\"",
    "I [msg#'s]\tSame, but include the message headers from included messages",
    "f [msg#'s]\tForward mail. Not indented, but marked as \"forwarded mail\"",
    "S[!]\t\tInclude Signature file [suppress file]",
    "F[!]\t\tAdd a fortune at end of letter [don't add]",
    "w file\t\tWrite msg buffer to file name",
    "a file\t\tAppend msg buffer to file name",
    "r file\t\tRead filename into message buffer",
    "q \t\tQuit message; save in dead.letter (unless \"nosave\" is set).",
    "x \t\tQuit message; don't save in dead.letter.",
    "$variable\tInsert the string value for \"variable\" into message.",
    ":cmd\t\tRun the mail command \"cmd\".",
    "|cmd\t\tPipe the message through the unix command \"cmd\".",
    "E[!]\t\tClear contents of letter after saving to dead.letter [unless !].",
    0
};

/*
 * TC_EDIT(tc) returns TRUE if tilde_command[tc] involves message
 * editing operations.  Used when EDIT_HDRS is active.
 */
#define TC_EDIT(tc) ((tc) && ((tc) < 8 || !tilde_commands[(tc)+1]))

/*
 * Add the line (char *) parameter to the letter.  Determine tilde
 * escapes and determine what to do.  This function returns 0 to
 * indicate user wants to end the letter, -1 if the letter cannot
 * be sent (~q, ~x no buffer after editor, etc...) or 1 to indicate
 * successful addition of the line to the letter.
 * This function may be called by toolmode just to change certain mush
 * internal variables via tilde escapes.  Thus, ed_fp might be null.
 */
add_to_letter(line)
char line[];
{
    register char *p;
    char buf[HDRSIZ > MAXPATHLEN ? HDRSIZ : MAXPATHLEN];

    killme = 0;
    if (ed_fp) /* may be null if istool */
	(void) fseek(ed_fp, 0L, 2);

    if (!strcmp(line, ".") && do_set(set_options, "dot"))
	return 0;
    if (line[0] != *escape || ison(glob_flags, QUOTE_MACRO)) {
	(void) fputs(line, ed_fp);
	(void) fputc('\n', ed_fp);
	(void) fflush(ed_fp);
	return 1;
    }
    /* all commands are "~c" (where 'c' is the command). set p = first
     * character after 'c' and skip whitespace
     */
    p = &line[2];
    skipspaces(0);
    switch (line[1]) {
	case 'v' : case 'p': case 'e' : case '|' : {
	    if (!*p || *p == 'i')
		switch (line[1]) {
		    case 'p' :
			if (!*p && !(p = do_set(set_options, "pager")))
			    p = DEF_PAGER;
			if (!*p || !strcmp(p, "internal"))
			    p = NULL;
		    when 'v' :
			if (*p && p[1] || (p = do_set(set_options, "visual")))
			    break;
			/* else fall through */
		    default :
			if (!(p = do_set(set_options, "editor")) || !*p)
			    p = DEF_EDITOR;
		    when '|' :
			print("No command for pipe\n");
			return 1;
		}
	    if (line[1] == 'p' || line[1] == '|')
		rewind(ed_fp);
	    if (line[1] == 'p') {
		(void) do_pager(p, TRUE); /* start the pager "p" */
		if (isoff(flags, EDIT_HDRS)) {
		    (void) do_pager(sprintf(buf, "To: %s\n", To), FALSE);
		    if (Subject[0])
			(void) do_pager(sprintf(buf, "Subject: %s\n", Subject),
					FALSE);
		    if (Cc[0])
			(void) do_pager(sprintf(buf, "Cc: %s\n", Cc), FALSE);
		    if (Bcc[0])
			(void) do_pager(sprintf(buf, "Bcc: %s\n", Bcc), FALSE);
		    (void) do_pager(strcpy(buf,
					    "--------\nMessage contains:\n"),
			FALSE);
		}
		while (fgets(buf, sizeof(buf), ed_fp))
		    if (do_pager(buf, FALSE) == EOF)
			break;
		(void) do_pager(NULL, FALSE); /* end pager */
	    } else if (line[1] == '|') {
		FILE *pipe_fp;
		(void) sprintf(buf, "( %s ) > %s", p, edfile);
		if (unlink(edfile) < 0) {
		    error("Can't unlink %s:", edfile);
		    break; /* Drop out of switch */
		}
		if ((pipe_fp = popen(buf, "w")) == NULL_FILE) {
		    error("Can't run \"%s\":", p);
		    (void) file_to_fp(edfile, ed_fp, "w");
		} else {
		    while (fgets(buf, sizeof(buf), ed_fp))
			if (fputs(buf, pipe_fp) == EOF) {
			    print("Broken pipe\n");
			    break;
			}
		    (void) pclose(pipe_fp);
		}
		pipe_fp = ed_fp; /* save ed_fp until we can reopen it */
		if (!(ed_fp = fopen(edfile, "r+"))) {
		    error("can't reopen %s", edfile);
		    (void) rewind(pipe_fp);
		    if (file_to_fp(edfile, pipe_fp, "w") < 0 ||
			    !(ed_fp = fopen(edfile, "r+"))) {
			error("can't restore old contents of %s", edfile);
			ed_fp = pipe_fp;
			dead_letter(0);
			return -1;
		    }
		}
		(void) fclose(pipe_fp);
	    } else {
		char **argv;
		int argc;
		(void) sprintf(buf, "%s %s", p, edfile);
		if ((argv = mk_argv(buf, &argc, FALSE)) && argc > 0) {
		    (void) fclose(ed_fp);
		    ed_fp = NULL_FILE;
		    execute(argv);
		    free_vec(argv);
		    /* tool will return even tho editor isn't done */
		    if (!(ed_fp = fopen(edfile, "r+"))) {
			error("can't reopen %s", edfile);
			return -1;
		    }
		} else
		    print("Unable to start \"%s\"\n", p);
	    }
	}
	when '$': {
	    register char *p2;
	    if (!(p2 = do_set(set_options, p)))
		print("(%s isn't set)\n", p);
	    else
		putstring(p2, ed_fp);
	}
	when ':': {
	    char new[MAXMSGS_BITS];
	    u_long save_flags = glob_flags;

	    turnon(glob_flags, IGN_SIGS);
	    turnoff(glob_flags, DO_PIPE);
	    turnoff(glob_flags, IS_PIPE);
	    (void) cmd_line(p, new);
	    glob_flags = save_flags;
	}
	when 'i': case 'f': case 'I': {
	    int  n;
	    u_long copy_flgs = 0;
	    char list[MAXMSGS_BITS];

	    if (!msg_cnt) {
		wprint("No messages.\n");
		break;
	    }
	    clear_msg_list(list);
	    if (line[1] != 'f') {
		turnon(copy_flgs, INDENT);
		if (line[1] == 'i')
		    turnon(copy_flgs, NO_HEADER);
		else if (!chk_option("alwaysignore", "include"))
		    turnon(copy_flgs, NO_IGNORE);
	     } else if (!chk_option("alwaysignore", "forward"))
		turnon(copy_flgs, NO_IGNORE);
#ifdef MSG_SEPARATOR
	    turnon(copy_flgs, NO_SEPARATOR);
#endif /* MSG_SEPARATOR */
	    if (!*p)
		set_msg_bit(list, current_msg);
	    else if (!do_range(p, list))
		return 1;
	    for (n = 0; n < msg_cnt; n++)
		if (msg_bit(list, n)) {
		    if (line[1] == 'f') {
			(void) reply_to(n, FALSE, buf);
			(void) fprintf(ed_fp,
				    "--- Forwarded mail from %s\n\n", buf);
		    }
		    wprint("Including message %d ... ", n+1);
		    wprint("(%d lines)\n", copy_msg(n, ed_fp, copy_flgs, NULL));
		    set_isread(n);
		    if (line[1] == 'f')
			(void) fprintf(ed_fp,
			    "\n--- End of forwarded message from %s\n\n", buf);
		    else
			(void) fputc('\n', ed_fp);
		}
	}
	/* To: Cc: and Bcc: headers */
	when 'b':
	case 't':
	case 'c': {
	    char *h = (line[1] == 't')? To : (line[1] == 'c')? Cc : Bcc;
	    char *Prompt = line[1] == 't'? "To: " :
			   line[1] == 'c'? "Cc: " : "Bcc: ";
	    if (ison(flags, EDIT_HDRS)) {
		print("You must use an editor to change your headers.\n");
		break;
	    }

	    if (*p) {
		fix_up_addr(p);
		if (*h)
		    (void) sprintf(h+strlen(h), ", %s", p);
		else
		    (void) strcpy(h, p);
	    } else if (!(p = set_header(Prompt, h, TRUE)) || !*p)
		*h = 0;
	    else {
		fix_up_addr(p);
		(void) strcpy(h, p);
	    }
	}
	when 's':
	    if (ison(flags, EDIT_HDRS)) {
		print("You must use an editor to change your headers.\n");
		break;
	    }
	    if (*p || (p = set_header("Subject: ", Subject, 1)))
		if (!*p)
		    Subject[0] = 0;
		else
		    (void) strcpy(Subject, p);
	when 'h':
	    if (ison(flags, EDIT_HDRS)) {
		print("You must use an editor to change your headers.\n");
		break;
	    }
	    while ((p = set_header("To: ", To, 1)) && !*p)
		print("(There must be a recipient.)\n");
	    (void) strcpy(To, p);
	    if (p = set_header("Subject: ", Subject, 1))
		if (!*p)
		    Subject[0] = 0;
		else
		    (void) strcpy(Subject, p);
	    if (p = set_header("Cc: ", Cc, 1))
		if (!*p)
		    Cc[0] = 0;
		else {
		    fix_up_addr(p);
		    (void) strcpy(Cc, p);
		}
	    if (p = set_header("Bcc: ", Bcc, 1))
		if (!*p)
		    Bcc[0] = 0;
		else {
		    fix_up_addr(p);
		    (void) strcpy(Bcc, p);
		}
        when 'm':
            ask_mime_part(0);
        when '*':
            ask_mime_part(1);
	when 'F': case 'S' : {
	    if (*p == '!')
		turnoff(flags, line[1] == 'F'? DO_FORTUNE : SIGN);
	    else
		turnon(flags, line[1] == 'F'? DO_FORTUNE : SIGN);
	    wprint("%sadding %s at end of message.\n", *p == '!'? "not " : "",
		line[1] == 'F'? "fortune" : "signature");
	}
	when 'w': case 'a': case 'r':
	    if (!*p) {
		print("(you must specify a filename)\n");
		return 1;
	    }
	    (void) fseek(ed_fp, 0L, 2); /* append */
	    (void) file_to_fp(p, ed_fp, (line[1] == 'r')? "r":
			      (line[1] == 'w')? "w": "a");
	/* go up one line in the message file and allow the user to edit it */
	when 'u': {
	    long newpos, pos = ftell(ed_fp);
	    char oldline[256];
	    if (pos <= 0L) { /* pos could be -1 if ftell() failed */
		print("(No previous line in file.)\n");
		return 1;
	    }
	    /* get the last 256 bytes written and read backwards from the
	     * current place until '\n' is found. Start by moving past the
	     * first \n which is at the end of the line we want to edit
	     */
	    newpos = max(0, pos - 256L);
	    (void) fseek(ed_fp, newpos, L_SET);
	    /* don't fgets -- it'll stop at a \n */
	    (void) fread(line, sizeof(char), (int)(pos-newpos), ed_fp);
	    pos--;
	    /* the last char in line should be a \n cuz it was last input */
	    if (line[(int)(pos-newpos)] != '\n')
		print("I don't know how, but your last line ended with %c.\n",
		    line[(int)(pos-newpos)]);
	    else
		line[(int)(pos-newpos)] = 0; /* null terminate \n for ^H-ing */
	    for (pos--; pos > newpos && line[(int)(pos-newpos)] != '\n'; pos--)
		;
	    /* we've gone back to the end of the second previous line. Check
	     * to see if the char we're pointing to is a \n.  It should be, but
	     * if it's not, we moved back to the first line of the file.
	     */
	    if (line[(int)(pos-newpos)] == '\n')
		++pos;
	    /* save the old line that's there in case the user boo-boos */
	    (void) strcpy(oldline, &line[(int)(pos-newpos)]);
	    /* let set header print out the line and get the input */
	    if (!(p = set_header("", &line[(int)(pos-newpos)], TRUE))) {
		print("Something bad happened and I don't know what it is.\n");
		p = oldline;
	    } else if (*p == *escape)
		print("(Warning: %c escapes ignored on %cu lines.)\n",
				*escape, *escape);
	    /* seek to to the position where the new line will go */
	    (void) fseek(ed_fp, pos, L_SET);
	    /* put the newly typed line */
	    (void) fputs(p, ed_fp); /* don't add \n. padding may be necessary */
	    /* if the new line is less than the old line, we're going to do
	     * one of two things.  The best thing to do is to truncate the
	     * file to the end of the new line.  Sys-v can't do that, so we
	     * pad the line with blanks.  May be messy in some cases, but...
	     */
	    if ((pos = strlen(p) - strlen(oldline)) < 0) {
#ifndef SYSV
		/* add the \n, flush the file, truncate to the current pos */
		(void) fputc('\n', ed_fp);
		(void) fflush(ed_fp);
		(void) ftruncate(fileno(ed_fp), (off_t) ftell(ed_fp));
#else /* SYSV */
		/* pad with blanks to the length of the old line. add \n */
		while (pos++ < 0)
		    (void) fputc(' ', ed_fp);
		(void) fputc('\n', ed_fp), (void) fflush(ed_fp);
#endif /* SYSV */
	    } else {
		/* the new line is >= the old line, add \n -- no trunc req. */
	        (void) fputc('\n', ed_fp);
		(void) fflush(ed_fp);
	    }
	    return 1;
	 }
	/* break;  not here cuz of "return" (lint). */
	case 'E':
	    if (ison(flags, EDIT_HDRS)) {
		print("You must use an editor to empty the message buffer.\n");
		break;
	    }
	    if (*p != '!' && !do_set(set_options, "nosave"))
		dead_letter(0);
	    if (emptyfile(&ed_fp, edfile) == -1) {
		error(edfile);
		return -1;
	    } else
		print("Message buffer empty\n");
	when 'q':
	    /* save in dead.letter if nosave not set -- rm_edfile(-2). */
	    rm_edfile(-2); /* doesn't return out of tool mode */
	    return -1;
	    /* break; not stated cuz of "return" (lint) */
	case 'x':
	    /* don't save dead.letter -- simulate normal rm_edfile() call */
	    rm_edfile(0);
	    return -1;
	    /* break; (not specified for lint) */
	default:
	    if (line[1] == *escape) {
		(void) fputs(&line[1], ed_fp);
		(void) fputc('\n', ed_fp);
		(void) fflush(ed_fp);
		return 1;
	    } else if (line[1] == '?') {
		register int x;
		(void) do_pager(NULL, TRUE); /* start pager */
		for (x = 0; tilde_commands[x]; x++) {
		    if (ison(flags, EDIT_HDRS) && TC_EDIT(x))
			continue;
		    (void) sprintf(buf, "%s%s\n", escape, tilde_commands[x]);
		    if (do_pager(buf, FALSE) == EOF)
			break;
		}
		if (tilde_commands[x] == NULL) {
		    (void) sprintf(buf,
			"%s%s\t\tBegin a line with a single %s\n",
			escape, escape, escape);
		    (void) do_pager(buf, FALSE);
		}
		(void) do_pager(NULL, FALSE); /* end pager */
	    } else
		print("`%c': unknown %c escape. Use %c? for help.\n",
		    line[1], *escape, *escape);
	    return 1;
    }
    if (ed_fp)
	(void) fseek(ed_fp, 0L, 2);
    if (!istool)
	wprint("(continue editing letter)\n");
    return 1;
}

/*
 * finish up the letter. ask for the cc line, if verify is set, ask to
 * verify sending, continue editing, or to dump the whole idea.
 * Then check for signature and fortune.  Finally, pass it to send_it()
 * to actually send it off.
 * Return 0 on success, -1 on failure.
 */
static int
finish_up_letter()
{
    register char *p;
    int c;
    char buf[MAXPATHLEN];

    /* forwarded mail has no additional personalized text */
    if (ison(flags, FORWARD)) {
	if (send_it() == -1) {
	    wprint("Message not sent!\n");
	    return -1;
	}
	turnoff(glob_flags, IS_GETTING);
	return 0;
    }

    /* REDIRECT should never be on here, but just in case */
    if (isoff(glob_flags, REDIRECT)) {
	if (!istool) {
	    if (isoff(flags, EDIT_HDRS) && do_set(set_options, "askcc")) {
		if (p = set_header("Cc: ", Cc, 1))
		    (void) strcpy(Cc, p);
	    }
	}
	/* ~v on the Cc line asks for verification, first initialize p! */
	p = NULL;
	if (!strncmp(Cc, "~v", 2) ||
		/* Flashy test for $verify either empty or set to "mail" */
		glob(p = do_set(set_options, "verify"),
					    "{,{,*[ \\,]}mail{,[ \\,]*}}")) {
	    if (!p) /* so we don't Cc to ~v! */
		*Cc = 0;
	    for (;;) {
#ifdef SUNTOOL
		if (istool)
		    c = (ask("Send Message?") == TRUE)? 's' : 'c';
		else
#endif /* SUNTOOL */
		{
		    print("send, continue editing, discard [s,c,d]? ");
		    c = Getstr(buf, sizeof(buf), 0);
		    if (c < 0) { /* This is probably not the best idea... */
			rm_edfile(-2);
			return 0;
		    }
		    else if (!istool)
			c = lower(*buf);
		}
		if (c == 'd') {
		    rm_edfile(-2);
		    return 0;
		} else if (c == 'c') {
		    wprint("(continue editing letter)\n");
		    return -1;
		} else if (c == 's')
		    break;
	    }
	}
    }

    if (send_it() == -1) {
	if (isoff(flags, SEND_NOW))
	    wprint("(continue)\n");
	return -1;
    }
    return 0;
}

/*
 * actually send the letter.
 * 1. reset all the signals because of fork.
 * 2. determine recipients (users, address, files, programs)
 * 3. determine mailer, fork and return (if not verbose).
 * 4. popen mailer, $record, and other files specified in step 1.
 * 5. make the headers; this includes To: line, and user set headers, etc...
 * 6. copy the letter right into the array of file pointers (step 1).
 * 7. close the mailer and other files (step 1) and remove the edit-file.
 * return -1 if mail wasn't sent.  could be user error, could be the system.
 * allow user to try again or to save message to file and abort message.
 * return 0 if successful.
 */
static int
send_it()
{
    static char mail_delivery[sizeof(MAIL_DELIVERY)
#ifdef DELIVER_ARG
	+ sizeof(DELIVER_ARG) + 1
#endif
	+ 1] = { 0 };

    register char *p, *b, *addr_list;
#ifdef MAXFILES
    register int size = MAXFILES - 1;
    FILE *files[MAXFILES];
    char *names[MAXFILES];
#else
    register int size = getdtablesize() - 1;
    FILE *files[30];  /* 30 should be sufficiently large enough */
    char *names[30];
#endif /* MAXFILES */
#if defined(VERBOSE_ARG)
    SIGRET (*oldchld)();
#endif /* VERBOSE_ARG */
    int next_file = 1; /* reserve files[0] for the mail delivery program */
    int log_file = -1; /* the index into the files array for mail logging */
    char buf[3*HDRSIZ];
    char *orig_to, *orig_cc, *orig_bcc; /* save originals to restore on error */
    char expand = !do_set(set_options, "no_expand");
    int fork_pid;
    FILE *mime_fp;    /* additional file for MIME support */

    if (!*mail_delivery) {
	strcpy(mail_delivery, MAIL_DELIVERY);
#ifdef DELIVER_ARG
	strcat(strcat(mail_delivery, " "), DELIVER_ARG);
#endif
    }

    names[0] = names[1] = NULL; /* for free_vec() */
    /* If edit_hdrs, make sure the correct headers exist and are intact
     * before bothering to continue.
     */
    if (ison(flags, EDIT_HDRS)) {
	/* fool header_field into thinking that the file is the folder */
	FILE *save_tmpf = tmpf;
	long old_offset = msg[msg_cnt].m_offset;

	if (!ed_fp) {
	    wprint("No file for headers!\n");
	    return -1;
	}
	Debug("Getting headers from file ... ");

	tmpf = ed_fp;
	msg[msg_cnt].m_offset = 0L;
	if (p = header_field(msg_cnt, "to")) {
	    (void) strcpy(To, p);
	    Cc[0] = Bcc[0] = 0;
	    if (p = header_field(msg_cnt, "cc"))
		(void) strcpy(Cc, p);
	    if (p = header_field(msg_cnt, "bcc"))
		(void) strcpy(Bcc, p);
	    if (p = header_field(msg_cnt, "fcc"))
		next_file += find_files(p, names+next_file, size-next_file, 1);
	} else
	    *To = 0; /* Error caught below */
	msg[msg_cnt].m_offset = old_offset;
	tmpf = save_tmpf;
	Debug("\n");
    }
    if (!*To) {
	wprint("You must have a To: recipient to send mail.\n");
	if (!istool) {
	    (void) signal(SIGINT, oldint);
	    (void) signal(SIGQUIT, oldquit);
	    (void) signal(SIGTERM, oldterm);
	}
	free_vec(&names[1]);
	return -1;
    }

    if (!(p = do_set(set_options, "sendmail")))
	p = mail_delivery;

#ifdef VERBOSE_ARG
    /* Tool mode can't do verbosity -- no window for the MTA output */
    if (!istool && (ison(flags, VERBOSE) || do_set(set_options, "verbose"))) {
	turnon(flags, VERBOSE); /* prevent fork when "verbose" has changed */
	oldchld = signal(SIGCHLD, SIG_DFL); /* let pclose() do the wait() */
#if defined(MMDF) && !defined(M_EXECMAIL)
	b = &buf[strlen(sprintf(buf, "%s%s", p, VERBOSE_ARG))];
#else /* MMDF */
	b = &buf[strlen(sprintf(buf, "%s %s", p, VERBOSE_ARG))];
#endif /* MMDF && !M_EXECMAIL */
    } else
#endif /* VERBOSE_ARG */
	b = buf + Strcpy(buf, p);
#ifdef METOO_ARG
    if (!strcmp(p, mail_delivery) && do_set(set_options, "metoo"))
	b += strlen(sprintf(b, " %s", METOO_ARG));
#endif /* METOO_ARG */
    *b++ = ' ', *b = 0; /* strcat(b, " "); */
    addr_list = b; /* save this position to check for addresses later */

    /* save original list.  If alias expansion fails, replace address lists
     * with what was originally typed so user can fix it.  This isn't necessary
     * if the lists are already in the file the user is editing (edit_hdrs).
     */
    if (isoff(flags, EDIT_HDRS))
	orig_to = savestr(To);
    /*
     * Build the address lines to give to the mail transfer system.  This
     * address line cannot contain comment fields!  First, expand aliases
     * since they may contain comment fields within addresses. Copy this
     * result back into the Buffer since this will go into the header ...
     * Next, remove all comments so the buffer contains ONLY valid addresses.
     * Next, strip off any filenames/programs which might occur in the list.
     * Finally, add this information to the command line buffer (buf).
     * Remove commas if necessary (see ifdefs).  In the event of errors,
     * force a dead letter by rm_edfile(-1).
     */
    if (!(p = alias_to_address(To))) {
	print("address expansion failed for To: list.\n");
	free_vec(&names[1]);
	if (isoff(flags, EDIT_HDRS))
	    strcpy(To, orig_to), xfree(orig_to);
	return -1;
    } else {
	next_file += find_files(p, names+next_file, size-next_file, 0);
	if (expand)
	    (void) strcpy(To, p);
	rm_cmts_in_addr(p);
	skipspaces(0);
	b += Strcpy(b, p);
    }
    if (isoff(flags, EDIT_HDRS))
	orig_cc = savestr(Cc);
    if (*Cc) {
	if (!(p = alias_to_address(Cc))) {
	    wprint("address expansion failed for Cc: list.\n");
	    free_vec(&names[1]);
	    if (isoff(flags, EDIT_HDRS)) {
		strcpy(To, orig_to), xfree(orig_to);
		strcpy(Cc, orig_cc), xfree(orig_cc);
	    }
	    return -1;
	} else {
	    next_file += find_files(p, names+next_file, size-next_file, 0);
	    if (expand)
		(void) strcpy(Cc, p);
	    rm_cmts_in_addr(p);
	    skipspaces(0);
	    if (*p) {
		*b++ = ',', *b++ = ' ';
		b += Strcpy(b, p);
	    }
	}
    }

    /* expand Bcc addrs, but don't add to list yet.  sign letter first */
    if (isoff(flags, EDIT_HDRS))
	orig_bcc = savestr(Bcc);
    if (*Bcc) {
	if (p = alias_to_address(Bcc))
	    p = strcpy(Bcc, p);
	else {
	    wprint("address expansion failed for Bcc: list.\n");
	    free_vec(&names[1]);
	    /* rm_edfile(-1); */
	    if (isoff(flags, EDIT_HDRS)) {
		strcpy(To, orig_to), xfree(orig_to);
		strcpy(Cc, orig_cc), xfree(orig_cc);
		strcpy(Bcc, orig_bcc), xfree(orig_bcc);
	    }
	    return -1;
	}
    } else
	p = NULL;

    /* Sign the letter before adding the Bcc list since they aren't
     * considered when adding a signature.
     */
    if (*addr_list && ison(flags, SIGN|DO_FORTUNE) &&
	    isoff(glob_flags, REDIRECT) && isoff(flags, FORWARD))
	sign_letter(addr_list, flags, ed_fp);

    if (p) { /* p still points to expanded Bcc list */
	next_file += find_files(p, names+next_file, size-next_file, 0);
	rm_cmts_in_addr(p);
	skipspaces(0);
	if (*p) {
	    *b++ = ',', *b++ = ' ';
	    b += Strcpy(b, p);
	}
    }
    if (!*addr_list && next_file == 1) {
	wprint("There must be at least 1 legal recipient.\n");
	if (isoff(flags, EDIT_HDRS)) {
	    strcpy(To, orig_to), xfree(orig_to);
	    strcpy(Cc, orig_cc), xfree(orig_cc);
	    strcpy(Bcc, orig_bcc), xfree(orig_bcc);
	}
	return -1;
    }

#ifdef NO_COMMAS
    for (p = buf; p = index(p, ','); p++)
	*p = ' ';
#endif /* NO_COMMAS */

    Debug("mail command: %s\n", buf);

    if (isoff(flags, VERBOSE) && debug < 3)
	switch (fork_pid = fork()) {
	    case  0:  /* the child will send the letter. ignore signals */
#if defined(SYSV) && !defined(AUX) && !defined(IRIX4)
		if (setpgrp() == -1)
#else /* !SYSV || AUX || IRIX4 */
		if (setpgrp(0, getpid()) == -1)
#endif /* SYSV && !AUX || IRIX4 */
		    error("setpgrp");
		/* NOTE: No special case needed for tool here because
		 * this is the sending child -- it's going to pclose()
		 * and then exit(), so who cares about the notifier?
		 */
		(void) signal(SIGCHLD, SIG_DFL);
		(void) signal(SIGTERM, SIG_IGN);
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
#ifdef SIGTTIN
		(void) signal(SIGTTOU, SIG_IGN);
		(void) signal(SIGTTIN, SIG_IGN);
#endif /* SIGTTIN */
#ifdef SIGCONT
		(void) signal(SIGCONT, SIG_IGN);
		(void) signal(SIGTSTP, SIG_IGN);
#endif /* SIGCONT */
		turnon(glob_flags, IGN_SIGS);
	    when -1:
		error("fork failed trying to send mail");
		if (isoff(flags, EDIT_HDRS)) {
		    strcpy(To, orig_to);
		    strcpy(Cc, orig_cc);
		    strcpy(Bcc, orig_bcc);
		}
		/* fall thru */
	    default:
		if (fork_pid > 0) {
#ifdef SUNTOOL
		    /* If we're a tool, we have to register a handler
		     * for the fork_pid.  Otherwise, it's used only as
		     * an error indicator, so reset it to zero.
		     */
		    if (istool)
			notify_set_wait3_func(mfprint_sw, my_wait3, fork_pid);
#endif /* SUNTOOL */
		    fork_pid = 0;
		}
		/* istool doesn't need ed_fp, so don't keep it around */
		if (istool || !fork_pid && isoff(glob_flags, REDIRECT))
		    (void) fclose(ed_fp), ed_fp = NULL_FILE;
		free_vec(&names[1]);
		if (isoff(flags, EDIT_HDRS))
		    xfree(orig_to), xfree(orig_cc), xfree(orig_bcc);
		if (!istool) {
		    (void) signal(SIGINT, oldint);
		    (void) signal(SIGQUIT, oldquit);
		    (void) signal(SIGTERM, oldterm);
		}
		return fork_pid;
	}

#if defined(MMDF) && !defined(M_EXECMAIL)
    *(addr_list-1) = '\0';
#endif /* MMDF && !M_EXECMAIL */
    if (debug > 2) {
	files[0] = stdout;
	if (!*addr_list)
	    addr_list = "[no recipients]";
    } else if (*addr_list) {
	if (!(files[0] = open_file(buf, TRUE, FALSE))) {
	    rm_edfile(-1); /* force saving of undeliverable mail */
	    if (isoff(flags, VERBOSE) && debug < 3)
		exit(-1);
	    else
		return 0;
	}
    } else
	files[0] = NULL_FILE;

    if (ison(flags, VERBOSE))
	wprint("Sending letter ... "), (void) fflush(stdout);
#if defined(MMDF) && !defined(M_EXECMAIL)
    /* give address list to submit */
    for (p = addr_list; *p && (p = any(p, ",<")); p++)
	if (*p == ',')
	    *p = '\n';
	else
	    p = index(p, '>');
    if (*addr_list)
	(void) fprintf(files[0], "%s\n\n", addr_list);
#endif /* MMDF && !M_EXECMAIL */

    /* see if log is set.  This is just to add message headers. No msg body. */
    if (p = do_set(set_options, "logfile")) {
	if (!*p)
	    p = "~/mail.log";
	if (!index("~|/+", *p))
	    (void) sprintf(buf, "%s/%s", do_set(set_options, "cwd"), p);
	else
	    (void) strcpy(buf, p);
	log_file = next_file;
	next_file += find_files(buf, names+next_file, size-next_file, 0);
	if (log_file == next_file)
	    log_file = -1;
    }

    /* see if record is set.  If so, open that file for appending and add
     * the letter in a format such that mail can be read from it
     */
    if (p = do_set(set_options, "record")) {
	if (!*p)
	    p = "~/record";
	if (!index("~|/+", *p))
	    (void) sprintf(buf, "%s/%s", do_set(set_options, "cwd"), p);
	else
	    (void) strcpy(buf, p);
	next_file += find_files(buf, names+next_file, size-next_file, 0);
    }

    /* Don't need to open names[0] as files[0], so skip those */
    next_file = 1 + open_list(names + 1, files + 1, next_file - 1);

    /* First, put the message separator in... */
    for (size = 1; size < next_file; size++)
#ifndef MSG_SEPARATOR
	{
	    time_t t;
	    (void) time(&t);
	    (void) fprintf(files[size], "From %s %s", login, ctime(&t));
	}
#else /* MSG_SEPARATOR */
#ifdef MMDF
	(void) fputs(MSG_SEPARATOR, files[size]);
#else /* MMDF */
	(void) fprintf(files[size], "%s\n", MSG_SEPARATOR);
#endif /* MMDF */
#endif /* MSG_SEPARATOR */

    /* if redirection, ed_fp = stdin, else rewind the file just made */
    if (isoff(glob_flags, REDIRECT))
	rewind(ed_fp);
    else
	ed_fp = stdin;

#ifndef MSG_SEPARATOR
    /* If forwarding or reading a draft, skip the leading From_ line.
     * This is done for drafts so that messages saved by dead_letter()
     * can be read back in as a draft; in other cases, this isn't done
     * for edit_hdrs because FORWARD wouldn't be set.
     */
    if (ison(flags, FORWARD|SEND_NOW) && fgets(buf, sizeof buf, ed_fp) &&
	strncmp(buf, "From ", 5) != 0)
	rewind(ed_fp); /* No From_ line (should never happen) */
#endif /* MSG_SEPARATOR */
    {
	long offset = add_headers(ed_fp, files, next_file, flags);
	if (offset == -1)
	    offset = 0L;
	(void) fseek(ed_fp, offset, L_SET);
    }

    /* Convert letter to MIME */
    mime_fp=enmime(ed_fp);

    /* Read from stdin or the edfile till EOF and send it all to the mailer
     * and other open files/folders/programs. Check for "From " at the
     * beginnings of these lines to prevent creating new messages in folders.
     */
    while (fgets(buf, sizeof buf, mime_fp))
	for (size = 0; size < next_file; size++) {
	    if (!files[size]) /* files[0] will be NULL if not calling MTA */
		continue;
	    if (size == log_file)
		continue;
#ifndef MSG_SEPARATOR
	    if (!strncmp(buf, "From ", 5))
		(void) fputc('>', files[size]);
#endif /* MSG_SEPARATOR */
	    if (fputs(buf, files[size]) == EOF) {
		if (size == 0) {
		    error("Lost connection to MTA");
		    dead_letter(-1);
		    break;
		} else {
		    /* Drop this file, but continue writing others */
		    if (names[size]) {
			error("Write failed: %s", names[size]);
			(void) close_lock(names[size], files[size]);
			xfree(names[size]);
		    } else
			error("Write failed");
		    if (size < --next_file) {
			names[size] = names[next_file];
			files[size--] = files[next_file];
		    }
		    files[next_file] = NULL_FILE;
		    names[next_file] = NULL;
		}
	    }
	}

    /* If mime_fp differs from ed_fp enmime created additional file.
       Remove it, as well as other temporary files and buffers.       */
    if(mime_fp!=ed_fp)
        mime_clear();

    /* loop thru the open files (except for the first: the mail delivery agent)
     * and append a blank line so that ucb-mail can read these folders.
     * Then close the files.
     */
    for (size = 1; size < next_file; size++) {
#ifdef END_MSG_SEP
	(void) fputs(END_MSG_SEP, files[size]);
#endif /* END_MSG_SEP */
	if (names[size]) {
#ifndef END_MSG_SEP
	    (void) fputc('\n', files[size]);
#endif /* !END_MSG_SEP */
	    if (close_lock(names[size], files[size]) == EOF) {
		error("Warning: Close failed: %s", names[size]);
	    }
	    xfree(names[size]);
	} else {
	    if (debug < 3)
		(void) fclose(files[size]); /* Don't mess with pclose() */
	    else
		(void) pclose(files[size]); /* unless we never forked */
	}
    }

    if (debug < 3) {
	int reply_code = files[0]? pclose(files[0]) : (MTA_EXIT << 8);
	Debug("pclose reply_code = %d\n", reply_code);
	rm_edfile((reply_code == (MTA_EXIT << 8))? 0 : -1);
    } else
	rm_edfile(0);

#ifdef VERBOSE_ARG
    if (!istool && ison(flags, VERBOSE))
	(void) signal(SIGCHLD, oldchld);
#endif /* VERBOSE_ARG */

    if (ison(flags, VERBOSE) || debug > 2) {
	if (isoff(glob_flags, REDIRECT))
	    wprint("sent.\n");
	if (!istool) {
	    (void) signal(SIGINT, oldint);
	    (void) signal(SIGQUIT, oldquit);
	    (void) signal(SIGTERM, oldterm);
	}
    } else
	exit(0); /* not a user exit -- a child exit */
    return 0;
}

/*
 * Add the necessary headers to make a file a legitimate mail message.
 * This could be for a file which the user will edit (via edit_hdrs) or
 * for delivery to an MTA.
 * Make folders conform to RFC-822 by adding From: and Date: headers.
 * Prefix certain header with the "Resent-" prefix when forwarding.
 * Return offset of fp if we're parsing it for headers (for delivery to MTA).
 */
static long
add_headers(fp, files, size, flags)
FILE *fp, *files[];
int size;
u_long flags;
{
    char buf[BUFSIZ], From_buf[256], *pF = From_buf, date_str[64];
    char *host = NULL, *p, *subj = NULL, *own_from = NULL; /* See WARNING */
    int i, for_editor = (fp == NULL_FILE);
    int got_date = for_editor, got_from = for_editor;
    struct options *opts;

    if (for_editor && hfile) {
	i = file_to_fp(hfile, files[0], "r");
	xfree(hfile), hfile = NULL;
	return (i < 0 ? -1 : TRUE);
    }

    buf[0] = 0;
    if (ourname)
	host = ourname[0];
    if (for_editor)
	turnoff(flags,FORWARD); /* forwarded messages must not be edited */

    /* [Re]create a From: header -- check first to see if the user has
     * created a From: header with the my_hdr command (the own_hdrs list).
     * If his is not legitimate, warn user and use the other header.
     */
    if ((for_editor || isoff(flags, EDIT_HDRS)) &&
	    own_hdrs && !do_set(set_options, "no_hdrs")) {
	for (opts = own_hdrs; opts; opts = opts->next)
	    if (!strcmp(opts->option, "From:")) {
		p = opts->value;
		skipspaces(0);
		sprintf(buf, "%sFrom: %s\n",
				ison(flags, FORWARD)? "Resent-" : "", p);
		own_from = buf;
		/* WARNING: the above depends on the following facts:
		 * 1. If for_editor, own_from will be output immediately,
		 *    so buf will not be overwritten;
		 * 2. If !for_editor but EDIT_HDRS, the "real" from line
		 *    will be read from the file so own_from isn't needed;
		 * 3. If neither, From: is the first line output, so
		 *    buf will not be overwritten.
		 * Any change in the above means a new buffer for own_from
		 * may be needed.  Check carefully.
		 */
	    }
	}
    if (ison(flags, FORWARD))
	pF += Strcpy(From_buf, "Resent-");
    pF += Strcpy(pF, "From: ");
#ifdef UUCP
    if (host && *host)
	pF += strlen(sprintf(pF, "%s!", host));
#endif /* UUCP */
    pF += Strcpy(pF, login);
#ifndef UUCP
    if (host && *host)
	pF += strlen(sprintf(pF, "@%s", host));
#endif /* UUCP */
    if ((p = do_set(set_options, "realname")) ||
	(p = do_set(set_options, "name")))
	pF += strlen(sprintf(pF, " (%s)", p));
    *pF++ = '\n', *pF++ = 0;

    /* First print From, Date, In-Reply-To */
    for (i = 0; i < size; i++) {
	if (!files[i])
	    continue;
	if (for_editor)
	    if (own_from)
		(void) fputs(own_from, files[i]);
	    else
		(void) fputs(From_buf, files[i]);
	else if (isoff(flags, EDIT_HDRS)) {
#ifdef PICKY_MAILER
	    if (i > 0)
#endif /* PICKY_MAILER */
	    if (own_from)
		(void) fputs(own_from, files[i]);
	    else
		(void) fputs(From_buf, files[i]);
	    got_from = TRUE;
	}
	if (for_editor || isoff(flags, EDIT_HDRS)) {
#ifdef PICKY_MAILER
	    if (i > 0 && !for_editor)
#endif /* PICKY_MAILER */
	    (void) fprintf(files[i], "%sDate: %s\n",
		ison(flags, FORWARD) ? "Resent-" : "", rfc_date(date_str));
	    got_date = TRUE;
	    if (*in_reply_to)
		fprintf(files[i], "In-Reply-To: %s\n", in_reply_to);
	}
    }
    if (own_from)
	*own_from = 0; /* buf[0] must be 0 below */
    /* next print user's own message headers */
    if (for_editor || isoff(flags, EDIT_HDRS))
	if (own_hdrs && !do_set(set_options, "no_hdrs")) {
	    for (opts = own_hdrs; opts; opts = opts->next) {
		if (!strcmp(opts->option, "From:"))
		    continue;
		for (i = 0; i < size; i++) {
		    if (!files[i])
			continue;
		    p = opts->value;
		    skipspaces(0);
		    fprintf(files[i], "%s %s\n", opts->option, p);
		}
	    }
	}

    /*
     * Now either prepare to put the rest of the headers into the file
     * or (when sending edited headers) copy them back out of the file
     */
    if (for_editor) {
	char *orig = NULL;
	/* for edit_hdrs, print the headers followed by a blank line */
	if (To[0]) {
	    orig = savestr(To);
	    if (!(p = alias_to_address(To))) {
		wprint("To: list unmodified.\n");
		p = orig;
	    }
	    (void) strcpy(To, p);
	}
	if (Cc[0]) {
	    strdup(orig, Cc);
	    if (!(p = alias_to_address(Cc))) {
		wprint("Cc: list unmodified.\n");
		p = orig;
	    }
	    (void) strcpy(Cc, p);
	}
	if (Bcc[0]) {
	    strdup(orig, Bcc);
	    if (!(p = alias_to_address(Bcc))) {
		wprint("Bcc: list unmodified.\n");
		p = orig;
	    }
	    (void) strcpy(Bcc, p);
	}
	xfree(orig);
    } else if (ison(flags, EDIT_HDRS)) {
	/* copy the headers of the message removing special headers */
	int print_hdr = FALSE;
	if (isoff(flags, SEND_NOW))
	    rewind(fp); /* Drafts may have had fp positioned */
	while (fgets(buf, sizeof(buf), fp)) {
	    (void) no_newln(buf);
	    if (!buf[0])
		break;
	    /* if the first char is NOT a space, it MUST be a new header.
	     * Otherwise, it is considered part of the message body.
	     */
	    if (!isspace(buf[0])) {
		print_hdr = TRUE;
		if (!(p = any(buf, " \t:")) || isspace(*p))
		    break; /* this is not a legitimate header */
		skipspaces(1);
		if (!*p)
		    print_hdr = FALSE; /* blank headers are not allowed */
		p = buf;
		if (!lcase_strncmp(buf, "resent-", 7)) {
		    if (ison(flags, EDIT_HDRS))
	    wprint("You can't use \"Resent-\" headers in edited messages.\n");
		    p += 7;
		}
		if (!lcase_strncmp(p, "to:", 3) ||
		    !lcase_strncmp(p, "cc:", 3) ||
		    !lcase_strncmp(p, "bcc:", 4) ||
		    !lcase_strncmp(p, "fcc:", 4) ||
		    !lcase_strncmp(p, "x-mailer:", 9) ||
		    !lcase_strncmp(p, "status:", 7))
		    print_hdr = FALSE;
		else if (!lcase_strncmp(p, "date:", 5))
		    if (got_date)
			wprint("You can't change or add date headers.\n");
		    else {
			got_date = TRUE;
			(void) sprintf(buf, "Date: %s", rfc_date(date_str));
			p = buf;
		    }
		else if (!lcase_strncmp(p, "subject:", 8))
		    (print_hdr = FALSE), strdup(subj, p);
		else if (!lcase_strncmp(p, "from:", 5)) {
		    char not_me[BUFSIZ];
		    (void) strcpy(not_me, buf + 5);
		    take_me_off(not_me);
		    if (*not_me) {
			/* Ignore bogus From: if we have a good one */
			if (got_from)
			    print_hdr = FALSE;
			/* otherwise, output a good one */
			else {
			    (void) strcpy(buf, From_buf);
			    (void) no_newln(buf);
			}
		    }
		    got_from = TRUE;
#ifdef PICKY_MAILER
		    /* don't send From: to mta -- fool "for loop" below
		     * by initializing the loop at files[1], not files[0]
		     */
		    if (!for_editor)
			print_hdr = 2;
#endif /* PICKY_MAILER */
		}
	    }
	    if (print_hdr)
		/* print_hdr may be 2 for From: header */
		for (i = print_hdr-1; i < size; i++)
		    if (files[i]) {
			(void) fputs(buf, files[i]);
			(void) fputc('\n', files[i]);
		    }
	}
    }
    /* Finally, do the required (or changed) headers (Date, To, Cc) */
    (void) wrap_addrs(To, 80);
    (void) wrap_addrs(Cc, 80);
    (void) wrap_addrs(Bcc, 80);
    for (i = 0; i < size; i++) {
	if (!files[i])
	    continue;
#ifdef PICKY_MAILER
	if (i > 0) {
#endif /* PICKY_MAILER */
	if (!got_from)
	    (void) fputs(From_buf, files[i]);
	if (!got_date)
	    (void) fprintf(files[i], "%sDate: %s\n",
		ison(flags, FORWARD) ? "Resent-" : "", rfc_date(date_str));
#ifdef PICKY_MAILER
	}
#endif /* PICKY_MAILER */
	(void) fprintf(files[i], "X-Mailer: %s\n", check_internal("version"));
	(void) fprintf(files[i], "%sTo: %s\n",
	    ison(flags, FORWARD) ? "Resent-" : "", To);
	if (for_editor || isoff(flags, EDIT_HDRS)) {
	    if (isoff(flags, FORWARD) &&
		    (*Subject || for_editor && (do_set(set_options, "ask") ||
					    do_set(set_options, "asksub"))))
		(void) fprintf(files[i], "Subject: %s\n", Subject);
	} else if (subj && *subj && strlen(subj) > 9)
	    (void) (fputs(subj, files[i]), fputc('\n', files[i]));
	if (*Cc || for_editor && do_set(set_options, "askcc"))
	    (void) fprintf(files[i], "%sCc: %s\n",
		ison(flags, FORWARD) ? "Resent-" : "", Cc);
	if (i > 0 || for_editor)
	    /* Do not send these to mail transfer agent */
	    if (*Bcc)
		(void) fprintf(files[i], "%sBcc: %s\n",
		    ison(flags, FORWARD) ? "Resent-" : "", Bcc);
	if (i > 0)
	    (void) fprintf(files[i], "Status: OR\n");
    }
    for (i = 0; i < size; i++)
	if (files[i])
	    (void) fflush(files[i]);
    if (buf[0]) /* last attempted header read was a line of msg text */
	for (i = 0; i < size; i++) {
	    if (files[i]) {
		(void) fputs(buf, files[i]);
		if(!mime_used)                       /* additional headers */
                    (void) fputc('\n', files[i]);    /* will be added... */
		(void) fflush(files[i]);
	    }
	}
    else
	if (isoff(flags, FORWARD))
	    for (i = 0; i < size; i++)
		if (files[i]) {
		    if(!mime_used)                     /* additional headers */
                        (void) fputc('\n', files[i]);  /* will be added... */
		    (void) fflush(files[i]);
		}
    return fp? ftell(fp) : (long)TRUE;
}

/* ARGSUSED */
SIGRET
rm_edfile(sig)
{
    if (sig > 0) {
	char *fix;
#ifdef linux
	m_getchar();
#endif
	if (ison(glob_flags, IGN_SIGS))
	    return;
	/* wrapcolumn may have been trashed -- restore it */
	if ((fix = do_set(set_options, "wrapcolumn")) && *fix)
	    wrapcolumn = atoi(fix);
	mac_flush(); /* abort pending macros */
    }
    /* now check whether we should abort the letter */
    if (sig > 0 && !killme && ison(glob_flags, IS_GETTING)) {
	if (!istool)
	    (void) signal(sig, rm_edfile);
	killme = 1;
	print("\n** interrupt -- one more to kill letter **\n");
	longjmp(cntrl_c_buf, 1);
    }
    killme = 0;
    /* if sig == -1, force a save into dead.letter.
     * else, check for nosave not being set and save anyway if it's not set
     * sig == 0 indicates normal exit (or ~x), so don't save a dead letter.
     */
    if (sig == -1 || sig != 0 && !do_set(set_options, "nosave"))
	dead_letter(sig);
    if (isoff(glob_flags, REDIRECT) && ed_fp) /* ed_fp may be null in toolmode*/
	(void) fclose(ed_fp), ed_fp = NULL_FILE;
    (void) unlink(edfile);

    turnoff(glob_flags, IS_GETTING);
    if (sig == -1)
	return;

    if (sig == SIGHUP)
	cleanup(0);
    if (!istool) {
	(void) signal(SIGINT, oldint);
	(void) signal(SIGQUIT, oldquit);
	(void) signal(SIGTERM, oldterm);
    }

    if (sig == 0 || sig == -2 || istool) /* make sure sigchld is reset first */
	return;

    if (isoff(glob_flags, DO_SHELL)) {  /* If we're not in a shell, exit */
#ifndef SILENT_EXIT
	puts("exiting");
#endif
	echo_on();
	exit(1);
    }
    longjmp(jmpbuf, 1);
}

/* save letter into dead letter */
dead_letter(sig)
int sig;	/* signal passed to rm_edfile() or 0 */
{
    char 	*p, buf[BUFSIZ];
    long 	t;
    FILE 	*dead;

    if (ison(glob_flags, REDIRECT)) {
	print("Input redirected -- can't save dead letter.\n");
	return;
    }
    /* If the file doesn't exist, get outta here. File may not exist if
     * user generated a ^C from a promptable header and catch sent us here.
     */
    if (!ed_fp && Access(edfile, R_OK) != 0)
	return;
    /* User may have killed mush via a signal while he was in an editor.
     * ed_fp will be NULL in this case.  Since the file does exist (above),
     * open it so we can copy it to dead letter.
     */
    if (!ed_fp && !(ed_fp = fopen(edfile, "r"))) {
	error("can't save dead letter from %s", edfile);
	return;
    }
    /* don't save a dead letter if there's nothing to save. */
    if (fseek(ed_fp, 0L, 2) || ftell(ed_fp) <= 1L)
	return;
    if (!(p = do_set(set_options, "dead")))
	p = "~/dead.letter";
    if (!(dead = open_file(p + (*p == '|'), (*p == '|'), TRUE)))
	return;
    (void) time (&t);
    (void) fflush(ed_fp);
    rewind(ed_fp);
#ifdef MSG_SEPARATOR
	(void) fputs(MSG_SEPARATOR, dead);
#ifndef MMDF
    (void) fputc('\n', dead);
#endif /* MMDF */
#else /* MSG_SEPARATOR */
    (void) fprintf(dead, "From %s %s", login, ctime(&t));
#endif /* MSG_SEPARATOR */
    (void) fprintf(dead, "From: %s\nTo: %s\nSubject: %s\n", login, To, Subject);
    (void) fprintf(dead, "Date: %s\n", rfc_date(buf));
    if (*Cc)
	(void) fprintf(dead, "Cc: %s\n", Cc);
    if (*Bcc)
	(void) fprintf(dead, "Bcc: %s\n", Bcc);
    (void) fputc('\n', dead);
    while (fgets(buf, sizeof(buf), ed_fp))
	(void) fputs(buf, dead);
    (void) fputc('\n', dead);
#ifdef END_MSG_SEP
    (void) fputs(END_MSG_SEP, dead);
#endif /* END_MSG_SEP */
    if (*p != '|')
	(void) close_lock(p, dead);
    else
	(void) pclose(dead);
    wprint("Saved%s letter in %s\n", sig > 0? " unfinished" : "", p);
}
