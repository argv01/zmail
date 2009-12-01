/* zmcomp.h		Copyright 1991 Z-Code Software Corp. */

#ifndef _ZMCOMP_H_
#define _ZMCOMP_H_

/*
 * $RCSfile: zmcomp.h,v $
 * $Revision: 2.76 $
 * $Date: 1998/12/07 22:49:53 $
 * $Author: schaefer $
 */

#ifndef OSCONFIG
#include <osconfig.h>
#endif /* OSCONFIG */

#include "config/features.h"
#include "gui_def.h"
#include "zctype.h"	/* Must come after gui_def.h (sigh) */
#include "zccmac.h"	/* Must come after gui_def.h (sigh) */
#include "linklist.h"
#include "attach.h"
#include "zfolder.h"
#include "zm_ask.h"
#include "zmaddr.h"
#include "zmflag.h"
#ifdef MOTIF
#include "motif/attach/area.h"
#endif /* MOTIF */
#ifdef VUI
# include <composef.h>
#endif /* VUI */

/* This structure should move to a more generic spot */

typedef struct HeaderField {
    struct link hf_link;
#define hf_name hf_link.l_name
    char *hf_body;	/* Eventually add or change to a union for various
			 * types of header, e.g. Dates, addresses, raw text,
			 * etc.
			 */
    u_long hf_flags;
#define DYNAMIC_HEADER	ULBIT(0)
} HeaderField;

/* Name of the variable used to return the value of a dynamic header */
#define VAR_HDR_VALUE "header_value"

/* Test for whether to expand an alias */
#define aliases_should_expand()	\
	(boolean_val(VarAlwaysexpand) && !boolean_val(VarNoExpand))

/* Test for whether to do $record_users, and if for all users */
#define record_users(c) \
	(ison((c)->send_flags, RECORDUSER)? (1 + \
	!!chk_option(VarRecordUsers, "all")) : 0)

#ifdef _COMPOSE_C_
/* We define these names here for clarity, but they are only needed
 * in compose.c.  Other files use the extern and indices defined below.
 */
char *address_headers[] = {
    "Subject", "To", "Cc", "Bcc", "Fcc",
};
#else /* _COMPOSE_C_ */
extern char *address_headers[];
#endif /* _COMPOSE_C_ */

#define SUBJ_ADDR	0	/* Not strictly an address, but ... */
#define TO_ADDR		1
#define CC_ADDR		2
#define BCC_ADDR	3
#define FCC_ADDR	4
#define NUM_ADDR_FIELDS	5

/* Magic header that holds envelope addresses, for use in outbound queue */
#define ENVELOPE_HEADER	"X-Zm-Envelope-To"

enum mref_type {
    mref_Reply, mref_Delete, mref_Forward
};

/* Bart: Sun Jun 14 18:18:05 PDT 1992 */
typedef struct mref {
    struct link link;
    u_long offset;
    enum mref_type type;
} msg_ref;

