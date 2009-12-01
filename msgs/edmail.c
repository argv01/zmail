/* edmail.c	Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	edmail_rcsid[] = "$Id: edmail.c,v 2.151 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "attach.h"
#include "catalog.h"
#include "child.h"
#include "cmdtab.h"
#ifdef VUI
#include "dialog.h" /* to circumvent "unreasonable include nesting" */
#endif /* VUI */
#include "dirserv.h"
#include "dynstr.h"
#include "edmail.h"
#include "fsfix.h"
#include "hashtab.h"
#include "pager.h"
#include "partial.h"	/* For compose_guess_size() */
#include "refresh.h"
#include "strcase.h"
#include "zmcomp.h"
#include "zmstring.h"

#if defined( IMAP ) && defined( MOTIF )
#include "../motif/finder.h"
#endif

extern zmInterpose **fetch_interposer_list();

/* This array contains formats to be passed to sprintf()
 */
catalog_ref tilde_commands[] = {
    catref( CAT_MSGS, 253, "%ccommands: [OPTIONAL argument]\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 255, "%c%c            Begin a line with a single %c\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 257, "Modify [set] headers:                %ch             All headers\n" ),
    catref( CAT_MSGS, 258, "%ct [to list]  To                     %cs [subject]   Subject\n" ),
    catref( CAT_MSGS, 259, "%cc [cc list]  Cc                     %cb [bcc list]  Bcc\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 261, "%cH header-name [header-value]   Add an arbitrary header\n" ),
#ifdef DSERV
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 276, "%cC           Check all addresses against address lookup immediately\n" ),
    catref( CAT_MSGS, 277, "%cD[!]        Check [do not check] each address as it is entered\n" ),
#endif /* DSERV */
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 263, "View or edit message text:           %cu           Edit previous line\n" ),
    catref( CAT_MSGS, 264, "%ce [editor]    Enter editor          %cv [editor]  Visual editor\n" ),
    catref( CAT_MSGS, 265, "%cp [pager]     View message body and headers using pager\n" ),
    catref( CAT_MSGS, 266, "%c| cmd [args]  Pass the message body through the unix command \"cmd\"\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 268, "%cE[!]          Erase message body after [not] saving to dead.letter\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 270, "Include other text:\n" ),
    catref( CAT_MSGS, 271, "%ci [msg#'s]  Bodies of messages [msg#'s], indented by \"indent_str\"\n" ),
    catref( CAT_MSGS, 272, "%cI [msg#'s]  Bodies and headers of messages, indented\n" ),
    catref( CAT_MSGS, 273, "%cf [msg#'s]  Bodies and headers of messages, marked \"forwarded\"\n" ),
    catref( CAT_MSGS, 274, "%cr filename  Contents of file \"filename\" (not indented)\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 278, "%cS[!]        Add [suppress] signature when sending message\n" ),
    catref( CAT_MSGS, 279, "%cF[!]        Add [suppress] fortune when sending message\n" ),
    catref( CAT_MSGS, 280, "%cR[!]        Request [do not request] a return receipt\n" ),
    catref( CAT_MSGS, 281, "%c$variable   Insert the string value of \"variable\" into message\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 283, "%cA           List the current attachments, if any\n" ),
    catref( CAT_MSGS, 284, "%cA filename  Add the file \"filename\" as an attachment\n" ),
    catref( CAT_MSGS, 285, "%cA! number   Remove the attachment indicated by \"number\"\n" ),
    catref( CAT_MSGS, 808, "%cM [msg#'s]  Attach messages as message/rfc822 attachment parts\n" ),
    catref( CAT_MSGS, 286, "%cP number    Preview the attachment indicated by \"number\"\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 288, "Store message in file:\n" ),
    catref( CAT_MSGS, 289, "%cw file      Write to file           %ca file      Append to file\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 291, "%cl[!] file   Record [or not] the entire message on send (see $record)\n" ),
    catref( CAT_MSGS, 292, "%cL[!] file   Log [or not] the outgoing headers on send (see $logfile)\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 294, "Leave compose mode:\n" ),
    catref( CAT_MSGS, 888, "%c.  Send the message\n" ),
    catref( CAT_MSGS, 295, "%cq  Quit, save message               %cx  Exit without saving\n" ),
    catref( CAT_MSGS, 296, "%cz  Suspend, to be resumed later via \"resume\" command\n" ),
    catref( CAT_MSGS, 254, "\n" ),
    catref( CAT_MSGS, 298, "%c:cmd [args]  Run \"cmd\", then return immediately to compose mode\n" ),
    /* The following are undocumented at present
    catref( CAT_MSGS, 300, "%cD [address-book-program]          Specify program for address lookup\n" ),
    catref( CAT_MSGS, 302, "%cO [address-sort-program]          Specify program for address sort\n" ),
    catref( CAT_MSGS, 303, "%cT transport-agent-program         Specify transport agent\n" ),
    */
};

/* neat trick stolen from the X11 people */
#define num_elts(array)  sizeof(array) / sizeof(array[0])

/*
 * Add the line (char *) parameter to the letter.  Determine tilde
 * escapes and determine what to do.  This function returns 0 to
 * indicate user wants to end the letter, -1 if the letter cannot
 * be sent (~q, ~x no buffer after editor, etc...) or 1 to indicate
 * successful addition of the line to the letter.
 * This function may be called by toolmode just to change certain zmail
 * internal variables via tilde escapes.  Thus, ed_fp might be null.
 */
int
add_to_letter(line, linemode)
char line[];
int linemode;
{
    register char *p;
#if defined( IMAP )
    char *p4;
    void *pFolder;
#if defined( MOTIF )
    FileFinderStruct *ffs;
#endif
#endif

    char buf[HDRSIZ > MAXPATHLEN ? HDRSIZ : MAXPATHLEN];

    if (comp_current->ed_fp) /* may be null if istool */
	(void) fseek(comp_current->ed_fp, 0L, 2);

    if (!strcmp(line, ".") && boolean_val(VarDot))
	return COMPCMD_FINISHED;
    turnon(comp_current->flags, MODIFIED);
    if (line[0] != *escape || ison(glob_flags, QUOTE_MACRO)) {
	(void) fputs(line, comp_current->ed_fp);
	(void) fputc('\n', comp_current->ed_fp);
	(void) fflush(comp_current->ed_fp);
	return COMPCMD_NORMAL;
    }
    /* all commands are "~c" (where 'c' is the command). set p = first
     * character after 'c' and skip whitespace
     */
    p = &line[2];
    skipspaces(0);
    /* Expand variables now unless that's our only reason for living */
    if (line[1] == '$' || !linemode || variable_expand(p)) switch (line[1]) {
#if defined(SPELLER) && !defined(GUI_ONLY)
	case 3:	/* Spell */
	    if ((p = value_of(VarIspeller))) {
		line[1] = 'v';
	    } else {
		if (!(p = value_of(VarSpeller)))
		    p = SPELL_CHECKER;
		line[1] = 'e';
	    }
	    /* Fall through */
#endif /* SPELLER && !GUI_ONLY */
	case 'v' : case 'p': case 'e' : case '|' : {
#ifndef MAC_OS
	    if (!*p || *p == 'i')
		switch (line[1]) {
		    case 'p' :
#ifdef apollo
			if (apollo_ispad()) {
			    p = "NONE";		/* The pad IS the pager */
			    break;
			}
#endif /* apollo */
			if (!*p && !(p = value_of(VarPager)))
			    p = DEF_PAGER;
			if (!*p || !strcmp(p, "internal"))
			    p = NULL;
		    when 'v' :
			if (*p && p[1] || (p = value_of(VarVisual)))
			    break;
			/* else fall through */
		    default :
			if (!(p = value_of(VarEditor)) || !*p)
			    p = DEF_EDITOR;
		    when '|' :
			print(catgets( catalog, CAT_MSGS, 304, "No command for pipe\n" ));
			return COMPCMD_WARNING;
		}
#else /* MAC_OS */
	    p = NULL;
#endif /* !MAC_OS */
	    if (line[1] == 'p' || line[1] == '|')
		(void) fseek(comp_current->ed_fp, comp_current->body_pos, 0);
	    if (line[1] == 'p') {
		ZmPager pager;
		pager = ZmPagerStart(PgNormal);
		if (p) ZmPagerSetProgram(pager, p);
		if (output_headers(&comp_current->headers, NULL_FILE, 0) == 0) {
		    (void)
		    ZmPagerWrite(pager, strcpy(buf, catgets( catalog, CAT_MSGS, 305, "--------\nMessage contains:\n" )));
		    while (fgets(buf, sizeof(buf), comp_current->ed_fp))
			ZmPagerWrite(pager, buf);
		}
		ZmPagerStop(pager);
	    } else if (line[1] == '|') {
#ifndef MAC_OS
		FILE *pipe_fp;
		(void) sprintf(buf, "( %s ) > %s", p, comp_current->edfile);
		if (unlink(comp_current->edfile) < 0) {		/* XXX DOS */
		    error(SysErrWarning, catgets( catalog, CAT_MSGS, 306, "Cannot unlink \"%s\"" ),
			    comp_current->edfile);
		    break; /* Drop out of switch */
		}
		if ((pipe_fp = popen(buf, "w")) == NULL_FILE) {
		    error(SysErrWarning, catgets( catalog, CAT_MSGS, 307, "Cannot run \"%s\"" ), p);
		    (void) file_to_fp(comp_current->edfile,
			    comp_current->ed_fp, "w", 0);
		} else {
		    while (fgets(buf, sizeof(buf), comp_current->ed_fp))
			if (fputs(buf, pipe_fp) == EOF) {
			    print(catgets( catalog, CAT_SHELL, 421, "Broken pipe.\n" ));
			    break;
			}
		    (void) pclose(pipe_fp);
		}
		/* save ed_fp until we can reopen it */
		pipe_fp = comp_current->ed_fp;
		if (!(comp_current->ed_fp =
			fopen(comp_current->edfile, "r+"))) {
		    error(SysErrWarning, catgets( catalog, CAT_MSGS, 309, "cannot reopen %s" ),
			    comp_current->edfile);
		    (void) fseek(pipe_fp, comp_current->body_pos, 0);
		    if (file_to_fp(comp_current->edfile, pipe_fp, "w", 0) ||
			    !(comp_current->ed_fp =
				fopen(comp_current->edfile, "r+"))) {
			error(SysErrWarning,
			    catgets( catalog, CAT_MSGS, 310, "cannot restore old contents of %s" ),
				comp_current->edfile);
			comp_current->ed_fp = pipe_fp;
			dead_letter(0);
			return COMPCMD_FAILURE;
		    }
		}
		(void) fclose(pipe_fp);
		comp_current->body_pos = 0;
#else /* MAC_OS */
		error(SysErrWarning, catgets(catalog, CAT_MSGS, 889, "Can't pipe commands in Mac Z-Mail!\n"));
		return COMPCMD_FAILURE;
#endif /* MAC_OS */
	    } else {
		mta_headers(comp_current);
		if (run_editor(p) < 0)
		    return COMPCMD_FAILURE;
		else
		    return COMPCMD_NORMAL;
	    }
	}
	when 'A': {	/* ~A[!] file [type [encoding ["comment"]]] */
	    int ac = 0, negate = (*p == '!');
	    char **av;

	    skipspaces(negate);
	    if (*p) {
		av = mk_argv(p, &ac, TRUE);
		if (ac < 0) {
		    free_vec(av);
		    return COMPCMD_WARNING;
		}
	    }
	    if (negate) {
		del_attachment(comp_current, ac? av[0]: p);
	    } else {
		/* XXX casting away const */
		p = (char *) add_attachment(comp_current, ac? av[0] : p,
					    ac > 1? av[1] : (char *) NULL,
					    ac > 3? av[3] : (char *) NULL,
					    ac > 2? av[2] : (char *) NULL,
					    NO_FLAGS, NULL);
		if (p) {
#ifdef GUI
		    if (istool)
			error(UserErrWarning, p);
#endif /* GUI */
		    free_vec(av);
		    return COMPCMD_WARNING;
		}
	    }
	    if (ac > 0)
		free_vec(av);
	}
	when 'P':
	    preview_attachment(comp_current, p);
	when 1: {
	    char **av;
	    int ac = 0;
	    av = mk_argv(p, &ac, TRUE);
	    if (ac < 2) {
		free_vec(av);
		return COMPCMD_WARNING;
	    }
	    get_comp_attachments_info(comp_current, av);
	    free_vec(av);
	}
#ifdef INTERPOSERS
	when 2: {
	    char **av, **v;
	    int ac = 0;
	    av = mk_argv(p, &ac, TRUE);
	    v = unitv("compcmd interpose");
	    vcat(&v, av);
	    zm_interpose_cmd(ac+1, v, &comp_current->interpose_table);
	    free_vec(v);
	}
#endif /* INTERPOSERS */
	/* case 3 is above for fallthrough */
	when 4 : {
	    register char *p2 = buf;
	    while (*p && (isalnum(*p) || *p == '_'))
		*p2++ = *p++;
	    if (p2 == buf || (*p && !isspace(*p)))
		error(UserErrWarning,
		      catgets( catalog, CAT_SHELL, 429, "Illegal variable name.\n" ));
	    else
		*p2 = 0;
	    set_int_var(buf, "=", compose_guess_size(comp_current));
	}
	when '$': {
	    register char *p2;
	    if (!(p2 = value_of(p)))
		print(catgets( catalog, CAT_MSGS, 311, "(%s isn't set)\n" ), p);
	    else
		putstring(p2, comp_current->ed_fp);
	}
	when 'k': {
	    comp_current->out_char_set =  GetMimeCharSet(p);
	}
	when '.':
	    return COMPCMD_FINISHED;
	when ':': {
	    u_long save_flags = glob_flags;

	    turnon(glob_flags, IGN_SIGS);
	    turnoff(glob_flags, DO_PIPE);
	    turnoff(glob_flags, IS_PIPE);
	    (void) cmd_line(p, NULL_GRP);
	    glob_flags = save_flags;
	}
	when 'i': case 'f': case 'I': case 'm': case 'M': {
	    int  n;
	    u_long copy_flgs = 0;
	    msg_group list;

	    if (!msg_cnt) {
		wprint(catgets( catalog, CAT_SHELL, 96, "No messages." ));
		wprint("\n");
		break;
	    }
	    if (line[1] != 'M' && line[1] != 'f') {
		turnon(copy_flgs, INDENT|FOLD_ATTACH);
		if (line[1] == 'i')
		    turnon(copy_flgs, NO_HEADER);
		else if (!bool_option(VarAlwaysignore, "include"))
		    turnon(copy_flgs, NO_IGNORE);
	     } else {
		if (line[1] == 'f')
		    turnon(copy_flgs, FOLD_ATTACH);
		if (line[1] == 'M' || !bool_option(VarAlwaysignore, "forward"))
		    turnon(copy_flgs, NO_IGNORE);
		turnon(copy_flgs, FORWARD);
	    }
	    turnon(copy_flgs, NO_SEPARATOR);
	    SetCurrentCharSet(displayCharSet);
	    init_msg_group(&list, msg_cnt, 1);
	    clear_msg_group(&list);
	    if (!*p)
		add_msg_to_group(&list, current_msg);
	    else if (!zm_range(p, &list)) {
		destroy_msg_group(&list);
		return COMPCMD_WARNING;
	    }
	    for (n = 0; n < msg_cnt; n++)
		if (msg_is_in_group(&list, n)) {
		    FILE *att_fp = NULL_FILE;
		    char *att_fname = NULL;
		    if (line[1] == 'M' || line[1] == 'f') {
			(void) reply_to(n, FALSE, buf);
			if (line[1] == 'M') {
			    FMODE_SAVE();
			    FMODE_BINARY();
			    att_fp = open_tempfile("msg", &att_fname);
			    FMODE_RESTORE();
			    if (! att_fp) {
				error(SysErrWarning, 
				      catgets(catalog, CAT_MSGS, 809, "Unable to create temporary file for forwarded message."));
				destroy_msg_group(&list);
				return COMPCMD_WARNING;
			    }
			} else
			    (void) fprintf(comp_current->ed_fp,
					   catgets(catalog, CAT_MSGS, 314, "--- Forwarded mail from %s\n\n"),
					   decode_header("from", buf));
		    }
		    wprint(catgets( catalog, CAT_MSGS, 315, "Including message %d ... " ), n+1);
		    wprint(catgets( catalog, CAT_SHELL, 132, "(%d lines)\n" ),
			copy_msg(n, att_fp? att_fp : comp_current->ed_fp,
				copy_flgs, NULL, 0));
		    set_isread(n);
#if defined( IMAP ) 
                    zimap_turnoff( current_folder->uidval, msg[current_msg]->
uid,
                        ZMF_UNREAD );
#endif
		    if (line[1] == 'M') {
			if (fclose(att_fp) != EOF) {
			    const char *errorMsg;
			    char description[MAXPRINTLEN];
			    sprintf(description, catgets(catalog, CAT_SHELL, 918, "Message from %s"), buf);

			    errorMsg = (char *) add_attachment(comp_current, att_fname,
							       MimeTypeStr(MessageRfc822), 
							       description, "none",
							       AT_TEMPORARY, NULL);
			    xfree(att_fname);
			    if (errorMsg) {
#ifdef GUI
				if (istool)
				    error(UserErrWarning, errorMsg);
#endif /* GUI */
				destroy_msg_group(&list);
				return COMPCMD_WARNING;
			    }
			} else {
			    error(SysErrWarning, 
				  "Unable to close temporary file %s for forwarded message.\nAttachment not added.", att_fname);
			    xfree(att_fname);
			    destroy_msg_group(&list);
			    return COMPCMD_WARNING;
			}
		    } else if (line[1] == 'f') {
			(void) fprintf(comp_current->ed_fp,
				       catgets( catalog, CAT_MSGS, 317, "\n--- End of forwarded mail from %s\n\n" ),
				       decode_header("from", buf));
			copy_attachments(n);

		    } else
			(void) fputc('\n', comp_current->ed_fp);
		}
	    /* Record which messages have been included */
	    msg_group_combine(&comp_current->inc_list, MG_ADD, &list);
	    destroy_msg_group(&list);
	}
	when 'H': {
	    char *p2;
	    int n = 0;
	    
	    if (*p == '!') {
		n = TRUE;
		skipspaces(1);
	    } else
		skipspaces(0);
	    if (!*p) {
		print(catgets( catalog, CAT_MSGS, 318, "(you must specify a header)\n" ));
		return COMPCMD_WARNING;
	    }
	    if (p = any(p2 = p, ": \t")) {
		*p++ = 0;
		skipspaces(0);
		(void) no_newln(p);
	    }
	    input_header(p2, p, n);
	}
	/* To: Subject: Cc: and Bcc: headers */
	when 'h':
	    p = "";
	    input_address(TO_ADDR, p);
	    input_address(SUBJ_ADDR, p);
	    input_address(CC_ADDR, p);
	case 'b':
	    input_address(BCC_ADDR, p);
	when 'c':
	    input_address(CC_ADDR, p);
	when 't':
	    input_address(TO_ADDR, p);
	when 's':
	    input_address(SUBJ_ADDR, p);
	when 'F': case 'S' : {
	    if (*p == '!')
		turnoff(comp_current->send_flags,
		    line[1] == 'F'? DO_FORTUNE : SIGN);
	    else
		turnon(comp_current->send_flags,
		    line[1] == 'F'? DO_FORTUNE : SIGN);
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
#ifdef GUI
	    if (istool < 2)
#endif /* GUI */
	    wprint(*p == '!' ? catgets( catalog, CAT_MSGS, 320, "Not adding %s at end of message.\n" )
		             : catgets( catalog, CAT_MSGS, 321, "Adding %s at end of message.\n" ),
		   line[1] == 'F' ? catgets( catalog, CAT_MSGS, 322, "fortune" )
		                  : catgets( catalog, CAT_MSGS, 323, "signature" ));
	}
	/* XXX Should do something smarter with the headers here for 'w'/'a' */
	when 'w': case 'a': case 'r':
	    if (!*p) {
		print(catgets( catalog, CAT_MSGS, 324, "(you must specify a filename)\n" ));
		return COMPCMD_WARNING;
	    }
	    (void) fseek(comp_current->ed_fp, 0L, 2); /* append */
	    (void) file_to_fp(p, comp_current->ed_fp, (line[1] == 'r')? "r":
			      (line[1] == 'w')? "w": "a", 0);
	when 'd':
	    if (!*p) {
		print(catgets( catalog, CAT_MSGS, 324, "(you must specify a filename)\n" ));
		return COMPCMD_WARNING;
	    }
#if defined( IMAP ) 
#if defined( VUI ) || defined( GUI )
#if defined( MOTIF )
	    ffs = Getffs();
   	    pFolder = ffs->pFolder;
	    if ( ffs->useIMAP ) {
#else
	    pFolder = FolderByName( p );
	    if ( UseIMAP() ) {
#endif
	    if ( !pFolder ) 
		    error(UserErrWarning, catgets(catalog, CAT_MSGS, 1108, "Cannot locate \"%s\"."), p);
	    else {

#if defined( MOTIF )
		p4 = (char *) GetCurrentDirectory( pFolder );
#else
		p4 = p;
#endif
		if ( p4 && strlen( p4 ) && FolderIsFile( pFolder ) ) {

			if (imap_write_draft(p4, NULL) < 0)
				return COMPCMD_FAILURE;
		}
		else {
			error(UserErrWarning, catgets(catalog, CAT_MSGS, 1107, "\"%s\" is not a file."), p);
			return COMPCMD_FAILURE;
		}
            } 
#endif /* VUI || GUI */
#endif /* IMAP */
#if defined( IMAP )
#if defined( VUI ) || defined( GUI )
	    } else {
#else
	    else {
#endif
#endif
		FolderType fotype;
		int silent = ison(glob_flags, NO_INTERACT);
		errno = 0;
		if ((fotype = test_folder(p,
			silent? SNGL_NULL :
			catgets(catalog, CAT_SHELL, 143, "not a folder, save anyway?"))) & FolderUnknown) {
		    if (errno != ENOENT) {
			if (fotype & FolderInvalid)
			    error(SysErrWarning,
				catgets(catalog, CAT_SHELL, 145, "cannot save in \"%s\""), p);
			return COMPCMD_WARNING;
		    }
		}
	    }
#if defined( IMAP )
#if defined( VUI ) || defined( GUI )
#if defined( MOTIF )
	    if ( !ffs->useIMAP )
#else
	    if ( !UseIMAP() ) 
#endif
#endif /* VUI || GUI */
#endif /* IMAP */
	    if (write_draft(p, NULL) < 0)
		return COMPCMD_FAILURE;
	when 'l':
	    if (*p == '!') {
		turnoff(comp_current->send_flags, RECORDING);
	    } else {
		turnon(comp_current->send_flags, RECORDING);
		if (*p)
		    ZSTRDUP(comp_current->record, p);
	    }
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
	when 'L':
	    if (*p == '!') {
		turnoff(comp_current->send_flags, LOGGING);
	    } else {
		turnon(comp_current->send_flags, LOGGING);
		if (*p)
		    ZSTRDUP(comp_current->logfile, p);
	    }
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
#ifdef DSERV
	when 'C':
	    if (*p == '!') {
		turnoff(comp_current->flags, DIRECTORY_CHECK);
	    } else {
		(void)check_all_addrs(comp_current, FALSE);
	    }
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
	when 'D':
	    if (*p == '!') {
		turnoff(comp_current->flags, DIRECTORY_CHECK);
	    } else {
		turnon(comp_current->flags, DIRECTORY_CHECK);
		if (*p)
		    ZSTRDUP(comp_current->addressbook, p);
	    }
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
#endif /* DSERV */
	when 'O':
	    if (*p == '!') {
		turnoff(comp_current->flags, SORT_ADDRESSES);
	    } else {
		turnon(comp_current->flags, SORT_ADDRESSES);
		if (*p)
		    ZSTRDUP(comp_current->sorter, p);
	    }
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
	when 'T':
	    if (*p) {
		ZSTRDUP(comp_current->transport, p);
		print(catgets( catalog, CAT_MSGS, 325, "Transport agent set to: %s" ), p);
	    } else {
		xfree(comp_current->transport);
		comp_current->transport = NULL;		/* Reset to default */
	    }
	when 'R':
	    if (*p == '!')
		request_receipt(comp_current, FALSE, NULL);
	    else
		request_receipt(comp_current, TRUE, p);
#ifdef GUI
	    if (istool < 2)
#endif /* GUI */
	      wprint(*p == '!' ? catgets( catalog, CAT_MSGS, 327, "Not requesting return receipt.\n" )
		               : catgets( catalog, CAT_MSGS, 328, "Requesting return receipt.\n" ));
	/* go up one line in the message file and allow the user to edit it */
	when 'u': {
	    long newpos, pos = ftell(comp_current->ed_fp);
	    char oldline[256];
	    if (pos <= 0L) { /* pos could be -1 if ftell() failed */
		print(catgets( catalog, CAT_MSGS, 329, "(No previous line in file.)\n" ));
		return COMPCMD_WARNING;
	    }
	    /* get the last 256 bytes written and read backwards from the
	     * current place until '\n' is found. Start by moving past the
	     * first \n which is at the end of the line we want to edit
	     */
	    newpos = max(0, pos - 256L);
	    (void) fseek(comp_current->ed_fp, newpos, L_SET);
	    /* don't fgets -- it'll stop at a \n */
	    (void) fread(line, sizeof(char), (int)(pos-newpos),
		    comp_current->ed_fp);
	    pos--;
	    /* the last char in line should be a \n cuz it was last input */
	    if (line[(int)(pos-newpos)] != '\n')
	      print(catgets( catalog, CAT_MSGS, 330, "The last line of this composition is missing a closing newline.\n" ));
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
		print(catgets( catalog, CAT_MSGS, 331, "Something bad happened and I don't know what it is.\n" ));
		p = oldline;
	    } else if (*p == *escape)
		print(catgets( catalog, CAT_MSGS, 332, "(Warning: %c escapes ignored on %cu lines.)\n" ),
				*escape, *escape);
	    /* seek to to the position where the new line will go */
	    (void) fseek(comp_current->ed_fp, pos, L_SET);
	    /* put the newly typed line w/o \n.  padding may be necessary */
	    (void) fputs(p, comp_current->ed_fp);
	    /* if the new line is less than the old line, we're going to do
	     * one of two things.  The best thing to do is to truncate the
	     * file to the end of the new line.  Sys-v can't do that, so we
	     * pad the line with blanks.  May be messy in some cases, but...
	     */
	    if ((pos = strlen(p) - strlen(oldline)) < 0) {
#if (defined(HAVE_FTRUNCATE) || defined(F_FREESP)) && !(defined(MSDOS) || defined(MAC_OS))
		/* add the \n, flush the file, truncate to the current pos */
		(void) fputc('\n', comp_current->ed_fp);
		(void) fflush(comp_current->ed_fp);
		(void) ftruncate(fileno(comp_current->ed_fp),
			(off_t)ftell(comp_current->ed_fp));
#else /* !(HAVE_FTRUNCATE || F_FREESP) && !(MSDOS || MAC_OS) */
		/* pad with blanks to the length of the old line. add \n */
		while (pos++ < 0)
		    (void) fputc(' ', comp_current->ed_fp);
		(void) fputc('\n', comp_current->ed_fp),
			(void) fflush(comp_current->ed_fp);
#endif /* !(HAVE_FTRUNCATE || F_FREESP) && !(MSDOS || MAC_OS) */
	    } else {
		/* the new line is >= the old line, add \n -- no trunc req. */
	        (void) fputc('\n', comp_current->ed_fp);
		(void) fflush(comp_current->ed_fp);
	    }
	    return COMPCMD_NORMAL;
	 }
	/* break;  not here cuz of "return" (lint). */
	case 'E':
	    if (*p != '!' && !boolean_val(VarNosave))
		dead_letter(0);
	    if (truncate_edfile(comp_current, 0L) == -1) {
		error(SysErrWarning, comp_current->edfile);
		return COMPCMD_FAILURE;
	    } else
		print(catgets( catalog, CAT_MSGS, 333, "Message buffer empty\n" ));
	when 'q':
	    if (isoff(comp_current->flags, DOING_INTERPOSE)) {
		/* save in dead.letter if nosave not set -- rm_edfile(-2). */
		rm_edfile(-2); /* doesn't return out of tool mode */
	    } else
		turnon(comp_current->send_flags, SEND_CANCELLED);
	    return COMPCMD_FAILURE;
	    /* break; (not specified for lint) */
	case 'x':
	    if (isoff(comp_current->flags, DOING_INTERPOSE)) {
		/* don't save dead.letter -- simulate normal rm_edfile() */
		rm_edfile(0);
	    } else
		turnon(comp_current->send_flags, SEND_KILLED);
	    return COMPCMD_FAILURE;
	    /* break; (not specified for lint) */
	case 'z':
	    if (isoff(comp_current->flags, DOING_INTERPOSE)) {
		if (ison(glob_flags, IS_SHELL|DO_SHELL)) {
		    suspend_compose(comp_current);
		    if (!istool)
			wprint(catgets( catalog, CAT_MSGS, 334, "[%s]  Stopped.\n" ), comp_current->link.l_name);
		    return COMPCMD_SUSPEND;
		} else
		    print(catgets( catalog, CAT_MSGS, 335, "You cannot suspend unless running the shell.\n" ));
	    } else
		interposer_thwart = TRUE;	/* XXX */
	otherwise:
	    if (line[1] == *escape) {
		(void) fputs(&line[1], comp_current->ed_fp);
		(void) fputc('\n', comp_current->ed_fp);
		(void) fflush(comp_current->ed_fp);
		return COMPCMD_NORMAL;
	    } else if (line[1] == '?') {
		register int x;
		ZmPager pager;
		pager = ZmPagerStart(PgInternal);
		(void) ZmPagerWrite(pager,
   "----------------------------------------------------------------------\n");
		for (x = 0; x < num_elts(tilde_commands); x++) {
		    (void) sprintf(buf, catgetref(tilde_commands[x]),
				    *escape, *escape, *escape);
		    ZmPagerWrite(pager, buf);
		}
		ZmPagerStop(pager);
		wprint(
    "----------------------------------------------------------------------\n"
			);
	    } else {
		print(catgets( catalog, CAT_MSGS, 336, "`%c': unknown %c escape. Use %c? for help.\n" ),
		    line[1], *escape, *escape);
		return COMPCMD_WARNING;
	    }
    }
    if (comp_current->ed_fp)
	(void) fseek(comp_current->ed_fp, 0L, 2);
    if (!istool && linemode)
	wprint(catgets( catalog, CAT_MSGS, 337, "(continue editing message)\n" ));
    return COMPCMD_NORMAL;
}

#define EMAIL_NOTSUPPORTED	ULBIT(0)
#define EMAIL_FILE_ARG		ULBIT(1)
#define EMAIL_MESSAGE_ARG	ULBIT(2)
#define EMAIL_COMMAND_ARG	ULBIT(3)
#define EMAIL_QUOTED_ARGS	ULBIT(4)
#define EMAIL_ADDRESS_ARG	ULBIT(5)
#define EMAIL_NO_ARGUMENT	ULBIT(6)
#define EMAIL_ONE_ARGUMENT	ULBIT(7)
#define EMAIL_ARG_OPTIONAL	ULBIT(8)
#define EMAIL_ONE_OPTIONAL	(EMAIL_ONE_ARGUMENT|EMAIL_ARG_OPTIONAL)

/*
 *	NOTE: If you add a new flag here, you need to add it to 
 *	gui_edmail as well.
 *
 *	Available tilde letters: BJjKkNnQUVWXYyZ
 */
struct edmail {
    char *tilde;
    char *keyword;
    u_long flags;
} comp_commands[] = {
    /* "append" and "insert" are equivalent at the moment ...		*/
    { " ", "append-text",	EMAIL_ADDRESS_ARG			},
    { " ", "insert-text",	EMAIL_ADDRESS_ARG			},
    { "\n","append-line",	EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL	},
    { "\n","insert-line",	EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL	},
    { "\1","attach-info",       0		    			},
#ifdef INTERPOSERS
    { "\2","interpose",	 	0 					},
#endif /* INTERPOSERS */
#ifdef SPELLER
    { "\3","spell",             EMAIL_NO_ARGUMENT                       },
#endif /* SPELLER */
    { "\4","approx-size",	EMAIL_ONE_ARGUMENT			},
    /* These are the regular tilde-escapes ...				*/
    { "$", NULL,		EMAIL_NOTSUPPORTED			},
    { ".", "send",		EMAIL_NO_ARGUMENT			},
    { ".", "send-message",	EMAIL_NO_ARGUMENT			},
    { ":", NULL,		EMAIL_NOTSUPPORTED			},
    { "A", "attach",		EMAIL_FILE_ARG    | EMAIL_QUOTED_ARGS	},
    { "A", "attach-file",	EMAIL_FILE_ARG    | EMAIL_QUOTED_ARGS	},
    { "a", "save",		EMAIL_FILE_ARG    | EMAIL_ONE_ARGUMENT	},
    { "a", "save-to-file",	EMAIL_FILE_ARG    | EMAIL_ONE_ARGUMENT	},
    { "b", "bcc",		EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL	},
    { "b", "blind-carbon",	EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL  },
    { "c", "cc",		EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL  },
    { "c", "carbon-copy",	EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL  },
    { "C", "check",		EMAIL_NO_ARGUMENT			},
    { "C", "address-check",	EMAIL_NO_ARGUMENT			},
    { "C", "directory-check",	EMAIL_NO_ARGUMENT			},
    { "D", "address-book",	EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "D", "directory",		EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "D", "directory-service",	EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "d", "draft",		EMAIL_FILE_ARG    | EMAIL_ONE_ARGUMENT	},
    { "d", "save-draft",	EMAIL_FILE_ARG    | EMAIL_ONE_ARGUMENT	},
    { "E", "erase",		EMAIL_NO_ARGUMENT			},
    { "e", "edit",		EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "e", "edit-message",	EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "F", "use-fortune",	EMAIL_FILE_ARG    | EMAIL_ARG_OPTIONAL	},
    { "F", "fortune",		EMAIL_FILE_ARG    | EMAIL_ARG_OPTIONAL	},
    { "f", "forward",		EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "f", "forward-unindented", EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "H", "insert-header",	EMAIL_ADDRESS_ARG			},
    { "H", "set-header",	EMAIL_ADDRESS_ARG			},
    { "H", "header",		EMAIL_ADDRESS_ARG			},
    { "h", NULL,		EMAIL_NOTSUPPORTED			},
    { "I", "insert-message",	EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "i", "include-message",	EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "i", "include",		EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "k", "charset",		EMAIL_ONE_ARGUMENT			},
    { "L", "log",		EMAIL_FILE_ARG    | EMAIL_ONE_OPTIONAL	},
    { "L", "logging",		EMAIL_FILE_ARG    | EMAIL_ONE_OPTIONAL	},
    { "l", "record",		EMAIL_FILE_ARG    | EMAIL_ONE_OPTIONAL	},
    { "M", "forward-attached",	EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "M", "message-attached",	EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "m", "insert-message",	EMAIL_MESSAGE_ARG | EMAIL_ARG_OPTIONAL	},
    { "O", "sort-addresses",	EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "P", "preview",		EMAIL_FILE_ARG				},
    { "p", "pager",		EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "q", "cancel",		EMAIL_NO_ARGUMENT			},
    { "q", "quit",		EMAIL_NO_ARGUMENT			},
    { "R", "return-receipt",	EMAIL_ADDRESS_ARG | EMAIL_ONE_OPTIONAL	},
    { "R", "return-receipt-to",	EMAIL_ADDRESS_ARG | EMAIL_ONE_OPTIONAL	},
    { "R", "receipt",		EMAIL_ADDRESS_ARG | EMAIL_ONE_OPTIONAL	},
    { "r", "insert-file",	EMAIL_FILE_ARG	  | EMAIL_ONE_ARGUMENT	},
    { "S", "use-signature",	EMAIL_FILE_ARG    | EMAIL_ONE_OPTIONAL	},
    { "S", "signature",		EMAIL_FILE_ARG    | EMAIL_ONE_OPTIONAL	},
    { "s", "subject",		EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL  },
    { "T", "transport-via",	EMAIL_COMMAND_ARG			},
    { "t", "to",		EMAIL_ADDRESS_ARG | EMAIL_ARG_OPTIONAL  },
    { "u",  NULL,		EMAIL_NOTSUPPORTED			},
    { "v", "edit-visual",	EMAIL_COMMAND_ARG | EMAIL_ARG_OPTIONAL	},
    { "w", "write",		EMAIL_FILE_ARG    | EMAIL_ONE_ARGUMENT	},
    { "w", "write-to-file",	EMAIL_FILE_ARG    | EMAIL_ONE_ARGUMENT	},
    { "x", "kill",		EMAIL_NO_ARGUMENT			},
    { "z", "suspend",		EMAIL_NO_ARGUMENT			},
    { "|", "pipe",		EMAIL_COMMAND_ARG			},
    /* These are special cases ...					*/
    { "",  "get-header",	EMAIL_ONE_ARGUMENT,			},
    { NULL, NULL,		NO_FLAGS				},
};

struct edmail *
fetch_comp_command(name)
char *name;
{
    int i;

    for (i = 0; comp_commands[i].tilde; i++)
	if (comp_commands[i].keyword &&
		ci_strcmp(name, comp_commands[i].keyword) == 0) {
	    return &comp_commands[i];
	}
    return 0;
}

/* Syntax:
 *	compcmd [%job] action[:off] [parameters]
 *
 * Examples:
 *	compcmd attach picture.ps postscript compress "A picture"
 *	compcmd %2 record ~/.outgoing
 *	compcmd log:off
 *	compcmd send
 */
int
zm_edmail(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    Compose *compose;
    struct edmail *e;
    char *p, *line = NULL, *tmpv[3];
    const char *argv0 = argv[0];
    int suspended = isoff(glob_flags, IS_GETTING);
    int negate = 0, guistate = EDMAIL_ABORT;

    if (!*++argv)
	return -1;
    if (*argv && argv[0][0] == '%' && argv[0][1]) {
	compose = (Compose *)retrieve_link(comp_list, &argv[0][1], strcmp);
	if (!compose) {
	    error(UserErrWarning,
		catgets(catalog, CAT_MSGS, 849, "%s: %s: no such composition"),
		argv0, argv[0]);
	    return -1;
	}
	++argv;
    } else
	compose = comp_current;
    if (!compose) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 850, "%s: no compositions"), argv0);
	return -1;
    }
    if (!*argv) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 851, "%s: no command specified"),
	    argv0);
	return -1;
    }
    if (comp_current && ison(comp_current->flags, DOING_INTERPOSE) &&
	    compose != comp_current) {
	return -1;
    } else if (compose->exec_pid) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 338, "%s: composition %%%s: editor running"),
	    argv0, compose->link.l_name);
	return -1;
    }
    if (p = index(*argv, ':')) {
	*p++ = 0;
	negate = !strcmp(p, "off");
    }
    if (!(e = fetch_comp_command(*argv++))) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 852, "%s: %s: no such compose command"),
	    argv0, *--argv);
	return -1;
    }
    if  (isoff(e->flags, EMAIL_NO_ARGUMENT)) {
	if (ison(e->flags, EMAIL_MESSAGE_ARG)) {
	    if ((argc = get_msg_list(argv, list)) < 0)
		return -1;
	    line = list_to_str(list);
	    argv += argc;
	} else if (*argv) {
	    if (ison(e->flags, EMAIL_ONE_ARGUMENT))
		line = savestr(*argv);
	    else if (ison(e->flags, EMAIL_COMMAND_ARG)) {
		/* Be clever like the "sh" command */
		if (vlen(argv) > 1)
		    line = smart_argv_to_string(NULL, argv, NULL);
		else
		    line = savestr(argv[0]);
	    } else if (ison(e->flags, EMAIL_ADDRESS_ARG))
		line = argv_to_string(NULL, argv);
	    else if (ison(e->flags, EMAIL_QUOTED_ARGS)) {
		char **qv = quote_argv(argv, '\'', TRUE);
		line = joinv(NULL, qv, " ");
		free_vec(qv);
	    } else
		line = joinv(NULL, argv, " ");
	} else if (isoff(e->flags, EMAIL_ARG_OPTIONAL)) {
	    error(UserErrWarning,
		catgets(catalog, CAT_MSGS, 853, "%s %s: not enough arguments"),
		argv0, e->keyword);
	    return -1;
	}
    }
