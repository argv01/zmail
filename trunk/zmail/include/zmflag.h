/* zmflag.h	Copyright 1992 Z-Code Software Corp. */

/*
 * $Revision: 2.42 $
 * $Date: 1998/12/07 22:49:53 $
 * $Author: schaefer $
 */

#ifndef _ZMFLAG_H_
#define _ZMFLAG_H_

#include "osconfig.h"
#include "general.h"

#ifdef _ZM_H_
#include "gui_def.h"	/* Must precede zccmac.h */
#endif /* _ZM_H_ */

#include "zccmac.h"
#include "strcase.h"

/***
 *
 * Flags are divided into three groups: global booleans, function controls,
 * and folder/message tags.  Flag numbers should not be re-used within a
 * group, and flag name/number pairs should not be re-used among groups.
 * Numbers may be reused in different groups if given different names.
 *
 ***/

extern u_long
    glob_flags;		/* global boolean flags throughout the program */

#define NO_FLAGS	((u_long)0)

/* Global flags for glob_flags.  Some numbers have been skipped to allow
 * additional flags to be added in each group.
 */

/* signal control flags */
#define IGN_SIGS    ULBIT(0)  /* true if catch() should not longjmp */
#define WAS_INTR    ULBIT(1)  /* catch interrupts, set this flag (signals.c) */
#define SIGNALS_OK  ULBIT(29) /* ok to call signal handlers immediately */

/* user-controllable boolean conditions */
#define DOT_LOCK    ULBIT(2) /* MTA requires that dot-locking be applied */
#define PICKY_MTA   ULBIT(3) /* MTA won't accept From: and Date: */
#define ECHO_FLAG   ULBIT(4) /* if true, echo|cbreak is ON, echo typing (-e) */
#define WARNINGS    ULBIT(5) /* if set, various warning messages are printed */
#define MIL_TIME    ULBIT(6) /* if $mil_time is set, use 24hr time fmt */
#define DATE_RECV   ULBIT(7) /* $date_received: show date received on msgs */
#define REV_VIDEO   ULBIT(8) /* reverse video for curses or toolmode */

/* shell-controlled boolean conditions */
#define HALT_ON_ERR ULBIT(9)  /* parser quits upon command-not-found error */
#define REDIRECT    ULBIT(10) /* true if stdin is being redirected */
#define PRE_CURSES  ULBIT(11) /* true if curses will run, but hasn't started */
#define DO_SHELL    ULBIT(12) /* run a shell even if no mail (true if tool) */
#define IS_SHELL    ULBIT(13) /* the shell is running (almost always true) */
#define IS_SENDING  ULBIT(14) /* was run to send mail, not run as a shell */
#define IS_GETTING  ULBIT(15) /* true if we're getting input for a letter */
#define IGN_BANG    ULBIT(16) /* ignore ! as a history reference */
#define DO_PIPE     ULBIT(17) /* if commands are piping to other commands */
#define IS_PIPE     ULBIT(18) /* true if commands' "input" comes from a pipe */
#define IS_FILTER   ULBIT(19) /* true if performing an automatic filtration */
#define NO_INTERACT ULBIT(20) /* don't allow interactive commands */
#define ADMIN_MODE  ULBIT(31) /* allow special administrative commands */

#define is_shell	ison(glob_flags, IS_SHELL)

/* I/O control conditions */
#define CNTD_CMD    ULBIT(21) /* curses -- promotes "...continue..." prompt */
#define CONT_PRNT   ULBIT(22) /* continue to print without a '\n' */
#ifdef NOT_NOW
#define NEW_FRAME   ULBIT(23) /* toolmode should build a new frame for pager */
#define PINUP_FRAME (NEW_FRAME|ULBIT(24))
			      /* this is a pinup window, not a help frame */