typedef struct Compose {
    struct link link;
/* Fields that actually comprise the message itself */
    char *addresses[NUM_ADDR_FIELDS];
    HeaderField *headers;
    long body_pos;
    Attach *attachments;
/* Fields associated with the composition of the message */
    char *rrto;			/* Save this separately to toggle ~R */
    msg_group replied_to;
    msg_ref *reply_refs;	/* Bart: Sun Jun 14 18:20:07 PDT 1992 */
    msg_group inc_list;
    char *hfile;
    char *Hfile;
    char *edfile;
    FILE *ed_fp;
    int exec_pid;
    char	*boundary;   /* MIME encapsulation boundary */
/* Flags to control composition */
    u_long flags;
#define INIT_HEADERS	ULBIT(0)  /* Outgoing headers need initialization */
#define IS_REPLY	ULBIT(1)  /* This message is a reply to another */
#define NEW_SUBJECT	ULBIT(2)  /* new subject regardless of $ask (mail -s) */
#define INCLUDE		ULBIT(3)  /* include msg in response */
#define INCLUDE_H	ULBIT(4)  /* include msg with header */
#ifndef FORWARD /* zmflag.h also defines this (sigh) */
#define FORWARD		ULBIT(5)  /* forward messages into the message buffer */
#endif /* FORWARD */
#define ACTIVE		ULBIT(6)  /* Composition is in progress */
#define DOING_INTERPOSE	ULBIT(7)  /* An interposer e.g. on send in progress */
#define EDIT		ULBIT(8)  /* enter editor by default on mailing */
#define EDIT_HDRS	ULBIT(9)  /* user edits headers using editor */
#define SORT_ADDRESSES	ULBIT(10) /* Sort addresses for display */
#define DIRECTORY_CHECK	ULBIT(11) /* Check addresses against directory */
#define SENDTIME_CHECK	ULBIT(12) /* Directory-check at send time required */
#define CONFIRM_SEND    ULBIT(13) /* Prompt before sending ($verify) */
#define AUTOCLEAR	ULBIT(14) /* Clear the screen/window after sending */
#define AUTOFORMAT	ULBIT(15) /* Useless, but we have it here for GUI */
/*#define ASK_DYNAMIC	ULBIT(16)  Dynamic headers should be prompted */
#define MODIFIED        ULBIT(17) /* Composition has been modified */
#ifndef FORWARD_ATTACH /* zmflag.h also defines this (sigh) */
#define FORWARD_ATTACH	ULBIT(18)  /* forward messages as attachments */
#endif /* FORWARD_ATTACH */
#define COMP_BACKGROUND	ULBIT(19) /* Start composition in background */

    u_long send_flags;
#define SIGN		ULBIT(0)  /* Auto-include ~/.signature in mail */
#define NO_SIGN		ULBIT(1)  /* Override SIGN and DO_FORTUNE */
#define DO_FORTUNE	ULBIT(2)  /* Add a fortune at end of msgs */
#define RETURN_RECEIPT	ULBIT(3)  /* Add Return-Receipt-To: header */
#define NO_RECEIPT	ULBIT(4)  /* Suppress Return-Receipt-To: header */
#define RECORDING	ULBIT(5)  /* Record outgoing mail in $record */
#define LOGGING		ULBIT(6)  /* Log outgoing headers in $logfile */
#define RECORDUSER	ULBIT(7)  /* Record outgoing mail in $folder/<user> */
#define SYNCH_SEND	ULBIT(8)  /* Synchronous send requested */
#undef VERBOSE			/* silence a "redefined" warning */
#define VERBOSE		ULBIT(9)  /* Verbose flag for sendmail */
#define DIRECT_STDIN    ULBIT(10) /* Copy stdin directly to MTA, no tempfile */
#define SEND_NOW	ULBIT(11) /* Send without further changes */
#define SEND_CANCELLED	ULBIT(12) /* Interposer called "compcmd cancel" */
#define SEND_KILLED	ULBIT(13) /* Interposer called "compcmd kill" */
#define NEED_CLEANUP	ULBIT(14) /* No fork, stop_compose() cleans up */
#define SEND_ENVELOPE	ULBIT(15) /* Don't rewrite Date: etc. headers */
#define SEND_Q_P	ULBIT(16) /* Message body has been q-p encoded */
#define PHONE_TAG	ULBIT(17) /* Is a phone tag message */
#define TAG_IT		ULBIT(18) /* Is a tag it message */
#define READ_RECEIPT	ULBIT(19) /* Send with return read receipt requested */
#ifdef GUI
# ifdef VUI
    struct zmlcomposeframe *compframe;
# endif /* VUI */
# ifdef _WINDOWS
    struct ZComposeFrame *compframe;
# endif /* _WINDOWS */
# ifdef MAC_OS
    struct TCompFrameView *compframe;
# endif /* MAC_OS */
# ifdef MOTIF
    struct ComposeInterface *interface;
# endif /* MOTIF */
#endif /* GUI */
    char *transport;		/* Program to use to deliver this composition */
    char *record;		/* File name other than $record for RECORDING */
    char *logfile;		/* File name other than $logfile for LOGGING */
    char *remark;		/* X-Remarks: body (also temp work space) */
    char *signature;		/* signature to use if $autosign */
    char *sorter;		/* Program for sorting addresses */
    char *addressbook;		/* Address-book program for lookups */
    u_long mta_flags;
#define MTA_ADDR_ARGS	ULBIT(0) /* MTA takes addresses on command line */
#define MTA_ADDR_STDIN	ULBIT(1) /* MTA takes addresses on stdin (submit) */
#define MTA_ADDR_FILE	ULBIT(2) /* MTA must get addresses from file */
#define MTA_ADDR_UUCP	ULBIT(3) /* MTA wants addresses in uucp format */
#define MTA_HDR_PICKY	ULBIT(4) /* MTA doesn't like From & Date headers */
#define MTA_NO_COMMAS	ULBIT(5) /* MTA vomits on comma-separated addresses */
#define MTA_EIGHT_BIT	ULBIT(6) /* MTA handles 8-bit data cleanly */
#define MTA_HIDE_HOST	ULBIT(7) /* MTA accepts bare <user> in MAIL FROM: */
#if defined(MOTIF) || defined(_WINDOWS)
    int autosave_ct; 		/* Modification count (for autosaving) */
#endif /* MOTIF || _WINDOWS */
    struct zmInterposeTable *interpose_table;
    	   			/* composition-specific interposers */
#ifdef PARTIAL_SEND
    unsigned long splitsize;
#endif /* PARTIAL_SEND */
    mimeCharSet     out_char_set;	/* enumerated charset */
} Compose;