#ifdef GUI
    /* A bit of unavoidable knowledge about the guts of sending
     * and its interaction with interposers.  Don't do anything
     * GUI-related during an interposition on send; we've already
     * dumped everything out to the file, so act like line mode.
     */
    if (istool > 1 && isoff(compose->flags, DOING_INTERPOSE)) {
	guistate = gui_edmail(e->tilde[0], negate, line, compose);
	if (guistate <= 0) {
	    xfree(line);
	    return guistate;
	}
	argc = COMPCMD_NORMAL;
    }
#endif /* GUI */
    if (suspended || compose != comp_current)
	resume_compose(compose);
    if (!e->tilde[0]) {
	/* This special case means "set the variable VAR_HDR_VALUE to the
	 * body of the argument header field".
	 */
	HeaderField *hf = lookup_header(&compose->headers, line, ", ", FALSE);
	if (hf) {
	    set_var(VAR_HDR_VALUE, "=", hf->hf_body);
	    destroy_header(hf);
	} else
	    (void) un_set(&set_options, VAR_HDR_VALUE);
    } else {
	/* A couple of special-cases for non-tilde-command operations */
	if (isspace(e->tilde[0])) {
	    if (e->tilde[0] == '\n')
		(void) strapp(&line, "\n");
	    else if (*line == *escape) {
		char *ptr = savestr(escape);
		ptr[1] = 0;
		(void) strapp(&ptr, line);
		xfree(line);
		line = ptr;
	    }
	} else {
	    tmpv[0] = zmVaStr("%s%s%s", escape, e->tilde, negate? "!" : "");
	    tmpv[1] = line;
	    tmpv[2] = 0;
	    line = joinv(NULL, tmpv, " ");
            xfree(tmpv[1]);
	}
	Debug("Compose command: (%s) ...\n", line);
	argc = add_to_letter(line, 0);
    }
    xfree(line);
    if (isoff(compose->flags, DOING_INTERPOSE)) {
	if (argc == COMPCMD_FINISHED && finish_up_letter() == 0)
	    suspended = 0;
#ifdef GUI
	else if (istool > 1) {
	    if (compose->ed_fp)
		prepare_edfile();
	    if (guistate == EDMAIL_STATESAVED)
		gui_end_edit(compose);
	}
#endif
	if (suspended && argc != COMPCMD_SUSPEND)
	    suspend_compose(compose);
    }
    if (argc == COMPCMD_FINISHED || argc == COMPCMD_NORMAL ||
	    argc == COMPCMD_SUSPEND)
	return 0;
    return -1;
}

