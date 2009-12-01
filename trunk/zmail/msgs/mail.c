/* mail.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * Combined with addrs.c and compose.c, mail.c forms the core of Z-Mail's
 * message composition functionality.  The files are organized as follows: 
 * 
 *	addrs.c         Manipulation and parsing of E-mail addresses 
 *	compose.c       Internals of the HeaderField and Compose structures
 *	mail.c          Control of initiating, editing, and sending   
 * 
 * The external interfaces to these three files are declared in:
 * 
 *	zmcomp.h     Declarations of data structures and functions
 */

#ifndef lint
static char	mail_rcsid[] = "$Id: mail.c,v 2.205 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "addrs.h"
#include "catalog.h"
#include "child.h"
#ifdef VUI
#include "dialog.h" /* to circumvent "unreasonable include nesting" */
#endif /* VUI */
#include "dirserv.h"
#include "config/features.h"
#include "fetch.h"
#include "fsfix.h"
#include "hooks.h"
#include "i18n.h"
#include "mail.h"
#include "maxfiles.h"
#include "strcase.h"
#include "zmcomp.h"

#include <general.h>
#ifndef MAC_OS
#include <c3/dyn_c3.h>
#else /* MAC_OS */
#include <dyn_c3.h>
#endif /* MAC_OS */

#ifdef ZMCOT
# include <spoor/view.h>
#endif /* ZMCOT */

#ifdef VUI
# include <spoor/im.h>
# include <zmlite/zmlite.h>
#endif /* VUI */

#ifdef PARTIAL_SEND
#include "partial.h"
#endif /* PARTIAL_SEND */

#define DEFAULT_TEMP_NAME "default"

static int killme;
static RETSIGTYPE (*oldterm)(), (*oldint)(), (*oldquit)();
static int send_it(), start_file();
static jmp_buf cntrl_c_buf;
#ifdef QUEUE_MAIL
static int open_queue_file P((int *, char **, FILE **));
#endif /* QUEUE_MAIL */
static void add_mref P ((Compose *, int, enum mref_type));
static mimeCharSet saved_charset;

#ifdef QUEUE_MAIL
# ifdef UUCP_SUPPORT
#  define ShouldQueueMail() (debug > 2 || (!boolean_val(VarConnected) && !ZmUsingUUCP()))
# else /* !UUCP_SUPPORT */
#  define ShouldQueueMail() (debug > 2 || !boolean_val(VarConnected))
# endif /* UUCP_SUPPORT */
#else /* !QUEUE_MAIL */
# define ShouldQueueMail() 0
#endif /* QUEUE_MAIL */

#ifndef UNIX
# ifdef UUCP_SUPPORT
#include "uucp.h"
#endif /* UUCP_SUPPORT */
FILE *do_start_sendmail P((Compose *, char *));
int do_sendmail P((Compose *));
#endif /* !UNIX */

/*
 * Reset signal handling for compositions.
 *
 * Note that this function does not deal with IS_GETTING.
 *
 * The compose_mode signal handling is in general a bit dangerous.
 * If we are in on_intr() for any reason, the stack of handlers that it
 * maintains may be left in a bad state by some other signal; notably,
 * SIGINT, SIGQUIT and SIGTERM may cause rm_edfile() to longjmp().
 *
 * XXX  Should those signals really be handled that way??
 *
 * NOTE: This function MUST NOT toggle IGN_SIGS!  Calling routines may
 * require local control of that flag, and are responsible for setting
 * it to make the signal handling transition safe.
 */
void
compose_mode(on)
int on;
{
    static int safe = 0;

    if (!istool) {
	if (on) {
	    oldterm = signal(SIGTERM, rm_edfile);
	    oldint = signal(SIGINT, rm_edfile);
	    oldquit = signal(SIGQUIT, rm_edfile);
	    killme = 1; /* remains 1 until ctrl_c_buf set */
	    safe = 1;
	} else if (safe) {
	    (void) signal(SIGTERM, oldterm);
	    (void) signal(SIGINT, oldint);
	    (void) signal(SIGQUIT, oldquit);
	    killme = 0;
	    safe = 0;
	}
    }
}

static
char *mail_flags[][2] = {
    { "-attach_msg",	"-M" },	/* Must precede -attach */
    { "-attach",	"-A" },
    { "-bcc",		"-b" },
    { "-blindcarbon",	"-b" },	/* Must precede -blind */
    { "-blind",		"-b" },
    { "-carbon",	"-c" },
    { "-cc",		"-c" },
    { "-comment",	"-C" },
    { "-copy",		"-c" },
    /* -d, -D */
    { "-editor",	"-e" },
    { "-edit_hdrs",	"-E" },
    { "-edit",		"-e" },
    { "-draft",		"-h" },
    { "-file",		"-H" },
    { "-fortune",	"-F" },
    { "-forward",	"-f" },	/* Must preced -forw */
    { "-forw",		"-f" },
    { "-fwd_attach",	"-M" },
    /* -g, -G */
    { "-headeredit",	"-E" },
    { "-headerfile",	"-h" },
    { "-help",		"-?" },
    { "-include_hdrs",	"-I" },
    { "-include",	"-i" },
    { "-inc_hdrs",	"-I" },
    { "-inc",		"-i" },
    /* -j, -J */
    /* -k, -K */
    { "-log",		"-L" },
    { "-msg_attach",	"-M" },
    { "-mta",		"-T" },
    /* -n, -N */
    { "-nosign",	"-u" },
    /* -o, -O */
    /* -P */
    { "-plate",		"-p" },
    /* -q, -Q */
    { "-receipt",	"-R" },
    { "-record",	"-l" },
    { "-remarks",	"-C" },
    { "-remark",	"-C" },
    { "-return-receipt","-R" },
    { "-resend",	"-Uf"},
    /* -S */
    { "-send!",		"-Uu"},
    { "-send",		"-U" },
    { "-subject",	"-s" },
    { "-template",	"-p" },
    { "-to",		"-t" },
    { "-transport",	"-T" },
    { "-unsigned",	"-u" },
    { "-unedited",	"-U" },
    { "-verbose",	"-v" },
    /* -w, -W */
    /* -x, -X */
    /* -y, -Y */
    /* -Z */
    { "-background",	"-z" },
    { NULL,		NULL }	/* This must be the last entry */
  };

