/* options.c    Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "zmcomp.h"
#include "options.h"
#include "glob.h"
#include "catalog.h"
#include "fetch.h"
#include <stdio.h>

#ifndef LICENSE_FREE
# ifndef MAC_OS
#include "license/server.h"
# else
#include "server.h"
# endif /* !MAC_OS */
#endif /* LICENSE_FREE */

#ifdef MOTIF
#include "zm_motif.h"	/* for {offer,accept}_{folder,command} */
#endif /* MOTIF */

/*
 * NOTE:  Any word flag which is a prefix of another word flag must be
 *  listed AFTER the flag it prefixes in the list below
 *
 * Letters still unused:
 *	a    (j) l oqw(x)yz
 *	 B(G)(J)K MOQW   Y
 */

char *word_flags[][2] = {
    { "-acceptCommands","-4" },
    { "-acceptFolders",	"-1" },
    { "-activate",	"-k" },	/* "key" */
    { "-attach",	"-A" },
    { "-bcc",		"-b" },
    { "-blindcarbon",	"-b" },
    { "-blind",		"-b" },
    { "-carbon",	"-c" },
    { "-cc",		"-c" },
    { "-command",	"-e" },
    { "-copy",		"-c" },
    { "-curses",	"-V" },
    { "-debug",		"-d" },
    { "-direct",	"-D" },
    { "-draft",		"-h" },
    { "-echo",		"-E" },
    { "-evaluate",	"-e" },
    { "-eval",		"-e" },
    { "-execute",	"-e" },
    { "-exec",		"-e" },
    { "-file",		"-F" },	/* Don't really like -file for -F */
    { "-filter",	"-j" },	/* XXX This should change */
    { "-folder",	"-f" },	/* Maybe -file should become -f too? */
    { "-fullscreen",	"-V" },	/* Don't interpret as -f -u -l -s -c -r -e -n */
    { "-gui",		"-g" },
    { "-headerfile",	"-h" },
    { "-headers",	"-H" },
    { "-help",		"-?" },
    { "-hostinfo",	"-\010" }, /* Ugh, ctrl-H */
    { "-initialize",	"-I" },
    { "-init",		"-I" },
    { "-interactive",	"-i" },
    { "-interact",	"-i" },
    { "-key",		"-k" },
    { "-library",	"-L" },
    { "-lib",		"-L" },
    { "-licensekey",	"-k" },
    { "-licenseserver",	"-Z" },
    { "-mailbox",	"-m" },
    { "-message",	"-h" },
#ifdef MOTIF
    { "-motif",		"-g" },
#endif /* MOTIF */
    { "-offerCommands", "-3" },
    { "-offerFolders",  "-0" },
#ifdef OLIT
    { "-ol",		"-t" },
    { "-openlook",	"-t" },
#endif /* OLIT */
    { "-netserver",	"-Z" },
    { "-nls",		"-Z" },
    { "-nofilter",	"-J" }, /* XXX This should change */
    { "-noheaders",	"-N" },
    { "-noindex",	"-X" },
    { "-noinitialize",	"-n" },
    { "-noinit",	"-n" },
    { "-notempfile",	"-D" },
    { "-picky",		"-P" },
    { "-plate",		"-p" },
    { "-readonly",	"-r" },
    { "-receive",	"-G" },	/* XXX "Get" mail from stdin */
    { "-refuseCommands","-5" },
    { "-refuseFolders", "-2" },
    { "-register",	"-R" },
    { "-send",		"-U" },
    { "-server",	"-Z" },
    { "-shell",		"-S" },
    { "-source",	"-F" },	/* This is better for -F */
    { "-subject",	"-s" },
    { "-template",	"-p" },
    { "-timeout",	"-T" },
    { "-tool",		"-t" },
    { "-unedited",	"-U" },
    { "-user",		"-u" },
    { "-verbose",	"-v" },
    { "-version",	"-\026" },	/* Ugh, ctrl-V */
    { "-visual",	"-V" },
    { "-zcnlserv",	"-Z" },
    { "-zmlib",		"-L" },
    { NULL,		NULL }	/* This must be the last entry */
};

typedef struct option_pair
{
  char *option;
  catalog_ref description;
} option_pair;