int
interpose_on_send(compose)
Compose *compose;
{
    static const char *av[3] = { "send", NULL, NULL };
    zmInterpose **send_interposers, **comp_send_interposers;

    if (ison(compose->flags, DOING_INTERPOSE))
	return -1;

    send_interposers =
	fetch_interposer_list(av[0], (zmInterposeTable **) 0);
    comp_send_interposers =
	fetch_interposer_list(av[0], &compose->interpose_table);
    if (!send_interposers && !comp_send_interposers)
	return 0;

    turnon(compose->flags, DOING_INTERPOSE);

    av[1] = compose->edfile;

    /* XXX casting way const */
    (void) call_all_interposers(send_interposers,      (char **) av, NULL_GRP);
    /* XXX casting way const */
    (void) call_all_interposers(comp_send_interposers, (char **) av, NULL_GRP);

    turnoff(compose->flags, DOING_INTERPOSE);

    if (interposer_thwart) {
	interposer_thwart = 0;
	return -1;
    }
    return 1;
}

char *
job_numbers()
{
    static char *jobstring;
    Compose *compose = comp_list;
    char buf[12];

    if (!compose)
	return NULL;
    if (jobstring)
	jobstring[0] = 0;

    if (comp_current) {
	(void) sprintf(buf, "%s", comp_current->link.l_name);
	if (strapp(&jobstring, buf) == 0)
	    return jobstring;
    }
    do {
	if (compose == comp_current)
	    continue;
	if (jobstring && jobstring[0])
	    if (strapp(&jobstring, " ") == 0)
		break;
	(void) sprintf(buf, "%s", compose->link.l_name);
	if (strapp(&jobstring, buf) == 0)
	    break;
    } while (compose != comp_list);

    return jobstring;
}