/* argc and argv could be null if coming from GUI mode compose */
int
zm_mail(n, argv, list)
register int n;   /* no need for "argc", so use the space for a variable */
register char **argv;
msg_group *list;
{
    char firstchar = argv? **argv : 'm';
    char *route = NULL;
    char addrbuf[HDRSIZ];

    if (ison(glob_flags, IS_GETTING)) {
	wprint(catgets( catalog, CAT_MSGS, 547, "You must finish the composition you are editing first.\n" ));
	return -1;
    }
    if (ison(glob_flags, DO_PIPE)) {
	wprint(catgets( catalog, CAT_MSGS, 548, "You cannot pipe through the mail command.\n" ));
	return -1;
    }
    if (!is_shell && isoff(glob_flags, IS_SENDING)) {
	wprint(catgets( catalog, CAT_MSGS, 549, "You cannot send mail during initialization.\n" ));
	return -1;
    }

    if (interpose_on_msgop("mail", -1, NULL) < 0)
	return 0;

    if (start_compose() < 0) {
	error(ZmErrWarning, catgets( catalog, CAT_MSGS, 550, "Unable to start another composition" ));
	return -1;
    }

    on_intr();	/* If interrupted, we need to stop_compose() */

    if (lower(firstchar) == 'r')
	turnon(comp_current->flags, IS_REPLY);

    /* Set SIGN, DO_FORTUNE, etc. now so we can turn them off later */
    if (boolean_val(VarAutosign))
	turnon(comp_current->send_flags, SIGN);
    if (boolean_val(VarFortune))
	turnon(comp_current->send_flags, DO_FORTUNE);
    if (boolean_val(VarRecord))
	turnon(comp_current->send_flags, RECORDING);
    if (boolean_val(VarRecordUsers))
	turnon(comp_current->send_flags, RECORDUSER);
    if (boolean_val(VarLogfile))
	turnon(comp_current->send_flags, LOGGING);
    if (boolean_val(VarAutoclear))
	turnon(comp_current->flags, AUTOCLEAR);
    if (bool_option(VarAutoformat, "compose"))
	turnon(comp_current->flags, AUTOFORMAT);
    if (bool_option(VarAddressCheck, "compose"))
	turnon(comp_current->flags, DIRECTORY_CHECK);
    if (chk_option(VarAddressCheck, "send"))
	turnon(comp_current->flags, SENDTIME_CHECK);
    if (bool_option(VarVerify, "mail,addresses"))
	turnon(comp_current->flags, CONFIRM_SEND);
    if (boolean_val(VarAddressSort))
	turnon(comp_current->flags, SORT_ADDRESSES);
    if (bool_option(VarAutoedit, "") /* Set to empty */ ||
	    isoff(comp_current->flags, IS_REPLY) &&
		chk_option(VarAutoedit, "new") ||
	    ison(comp_current->flags, IS_REPLY) &&
		chk_option(VarAutoedit, "reply"))
	turnon(comp_current->flags, EDIT);
    
    /* If piped to mail, include the messages piped */
    if (ison(glob_flags, IS_PIPE) ||
	    ison(comp_current->flags, IS_REPLY) &&
	    boolean_val(VarAutoinclude)) {
	turnon(comp_current->flags, INCLUDE);
	msg_group_combine(&comp_current->inc_list, MG_SET, list);
    }

    /* In this loop, use the WAS_INTR flag as a multi-level break */
    while (argv && *argv && *++argv && **argv == '-') {
	n = 1;
	fix_word_flag(argv, mail_flags);
	while (n && argv[0][n] && isoff(glob_flags, WAS_INTR))
	    switch (argv[0][n]) {
		case '\004':	/* MAGIC HACKERY */
		    if (ison(glob_flags, REDIRECT))
			turnon(comp_current->send_flags, DIRECT_STDIN);
		    n++;
		when 'A':
		    if (argv[1]) {
			n = 0;
			if (init_attachment(*++argv) < 0)
			    turnon(glob_flags, WAS_INTR);
		    } else {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 551, "-%c: Must specify a file\n" ), argv[0][n]);
			turnon(glob_flags, WAS_INTR);
		    }
		when 'b':	/* BCC list follows */
		    if (argv[1]) {
			n = 0;
			strcpy(addrbuf, *++argv);
			fix_up_addr(addrbuf, 0);
			comp_current->addresses[BCC_ADDR] = savestr(addrbuf);
		    } else {
			wprint(catgets( catalog, CAT_MSGS, 552, "Must specify blind-carbon list." ));
			wprint("\n");
			turnon(glob_flags, WAS_INTR);
		    }
		when 'C':	/* Comment for X-Remarks: header */
		    if (argv[1]) {
			n = 0;
			comp_current->remark = savestr(*++argv);
		    } else {
			wprint(catgets( catalog, CAT_MSGS, 553, "Must specify comment string\n" ));
			turnon(glob_flags, WAS_INTR);
		    }
		when 'c':	/* CC list follows */
		    if (argv[1]) {
			n = 0;
			strcpy(addrbuf, *++argv);
			fix_up_addr(addrbuf, 0);
			comp_current->addresses[CC_ADDR] = savestr(addrbuf);
		    } else {
			wprint(catgets( catalog, CAT_MSGS, 554, "Must specify carbon-copy list\n" ));
			turnon(glob_flags, WAS_INTR);
		    }
		when 'v': turnon(comp_current->send_flags, VERBOSE); n++;
		when 'H':	/* Prepared draft without headers */
		    if (argv[1]) {
			n = 0;
			comp_current->Hfile = *++argv;
		    } else {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 551, "-%c: Must specify a file\n" ), argv[0][n]);
			turnon(glob_flags, WAS_INTR);
		    }
		/* 'p' is out of alpha order because of fall through */
		when 'p':	/* Prepared template or form */
		    if (argv[1]) {
			char *str;
			if (!(str = get_template_path(argv[1], 0))) {
			    turnon(glob_flags, WAS_INTR);
			    break;
			}
			free(argv[1]);
			argv[1] = str;
		    }
		    /* Fall through */
		case 'h':	/* Prepared draft with headers */
		case 'Q':	/* Draft treated as from outbound queue */
		    if ('h' == argv[0][n] &&
			ison(comp_current->send_flags, SEND_ENVELOPE)) {
			error(UserErrWarning,
			      catgets(catalog, CAT_MSGS, 1009, "-h cannot be used with -Q or -q"));
			turnon(glob_flags, WAS_INTR);
		    }
		    if (argv[1]) {
			xfree(comp_current->hfile);
			comp_current->hfile = savestr(argv[1]);
			if (ison(glob_flags, REDIRECT)) {
			    turnoff(glob_flags, REDIRECT);
			    turnon(glob_flags, NO_INTERACT);
			    turnon(comp_current->send_flags, SEND_NOW);
			}
			if (!*comp_current->hfile) {
			  xfree(comp_current->hfile);
			  comp_current->hfile = NULL;
			}
			if (argv[0][n] == 'Q' && comp_current->hfile) {
			    if (test_folder(comp_current->hfile, NULL) &
				FolderStandard)
				turnon(comp_current->send_flags,
				       SEND_ENVELOPE|SEND_NOW);
			    else {
				error(UserErrWarning,
				      catgets(catalog, CAT_MSGS, 892, "-%c: Must specify a file containing headers"),
				      argv[0][n]);
				turnon(glob_flags, WAS_INTR);
			    }
			}
			argv++;
			n = 0;
		    } else {
			error(UserErrWarning,
			    catgets(catalog, CAT_MSGS, 891, "-%c: Must specify a file name"),
			    argv[0][n]);
			turnon(glob_flags, WAS_INTR);
			n++;
		    }
		when 'E':
		    n++;
		    if (argv[0][n] == '!') {
			turnoff(comp_current->flags, EDIT_HDRS);
			n++;
		    } else
			turnon(comp_current->flags, EDIT_HDRS);
		when 'e':
		    n++;
		    if (argv[0][n] == '!') {
			turnoff(comp_current->flags, EDIT);
			n++;
		    } else
			turnon(comp_current->flags, EDIT);
		when 'F':
		    n++;
		    if (argv[0][n] == '!') {
			turnoff(comp_current->send_flags, FORTUNE);
			n++;
		    } else
			turnon(comp_current->send_flags, FORTUNE);
  	        when 'f': case 'I': case 'i': case 'M': case 'q': {
		    int m;
		    if (!msg_cnt) {
			error(UserErrWarning, catgets( catalog, CAT_MSGS, 557, "No message to include!" ));
			turnon(glob_flags, WAS_INTR);
			break;
		    }
		    if (argv[0][n] == 'i') {
			turnon(comp_current->flags, INCLUDE);
			turnoff(comp_current->flags, INCLUDE_H);
			turnoff(comp_current->flags, FORWARD);
		    } else if (argv[0][n] == 'I') {
			turnon(comp_current->flags, INCLUDE_H);
			turnoff(comp_current->flags, INCLUDE);
			turnoff(comp_current->flags, FORWARD);
		    } else if (argv[0][n] == 'f' || argv[0][n] == 'q') {
			turnon(comp_current->flags, FORWARD);
			turnoff(comp_current->flags, INCLUDE_H);
			turnoff(comp_current->flags, INCLUDE);
		    } else if (argv[0][n] == 'M') {
			turnon(comp_current->flags, FORWARD_ATTACH);
			turnoff(comp_current->flags, INCLUDE_H);
			turnoff(comp_current->flags, INCLUDE);
		    }
		    if (argv[0][n] == 'q') {
			turnon(comp_current->send_flags, SEND_ENVELOPE);
			if (argv[0][n+1] != '!')
			    turnon(comp_current->send_flags, SEND_NOW);
			else
			    n++;
		    }
		    /* "-i 3-5" or "-i3-5"  Consider the latter case first */
		    if (!argv[0][++n])
			argv++, n = 0;
		    (*argv) += n;
		    m = get_msg_list(argv, &comp_current->inc_list);

		    msg_group_combine(list, MG_ADD, &comp_current->inc_list);

		    if (any_p(comp_current->flags, FORWARD|FORWARD_ATTACH) ||
			    ison(comp_current->send_flags, SEND_ENVELOPE)) {
			int i;
			for (i = 0; i < msg_cnt; i++) {
			    if (!msg_is_in_group(&comp_current->inc_list, i))
				continue;
			    if (isoff(comp_current->send_flags,
					SEND_ENVELOPE))
				add_mref(comp_current, i, mref_Forward);
			    else if (ison(current_folder->mf_flags,
					    QUEUE_FOLDER))
				add_mref(comp_current, i, mref_Delete);
			}
		    }

		    (*argv) -= n;
		    if (m == -1) {
			turnon(glob_flags, WAS_INTR);
			break;
		    }
		    /* if there were args, then go back to the first char
		     * in the next argv
		     */
		    if (m)
			n = 0;
		    if (!n) /* n may be 0 from above! */
			argv += (m-1);
		}
		when 'l':
		    n++;
		    if (argv[0][n] == '!') {
			turnoff(comp_current->send_flags, RECORDING);
			n++;
		    } else
			turnon(comp_current->send_flags, RECORDING);
		when 'L':
		    n++;
		    if (argv[0][n] == '!') {
			turnoff(comp_current->send_flags, LOGGING);
			n++;
		    } else
			turnon(comp_current->send_flags, LOGGING);
		when 'R':
		    n++;
		    if (argv[0][n] == '!') {
			turnoff(comp_current->send_flags, RETURN_RECEIPT);
			turnon(comp_current->send_flags, NO_RECEIPT);
			n++;
		    } else
			turnon(comp_current->send_flags, RETURN_RECEIPT);
		when 'U':
		    turnon(comp_current->send_flags, SEND_NOW);
		    n++;
		when 'u':
		    turnoff(comp_current->send_flags, SIGN);
		    turnoff(comp_current->send_flags, DO_FORTUNE);
		    n++;
		/* 'r' is out of alpha order because of fall through */
		when 'r':
		    if (ison(comp_current->flags, IS_REPLY)) {
			route = *++argv;
			n = 0;
			break;
		    }
		    /* Fall through */
		default:
		    turnon(glob_flags, WAS_INTR);
		    if (argv[0][n] != '?') {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 558, "%c: unknown option" ), argv[0][n]);
		    } else {
			/* Can't get away with WAS_INTR */
			off_intr();	/* Do this first!! */
			stop_compose();
			return help(0, "mail", cmd_help);
		    }
		when 's':
		    if (argv[1]) {
			n = 0;
			comp_current->addresses[SUBJ_ADDR] =
			    savestr(*++argv);
		    } else
			n++, turnon(comp_current->flags, NEW_SUBJECT);
		when 't':	/* TO list follows */
		    if (argv[1]) {
			n = 0;
			strcpy(addrbuf, *++argv);
			fix_up_addr(addrbuf, 0);
			comp_current->addresses[TO_ADDR] = savestr(addrbuf);
		    } else {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 552, "Must specify blind-carbon list." ));
			turnon(glob_flags, WAS_INTR);
		    }
		when 'T':
		    if (argv[1]) {
			n = 0;
			comp_current->transport = savestr(*++argv);
		    } else {
			error(UserErrWarning, catgets( catalog, CAT_MSGS, 560, "Must specify transport agent" ));
			turnon(glob_flags, WAS_INTR);
		    }
		when 'z':
		    n++;
		    turnon(comp_current->flags, COMP_BACKGROUND);
	    }
	/* Handle all sorts of interrupts, including parse errors */
	if (ison(glob_flags, WAS_INTR)) {
	    off_intr();		/* Do this first!! */
	    stop_compose();
	    return -1;
	}
    }

    off_intr();	/* Let rm_edfile() take it from here */

    if (!comp_current->hfile &&
	    none_p(comp_current->send_flags, DIRECT_STDIN|SEND_ENVELOPE)) {
	char *default_temp = NULL;

	if (boolean_val(VarTemplates))
	    default_temp = get_template_path(DEFAULT_TEMP_NAME, 1);
	if (default_temp) {
	    comp_current->hfile = default_temp;
	    if (ison(glob_flags, REDIRECT)) {
		turnoff(glob_flags, REDIRECT);
		turnon(glob_flags, NO_INTERACT);
	    }
	}
    }

    if (ison(glob_flags, NO_INTERACT))
	turnon(comp_current->send_flags, SEND_NOW);

    /* Check validity of option combinations */
    if (!msg_cnt && ison(comp_current->flags, FORWARD))
	error(ison(glob_flags, REDIRECT)? ZmErrFatal : UserErrWarning,
	    catgets(catalog, CAT_MSGS, 939, "Forwarding from an empty folder"));
    if (ison(comp_current->send_flags, SEND_NOW)) {
	if (ison(comp_current->flags, SENDTIME_CHECK))
	    turnon(comp_current->flags, DIRECTORY_CHECK);
	if ((comp_current->hfile || comp_current->Hfile ||
		    comp_current->attachments)) {
	    if (ison(comp_current->flags, FORWARD)) {
		error(UserErrWarning, 
		    catgets( catalog, CAT_MSGS, 893, "Cannot include or attach files when resending or resubmitting." ));
		stop_compose();
		return -1;
	    }
	} else if (none_p(comp_current->flags, FORWARD|INCLUDE|INCLUDE_H) &&
		       !comp_current->attachments) {
	    error(UserErrWarning,
		catgets( catalog, CAT_MSGS, 563, "Cannot send immediately without input file." ));
	    stop_compose();
	    return -1;
	}
	if (ison(comp_current->flags, EDIT_HDRS) &&
		ison(comp_current->send_flags, DIRECT_STDIN)) {
	    /* Must copy to scan headers */
	    error(UserErrWarning,
		catgets( catalog, CAT_MSGS, 564, "Cannot write direct to MTA with draft file." ));
	    turnoff(comp_current->send_flags, DIRECT_STDIN);
	}
	turnoff(comp_current->flags, EDIT); /* -U overrides -e */
    }
    if (comp_current->attachments &&
	    ison(comp_current->send_flags, SEND_NOW)) {
	turnoff(comp_current->send_flags, SIGN);
	turnoff(comp_current->send_flags, DO_FORTUNE);
    }

    if (boolean_val(VarVerbose))
	turnon(comp_current->send_flags, VERBOSE);

    /* Subject is also generated if necessary */
    if (generate_addresses(argv, route, firstchar == 'R', list) < 0) {
	stop_compose();
	return -1;
    }

    /* If forwarding w/o editing, start a new file for each. */
    if (ison(comp_current->flags, FORWARD) &&
	    isoff(comp_current->send_flags, DIRECT_STDIN) &&
	    ison(comp_current->send_flags, SEND_NOW)) {
	msg_group fwd;
	register int i, j;
	Compose *first_compose = comp_current;

	suspend_compose(comp_current);
	start_compose();	/* To be a working copy of first_compose */

	/* NOTE: This is really a sloppy way to do this -- we ought
	 * to swap the usages of fwd and comp_current->inc_list so
	 * that no list needs to be passed to start_file().	XXX
	 */
	init_msg_group(&fwd, 1, 1);
	clear_msg_group(&fwd);
	for (i = j = 0; i < msg_cnt; i++)
	    if (msg_is_in_group(&first_compose->inc_list, i)) {
		char buf[64];
		add_msg_to_group(&fwd, i);
		/* Bart: Mon Jul 13 14:02:30 PDT 1992
		 * We can't forward huge numbers of messages asynchronously,
		 * it swamps file descriptor and process tables or uses up
		 * all the swap ... so forcibly go into SYNCH_SEND after we
		 * forward the third message.  This is an arbitrary number,
		 * but probably safe.
		 */
		if (j++ > 2)
		    turnon(first_compose->send_flags, SYNCH_SEND);
		/* Need to start a fresh compose after every send */
		(void) sprintf(buf, catgets(catalog, CAT_MSGS, 565, "Resending message %d ..."), i+1);
		assign_compose(comp_current, first_compose);
		if (check_intr_msg(buf) || start_file(&fwd) < 0) {
		    destroy_msg_group(&fwd);
		    resume_compose(first_compose);
		    stop_compose();
		    return -1;
		}
#if 0
		if (isoff(comp_current->send_flags, SEND_ENVELOPE))
		    turnon(msg[i]->m_flags, RESENT);
		/* else
		    turnon(msg[i]->m_flags, SENT_ONCE); */
		if (isoff(folder_flags, READ_ONLY))
		    turnon(folder_flags, DO_UPDATE);
#endif /* 0 */
		clear_msg_group(&fwd);
	    }
	destroy_msg_group(&fwd);
	stop_compose();	/* Wipe out the working copy */
	resume_compose(first_compose);
	stop_compose(); /* Wipe out the original copy */
    } else
	return start_file(&comp_current->inc_list);
    return 0;
}

static void
add_mref(cmp, n, type)
Compose *cmp;
int n;
enum mref_type type;
{
    msg_ref *tmp = zmNew(msg_ref);

    tmp->offset = msg[n]->m_offset;
    tmp->link.l_name = (char *)current_folder;
    tmp->type = type;
    push_link(&(cmp->reply_refs), tmp);
}