option_pair use_strs[] = {
/*  { "-acceptCommands", NULL }, */
    { "-acceptFolders",	catref(CAT_SHELL, 878, "act as server for requests to open folders") },
    { "-attach [type:]file", catref( CAT_SHELL, 526, "attach a file of the optional type" ) },
    { "-bcc \"blind-carbon addresses\"", catref( CAT_SHELL, 527, "specify blind-carbon recipients" ) },
    { "-cc \"carbon-copy addresses\"", catref( CAT_SHELL, 528, "specify carbon-copy recipients" ) },
    { "-debug", catref( CAT_SHELL, 529, "activate verbose debugging output" ) },
    { "-direct", catref( CAT_SHELL, 530, "pass redirected input direct to MTA" ) },
    { "-draft draft-file", catref( CAT_SHELL, 531, "read a prepared draft of a message" ) },
    { "-echo", catref( CAT_SHELL, 532, "disable cbreak mode tty processing" ) },
    { "-eval[!] \"command\"", catref( CAT_SHELL, 533, "execute the given Z-Mail command" ) },
    { "-filter", catref( CAT_SHELL, 534, "apply message filters on startup" ) },
    { "-folder [folder]", catref( CAT_SHELL, 535, "open the named folder on startup" ) },
#ifdef CURSES
    { "-fullscreen", catref( CAT_SHELL, 536, "start Z-Mail in fullscreen mode" ) },
#endif /* CURSES */
    { "-gui", catref( CAT_SHELL, 537, "start Z-Mail in gui (X Window) mode" ) },
    { "-headers[:adfmnoprsu]", catref( CAT_SHELL, 538, "display header summaries only" ) },
    { "-help", catref( CAT_SHELL, 539, "print this usage message" ) },
    { "-init[!] init-file", catref( CAT_SHELL, 540, "initialize from the named file" ) },
    { "-interact", catref( CAT_SHELL, 541, "force interactive mode" ) },
    { "-lib library-directory", catref( CAT_SHELL, 542, "specify Z-Mail library location" ) },
    { "-mailbox system-mailbox", catref( CAT_SHELL, 543, "specify system mailbox location" ) },
#ifdef MOTIF
    { "-motif", catref( CAT_SHELL, 544, "start Z-Mail as Motif application" ) },
#endif /* MOTIF */
/*  { "-offerCommands", NULL }, */
    { "-offerFolders", catref(CAT_SHELL, 874, "request that another Z-Mail open my folders") },
#ifdef OLIT
    { "-openlook", catref( CAT_SHELL, 545, "start Z-Mail as OpenLook application" ) },
#endif /* OLIT */
    { "-nofilter", catref( CAT_SHELL, 546, "do not apply message filters" ) },
    { "-noheaders", catref( CAT_SHELL, 547, "do not display header summaries" ) },
    { "-noinit[!]", catref( CAT_SHELL, 548, "do not read initialization files" ) },
    { "-picky", catref( CAT_SHELL, 549, "do not supply From: or Date: to MTA" ) },
    { "-readonly", catref( CAT_SHELL, 550, "do not allow updates to the folder" ) },
    { "-receive", catref( CAT_SHELL, 551, "receive mail on stdin and filter it" ) },
/*  { "-refuseCommands", NULL }, */
    { "-refuseFolders", catref(CAT_SHELL, 875, "do not act as server for folder requests") },
    { "-register[!] password [users]", catref( CAT_SHELL, 552, "register host's license information" ) },
    { "-send[!]", catref( CAT_SHELL, 553, "send immediately (with -draft)" ) },
    { "-shell[!]", catref( CAT_SHELL, 554, "run shell even if there is no mail" ) },
    { "-source script-file", catref( CAT_SHELL, 555, "read commands from the named file" ) },
    { "-subject \"subject-string\"", catref( CAT_SHELL, 556, "specify message subject" ) },
    { "-template template-name", catref( CAT_SHELL, 557, "read a template for a message" ) },
    { "-timeout seconds", catref( CAT_SHELL, 558, "time between mail checks (GUI mode)" ) },
    { "-user [user]", catref( CAT_SHELL, 559, "set user name to given user" ) },
#ifdef VERBOSE_ARG
    { "-verbose", catref( CAT_SHELL, 560, "MTA sends messages verbosely" ) },
#endif /* VERBOSE_ARG */
    { "-version", catref( CAT_SHELL, 561, "print version information and exit" ) },
    { "-zcnlserv host:port", catref( CAT_SHELL, 562, "find license server on host at port" ) },
    { NULL, catref( CAT_SHELL, 0, "" ) }
};