int
zm_jobs(argc, argv, unused)
int argc;
char **argv;
msg_group *unused;
{
    Compose *compose = comp_list;
    char *p = *argv++;

    if (*p == 'j') {
	if (compose) {
	    argc = 0;
	    argv = DUBL_NULL; /* freed elsewhere */
	    do {
		argc = catv(argc, &argv, 1,
			unitv(zmVaStr(" [%s]%s %c To: %-20.20s%s   %-36.36s\n",
				    compose->link.l_name,
				    compose->link.l_name[1]? "" : " ",
				    compose == comp_current? '+' : ' ',
				    compose->addresses[TO_ADDR],
				    strlen(compose->addresses[TO_ADDR]) > 20?
					"..." : "   ",
				    compose->addresses[SUBJ_ADDR]?
					compose->addresses[SUBJ_ADDR]: "")));
		compose = (Compose *)compose->link.l_next;
	    } while (argc > 0 && compose != comp_list);
	    if (argc > 0) {
		ZmPager pager;
		qsort((char *)argv, argc, sizeof(char *),
		      (int (*) NP((CVPTR, CVPTR))) strptrcmp);
		pager = ZmPagerStart(PgInternal);
		for (argc = 0; argv[argc]; argc++)
		    ZmPagerWrite(pager, argv[argc]);
		ZmPagerStop(pager);
	    }
	    free_vec(argv);
	} else
	    print(catgets( catalog, CAT_MSGS, 340, "%s: No compositions.\n" ), p);
	return 0 - in_pipe();
    } else if (istool)
	return -1;
#ifndef GUI_ONLY
    else if (ison(glob_flags, IS_GETTING)) {
	/* No need to error() because GUI mode can't get here */
	wprint(catgets( catalog, CAT_MSGS, 341, "%s: finish or suspend current composition first.\n" ), p);
	return -1;
    } else if (!*argv) {
	if (comp_list && comp_current) {
	    resume_compose(comp_current);
	    wprint(" [%s]%s   To: %-20.20s%s   %-36.36s\n",
			comp_current->link.l_name,
			comp_current->link.l_name[1]? "" : " ",
			comp_current->addresses[TO_ADDR],
			strlen(comp_current->addresses[TO_ADDR]) > 20?
			    "..." : "   ",
			comp_current->addresses[SUBJ_ADDR]?
			    comp_current->addresses[SUBJ_ADDR] : "");
	    wprint(catgets( catalog, CAT_MSGS, 337, "(continue editing message)\n" ));
	    return compose_letter();
	} else
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 343, "%s: No current composition." ), p);
	return -1;
    }
    p = (argv[0][0] == '%')? &argv[0][1] : argv[0];
    if (compose = (Compose *)retrieve_link(comp_list, p, strcmp)) {
	resume_compose(compose);
	wprint(" [%s]%s   To: %-20.20s%s   %-36.36s\n",
		    comp_current->link.l_name,
		    comp_current->link.l_name[1]? "" : " ",
		    comp_current->addresses[TO_ADDR],
		    strlen(comp_current->addresses[TO_ADDR]) > 20?
			"..." : "   ",
		    comp_current->addresses[SUBJ_ADDR]?
			comp_current->addresses[SUBJ_ADDR] : "");
	wprint(catgets( catalog, CAT_MSGS, 337, "(continue editing message)\n" ));
	return compose_letter();
    } else
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 345, "%%%s: No such composition." ), p);
#endif /* !GUI_ONLY */
    return -1;
}