#define HELP_TEXT   ULBIT(25) /* create textsw frame for paging help messages */
#endif /* NOT_NOW */
#define IN_MACRO    ULBIT(26) /* input is currently being read from a macro */
#define LINE_MACRO  ULBIT(27) /* escape to line mode from curses in progress */
#define QUOTE_MACRO ULBIT(28) /* protect current macro from recursive expand */
/* ULBIT(29) is taken, see above */
#define NOT_ASKED   ULBIT(30) /* cleared whenever the user is asked something */
/* ULBIT(31) is taken, see above */

#define in_pipe() (!!ison(glob_flags, DO_PIPE|IS_PIPE))
#define in_macro() (!!ison(glob_flags, LINE_MACRO|IN_MACRO))
#define line_macro(s) (void) (turnon(glob_flags, LINE_MACRO), mac_push(s))
#define curs_macro(s) (void) (turnon(glob_flags, IN_MACRO), mac_push(s))
#define Ungetstr(s) (void) (turnon(glob_flags, QUOTE_MACRO), mac_push(s))

/* Function control flags.  These generally refer to the way messages are
 * dealt with by functions which copy them or make use of the copies, but
 * also include miscellaneous local booleans set by command options.  The
 * FORWARD bit applies only to copying messages when composing, so there
 * is no conflict because of the overlap.
 */

/* copyback or copy_msg flags */
#define NO_HEADER	ULBIT(0)  /* Don't print header of message */
#define NO_IGNORE	ULBIT(1)  /* Don't ignore headers */
#define NO_PAGE		ULBIT(2)  /* Don't page this message */
#define INDENT		ULBIT(3)  /* Indent included msg with some string */
#define M_TOP		ULBIT(4)  /* Just print the top of msg (same as pre) */
#ifndef FORWARD /* zmcomp.h also defines this */
#define FORWARD		ULBIT(5)  /* Forward messages into compose buffer */
#endif /* FORWARD */
#define RETAIN_STATUS	ULBIT(6)  /* Don't change NEW status to OLD */
#define UPDATE_STATUS	ULBIT(7)  /* Write Status: header when copyback */
#define FOLD_ATTACH	ULBIT(8)  /* Show attachments as summaries only */
#define NO_SEPARATOR	ULBIT(9)  /* Don't include message separator lines */
#define GENERATE_INDEX	ULBIT(10) /* Don't actually write, compute sizes */
#define FULL_INDEX	ULBIT(11) /* Generate a full index of Msg contents */
#define REWRITE_ALL	ULBIT(12) /* Copyback with modify-in-place */
#define RETAINED_ONLY	ULBIT(13) /* Show only retained headers */
#define UNIGNORED_ONLY	ULBIT(14) /* Show only non-ignored headers */
#ifndef FORWARD_ATTACH 		  /* zmcomp.h also defines this (sigh) */
#define FORWARD_ATTACH	ULBIT(18) /* forward messages as attachments */
#endif /* FORWARD_ATTACH */
#define PINUP_MSG	ULBIT(19) /* this is for a pinup window */
#define PRIORITY_ONLY	ULBIT(20) /* Write priority only (for status_line) */
#define MODIFYING	ULBIT(21) /* invalidate Encoding & Content-Lines */
#define DELAY_FFLUSH	ULBIT(22) /* don't fflush() after each message */
#define TAGIT_MSG	ULBIT(23) /* this is a tagit window */
#define PHONETAG_MSG	ULBIT(24) /* this is a phonetag window */

/* flags to control folder changing or updating */
#define SUPPRESS_UPDATE	ULBIT(0) /* override the DO_UPDATE setting */
#define SUPPRESS_HDRS	ULBIT(1) /* don't display list of summaries */
#define READONLY_FOLDER	ULBIT(2) /* load or set folder as read-only */
#define ADD_CONTEXT	ULBIT(3) /* load this folder in a new context */
#define DELETE_CONTEXT	ULBIT(4) /* remove the context of this folder */
#define LIST_CONTEXTS	ULBIT(5) /* show one or more available contexts */
#define PERFORM_FILTER	ULBIT(6) /* apply the folder_filters to messages */
#define SUPPRESS_FILTER	ULBIT(7) /* do not perform filtering on folders */
#define REMOVE_CONTEXT	ULBIT(8) /* close the folder and remove the file */
#define CREATE_INDEX	ULBIT(9) /* update to an external index file only */
#define SUPPRESS_INDEX	ULBIT(10) /* open or update without an index */
#define MAIL_WATCH	ULBIT(11) /* watch folder for new mail */
#define NO_MAIL_WATCH	ULBIT(12) /* do NOT watch folder for new mail */
/* ULBIT(16) is TEMP_FOLDER */