extern void reset_comp_opts P((Compose *, int));
extern void update_comp_struct P((Compose *));

/* XXX -- OLIT still uses this */
#define COMP_FILENAME        1

extern struct Compose *comp_list, *comp_current;

/* Return codes for gui_edmail() */
#define EDMAIL_ABORT	      (-1)
#define EDMAIL_COMPLETED	0
#define EDMAIL_STATESAVED	1
#define EDMAIL_UNCHANGED	2

/* Return codes for add_to_letter() [compcmd] */
#define COMPCMD_SUSPEND	      (-2)
#define COMPCMD_FAILURE	      (-1)
#define COMPCMD_FINISHED	0
#define COMPCMD_NORMAL		1
#define COMPCMD_WARNING		2

/* The remainder of this file describes the composition interface */ 

/* PROBLEMS TO SOLVE/THINGS TO DO:
 *
 *	What to do about ~t, ~c, etc.?  wrap_addrs() on each command??
 */

/*
 * addrs.c     Copyright 1990, 1991 Z-Code Software Corp.
 *
extern int
addr_count( char *p );
 *	Count the number of addresses in a string of several.
 *
extern char **
addr_vec( char *s );
 *	Convert a string of several addresses to a vector of addresses.
 *
extern char *
alias_to_address( char *s );
 *	Convert a list of alias names to real addresses.
 *
extern char *
bang_form( char *d, char *s );
 *	Construct a !-style form of an address.
 *
extern int
compare_addrs( char *list1, char *list2, char *ret_buf );
 *	Check that all addresses in list1 match address patterns in list2.
 *	Place in ret_buf the first address that fails to match.
 *
extern int
fix_up_addr( char *str );
 *	Force a list of addresses to be well-formed and comma-separated.
 *
extern char *
get_name_n_addr( char *str, char *name, char *addr );
 *	Get name and address from a well-formed address.
 *
extern int
improve_uucp_paths( char *original, int size, char *route_path, int fix_route );
 *	Perform path minimization and domain short-circuiting on UUCP addresses.
 *
extern char *
message_id( char *buf );
 *	Create a message-id field-body and write it in buf.  The <> that
 *	normally appear around a message-id are not included.
 *
extern char *
message_boundary();
 *	Allocate an encapsulation boundary for MIME attachment parts.
 *
extern void
prepare_mta_addrs( char *str, unsigned long how );
 *	Convert all addresses to MTA input format according to flags `how'.
 *
extern int
rm_cmts_in_addr( char *str );
 *	Remove all comment fields from a list of addresses.
 *
extern int
rm_redundant_addrs( char *to, char *cc );
 *	Remove any duplicate addresses from a list of addresses.
 *
extern int
route_addresses( char *to, char *cc, char *route_path );
 *	Construct the reply paths for addresses in the To: and Cc: lines.
 *
extern int
take_me_off( char *str );
 *	Take all alternate names for the user out of the list of addresses.
 *
extern char *
unscramble_addr( char *addr, char *naddr );
 *	Unwind particularly brain-damaged RFC822-ish addresses.
 *
extern char *
wrap_addrs( char *str, int n );
 *	Insert newlines in RFC822 continuation form into a list of addresses.
 */