char *
dyn_gets(dsp, fp)
struct dynstr *dsp;
FILE *fp;
{
    char buf[BUFSIZ];
    int c = 0;

    do {	/* In case reading from a pipe, don't fail on SIGCHLD */
	errno = 0;
	while (fgets(buf, sizeof(buf), fp)) {
	    dynstr_Append(dsp, buf);
	    if ((c = dynstr_Chop(dsp)) != '\n') {
		dynstr_AppendChar(dsp, c);
	    } else {
#ifdef USE_CRLFS	/* Discard \r\n not just \n */
		if ((c = dynstr_Chop(dsp)) != '\r')
		    dynstr_AppendChar(dsp, c);
#endif /* USE_CRLFS */
		break;
	    }
	}
    } while (errno == EINTR && !feof(fp));
    return (c ? dynstr_Str(dsp) : (char *) NULL);
}

#ifdef VUI			/* was #if 0; perhaps will be again
				 * someday */
char *
address_sort(s)
char *s;
{
    char *p, **addrs;
    static char *buf;
#define sizeofbuf HDRSIZ

    if (!buf && !(buf = (char *) malloc(sizeofbuf)))
	error(SysErrFatal, catgets( catalog, CAT_MSGS, 14, "cannot continue" ));

    if (!s || !*s)
	return NULL;
    addrs = addr_list_sort(addr_vec(s));
    if (addrs) {
	p = joinv(buf, addrs, ", ");
	free_vec(addrs);
    } else
	(void) strcpy(buf, s);

#undef sizeofbuf
    return buf;
}
#endif /* VUI */