/*
 * get the full pathname of a template file.  Return NULL if we can't.
 */
char *
get_template_path(str, quiet)
const char *str;
int quiet;
{
  int tc; char **tv;
  char *full;

#ifndef MAC_OS
  if (is_fullpath(str)) return savestr(str);
#endif
  if ((tc = list_templates(&tv, str, quiet)) <= 0) return NULL;
  full = tv[--tc];
  tv[tc] = NULL;
  free_vec(tv);
  return full;
}

static int
start_file(list)
msg_group *list;
{
    register int   i;
    int		   had_hfile = FALSE;

    if (ison(comp_current->send_flags, DIRECT_STDIN))
	comp_current->ed_fp = stdin;
    else if (!(comp_current->ed_fp =
	    open_tempfile(EDFILE, &comp_current->edfile))) {
	stop_compose();
	return -1;
    }

    if (any_p(glob_flags, REDIRECT|NO_INTERACT) &&
	    isoff(glob_flags, IS_FILTER) &&
	    !(comp_current->hfile || comp_current->Hfile)) {
	int sent;
	(void) generate_headers(comp_current, NULL_FILE, 0);
	if (isoff(comp_current->send_flags, DIRECT_STDIN))
	    (void) fp_to_fp(stdin, -1, -1, comp_current->ed_fp);
	saved_charset = outMimeCharSet;
	outMimeCharSet = comp_current->out_char_set;
	SetCurrentCharSet(comp_current->out_char_set);
	if ((sent = send_it()) == -1) {
	    error(ForcedMessage,
		  catgets( catalog, CAT_MSGS, 566, "Message not sent!" ));
	    rm_edfile(-1);
	} else
	    stop_compose();
	SetCurrentCharSet(displayCharSet);	/* Just in case */
	outMimeCharSet = saved_charset;
	return sent;
    }

    if ((isoff(comp_current->flags, FORWARD) ||
	 isoff(comp_current->send_flags, SEND_NOW)) &&
	    boolean_val(VarEditHdrs)) {
	turnon(comp_current->flags, EDIT_HDRS);
    }

    if (comp_current->hfile) {
	if (file_to_fp(comp_current->hfile, comp_current->ed_fp, "r", 1) < 0) {
	    rm_edfile(-1);
	    return -1;
	}
	had_hfile = TRUE;
	xfree(comp_current->hfile), comp_current->hfile = NULL;
    }

    if (generate_headers(comp_current, comp_current->ed_fp, had_hfile) < 0) {
	rm_edfile(-1);
	return -1;
    }

    if (comp_current->Hfile) {
	(void) file_to_fp(comp_current->Hfile, comp_current->ed_fp, "r", 1);
	comp_current->Hfile = NULL;
	had_hfile = TRUE;
    }

    /* if flags call for it, include current message (with header?) */
    if (ison(comp_current->flags, INCLUDE|FORWARD|FORWARD_ATTACH|INCLUDE_H)) {
	long copy_flgs = 0;
	char buf[HDRSIZ];
	if (isoff(comp_current->flags, FORWARD_ATTACH) &&
		isoff(comp_current->flags, FORWARD)) {
	    turnon(copy_flgs, INDENT|FOLD_ATTACH);
	    if (ison(comp_current->flags, INCLUDE_H) &&
		    !bool_option(VarAlwaysignore, "include"))
		turnon(copy_flgs, NO_IGNORE);
	    else if (ison(comp_current->flags, INCLUDE))
		turnon(copy_flgs, NO_HEADER);
	} else {
            if (ison(comp_current->flags, FORWARD) &&
		    isoff(comp_current->flags, FORWARD_ATTACH) &&
		    isoff(comp_current->send_flags, SEND_NOW) /* &&
		    isoff(comp_current->send_flags, SEND_ENVELOPE) */)
		turnon(copy_flgs, FOLD_ATTACH);
	    if (ison(comp_current->send_flags, SEND_NOW) ||
		    ison(comp_current->send_flags, SEND_ENVELOPE) ||
		    ison(comp_current->flags, FORWARD_ATTACH) ||
		    !bool_option(VarAlwaysignore, "forward"))
		turnon(copy_flgs, NO_IGNORE);
	    turnon(copy_flgs, FORWARD);
	}
	turnon(copy_flgs, NO_SEPARATOR);
	SetCurrentCharSet(displayCharSet);
	for (i = 0; i < msg_cnt; i++)
	    if (msg_is_in_group(list, i)) {
		char xbuf[64], *att_fname = NULL;
		FILE *att_fp = NULL_FILE;
		if ((ison(comp_current->flags, FORWARD_ATTACH) ||
		     ison(comp_current->flags, FORWARD)) &&
		    isoff(comp_current->send_flags, SEND_NOW) &&
		    isoff(comp_current->send_flags, SEND_ENVELOPE)) {
		    (void) reply_to(i, FALSE, buf);
		    if (ison(comp_current->flags, FORWARD_ATTACH)) {
			FMODE_SAVE();
			FMODE_BINARY();
			att_fp = open_tempfile("msg", &att_fname);
			FMODE_RESTORE();
			if (! att_fp) {
			    error(SysErrWarning, 
				  catgets(catalog, CAT_MSGS, 809, "Unable to create temporary file for forwarded message."));
			    return -1;
			}
		    }
		    else {
			if (istool && isoff(comp_current->flags, EDIT))
			    (void) fputc('\n', comp_current->ed_fp);
			(void) fprintf(comp_current->ed_fp,
				       catgets( catalog, CAT_MSGS, 314, "--- Forwarded mail from %s\n\n" ), decode_header("from", buf));
		    }
		}
		(void) sprintf(xbuf, (ison(comp_current->flags, FORWARD_ATTACH) ||
				      ison(comp_current->flags, FORWARD)) ?
				      catgets( catalog, CAT_MSGS, 569, "forwarding message %d ..." )
				    : catgets( catalog, CAT_MSGS, 570, "including message %d..." ),
			       i+1);
		wprint(xbuf);
		if (check_intr_msg(xbuf)) {
		    wprint("\n");
		    return -1;
		}
		wprint(catgets( catalog, CAT_MSGS, 571, " (%d lines)\n" ),
		       copy_msg(i,
				att_fp? att_fp : comp_current->ed_fp,
				(u_long)copy_flgs, NULL, 0));
		set_isread(i); /* if we included it, we read it, right? */
#if defined( IMAP )
                zimap_turnoff( current_folder->uidval, msg[current_msg]->uid,
                        ZMF_UNREAD );
#endif
		if ((ison(comp_current->flags, FORWARD_ATTACH) ||
			    ison(comp_current->flags, FORWARD)) &&
			isoff(comp_current->send_flags, SEND_NOW) &&
			isoff(comp_current->send_flags, SEND_ENVELOPE)) {
		    if (ison(comp_current->flags, FORWARD_ATTACH)) {
			if (att_fp && fclose(att_fp) != EOF)
			    add_attachment(comp_current, att_fname,
					   MimeTypeStr(MessageRfc822),
					   zmVaStr("Message from %s", buf),
					   NULL, AT_TEMPORARY, NULL);
			else
			    error(SysErrWarning, 
				  catgets( catalog, CAT_MSGS, 812, "Unable to close temporary file %s for forwarded message.\nAttachment not added." ), att_fname);
			xfree(att_fname);
		    }
		    else {
			(void) fprintf(comp_current->ed_fp,
				       catgets( catalog, CAT_MSGS, 572, "\n---\
End of forwarded mail from %s\n" ), decode_header("from", buf));
		    }
		}
		if (isoff(comp_current->flags, FORWARD_ATTACH) &&
		    isoff(comp_current->flags, FORWARD) &&
		    ison(comp_current->flags, INCLUDE|INCLUDE_H))
		    (void) fputc('\n', comp_current->ed_fp);
		if (ison(comp_current->flags, FORWARD) &&
		    isoff(comp_current->flags, FORWARD_ATTACH) &&
		    isoff(comp_current->send_flags, SEND_NOW))
		    copy_attachments(i);
	    }
	(void) fflush(comp_current->ed_fp);
    }
    if (!istool && ison(glob_flags, WARNINGS)) {
	if (escape && strncmp(escape, DEF_ESCAPE, 1))
	    print(catgets( catalog, CAT_MSGS, 573, "(escape character is set to `%c')\n" ), *escape);
	if (wrapcolumn && wrapcolumn < 20)
	    print(catgets( catalog, CAT_MSGS, 574, "(warning: wrapping only %d columns from the left!)\n" ),
		    wrapcolumn);
    }

    /* Attempt to restore sanity if pulling a message out of the queue */
    if (ison(comp_current->send_flags, SEND_ENVELOPE)) {
	int edheads = ison(comp_current->flags, EDIT_HDRS);
	/*
	 * What's happening:  We've just "forwarded" the message, headers
	 * and all, into our own message editor file.  So we have to make
	 * that message's headers become our own, for later processing.
	 * We do this by forcing EDIT_HEADERS and reloading the editor file.
	 */
	turnon(comp_current->flags, EDIT_HDRS);
	reload_edfile();
	/*
	 * Next, if we are NOT going to send the message right away, we have
	 * to get the composition into a state usable by the composition.
	 * What should happen here is that we identify the attachment part
	 * (if any) that is the "main message body" and copy that into the
	 * editor file.  Since we don't know what part to copy (there may
	 * not have been any "main message body" when the message got sent)
	 * we have two choices: scrap the message body entirely and make
	 * them edit the attachments (#if 1 below) or leave all the folded
	 * attachment crap in the "message text" and make the user delete
	 * what he doesn't want (#else below).
	 *
	 * In either case, we reset the correct state of EDIT_HDRS.  If
	 * there is an X-Zm-Envelope-To:, this could hide it, which is
	 * somewhat dangerous ...
	 */
	if (isoff(comp_current->send_flags, SEND_NOW)) {
#if 1
	    turnoff(comp_current->flags, EDIT_HDRS);
	    if (comp_current->attachments)
		truncate_edfile(comp_current, 0L);
	    if (edheads)
		turnon(comp_current->flags, EDIT_HDRS);
#else
	    if (!edheads)
		turnoff(comp_current->flags, EDIT_HDRS);
#endif
	    turnoff(comp_current->send_flags, SEND_ENVELOPE);
	}
    }

    /* forwarded messages must be unedited */
    if (isoff(comp_current->send_flags, SEND_NOW))
	turnoff(comp_current->flags, FORWARD);

    /* This is really stupid, and nobody paid extra for it, either. */
    if (ison(comp_current->send_flags, SIGN|DO_FORTUNE) &&
	    none_p(glob_flags, REDIRECT|NO_INTERACT) &&
	    (istool || ison(comp_current->flags, EDIT)) &&
	    isoff(comp_current->flags, FORWARD) && boolean_val(VarPresign)) {
	char *addr_list = savestr(comp_current->addresses[TO_ADDR]);

	if (addr_list && *addr_list && comp_current->addresses[CC_ADDR] &&
		*(comp_current->addresses[CC_ADDR]))
	    (void) strapp(&addr_list, ", ");
	if (comp_current->addresses[CC_ADDR])
	    (void) strapp(&addr_list, comp_current->addresses[CC_ADDR]);

	/* sign_letter() ignores an empty or NULL address list, so ... */
	if (!addr_list || addr_count(addr_list) == 0) {
	    xfree(addr_list);
	    addr_list = savestr(VarPresign);
	} else {
	    char *p;
	    rm_cmts_in_addr(p = alias_to_address(addr_list));
	    ZSTRDUP(addr_list, p);
	}

	sign_letter(addr_list, comp_current->send_flags, comp_current->ed_fp);
	/* Don't sign again */
	turnoff(comp_current->send_flags, SIGN|DO_FORTUNE);

	xfree(addr_list);
    }

    if (!istool && ison(comp_current->flags, EDIT)) {
	if (run_editor(NULL) < 0)
	    return -1;
	turnon(comp_current->flags, MODIFIED);
    } else if (ison(comp_current->send_flags, SEND_NOW)) {
	/* if finish_up_letter() was successful, file was successfully sent. */
#if defined(_XOPEN_SOURCE) || defined(_POSIX_C_SOURCE)
	if (!(killme = sigsetjmp(cntrl_c_buf, 1)) && finish_up_letter() == 0) {
#else /* XOPEN || POSIX */
	if (!(killme = setjmp(cntrl_c_buf)) && finish_up_letter() == 0) {
#endif /* XOPEN || POSIX */
	    return 0;
        }
#ifdef ZMCOT
	else if (spIm_view(ZmlIm) == (struct spView *) ZmcotDialog) {
	    rm_edfile(0);
	    return (-1);
	}
#endif /* ZMCOT */
	else if (istool > 1 && ison(comp_current->flags, FORWARD)) {
	    rm_edfile(0);
	    return -1;
	}
    } else if (!istool) {
	if (had_hfile) {
	    /* it's not obvious what's going on -- enlighten user */
	    wprint(catgets( catalog, CAT_MSGS, 575, "(continue editing or ^%c to send)\n" ), eofc + '@');
	} else {
	    if (ison(comp_current->flags, COMP_BACKGROUND)) {
		wprint("[%s]\n", comp_current->link.l_name);
	    } else {
	    /* Moved this here from generate_addresses() to clean up the
	     * interface with the Cray dynamic headers foolishness.
	     */
		wprint("\n");
	    }
	}
    }

#ifdef GUI
    if (istool > 1) {
	/* toolmode doesn't care if SEND_NOW -- user continues to edit file.
	 * if SEND_NOW is not set, then the editor file has just been started,
	 * so again, just return so user can edit file.
	 */
	if (prepare_edfile() < 0) {
	    rm_edfile(-1);
	    return  -1;
	}
	suspend_compose(comp_current); /* actually, the tool is taking over */

	/* make sure frame is open */
	if (gui_open_compose(ask_item, comp_current) < 0) {
	    rm_edfile(-1);
	    return  -1;
	}	
	return 0;
    }
#endif /* GUI */
#ifdef GUI_ONLY
    return -1;	/* Should never get here */
#else /* !GUI_ONLY */
    if (ison(comp_current->send_flags, SEND_NOW)) {
	/* editing couldn't have been on -- finish_up_letter() failed */
	rm_edfile(0 - !!ison(comp_current->flags, FORWARD));
	return -1;
    }
    /* run the compose mode hook function */
    if (lookup_function(COMPOSE_HOOK)) {
	char buf[sizeof COMPOSE_HOOK + 1];	/* XXX Blechh */
	(void) cmd_line(strcpy(buf, COMPOSE_HOOK), NULL_GRP);
    }
    if (ison(comp_current->flags, COMP_BACKGROUND)) {
	suspend_compose(comp_current);
	return 0;
    }
    return compose_letter();
#endif /* !GUI_ONLY */
}

#ifndef GUI_ONLY
int
compose_letter()
{
    char line[MAXPATHLEN*2];
    int i = 0;
    static short eofcount;

    if (istool) {
	error(ZmErrWarning, catgets( catalog, CAT_MSGS, 576, "Tool reached compose loop?!?" ));
	return -1;
    }
    turnon(glob_flags, IS_GETTING);
#ifdef SIGCONT
#ifdef ZM_JOB_CONTROL
    /* This should be more modular if it's going to be implemented
     * correctly.  See stop_start() in signals.c for the other half,
     * and execute.c for the final analysis.
     */
    if (comp_current->exec_pid) {
	exec_pid = comp_current->exec_pid;
	comp_current->exec_pid = 0;
	execute(DUBL_NULL);	/* Wait for the editor */
	wprint(catgets( catalog, CAT_MSGS, 577, "(continue editing 337)\n" ));
    }
#endif /* ZM_JOB_CONTROL */
#endif /* SIGCONT */
    eofcount = 0;
    do  {
	/* If the user hits ^C in cbreak mode, Z-Mail could return to
	 * Getstr and not clear the buffer.  Whatever is typed next would
	 * be appended to the line.  Jumping here will force the line to
	 * be cleared cuz it's a new call.
	 *
	 * NOTE: Return of 2 is presently impossible.  What is this for?
	 */
	(void) signal(SIGINT, rm_edfile);
#if defined(_XOPEN_SOURCE) || defined(_POSIX_C_SOURCE)
	if ((killme = sigsetjmp(cntrl_c_buf, 1)) == 2)
#else /* XOPEN || POSIX */
	if ((killme = setjmp(cntrl_c_buf)) == 2)
#endif /* XOPEN || POSIX */
	    break;

	/* Short-circuit evaluation important in this expression! */
	while (Getstr("", line, sizeof(line), 0) > -1 || ++eofcount == 0) {
	    killme = 0;
	    eofcount = 0;
	    if ((i = add_to_letter(line, 1)) <= 0)
		break;
	    else
		i = 0; /* return 0 if the NEXT Getstr() ends with ^D */
	    /* if new mail comes in, get it */
	    if (!istool) { /* toolmode checks on a timer -- don't do it here */
		/* Bart: Wed Jan  4 14:40:39 PST 1995
		 * We can't even GET here in GUI mode, can we?
		 */
#ifdef TIMER_API
		timer_catch_up();
	        if (timer_state(passive_timer) == TimerInactive)
		    fetch_passively();
#else /* !TIMER_API */
		shell_refresh();
#endif /* TIMER_API */
	    }
	}
	if (eofcount > 100) {
	    rm_edfile(-2);
	    i = -1;
	}
    } while (i == 0 && finish_up_letter() == -1);
    return i == -2? 0 : i; /* return 0 if ~z or sent, -1 if ~x or ~q */
}
#endif /* !GUI_ONLY */

#ifdef NOT_NOW
/**** Note that these have not been updated, as they are not being used now
 */
/* This array contains formats to be passed to sprintf()
 */
char *tilde_commands[] = {
"%ccommands: [OPTIONAL argument]\n"
"%c%c\t\tBegin a line with a single %c\n"
"%ct [list]\tChange list of recipients\n"
"%cs [subject]\tModify [set] subject header\n"
"%cc [cc list]\tModify [set] carbon copy recipients\n"
"%cb [bcc list]\tModify [set] blind carbon recipients\n"
"%ch\t\tModify all message headers\n"
"%ce [editor]\tEnter editor. Editor used: \"set editor\", env EDITOR, vi\n"
"%cv [editor]\tEnter visual editor. \"set visual\", env VISUAL, vi\n"
"%cu\t\tEdit previous (last) line in file.\n"
"%cp [pager]\tPage message; pager used: \"set pager\", env. PAGER, more\n"
"%c|cmd\t\tPipe the message through the unix command \"cmd\".\n"
"%cE[!]\t\tClear contents of letter after saving to dead.letter [unless !].\n"
"%ci [msg#'s]\tInclude current msg body [msg#'s] indented by \"indent_str\"\n"
"%cI [msg#'s]\tSame, but include the message headers from included messages\n"
"%cf [msg#'s]\tForward mail. Not indented, but marked as \"forwarded mail\"\n"
"%cr file\t\tRead filename into message buffer\n"
"%cS[!]\t\tInclude Signature file [suppress file]\n"
"%cF[!]\t\tAdd a fortune at end of letter [don't add]\n"
"%cR[!]\t\tRequest [do not request] a return receipt\n"
"%c$variable\tInsert the string value for \"variable\" into message.\n"
"%cw file\t\tWrite msg buffer to file name\n"
"%ca file\t\tAppend msg buffer to file name\n"
"%cq \t\tQuit message; save in dead.letter (unless \"nosave\" is set).\n"
"%cx \t\tQuit message; don't save in dead.letter.\n"
"%cz \t\tSuspend message; resume later via \"fg\" command.\n"
"%c:cmd\t\tRun the mail command \"cmd\".\n"
0
};
#endif /* NOT_NOW */

/*
 * finish up the letter. ask for the cc line, if verify is set, ask to
 * verify sending, continue editing, or to dump the whole idea.
 * Then check for signature and fortune.  Finally, pass it to send_it()
 * to actually send it off.  If send_it() is successful, stop_compose().
 *
 * Return 0 on success, -1 on failure.
 *
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
int
finish_up_letter()
{
    register char *p;
    int send_result = AskYes;
#ifdef PARTIAL_SEND
    unsigned long saved_splitsize = comp_current->splitsize;
#endif /* PARTIAL_SEND */

    /* forwarded mail has no additional personalized text */
    if (ison(comp_current->flags, FORWARD)) {
	saved_charset = outMimeCharSet;
	outMimeCharSet = comp_current->out_char_set;
	SetCurrentCharSet(comp_current->out_char_set);
	if ((send_result = send_it()) < 0)
	    error(ForcedMessage,
		  catgets( catalog, CAT_MSGS, 566, "Message not sent!" ));
	else
	    reset_compose(comp_current); /* zm_mail() does stop_compose() */
	SetCurrentCharSet(displayCharSet);	/* Just in case */
	outMimeCharSet = saved_charset;
	return send_result;
    }

    /* REDIRECT should never be on here, but just in case */
    if (none_p(glob_flags, REDIRECT|NO_INTERACT)) {
	char confirm = chk_option(VarVerify, "mail,addresses");

	if (!istool && boolean_val(VarAskcc))
	    input_address(CC_ADDR, p = "");

	/* ~v on the Cc line asks for verification */
	if (!istool && comp_current->addresses[CC_ADDR] &&
		!strncmp(comp_current->addresses[CC_ADDR], "~v", 2)) {
	     *comp_current->addresses[CC_ADDR] = 0; /* so we don't Cc to ~v! */
	     confirm = TRUE;
	}

#ifdef DSERV
#if defined(_WINDOWS) || defined(MAC_OS)
	if (ison(comp_current->flags, SENDTIME_CHECK) && 
		boolean_val(VarConnected) && (!(ZmUsingUUCP())))
#else
	if (ison(comp_current->flags, SENDTIME_CHECK))
#endif
	{
	    if (check_all_addrs(comp_current, TRUE) < 0)
		return -1;
	}
#endif /* DSERV */

	if (ison(comp_current->flags, CONFIRM_SEND)) {
#ifdef DSERV
	    if (chk_option(VarVerify, "addresses") ||
		    !confirm && ison(comp_current->flags, SENDTIME_CHECK)) {
		send_result = (int) confirm_addresses(comp_current);
	    } else
#endif /* DSERV */
	    {
		send_result = (int)
		    ask((istool > 1 ||
			ison(comp_current->send_flags, SEND_NOW))?
			AskOk : AskYes, catgets( catalog, CAT_MSGS, 367, "Send Message?" ));
	    }
	}
#ifdef PARTIAL_SEND
	if (send_result == AskYes)
	    send_result = partial_confirm(comp_current);
#endif /* PARTIAL_SEND */
	switch (send_result) {
	case AskCancel :
#ifdef GUI
	    /* XXX I hate having to know about the compose window! */
	    if (istool) {
		turnon(comp_current->send_flags, SEND_CANCELLED);
#ifdef PARTIAL_SEND
		comp_current->splitsize = saved_splitsize;
#endif /* PARTIAL_SEND */
		return -1;
	    } else
#endif /* GUI */
		rm_edfile(-2);
	    return 0;
	case AskNo :
	    if (istool < 2 &&
		isoff(comp_current->send_flags, SEND_NOW))
		wprint(catgets( catalog, CAT_MSGS, 337, "(continue editing message)\n" ));
#ifdef PARTIAL_SEND
	    comp_current->splitsize = saved_splitsize;
#endif /* PARTIAL_SEND */
	    return -1;
	}
    }

    saved_charset = outMimeCharSet;
    outMimeCharSet = comp_current->out_char_set;
    SetCurrentCharSet(comp_current->out_char_set);
    send_result = send_it();
    SetCurrentCharSet(displayCharSet);		/* Just in case */
    outMimeCharSet = saved_charset;
#ifdef PARTIAL_SEND
    comp_current->splitsize = saved_splitsize;
#endif /* PARTIAL_SEND */

    switch (send_result) {
    case -1:
	if (istool < 2 && isoff(comp_current->send_flags, SEND_NOW))
	    wprint(catgets( catalog, CAT_MSGS, 608, "(continue)\n" ));
	break;
    case 0:
	if (ison(comp_current->send_flags, SEND_KILLED|SEND_CANCELLED)) {
#ifdef GUI
	    /* XXX I hate having to know about the compose window! */
	    if (istool == 2) {
		return -1;
	    } else
#endif /* GUI */
	    rm_edfile(SIGKILL);
	} else
	    stop_compose();
    }
    
    return send_result;
}