void
fix_word_flag(argp, words)
    register char **argp;
    register char *(*words)[2];
{
    int i;

    Debug("%s --> ", *argp);
    for (i = 0; words[i][0]; i++) {
	int len = strlen(words[i][0]);
	if (! strncmp(*argp, words[i][0], len)) {
	    char buf[BUFSIZ], *p = buf;
	    p += Strcpy(buf, words[i][1]);
	    (void) strcpy(p, *argp + len);
	    (void) bzero(*argp, strlen(*argp));
	    (void) strcpy(*argp, buf);
	}
    }
    Debug("%s\n", *argp);
}

#ifdef MAC_OS
# include "zminit.seg"
#endif /* MAC_OS */
/*
 * preparse the command line to determine whether or not we're going
 * to bail out after checking that the user has no mail.  Also, check
 * to see if we're going to run a tool because it must be built first.
 */
void
preparse_opts(argcp, argv)
    int *argcp;
    char **argv;
{
    char **args;

#ifdef GUI
# if defined(VUI) || defined(_WINDOWS) || defined(MAC_OS)
#  ifdef MAC_OS
    istool = 2;
#  else /* !MAC_OS */
    istool = 1;
#  endif /* !MAC_OS */
# else /* !(VUI || _WINDOWS || MAC_OS) */
#  ifdef GUI_ONLY
    istool = 1;
#  else /* !GUI_ONLY */
    if (istool = (prog_name[0] == 'x'))
#  endif /* !GUI_ONLY */
    {
	turnon(glob_flags, DO_SHELL);
	parse_tool_opts(argcp, argv);
    }
# endif /* !(VUI || _WINDOWS || MAC_OS) */
#endif /* GUI */

    if (*argcp > 1) {
	for (args = argv+1; *args && args[0][0] == '-'; args++) {
	    int next = 1;
	    /* NOTE: "xzmail -t" may not work as expected ... */
	    fix_word_flag(&args[0], word_flags);
DoNext:
#ifdef VUI
	    switch (args[0][next]) {
	      case '?':		/* "-help" */
	      case 'A':		/* "-attach" */
	      case 'D':		/* "-direct" */
	      case 'E':		/* "-echo" */
	      case 'G':		/* "-receive" */
	      case 'H':		/* "-headers" */
	      case '\010':	/* "-hostinfo" */
	      case 'N':		/* "-noheaders" */
	      case 'R':		/* "-register" */
	      case 'S':		/* "-shell" */
	      case 'U':		/* "-unedited" */
	      case '\026':	/* "-version" */
	      case 'b':		/* "-blindcarbon" */
	      case 'c':		/* "-copy" */
	      case 'h':		/* "-message" */
	      case 'p':		/* "-template" */
	      case 's':		/* "-subject" */
	      case 'v':		/* "-verbose" */
		istool = 0;
		break;
	    }
#endif /* VUI */
	    switch (args[0][next]) {
#if defined(GUI) || defined(VUI)
	      case 'T' :
		  if (args[1])
		      args++;
		case 't' : case 'g' :
		    if (!istool) {
			/* Note: we won't ever get here if started as
			 * "xzmail" because istool is already true.
			 */
#ifndef MAC_OS
			istool = 1;
#else /* MAC_OS */
			istool = 2; /* we're all ready for gui by now */
#endif /* !MAC_OS */
			/* Before parsing tool opts, we have to take out
			 * the -t, because the window system may interpret
			 * it as introducing a title.  This fails if the
			 * program was invoked as "xzmail" because
			 * parse_tool_opts() has already been called.
			 */
			if (next == 1 && !args[0][2]) {
			    int n;

			    for (n = 0; args[n] = args[n+1]; n++)
				;
			    *argcp -= 1;
			} else if (args[0][next] == 't') {
			    int n;

			    for (n = next; args[0][n] = args[0][n+1]; n++)
				;
			}
#if (!defined(VUI) && !defined (_WINDOWS))
			parse_tool_opts(argcp, argv);
#endif /* VUI */
			turnon(glob_flags, DO_SHELL);
			/* Start over because argv may have been changed */
			args = argv;	/* incremented by loop */
			next = 0;	/* end the current word */
		    }
		    break;
#endif /* GUI || VUI */
#ifndef LICENSE_FREE
		case 'R' : /* register */
		    /* don't kick him out if he's not registered.  He's going
		     * to register himself later in parse_options().
		     */
		    turnoff(license_flags, FULL_LICENSE);
		    return;	/* Remaining flags go to ls_register */
#endif /* LICENSE_FREE */
		case 'S' :
		    turnon(glob_flags, DO_SHELL);
		case 'G' : /* Get (filter stdin) */
		case 'j' : /* filter */
		case '?' : /* help */
		    break;
		case 'e' :
		    turnon(glob_flags, DO_SHELL);
		case 'f' :
		case 'F' :
		case 'h' :
		case 'p' :
		case 'u' :
		case 'A' :
		case 'b' :
		case 'c' :
		case 'I' :
		case 's' :
		    if (args[1]) {
			args++;
			next = 0;
		    }
		    break;
		case 'H' :
		    if (args[0][next+1] == ':')
			next = 0;
		    break;
		case 'm' :
		    if (args[1])
			ZSTRDUP(spoolfile, *++args);
		    else
			print(catgets( catalog, CAT_SHELL, 563, "-m: missing mailbox name.\n" )), exit(1);
		    next = 0;
		    break;
		case 'L':
		    /* This and Z are the only options that MUST be
		     * fully processed before init() is called.
		     */
		    if (args[1])
			init_lib(*++args);
		    else
			puts(catgets( catalog, CAT_SHELL, 564, "-L: specify filename for library." )), exit(1);
		    /* Fall through */
		case '\0':
		    next = 0;
		default :
		    break;
		case 'd': debug = 1;
		    break;
		case '\010' : /* hostinfo -- bleah */
		    zm_hostinfo();
		    break;
		case '\026' : /* version -- bleah */
		    zm_version();
		    break;
		case 'Z':
#ifndef LICENSE_FREE
		    if (args[1]) {
			char *z = *args;
			*args = "ZCNLSERV";
			(void) Setenv(3, args - 1);
			*args++ = z;
		    } else
			puts(catgets( catalog, CAT_SHELL, 565, "-Z: specify serverhost:portnumber." )), exit(1);
		    next = 0;
#endif /* LICENSE_FREE */
		    break;
	    }
	    if (next) {
		++next;
		goto DoNext;
	    }
	}
	if (*args) {  /* unused args indicates sending mail to someone */
	    if (!istool)
		turnon(glob_flags, IS_SENDING);
	}
    }
}