/*
 * compose.c	Copyright 1991, 1992 Z-Code Software Corp.
 *
extern void
add_address( Compose *compose, int hdr_index, char *value )
 *	Append a new address to the indexed header.  SUBJ_ADDR is ignored.
 *	
extern void
add_headers( FILE *files[], int size, int log_file );
 *	Adds completed headers from comp_current to the MTA and other output
 *	files.  Removes Bcc: in all cases.  Removes From: and Date: if MTA
 *	is picky.  Adds final content information headers when attachments.
 *	Adds Status: when writing to files (other than MTA).
 *
extern HeaderField *
create_header( char *name, char *body );
 *	Allocates a HeaderField and assigns name and body.  Does NOT
 *	allocate name and body.  See destroy_header().
 *
extern HeaderField *
duplicate_header( HeaderField *hf );
 *	Allocates a HeaderField and duplicates the name and body of hf.
 *	Intended for passing to copy_all_links() (see linklist.h).
 *
extern void
destroy_header( HeaderField *hf );
 *	Free a HeaderField.  Also frees its components.  See create_header().
 *
extern void
free_headers( HeaderField **hf );
 *	Calls destroy_header() on every HeaderField in a list.
 *
extern int
generate_addresses( char **to_list, char *route, int all, msg_group *list );
 *	Store the argument addresses and add any drawn from replied-to
 *	messages.  If allowed to do user input, prompt for any others.
 *
extern long
generate_headers( Compose *compose, FILE *fp, int merge );
 *	If initializing, call personal_headers().  If using a header file,
 *	merge in the headers from that file.  Call mta_headers().
 *	Return the seek position of the message body, or -1 on error.
 *
extern char **
get_address( Compose *compose, int hdr_index )
 *	Retrieve the addresses from the indexed address header.  Returns
 *	an array of strings, one address per string.  As a special case,
 *	the SUBJ_ADDR is returned as pointer to a one-element array.
 *
extern char *
get_envelope_addresses( Compose *compose )
 *	Grody hack to dig the list of addresses out of the ENVELOPE_HEADER
 *	instead of out of the real addressing headers.  Has the side-effect
 *	of deleting the ENVELOPE_HEADER from the message.
 *
extern void
input_address( int h, char *p );
 *	Given an index into comp_current->addresses and a possible value,
 *	either set the value or query the user for one.
 *
extern HeaderField *
lookup_header( HeaderField **hf, char *name, char *join, int crunch );
 *	Searches a HeaderField list for the indicated header.  Combines
 *	same-name headers into one using join as separator.  If crunch,
 *	modifies the list to contain the unified header and returns the
 *	unified header.  Otherwise, allocates a new header for the result
 *	and returns that.
 *
extern void
merge_headers( HeaderField **hf1, HeaderField **hf2, char *join, int crunch );
 *	Combines two HeaderField lists.  If crunch, items from hf2 with names
 *	already represented in hf1 are discarded.  If join, same-name items
 *	in the merged list are unified into single items using join.
 *
extern void
mta_headers( Compose *compose );
 *	Makes sure a HeaderField list contains all necessary and valid headers
 *	for use as the envelope of a mail message.  If initializing, places
 *	the information from generate_addresses() in the appropriate headers,
 *	then turns off initializing flag.  Adds Return-Receipt-To and other
 *	special headers that may have been requested.  Does NOT remove From:
 *	and Date: for picky mailers.  See personal_headers(), add_headers().
 * 
extern int
output_headers( HeaderField **hf, FILE *fp, int all );
 *	Prints a HeaderField list.  If fp is 0, uses zm_pager(), otherwise,
 *	prints to fp.  If all is true, includes blank headers.
 * 
extern void
personal_headers( Compose *compose );
 *	Generates the personal headers (my_hdr) and adds them to the list.
 *	For replies, generates the In-Reply-To: and References: headers.
 * 
extern void
request_receipt( Compose *, int on, char *p );
 *	Given an on-or-off indication and an address (ignored if not on),
 *	add (or remove) the Return-Receipt-To: header.
 *
extern void
reset_compose( Compose *compose );
 *	Reset the indicated composition to the state it was in just after
 *	generate_addresses(), to re-use the addresses for another message.
 *
extern void
resume_compose( Compose *compose );
 *	Calls compose_mode(), and turns on IS_GETTING.  If the composition
 *	it is asked to resume is not the current one, suspends the current.
 *
extern void
set_address( Compose *compose, int hdr_index, const char **value )
 *	Assign a new value to the indexed header.  The new value is passed
 *	as an array of strings, one or more addresses per string.  As a
 *	special case, the SUBJ_ADDR should be passed as a pointer to a
 *	pointer to a single string.
 *	
extern char *
set_header( char *str, const char *curstr, int do_prompt );
 *	Prompt for the value of a header, supplying a default answer.  This
 *	function is currently a no-op in GUI mode.
 *
extern int
start_compose( void );
 *	Allocates and initializes a Compose structure, including initializing
 *	message groups for replies and inclusions.  Calls compose_mode(), and
 *	adds the composition to the job list.
 *
extern void
stop_compose( void );
 *	Destroys a compose structure, freeing all fields and destroying the
 *	reply and inclusion message groups.  Removes the composition from
 *	the jobs list.  Calls compose_mode(), turns off the IS_GETTING flag.
 *
extern long
store_headers( HeaderField **fields, FILE *fp );
 *	Reads headers from a file, omitting a leading From_ line.  Stores the
 *	headers in a HeaderField list.  Returns the seek position of the end
 *	of the message headers.
 *
extern void
suspend_compose( Compose *compose );
 *	Turns off IS_GETTING, adjusts the jobs list, calls compose_mode().
 *
extern int
validate_from( char *from );
 *	Checks an address to be sure we believe the user is who he says.
 */