char *
get_message_state()
{
    char **itemstr;
    static char buf[128];
    static char *state_items[] = {
	"is_next", "is_prev", "pinup", "attachments", "active_comp", "plain_text", "phone_tag", "tag_it", NULL
    };
    int item = GSTATE_IS_NEXT;

    *buf = 0;
    for (itemstr = state_items; *itemstr; itemstr++, item++) {
#ifdef GUI
	if (istool) {
	    if (gui_get_state(item) < 1) continue;
	    if (*buf) strcat(buf, ",");
	    strcat(buf, *itemstr);
	} else
#endif /* GUI */
	switch(item) {
	    case GSTATE_ATTACHMENTS:
		if (msg[current_msg]->m_attach != 0) {
		    if (*buf) strcat(buf, ",");
		    strcat(buf, *itemstr);
		}
		break;
	    case GSTATE_IS_NEXT: case GSTATE_IS_PREV:
		if (next_msg(current_msg, (item == GSTATE_IS_NEXT) ? 1 : -1) !=
			current_msg) {
		    if (*buf) strcat(buf, ",");
		    strcat(buf, *itemstr);
		}
		break;
	    case GSTATE_PLAIN_TEXT:
		if (is_plaintext(msg[current_msg]->m_attach)) {
		    if (*buf) strcat(buf, ",");
		    strcat(buf, *itemstr);
		}
		break;
	    default:
		continue;
	}
    }
    return buf;
}