static void
usage(brief)
int brief;
{
    option_pair *p;

    print(zmVaStr(catgets( catalog, CAT_SHELL, 566, "Usage:\n\
  %s [-verbose] [-send] [-direct] [-draft file] [-attach [type:]file]\n\
        [-subject \"subject\"] [-bcc bcc-list] [-cc cc-list] to-list\n\
  %s" ), prog_name,
#ifdef GUI
    catgets( catalog, CAT_SHELL, 567, "%s [-gui | -noheaders] [-readonly] [-folder [folder]]\n" )
#else
#ifdef CURSES
    "%s [-fullscreen | -noheaders] [-readonly] [-folder [folder]]\n"
#else
    catgets( catalog, CAT_SHELL, 569, "%s [-noheaders] [-readonly] [-folder [folder]]\n" )
#endif /* CURSES */
#endif /* GUI */
    ), prog_name);
    if (brief) {
	print(catgets( catalog, CAT_SHELL, 570, "\nType %s -help for additional options and descriptions.\n" ),
	    prog_name);
	exit(1);
    }
    print(catgets( catalog, CAT_SHELL, 571, "\nOther options:\n" ));
    for (p = use_strs; p->option; p++)
	print("    %-38s%s\n", p->option, catgetref(p->description));
    print("\n%s\n",
catgets( catalog, CAT_SHELL, 572, "Most options have a one-character abbreviation that may not be the same as\n\
the first letter of the option word.  These abbreviations may be clustered,\n\
so beware of typographical errors when typing option names.  Many options\n\
are also recognized by other descriptive names; this list is not complete.\n\
See your Z-Mail documentation for a list of option abbreviations and names." ));
    exit(0);
}

