/* copy_msg.c	Copyright 1992-94 Z-Code Software Corp. */

#ifndef lint
static char	copy_msg_rcsid[] = "$Id: copy_msg.c,v 2.130 2005/05/31 07:36:40 syd Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "copy_msg.h"
#include "file.h"
#include "glob.h"
#include "linklist.h"
#include "mimehead.h"
#include "pager.h"
#include "prune.h"
#include "strcase.h"
#include "fsfix.h"

#if defined(DARWIN)
#include <stdlib.h>
#include <libgen.h>
#endif

#ifdef C3
#ifndef MAC_OS
#include "c3/dyn_c3.h"
#else /* MAC_OS */
#include "dyn_c3.h"
#endif /* MAC_OS */
#endif /* C3 */

#include <general.h>
#include <dynstr.h>
#include <except.h>

#include "zmindex.h"

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else /* !HAVE_STDARG_H */
#ifndef va_dcl
#include <varargs.h>
#endif /* !va_dcl */
#endif /* HAVE_STDARG_H */

static int ix_cmp_num P ((const char *, const char *));
static int ix_cmp_off P ((const char *, const char *));

extern void parse_pattern();
extern long qp_decode();

typedef struct cp_pos {		/* State for position within message */
    char	 msg_hdr;
    char	 msg_bdy;
    char	 apt_hdr;
    char	 apt_bdy;
    char	 cont_ln;
    char	 on_boundary;
} cp_pos;

typedef struct cp_pat {		/* State for pattern searching */
    char	*beg_pat;
    char	*end_pat;
    int		 pat_len;
    char	 at_beg;
    char	 at_end;
} cp_pat;

typedef struct cp_state {	/* Control state for copying message */
    u_long	 flags;		/* Control flags (see zmail.h) */
    FolderType   rdtype;	/* Type of folder being read */
    FolderType   wrtype;	/* Type of folder being written */
    Msg		*mesg;		/* Description of message to copy */
    Attach	*attach;	/* Work area for walking attachments */
    cp_pos	 pos;		/* Current postion within message */
    cp_pat	 pat;		/* Search pattern description */
    int		 lines;		/* Number of output lines generated */
    long	 bytes;		/* Number of input bytes copied */
    int		 top;		/* Number of lines to display (0 == all) */
    int		 squeeze;	/* Boolean and counter for squeeze */
    char	 ignoring;	/* Work area for ignore operation */
    char	 paging;	/* Boolean: Send output to the pager? */
    char	*indent_str;	/* String to indent each line */
    Msg		*result;	/* Description of result of copy */
    char	*worksp;	/* Work area for copying operation */
    char	*boundary;	/* Top-level MIME interpart boundary param */
    mimeCharSet	in_char_set;	/* Assumed (default) input character set */
    mimeCharSet	out_char_set;	/* Current output character set */
    mimeCharSet	att_char_set;	/* Attachment's character set, if known */
} cp_state;

typedef enum cp_control {
    CpAttachment,	/* Not settable */
    CpBytes,		/* Not settable */
    CpFlags,
    CpFolderType,
    CpReadFolderType,
    CpWriteFolderType,
    CpIgnoring,		/* Not settable, see CpFlags */
    CpIndentStr,	/* Not settable, see CpFlags and VarIndentStr */
    CpLines,		/* Not settable */
    CpMessage,
    CpPaging,
    CpPattern,
    CpPosition,		/* Not settable */
    CpResult,
    CpSqueezing,	/* Not currently settable, see VarSqueeze */
    CpTopLines,		/* Not currently settable, see VarTopLines */
    CpWorkspace,	/* Settable only as an optimization */
    CpInCharSet,
    CpOutCharSet,
    CpDoNotTouch,	/* Add above this */
    CpEndArgs		/* Don't move this */
} cp_control;

static void fix_char_set P ((cp_state *));
static int test_ignore P ((char *, cp_state *));
static int dumpMsgIx P ((msg_index *, FILE *));
static int dumpMsg P ((Msg *, FILE *, int));
static int fillMsg P ((Msg *, char *, Source *));
static int dumpAttachList P ((Attach *, FILE *, long));
static int fillAttachList P ((Attach **, Source *));
static int fillAttach P ((Attach *, char *, Source *));
extern int dumpAttach P ((Attach *, FILE *, long));

/* Initialize a cp_state.  The following fields may be set:
 *	CpFlags		u_long		Describe type of copying to perform.
 *	CpFolderType	FolderType	The type of folder being *written*.
 *	CpIndentStr	char *		Indent each line with this string.
 *	CpMessage	Msg *		The message being copied.
 *	CpPaging	int		Determine whether the pager is used.
 *	CpPattern	char *		Used as a pattern to search for.
 *	CpResult	Msg *		Description of result of copy.
 *	CpInCharSet	char *		Default input character set name
 *	CpOutCharSet	char *		Output character set name
 *
 * The special value CpDoNotTouch can be followed by a field specifier to
 * force that field to remain uninitialized.  Otherwise, the following fields
 * are reset as indicated:
 *	CpAttachment			Set to attachment list of CpMessage.
 *	CpBytes				Set to 0.
 *	CpFlags				Cleared.
 *	CpFolderType			Set to FolderStandard.
 *	CpIgnoring			Set to FALSE.
 *	CpIndentStr			Cleared.
 *	CpLines				Set to 0.
 *	CpMessage			Cleared.
 *	CpPaging			Set to FALSE.
 *	CpPattern			Cleared.
 *	CpPosition			Set to beginning of message headers.
 *	CpResult			Cleared.
 *	CpSqueezing			Set to value of VarSqueeze.
 *	CpTopLines			Set to value of VarTopLines.
 *	CpWorkspace			Set to malloc'd work area.
 *
 * Returns its first argument (pointer to the initialized cp_state).
 */
cp_state *
#ifdef HAVE_STDARG_H
init_cps(struct cp_state *cps, ...)
#else /* !HAVE_STDARG_H */
init_cps(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    int i;
    char *p;
    char scratch[(int)CpEndArgs]; /* Booleans */
    cp_control arg = CpEndArgs;
    va_list args;
#ifndef HAVE_STDARG_H
    cp_state *cps;

    va_start(args);
    cps = va_arg(args, cp_state *);
#else /* HAVE_STDARG_H */
    va_start(args, cps);
#endif /* !HAVE_STDARG_H */

    if (!cps)
	return 0;
    bzero((char *) scratch, (int)CpEndArgs);

    while ((arg = va_arg(args, cp_control)) != CpEndArgs) {
	switch ((int)arg) {
	    case CpDoNotTouch :
		/* Get the next arg to set its scratch entry */
		arg = va_arg(args, cp_control);
		if (arg == CpPattern && cps->pat.beg_pat) {
		    /* We still touch the pattern, but carefully */
		    if (!cps->pat.end_pat) {
			cps->pat.at_beg = !*(cps->pat.beg_pat);
			cps->pat.at_end = FALSE;
		    } else
			cps->pat.at_beg = cps->pat.at_end = FALSE;
		    cps->pat.pat_len = strlen(cps->pat.beg_pat);
		}
	    when CpFlags :
		cps->flags = va_arg(args, u_long);
	    when CpFolderType :
		cps->rdtype = cps->wrtype = va_arg(args, FolderType);
	    when CpReadFolderType :
		cps->rdtype = va_arg(args, FolderType);
	    when CpWriteFolderType :
		cps->wrtype = va_arg(args, FolderType);
	    when CpIndentStr :
		cps->indent_str = va_arg(args, char *);
	    when CpMessage :
		cps->mesg = va_arg(args, Msg *);
	    when CpPaging :
		cps->paging = va_arg(args, int);
	    when CpPattern :
		parse_pattern(va_arg(args, char *), &cps->pat);
	    when CpResult :
		cps->result = va_arg(args, Msg *);
	    when CpWorkspace :
		cps->worksp = va_arg(args, char *);
	    when CpInCharSet :
		cps->in_char_set = va_arg(args, mimeCharSet);
	    when CpOutCharSet :
		cps->out_char_set = va_arg(args, mimeCharSet);
	    otherwise:
		error(ZmErrWarning, catgets( catalog, CAT_MSGS, 213, "Improper cp_control" ));
		continue;
	}
	scratch[(int)arg] = 1;
    }

    /* Need to do this first */
    if (!scratch[(int)CpFlags])
	cps->flags = NO_FLAGS;
    if (!scratch[(int)CpMessage])
	cps->mesg = 0;

    /* When updating to a folder, always write all headers! */
    if (ison(cps->flags, UPDATE_STATUS)) {
	turnon(cps->flags, NO_IGNORE);
	scratch[(int)CpFlags] = 1;
    /* historic behavior: if alwaysignore='', ignore all headers,
     * unless we are resending.  (historically, NO_IGNORE was only
     * used for Read, Print, and friends.)
     */
    } else if (ison(cps->flags, NO_IGNORE) &&
	    isoff(cps->flags, FORWARD) &&
	    (p = value_of(VarAlwaysignore)) && !*p) {
	turnoff(cps->flags, NO_IGNORE);
    }

    for (i = 0; i < (int)CpEndArgs; i++) {
	if (scratch[i])
	    continue;
	switch (i) {
	    case CpAttachment :
		if (cps->mesg && cps->mesg->m_attach &&
			(p = cps->mesg->m_attach->content_type) &&
			ci_strcmp(p, "text") != 0) {
		    cps->attach = cps->mesg->m_attach;
		    /* Bart: Thu Jun 30 17:05:49 PDT 1994
		     * This doesn't really belong here, but this is where we
		     * need to be absolutely certain that it is set, so ....
		     *
		     * THIS SHOULD GO AWAY WHEN Content-Disposition IS ADDED!!
		     */
		    if (isoff(cps->attach->a_flags, AT_PRIMARY)) {
			turnon(cps->attach->a_flags, AT_PRIMARY);
			if (is_multipart(cps->attach) &&
			    is_plaintext((Attach *)cps->attach->a_link.l_next))
			    turnon(((Attach *)
				    cps->attach->a_link.l_next)->a_flags,
				AT_PRIMARY);
		    }
		} else
		    cps->attach = 0;
	    when CpBytes :
		cps->bytes = 0;
	    when CpFolderType :
		if (!scratch[(int)CpReadFolderType])
		    cps->rdtype = FolderStandard;
		if (!scratch[(int)CpWriteFolderType])
		    cps->wrtype = FolderStandard;
	    when CpIgnoring :
		cps->ignoring = 0;
	    when CpIndentStr :
		cps->indent_str = 0;
	    when CpLines :
		cps->lines = 0;
	    when CpPaging :
		cps->paging = FALSE;
	    when CpPattern :
		bzero((char *) &cps->pat, sizeof(cp_pat));
	    when CpPosition :
		bzero((char *) &cps->pos, sizeof(cp_pos));
		cps->pos.msg_hdr = TRUE;
	    when CpResult :
		cps->result = 0;
	    when CpSqueezing :
		if (isoff(cps->flags, NO_IGNORE))
		    cps->squeeze = boolean_val(VarSqueeze);
		else
		    cps->squeeze = 0;
	    when CpTopLines :
		if (ison(cps->flags, M_TOP)) {
		    p = value_of(VarToplines);
		    cps->top = (p)? atoi(p) : crt;
		}
	    when CpWorkspace :
		cps->worksp = savestr(cps->indent_str);
	    when CpInCharSet :
		cps->in_char_set = inMimeCharSet;
	    when CpOutCharSet :
		cps->out_char_set = currentCharSet;
	    otherwise:
		continue;
	}
    }

    if (cps->pat.beg_pat)
	turnoff(cps->flags, FOLD_ATTACH);

    if (cps->mesg && cps->attach && MsgIsMime(cps->mesg) &&
	    ison(cps->flags, MODIFYING))
	cps->boundary = FindParam(MimeGlobalParamStr(BoundaryParam),
				  &cps->attach->content_params);
    else
	cps->boundary = 0;
    fix_char_set(cps);
    
    return cps;
}

/* The only guarantee about clean_cps() is that it won't touch the
 * line count or the byte count, and won't free cps itself.
 *
 * Everything else is fair game.
 */
void
clean_cps(cps)
struct cp_state *cps;
{
    xfree(cps->worksp);
}

/*
 *-------------------------------------------------------------------------
 *
 *  can_show_inline --
 *	Given a pointer to an attachment structure, check whether it can be
 *	displayed in line in a message reading window.
 *
 *  Results:
 *	Return true if the attachment can be displayed or it is NULL.
 *	Return false otherwise.
 *
 *  Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
/* #define MAX_ATTACH_SHOW (10*BUFSIZ) */