/* Folder and message flags.  DO_UPDATE is permitted to serve double duty
 * here to indicate updating of individual messages within the folder.
 */

/* msg flags */
#define ZMF_NEW		ULBIT(0) /* used by specl_hdrs() and GUI mode */
#define ZMF_SAVED	ULBIT(1) /* when message has been saved */
#define ZMF_REPLIED	ULBIT(2) /* message has been replied to */
#define ZMF_RESENT	ULBIT(3) /* message was forwarded via outgoing mail */
#define ZMF_PRINTED	ULBIT(4) /* message was sent through lpr command */
#define ZMF_DELETE	ULBIT(5) /* delete from folder upon update */
#define ZMF_PRESERVE	ULBIT(6) /* preserve in folder unless deleted */
#define ZMF_UNREAD	ULBIT(7) /* XXX: why isn't this called READ ? */
#define ZMF_OLD		ULBIT(8) /* XXX: NEW should be an empty slate! */
#define ZMF_ATTACHED	ULBIT(9) /* Message has an attachment */
/*	DO_UPDATE	ULBIT(10) /* folder or message needs write back */
#define ZMF_MIME	ULBIT(11) /* MIME-format multimedia mail */
#define ZMF_INDEXMSG	ULBIT(12) /* Z-Mail folder index message */
#define ZMF_HIDDEN      ULBIT(15) /* not displayed in GUI mode  */
#define ZMF_EDITING     ULBIT(16) /* currently being edited externally */
#define ZMF_VISIBLE	ULBIT(17) /* message is explicitly NOT hidden */
#define ZMF_INDEXED	ULBIT(18) /* message was loaded via index */
#define ZMF_EXPUNGE	ULBIT(19) /* If IMAP, we just received an expunge
				     and we are processing it */

#define MsgIsNew(M)  		(ison((M)->m_flags, ZMF_NEW))
#define MsgIsSaved(M)  		(ison((M)->m_flags, ZMF_SAVED))
#define MsgIsReplied(M)  	(ison((M)->m_flags, ZMF_REPLIED))
#define MsgIsResent(M)  	(ison((M)->m_flags, ZMF_RESENT))
#define MsgIsPrinted(M)  	(ison((M)->m_flags, ZMF_PRINTED))
#define MsgIsDeleted(M)  	(ison((M)->m_flags, ZMF_DELETE))
#define MsgIsPreserved(M)  	(ison((M)->m_flags, ZMF_PRESERVE))
#define MsgIsUnread(M)  	(ison((M)->m_flags, ZMF_UNREAD))
#define MsgIsOld(M)  		(ison((M)->m_flags, ZMF_OLD))
#define MsgHasAttachments(M)  	(ison((M)->m_flags, ZMF_ATTACHED))
#define MsgIsMime(M)  		(ison((M)->m_flags, ZMF_MIME))
#define MsgIsIndexmsg(M)  	(ison((M)->m_flags, ZMF_INDEXMSG))
#define MsgIsHidden(M)  	(ison((M)->m_flags, ZMF_HIDDEN))
#define MsgIsEdited(M)  	(ison((M)->m_flags, ZMF_EDITING))
#define MsgIsVisible(M)  	(ison((M)->m_flags, ZMF_VISIBLE))
#define MsgIsIndexed(M)  	(ison((M)->m_flags, ZMF_INDEXED))