/*
 * edmail.c   Copyright 1992 Z-Code Software Corp.
 *
extern char *
address_book( char *s );
 *	Check addresses (one or more, in s) against an external "directory
 *	service" or address-book program.
 *
extern char **
addr_list_sort( char **addrs );
 *	Sort a vector of addresses.  Frees its input and returns a new
 *	vector as output, so usage is:  addrs = addr_list_sort(addrs);
 *
extern AskAnswer
confirm_addresses( Compose *compose );
 */

/*
 * mail.c     Copyright 1990, 1991 Z-Code Software Corp.
 *
extern int
add_to_letter( char line[], int noisy );
 *	Adds line to message.  If line indicates change of headers, changes
 *	them.  If line indicates start of editor, calls run_editor().  If
 *	line is end of input, calls finish_up_letter().  If noisy, prints
 *	"(continue editing message)" after tilde-commands.
 *
extern int
close_edfile( Compose *compose );
 *	Closes the editor file and resets compose->ed_fp.  Returns 0 on
 *	success, -1 on error.
 *
extern int
compose_letter( void );
 *	Loop around add_to_letter().  Turns on IS_GETTING, then loops to
 *	get input and calls add_to_letter() for each line.
 *
extern void
compose_mode( int on );
 *	Sets up (or removes) signal handling for composition mode.
 *
extern int
extract_addresses( char *addr_list, char **names, int next_file, int size );
 *	Copies all the addresses from the current composition into addr_list,
 *	expanding aliases and removing file names (which are stored in names,
 *	beginning at index next_file).  Signs the letter if appropriate.
 *	Returns the next available index in names, -1 on error.
 *	NOTE:  Modifies comp_current, because it expands aliases and removes
 *	file names.  See send_it().
 *
extern int
finish_up_letter( void );
 *	If forwarding, call send_it(), then return.  Otherwise, do askcc and
 *	verify, then call send_it(); if send_it() succeeds, stop_compose().
 *
void
invoke_editor( char *edit );
 *	Runs editor for comp_current.  Turns off EDIT flag on success.
 *
extern int
init_attachment( const char *attachment );
 *	Parses type:file and adds the attachment to comp_current.  Does not
 *	cause loading or encoding of the attachment.  See send_it().
 *
extern int
open_edfile( Compose *compose );
 *	Reopens the editor file and assigns compose->ed_fp.  Returns 0 on
 *	success, calls error() and returns -1 on failure.
 *
extern int
parse_edfile( Compose *compose );
 *	Loads message headers from the editor file if necessary.
 *
extern int
prepare_edfile( void );
 *	Sets up the editor file for autoedit or ~v/~e.  When EDIT_HDRS, calls
 *	output_headers() then appends the current message body and changes
 *	comp_current->edfile to the name of the new file.  Otherwise, uses
 *	the existing editor file.  Unless file preparation fails, closes
 *	comp_current->ed_fp and sets it to 0.  Returns 0 on success, or -1.
 * 
extern int
reload_edfile( void );
 *	Reopens the editor file and loads the message headers if necessary.
 *
extern int
rm_edfile( int sig );
 *	Handles interrupt signals and exits.  Closes comp_current->ed_fp,
 *	unlinks comp_current->edfile, calls stop_compose().
 *
extern int
run_editor( char *edit );
 *	Calls prepare_edfile().  If that succeeds, runs editor, then calls
 *	reload_edfile().  Returns 0 on success, -1 on error.
 *
static int
send_it( void );
 *	 1. Call mta_headers() to be sure all headers are correct.
 *	 2. Construct MTA command (without address arguments).
 *	 3. Call extract_addresses() to handle aliases, files, signatures.
 *	 4. Fork:  Parent closes/zeros comp_current->ed_fp and returns 0.
 *		   Child continues.
 *	    Fork failed:  returns -1.
 *	    No fork (debugging):  continues.
 *	 5. Call attach_files().  If fails, either rm_edfile() and exit(),
 *	    or if no fork, return -1.
 *	 6. Open MTA (stdout if debugging).  If fails, either rm_edfile()
 *	    and exit(), or if no fork, return -1.
 *	 7. Open record and logfiles.
 *	 8. Call add_headers().
 *	 9. Copy message body to MTA and all files.  If MTA fails, call
 *	    dead_letter().
 *	10. Close MTA and (unless no fork) call rm_edfile().
 *	11. If no fork, return 0, else exit.
 *
static int
start_file( msg_group *list );
 *	Create the editing tempfile (if any).  If DIRECT_STDIN, return the
 *	value of send_it(). Otherwise, set EDIT_HDRS as needed, copy hfile
 *	into comp_current->ed_fp (if necessary), then call generate_headers().
 *	Append Hfile (if any) to comp_current->ed_fp, then handle inclusions
 *	or forwards of messages in list.  If not SEND_NOW, turn off FORWARD.
 *	If EDIT, run_editor(), else if SEND_NOW, finish_up_letter() and if
 *	not GUI, return.  If GUI, prepare_edfile(), suspend_compose(), and
 *	open_compose(), then return, otherwise return compose_letter().
 *	NOTE:  GUI is responsible for reload_edfile() when it is finished.
 *
extern int
zm_mail( int n, char **argv, msg_group *list );
 *	Calls start_compose(), parses arguments to initialize flags, checks
 *	validity of combinations.  Calls generate_addresses().  If forwarding
 *	and sending immediately, loops calling start_file().  Otherwise, calls
 *	start_file() and returns what it returns.
 */

/* Prototypes that used to be here; now moved down into other headers. */
#include "addrs.h"
#include "attach.h"
#include "compose.h"
#include "dirserv.h"
#include "edmail.h"
#include "mail.h"

/* Just in case somebody was actually using the old uucp_only() function */
#define uucp_only(x)	prepare_mta_addrs(x, MTA_ADDR_UUCP|MTA_ADDR_FILE)

#ifdef MOTIF
#include "zmframe.h"
extern Compose *FrameComposeGetComp P((ZmFrame));
extern Widget *FrameComposeGetItems P((ZmFrame));
#endif /* MOTIF */

#endif /* _ZMCOMP_H_ */