char *
get_main_state()
{
    static char buf[100];

    *buf = 0;
    if (ison(current_folder->mf_flags, CONTEXT_IN_USE)) {
	strcat(buf, "has_folder");
	if (isoff(current_folder->mf_flags, READONLY_FOLDER)) {
	    if (*buf) strcat(buf, ",");
	    strcat(buf, "folder_writable");
	}
	if (current_folder->mf_count) {
	    if (*buf) strcat(buf, ",");
	    strcat(buf, "folder_has_messages");
	}
	if (current_folder == &spool_folder) {
	    if (*buf) strcat(buf, ",");
	    strcat(buf, "folder_is_spool");
	}
#ifdef QUEUE_MAIL
	if (ison(current_folder->mf_flags, QUEUE_FOLDER)) {
	    if (*buf) strcat(buf, ",");
	    strcat(buf, "folder_is_queue");
	}
#endif /* QUEUE_MAIL */
	if (ison(current_folder->mf_flags, DO_UPDATE)) {
	    if (*buf) strcat(buf, ",");
	    strcat(buf, "folder_needs_update");
	}
    }
    return buf;
}

struct state_item {
    char *item;
    int is_send_flag;
    int flag, writable;
};

static struct state_item state_items[] = {
    { "active", 0, ACTIVE },
    { "autosign", 1, SIGN },
    { "autoformat", 0, AUTOFORMAT },
    { "return_receipt", 1, RETURN_RECEIPT },
    { "edit_headers", 0, EDIT_HDRS },
    { "record_msg", 1, RECORDING },
    { "sort_addresses", 0, SORT_ADDRESSES },
    { "directory_check", 0, DIRECTORY_CHECK },
    { "sendtime_check", 0, SENDTIME_CHECK },
    { "log_msg", 1, LOGGING },
    { "verbose", 1, VERBOSE },
    { "synchronous", 1, SYNCH_SEND },
    { "autoclear", 0, AUTOCLEAR },
    { "record_user", 1, RECORDUSER },
    { "confirm_send", 0, CONFIRM_SEND },
    { "phone_tag", 1, PHONE_TAG },
    { "tag_it", 1, TAG_IT },
    { "read_receipt", 1, READ_RECEIPT },
    { NULL, 0 }
};