/*
 * Build the address line to give to the mail transfer system.  This
 * address line cannot contain comment fields!  First, expand aliases
 * since they may contain comment fields within addresses. Copy this
 * result back into the address field; this will go into the header ...
 * Next, remove all comments so the buffer contains ONLY valid addresses.
 * Next, strip off any filenames/programs which might occur in the list.
 * Finally, add this information to the command line buffer, pointed to
 * by parameter b.  Remove commas if necessary (see ifdefs).
 *
 * Return the number of files added to names, or -1 on error.
 */
int
extract_addresses(addr_list, names, next_file, size)
    char *addr_list;
    char **names;
    int next_file, size;
{
    char expand = !boolean_val(VarNoExpand);
    char *p, *b = addr_list;
    int first_file = next_file;

    if (p = get_envelope_addresses(comp_current)) {
	/* This should mean that the message has been through this function
	 * once already.  Don't bother with signatures and so forth, and
	 * assume we've already stripped out all the file names and so on.
	 */
	(void) strcpy(b, p);
	return next_file;
    }

    if (!(p = alias_to_address(comp_current->addresses[TO_ADDR]))) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 609, "address expansion failed for To: list" ));
	return -1;
    } else {
	next_file += find_files(p, names+next_file, size-next_file, 0,
	    record_users(comp_current));
	if (expand)
	    (void) strcpy(comp_current->addresses[TO_ADDR], p);
	rm_cmts_in_addr(p);
	skipspaces(0);
	b += Strcpy(b, p);
    }
    if ((p = comp_current->addresses[CC_ADDR]) && *p) {
	if (!(p = alias_to_address(comp_current->addresses[CC_ADDR]))) {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 610, "address expansion failed for Cc: list" ));
	    return -1;
	} else {
	    next_file += find_files(p, names+next_file, size-next_file, 0,
		record_users(comp_current));
	    if (expand)
		(void) strcpy(comp_current->addresses[CC_ADDR], p);
	    rm_cmts_in_addr(p);
	    skipspaces(0);
	    if (*p) {
		*b++ = ',', *b++ = ' ';
		b += Strcpy(b, p);
	    }
	}
    }

    /* expand Bcc addrs, but don't add to list yet.  sign letter first */
    if ((p = comp_current->addresses[BCC_ADDR]) && *p) {
	if (p = alias_to_address(comp_current->addresses[BCC_ADDR])) {
	    p = strcpy(comp_current->addresses[BCC_ADDR], p);
	} else {
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 611, "address expansion failed for Bcc: list" ));
	    return -1;
	}
    } else
	p = NULL;

    /* Sign the letter before adding the Bcc list since they aren't
     * considered when adding a signature.
     */
    if (*addr_list && ison(comp_current->send_flags, SIGN|DO_FORTUNE) &&
	    isoff(glob_flags, REDIRECT) &&
	    isoff(comp_current->flags, FORWARD)) {
	sign_letter(addr_list, comp_current->send_flags, comp_current->ed_fp);
	/* Don't sign again */
	turnoff(comp_current->send_flags, SIGN|DO_FORTUNE);
    }

    if (p) { /* p still points to expanded Bcc list */
	next_file += find_files(p, names+next_file, size-next_file, 0,
	    record_users(comp_current));
	rm_cmts_in_addr(p);
	skipspaces(0);
	if (*p) {
	    *b++ = ',', *b++ = ' ';
	    b += Strcpy(b, p);
	}
    }

    if (p = comp_current->addresses[FCC_ADDR]) {
	next_file +=
	    find_files(p, names + next_file, size - next_file, 1, 0);
    }

    return next_file;
}