#ifndef _WINNT_
/* GREAT naming convention, Dan.  (You too, Bill.) */
#define NEW  	 	ZMF_NEW
#define SAVED  	 	ZMF_SAVED
#define REPLIED  	ZMF_REPLIED
#define RESENT  	ZMF_RESENT
#define PRINTED  	ZMF_PRINTED
#define DELETE  	ZMF_DELETE
#define EXPUNGE  	ZMF_EXPUNGE
#define PRESERVE  	ZMF_PRESERVE
#define UNREAD  	ZMF_UNREAD
#define OLD  		ZMF_OLD
#define ATTACHED  	ZMF_ATTACHED
#define MIME  		ZMF_MIME
#define INDEXMSG  	ZMF_INDEXMSG
#define HIDDEN  	ZMF_HIDDEN
#define EDITING  	ZMF_EDITING
#define VISIBLE  	ZMF_VISIBLE
#define INDEXED		ZMF_INDEXED
#endif /* !_WINNT_ */

/* Debugging doublechecks */
#define FINISHED_MSG	ULBIT(13) /* Lucky 13 */
#define STATUS_LINE	ULBIT(14) /* Needed for external indexing */

#define ALL		(~(u_long)0) /* currently used only by specl_hdrs() */

#define MsgSetPri(M, P) (turnon((M)->m_pri, (P)))
#define MsgClearPri(M, P) (turnoff((M)->m_pri, (P)))
#define MsgClearAllPri(M) (turnoff((M)->m_pri, ~PRI_MARKED_BIT))
#define MsgSetMark(M) (turnon((M)->m_pri, PRI_MARKED_BIT))
#define MsgClearMark(M) (turnoff((M)->m_pri, PRI_MARKED_BIT))
#define MsgIsMarked(M) (ison((M)->m_pri, PRI_MARKED_BIT))
#define MsgHasPri(M, P) (ison((M)->m_pri, (P)))

#define PRI_MARKED	0
#define PRI_MARKED_BIT	M_PRIORITY(PRI_MARKED)
#define PRI_UNDEF	31
#define PRI_UNDEF_BIT	M_PRIORITY(PRI_UNDEF)
#define PRI_COUNT	32
#define PRI_NAME_COUNT  31
#define PRI_FROM_LETTER(c) M_PRIORITY(upper(c)-'A'+1)
#define M_PRIORITY(n)	   ULBIT((n))

extern char *pri_names[PRI_NAME_COUNT];

/* this tells which priorities were not set in system.zmailrc */
extern u_long pri_user_mask;

#ifndef __cplusplus
extern int pri_index P((char *));
extern char *priority_string P((int));
#else
extern int pri_index(char *);
extern char *priority_string(int);
#endif /* !__cplusplus */

/* folder flags */
#define CONTEXT_IN_USE	ULBIT(0)  /* folder structure is available for use */ 
#define NEW_MAIL	ULBIT(1)  /* new mail has arrived */
#define READ_ONLY	ULBIT(2)  /* -r passed to folder() for read only */
#define CORRUPTED	ULBIT(3)  /* error loading new mail has occurred */
#define CONTEXT_RESET	ULBIT(4)  /* folder has been loaded or flushed */ 
#define GUI_REFRESH	ULBIT(5)  /* folder has changed, refresh displays */
#define BACKUP_FOLDER	ULBIT(6)  /* folder is a backup copy of a re-init */
#define UPDATE_F_LIST	ULBIT(7)  /* folder added or removed--update lists */
#define HAS_FULL_INDEX	ULBIT(8)  /* folder has a full index for loading */
#define IGNORE_INDEX	ULBIT(9)  /* ignore any index when loading */

#define DO_UPDATE	ULBIT(10) /* folder or message needs write back */
#define UPDATE_INDEX	ULBIT(11) /* folder index should be written back */
#define RETAIN_INDEX	ULBIT(12) /* always write folder index on update */

#define CONTEXT_LOCKED	ULBIT(13) /* temporary freeze on folder state */ 
#define DO_NOT_WRITE	ULBIT(14) /* writable folder in readonly state */
#define REFRESH_PENDING ULBIT(15) /* a refresh is about to happen */
#define TEMP_FOLDER	ULBIT(16) /* don't show in folder menu */