int
can_show_inline(attachPtr) 
    const Attach	*attachPtr;
{
    return (!attachPtr ||
#ifdef MAX_ATTACH_SHOW
	    attachPtr->content_length <= MAX_ATTACH_SHOW &&
#endif /* MAX_ATTACH_SHOW */
	    (is_plaintext(attachPtr) ||
	     /* XXX Until we interpret nested parts, print some parts inline.
	      * This violates the MIME spec, as it means that encoded parts
	      * and boundaries will be shown, but for the common case of e.g.
	      * an attached plaintext message, it is nice to see it inline.
	      * Some people (like Dan) think this behavior is better.
	      */
	     (is_inline_type(attachPtr->data_type) &&
	      has_simple_encoding(attachPtr))) &&
	    isoff(attachPtr->a_flags, AT_NOT_INLINE) &&
	    (any_p(attachPtr->a_flags, AT_PRIMARY|AT_TRY_INLINE) ||
	     !boolean_val(VarFirstPartPrimary)));
}

/* Generate in *p a summary of the attachment, and return the number of
 * newline characters that appear in the summary.  *p is assumed to be
 * either NULL or previously malloc'd space, which is realloc'd as needed.
 *
 * Don't do a full summary if it can be displayed in the window.
 */

int
summarize_attachment(cps, p)
struct cp_state *cps;
char **p;
{
    int lines = 0;
    Attach *ap = cps->attach;
    char	*charSetStr;
    
    if (ap) {
	if (!can_show_inline(ap)) {
	    (void) strapp(p, zmVaStr(catgets( catalog, CAT_MSGS, 214, 
					     "\n%s[ Attachment (%s):" ),
				     cps->indent_str? cps->indent_str : "",
				     ap->data_type? attach_data_type(ap) : 
				     catgets(catalog, CAT_MSGS, 827, 
					     "unknown")));
	    if (ap->content_name && isoff(ap->a_flags, AT_NAMELESS))
		(void) strapp(p, zmVaStr(" \"%s\"", ap->content_name));
	    (void) strapp(p, zmVaStr(catgets( catalog, CAT_MSGS, 216, 
					     " %ld bytes" ), 
				     ap->content_length));
	    if ((GetMimePrimaryType(attach_data_type(ap)) == MimeText) && 
		(charSetStr = FindParam(MimeTextParamStr(CharSetParam),
					&ap->content_params))) {
		(void) strapp(p, zmVaStr("\n%s  %s: %s",
					 cps->indent_str? cps->indent_str : "",
					 catgets(catalog, CAT_MSGS, 848, "Character set"),
					 charSetStr));
		lines++;
	    }
	    if (ap->content_abstract &&
		(!ap->data_type || ci_strcmp(ap->data_type, 
						ap->content_abstract))) {
		(void) strapp(p, zmVaStr("\n%s  %s",
					 cps->indent_str? cps->indent_str : "",
					 ap->content_abstract));
		lines++;
	    }
	    lines++;
	} else {
	    (void) strapp(p, zmVaStr("\n%s[ %s",
				     cps->indent_str? cps->indent_str : "",
				     ap->content_abstract? 
				     ap->content_abstract :
				     ap->data_type? attach_data_type(ap) : 
				     catgets(catalog, CAT_MSGS, 827, 
					     "unknown")));
	    lines++;
	}
	if ((ap->encoding_algorithm) && 
	    (!ap->mime_encoding || ap->mime_encoding != SevenBit))
	    {
		(void) strapp(p, zmVaStr(catgets( catalog, CAT_MSGS, 217, 
						 "\n%s  Encoded with \"%s\"" ),
					 cps->indent_str? cps->indent_str : "",
					 ap->encoding_algorithm));
		lines++;
	    }
	if (can_show_inline(ap))
	    (void) strapp(p, " ] :\n");
	else
	    (void) strapp(p, " ]\n");
	lines++;
    }
    return lines;
}

/* Return TRUE if we should print this line, FALSE otherwise.  Assumes
 * that cps->pat was initialized as follows [see parse_pattern()]:
 *
 *  (no pattern)	beg_pat = NULL, end_pat = NULL,	
 *			at_beg = FALSE, at_end = FALSE, pat_len = 0
 *  //,//		beg_pat = "", end_pat = "",
 *			at_beg = TRUE, at_end = FALSE, pat_len = 0
 *  //,/anything/	beg_pat = "", end_pat = "anything",
 *			at_beg = TRUE, at_end = FALSE, pat_len = 0
 *  /anything/,//	beg_pat = "anything", end_pat = "",
 *			at_beg = FALSE, at_end = FALSE, pat_len = 8
 *  /any/,/thing/	beg_pat = "any", end_pat = "thing",
 *			at_beg = FALSE, at_end = FALSE, pat_len = 3
 *  anything		beg_pat = "anything", end_pat = NULL,	
 *			at_beg = FALSE, at_end = FALSE, pat_len = 8
 */
int
scan_for_pat(line, cpp)
char *line;
struct cp_pat *cpp;
{
    if (cpp->at_beg) {
	if (cpp->at_end)
	    return FALSE;
	if (cpp->pat_len == 0)
	    return TRUE;
	Debug("Seeking (%s) in (%s)", cpp->end_pat, line);
	/* XXX Could do a more sophisticated search here */
	if (strncmp(line, cpp->end_pat, cpp->pat_len) == 0)
	    cpp->at_end = TRUE;
    } else {
	if (cpp->pat_len == 0)
	    return TRUE;
	Debug("Seeking (%s) in (%s)", cpp->beg_pat, line);
	/* XXX Could do a more sophisticated search here */
	if (strncmp(line, cpp->beg_pat, cpp->pat_len) == 0) {
	    cpp->at_beg = TRUE;
	    if (cpp->end_pat)
		cpp->pat_len = strlen(cpp->end_pat);
	    else
		cpp->pat_len = 0;
	} else
	    return FALSE;
    }
    return TRUE;
}

void
parse_pattern(pattern, cpp)
char *pattern;
struct cp_pat *cpp;
{
    char *p;

    cpp->beg_pat = cpp->end_pat = NULL;
    cpp->at_beg = cpp->at_end = FALSE;

    if (pattern) {
	if (*pattern == '/' && (p = index(pattern+1, '/')) && p[1] == ',') {
	    cpp->beg_pat = pattern + 1;
	    cpp->at_beg = (cpp->beg_pat == p);
	    *p = 0;
	    pattern = p + 2;
	    if (*pattern++ == '/' && (p = index(pattern, '/'))) {
		cpp->end_pat = pattern;
		*p = 0;
	    }
	} else
	    cpp->beg_pat = pattern;
    }

    cpp->pat_len = cpp->beg_pat? strlen(cpp->beg_pat) : 0;
}

/* Figure out where we are in the message -- on the header, in the body,
 * in the header part of one section of a multipart attachment, etc.
 */
int
fix_position(line, len, cps)
char *line;
long len;
struct cp_state *cps;
{
    long att_beg, att_end, cur_pos;
    Attach *ap = cps->attach;
    cp_pos *cpp = &cps->pos;
    long msg_off = 0;

    if (cps->mesg && cps->bytes >= cps->mesg->m_size)
	return -1;
    if (cps->mesg)
	msg_off = cps->mesg->m_offset;

    if (len && line[len-1] == '\n') {
#ifdef USE_CRLFS
	/* The byte count for the message includes \r\n at end of each
	 * line, but fgets() translates this to just \n, so we lose one
	 * byte each time.  This sort of thing is going to cause a lot
	 * of confusion elsewhere ....
	 */
	len++;
#endif /* USE_CRLFS */
	cpp->cont_ln = FALSE;
    } else
	cpp->cont_ln = TRUE;
    cps->bytes += len;


    if (cpp->msg_hdr && *line == '\n') {
	cpp->msg_hdr = FALSE;
	cpp->msg_bdy = TRUE;
    }
    if (cpp->msg_bdy && ap) {
	cur_pos = msg_off + cps->bytes;
	att_beg = ap->content_offset;
	att_end = ap->content_offset + ap->content_length;
#ifdef USE_CRLFS
	att_end += ap->content_lines;
#endif /* USE_CRLFS */
	if (cur_pos - len >= att_end ||
	    (cur_pos > att_beg && cur_pos < att_end &&
		/* there's just a single attachment, or we're top multipart */
		(((Attach *) cps->mesg->m_attach->a_link.l_next ==
			     cps->mesg->m_attach) ||
		 (ap == cps->mesg->m_attach && is_multipart(ap))))) {
	    ap = (Attach *)ap->a_link.l_next;
	    if (cps->mesg && (ap == cps->mesg->m_attach) &&
		((Attach *) cps->mesg->m_attach->a_link.l_next !=
		     cps->mesg->m_attach)) {
		/* We've traversed the list and handled all attachments */
		cps->attach = 0;
		cpp->apt_hdr = FALSE;
		cpp->apt_bdy = FALSE;
		{
#ifdef C3
		char *p = value_of(OverrideCharSet);
		if (p && *p)
		    cps->att_char_set = GetMimeCharSet(p);
		else
#endif
		    cps->att_char_set = cps->in_char_set;
		}
		return 0;
	    } else if (ap != cps->attach) {
		att_beg = ap->content_offset;
		att_end = ap->content_offset + ap->content_length;
#ifdef USE_CRLFS
		att_end += ap->content_lines;
#endif /* USE_CRLFS */
		cps->attach = ap;
		fix_char_set(cps);
	    }
	    cpp->apt_bdy = TRUE;	/* Temporarily guess true */
	}
	/* Bart: Mon Sep  7 11:39:38 PDT 1992
	 * For pure RFC1154 attachments, apt_hdr and apt_bdy can both
	 * be true for the single line that starts the attachment.
	 * This is, strictly speaking, wrong, but it's the only way
	 * we can tell that we've passed from one attachment into the
	 * next.  At this point, the attachment summary is output.
	 * Testing cpp->apt_bdy in the first "if" below tells us that
	 * we just left the body of the previous attachment.
	 *
	 * Also, MIME messages with a single part are treated this way;
	 * the first line of the body is called a header here.
	 *
	 * Bart: Wed May 25 14:26:15 PDT 1994
	 * Unfortunately, calling the first line of the body a header
	 * can cause that first line to be dropped if a summary is also
	 * to be generated.  This should really be fixed more thoroughly,
	 * but see the hack in copy_each_line() to work around it.
	 */
	if (cur_pos < att_beg || cpp->apt_bdy && cur_pos - len == att_beg)
	    cpp->apt_hdr = TRUE;
	else
	    cpp->apt_hdr = FALSE;
	if (cur_pos <= att_end && cur_pos > att_beg)
	    cpp->apt_bdy = TRUE;
	else
	    cpp->apt_bdy = FALSE;
    }
    if (*line == '\n' && cps->bytes >= cps->mesg->m_size && cpp->msg_bdy)
	cpp->msg_bdy = FALSE;
    else if (cpp->apt_hdr && cps->boundary) {
	if (cpp->on_boundary)
	    cpp->on_boundary = FALSE;
	else if (line[0] == '-' && line[1] == '-') {
#ifdef NOT_NOW
	    long blen = strlen(cps->boundary);
	    if (!strncmp(line + 2, cps->boundary, blen)) {
		char *p = &line[blen+2];
	
		while (*p == ' ' || *p == '\t') ++p;
		/* The !*p test here is strictly not correct,
		 * we should read more input until we find a
		 * newline.  Unlikely ever to fail, though.
		 */
		cpp->on_boundary = (*p == '\n' || !*p);
	    }
#endif /* NOT_NOW */
	    cpp->on_boundary = TRUE;
	}
    } else
	cpp->on_boundary = FALSE;

    return 0;
}

static void
fix_char_set(cps)
cp_state *cps;
{
    Attach *ap = cps->attach;
    char *p;

    if (ap) {
#ifdef C3
	if ((p = value_of(OverrideCharSet)) && *p)
	    cps->att_char_set = GetMimeCharSet(p);
	else
#endif
	    cps->att_char_set = ap->mime_char_set;
	if (!IsKnownMimeCharSet(cps->att_char_set))
	    cps->att_char_set = cps->in_char_set; 
    }
    else 
	cps->att_char_set = cps->in_char_set; 
}

#define PREFIX(line, prefix)  (!ci_strncmp((line), (prefix), sizeof(prefix) - 1))

/* Check whether we should omit the current line.  If we are on the header
 * and either the line is one we shouldn't print or is a continuation of
 * a previously ignored line (cps->ignoring), then return TRUE, else FALSE.
 */
int
ignore_this_line(line, cps)
char *line;
struct cp_state *cps;
{
    char *p;