extern char *get_priority();

/*
 * returns TRUE if the composition should be recorded, according
 * to the record_max and record_control variables.
 */
static int
recording_ok(compose)
Compose *compose;
{
    char *maxstr;
    long maxlen;

#ifdef QUEUE_MAIL
    if (ShouldQueueMail())
    	return FALSE;
#endif /* QUEUE_MAIL */    	
    if ((maxstr = value_of(VarRecordMax)) && *maxstr &&
	(maxlen = atol(maxstr)) && file_size(compose->ed_fp) > maxlen)
	return FALSE;
    if (!boolean_val(VarRecordControl))
	return TRUE;
    /* XXX CHARSET -- it's now outMimeCharSet, is that correct? */
    if (chk_option(VarRecordControl, "attachments") &&
	    !is_plaintext(compose->attachments))
	return FALSE;
    if (chk_option(VarRecordControl, "priority") &&
	    !*get_priority(compose))
	return FALSE;
    if (chk_option(VarRecordControl, "resend") &&
	    ison(compose->send_flags, SEND_NOW) &&
	    ison(compose->flags, FORWARD))
	return FALSE;
    return TRUE;
}

/*
 * actually send the letter.
 * 1. reset all the signals because of fork.
 * 2. determine recipients (users, address, files, programs)
 * 3. determine mailer, fork and return (if not verbose).
 * 4. popen mailer, $record, and other files specified in step 2.
 * 5. make the headers; this includes To: line, and user set headers, etc...
 * 6. copy the letter right into the array of file pointers (step 4).
 * 7. close the mailer and other files (step 4) and remove the edit-file.
 * return -1 if mail wasn't sent.  could be user error, could be the system.
 * allow user to try again or to save message to file and abort message.
 * return 0 if successful.
 *
 * This function is the anti-christ.
 */
static int
send_it()
{
    register char *p, *v, *b, *addr_list;
#ifndef ZM_CHILD_MANAGER
    RETSIGTYPE (*oldchld)() = 0;
#endif /* ZM_CHILD_MANAGER */
    char *orig_addrs[NUM_ADDR_FIELDS], buf[3*HDRSIZ];
    int i, fork_pid = 1;
#ifdef GUI
    int con[2];
#endif /* GUI */
    int next_file = 1; /* reserve files[0] for the mail delivery program */
    int log_file = -1; /* the index into the files array for mail logging */
#ifdef QUEUE_MAIL
    int queue_file = -1;
#endif /* QUEUE_MAIL */
#ifdef MAC_OS
    int rec_file = -1;
#endif /* MAC_OS */
#ifdef MAXFILES
    register int size = MAXFILES - 1;
    FILE *files[MAXFILES];  /* The files into which the message should be
			     * written, like the file for the MTA and the
			     * record file */
    char *names[MAXFILES];
#else /* !MAXFILES */
    register int size = maxfiles() - 1;
    static FILE **files;
    static char **names;
#ifdef PARTIAL_SEND
    char *sendmail_rewrite = 0;
#endif /* PARTIAL_SEND */
    if (!files && !(files = (FILE **)calloc(size + 1, sizeof(FILE *)))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 612, "Cannot allocate FILE pointers" ));
	return -1;
    }
    if (!names && !(names = (char **)calloc(size + 1, sizeof(char *)))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 613, "Cannot allocate name pointers" ));
	return -1;
    }
#endif /* !MAXFILES */

#ifdef QUEUE_MAIL
    if (ShouldQueueMail() && !value_of(VarMailQueue)) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 940, "Can't find mail queue location: $%s not set"),
	    VarMailQueue);
	return -1;
    }
#endif /* QUEUE_MAIL */

    names[0] = names[1] = NULL; /* for free_elems() */

    /* If EDIT_HDRS, make sure the correct headers exist and are intact
     * before bothering to continue.  It should be impossible to get
     * here with EDIT_HDRS set unless we've also been through the part
     * of start_file() that generates the list.  Calling mta_headers()
     * assures that the important address headers have been unified and
     * that comp_current->addresses points to them.
     */
    if (ison(comp_current->flags, EDIT_HDRS) && !comp_current->headers) {
	error(ZmErrWarning, catgets( catalog, CAT_MSGS, 614, "No headers for composition." ));
	return -1;
    }
#ifdef GUI 
#if defined(MOTIF) || defined(VUI)
    gui_compose_headers(comp_current); /* Override headers */
#endif
#endif

    mta_headers(comp_current);	/* May be redundant, but ... */

    i = interpose_on_send(comp_current);
    if (ison(comp_current->send_flags, SEND_KILLED|SEND_CANCELLED))
	return 0;
    if (i < 0)
	return -1;
    else if (i > 0)
	mta_headers(comp_current);	/* May be redundant, but ... */

    if (!*comp_current->addresses[TO_ADDR]) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 615, "You must have a To: recipient to send mail" ));
	return -1;
    }

/* RJL ** 6.9.93 - MSDOS has to take addresses on stdin and define sendmail var */
#if !defined( MSDOS )
    if (!(p = comp_current->transport) && !(p = value_of(VarSendmail))) {
	p = MAIL_DELIVERY;
#ifdef HAVE_SUBMIT				/* XXX THIS SHOULD GO AWAY */
	turnon(comp_current->mta_flags, MTA_ADDR_STDIN);
#endif /* HAVE_SUBMIT */
    }
#else /* MSDOS */
    if (!(p = comp_current->transport) && !(p = value_of(VarSendmail))) {
	p = MAIL_DELIVERY;
    }
#ifndef _WINDOWS
    turnon(comp_current->mta_flags, MTA_ADDR_STDIN);
    if(!getenv("MTA_ADDR_CMDLINE"))  /* this is a temporary thing to dealwith UUPC */
	turnon(comp_current->mta_flags, MTA_ADDR_STDIN);
#endif /* !_WINDOWS */
#endif /* MSDOS */

    i18n_mta_interface(buf);
    b = &buf[strlen(buf)];

#ifdef PARTIAL_SEND
    if (comp_current->splitsize) {
	const char *splitter = value_of(VarSplitSendmail);
	if (splitter && *splitter) {
	    sprintf(b, "SPLITSIZE=%lu %s ",
		    comp_current->splitsize * 1024, splitter);
	    b = &b[strlen(b)];
	    sendmail_rewrite = b;
	}
    }
#endif /* PARTIAL_SEND */

    if (ison(comp_current->send_flags, VERBOSE) || boolean_val(VarVerbose)) {
	/* prevent fork when "verbose" has changed */
	turnon(comp_current->send_flags, VERBOSE);
#ifndef ZM_CHILD_MANAGER
	oldchld = signal(SIGCHLD, SIG_DFL); /* let pclose() do the wait() */
#endif /* ZM_CHILD_MANAGER */
	if (!(v = value_of(VarVerbose)) || !*v) {
#ifdef VERBOSE_ARG
	    v = VERBOSE_ARG;
	} /* always */ {
#else /* VERBOSE_ARG */
	    b += Strcpy(b, p);
	} else {
#endif /* VERBOSE_ARG */
	    if (strncmp(v, "-", 1) != 0) {
		sprintf(b, "%s%s", p, v);
		b = &b[strlen(b)];
	    } else {
		sprintf(b, "%s %s", p, v);
		b = &b[strlen(b)];
	    }
	}
    } else {
	b = b + Strcpy(b, p);
    }

    if (boolean_val(VarMetoo)) {
	if (!(v = value_of(VarMetoo)) || !*v) {
#ifdef METOO_ARG
	    v = METOO_ARG;
	} /* always */ {
#else /* METOO_ARG */
	    ;
	} else {
#endif /* METOO_ARG */
	    if (strncmp(v, "-", 1) != 0)
		b += Strcpy(b, v);
	    else {
		sprintf(b, " %s", v);
		b += strlen(b);
	    }
	}
    }
    *b++ = ' ', *b = 0; /* strcat(b, " "); */
    addr_list = b; /* save this position to check for addresses later */

    /* Make temporary copies of all address fields so we can restore. */
    for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++) {
	if (comp_current->addresses[i])
	    orig_addrs[i] = savestr(comp_current->addresses[i]);
	else
	    orig_addrs[i] = NULL;
    }

    /* Get the addresses for the MTA */
    if ((next_file = extract_addresses(b, names, next_file, size)) < 0) {
	for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++) {
	    if (comp_current->addresses[i])
		(void) strcpy(comp_current->addresses[i], orig_addrs[i]);
	    xfree(orig_addrs[i]);
	}
	free_elems(&names[1]);
	return -1;
    }

    if (!*addr_list && next_file == 1) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 617, "There must be at least 1 legal recipient" ));
	return -1;
    }

    if (ShouldQueueMail())
	turnoff(comp_current->mta_flags, MTA_NO_COMMAS);

    prepare_mta_addrs(addr_list, comp_current->mta_flags);

    if (ison(comp_current->mta_flags, MTA_ADDR_STDIN|MTA_ADDR_FILE))
	*(addr_list-1) = '\0';

#ifdef PARTIAL_SEND
    if (sendmail_rewrite)
	strcpy(sendmail_rewrite, quotesh(sendmail_rewrite, 0, 0));
#endif /* PARTIAL_SEND */

    Debug("mail command: %s\n" , buf);

#ifdef GUI
    /* Last chance to catch the Stop button */
    if (check_nointr_msg(catgets( catalog, CAT_MSGS, 618, "Starting transport agent ..." )))
	return -1;
#endif /* GUI */