void
parse_options(argvp, flags)
    char ***argvp;
    struct zc_flags *flags;
{
    char buf[MAXPATHLEN];

    bzero((char *) flags, sizeof (struct zc_flags));
    flags->source_rc = TRUE;
    flags->folder = flags->draft = flags->attach = NULL;
    flags->src_cmds = DUBL_NULL;

    for (++(*argvp); **argvp && ***argvp == '-'; (*argvp)++) {
	int look_again;
DoLookAgain:
	look_again = TRUE;
	switch ((*argvp)[0][1]) {
	    case 'A':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 573, "-attach: bad option when using GUI mode" )), exit(1);
		else if ((*argvp)[1])
		    flags->attach = *++(*argvp);
		else
		    usage(1);
		turnon(glob_flags, IS_SENDING);
		look_again = FALSE;
	    when 'b':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 574, "-bcc: bad option when using GUI mode" )), exit(1);
		else if ((*argvp)[1])
		    flags->Bcc = *++(*argvp);
		else
		    usage(1);
		turnon(glob_flags, IS_SENDING);
		look_again = FALSE;
	    when 'c':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 575, "-cc: bad option when using GUI mode" )), exit(1);
		else if ((*argvp)[1])
		    flags->Cc = *++(*argvp);
		else
		    usage(1);
		turnon(glob_flags, IS_SENDING);
		look_again = FALSE;
	    when 'D':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 576, "-direct: bad option when using GUI mode" )), exit(1);
		else if (ison(glob_flags, REDIRECT))
		    turnon(flags->flg, DIRECT_STDIN);
	    when 'd': debug = 2;
	    when 'e':
		flags->src_n_exit |= ((*argvp)[0][2] == '!')? 2 : 0;
		if ((*argvp)[1]) {
		    if (vcat(&flags->src_cmds, unitv(*++(*argvp))) < 0)
			(void) puts(catgets( catalog, CAT_SHELL, 577, "-e: out of memory" )), exit(1);
		} else
		    (void) puts(catgets( catalog, CAT_SHELL, 578, "-e: no command to execute" )), exit(1);
		look_again = FALSE;
	    when 'F':
		flags->src_n_exit |= ((*argvp)[0][2] == '!')? 1 : 0;
		if (!(flags->src_file = *++(*argvp)) || !*(flags->src_file))
		    puts(catgets( catalog, CAT_SHELL, 579, "-F: specify filename to source" )), exit(1);
		if (fullpath(strcpy(buf, flags->src_file), FALSE))
		    flags->src_file = savestr(buf);
		look_again = FALSE;
		/* fall thru! */
	    case 'N':
		(void) strcat(flags->f_flags, "-N ");
	    when 'f':
		if (flags->folder)
		    puts(catgets( catalog, CAT_SHELL, 580, "You cannot specify more than one mailbox" )), exit(1);
		if ((*argvp)[1]) {
		    flags->folder = savestr(*++(*argvp));
		    if (!strcmp(flags->folder, "-")) {
			flags->src_n_exit = TRUE;
			flags->filter_mode = TRUE;
		    }
		    look_again = FALSE;
		} else
		    flags->folder = savestr("&");
	    when 'H':
		if (*(hdrs_only = (*(*argvp))+2) != ':')
		    hdrs_only = ":a";
		else
		    look_again = FALSE;
		if (istool) {
		    puts(catgets( catalog, CAT_SHELL, 581, "-H: option ignored in GUI mode." ));
		    hdrs_only = NULL;
		    break;
		}
		turnoff(glob_flags, PRE_CURSES);
		/* read only cuz no updates */
		(void) strcat(flags->f_flags, "-N -r ");
		/* fall through */
	    case 'E':
		/*
		 * don't set tty modes -- e.g. echo and cbreak modes aren't
		 * changed.
		 */
		turnon(glob_flags, ECHO_FLAG);
	    when 'p':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 582, "-template: bad option when using GUI mode" )), exit(1);
		flags->use_template = TRUE;
		/* Fall through */
	    case 'h':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 583, "-draft: bad option when using GUI mode" )), exit(1);
		if ((*argvp)[1])
		    flags->draft = *++(*argvp);
		else
		    usage(1);
		turnon(glob_flags, IS_SENDING);
		look_again = FALSE;
	    when 'I':
		if ((*argvp)[0][2] == '!' && flags->source_rc > 0)
		    flags->source_rc = 0;	/* Only ~/.zmailrc sourced */
		if (!(flags->init_file = *++(*argvp)))
		    puts(catgets( catalog, CAT_SHELL, 584, "-I: specify filename for init." )), exit(1);
		if (fullpath(strcpy(buf, flags->init_file), FALSE))
		    flags->init_file = savestr(buf);
		look_again = FALSE;
	    when 'i':
		/* force interactive even if !isatty(0) */