    if (cps->pos.msg_hdr) {
	if (ison(cps->flags, UPDATE_STATUS) &&
	    (PREFIX(line, "status:") ||
	     PREFIX(line, "x-zm-priority:") ||
	     PREFIX(line, "priority:")))
	    return cps->ignoring = TRUE;
	if (ison(cps->flags, MODIFYING) && (PREFIX(line, "encoding:")
					 || PREFIX(line, "content-lines:")))
	    return cps->ignoring = TRUE;
    }
    
    if (!cps->pos.msg_hdr ||
	    ison(cps->flags, NO_IGNORE) && isoff(cps->flags, FORWARD|MODIFYING))
	return cps->ignoring = FALSE;

    p = any(line, " \t:");

    if (!p) {
	cps->ignoring = FALSE;
	/* Do error recovery for a case fix_position() doesn't catch. */
	cps->pos.msg_hdr = FALSE;
	cps->pos.msg_bdy = TRUE;
	(void) fix_position(line, 0L, cps);
    } else if (cps->ignoring) {
	if (*p != ':') {
	    Debug("Ignoring: %s", line);
	    return TRUE;
	} else {
	    cps->ignoring = FALSE;
	}
    }

    if (p && (*p == ':' || !strncmp(line, "From ", 5))) {
	if (*p == ':')
	    *p = 0;
	if (test_ignore(line, cps))
	    cps->ignoring = TRUE;
	if (!*p)
	    *p = ':';
	else
	    p = NULL;	/* It's "From ", don't decode_header() below */
	if (cps->ignoring)
	    Debug("Ignoring: %s", line);
    }

    if (p && !cps->ignoring && ison(cps->flags, FOLD_ATTACH)) {
	char *q;

	/* This doesn't belong in this function, but this
	 * is the only place we have all the information
	 * necessary to perform the decoding.
	 */
	if (*++p == ' ' || *p == '\t') ++p;
	q = decode_header(NULL, p);
	if (q && *q)
	    strcpy(p, q);
    }

    return cps->ignoring;
}

/* Copy one line of the message from line to lineout (or out to the
 * pager if paging is being done).  If the line should not be printed,
 * ignore lineout and return zero (or don't send to pager).  If the
 * end of the output is reached or on any error, return -1.
 */
long
copy_each_line(line, len, lineout, cps)
    char *line, **lineout;
    size_t len;
    struct cp_state *cps;
{
    char *p;
    cp_pos prev_pos;

    /* Optimization */
    if (cps->pat.at_end)
	return -1;

    prev_pos = cps->pos;
    if (fix_position(line, len, cps) < 0)
	return -1;

    /* Don't copy the blank line between headers and body when NO_HEADER */
    if (ison(cps->flags, NO_HEADER) &&
	    (!prev_pos.msg_bdy || !cps->pos.msg_bdy))
	return 0;

    /* Don't look for the separator within attachments,
     * they may eventually be binary data ...
     */
    if (cps->rdtype == FolderDelimited &&
	    (cps->wrtype != FolderDelimited || ison(cps->flags, NO_SEPARATOR))
	    && (cps->pos.msg_hdr || !cps->pos.apt_hdr && !cps->pos.apt_bdy)) {
	if (!strncmp(line, msg_separator, strlen(msg_separator))) {
#ifdef NOT_NOW	/* XXX */
	    if (cps->wrtype == FolderStandard && cps->pos.msg_hdr)
		(void) sprintf(line, "From %s %s",
				best_addr(cps->mesg),
				date_to_ctime(cps->mesg->date_recv));
	    else
#endif /* NOT_NOW */
		return 0;
	}
    }

    if (!scan_for_pat(line, &cps->pat))
	return 0;
/* If phone tag or tag it do not print beyond a line containing $EOM$ */
    if (((ison(cps->flags, PHONETAG_MSG)) || (ison(cps->flags, TAGIT_MSG))) &&
        (strstr(line,"$EOM$") != NULL))
      return(-1);
/* If a phone tag message do not print the Message: before the message body */
    if (ison(cps->flags, PHONETAG_MSG) && 
        (strncmp(line,"Message: ",9) == 0))
      line = line + 9;

    /* Handle "squeeze" of multiple blank lines */
    if (*line == '\n' && cps->squeeze &&
	    (ison(cps->flags, FOLD_ATTACH) || !cps->pos.apt_bdy)) {
	if ((cps->squeeze)++ > 1)
	    return 0;
    } else if (cps->squeeze > 1)
	cps->squeeze = 1;

    /* Track the output line in cps->worksp, so we can append to it
     * as necessary, re-use malloc'd space, and free it at the end.
     */
    if (*lineout != cps->worksp)
	*lineout = cps->worksp;

    **lineout = 0;
    if (cps->indent_str && !prev_pos.cont_ln)
	cps->worksp = strapp(lineout, cps->indent_str);
	
    if (ignore_this_line(line, cps))
	return 0;
    else if (cps->pos.msg_hdr && ison(cps->flags, FOLD_ATTACH))
	len = strlen(line);	/* It may have been RFC1522-decoded */

    if (prev_pos.msg_hdr && ison(cps->flags, UPDATE_STATUS)) {
	if (cps->pos.msg_bdy || !cps->pos.msg_bdy && !cps->pos.msg_hdr) {
	    /* Don't pass GENERATE_INDEX to status_line() here, it
	     * screws up the byte count by adding extra flags that
	     * won't appear in the actual folder.
	     */
	    if (isoff(cps->flags, GENERATE_INDEX) ||
		    ison(cps->flags, FULL_INDEX) ||
		    ison(cps->mesg->m_flags, STATUS_LINE)) {
		/* Bart: Wed Sep  9 13:53:20 PDT 1992
		 * Even with the test of FULL_INDEX || STATUS_LINE above,
		 * this still may be wrong, because the status line in
		 * the file itself may not be the same number of bytes
		 * as the status line generated for the index.  New bits
		 * may have been added to the index status (e.g. "r" or
		 * "f") that won't appear in the folder.  Sigh.
		 */
		cps->lines +=
		    status_line(cps->mesg,
				cps->flags & ~GENERATE_INDEX,	/* XXX */
				lineout);
		line = NULL;	/* Replace it with the status line */
	    }
	}
    } else if (cps->pos.msg_bdy && cps->attach) {
	if (ison(cps->flags, FOLD_ATTACH)) {
#ifdef NOT_NOW
	    /* This summarizes only the parts not shown in line
	     * if (!can_show_inline(cps->attach)) { }
	     * This summarizes all except plaintext attachments
	     * if (!is_plaintext(cps->attach)) { }
	     */
#else
	    /* Summarize every part if the message is not a single-part plain 
	     * text message, with one exception - if the first part is 
	     * plain text, don't summarize it.
	     */
	    if (!is_plaintext(cps->mesg->m_attach) &&
		(!(is_multipart(cps->mesg->m_attach)) ||
		 !(cps->attach == 
		   (Attach *) cps->mesg->m_attach->a_link.l_next) ||
		 !is_plaintext((Attach *) 
			       cps->mesg->m_attach->a_link.l_next))) {
#endif
		if (cps->pos.apt_hdr && !prev_pos.apt_hdr) {
		    if (is_multipart(cps->mesg->m_attach) || 
			!can_show_inline(cps->attach))
			cps->lines += summarize_attachment(cps, lineout);
#ifdef NOT_NOW
		    /* Not sure exactly why we want to do this now, but
		     * it's obsolete anyway ...  CML Wed Jun  9 17:10:10 1993
		     */
		    /* This outputs the boundary line between parts */
		    if (!cps->attach->encoding_algorithm 
#ifdef MAX_ATTACH_SHOW
			||
			cps->attach->content_length <= MAX_ATTACH_SHOW
#endif
			) {
			(void) strapp(lineout, line);
			(cps->lines)++;
		    } else
#endif /* NOT_NOW */
		    /* Not sure exactly why we want to do this now, after
		     * emitting the summary ...  CML Wed Jun  9 17:10:10 1993
		     */
		    /* Bart: Mon Sep  7 11:48:56 PDT 1992
		     * The same line can be a "header" and a "body" if we
		     * have a pure RFC1154 attachment (no embedded headers).
		     *
		     * Bart: Wed May 25 15:04:29 PDT 1994
		     * As explained in fix_position(), this can also be true
		     * of a simple MIME message.  Check whether we are going
		     * to inline this part before deciding to drop it.  Sigh
		     * heavily and duplicate the q-p decoding from below.
		     */
   		    if (cps->pos.apt_bdy && can_show_inline(cps->attach)) {
			if (cps->attach->mime_encoding == QuotedPrintable &&
				line != 0) {
			    qp_decode(line, line);
			    len = strlen(line);
			    if (line[len-1] != '\n')
				cps->pos.cont_ln = TRUE;
			}
#ifdef C3
			(void) strapp(lineout,
			    quick_c3_translate(line, &len,
				cps->att_char_set,
				cps->out_char_set));
#else /* C3 */
			(void) strapp(lineout, line);
#endif /* C3 */
			(cps->lines)++;
		    }
		    line = NULL;	/* Replace it with the summary */
		} else if (!can_show_inline(cps->attach) || cps->pos.apt_hdr)
		    return 0;
	    } else if (cps->pos.apt_hdr) {
		/* For a MIME part with no headers, the first line is both a 
		 * header and a body, and we don't want to skip it if we're 
		 * emitting
		 * the body part
		 */
		if (!cps->pos.apt_bdy)
		    return 0;
	    }
	    if (line && cps->attach->mime_encoding == QuotedPrintable) {
		qp_decode(line, line);
		len = strlen(line);
		if (line[len-1] != '\n')
		    cps->pos.cont_ln = TRUE;
	    }
#ifdef C3
	    if (line)
		line = (char *) quick_c3_translate(line, &len,
						   cps->att_char_set,
						   cps->out_char_set);
#endif /* C3 */
	} else if (MsgIsMime(cps->mesg)
		   && ison(cps->attach->a_flags, AT_DELETE | AT_OMIT)) {
	    if (cps->pos.apt_hdr) {
		if (cps->pos.on_boundary) {
		    strapp(lineout, line);
		    strapp(lineout, prune_externalize);
		    line = 0;
		    cps->lines += 4;
		}
	    } else
		return 0;
#ifdef NOT_NOW
	} else {
	    if (cps->pos.apt_hdr && ison(cps->flags, UPDATE_STATUS)) {
		if (!prev_pos.apt_hdr) {
		    /* Here's where we rewrite lineout if we want to
		     * change the headers of an attachment part!
		     */
		    line = NULL;
		} else
		    return 0;
	    }
#endif /* NOT_NOW */
	}
    } else if (cps->pos.msg_bdy && cps->mesg->m_attach  &&
	       !cps->pos.apt_bdy) {
/*	       strcmp(cps->mesg->m_attach->content_type, "text")) {*/
	/* we're past the end of the last attachment.  The last clause is
	 * there in case we're treating a message which we originally thought 
	 * had an attachment as plaintext (except that we want the
	 * attachments button to be enabled)
	 */
	if (ison(cps->flags, FOLD_ATTACH)) {
	    return 0;
	}
#ifdef C3
    } else {
	/*
	 * this is unlabeled mail
	 */
	line = (char *) quick_c3_translate(line, &len,
					   cps->in_char_set,
					   cps->out_char_set);
#endif /* C3 */
    }
    if (line)
	(cps->lines)++;

    /* If we generated a multi-line string, we may need to back up */
    if (ison(cps->flags, M_TOP)) {
	while (*lineout && **lineout && cps->lines > cps->top) {
	    if (p = rindex(*lineout, '\n')) {
		if (p == *lineout)
		    break;
		while (*--p != '\n')
		    ;
		*++p = 0;
		(cps->lines)--;
	    }
	}
    }

    if (line && ison(cps->flags, INDENT)) {
	cps->worksp = strapp(lineout, line);
	line = NULL;
    }  else
	cps->worksp = *lineout;	/* It may have been realloc'd */

    if (cps->result) {
	/* Is there anything else we'd need?  Offsets of attachments,
	 * I suppose ... for now, just get the length of the message.
	 */
	if (lineout && *lineout)
	    cps->result->m_size += strlen(*lineout);
	if (line)
	    cps->result->m_size += strlen(line);
	/* OPTIMIZATION!  See note below. */
	if (cps->pos.msg_bdy && cps->bytes > 0 &&   
        (!cps->attach || isoff(cps->attach->a_flags, AT_DELETE | AT_OMIT)))

	    cps->result->m_size += cps->mesg->m_size - cps->bytes;
    }