#ifdef UNIX
    if ((istool == 2 ||
	isoff(comp_current->send_flags, VERBOSE|SYNCH_SEND)) && debug < 3) {
#ifdef GUI
	if (istool == 2) {
	    if (pipe(con) != 0)
		con[0] = con[1] = -1;
	}
#endif /* GUI */
	switch (fork_pid = zmChildFork()) {
	    case  0:  /* the child will send the letter. ignore signals */
#ifdef SYSV_SETPGRP
		if (setpgrp() == -1)
#else /* !SYSV_SETPGRP */
		if (setpgrp(0, getpid()) == -1)
#endif /* SYSV_SETPGRP */
		    error(SysErrWarning, "setpgrp");
		/* NOTE: No special case needed for tool here because
		 * this is the sending child -- it's going to pclose()
		 * and then exit(), so who cares about the notifier?
		 */
#ifndef ZM_CHILD_MANAGER
		(void) signal(SIGCHLD, SIG_DFL);
#endif /* ZM_CHILD_MANAGER */
		(void) signal(SIGTERM, SIG_IGN);
		(void) signal(SIGINT, SIG_IGN);
		(void) signal(SIGHUP, SIG_IGN);
		(void) signal(SIGQUIT, SIG_IGN);
#ifdef SIGTTIN
		(void) signal(SIGTTOU, SIG_IGN);
		(void) signal(SIGTTIN, SIG_IGN);
#endif /* SIGTTIN */
#ifdef SIGCONT
#ifdef _SC_JOB_CONTROL
                if (sysconf(_SC_JOB_CONTROL) >= 0)
#endif /* _SC_JOB_CONTROL */
		{
		(void) signal(SIGCONT, SIG_IGN);
		(void) signal(SIGTSTP, SIG_IGN);
		}
#endif /* SIGCONT */
		turnon(glob_flags, IGN_SIGS);
#ifdef GUI
		if (istool == 2) {
		    istool = 3;
		    if (con[1] >= 0) {
			dup2(con[1], fileno(stdout));
			dup2(con[1], fileno(stderr));
			(void) close(con[0]);
			(void) close(con[1]);
		    }
		}
#endif /* GUI */
#ifdef FAILSAFE_LOCKS
		drop_locks();
#endif /* FAILSAFE_LOCKS */
		closefileds_except(fileno(comp_current->ed_fp));
	    when -1:
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 619, "fork failed trying to send mail" ));
		for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++)
		    if (comp_current->addresses[i])
			(void)strcpy(comp_current->addresses[i],orig_addrs[i]);
		/* fall thru */
	    default:
		if (fork_pid > 0) {
#if defined(GUI) && !defined(MAC_OS)
		    if (istool == 2) {
			if (con[0] >= 0) {
			    (void) close(con[1]);
			    gui_watch_filed(con[0], fork_pid, DIALOG_NEEDED,
			    	catgets( catalog, CAT_MSGS, 620, "Output from Mail Transport Agent" ));
			}
		    }
#ifdef ZM_CHILD_MANAGER
		    if (ison(comp_current->send_flags, SYNCH_SEND)) {
			handle_intrpt(INTR_ON|INTR_NONE, NULL, 0);
			gui_wait_for_child(fork_pid, catgets( catalog, CAT_MSGS, 621, "Sending mail ..." ), 0L);
			handle_intrpt(INTR_OFF, NULL, 0);
		    }
#endif /* ZM_CHILD_MANAGER */
#endif /* GUI && !MAC_OS */
		    fork_pid = 0;
		}
		/* istool doesn't need ed_fp, so don't keep it around */
		if (istool || !fork_pid) {
		    if (isoff(comp_current->send_flags, DIRECT_STDIN)) {
			if (comp_current->ed_fp)
			    (void) fclose(comp_current->ed_fp);
		    }
		    comp_current->ed_fp = NULL_FILE;
		}
		for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++)
		    xfree(orig_addrs[i]);
		free_elems(&names[1]);
		mark_replies(comp_current);
		return fork_pid;
	}
    }
#else /* !UNIX */
    /* Enable verbose output because it takes so long */
    turnon (comp_current->send_flags, VERBOSE);
#ifdef CHECK_SMTP_ADDRS
    if (!ShouldQueueMail() && *addr_list && chk_option(VarPickyMta, "precheck")) {
	char **tmp_vec = addr_vec(addr_list);
	if (tmp_vec && SMTP_testaddrs(tmp_vec) != 0) {
	    turnon(comp_current->send_flags, SEND_CANCELLED);
	    for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++) {
		if (comp_current->addresses[i])
		    (void)strcpy(comp_current->addresses[i],orig_addrs[i]);
		xfree(orig_addrs[i]);
	    }
	    free_elems(&names[1]);
	    free_vec(tmp_vec);
	    return -1;
	}
	if (tmp_vec)
	    free_vec(tmp_vec);
	else return -1;
    }
#endif /* CHECK_SMTP_ADDRS */
#endif /* !UNIX */

#ifdef C3
    if (isoff(comp_current->flags, FORWARD) &&
	    C3_TRANSLATION_REQUIRED(displayCharSet, outMimeCharSet)) {
	/* Convert the edfile into the outMimeCharSet */
	char *xname = 0;
	FILE *xfp = open_tempfile(EDFILE, &xname);

	if (xfp) {
	    if (fseek(comp_current->ed_fp, comp_current->body_pos, 0) == 0 &&
		fp_c3_translate(comp_current->ed_fp, xfp,
				displayCharSet, outMimeCharSet) == 0) {
		fclose(comp_current->ed_fp);
		unlink(comp_current->edfile);
		xfree(comp_current->edfile);
		comp_current->edfile = xname;
		comp_current->ed_fp = xfp;
		comp_current->body_pos = 0;
		turnoff(comp_current->flags, EDIT_HDRS);
	    } else {
		fclose(xfp);
		unlink(xname);
		xfree(xname);
	    }
	}
    }
#endif /* C3 */

    if (isoff(comp_current->flags, FORWARD) &&
	    attach_files(comp_current) < 0) {
	/* If we never forked, let the user try again */
	if (fork_pid != 1) {
#ifdef UNIX
	    rm_edfile(-1); /* force saving of undeliverable mail */
	    _exit(-1); /* Not a user exit -- a child exit */
#endif /* UNIX */
	} else {
	    turnon(comp_current->send_flags, SEND_KILLED);
	    for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++) {
		if (comp_current->addresses[i])
		    (void) strcpy(comp_current->addresses[i], orig_addrs[i]);
		xfree(orig_addrs[i]);
	    }
	    free_elems(&names[1]);
	    return -1;
	}
    }

    if (debug > 2) {
#ifdef UNIX
	files[0] = stdout;
#else /* !UNIX */
	files[0] = NULL_FILE;
#endif /* !UNIX */
	if (!*addr_list)
	    addr_list = catgets( catalog, CAT_MSGS, 622, "[no recipients]" );
    } else if (*addr_list && !ShouldQueueMail()) {
	/* write to the MTA, if we're connected */
#ifdef ENV_NAME_BREAKS_SENDMAIL
	{
	static char *argv[3] = {"unsetenv", "NAME", 0};
	Unsetenv(2, argv);
	}
#endif /* ENV_NAME_BREAKS_SENDMAIL */
#ifndef UNIX
	files[0] = (FILE *)do_start_sendmail(comp_current, addr_list);
#else /* UNIX */
        if (!(value_of(VarSmtphost))) {
	   files[0] = open_file(buf, TRUE, FALSE);
	} else {
	   files[0] = (FILE *)do_start_sendmail(comp_current, addr_list);
	}
#endif /* UNIX */
	if (files[0] == NULL_FILE) {
	    /* If we never forked, let the user try again */
	    if (fork_pid != 1) {
#ifdef UNIX
		rm_edfile(-1); /* force saving of undeliverable mail */
		_exit(-1); /* Not a user exit -- a child exit */
#endif /* UNIX */
	    } else {
		/* pf Tue Mar  1 14:54:44 1994
		 * we can't back out at this point; attach_files()
		 * has already mangled the attachment list.
		 */
		turnon(comp_current->send_flags, SEND_KILLED);
		for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++) {
		    if (comp_current->addresses[i])
			(void)strcpy(comp_current->addresses[i],orig_addrs[i]);
		    xfree(orig_addrs[i]);
		}
		free_elems(&names[1]);
		return -1;
	    }
	}
    } else
	files[0] = NULL_FILE;

    /* Now we're committed -- no more second chances */
    for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++)
	xfree(orig_addrs[i]);

    if (ison(comp_current->send_flags, VERBOSE)) {
	print(catgets( catalog, CAT_MSGS, 623, "Sending message ... " ));
	if (istool < 2)
	    (void) fflush(stdout);
    }

    if (ison(comp_current->mta_flags, MTA_ADDR_STDIN)) {
	/* give address list to MTA -- assumes appropriate preprocessing
	 * was already done by prepare_mta_addrs().
	 */
	if (*addr_list && files[0])
	    (void) fprintf(files[0], "%s\n\n", addr_list);
    }

    /* see if log is set.  This is just to add message headers. No msg body. */
    if (ison(comp_current->send_flags, LOGGING)) {
	if (!(p = comp_current->logfile) || !*p)
	    p = value_of(VarLogfile);
	if (!p || !*p)
	    p = "~/mail.log";
	log_file = next_file;
#ifndef MAC_OS
#ifdef MSDOS
	/* this is not good...  we need an is_fullpath() which expands
	 * its argument first.  pf Tue Mar 22 22:18:50 1994
	 */
	if (!index("~|/\\+", *p) && !is_fullpath(p))
#else /* !MSDOS */
	if (!index("~|/+", *p))
#endif /* !MSDOS */
	    (void) sprintf(buf, "%s%c%s", value_of(VarCwd), SLASH, p);
	else
	    (void) strcpy(buf, p);
	next_file += find_files(buf, names+next_file,
				size-next_file, 0, 0);
#else
	/* 9/23 !GF:  set force = TRUE to catch file name;  see below. */
	(void) strcpy(buf, p);
	next_file += find_files(buf, names+next_file,
				size-next_file, TRUE, 0);
#endif /* !MAC_OS */
	if (log_file == next_file)
	    log_file = -1;
#ifdef MAC_OS
	else
	    gui_set_filetype(FolderFile, p, NULL);
#endif /* MAC_OS */
    }

    /* see if record is set.  If so, open that file for appending and add
     * the letter in a format such that mail can be read from it
     */
    if (ison(comp_current->send_flags, RECORDING) &&
	    recording_ok(comp_current)) {
	if (!(p = comp_current->record) || !*p)
	    p = value_of(VarRecord);
#ifndef MAC_OS
	if (!p || !*p)
	    p = "~/record";
#ifdef MSDOS
	/* this is not good...  we need an is_fullpath() which expands
	 * its argument first.  pf Tue Mar 22 22:18:50 1994
	 */
	if (!index("~|/\\+", *p) && !is_fullpath(p))
#else /* !MSDOS */
	if (!index("~|/+", *p))
#endif /* !MSDOS */
	    (void) sprintf(buf, "%s%c%s", value_of(VarCwd), SLASH, p);
	else
	    (void) strcpy(buf, p);
	next_file += find_files(buf, names+next_file,
				size-next_file, 0, 0);
#else /* MAC_OS */
	{
	    char recname[MAXPATHLEN];
	    if (!p || !*p) {
		if (!(p = value_of(VarFolder)) || !*p)
		    p = value_of("ROOT");
		sprintf(recname, "%s%c%s", p, SLASH, DEF_RECORD);
	    } else strcpy(recname, p);
	    if (size - next_file) {
		rec_file = next_file;
		names[next_file++] = savestr(recname);
		names[next_file] = 0;
	    }
	}
#endif /* !MAC_OS */
    }

    /* Don't need to open names[0] as files[0], so skip those */
    next_file = 1 + open_list(names + 1, files + 1, next_file - 1);
#ifdef MAC_OS    
    if (rec_file > -1)
	gui_set_filetype(FolderFile, names[rec_file], NULL);
#endif /* MAC_OS */
#ifdef QUEUE_MAIL
    /* write to the queue file if we're disconnected */
    if (ShouldQueueMail())
	queue_file = open_queue_file(&next_file, names, files);
# ifdef MAC_OS
    if (queue_file > -1)
	gui_set_filetype(FolderFile, names[queue_file], NULL);
# endif /* MAC_OS */
#endif /* QUEUE_MAIL */

    /* First, put the message separator in... */
    for (size = 1; size < next_file; size++) {
	if (!files[size]) /* files[size] will be NULL when open failed */
	    continue;
	/* XXX Ideally, we would figure out the type of each file and
	 * write that type; but that's too time consuming, and there
	 * probably won't be more than one on any given machine.
	 */
	if (def_fldr_type == FolderStandard) {
	    time_t t;
	    (void) time(&t);
	    (void) fprintf(files[size], "From %s %s", zlogin, ctime(&t));
	} else if (def_fldr_type == FolderDelimited)
	    (void) fputs(msg_separator, files[size]);
#ifdef QUEUE_MAIL
	if (size == queue_file)
	    fprintf(files[size], "%s: %s\n", ENVELOPE_HEADER, addr_list);
#endif /* QUEUE_MAIL */
    }

    /*  Output all the headers to all the files */
    add_headers(files, next_file, log_file);