#ifdef GUI_ONLY
		if (!istool)
		    puts(catgets(catalog, CAT_SHELL, 914, "-interactive: shell interface is not supported in this version")), exit(1);
#else /* !GUI_ONLY */
		turnoff(glob_flags, REDIRECT);
#endif /* !GUI_ONLY */
	    when 'G':	/* should change */
		if (flags->folder)
		    xfree(flags->folder);
		flags->folder = savestr("-");
		flags->src_n_exit = TRUE;
		flags->filter_mode = TRUE;
		/* Fall through */
	    case 'j':	/* should change */
		(void) strcat(flags->f_flags, "-f "); /* folder() argument */
	    when 'J':	/* should change */
		(void) strcat(flags->f_flags, "-F "); /* folder() argument */
	    when 'L': case 'm': case 'Z':	/* processed in preparse_opts */
		++(*argvp);
		look_again = FALSE;
	    when 'n':
		if ((*argvp)[0][2] == '!') {
		    ++(**argvp);
		    flags->source_rc = -1;	/* No init files sourced */
		} else
		    flags->source_rc = 0;	/* Only ~/.zmailrc sourced */
	    when 'P':
		turnon(glob_flags, PICKY_MTA);
	    when 'r':
		(void) strcat(flags->f_flags, "-r "); /* folder() argument */
	    when 'R':
#ifdef LICENSE_FREE
		print(catgets( catalog, CAT_SHELL, 585, "No password required.\n" ));
		exit(0);
#else /* LICENSE_FREE */
		if (isoff(glob_flags, REDIRECT)) {
		    turnon(glob_flags, ECHO_FLAG);
		    tty_settings();
		    (void) set_var(VarPager, "=", "internal");
		    if (help(HelpNoComplain, "SLA", cmd_help) != 0 &&
			    help(0, "License.txt", zmlibdir) != 0)
			exit(1);
		}
		/* Can't register and do something else too */
		exit(ls_register(*argvp));
#endif /* LICENSE_FREE */
	    when 's':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 586, "-subject: bad option when using GUI mode" )), exit(1);
		else if ((*argvp)[1])
		    flags->Subj = *++(*argvp);
		else
		    usage(1);
		turnon(glob_flags, IS_SENDING);
		look_again = FALSE;
		break; /* Prevent fallthrough regardless of GUI defined */
#if defined(GUI) || defined(VUI)
	    case 'T':
		if ((passive_timeout = atoi(*++(*argvp))) < MIN_PASSIVE_TIMEOUT)
		    passive_timeout = MIN_PASSIVE_TIMEOUT;
		look_again = FALSE;
		/* -T implies -t */
	    case 't':
# ifndef MAC_OS
	    	istool = 1; /* fall through */
# else
	    	istool = 2; /* fall through */
# endif /* !MAC_OS */
#endif /* GUI || VUI */
	    case 'S':
#ifdef GUI_ONLY
		if (!istool)
		    puts(catgets(catalog, CAT_SHELL, 915, "-shell: shell interface is not supported in this version")), exit(1);
#else /* !GUI_ONLY */
		turnon(glob_flags, DO_SHELL);
		if ((*argvp)[0][2] == '!') {
		    turnoff(glob_flags, IS_SENDING);
		    ++(**argvp);
		}