    if (ison(cps->flags, GENERATE_INDEX)) {
	/* Bart: Sun Jul 31 18:24:43 PDT 1994
	 * Huge optimization here.  Depends on the knowledge that:
	 *  (a) ix_gen() doesn't care what fioxlate() returns, as
	 *      long as it's greater than zero;
	 *  (b) fioxlate() returns the number of bytes copied so
	 *      far [cps->bytes], whenever any error occurs;
	 *  (c) ix_gen() doesn't care that cps->lines != number of
	 *      lines in the message (it uses original line count);
	 *  (d) we can compute the size of the message body by
	 *      subtracting our current read position [cps->bytes]
	 *      from the previously known message size (see above).
	 * So far, all of these hold true.
	 */
	if (cps->pos.msg_bdy && cps->bytes > 0 &&
        (!cps->attach || isoff(cps->attach->a_flags, AT_DELETE | AT_OMIT)))
	    return -1;	/* Break fioxlate() loop early when indexing */
	else
	    return 0;
    }

    /* At last!  Now we know that we can output this line.  If line has
     * not been set to NULL, we output it with a separate ZmPagerWrite() call
     * to avoid having to copy it to append it to the indent_str.  If we
     * aren't paging we can do a different optimization, because lineout
     * is never realloc'd unless line is set to NULL.
     */
    if (cps->paging) {
	if (*lineout && **lineout)
	    ZmPagerWrite(cur_pager, *lineout);
	if (line)
	    ZmPagerWrite(cur_pager, line);
	len = 0;
    } else if (line)
	*lineout = line;
    else
	len = *lineout? strlen(*lineout) : 0;

    if (ison(cps->flags, M_TOP) && cps->lines >= cps->top)
	return -1;

    return len;
}

int fastwriteok = 1;

/* Copy message 'n' to file "fp" according to various flag arguments
 * return number of lines copied or -1 if system error on fputs.
 * If "fp" is null, send to internal pager.  This can only happen from
 * display_msg.
 */
long
copy_msg(n, fp, flags, pattern, prune)
register int n;
register FILE *fp;
u_long flags;
const char *pattern;
unsigned long prune;
{
    Msg result;
    cp_control get_result = CpEndArgs;
    cp_state cps;
    char *p;
    long x;

#ifdef NOT_NOW
    /* Support for redoing the msg[] array in place so we needn't reload */
    if (ison(flags, REWRITE_ALL)) {
	result = *(msg[n]);	/* Start with a copy of the struct */
	get_result = CpResult;
	turnoff(flags, INDENT);	/* Avoid stupidity */
    }
#endif /* NOT_NOW */

    /* Output the prologue now, so we can use format_hdr() again */
    if (ison(flags, INDENT)) {
	if ((p = value_of(VarPreIndentStr)) && *p) {
	    fputs(format_hdr(n, p, FALSE), fp);
	    fputc('\n', fp);
	}
	if (!(p = value_of(VarIndentStr)))
	    p = DEF_INDENT_STR;
	p = format_hdr(n, p, FALSE);
    } else
	p = NULL;

    {
	int interrupted = 0;
	Attach * attachments = msg[n]->m_attach;
	
	TRYSIG ((SIGINT, 0)) {
	    if (attachments && prune_omit_set(attachments, prune))
		turnon(flags, MODIFYING);

	    if (ison(flags, DELAY_FFLUSH) &&	/* XXX -- overloaded! */
		    fastwriteok &&
		    none_p(flags, MODIFYING|GENERATE_INDEX) &&
		    /* get_result == CpEndArgs && */
		    none_p(msg[n]->m_flags, NEW|DO_UPDATE) &&
		    all_p(msg[n]->m_flags, OLD|STATUS_LINE)) {
		(void) init_cps(&cps, CpWorkspace, NULL, CpEndArgs);
		x = fp2fp(tmpf, msg[n]->m_offset, msg[n]->m_size, fp);
		if (x > 0)
		    cps.lines = msg[n]->m_lines;
	    } else
	    {

	    /* General initialization */
	    (void) init_cps(&cps,
			    CpMessage,	msg[n],
			    CpPaging,	!fp,
			    CpFlags,	flags,
			    CpPattern,	pattern,
			    CpFolderType,	folder_type,
			    CpIndentStr,	p,
			    get_result,	&result,
			    CpEndArgs);
	    flags = cps.flags;

	    /* Guarantee that tmpf is valid before fioxlate() */
	    (void) msg_seek(msg[n], 0);

	    x = fioxlate(tmpf, msg[n]->m_offset, msg[n]->m_size,
			 fp, copy_each_line, &cps);

	    }
	} EXCEPTSIG(SIGINT) {
	    interrupted = 1;

	} FINALLYSIG {
	    if (ison(flags, MODIFYING))
		prune_omit_clear(attachments);
	} ENDTRYSIG;
	
	if (interrupted)
	    kill(getpid(), SIGINT);
    }
	
#ifdef NOT_NOW
    result->m_lines = cps.lines;
#endif /* NOT_NOW */

    clean_cps(&cps);

    if (x < 0)
	return x;

#ifdef NOT_NOW
    if (ison(flags, REWRITE_ALL)) {
	*(msg[n]) = result;
	/* XXX Also redo attachments! */
    }
#endif /* NOT_NOW */

    /* Output the epilogue */
    if (ison(flags, INDENT) && (p = value_of(VarPostIndentStr)) && *p) {
	(void) fprintf(fp, "%s\n", format_hdr(n, p, FALSE));
    }
    if (isoff(flags, DELAY_FFLUSH) && fp && fflush(fp) == EOF)
	return -1;	/* Write failure? */

    return cps.lines;
}

static int
test_ignore(line, cps)
char *line;
cp_state *cps;
{
    register struct options *opts;
    extern DescribeAttach *get_attach_description();

    if (ison(cps->flags, FORWARD)) {
#ifdef NOT_NOW
	/* Bart: Tue Aug 25 12:20:58 PDT 1992
	 * Making sure attachments get forwarded
	 * takes precedence over historic behavior.
	 *
	 * Bart: Tue Aug 10 18:25:57 PDT 1993
	 * One year later, we are now forwarding attachments
	 * as actual attachments.  This is obsolete.
	 */
	if (get_attach_description(line, TRUE))
	    return FALSE;
#endif /* NOT_NOW */
	if (chk_two_lists(line, IGNORE_ON_FWD, ", \t:") ||
		!strncmp(line, "From ", 5))
	    return TRUE;
    }
    if (ison(cps->flags, NO_IGNORE))
	return FALSE;
    if (isoff(cps->flags, UNIGNORED_ONLY) &&
	    (show_hdr || ison(cps->flags, RETAINED_ONLY))) {
	optlist_sort(&show_hdr);		/* XXX */
	for (opts = show_hdr; opts; opts = opts->next) {
	    if (!ci_strcmp(opts->option, line) ||
		    zglob(line, opts->option)) {
		return FALSE;
	    }
	}
	return TRUE;
    } else if (cps->attach && get_attach_description(line, TRUE))
	return TRUE;
    else {
	optlist_sort(&ignore_hdr);		/* XXX */
	for (opts = ignore_hdr; opts; opts = opts->next) {
	    if (!ci_strcmp(opts->option, line) ||
		    zglob(line, opts->option)) {
		return TRUE;
	    }
	    /* could handle from_ as an else here */
	}
    }
    return FALSE;
}

int
status_line(mesg, flags, lineout)
Msg *mesg;
u_long flags;
char **lineout;
{
    char status[1024], *p = status;
    int i, write_priority = 0;
    int lines = 0;

    if (!mesg)
	return 0;

    p += Strcpy(p, "X-Zm-Priority:");
#ifdef NOT_NOW	/* Temporary marks should not be saved across update */
    if (ison(flags, GENERATE_INDEX) && ison(flags, RETAIN_STATUS) &&
	    MsgIsMarked(mesg)) {
	write_priority = 1;
	*p++ = ' ';
	*p++ = '+';
    }
#endif /* NOT_NOW */
    for (i = PRI_MARKED+1; i < PRI_COUNT; i++) {
	char *str;
	if (MsgHasPri(mesg, M_PRIORITY(i)) && (str = priority_string(i))) {
	    write_priority = 1;
	    *p++ = ' ';
	    strcpy(p, str); p += strlen(str);
	}
    }
    if (write_priority) {
	*p++ = '\n';
	lines++;
    } else {
	p = status;
    }

    /* RETAIN_STATUS here avoids changing new message status */
    if (isoff(flags, PRIORITY_ONLY)) {
	if (isoff(flags, RETAIN_STATUS) || ison(flags, GENERATE_INDEX) ||
	    ison(mesg->m_flags, OLD|SAVED|REPLIED|PRINTED|RESENT) ||
	    isoff(mesg->m_flags, UNREAD)) {
	    p += Strcpy(p, "Status: ");
#if defined( IMAP )
            if (ison(flags, GENERATE_INDEX)) {
                if (ison(mesg->m_flags, ATTACHED))
                    *p++ = 'A';
                if (ison(mesg->m_flags, DELETE))
                    *p++ = 'D';
                if (ison(mesg->m_flags, MIME))
                    *p++ = 'M';
            }
            else if ( current_folder->uidval )
                if (ison(mesg->m_flags, DELETE))
                    *p++ = 'D';
#else
            if (ison(flags, GENERATE_INDEX)) {
                if (ison(mesg->m_flags, ATTACHED))
                    *p++ = 'A';
                if (ison(mesg->m_flags, DELETE))
                    *p++ = 'D';
                if (ison(mesg->m_flags, MIME))
                    *p++ = 'M';
            }
#endif
	    if (isoff(flags, RETAIN_STATUS) )
		*p++ = 'O';
	    else if (ison(mesg->m_flags, OLD)) 
		*p++ = 'O';
            else if ( isoff(mesg->m_flags, UNREAD))
		*p++ = 'O';
	    else if (ison(flags, RETAIN_STATUS))
		*p++ = 'N';
	    if (isoff(mesg->m_flags, UNREAD))
		*p++ = 'R';
	    if (ison(mesg->m_flags, SAVED))
		*p++ = 'S';
	    if (ison(mesg->m_flags, REPLIED))
		*p++ = 'r';
	    if (ison(mesg->m_flags, PRINTED))
		*p++ = 'p';
	    if (ison(mesg->m_flags, RESENT))
		*p++ = 'f';
	    lines++;
	    *p++ = '\n';
	}
	*p++ = '\n';
	lines++;
    }
    *p = 0;

    (void) strapp(lineout, status);

    return lines;
}

#ifdef NOT_NOW
/* true if a message needs the X-Zm-Priority header.  It needs the new
 * header if a message has priorities other than the first five (A-E),
 * or if those priorities have a symbolic name defined by the user.
 */
int
MsgHasNewPris(mesg)
Msg *mesg;
{
    int i;
    
    if (ison(mesg->m_pri, PRI_UNDEF_BIT|~0x3f)) return TRUE;
    for (i = 1; i != 6; i++)
	if (MsgHasPri(mesg, M_PRIORITY(i)) && pri_names[i]) return TRUE;
    return FALSE;
}
#endif /* NOT_NOW */

/* Folder indexing */