char *get_priority();

char *
get_compose_state()
{
    static char buf[512];
    struct state_item *sp;
    char *pri;

    if (!comp_current) return "";
#ifdef GUI
    if (istool && gui_get_state(GSTATE_ACTIVE_COMP) != TRUE) return "";
#endif /* GUI */
    *buf = 0;
    for (sp = state_items; sp->item; sp++) {
	if (isoff(sp->is_send_flag ?
		 comp_current->send_flags : comp_current->flags, sp->flag))
	    continue;
	if (*buf) strcat(buf, ",");
	strcat(buf, sp->item);
    }
    if (*buf) {
	pri = get_priority(NULL);
	if (!*pri) strcat(buf, ",pri_none");
	else {
	    strcat(buf, ",pri_");
	    strcat(buf, pri);
	}
	if (comp_current->attachments)
	    strcat(buf, ",attachments");
    }
    return buf;
}

/*
 * Change the value of the per-composition variable "compose_state".
 *
 * how ==  0	--> value didn't actually change [see add_option()]
 * how ==  1	--> add new item(s) to value
 * how == -1	--> remove item(s) from value
 *
 * BUG:  Doesn't work for priorities when the priority is
 *       one of several items being set simultaneously.
 */
void
set_compose_state(item, how)
const char *item;
int how;
{
    struct state_item *sp;
    const char *pri;
    u_long *flagp;

    if (!comp_current) return;
    if (!how) return;
    if (comp_current->exec_pid) {
	error(UserErrWarning,
	    catgets(
	    catalog, CAT_MSGS, 890, "Composition %%%s: can't change %s while running an editor."),
	    comp_current->link.l_name, "compose_state");
	return;
    }
    if (!ci_strncmp(item, "pri_", 4)) {
	if (!ci_strcmp(item, "pri_none") || how < 0) {
	    pri = NULL;
	} else {
	    pri = item+4;
	}
#ifdef GUI
	gui_request_priority(comp_current, pri);
#else /* !GUI */
	request_priority(comp_current, pri);
#endif /* !GUI */
	return;
    }
    for (sp = state_items; sp->item; sp++) {
	if (chk_two_lists(item, sp->item, ", ") == 0) continue;
	if (!sp->is_send_flag && sp->flag == ACTIVE) continue;
	if (sp->is_send_flag && sp->flag == RETURN_RECEIPT) {
#ifdef GUI
	    gui_request_receipt(comp_current, how == 1, NULL);
#else /* !GUI */
	    request_receipt(comp_current, how == 1, NULL);
#endif /* !GUI */
	    continue;
	}
	flagp = (sp->is_send_flag) ? &comp_current->send_flags :
	        &comp_current->flags;
	if (how == 1)
	    turnon(*flagp, sp->flag);
	else
	    turnoff(*flagp, sp->flag);
    }
    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
}

#ifdef UNIX
int
strs_from_program(command, strs)
const char *command;
char ***strs;
{
    char *p;
    struct dynstr ds;
    FILE *fp;
    int n = 0, x;

    if (!command || !*command || !strs)
	return 0;
    *strs = DUBL_NULL;
    fp = popen(command, "r");
    if (!fp) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 347, "Unable to run ( %s )" ), command);
	return -1;
    }
    dynstr_Init(&ds);
    while (n >= 0 && (p = dyn_gets(&ds, fp))) {
	n = catv(n, strs, 1, unitv(p));
	dynstr_Set(&ds, "");
    }
    dynstr_Destroy(&ds);
    /* WEXITSTATUS() may try to take address of its argument (blech) */
    x = pclose(fp);
    return n < 0? -1 : WEXITSTATUS((*(WAITSTATUS *) &x));
}
#endif /* UNIX */