#ifndef UNIX
    if (files[0])
	fclose(files[0]);
    files[0] = NULL_FILE;
#endif /* !UNIX */

    /* if redirection, ed_fp == stdin, else rewind the file just made */
    if (isoff(comp_current->send_flags, DIRECT_STDIN))
	(void) fseek(comp_current->ed_fp, comp_current->body_pos, 0);

    /* Read from stdin or the edfile till EOF and send it all to the mailer
     * and other open files/folders/programs. Check for "From " at the
     * beginnings of these lines to prevent creating new messages in folders.
     */
    while (fgets(buf, sizeof buf, comp_current->ed_fp))
	for (size = 0; size < next_file; size++) {
	    if (!files[size]) /* files[0] will be NULL if not calling MTA */
		continue;
	    if (size == log_file)
		continue;
	    if (def_fldr_type == FolderStandard) {
		/* XXX We shouldn't be using def_fldr_type here -- the output
		 *     folder may have different type than the current folder
		 */
		if (!comp_current->attachments && !strncmp(buf, "From ", 5))
		    (void) fputc('>', files[size]);
	    }
	    if (fputs(buf, files[size]) == EOF) {
		if (size == 0) {
		    error(SysErrWarning, catgets( catalog, CAT_MSGS, 624, "Lost connection to MTA" ));
		    dead_letter(-1);
		    goto Break; /* Two-level break */
		} else {
		    /* Drop this file, but continue writing others */
		    if (names[size]) {
			error(SysErrWarning, catgets( catalog, CAT_MSGS, 625, "Write failed: %s" ), names[size]);
			(void) close_lock(names[size], files[size]);
			xfree(names[size]);
		    } else {
			error(SysErrWarning, catgets( catalog, CAT_MSGS, 626, "Write to pipe failed" ));
		    if (fork_pid != 1)
			(void) fclose(files[size]); /* Don't mess with pclose() */
#ifndef MAC_OS
		    else		
			(void) pclose(files[size]); /* unless we never forked */
#endif /* !MAC_OS */
		    }
		    names[size] = NULL;
		    files[size] = NULL_FILE;
		}
	    }
	}
    Break:

    /* loop thru the open files (except for the first: the mail delivery agent)
     * and append a blank line so that ucb-mail can read these folders.
     * Then close the files.
     */
    for (size = 1; size < next_file; size++) {
	if (!files[size]) /* files[size] will be NULL on write failure */
	    continue;
	/* XXX We shouldn't be using def_fldr_type here -- the output
	 *     folder may have different type than the current folder
	 */
	if (def_fldr_type == FolderDelimited)
	    (void) fputs(msg_separator, files[size]); /* ENDING separator */
	if (names[size]) {
	    /* XXX We shouldn't be using def_fldr_type here -- the output
	     *     folder may have different type than the current folder
	     */
	    if (def_fldr_type == FolderStandard)
		(void) fputc('\n', files[size]);
	    if (close_lock(names[size], files[size]) == EOF)
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 627, "Warning: Close failed: %s" ), names[size]);
	    xfree(names[size]);
	} else {
	    if (files[size]) {
		if (fork_pid != 1)
		    (void) fclose(files[size]); /* Don't mess with pclose() */
#ifndef MAC_OS
		else		
		    (void) pclose(files[size]); /* unless we never forked */
#endif /* !MAC_OS */
	    }
	}
    }

    if (debug < 3) {
#ifndef UNIX
	int reply_code = do_sendmail(comp_current);
#else /* UNIX */
	int reply_code;
        if (!(value_of(VarSmtphost))) {
	   reply_code = files[0]? pclose(files[0]) : (MTA_EXIT << 8);
	   Debug("pclose reply_code = %d\n", reply_code);
	} else {
	   Debug("*** using remote sendmail...\n");
	   reply_code = do_sendmail(comp_current);
	}
#endif /* UNIX */
	if (fork_pid != 1)
	    rm_edfile((reply_code == (MTA_EXIT << 8))? 0 : -1);
	else
	    turnon(comp_current->send_flags, NEED_CLEANUP);
    } else if (fork_pid != 1)
	rm_edfile(0);
    else
	turnon(comp_current->send_flags, NEED_CLEANUP);

#ifndef ZM_CHILD_MANAGER
    if (oldchld)
	(void) signal(SIGCHLD, oldchld);
#endif /* ZM_CHILD_MANAGER */

    if (fork_pid == 1) {
	if (isoff(glob_flags, REDIRECT))
#ifdef QUEUE_MAIL
	if (queue_file == -1) {
	    print(catgets( catalog, CAT_MSGS, 628, "sent.\n" ));
	} else {
	    print(catgets(catalog, CAT_MSGS, 941, "queued for sending.\n"));
	    if (boolean_val(VarAlwaysSendSpooled))
	    	gui_error(HelpMessage, catgets (catalog, CAT_MSGS, 959, "Message queued for delivery. It will be sent when you reconnect."));
	    else gui_error(HelpMessage, catgets (catalog, CAT_MSGS, 960, "Message queued."));
	    if (ison(folder_flags, QUEUE_FOLDER))
		check_new_mail();
	}
#else /* QUEUE_MAIL */
	print(catgets(catalog, CAT_MSGS, 628, "sent.\n"));
#endif /* QUEUE_MAIL */
    }
#ifdef UNIX
    else _exit(0); /* Not a user exit -- a child exit */
#else
    mark_replies(comp_current);
    (void) fclose(comp_current->ed_fp);
    comp_current->ed_fp = NULL_FILE;
#endif /* UNIX */
    return 0;
}

/* ARGSUSED */
RETSIGTYPE
rm_edfile(sig)
int sig;
{
    if (sig > 0) {
	char *fix;
	if (ison(glob_flags, IGN_SIGS))
	    return;
	/* wrapcolumn may have been trashed -- restore it */
	if ((fix = value_of(VarWrapcolumn)) && *fix)
	    wrapcolumn = atoi(fix);
	mac_flush(); /* abort pending macros */
    }
    /* now check whether we should abort the letter */
    if ((sig == SIGINT || sig == SIGQUIT || sig == SIGTERM) &&
	    !istool && !killme && ison(glob_flags, IS_GETTING)) {
	if (signal(sig, rm_edfile) == SIG_ERR)
            printf("rm_edfile: couldn't re-set signal!\n");
	killme = 1;
	print(catgets( catalog, CAT_MSGS, 629,
                       "\n** interrupt -- one more to kill composition **\n" ));
#if defined(_XOPEN_SOURCE) || defined(_POSIX_C_SOURCE)
	siglongjmp(cntrl_c_buf, 1);
#else
	longjmp(cntrl_c_buf, 1);
#endif
    }
    /* if sig == -1, force a save into dead.letter.
     * else, check for nosave not being set and save anyway if it's not set
     * sig == 0 indicates normal exit (or ~x), so don't save a dead letter.
     */
    if (sig == -1 || sig != 0 && !boolean_val(VarNosave))
	dead_letter(sig);
    /* ed_fp may be null in GUI */
    if (isoff(comp_current->send_flags, DIRECT_STDIN) && comp_current->ed_fp)
	(void) fclose(comp_current->ed_fp), comp_current->ed_fp = NULL_FILE;
    if (comp_current->edfile) {
#ifdef UNIX
	/* If an external editor was left running, kill it. */
	if (istool && comp_current->exec_pid) {
	    (void) kill(comp_current->exec_pid, SIGHUP);
	    comp_current->exec_pid = 0;
	}
#endif /* UNIX */
	(void) unlink(comp_current->edfile);
    }
    if (comp_current->attachments)
	free_attachments(&comp_current->attachments, TRUE);
    stop_compose();
    if (sig == -1)
	return;

    if (sig == SIGHUP) 	/* Can this ever happen? */
	cleanup(0);
    if (sig != SIGINT && sig != SIGQUIT && sig != SIGTERM)
	return;

    if (sig == 0 || sig == -2 || istool) /* make sure sigchld is reset first */
	return;

    if (isoff(glob_flags, DO_SHELL)) {  /* If we're not in a shell, exit */
	echo_on();
	exit(1);
    }
    LongJmp(jmpbuf, 1);
}

#if defined( IMAP )
int
imap_write_draft(file, outbox_addresses)
char *file, *outbox_addresses;
{
    char 	buf[BUFSIZ];
    char	path[MAXPATHLEN];
    time_t 	t;			/* Greg: 3/11/93.  Was type long */
    FILE 	*draft;
    char 	*tmp;	
    int 	retval = 0;
    struct stat statbuf;
    char sfn[32] = "";

    if (!comp_current->ed_fp) {
	error(ZmErrWarning,
	    catgets(catalog, CAT_MSGS, 942, "Cannot save draft: No editor file for composition"));
	return -1;
    }

#if 0
    tmp = tmpnam(NULL);
#else
    strncpy(sfn, "/tmp/zmail.XXXXXX", sizeof(sfn));
    tmp = mktemp(sfn);
#endif

    if (!(draft = open_file(tmp, 0, TRUE)))
        return -1;

    (void) fflush(comp_current->ed_fp);
    (void) fseek(comp_current->ed_fp, comp_current->body_pos, 0);
    /* XXX We shouldn't be using def_fldr_type here -- the output
     *     folder may have different type than the current folder
     */
    if (def_fldr_type == FolderDelimited)
	(void) fputs(msg_separator, draft);
    else if (def_fldr_type == FolderStandard) {
	(void) time (&t);
	(void) fprintf(draft, "From %s %s", zlogin, ctime(&t));
    }
    if (outbox_addresses)
	(void) fprintf(draft, "%s: %s\n", ENVELOPE_HEADER, outbox_addresses);
    (void) output_headers(&comp_current->headers, draft, FALSE);
    (void) fputc('\n', draft);
    while (fgets(buf, sizeof(buf), comp_current->ed_fp))
	(void) fputs(buf, draft);
    /* XXX We shouldn't be using def_fldr_type here -- the output
     *     folder may have different type than the current folder
     */
    if (def_fldr_type == FolderDelimited)
	(void) fputs(msg_separator, draft);	/* ENDING separator */
    else if (def_fldr_type == FolderStandard)
	(void) fputc('\n', draft);

    if (*tmp != '|')
	(void) close_lock(tmp, draft);
#ifndef MAC_OS
    else
	(void) pclose(draft);
#endif /* !MAC_OS */

    draft = fopen( tmp, "r" );

    sprintf( path, "%s%s", current_folder->imap_prefix, file );

    if ( stat( tmp, &statbuf ) == -1 )  
	retval = -1;
    else if ( statbuf.st_size != 0 && zimap_writemsg( draft, path, 0, statbuf.st_size ) == 0 )
	retval = -1;

    fclose( draft );

    unlink( tmp );
    return retval;
}
#endif

int
write_draft(file, outbox_addresses)
char *file, *outbox_addresses;
{
    char 	buf[BUFSIZ];
    time_t 	t;			/* Greg: 3/11/93.  Was type long */
    FILE *draft;

    if (!comp_current->ed_fp) {
	error(ZmErrWarning,
	    catgets(catalog, CAT_MSGS, 942, "Cannot save draft: No editor file for composition"));
	return -1;
    }

    if (!(draft = open_file(file + (*file == '|'), (*file == '|'), TRUE)))
	return -1;
#ifdef MAC_OS
    gui_set_filetype(FolderFile, file, NULL);
#endif

    (void) fflush(comp_current->ed_fp);
    (void) fseek(comp_current->ed_fp, comp_current->body_pos, 0);
    /* XXX We shouldn't be using def_fldr_type here -- the output
     *     folder may have different type than the current folder
     */
    if (def_fldr_type == FolderDelimited)
	(void) fputs(msg_separator, draft);
    else if (def_fldr_type == FolderStandard) {
	(void) time (&t);
	(void) fprintf(draft, "From %s %s", zlogin, ctime(&t));
    }
    if (outbox_addresses)
	(void) fprintf(draft, "%s: %s\n", ENVELOPE_HEADER, outbox_addresses);
    (void) output_headers(&comp_current->headers, draft, FALSE);
    (void) fputc('\n', draft);
    while (fgets(buf, sizeof(buf), comp_current->ed_fp))
	(void) fputs(buf, draft);
    /* XXX We shouldn't be using def_fldr_type here -- the output
     *     folder may have different type than the current folder
     */
    if (def_fldr_type == FolderDelimited)
	(void) fputs(msg_separator, draft);	/* ENDING separator */
    else if (def_fldr_type == FolderStandard)
	(void) fputc('\n', draft);
    if (*file != '|')
	(void) close_lock(file, draft);
#ifndef MAC_OS
    else
	(void) pclose(draft);
#endif /* !MAC_OS */

    return 0;
}