/*
 * A brief description of the indexing algorithm:
 *
 * General information in the index --
 *	Name of the folder file (if read-only)
 *	Modification time of the folder file (if read-only)
 *	The total size of the folder *except the index*
 *
 * The index entry for each message --
 *	Unique message identifier (currently its message-id)
 *	Message number message should have after loading
 *	Offset of message in the folder (relative to end of index)
 *	Byte count of the message plus its headers
 *	Line count of the message (required for paging later)
 *	Additional message info (at least Priority: and Status:)
 *
 * The size of the index can't be computed in advance, so the offset
 * stored for each message must be adjusted by the size of the index
 * message that begins the folder.  If the index is in a separate file,
 * no adjustment is necessary.
 *
 * There are two forms of index:  A "full index" may appear only inside
 * the folder to which it applies, and contains a complete dump of the
 * folder->mf_msgs array at the time of last update.  A "short index"
 * contains only the information listed above and may be written to a
 * separate file to save the state of a folder without writing the folder
 * itself.  A full index is validated by scanning the folder to see that
 * all message offsets are correct, but unique ID's are not checked unless
 * an offset mismatches.  In that case, the index is assumed inaccurate
 * and we fall back on the short index loading algorithm, described next.
 *
 * An index can be matched against a folder even if the folder has been
 * modified by comparing the unique message identifiers saved in the index.
 * Use of the message-id as unique identifier is questionable.  It might
 * be better to perform a checksum on the message headers and use the
 * message-id only as a fallback (or use the checksum as a fallback if
 * the message-id is not available).
 *
 * Any additional info to be stored about the message is stored in the
 * form of RFC822-style message headers.  This allows the information
 * stored in a short index to be expanded later without breaking other
 * versions of the software.  New versions should take care to remain
 * capable of loading an index that has the minimal information above.
 * Loading of a full index may by necessity be version-dependent.
 *
 * Accuracy of the index is checked by an algorithm similar to that used
 * to load new mail in an active folder.  If the folder is smaller when
 * loaded than when the index was created or if it is larger but a new
 * message cannot be found at the previous end of folder, then the index
 * must be checked more rigorously than if the folder size is unchanged.  
 *
 * The minimal loading algorithm --
 *  1.	Scan the index into a linked list, storing all relevant info
 *	about each expected message.  The list is maintained in sorted
 *	order by message offset.
 *  2.	Scan the headers of the next message in the folder.  Search the
 *	index list for a matching message (see comparison algorithms below).
 *	Note that if the offsets have not changed, the first entry in the
 *	list will match immediately, so expected search time is better than
 *	a simple linear search.
 *  3.	If there is a match, seek past the message using the size indicated
 *	in the index entry.  Move the matched index entry to a new list, this
 *	list sorted by intended message number as stored in the index.
 *	Repeat at step 2.
 *  4.	If the match in step 2 failed, create a new index entry numbered
 *	higher than any so far, and insert that entry in the number-sorted
 *	list.  Load the "new" message by the standard algorithm as if no
 *	index were available.  Resume at step 2 for the next message.
 *  5.	When the end of the folder has been reached, discard any unused
 *	entries from the offset-sorted list, and sort the messages using
 *	the number-sorted index.
 *
 * Comparisons --
 *	If unique id matches, there is a hit.  To make stuff like shared
 *	folders based on indexing work, we treat this as a match even if
 *	all other comparisons fail.  The other comparisons are done only
 *	to decide if we should trust the index for purposes of loading
 *	this message or folder.  Other comparisons include checking both
 *	the message offset and the message size of the potential match.
 *	Message offsets are only required to match if the index is internal
 *	to the folder; if any offsets fail to match, we convert the index
 *	to external index format and load that way.
 */

/*
 * Generate an index structure for a message.  Note that result must be
 * initialized to zeros by a call to ix_header(), and must not be reset
 * (except by this function) until after the call to ix_footer().
 */
msg_index *
ix_gen(n, flags, result)
int n;
u_long flags;
msg_index *result;
{
    cp_state cps;
    long x;

    if (!result)
	return 0;

    /* Save the computed offset from the previous call */
    result->mi_adjust = result->mi_msg.m_offset + result->mi_msg.m_size;

    result->mi_id = id_field(n);	/* XXX */
    result->mi_num = n;
    result->mi_msg = *(msg[n]);	/* Start with a copy of the struct */
    result->mi_msg.m_size = 0;	/* Recompute size */

    /* Reset the new offset for this message */
    result->mi_msg.m_offset = result->mi_adjust;

    /* General initialization */
    (void) init_cps(&cps,
	CpMessage,	msg[n],
	CpFlags,	flags|GENERATE_INDEX,
	CpFolderType,	folder_type,
	CpResult,	&result->mi_msg,
	CpEndArgs);

    /* Guarantee that tmpf is valid before fioxlate() */
    (void) msg_seek(msg[n], 0);

    x = fioxlate(tmpf,
                 msg[n]->m_offset,
                 msg[n]->m_size,
                 NULL_FILE,
                 copy_each_line,
                 (char *)&cps);

    /* Compute the new starting position for the message attachments, if any.
     * Both the message's position in the folder and the size of it's headers
     * may have changed, so this gets a little hairy ... for the moment, we
     * know that the attachments don't change size, so we can work backwards
     * from there.  If we start adding to the attachment headers, we'll have
     * to recompute them completely along with the rest of the message, so
     * this will become irrelevant.
     *
     * Bart: Mon Nov 30 17:44:27 PST 1992
     * Unfortunately, the assumption that attachments don't change size is
     * wrong.  A MIME message has a single attachment that *includes* the
     * message headers, so it's size may vary by the number of bytes added
     * or removed from the Status header (goddamn thing).
     */
    if (x > 0) {
	result->mi_adjust =
	    (msg[n]->m_offset + msg[n]->m_size) -
	    (result->mi_msg.m_offset + result->mi_msg.m_size);
	/* Bart: Thu Sep 17 12:10:10 PDT 1992
	 * Don't worry about the attachment position if not doing an
	 * internal index.  This is more compensation for the stupidity
	 * with Status: lines and the external index.
	 */
	if (ison(flags, FULL_INDEX) && msg[n]->m_attach) {
	    if (msg[n]->m_attach->content_offset == msg[n]->m_offset) {
		/* Bart: Mon Nov 30 17:48:05 PST 1992
		 * This is a terrible hack, but:  We know that if this
		 * is an internal index, we're rewriting the folder,
		 * and this information is going to get thrown away.
		 * So fix the size of the attachment.		XXX
		 * This leaves the attachment having the wrong size if
		 * the write fails, but copy_msg() never copies more or
		 * less than msg[n]->m_size, so we're safe.
		 */
		msg[n]->m_attach->content_length = result->mi_msg.m_size;
		result->mi_adjust = msg[n]->m_offset - result->mi_msg.m_offset;
	    }
	    if (msg[n]->m_attach->content_offset - result->mi_adjust <
			result->mi_msg.m_offset)
		error(ZmErrWarning, catgets( catalog, CAT_MSGS, 222, "Index will be screwy!" ));
	}
    }

    clean_cps(&cps);

    return x <= 0 ? 0 : result;
}

void
ix_footer(fp, bytes)
FILE *fp;
long bytes;
{
    struct stat sbuf;

    if (!mailfile)	/* Bart: Tue Sep  1 15:43:54 PDT 1992 */
	return;

    (void) fputs("--\n", fp);
    if (ison(folder_flags, READ_ONLY) && stat(mailfile, &sbuf) == 0)
	(void) fprintf(fp, "Folder-Time: %ld\n", sbuf.st_mtime);
    (void) fprintf(fp, "Folder-Size: %ld\n", bytes);
    (void) fputs("End-Index\n\n", fp);

    /* XXX This requires the current folder to be the one being indexed! */
    if (isoff(folder_flags, READ_ONLY) && folder_type == FolderDelimited)
	(void) fputs(msg_separator, fp);
}

#define IX_REMARK "X-Remarks: \
If you are reading this message,\n\tyou are not using %s.\n\t\
If you update this folder, %s\n\tmay warn you about an invalid index.\n",\
    check_internal("version"), zmName()

void
ix_header(fp, mix)
FILE *fp;
msg_index *mix;
{
    time_t t;
    char *p;

    if (!mailfile)	/* Bart: Tue Sep  1 15:43:54 PDT 1992 */
	return;

    /* XXX This requires the current folder to be the one being indexed! */
    if (isoff(folder_flags, READ_ONLY) && folder_type == FolderDelimited)
	(void) fputs(msg_separator, fp);

    (void) time(&t);
    (void) fprintf(fp, "From Z-Mail %s", p = ctime(&t));
    (void) fprintf(fp, "From: %s\n", check_internal("version"));
    (void) fprintf(fp, "Date: %s", p);
    (void) fprintf(fp, "X-Zm-Folder-Index: %s\n", zmVersion(0));

    (void) fprintf(fp, IX_REMARK);
    (void) fprintf(fp, "To: %s\n", zlogin);
    (void) fputs("Subject: Index\nStatus: ROS\n\n", fp);
    (void) fprintf(fp, "Folder-Name: %s\n\n", mailfile);

    mix->mi_adjust = mix->mi_msg.m_offset = mix->mi_msg.m_size = 0;
}

char *
ix_locate(file, buf)
char *file, *buf;
{
    char *p = value_of(VarIndexDir);
    int ixfile_bad;

    if (p && *p)
	(void) sprintf(buf, "%s%c%s", p, SLASH, basename(file));
    else
	(void) sprintf(buf, "%s%c%s", DEF_IXDIR, SLASH, basename(file));
    ixfile_bad = dgetstat("+", buf, buf, NULL); 

    if (!ixfile_bad && pathcmp(file, buf) == 0) {
#if defined(MSDOS) || defined(MAC_OS)
	(void) sprintf(buf, "%s%c%.8s.ix", p, SLASH, basename(file));
#else /* MSDOS || MAC_OS */
#ifdef HAVE_LONG_FILE_NAMES
	(void) sprintf(buf, "%s%c%s.ix", p, SLASH, basename(file));
#else /* HAVE_LONG_FILE_NAMES */
	(void) sprintf(buf, "%s%c%.11s.ix", p, SLASH, basename(file));
#endif /* HAVE_LONG_FILE_NAMES */
#endif /* MSDOS || MAC_OS */
	ixfile_bad = dgetstat("+", buf, buf, NULL);
    }

    if (!ixfile_bad && Access(buf, F_OK) == 0)
	return buf;
    return NULL;
}

int
ix_folder(ix_fp, box_fp, flg, isspool)
FILE *ix_fp, *box_fp;
u_long flg;
int isspool;
{
    char ixfile[MAXPATHLEN], *p;
    int werr, held = 0, saved = 0;

    if (!mailfile)	/* Bart: Tue Sep  1 15:43:54 PDT 1992 */
	return -1;

    flg |= GENERATE_INDEX;
    if (!ix_fp) {
	/* Create the index in another file in $index_dir */
	(void) ix_locate(mailfile, ixfile);
	if (p = last_dsep(ixfile)) {
	    werr = *p;
	    *p = 0;
	    if (getdir(ixfile, TRUE))
		*p = werr;
	    else
		return -1;
	} else
	    return -1;
	if (!(ix_fp = mask_fopen(ixfile, FLDR_WRITE_MODE)))
	    return -1;
#ifdef MAC_OS
	gui_set_filetype(ExternalIndexFile, ixfile, NULL);
#endif
    } else {
    	/* like ix_header, assume current_folder */
	flg |= FULL_INDEX;
	ixfile[0] = 0;
    }
#ifdef GUI
    if (istool)
	timeout_cursors(TRUE);
#endif /* GUI */
    check_nointr_mnr(catgets( catalog, CAT_MSGS, 224, "Writing index ..." ), 0);
    werr = copy_all(ix_fp, box_fp, flg, isspool, &held, &saved);
    check_nointr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), 100L);
    if (ixfile[0]) {
	(void) fclose(ix_fp);
	if (!werr && held == 0)
	    (void) unlink(ixfile);
    } else if (!werr && held == 0) {
	(void) rewind(ix_fp);
#if defined(HAVE_FTRUNCATE) || defined(F_FREESP)
	(void) ftruncate(fileno(ix_fp), 0L);
#else /* !(HAVE_FTRUNCATE || F_FREESP) */
#ifdef HAVE_CHSIZE
	(void) chsize(fileno(ix_fp), 0L);
#else
	ix_fp = fopen(mailfile, FLDR_WRITE_MODE);	/* Truncate */
	if (ix_fp)
	    (void) fclose(ix_fp);
#endif /* HAVE_CHSIZE */
#endif /* !(HAVE_FTRUNCATE || F_FREESP) */
    }
#ifdef GUI
    if (istool)
	timeout_cursors(FALSE);
#endif /* GUI */
    return werr;
}

/*
 * Write a folder-index entry for a message
 */
void
ix_write(n, mix, flags, fp)
int n;		/* Message number to use */
msg_index *mix;	/* Message data to index */
u_long flags;	/* RETAIN_STATUS or not; GENERATE_INDEX assumed */
FILE *fp;	/* Where to write it */
{
    static char *statline;
    Msg *mesg = &mix->mi_msg;
    char *m_id = mix->mi_id? mix->mi_id : "";

    if (statline)
	*statline = 0;

    if (ison(flags, FULL_INDEX))
	turnon(flags, PRIORITY_ONLY);
    (void) status_line(mesg, flags, &statline);
#if defined( IMAP )
    if ( current_folder->uidval )
    (void) fprintf(fp, "Message-id: %s (%d, offset %lu, size %lu:%d) X-ZmUID: %08lx\n%s%s",
                        m_id,                                   /* XXX */
                        n+1, mesg->m_offset, mesg->m_size, mesg->m_lines,
                        mesg->uid, statline,
                        ison(flags, FULL_INDEX)? "Zm-Data: " : "");
    else
    (void) fprintf(fp, "Message-id: %s (%d, offset %lu, size %lu:%d)\n%s%s",
                        m_id,                                   /* XXX */
                        n+1, mesg->m_offset, mesg->m_size, mesg->m_lines,
                        statline,
                        ison(flags, FULL_INDEX)? "Zm-Data: " : "");
#else
    (void) fprintf(fp, "Message-id: %s (%d, offset %lu, size %lu:%d)\n%s%s",
                        m_id,                                   /* XXX */
                        n+1, mesg->m_offset, mesg->m_size, mesg->m_lines,
                        statline,
                        ison(flags, FULL_INDEX)? "Zm-Data: " : "");
#endif
    if (ison(flags, FULL_INDEX))
	(void) dumpMsgIx(mix, fp);
}