#define NO_NEW_MAIL	ULBIT(18) /* folder is NOT watched for new mail */

#define SORT_PENDING	ULBIT(19) /* an always-sort has been scheduled */
#define QUEUE_FOLDER	ULBIT(20) /* this is the folder of queued msgs */

#define REINITIALIZED	ULBIT(21) /* folder recently reloaded from scratch */

/* Constants for choose_one() (and PromptBox()) */
#define PB_MUST_MATCH	ULBIT(0)	/* Must == 1 for zm_ask() */
#define PB_FILE_BOX	ULBIT(1)
#define PB_FILE_OPTION	ULBIT(2)
#define PB_NO_TEXT	ULBIT(3)
#define PB_NO_ECHO	ULBIT(4)
#define PB_MUST_EXIST	ULBIT(5)
#define PB_NOT_A_DIR	ULBIT(6)
#define PB_MSG_LIST	ULBIT(7)
#define PB_TRY_AGAIN	ULBIT(8)
#define PB_EMPTY_IS_DEFAULT ULBIT(9)  /* "" == default resp., if dr exists */
#define PB_FOLDER_TOGGLE    ULBIT(10) /* filefinder "Folders Only" switch */
#define PB_FILE_READ	ULBIT(11)     /* GF: Mac open dlog, no text field */
#define PB_FILE_WRITE	ULBIT(12)     /* GF: Mac save dlog, gray file list */
#define PB_SHOW_EXEC	ULBIT(13)     /* show executables only */
#define PB_MULTIVAL	ULBIT(14)     /* allow multiple selections */
#define PB_NOIMAP	ULBIT(15)     /* don't enable IMAP toggle */

/* Constants for help() and gui_help() */
enum {
    HelpContext	   = ULBIT(0),
    HelpInterface  = ULBIT(1),
    HelpNoComplain = ULBIT(2),
    HelpCommands   = ULBIT(3),
    HelpContents   = ULBIT(4)
};

/* bits and pieces */
#ifndef turnon
#define turnon(flg,val)   ((flg) |= (u_long)(val))
#define turnoff(flg,val)  ((flg) &= ~(u_long)(val))
#endif /* !turnon */

#define none_p(flg,val)   (!((u_long)(flg) & (u_long)(val)))
#define any_p(flg,val)    (!none_p(flg,val))
#define all_p(flg,val)    (((u_long)(flg) & (u_long)(val)) == (u_long)(val))
#define only_p(flg,val)   (((u_long)(flg) & ~(u_long)(val)) == (u_long)0)

#define true_p(flg,val)   all_p(flg,val)
#define false_p(flg,val)  !true_p(flg,val)

/* old but pervasive stuff */
#ifndef isoff
#define isoff(flg,val)    none_p(flg,val)
#define ison(flg,val)     any_p(flg,val)
#endif /* !isoff */

/* 3/10/95 gsf -- workaround for bad MPW preproc expansion in set_message_flag */
#if defined(MAC_OS) && defined(DO_UPDATE)
# undef DO_UPDATE
# define DO_UPDATE	((u_long)((u_long)1 << (u_long)(10)))
#endif /* MAC_OS && DO_UPDATE */

#define set_message_flag(n, f)	\
	(isoff(msg[n]->m_flags, (f)) ? \
	    turnon(folder_flags, DO_UPDATE), \
	    turnon(msg[n]->m_flags, DO_UPDATE|(f)) : 0)
#define set_resent(n)	set_message_flag((n), RESENT)
#define set_replied(n)	set_message_flag((n), REPLIED)
#define set_isread(n)	  \
    if (turnon(msg[n]->m_flags, DO_UPDATE+OLD) && \
	    ison(msg[n]->m_flags, UNREAD|NEW)) \
	turnon(folder_flags, DO_UPDATE), turnoff(msg[n]->m_flags, UNREAD+NEW)
	
#endif /* _ZMFLAG_H_ */