/* save letter into dead letter */
void
dead_letter(sig)
int sig;	/* signal passed to rm_edfile() or 0 */
{
    char *p;

    if (ison(comp_current->send_flags, DIRECT_STDIN)) {
	print(catgets( catalog, CAT_MSGS, 630, "Input redirected -- cannot save dead letter.\n" ));
	print(catgets( catalog, CAT_MSGS, 631, "Your Mail Transport Agent may have saved it.\n" ));
	return;
    }
    /* if nothing was modified, don't save dead.letter */
    if (sig != -1 && isoff(comp_current->flags, MODIFIED))
	return;
    /* If the file doesn't exist, get outta here. File may not exist if user
     * generated a ^C from a promptable header and rm_edfile() sent us here.
     */
    if (!comp_current->ed_fp &&
	    (!comp_current->edfile || Access(comp_current->edfile, R_OK) != 0))
	return;
    /* User may have killed Z-Mail via a signal while he was in an editor.
     * ed_fp will be NULL in this case.  Since the file does exist (above),
     * open it so we can copy it to dead letter.
     *
     * We have a real problem here -- if Z-Mail died from a seg fault or other
     * memory corruption, it may not be possible to reload the headers from
     * the file.  (Of course, in that case, fopen() may not work either.)
     * First try to guess whether it's reasonable to reload the headers; if
     * that is likely to fail, turn off EDIT_HDRS before proceeding.  The
     * headers from the editor file may be duplicated, but at least won't be
     * lost, and when we *can* reload them we'll be doing the right thing.
     */
    if (!comp_current->ed_fp) {
	if (sig == SIGBUS || sig == SIGSEGV || (sig != 0 && errno == ENOMEM))
	    turnoff(comp_current->flags, EDIT_HDRS);
	if (reload_edfile() < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 632, "cannot save dead letter from %s" ),
		    comp_current->edfile);
	    return;
	}
    }
    /* don't save a dead letter if there's nothing to save. */
    if (fseek(comp_current->ed_fp, 0L, 2) || ftell(comp_current->ed_fp) <= 1L)
	return;
    if (!(p = value_of(VarDead)) || !*p)
	p = DEF_DEAD;
    if (write_draft(p, NULL) < 0)
	return;
    wprint(sig > 0
	   ? catgets( catalog, CAT_MSGS, 633, "Saved unfinished composition in %s.\n" )
	   : catgets( catalog, CAT_MSGS, 634, "Saved composition in %s.\n" ),
	   p);
}

int
prepare_edfile()
{
    char *name = NULL;
    FILE *ed_fp = NULL_FILE;
    long size, body_pos = 0;
    int ret = -1;

    on_intr();
    if (ison(comp_current->flags, EDIT_HDRS) ||
	    comp_current->body_pos > 0) {
	if (fseek(comp_current->ed_fp, 0L, 2) < 0) {
	    error(SysErrWarning, comp_current->edfile);
	    goto Return;
	}
	size = ftell(comp_current->ed_fp) - comp_current->body_pos;
	if (!(ed_fp = open_tempfile(EDFILE, &name)))
	    goto Return;
	else if (ison(comp_current->flags, EDIT_HDRS) &&
		    (output_headers(&comp_current->headers, ed_fp, 1) < 0 ||
		    fputc('\n', ed_fp) == EOF ||
		    (body_pos = ftell(ed_fp)) < 0) ||
		fp_to_fp(comp_current->ed_fp,
			comp_current->body_pos, -1L, ed_fp) < size) {
	    error(SysErrWarning, name);
	    xfree(name);
	    if (ed_fp)
	    	(void) fclose(ed_fp);
	    goto Return;
	}
	if (fclose(ed_fp) < 0) {
	    error(SysErrWarning, name);
	    xfree(name);
	    goto Return;
	}
	if (comp_current->ed_fp)
	    (void) fclose(comp_current->ed_fp);

	unlink(comp_current->edfile);
	xfree(comp_current->edfile);
	comp_current->edfile = name;
    } else if (comp_current->ed_fp)
	(void) fclose(comp_current->ed_fp);
    comp_current->ed_fp = NULL_FILE;
    comp_current->body_pos = body_pos;
    ret = 0;
Return:
    off_intr();
    return ret;
}

/* Truncate the edfile and set the seek position to the end. */
int
truncate_edfile(compose, new_size)
Compose *compose;
long new_size;
{
    int n;

    if (new_size < compose->body_pos) {
	if (new_size > 0)
	    error(ZmErrWarning, catgets( catalog, CAT_MSGS, 635, "Truncating message headers (probably ok)." ));
	compose->body_pos = new_size;
    }
#if defined(HAVE_FTRUNCATE) || defined(F_FREESP)
    (void) fseek(compose->ed_fp, new_size, L_SET);
    n = ftruncate(fileno(compose->ed_fp), new_size);
#else /* !(HAVE_FTRUNCATE || F_FREESP) */
#ifdef HAVE_CHSIZE
    (void) fseek(compose->ed_fp, new_size, L_SET);
    n = chsize(fileno(compose->ed_fp), new_size);
#else
    {
	char *newfile = NULL;
	FILE *new_fp = open_tempfile(EDFILE, &newfile);

	if (new_fp) {
	    on_intr();
	    if (new_size)
		n = fp_to_fp(compose->ed_fp, 0, new_size, new_fp);
	    else
		n = compose->body_pos = 0;
	    if (n < new_size) {
		if (new_fp)
		    (void) fclose(new_fp);
		xfree(newfile);
	    } else {
		n = 0;
		if (compose->ed_fp)
		    (void) fclose(compose->ed_fp);
		compose->ed_fp = new_fp;
		(void) unlink(compose->edfile);
		xfree(compose->edfile);
		compose->edfile = newfile;
	    }
	    off_intr();
	} else
	    n = -1;
    }
#endif /* HAVE_CHSIZE */
#endif /* !(HAVE_FTRUNCATE || F_FREESP) */
    return n;
}

int
open_edfile(compose)
Compose *compose;
{
    if (!compose)
	return -1;
    if (compose->ed_fp)	/* Already open (we presume) */
	return 0;
    if (!(compose->ed_fp = fopen(compose->edfile, "r+"))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 636, "Cannot reopen %s" ), compose->edfile);
	return -1;
    }
#if defined(MAC_OS) && defined(USE_SETVBUF)
    else (void) setvbuf(compose->ed_fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
    return 0;
}

int
close_edfile(compose)
Compose *compose;
{
    int ret = 0;

    if (!compose || !compose->ed_fp)
	return 0;
    if (compose->ed_fp) {	
	if (fclose(compose->ed_fp) == EOF)
	    ret = -1;
    }
    compose->ed_fp = NULL_FILE;
    /* compose->body_pos = 0; */
    return ret;
}

int
parse_edfile(compose)
Compose *compose;
{
    if (!compose || !compose->ed_fp)
	return -1;
    if (ison(compose->flags, EDIT_HDRS)) {
	free_headers(&compose->headers);
	/* Let the editor file determine the state of RETURN_RECEIPT */
	turnoff(compose->send_flags, RETURN_RECEIPT+NO_RECEIPT);
	if (generate_headers(compose, compose->ed_fp, 1) < 0)
	    return -1;
    } else
	compose->body_pos = 0;
    return 0;
}

int
reload_edfile()
{
    if (open_edfile(comp_current) < 0 || parse_edfile(comp_current) < 0) {
	if (!istool)
	    stop_compose();
	return -1;
    }
    (void) fseek(comp_current->ed_fp, 0L, 2);
    return 0;
}

void
invoke_editor(edit)
const char *edit;
{
#ifdef VUI
    if (istool == 2) {
	EnterScreencmd(1);
	++spIm_LockScreen;
    }
    TRY {
#endif /* VUI */
	if (edit_file(comp_current->edfile, edit, !edit) == 0)
	    turnoff(comp_current->flags, EDIT);
#ifdef VUI
    } FINALLY {
    if (istool == 2) {
	--spIm_LockScreen;
	ExitScreencmd(0);
    }
    } ENDTRY;
#endif /* VUI */
}

int
run_editor(edit)
const char *edit;
{
    if (prepare_edfile() < 0)
	return 0;

    invoke_editor(edit);

    if (reload_edfile() < 0)
	return -1;
    /* upon exit of editor, user must now type eofc or "." to send */
    wprint(catgets( catalog, CAT_MSGS, 637, "(continue editing message or ^%c to send)\n" ), eofc + '@');
    return 0;
}

int
init_attachment(attachment)
    const char *attachment;
{
    char *type;
    const char *file;
    const char *delimit;
    const char *message;

    /* Attachment can be of the form type:file */
    if (delimit = index(attachment, ':')) {
	type = strncpy((char *)malloc(delimit - attachment + 1), attachment, 
		       delimit - attachment);
	*(type+(delimit-attachment)) = '\0';
	file = delimit + 1;
    } else {
	type = NULL;
	file = attachment;
    }
    message = add_attachment(comp_current, file, type, NULL, NULL, NO_FLAGS, NULL);
    xfree(type);
    
    if (message) {
	if (istool && isoff(comp_current->send_flags, SEND_NOW))
	    error(UserErrWarning, message);
	return -1;
    }
    return 0;
}

#ifdef UUCP_SUPPORT
static uucpsend_t usend;
#endif /* UUCP_SUPPORT */

FILE *
do_start_sendmail(compose, addr_list)
Compose *compose;
char *addr_list;
{
    char **addrv;
    int i = -1;
    char tbuf[MAXPATHLEN];
    FILE *fp;

    /*
     * pf Sat Feb 19 16:17:38 1994
     * XXX XXX XXX temporary, last-minute hack to get UUCP working.
     * We're going to fix all this code the right way very soon.  If
     * this code still exists on April 19, 1995, please kill me.
     */
    char *uucp_root = value_of(VarUucpRoot);

    if (!(addrv = addr_vec(addr_list)))
	return NULL_FILE;
    if (ZmUsingUUCP()) {
#ifdef UUCP_SUPPORT
	usend = uucpsend_Start(uucp_root, addrv);
	if (usend) {
	    uucpsend_SetBodyFileName(usend, compose->edfile);
	    return uucpsend_GetHdrFilePtr(usend);
	}
#else /* !UUCP_SUPPORT */
	i = -1;
#endif /* !UUCP_SUPPORT */
    } else
	i = setup_sendmail(addrv);
    free_vec(addrv);
    if (i < 0)
	return NULL_FILE;
    else {
	sprintf(tbuf, "%sh", compose->edfile);
	fp = fopen(tbuf, "w");
#if defined(MAC_OS) && defined(USE_SETVBUF)
	if (fp)
	    (void) setvbuf(fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
	return (fp);
    }

} /* do_start_sendmail */


int do_sendmail(compose)
Compose *compose;
{
    char tbuf[MAXPATHLEN];
    FILE *hdr_fp;
    int i = -1;

#ifdef UUCP_SUPPORT
    if (ZmUsingUUCP())
	return uucpsend_Finish(usend) ? 0 : -1;
#endif /* UUCP_SUPPORT */
    if (compose && !fseek(compose->ed_fp, compose->body_pos, SEEK_SET)) {
	sprintf(tbuf, "%sh", compose->edfile);
	hdr_fp = fopen(tbuf, "r");
	if (hdr_fp) {
	    i = sendmail(hdr_fp, compose->ed_fp);
	}
    }
    return i;

} /* do_sendmail */

#ifdef QUEUE_MAIL
/*
 * write a copy of the message to the queue file, if we're in
 * disconnected mode.  Returns the queue file index, or -1 if
 * we failed.
 */
static int
open_queue_file(val, names, files)
int *val;
char **names;
FILE **files;
{
    char *qname = value_of(VarMailQueue);
    FILE *f;

    if (!qname)
	return -1;
    f = open_file(qname, False, True);
    if (!f) {
	error(SysErrWarning, catgets(catalog, CAT_MSGS, 943, "Can't open mail queue file: %s"),
	      qname);
	return -1;
    }
#ifdef MAC_OS
    gui_set_filetype(FolderFile, qname, NULL);
#endif
    names[*val] = savestr(qname);
    names[*val+1] = NULL;
    files[*val] = f;
    files[*val+1] = NULL_FILE;
    return (*val)++;
}
#endif /* QUEUE_MAIL */