/*
 * Parse a folder-index entry for a message out of a Source
 * (for compatibility with folder and attachment loading).
 *
 * Returns the message number that should be used for the message,
 * and fills in the Msg structure and m_id with the other values.
 * Returns -1 if no index entry was located, 0 at the end of index.
 */
int
ix_parse(mesg, m_id, ss)
Msg *mesg;
struct dynstr *m_id;
Source *ss;
{
    char *p, line[BUFSIZ];
    int n = -1;

    mesg->m_size = mesg->m_lines = 0;
    while (Sgets(line, sizeof line, ss) && line[0] != '\n') {
	if (strncmp(line, "--", 2) == 0)
	    return 0;
	if (ci_strncmp(line, "Message-id:", 11) == 0) {
	    if (p = rindex(line + 11, '(' /*)*/)) {
#if defined( IMAP )
                if ( current_folder->uidval )
                (void) sscanf(p, "(%d, offset %ld, size %ld:%d) X-ZmUID: %08lx",
                              &n, &mesg->m_offset,
                              &mesg->m_size, &mesg->m_lines,
                              &mesg->uid);
                else
                (void) sscanf(p, "(%d, offset %ld, size %ld:%d)",
                              &n, &mesg->m_offset,
                              &mesg->m_size, &mesg->m_lines);
#else
                (void) sscanf(p, "(%d, offset %ld, size %ld:%d)",
                              &n, &mesg->m_offset,
                              &mesg->m_size, &mesg->m_lines);
#endif
		*p = 0;
	    }
	    p = line + 11;
	    skipspaces(0);
	    dynstr_Set(m_id, p);
	    p = index(dynstr_Str(m_id), '>');	/* Should be end of ID */
	    if (p) {
		++p;
		dynstr_Delete(m_id, p - dynstr_Str(m_id),
			      dynstr_Length(m_id) - (p - dynstr_Str(m_id)));
	    } else
		while (dynstr_Length(m_id)) {
		    int c = dynstr_Chop(m_id);
		    if (!isspace(c)) {
			dynstr_AppendChar(m_id, c);
			break;
		    }
		}
	} else if (ci_strncmp(line, "Zm-Data:", 8) == 0) {
	    fillMsg(mesg, line + 9, ss);
	    turnon(mesg->m_flags, INDEXED);
	    break;
	} else {
	    parse_header(line, mesg, NULL, TRUE, 0L, n);
	}
    }

    return n;
}

fldr_index folder_ix;
#define loading_ix folder_ix.f_load_ix
#define sorting_ix folder_ix.f_sort_ix
#define waiting_ix folder_ix.f_wait_ix
#define external_ix folder_ix.f_ext_ix
#define expected_size folder_ix.f_folder_size

void
ix_destroy(ix, clean)
msg_index **ix;
int clean;
{
    msg_index *next_ix;

    while (ix && *ix) {
	next_ix = *ix;
	remove_link(ix, next_ix);
	if (clean)
	    clearMsg(&next_ix->mi_msg, TRUE);
	xfree(next_ix->mi_id);
	xfree(next_ix);
    }
}

/* Comparison of two index entries for insert_sorted_link() */
static int
ix_cmp_off(e1, e2)
const char *e1, *e2;
{
    msg_index *m1 = (msg_index *)e1;
    msg_index *m2 = (msg_index *)e2;
    long diff = m1->mi_msg.m_offset - m2->mi_msg.m_offset;

    return (diff > 0 ? 1 : diff < 0 ? -1 : 0);
}

static int
ix_cmp_num(e1, e2)
const char *e1, *e2;
{
    msg_index *m1 = (msg_index *)e1;
    msg_index *m2 = (msg_index *)e2;

    /* Special case for negative numbers, which mean that the message
     * was not found in the index and we're sorting in a dummy entry.
     */
    if (m1->mi_num < 0)
	return m2->mi_num < 0? -(m1->mi_num) + m2->mi_num : 1;
    else if (m2->mi_num < 0)
	return -1;

    return m1->mi_num - m2->mi_num;
}

/* Preliminary match of an index entry against a message */
int
ix_match(mix, mesg)
msg_index *mix;
Msg *mesg;
{
    if (mix->mi_id && mesg->m_id && strcmp(mix->mi_id, mesg->m_id) != 0)
	return FALSE;
    if (!(mix->mi_id && mesg->m_id) && mix->mi_msg.m_offset != mesg->m_offset)
	return FALSE;
    if (!(mix->mi_id && mesg->m_id) && !mesg->m_offset)
	return FALSE;	/* Both offsets == 0 is not good enough to match */
#ifdef NOT_NOW
    /* Bart: Tue Nov 23 22:33:01 PST 1993
     * Move this to ix_load_msg() -- attachments should only affect whether
     * we use the index entry for loading, not if we use it for sorting.
     * Left the code because this decision is not definite; any method for
     * uniquely identifying a message should be quite thorough ...
     */
    /* Final sanity check -- one can't have attachments and the other not */
    if (ison(mix->mi_msg.m_flags, ATTACHED) != ison(mesg->m_flags, ATTACHED))
	return FALSE;
#endif /* NOT_NOW */
    return TRUE;
}

/* Convert a full index into a short index */
void
ix_shrink(mix)
msg_index *mix;
{
    msg_index *tmp = mix;

    if (!tmp)
	return;

    do {
	free_attachments(&tmp->mi_msg.m_attach, FALSE);
	xfree(tmp->mi_msg.m_date_recv);
	xfree(tmp->mi_msg.m_date_sent);
    } while ((tmp = (msg_index *)tmp->mi_link.l_next) != mix);
}

/* Special case combine of two indices for ix_switch() */
static void
ix_reorder(old, new)
msg_index **old, **new;
{
    msg_index *walk_n, *walk_o, *save_n = 0, *save_o = 0;
    int found;

    if (!new || !*new || !old)
	return;

    if (!*old) {
	*old = *new;
	*new = 0;
	return;
    }

    while (walk_n = *new) {
	remove_link(new, walk_n);
	found = 0;

	/* Search for a matching entry in the old index */
	if (walk_o = *old)
	    do {
		if (ix_match(walk_n, &walk_o->mi_msg)) {
		    found = 1;
		    break;
		}
	    } while ((walk_o = (msg_index *)walk_o->mi_link.l_next) != *old);

	if (found) {
	    remove_link(old, walk_o);
	    walk_o->mi_num = walk_n->mi_num;
	    walk_o->mi_msg.m_flags = walk_n->mi_msg.m_flags;
	    walk_o->mi_msg.m_pri   = walk_n->mi_msg.m_pri; /* ?? -pf */
	    xfree(walk_n->mi_id);
	    xfree(walk_n);
	    insert_sorted_link(&save_o, walk_o, ix_cmp_num);
	} else
	    insert_link(&save_n, walk_n); /* maintains order by offset */
    }

    *new = save_n;	/* Whatever's left of new after the combine */

    if (save_o) {
	while (walk_o = *old) {
	    remove_link(old, walk_o);
	    if (walk_o->mi_num > 0)
		walk_o->mi_num = 0 - walk_o->mi_num;
	    insert_sorted_link(&save_o, walk_o, ix_cmp_num);
	}
	*old = save_o;	/* The old index, reordered according to new */
    }
}

/* Switch from the internal to the external index.  Return 0 on success. */
int
ix_switch(do_sort_num)
int do_sort_num;
{
    if (!do_sort_num && sorting_ix) {
	/* We're switching from internal to external index in mid-load. */
	if (isoff(folder_flags, READ_ONLY))
	    turnon(folder_flags, DO_UPDATE); /* The index is incomplete */
    }

    if (!external_ix)
	return -1;

    ix_destroy(&loading_ix, FALSE);	/* It should already be empty */
    ix_destroy(&waiting_ix, FALSE);	/* It should already be empty */

    /* If a sorting index exists, we've done a partial load,
     * and now we want to finish up using the external index.
     */
    if (sorting_ix) {
	print(catgets( catalog, CAT_MSGS, 227, "Using external index to sort.\n" ));
	ix_reorder(&sorting_ix, &external_ix);
    }

    if (do_sort_num) {
	/* We've already reordered sorting_ix according to external_ix
	 * by calling ix_reorder(), so toss out whatever's left.
	 */
	ix_destroy(&external_ix, FALSE);
	return !sorting_ix;
    } else {
	if (msg_cnt > 1) { /* Bart: Sat Aug 15 20:26:57 PDT 1992 */
	    /* Internal index is out of date */
	    if (isoff(folder_flags, READ_ONLY))
		turnon(folder_flags, DO_UPDATE);
	    print(catgets( catalog, CAT_MSGS, 228, "Loading via external index beginning at %d.\n" ), msg_cnt);
	} else
	    print(catgets( catalog, CAT_MSGS, 229, "Using external index to load.\n" ));
	loading_ix = external_ix;
	external_ix = 0;
	return !loading_ix;
    }
}