#endif /* !GUI_ONLY */
	    when 'U':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 587, "-send: bad option when using GUI mode" )), exit(1);
		turnon(flags->flg, SEND_NOW);
		if ((*argvp)[0][2] == '!') {
		    turnon(flags->flg, NO_SIGN);
		    ++(**argvp);
		}
	    when 'u': /* specify a user's mailbox */
		if (flags->folder)
		    puts(catgets( catalog, CAT_SHELL, 580, "You cannot specify more than one mailbox" )), exit(1);
#ifdef HOMEMAIL
		{
		    char *p;
		    int isdir = 1;
		    (void) sprintf(buf, "%%%s",
				(*argvp)[1] ? (*argvp)[1] : "root");
		    if ((p = getpath(buf, &isdir)) && !isdir)
			flags->folder = savestr(p);
		    else if (isdir < 0)
			print("%s: %s\n", buf, p), exit(1);
		    else if (isdir)
			print(catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), p), exit(1);
		}
#else /* HOMEMAIL */
		sprintf(buf, "%s%c%s",
			spooldir, SLASH, ((*argvp)[1])? (*argvp)[1] : "root");
		flags->folder = savestr(buf);
#endif /* HOMEMAIL */
		if ((*argvp)[1]) {
		    init_user(*++(*argvp));
		} else
		    init_user("root");
		look_again = FALSE;
#ifdef CURSES
	    when 'V': case 'C':
		/* don't init curses -- don't even set iscurses. */
		if (istool) {
		    puts("-fullscreen: already running in GUI mode");
		    turnoff(glob_flags, PRE_CURSES);
		} else if (hdrs_only)
		    puts("headers only: ignoring -fullscreen flag");
		else
		    turnon(glob_flags, PRE_CURSES);
#endif /* CURSES */
#ifdef VERBOSE_ARG
	    when 'v':
		if (istool)
		    puts(catgets( catalog, CAT_SHELL, 592, "-verbose: bad option when using GUI mode" )), exit(1);
		turnon(flags->flg, VERBOSE);
#endif /* VERBOSE_ARG */
	    when '\026':
	    case '\010':
		if (isoff(glob_flags, DO_SHELL))
		    exit(0);
	    when 'X':
		(void) strcat(flags->f_flags, "-X "); /* folder() argument */
	    when '\0': look_again = FALSE;
#ifdef MOTIF
	    when '0':
		if (!istool)
		    puts(catgets(catalog, CAT_SHELL, 876, "Warning: Only GUI Z-Mail can make requests of a folder server"));
		else
		    offer_folder = 1;
	    when '1':
		if (!istool)
		    puts(catgets(catalog, CAT_SHELL, 877, "Warning: Only GUI Z-Mail can act as a folder server"));
		else
		    accept_folder = 1;
	    when '2':
		accept_folder = 0;
	    when '3':
		offer_command = 1;
	    when '4':
		accept_command = 1;
	    when '5':
		accept_command = 0;
#else /* !MOTIF */
	    when '0': case '1': case '2': case '3': case '4': case '5':
		/* Nothing */
#endif /* MOTIF */
	    otherwise:
		print(catgets( catalog, CAT_SHELL, 593, "%s: unknown option: `%c'\n" ), prog_name,
		    (*argvp)[0][1]? (*argvp)[0][1] : '-');
	    case '?':
		usage(0);
	}
	/* XXX  This used to say:
	 *
	 *  if (look_again && ++(**argvp) != '\0')
	 *
	 * I can't imagine that was what was meant ....
	 */
	if (look_again && *++(**argvp) != '\0')
	    goto DoLookAgain;
    }

    if (ison(flags->flg, SEND_NOW) && !flags->draft) {
	print(catgets( catalog, CAT_SHELL, 594, "You must specify a draft file to autosend.\n" ));
	exit(1);
    }

    if (**argvp && isoff(glob_flags, IS_SENDING)) {
	char **av = unitv(prog_name);
	int ac = vcat(&av, vdup(*argvp));
	(void) push_args(ac, av, NULL_GRP);
	turnon(zmfunc_args->o_flags, ZFN_INIT_ARGS);
    }
}