int
ix_init(ss, cnt, offset)
Source *ss;
int cnt;
long offset;	/* Current seek position or -1 to load external index */
{
    char buf[MAXPATHLEN+32], name[MAXPATHLEN];
    struct dynstr msgid;
    msg_index *next_ix, *tmp_ix = 0, **store_ix;
    long ixtime = 0, size = -1;
    int n = -1;
    Source *ixs = ss;

    if (!mailfile)	/* Bart: Tue Sep  1 15:43:54 PDT 1992 */
	return TRUE;

    if (offset == -1) {
	if (ison(folder_flags, IGNORE_INDEX))
	    return TRUE;

	if (!ix_locate(mailfile, buf) || !(ixs = Sopen(SourceFile, buf)))
	    return TRUE;

	/* The index has to look like a message */
	msg[cnt]->m_flags = NO_FLAGS;
	if (Sgets(buf, sizeof buf, ixs) && match_msg_sep(buf, folder_type)) {
	    while (Sgets(buf, sizeof (buf), ixs) && (*buf != '\n'))
		parse_header(buf, msg[cnt], (Attach **)0, TRUE, 0L, cnt);
	}
	if (isoff(msg[cnt]->m_flags, INDEXMSG)) {
	    clearMsg(msg[cnt], TRUE);
	    goto ix_initdone;
	}
	/* Bart: Tue Aug 25 18:14:12 PDT 1992 -- plug small leak */
	xfree(msg[cnt]->m_date_sent);
	xfree(msg[cnt]->m_date_recv);

	/* Always attempt to use an external index (which is what we
	 * know we have inside this block); don't skip it for version
	 * incompatibility.  The only version-specific index field is
	 * Zm-Data: -- which we don't write to external index files
	 * for exactly that reason.  At worst, we fail to parse and
	 * give up later -- we've fixed the index parsing core dump
	 * bugs, right? :-)  Bart: Tue Nov 23 23:48:18 PST 1993
	 */
	turnoff(folder_flags, IGNORE_INDEX);
    }

    (void) sprintf(buf, ison(folder_flags, IGNORE_INDEX)
		   ? catgets( catalog, CAT_MSGS, 231, "Skipping index ..." )
		   : catgets( catalog, CAT_MSGS, 232, "Loading index ..." ));
    check_nointr_msg(buf);

    /* First line should be Folder-Name: */
    if (!Sgets(buf, sizeof name + 13, ixs) ||
	    sscanf(buf, "Folder-Name: %[^\n]", name) != 1)
	goto ix_initdone;

    if (offset == -1 && pathcmp(name, mailfile) != 0) {
	if (ison(glob_flags, NO_INTERACT) ||
		ask(WarnNo, catgets( catalog, CAT_MSGS, 233, "Loading %s:\nFound index for %s.\nUse it?" ),
		    mailfile, name) != AskYes) {
	    size = 0;
	    goto ix_initdone;
	}
    }

    /* Assume a full index until proven wrong; see ix_parse() */
    if (!loading_ix)
	turnon(folder_flags, HAS_FULL_INDEX);

    if (isoff(folder_flags, IGNORE_INDEX)) {
	/* Next line ought to be a blank line */
	if (Sgets(buf, sizeof buf, ixs) && buf[0] == '\n')
	    do {
		next_ix = zmNew(msg_index);
		dynstr_Init(&msgid);
		n = ix_parse(&next_ix->mi_msg, &msgid, ixs);
		if (n <= 0) {
		    xfree(next_ix);
		    dynstr_Destroy(&msgid);
		} else {
		    if (isoff(next_ix->mi_msg.m_flags, INDEXED))
			turnoff(folder_flags, HAS_FULL_INDEX);
		    turnoff(next_ix->mi_msg.m_flags, INDEXED);	/* XXX */
		    next_ix->mi_id = dynstr_GiveUpStr(&msgid);
		    next_ix->mi_num = cnt + n;
		    push_link(&tmp_ix, next_ix);
		}
	    } while (n > 0);
	
	if (n < 0)
	    goto ix_initdone;
    }

    /* Last two lines are time and size */
    while (Sgets(buf, sizeof buf, ixs)) {
	if (strncmp(buf, "End-Index", 9) == 0)
	    break;
	if (strncmp(buf, "Folder-Size:", 12) == 0) {
#ifdef WIN16   
	    size = atol(buf); /* don't need the +13 */
#else
	    size = atoi(buf + 13);
#endif
	} else if (sscanf(buf, "Folder-Time: %ld", &ixtime) == 1) {
	    continue;
	} else if (match_msg_sep(buf, folder_type)) {
	    Sungets(buf, ixs);
	    goto ix_initdone;  /* must be a corrupted/bad/new type of index */
	}
    }

    /* Scarf the remaining blank line */
    if (Sgets(buf, sizeof buf, ixs) && buf[0] != '\n')
	goto ix_initdone;	/* We blew it */
    /* Scarf the trailing delimiter if FolderDelimited */
    if (offset != -1 && folder_type == FolderDelimited &&
	    Sgets(buf, sizeof buf, ixs) && !match_msg_sep(buf, folder_type))
	goto ix_initdone;	/* We blew it */

    /* Check for duplication */
    if (offset == 0 && loading_ix || ison(folder_flags, IGNORE_INDEX)) {
	size = 0;
	goto ix_initdone;
    } else if (offset == -1) {
	/* Dump the index into the external_ix list for later processing. */
	store_ix = &external_ix;
    } else {
	store_ix = &loading_ix;
	if (size > 0)
	    expected_size = size;
	msg[cnt]->m_offset = offset;	/* For ix_verify() */
    }
    offset = Stell(ss);		/* Yep, ss, not ixs */

    if (!tmp_ix)
	goto ix_initdone;

    while (next_ix = tmp_ix) {
	remove_link(&tmp_ix, next_ix);
	/* Only read-only folders or those where the index is in the
	 * middle of the file need to add in the offset; if there is an
	 * empty tempfile, index-relative offsets are already correct.
	 * Slow down the load that starts out faster ...
	 */
	if (ison(folder_flags, READ_ONLY) || msg[cnt]->m_offset > 0) {
	    Attach *a;
	    /* Bart: Sun Jul 26 18:33:01 PDT 1992
	     * Add nonzero msg[cnt]->m_offset into the adjustment.
	     */
	    long adjust =
		ison(folder_flags, READ_ONLY)? offset : msg[cnt]->m_offset;
	    next_ix->mi_msg.m_offset += adjust;
	    if (a = next_ix->mi_msg.m_attach)
		do {
		    a->content_offset += adjust;
		    a = (Attach *)a->a_link.l_next;
		} while (a != next_ix->mi_msg.m_attach);
	}
	insert_sorted_link(store_ix, next_ix, ix_cmp_off);
    }

    if (ison(folder_flags, HAS_FULL_INDEX)) {
	if (ix_verify(ss, cnt, isoff(folder_flags, READ_ONLY)) != 0) {
	    ix_shrink(loading_ix);
	    size = Sseek(ss, offset, 0);		/* 0 or -1 */
	}
    }

ix_initdone:
    if (ixs != ss)
	Sclose(ixs);
    ix_destroy(&tmp_ix, TRUE);
    return size == -1;
}

void
ix_confirm(cnt)
int cnt;
{
    msg_index *tmp;
    int attached;

    if (tmp = waiting_ix) {
	if (cnt < 1 || tmp->mi_msg.m_offset != msg[cnt-1]->m_offset)
	    error(ZmErrFatal,
		catgets(catalog, CAT_MSGS, 234,
		    "ix_confirm: confirming the wrong message!"));
	remove_link(&waiting_ix, tmp);
	/* Don't change the state of ATTACHED based on the index! */
	attached = ison(msg[cnt-1]->m_flags, ATTACHED);
	msg[cnt-1]->m_flags = tmp->mi_msg.m_flags;
	if (attached)
	    turnon(msg[cnt-1]->m_flags, ATTACHED);
	else
	    turnoff(msg[cnt-1]->m_flags, ATTACHED);
	msg[cnt-1]->m_pri = tmp->mi_msg.m_pri; /* ?? -pf */
	insert_sorted_link(&sorting_ix, tmp, ix_cmp_num);
    }
}

/* Attempt to load a message by matching it to the index.
 * Return 0 for success, non-zero for "had error".
 */
int
ix_load_msg(ss, cnt)
Source *ss;
int cnt;
{
    int found = 0;
    long ok;
    msg_index *tmp;

    /* First, if there is an entry in the waiting list, confirm it */
    ix_confirm(cnt);

    if (!(tmp = loading_ix))
	return 1;

    /* Search for a matching message in the loading index */
    do {
	if (ix_match(tmp, msg[cnt])) {
	    found = 1;
	    break;
	}
    } while ((tmp = (msg_index *)tmp->mi_link.l_next) != loading_ix);

    if (found) {
	u_long flg, pri;

	/* We know the list is circularly linked, so this is a fast
	 * way to move to the end all items we just failed to match.
	 */
	loading_ix = (msg_index *)tmp->mi_link.l_next;

	/* Final sanity check -- if one has attachments and the other
	 * does not, don't actually attempt to use this index to load.
	 * It's still OK for sorting order, though.
	 */
	if (ison(tmp->mi_msg.m_flags, ATTACHED) !=
		ison(msg[cnt]->m_flags, ATTACHED)) {
	    found = 0;
	} else {
	    /* Copy the last computed sizes from the index into the actual
	     * message structure.  Then force the index entry to be exactly
	     * the same as the message it matched.  This will be checked
	     * later for sorting.  Preserve the flags from the index;
	     * they'll be assigned at confirm.
	     */
	    msg[cnt]->m_size = tmp->mi_msg.m_size;
	    msg[cnt]->m_lines = tmp->mi_msg.m_lines;
	}
	flg = tmp->mi_msg.m_flags;
	pri = tmp->mi_msg.m_pri;
	tmp->mi_msg = *(msg[cnt]);
	/* pf Tue Jun  8 16:03:41 1993: set DO_UPDATE */
	if (msg[cnt]->m_flags != flg && isoff(folder_flags, READ_ONLY)) {
	    turnon(folder_flags, DO_UPDATE);
	    turnon(msg[cnt]->m_flags, DO_UPDATE);
	    turnon(flg, DO_UPDATE);
	}
	tmp->mi_msg.m_flags = flg;
	tmp->mi_msg.m_pri   = pri;

	/* Move the item that did match to the waiting list */
	remove_link(&loading_ix, tmp);
	insert_sorted_link((VPTR)&waiting_ix, (VPTR)tmp, ix_cmp_off);
    } else {
	/* We invent a matching item and put it in the waiting list so that
	 * we can order everthing that does match ahead of this "new" item.
	 * Make the message number negative to differentiate from the "old"
	 * messages; we'll sort this out later (pun :-).
	 */
	tmp = zmNew(msg_index);
	tmp->mi_id = savestr(msg[cnt]->m_id);
	tmp->mi_num = -(cnt+1);
	tmp->mi_msg = *(msg[cnt]);
	insert_sorted_link((VPTR)&waiting_ix, (VPTR)tmp, ix_cmp_off);
    }

    /* Set up the waiting entry so we can use it to recover state should
     * it turn out that we had a "bad" match.  We're using the m_size
     * field to store the offset of the message headers, and if not in a
     * read-only folder, the m_lines field to store the difference between
     * that position and the real seek position in the original folder.
     * Hopefully that will never be a huge value.		XXX
     */
    if (isoff(folder_flags, READ_ONLY)) {
	tmp->mi_msg.m_size = ftell(tmpf);
	tmp->mi_msg.m_lines = (int)(Stell(ss) - tmp->mi_msg.m_size);
    } else
	tmp->mi_msg.m_size = Stell(ss);

    /* Don't even bother with an index load of a message with attachments;
     * we can load the message just as fast and with fewer errors by using
     * the data from the attachment headers.
     *
     * For messages whose line count from the index comes up zero, force a
     * regular load so we can get the correct line count into the index on
     * update.  This should never happen, but ....
     */
    if (!found || ison(msg[cnt]->m_flags, ATTACHED) || msg[cnt]->m_lines == 0)
	return 1;

    /* Now we can proceed with the load. */
    if (isoff(folder_flags, READ_ONLY)) {
	/* XXX This assumes a SourceFile!! */
	ok = msg[cnt]->m_size - (ftell(tmpf) - msg[cnt]->m_offset);
	if (ok > 0)
        ok -= Source_to_fp(ss, -1L, (long)ok, tmpf);
	else
	    ok = -1;
    } else
	ok = Sseek(ss, msg[cnt]->m_size + msg[cnt]->m_offset, 0);

    return ok != 0;
}

int trustixok = 0, noresortix = 1;

/* Verify a full index against the folder.
 *
 * At the moment, this is duplicating some code from load_folder() in the
 * interest of speed.  If load_folder() were more modular, we might be able
 * to combine some of the work here with the work there.  Oh, well.
 */
int
ix_verify(ss, cnt, copy)
Source *ss;
int cnt, copy;
{
    char buf[BUFSIZ];
    msg_index *tmp;
    long where = Stell(ss);
    long adjust = ison(folder_flags, READ_ONLY)? 0 : where;
    int n = 0;

    if (isoff(folder_flags, READ_ONLY) && msg[cnt]->m_offset > 0)
	adjust -= msg[cnt]->m_offset;

    /* Once this function is done, we either don't have an index any more
     * or we don't want to treat it as a full index.
     */
    turnoff(folder_flags, HAS_FULL_INDEX);

    if (!(tmp = loading_ix))
	return -1;
    check_nointr_msg(catgets( catalog, CAT_MSGS, 235, "Verifying index ..." ));
    if (trustixok) {	/* Perftweak -- believe the index without verify */
	tmp = (msg_index *)tmp->mi_link.l_prev;
	n = number_of_links(loading_ix);
    }
    do {
	if (Sseek(ss, tmp->mi_msg.m_offset + adjust, 0) < 0) {
	    goto ix_bailout;
        }
	if (!Sgets(buf, sizeof buf, ss) || !match_msg_sep(buf, folder_type)) {
            printf("bailout 2\n");
	    goto ix_bailout;
        }
	n++;
    } while ((tmp = (msg_index *)tmp->mi_link.l_next) != loading_ix);

    /* Bart: Tue Nov 24 17:05:04 PST 1992
     * We haven't actually verified that the last message *ends* in the
     * right place, only that it *begins* in the right place.  Check
     * the former as well.  If it fails, throw out the last index entry
     * and copy only the part that succeeded.
     */
    tmp = (msg_index *)tmp->mi_link.l_prev;
    if (expected_size + where !=
	    tmp->mi_msg.m_offset + tmp->mi_msg.m_size + adjust) {
	if (!bool_option(VarQuiet, "index"))
	    error(ZmErrWarning, catgets( catalog, CAT_MSGS, 236,
		"Possibly corrupted index in this folder (scrapped it)" ));
	goto ix_bailout;
    }
    if (Sseek(ss, expected_size + where, 0) < 0 ||
	    Sgets(buf, sizeof buf, ss) &&
	    !match_msg_sep(buf, folder_type)) {
	remove_link(&loading_ix, tmp);
	expected_size = tmp->mi_msg.m_offset;
	insert_link(&waiting_ix, tmp);	/* So we can shrink+destroy */
	ix_shrink(waiting_ix);		/* To free the message part */
	ix_destroy(&waiting_ix, FALSE);	/* To free the entry itself */
    } else if (!copy)
	expected_size += where;

    if (copy) {
	(void) Sseek(ss, where, 0);
	(void) fseek(tmpf, msg[cnt]->m_offset, 0);
	check_nointr_msg(catgets( catalog, CAT_MSGS, 237, "Copying to tempfile ..." ));
	if (Source_to_fp(ss, -1L, (long)expected_size, tmpf) != expected_size)
	    goto ix_bailout;
	tmp = loading_ix;
    } else {
	if (Sseek(ss, expected_size, 0) != 0)
	    goto ix_bailout;
    }
    msg = (Msg **)realloc(msg, (unsigned)((cnt+n+2)*sizeof(Msg *)));
    if (!msg) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 238, "Cannot complete load" ));
	goto ix_bailout;
    }
    if (external_ix || noresortix) {
	/* Bart: Thu Aug 13 14:23:28 PDT 1992
	 * Don't bother reordering now, as we are going to reorder again
	 * based on the external index anyway.  Just slam the messages
	 * straight from loading_ix into the msg[] array.
	 */
	sorting_ix = loading_ix;
	loading_ix = 0;
    } else {
	/* noresortix == 1 presumes this loop is an expensive no-op */
	while (tmp = loading_ix) {
	    remove_link(&loading_ix, tmp);
	    insert_sorted_link(&sorting_ix, tmp, ix_cmp_num);
	}
    }
    if (tmp = sorting_ix)
    do {
	/* Bart: Sat Aug 15 17:54:32 PDT 1992 */
	turnon(tmp->mi_msg.m_flags, FINISHED_MSG);
	/* Bart: Fri Aug 14 15:03:18 PDT 1992 */
	tmp->mi_msg.m_id = tmp->mi_id;	/* For ix_reorder() */
	*(msg[cnt++]) = tmp->mi_msg;
	msg[cnt] = malloc(sizeof(Msg));
	tmp->mi_id = 0;			/* Don't free it in ix_destroy() */
    } while ((tmp = (msg_index *)tmp->mi_link.l_next) != sorting_ix);
    msg_cnt = cnt;
    msg[msg_cnt]->m_offset = expected_size;
    expected_size = 0;
    check_nointr_msg(catgets( catalog, CAT_MSGS, 239, "Load from index complete (please wait)." ));
    if (!external_ix)		/* Bart: Thu Aug 13 14:25:17 PDT 1992 */
	ix_destroy(&sorting_ix, FALSE);	/* Don't waste time sorting again */
    return 0;
ix_bailout:
    if (!bool_option(VarQuiet, "index")) {
	check_intr_msg(catgets( catalog, CAT_MSGS,
	    240, "Index verification failed." ));
	print(catgets( catalog, CAT_MSGS, 241,
	    "Folder index verification failed.\n" ));
    }
    Sseek(ss, where, 0);
    if (copy)
	fseek(tmpf, msg[cnt]->m_offset, 0);
    return -1;
}

/* Reset from a "false" attempt to do an indexed load */
long
ix_reset(ss, cnt)
Source *ss;
int cnt;
{
    long ok = 0;
    long offset;
    msg_index *tmp = waiting_ix;

    if (!waiting_ix)
	return (external_ix? 0 : -1);
    if (cnt < 1 || tmp->mi_msg.m_offset != msg[cnt-1]->m_offset)
	error(ZmErrFatal, catgets( catalog, CAT_MSGS, 242, "ix_reset: trashing the wrong message!" ));

    if (isoff(folder_flags, READ_ONLY)) {
	ok = (long) fseek(tmpf, tmp->mi_msg.m_size, 0);
	offset = tmp->mi_msg.m_size + tmp->mi_msg.m_lines;
	ok += Sseek(ss, offset, 0);
    } else
	ok = Sseek(ss, tmp->mi_msg.m_size, 0); /* XXX SourceFile only */

    /* Bart: Wed Sep  9 11:45:39 PDT 1992
     * Even if the sizes don't match, ix_match() must have made a hit.
     * Preserve the sorting order and status flags. 
     */
    ix_confirm(cnt);

    return ok;
}

/* Finish up the indexed load */
void
ix_complete(clear_only)
int clear_only;
{
    int i, j = 0, fatal = TRUE;
    Msg *save;
    msg_index *tmp;

    ix_confirm(msg_cnt);

    if (clear_only) {
	ix_destroy(&sorting_ix, FALSE);
	ix_destroy(&external_ix, FALSE);
    } else {
	if (external_ix)
	    fatal = ix_switch(TRUE);
	while (tmp = sorting_ix) {
	    for (i = j; i < msg_cnt; i++) {
		if (msg[i]->m_offset != tmp->mi_msg.m_offset) continue;
		/* pf Tue Jun  8 16:03:41 1993: set DO_UPDATE */
		if (msg[i]->m_flags != tmp->mi_msg.m_flags ||
		    msg[i]->m_pri   != tmp->mi_msg.m_pri) {
		    msg[i]->m_flags = tmp->mi_msg.m_flags;
		    msg[i]->m_pri   = tmp->mi_msg.m_pri;
		    if (isoff(folder_flags, READ_ONLY)) {
			turnon(folder_flags, DO_UPDATE);
			turnon(msg[i]->m_flags, DO_UPDATE);
		    }
		}
		turnon(msg[i]->m_flags, FINISHED_MSG);
		if (i == j) break;
		save = msg[j];
		msg[j] = msg[i];
		msg[i] = save;
		break;
	    }
	    if (i < msg_cnt || !fatal) {
		remove_link(&sorting_ix, tmp);
		insert_link(&loading_ix, tmp);	/* Awaiting destroy */
	    } else
		error(ZmErrFatal, catgets( catalog, CAT_MSGS, 243, "Cannot find message %d in index" ), j+1);
	    j++;
	}
    }

    ix_destroy(&loading_ix, FALSE);
    ix_destroy(&waiting_ix, FALSE);	/* Should be empty already, but ... */
}

int
dumpAttach(attach, fp, adjust)
Attach *attach;
FILE *fp;
long adjust;
{
    char *ns = "";
    
    return fprintf(fp,
		   "\t\"%s\" %lu %lu %lu \"%s\"\n\t%s#\n\t%s#\n\t%s#\n\t%s#\n\t%s#\n",
		   attach->orig_content_name ? 
		   basename(attach->orig_content_name) : 
		   (attach->content_name ? 
		    basename(attach->content_name) : 
		    ns),
		   attach->content_offset - adjust,
		   attach->content_length,
		   attach->content_lines,
		   attach->content_type ? attach->content_type : ns,
		   ((attach->content_abstract && 
		     (ci_strcmp(attach->content_abstract, 
				   attach->content_type))) ?
		    attach->content_abstract    : ns),
		   (attach->encoding_algorithm ? 
		    attach->encoding_algorithm : ns),
		   ((attach->data_type && 
		     (ci_strcmp(attach->data_type, attach->content_type))) ?
		    attach->data_type : ns),
		   ((attach->mime_encoding_str &&
		     (ci_strcmp(attach->mime_encoding_str,
				   attach->encoding_algorithm)))?
		    attach->mime_encoding_str : ns),
		   attach->orig_mime_content_type_str ?
		   attach->orig_mime_content_type_str : ns);
}

static int
fillAttach(attach, adata, ss)
    Attach	*attach;
    char	*adata;
    Source	*ss;
{
    char *p, buf[BUFSIZ], name[256], type[256];
    
    if (sscanf(adata, "\t\"%256[^\"]\" %lu %lu %lu \"%256[^\"]\"",
	       name,
	       &attach->content_offset,
	       &attach->content_length,
	       &attach->content_lines,
	       type) != 5)
	return -1;
    attach->content_name = savestr(name);
    if (strcpyStrip(buf, attach->content_name, TRUE)) {
	/* Don't bother telling them every time they load the message */
	Debug("Shell metacharacters included in attachment filename %s.\nUsing: %s.\n",
	      attach->content_name, buf);
	attach->orig_content_name = attach->content_name;
	attach->content_name = savestr(buf);
    }
    attach->content_type = savestr(type);
    
    if (!Sgets(buf, sizeof buf, ss) || !(p = rindex(buf, '#')))
	return -1;
    if (p > buf+1) {
	*p = 0;
	attach->content_abstract = savestr(buf+1);	/* +1 to skip tab */
    }
#if 0
    else if (attach->content_type)
	attach->content_abstract = savestr(attach->content_type);
#endif /* 0 */
    if (!Sgets(buf, sizeof buf, ss) || !(p = rindex(buf, '#')))
	return -1;
    if (p > buf+1) {
	*p = 0;
	attach->encoding_algorithm = savestr(buf+1);	/* +1 to skip tab */
    }
    if (!Sgets(buf, sizeof buf, ss) || !(p = rindex(buf, '#')))
	return -1;
    if (p > buf+1) {
	*p = 0;
	attach->data_type = savestr(buf+1);		/* +1 to skip tab */
    }
    else if (attach->content_type)
	attach->data_type = savestr(attach->content_type);
    if (!Sgets(buf, sizeof buf, ss) || !(p = rindex(buf, '#')))
	return -1;
    if (p > buf+1) {
	*p = 0;
	attach->mime_encoding_str = savestr(buf+1);	/* +1 to skip tab */
    }
    else if (attach->encoding_algorithm)
	attach->mime_encoding_str = savestr(attach->encoding_algorithm);
    if (!Sgets(buf, sizeof buf, ss) || !(p = rindex(buf, '#')))
	return -1;
    if (p > buf+1) {
	*p = 0;
	attach->mime_content_type_str = savestr(buf+1);	/* +1 to skip tab */
    }
    if (attach->mime_content_type_str) {
	p = attach->data_type;	/* Preserve indexed data_type */
	attach->data_type = 0;	/* User may have changed it, */
	DeriveMimeInfo(attach);	/* DeriveMimeInfo stomps it */
	if (p) {
	    xfree(attach->data_type);
	    attach->data_type = p;
	}
    }
    return 0;
}

static int
dumpAttachList(attach, fp, adjust)
Attach *attach;
FILE *fp;
long adjust;
{
    Attach *tmp = attach;

    if (tmp)
	do {
	    if (dumpAttach(tmp, fp, adjust) == EOF)
		return EOF;
	} while ((tmp = (Attach *)tmp->a_link.l_next) != attach);

    return fputc('\n', fp);
}

static int
fillAttachList(ap, ss)
Attach **ap;
Source *ss;
{
    char buf[BUFSIZ];
    Attach *attach;

    while (Sgets(buf, sizeof buf, ss) && buf[0] != '\n') {
	attach = zmNew(Attach);
	if (!attach) {
	    free_attachments(ap, FALSE);
	    return -1;
	}
	insert_link(ap, attach);
	if (fillAttach(attach, buf, ss) < 0) {
	    free_attachments(ap, FALSE);
	    return -1;
	}
    }
    return 0;
}

#define OMIT_FLAGS	(NEW|DO_UPDATE|FINISHED_MSG|PRESERVE|HIDDEN)

static int
dumpMsg(mesg, fp, dump_attach)
Msg *mesg;
FILE *fp;
int dump_attach;
{
    int ok = fprintf(fp, "%lu %lu %lu %d \"%s\" \"%s\"\n",
		mesg->m_flags				& ~OMIT_FLAGS,
		mesg->m_offset,
		mesg->m_size,
		mesg->m_lines,
		mesg->m_date_recv,
		mesg->m_date_sent);
    if (ok == 0 && dump_attach)
	return dumpAttachList(mesg->m_attach, fp, 0L);
    return ok;
}

static int
dumpMsgIx(mix, fp)
msg_index *mix;
FILE *fp;
{
    if (dumpMsg(&mix->mi_msg, fp, FALSE) != EOF)
	return dumpAttachList(mix->mi_msg.m_attach, fp, mix->mi_adjust);
    return EOF;
}

static int
fillMsg(mesg, mdata, ss)
Msg *mesg;
char *mdata;
Source *ss;
{
    char *dummy_date = "0000000000XXX-0000 XXX";

    mesg->m_date_recv = savestr(dummy_date);
    mesg->m_date_sent = savestr(dummy_date);

    /* Bart: Sun Jun 13 12:26:13 PDT 1993
     * Prevent array overwrite on corrupted indexes.  The 22 here
     * refers to strlen(dummy_date), but sscanf() has no way to
     * specify that as a parameter, so we hardwire it.  If the
     * date format changes, change this as well.	XXX
     */
    if (sscanf(mdata, "%lu %lu %lu %d \"%22[^\"]\" \"%22[^\"]\"\n",
	    &mesg->m_flags,
	    &mesg->m_offset,
	    &mesg->m_size,
	    &mesg->m_lines,
	    mesg->m_date_recv,
	    mesg->m_date_sent) != 6) {
	xfree(mesg->m_date_recv);
	xfree(mesg->m_date_sent);
	return -1;
    }
    return fillAttachList(&mesg->m_attach, ss);
}
