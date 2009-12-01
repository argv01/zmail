/* foload.c	Copyright 1992 Z-Code Software Corp. */

#ifndef lint
static char	foload_rcsid[] = "$Id: foload.c,v 2.121 2005/05/31 07:36:41 syd Exp $";
#endif

#include "zmail.h"
#include "catalog.h"
#include "dates.h"
#include "foload.h"
#include "glob.h"
#include "linklist.h"
#include "strcase.h"
#include "zmaddr.h"
#include "zmindex.h"
#include "zmstring.h"

#if defined(DARWIN)
#include <libgen.h>
#endif

#ifdef GUI
#include "zmframe.h"
#endif /* GUI */

#ifndef LICENSE_FREE
#include "license/server.h"
#endif /* LICENSE_FREE */

static void phony_header();

/*
 * Identify the line that separates two mail messages.  As an optimization
 * for FolderStandard folders, returns a pointer to the parsed date taken
 * from the separator line.  For FolderDelimted folders, returns buf.  In
 * either case, returns NULL if buf is not a message separator.
 *
 * Bart: Thu Jun 10 14:07:27 PDT 1993
 * If fotype is FolderUnknown, try both and succeed for either.
 */
char *
match_msg_sep(buf, fotype)
char *buf;
FolderType fotype;
{
    int warn = ison(glob_flags, WARNINGS);
    char *p = NULL;

    if ((fotype & FolderDirectory) == FolderDirectory)
	return NULL;

    /* First check quickly for msg_separator */
    if ((fotype == FolderDelimited || fotype == FolderUnknown) &&
	    msg_separator && *msg_separator) {
	if (!strncmp(buf, msg_separator, strlen(msg_separator)))
	    return buf;
    }
    if (fotype != FolderDelimited && !strncmp(buf, "From ", 5)) {
	/* skip the address to get to the date */
	p = buf + 5;	/* skip "From " */
	skipspaces(0);
	p = any(p, " \t");
	turnoff(glob_flags, WARNINGS);
	if (p && !(p = parse_date(p + 1, 0L))) {
	    /* Try once more the hard way */
	    extern char *get_name_n_addr();
	    if ((p = get_name_n_addr(buf + 5, NULL, NULL)) != 0)
		p = parse_date(p + 1, 0L);
	}
	if (warn)
	    turnon(glob_flags, WARNINGS);
    }

    return p;
}

/*
 *  For uucp mailers that use >From lines with "remote from <path>":
 * (where "path" is a hostname or pathnames)
 *
 *  a. Set the return_path to the empty string.
 *  b. For each From_ or >From_ line:
 *  c. Save the username (second token).
 *  d. Save the date (3-7 tokens).
 *  e. If it has a "remote from" then append the remote host
 *	(last token) followed by a "!" to the return_path.
 *  f. If the saved username has a '@' but no '!' then convert it
 *	to UUCP path form.
 *  g. Append the saved username to return_path.
 */
void
parse_from(ss, path)
struct Source *ss;
char path[];
{
    char user[256], buf[256]; /* max size for each line in a mail file */
    register char *p = NULL;

    user[0] = path[0] = '\0';
    while (Sgets(buf, sizeof buf, ss)) {
	if (strncmp(buf, ">From ", 6))
	    break;
	p = buf + 6;

	(void) sscanf(p, "%s", user);

	while (p = index(p+1, 'r')) {
	    if (!strncmp(p, "remote from ", 12)) {
		char *p2 = path+strlen(path);
		skipspaces(12);
		(void) sscanf(p, "%s", p2); /* add the new machine to path */
		(void) strcat(p2, "!");
		break;
	    }
	}
    }
    if (user[0])
        (void) bang_form(path + strlen(path), user);
    Sungets(buf, ss);
}

int
explode_multipart(msg_no, mpart, ss, lines, inlining)
int msg_no, *lines;
Attach *mpart;
struct Source *ss;
int inlining;
{
    FolderType fotype = folder_type;
    Attach *atmp = link_prev(Attach,a_link,mpart);
    Attach *attach = msg[msg_no]->m_attach;
    Source *stmp = ss;
    char *msep = msg_separator;
    char *boundary;
    int ret = 0, dummy = 0;
    int ro = ison(folder_flags, READ_ONLY);

    if (isoff(msg[msg_no]->m_flags, MIME) ||
	GetMimePrimaryType(mpart->mime_content_type_str) != MimeMultipart ||
	!(boundary = FindParam(MimeGlobalParamStr(BoundaryParam),
				&attach->content_params))) {
#ifdef NOT_NOW
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 1012, "Attachment is not a properly formed MIME multipart"));
#endif /* NOT_NOW */
	return 0;
    }

    /* Handle special case -- called from detach_parts()
     */
    if (!lines)
	lines = &dummy;
    if (!ss) {
	if (!tmpf)
	    return -1;
	ss = Sinit(SourceFile, tmpf);
	if (Sseek(ss, mpart->content_offset, 0) == EOF)
	    return -1;
    }

    on_intr();

    /* Set up the folder to pretend that the multipart boundary strings
     * are actually message delimiters.
     */
    msg[msg_no]->m_attach = mpart;
    folder_type = FolderDelimited;
    turnon(folder_flags, READ_ONLY);
    msg_separator = savestr("--");
    strapp(&msg_separator, boundary);

    /* Reset the attachment structure to be properly refilled as an
     * exploded multipart.
     */
    FreeContentParameters(&mpart->content_params);
    xfree(mpart->mime_content_type_str);
    mpart->mime_content_type_str = mpart->orig_mime_content_type_str;
    mpart->orig_mime_content_type_str = 0;

    /* Call load_attachments to explode the multipart
     */
    ret = load_attachments(msg_no, ss, lines, TRUE);

#ifdef NOT_NOW
    /* Handle errors.  As it turns out, every bad mangling of boundary
     * strings that I can find still leaves us in a pretty sane state, 
     * so short of backing out altogether there's really not much of
     * any use to be done here.
     *
     * The real problem may be with nested multiparts -- a multipart
     * that contains two other multiparts, where an error occurs in
     * the first of those multiparts, leaving its sibling unexploded.
     * We'll have trouble exploding the second nested multipart later
     * in that case, because we'll have discarded boundary information
     * from the enclosing parent mulipart.  I think this could cause
     * parts from different nesting levels to be treated as all being
     * the same (always last) part from the inner nesting, but that
     * would require a pretty unlikely combination of part-boundary
     * mistakes in the whole structure.
     */
    if (x != 0) {
	/* We had a failure (< 0), or were interupted (> 0). */
    }
#endif /* NOT_NOW */

    /* Remove the multipart from the chain, leaving the exploded
     * sublist behind.
     */
    remove_link(&msg[msg_no]->m_attach, mpart);
    msg[msg_no]->m_attach = link_next(Attach,a_link,atmp);
    insert_link(&msg[msg_no]->m_attach, mpart);
    msg[msg_no]->m_attach = attach;
    turnon(mpart->a_flags, AT_EXPLODED);

    if (!inlining) {
	mpart = link_next(Attach,a_link,attach);
	while (mpart != attach) {
	    atmp = link_next(Attach,a_link,mpart);
	    if (ison(mpart->a_flags, AT_EXPLODED)) {
		remove_link(&msg[msg_no]->m_attach, mpart);
		free_attach(mpart, TRUE);
	    }
	    mpart = atmp;
	}
    }

    /* Restore the folder state
     */
    folder_type = fotype;
    xfree(msg_separator);
    msg_separator = msep;
    if (!ro && isoff(folder_flags, CORRUPTED))
	turnoff(folder_flags, READ_ONLY);

    /* Clean up special cases
     */
    if (!stmp) {
	if (ss->st)
	    Sclose(ss->st);
	xfree(ss);	
    }

    off_intr();

    return ret;
}

/* Holds boolean value of VarUseContentLength; referenced below */
unsigned int use_content_length = 1;

/*
 * Load information about all of a message's attachments into the msg
 * array, by scanning from the source, up to the size specified in the 
 * struct attach.
 * Also, copy all the attachments to the temp file, exactly duplicating
 * the source data.
 * The source is currently restricted to being a file.
 * Some information about the attachments has been loaded from the
 * main message headers before calling this function.
 *
 * Put the amount read into the parameter "lines".
 *
 * WARNING: You are about to enter what may be the most horrific
 * spaghetti code you have ever seen.   It will be rewritten, someday.
 *
 * 					CML, Wed May 26 17:16:27 1993
 */
int
load_attachments(msg_no, ss, lines, explode)
struct Source	*ss;
int	msg_no, *lines, explode;
{
    Attach	*attach = msg[msg_no]->m_attach, *next = 0;
    long	msg_len = attach->content_length, rd_len = 0;
    int		multipart = 0, text, c, x = 0; 
    int		rdonly = ison(folder_flags, READ_ONLY);
    int		count_lines = FALSE, on_hdr = TRUE, sun_kluge = FALSE;
    char	buf[BUFSIZ], buf2[BUFSIZ], *code = NULL;
    char	boundary[256], endingBoundary[256];
    char	*boundaryParam;
    int		reachedMimeEndingBoundary = 0;
    long	wr_len, bodypart_lines;
    int		assertedMessageContentLength = -1;

    /* 
     * Treat it as MIME if it has a content-type header 
     * and no X-Zm-Content-Type header,
     * and the content-type header doesn't indicate that it's a sun format
     * or text, and the type is recognized
     */
    if (isoff(msg[msg_no]->m_flags, MIME) &&
	!attach->content_type &&
	attach->mime_content_type_str) {
	if (ci_strcmp(attach->mime_content_type_str, "x-sun-attachment") &&
	    (ci_strcmp(attach->mime_content_type_str, "text") &&
	     is_known_type(attach->mime_content_type_str))) {
	    turnon(msg[msg_no]->m_flags, MIME);
	    Debug("Message %d: Treating non-MIME message of type %s as MIME.\n" ,
		  msg_no + 1,
		  attach->mime_content_type_str);
	}
	ZSTRDUP(attach->content_type, attach->mime_content_type_str);
    }

    if (ison(msg[msg_no]->m_flags, MIME)) {
	if (!ci_strcmp(attach->mime_content_type_str, "x-zm-multipart")) {
	    /* it's a Z-Mail multipart which got mislabelled as MIME 
	     * This bug occured with an early version of 3.0a when you
	     * resent an old Z-Mail message
	     */
	    turnoff(msg[msg_no]->m_flags, MIME);
	    multipart = 1;
	    ZSTRDUP(attach->mime_content_type_str,
		   MimeTypeStr(MultipartMixed));
	} else {
	    /* Ignore any size info from non-MIME headers, saving the 
	     * content_length as a backup in case we appear to have run
	     * into the next message
	     */
	    assertedMessageContentLength = attach->content_length;
	    attach->content_lines = 0;
	    attach->content_length = 0;
	    msg_len = 0;
	    DeriveMimeInfo(attach);
	    boundaryParam = FindParam(MimeGlobalParamStr(BoundaryParam),
				      &attach->content_params);
	    if (boundaryParam) {
		sprintf(boundary, "--%s\n", boundaryParam);
		sprintf(endingBoundary, "--%s--\n", boundaryParam);
	    }
	}
    }
    
    /* Give it a content name if it doesn't have one */
    if (!attach->content_name || attach->mime_type == MessageExternalBody) {
	attach->a_name = alloc_tempfname_dir("prt", get_detach_dir());
	attach->content_name = savestr(basename(attach->a_name));
#if 0
	attach->content_name = savestr(zmVaStr("F%dM%d.A%02d",
			    current_folder->mf_number, msg_no + 1, x));
#endif /* 0 */
	turnon(attach->a_flags, AT_NAMELESS);
    } else if (strcpyStrip(buf2, attach->content_name, TRUE)) {
	print(catgets(catalog, CAT_MSGS, 833, "Message %d: Shell metacharacters included in attachment filename %s.\nUsing: %s.\n"),
	      msg_no + 1,
	      attach->content_name, buf2);
	attach->orig_content_name = attach->content_name;
	attach->content_name = savestr(buf2);
    }
    
    /* 
     * look at the content_type to see if this is a multipart message, 
     * If so, set multipart, set sun_kluge (if it's rfc1154 or 
     * x-sun-attachment) and set the content_type to "multipart".
     *
     * If it's an old-style Z-Mail message it will have a content type 
     * of multipart or X-Zm-Multipart.  (If it were a single-part Z-Mail text 
     * message, it wouldn't have an attach struct, so we wouldn't be here.)
     *
     * If the content type is text, it may be an  old-ATT-format message; 
     * If the content type is text/plain, it's a MIME message.  Either
     * way, don't load it here, just return and we'll treat it like a normal
     * text message.
     *
     * In the case of MIME, this is an error if it's encoded.
     *						CML Fri May 28 13:05:15 1993
     */
    if (attach->content_type) {
	if (!(text = !ci_strcmp(attach->content_type, "text"))) {
	    if (!ci_strcmp(attach->content_type, 
			      "x-sun-attachment")) {
		multipart = sun_kluge = TRUE;	/* See below */
	    } else if (!ci_strcmp(attach->content_type, RFC1154)) {
		multipart = sun_kluge = TRUE; /* count_lines = TRUE below */
		on_hdr = FALSE;
		code = attach->content_abstract;
	    } else if (!ci_strncmp(attach->content_type, "text/plain", 10)) {
		if (!attach->content_abstract)
		    /* XXX This should look up the comment for text/plain,
		     * rather than using something from the catalog 
		     */
		    ZSTRDUP(attach->content_abstract, 
			   catgets( catalog, CAT_MSGS, 476, "plain text" ));
	    } else if (isoff(msg[msg_no]->m_flags, MIME)) {
		multipart =
		    !ci_strcmp(attach->content_type, "multipart") ||
			!ci_strcmp(attach->content_type, "x-zm-multipart");
		if (multipart)
		    ZSTRDUP(attach->mime_content_type_str,
			   MimeTypeStr(MultipartMixed));
	    } else if (is_multipart(attach)) {
		/* it's a MIME multipart */
		multipart = TRUE;
		attach->mime_type = MultipartMixed;
	    }
	    /*
	     * else it's a mime single part message of major type text, 
	     * we want to load normally and show the appropriate icon
	     */
	    if (multipart)
		ZSTRDUP(attach->content_type, "multipart");
	}
    }
    attach->content_offset = rdonly? Stell(ss) : ftell(tmpf);
    /* 
     * Just give up on loading if it's not multipart; we'll display it 
     * like regular text.
     * If it doesn't have a length or MIME boundaries, we can't 
     * load it here now.
     * If we're guessing about the type, we want to leave the attach struct 
     * so that the attachments button will become sensitive, and the user 
     * can thereby bring up the Attachments dialog and manually detach or 
     * transform the message, in case we were wrong.
     *
     * THIS IS STILL A HACK			CML Thu Jun  3 19:01:01 1993
     */
    if (!multipart ) {
	/* To inline (without decoding!) all top-level text types, use:
	if (!ci_strncmp(attach->content_type, "text", 4))
	 */
	if (!ci_strcmp(attach->content_type, "text")) {
	    if (attach->content_length && use_content_length) {
		turnon(attach->a_flags, AT_BYTECOUNTED);
		DeriveMimeInfo(attach);
		return 2;
	    }
	    return -1;
	}
	/* Don't treat stuff like Andrew X-BE2 as MIME */
	else if (attach->mime_content_type_str &&
		 isoff(msg[msg_no]->m_flags, MIME) &&
		 !is_known_type(attach->mime_content_type_str)) {
	    return -1;
	} else {
	    /* This will be filled in later, after the next message
	     * is found
	     */
	    if (use_content_length &&
		    (assertedMessageContentLength >= 0 ||
		     attach->content_length)) {
		turnon(attach->a_flags, AT_BYTECOUNTED);
		if (assertedMessageContentLength >= 0)
		    attach->content_length = assertedMessageContentLength;
	    }
	    if (isoff(msg[msg_no]->m_flags, MIME)) {
		turnon(msg[msg_no]->m_flags, MIME);
		if (!(attach->content_type)) {
		    Debug("Message %d: Treating non-MIME message of unknown type as MIME type %s.\n" ,
			  msg_no + 1,
			  MimeTypeStr(TextPlain));
		    ZSTRDUP(attach->mime_content_type_str, 
			   MimeTypeStr(TextPlain));
		} else {
		    Debug("Message %d: Treating non-MIME message of type %s as MIME.\n",
			  msg_no + 1,
			  attach->content_type);
		    ZSTRDUP(attach->mime_content_type_str, 
			   attach->content_type);
		}
		attach->content_lines = 0;
		msg_len = 0;
		DeriveMimeInfo(attach);
	    }
	    return 2;
	}
    }
    
    /* If it's not a MIME message, and we don't know how long it is, 
     * just give up and free the whole attach structure
     */
    if (isoff(msg[msg_no]->m_flags, MIME)) {
	if (!attach->content_length && !attach->content_lines && !sun_kluge) {
	    if (ison(glob_flags, WARNINGS))
		print(catgets( catalog, CAT_MSGS, 479, "Message %d: No content length for attachment.\n" ),
		      msg_no + 1);
	    msg[msg_no]->m_attach = (Attach *)0;
	    xfree((char *)attach);
	    return -1;
	} else if (!attach->content_length) {
	    /* Sun passes no content-size information in the main message 
	     * header,
	     * instead specifying only that the contents are attachments.  They
	     * then put Content-Lines in each attachment part.  Blecch.
	     */
	    sun_kluge = !attach->content_lines;
	    count_lines = TRUE;
	} else
	    attach->content_lines = 0; /* Ignore lines in favor of length */
    }
    
    /* If it's MIME, copy the preamble to the temp file */
    if (ison(msg[msg_no]->m_flags, MIME) && multipart) {
	if (!boundaryParam) {
	    print(catgets( catalog, CAT_MSGS, 480, "Cannot load attachment, message %d.\n\
Bad message format -- multipart message has no boundary parameter!\n" ),
		  msg_no + 1);
	    return -1;
	} else {
	    do {
		errno = 0;
		if (Sgets(buf, sizeof buf, ss) &&
		    (rdonly || fputs(buf, tmpf) != EOF)) {
		    /* If we reach a message separator in the middle,
		     * bail out, unless the message had a header 
		     * specifying content length.  In that case, 
		     * don't worry about a message separater if
		     * we haven't passed that length.
		     */
		    if (match_msg_sep(buf, folder_type) &&
			((assertedMessageContentLength < 0) || 
			 ((rd_len+attach->content_length+strlen(buf)) >
			  assertedMessageContentLength))) {
			print(catgets( catalog, CAT_MSGS, 481, "Cannot load attachment, message %d.\n\
Reached next message before finding MIME boundary.\n" ), 
			      msg_no + 1);
			return -1;
		    } else {
			*lines += 1;
		    }
		} else {
		    print(catgets( catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n" ),
			  errno? "" : catgets( catalog, CAT_MSGS, 483, "Internal Error: " ),
			  msg_no + 1,
			  errno? ": " : "",
			  errno? strerror(errno) : "");
		    return -1;
		}
		rd_len += (wr_len = strlen(buf));
		attach->content_length += wr_len;
	    /* We need an EXACT match to have a boundary, no comments 
	     * allowed 
	     */
	    } while (strcmp(buf, boundary) != 0);
	}
    }
    
    /* 
     * This is the main loop for loading attachments.
     *
     * Convert content_lines to content_length in this loop,
     * then ignore the content_lines thereafter.
     *
     * Bart: Thu Feb  3 15:58:13 PST 1994
     * Compute the real content_lines and reset it, for use in doing CR/LF
     * translation in DOS/Windows attachment processing.  The content_lines
     * should not include the part headers.
     */
    while (msg_len > 0 || attach->content_lines > 0 || sun_kluge ||
	   (ison(msg[msg_no]->m_flags, MIME) && !reachedMimeEndingBoundary)) {
	/* same interrupt checking as load_folder */
	if (!(*lines % 500) &&
	    check_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 484, "Loading message %d (attachment)" ),
				   msg_no + 1), -1))
	    return 1;
	
	if (multipart) {
	    long	cnt = 0;
	    if (code && !next) {
		if (!*code)
		    break;
		code = parse_encoding(code, &next);
		if (next) {
		    bodypart_lines = 0;
		    next->content_offset = rdonly? Stell(ss) : ftell(tmpf);
		    insert_link(&attach, next);
		} else
		    break;
	    }
	    /* Bart: Tue Jan 26 14:44:48 PST 1993
	     * Handle recursive MESSAGE encoding, one level deep only.
	     * This covers our own ass if we have to fall back on an
	     * Encoding: header that we generated (X.400 mangling?).
	     */
	    if (code) {
		if (next->content_type)
		    on_hdr = (ci_strcmp(next->content_type, "MESSAGE") == 0);
		else
		    x++;
	    }
	    /* First parse the recognized headers, then snarf the rest.
	     * Unfold long headers.
	     */
	    *buf2='\0';
	    while (on_hdr && Sgets(buf, sizeof buf, ss)) {
		if (!rdonly && fputs(buf, tmpf) == EOF)
		    return 1;
		*lines += 1;
		if (buf[0] != '\n') {
		    if ((*buf == ' ') || (*buf == '\t')) {
			if ((strlen(buf) + strlen(buf2) + 1) < sizeof(buf2)) {
			    *buf = ' ';
			    (void) no_newln(buf2);
			    (void) strcpy(buf2+strlen(buf2), buf);
			} else {
#ifdef NOT_NOW
			    /* This just disturbs users */
			    error(ZmErrWarning, 
				  "Ignoring end of long incoming message header (probably ok).");
#endif
			}
		    } else {	    
			if (*buf2) /* won't be the first time through */
			    (void) content_header(buf2, &next);
			strcpy(buf2, buf);
		    }
		} else {
		    if (*buf2)
			(void) content_header(buf2, &next);
		    x++;
		    on_hdr = FALSE;
		    bodypart_lines = 0;
		    if (next) {
			next->content_offset = 
			    rdonly? Stell(ss) : ftell(tmpf);
			insert_link(&attach, next);
		    }
		}
		if (count_lines) {
		    if (!sun_kluge)
			attach->content_lines -= 1;
		} else
		    rd_len += strlen(buf);
	    }
	    if (!next && ison(msg[msg_no]->m_flags, MIME)) {
		next = zmNew(Attach);
		bzero((char *) next, sizeof(Attach));
		next->content_offset = 
		    rdonly? Stell(ss) : ftell(tmpf);
		bodypart_lines = 0;
		insert_link(&attach, next);
	    }
	    if (next){
		if (ison(msg[msg_no]->m_flags, MIME)) {
		    /* Ignore any size info from non-MIME headers */
		    next->content_lines = 0;
		    next->content_length = 0;
		    DeriveMimeInfo(next);
		}
		if (!next->content_name || next->mime_type == MessageExternalBody) {
		    next->content_name = alloc_tempfname_dir("prt", get_detach_dir());
#if 0
			savestr(zmVaStr("F%dM%d.A%02d",
					current_folder->mf_number, 
					msg_no + 1, x));
#endif /* 0 */
		    turnon(next->a_flags, AT_NAMELESS);
		} else if (strcpyStrip(buf2, next->content_name, TRUE)) {
		    print(catgets(catalog, CAT_MSGS, 833, "Message %d: Shell metacharacters included in attachment filename %s.\nUsing: %s.\n"),
			  msg_no + 1,
			  next->content_name, buf2);
		    next->orig_content_name = next->content_name;
		    next->content_name = savestr(buf2);
		}
		if (isoff(msg[msg_no]->m_flags, MIME)) {
		    if (!next->content_type)
			next->content_type = savestr("text");
		    if (!next->data_type)
			next->data_type = savestr(next->content_type);
		    /* 
		     * Now that we've done the headers,
		     * figure out how we're going to load the attachment 
		     * contents, and use the dreaded goto if we need to 
		     * do something unusual.
		     *			CML Fri May 28 13:06:06 1993
		     */
		    if (count_lines || next->content_lines) {
			if (next->content_lines)
			    next->content_length = 0;
			else if (!next->content_length)
			    goto load_zero_length;
			if (next->content_lines || attach->content_lines)
			    goto load_by_lines;
			/* else we've got a sun_kluge with Content-Length
			 * specified instead of Lines.  What a disaster.
			 */
		    }
		}
	    }
	    if (!next || 
		(!sun_kluge && isoff(msg[msg_no]->m_flags, MIME) &&
		 (rd_len+next->content_length > attach->content_length))) {
		if (isoff(msg[msg_no]->m_flags, MIME))
		    print("Content %s mismatch in message %d, attachment %d.\n",
			  count_lines? catgets( catalog, CAT_MSGS, 485, "line count" ) : catgets( catalog, CAT_MSGS, 486, "length" ), msg_no + 1, x);
		else
		    print(catgets( catalog, CAT_MSGS, 487, "Cannot load message %d, attachment %d.\n" ),
			  msg_no + 1, x);
		return -1;
	    }
	    /*
	     *  This is MIME; load the attachment by searching for
	     *  the boundary and copy it to the temp file.  Set the
	     *  content_length appropriately.
	     * 				CML Mon May 31 18:17:08 1993
	     */
	    if (ison(msg[msg_no]->m_flags, MIME)) {
		if (istool && 
		    check_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 488, "Loading message %d (attachment %d)" ),
					   msg_no + 1, x), -1))
		    return 1;
		if (explode &&
			GetMimePrimaryType(next->mime_content_type_str) ==
			    MimeMultipart) {
		    int ret = explode_multipart(msg_no, next, ss, lines, TRUE);
		    rd_len += next->content_length;
		    if (ret)
			return ret;
		}
		/* Read the whole body, a line at a time */
		do {
		    if (!(*lines % 500) && 
			check_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 489, "Loading message %d (attachment %d, line %d)" ),
					       msg_no + 1, x, *lines), -1))
			return 1;
		    errno = 0;
		    if (cnt = Sgets(buf, sizeof buf, ss) &&
			(rdonly || fputs(buf, tmpf) != EOF)) {
			/* If we reach a message separator in the middle,
			 * bail out, unless the message had a header 
			 * specifying content length.  In that case, 
			 * don't worry about a message separater if
			 * we haven't passed that length.
			 */
			if (match_msg_sep(buf, folder_type) &&
			    ((assertedMessageContentLength < 0) || 
			     ((rd_len+next->content_length+strlen(buf)) >
			      assertedMessageContentLength))) {
			    print(catgets( catalog, CAT_MSGS, 481, "Cannot load attachment, message %d.\n\
Reached next message before finding MIME boundary.\n" ), 
				  msg_no + 1);
			    return -1;
			} else {
			    bodypart_lines++;
			    *lines += 1;
			}
		    } else {
			print(catgets( catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n" ),
			      errno? "" : catgets( catalog, CAT_MSGS, 483, "Internal Error: " ),
			      msg_no + 1,
			      errno? ": " : "",
			      errno? strerror(errno) : "");
			return -1;
		    }
		    rd_len += (wr_len = strlen(buf));
		    next->content_length += wr_len;
		/* We need an EXACT match to have a boundary, 
		 * no comments allowed
		 */
		} while (strcmp(buf, endingBoundary) && strcmp(buf, boundary));

		/* increase the attachment size appropriately, including
		 * the boundary length
		attach->content_length += next->content_length;
		 */
		attach->content_length =
		    (rdonly? Stell(ss) : ftell(tmpf)) - attach->content_offset;
		/* Normally, don't count the boundary or preceding newline, 
		 * which is actually part of the boundary, in the attachment 
		 * length.
		 * However, if the part is is multipart, DO count the
		 * preceding newline.  This compensates for illegal mime
		 * messages which do not provide the required two newlines
		 * between the closing boundary of the body and the 
		 * boundary of the enclosing message.
		 * e.g. the following is invalid MIME, but we accept it
		 *
		 *  --OtherAccess--
		 *  --NextPart--
		 *
		 * It's possible that this could cause us problems at some
		 * point, as we are counting a single newline for two purposes,
		 * as part of both boundaries
		 */
		if (GetMimePrimaryType(next->content_type) == MimeMultipart ||
			bodypart_lines <= 1)
		    next->content_length -= wr_len;
		else if (next->content_length > wr_len) {
		    next->content_length -= wr_len + 1;
		    /* Bart: Thu Feb  3 17:12:40 PST 1994
		     * We may not actually want to do this -- depends on
		     * how copy_msg() processes the attachment.
		     */
		    bodypart_lines--;	/* XXX */
		}
		bodypart_lines--;
		if ((strlen(buf) == strlen(endingBoundary)) &&
		    !strcmp(buf, endingBoundary))
		    reachedMimeEndingBoundary = TRUE;
	    /*
	     *  This is an old Z-Mail message; load the attachment using the
	     *  content length and copy it to the temp file.
	     * 				CML Fri May 28 13:06:22 1993
	     */
	    } else if (!ci_strcmp(next->content_type, "text")) {
		if (istool && check_intr_mnr(
			    zmVaStr(catgets( catalog, CAT_MSGS, 488, "Loading message %d (attachment %d)" ),
			    msg_no + 1, x), -1))
		    return 1;
		/* Read the whole body, a line at a time */
		for (cnt = next->content_length; cnt > 0; cnt -= wr_len) {
		    if (!(*lines % 500) && check_intr_mnr(
			zmVaStr(catgets( catalog, CAT_MSGS, 489, "Loading message %d (attachment %d, line %d)" ),
				msg_no + 1, x, *lines), -1))
			return 1;
		    errno = 0;
		    if (Sgets(buf, sizeof buf, ss) &&
			    (rdonly || fputs(buf, tmpf) != EOF)) {
			*lines += 1;
			bodypart_lines++;
		    } else {
			print(catgets( catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n" ),
			    errno? "" : catgets( catalog, CAT_MSGS, 483, "Internal Error: " ),
			    msg_no + 1,
			    errno? ": " : "",
			    errno? strerror(errno) : "");
			return -1;
		    }
		    rd_len += (wr_len = strlen(buf));
		}
	    } else {
		/* Update the task meter but don't actually interrupt */
		if (istool)
		    check_intr_mnr(zmVaStr(catgets( catalog, CAT_MSGS, 488, "Loading message %d (attachment %d)" ),
			msg_no + 1, x), -1);
		/* Read the whole body, in the largest chunks possible */
		for (cnt = next->content_length; cnt > 0; cnt -= wr_len) {
		    errno = 0;
		    wr_len = Sread(buf, sizeof(char), min(cnt, sizeof buf), ss);
		    if (wr_len > 0 && (rdonly ||
			    fwrite(buf, sizeof(char), wr_len, tmpf) == wr_len))
			rd_len += wr_len;
		    else {
			print(catgets( catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n" ),
			    errno? "" : catgets(catalog, CAT_MSGS, 483, "Internal Error: "),
			    msg_no + 1,
			    errno? ": " : "",
			    errno? strerror(errno) : "");
			return wr_len > 0? 1 : -1;
		    }
		}
		*lines += 1; /* Treat binary attachments as a single line */
	    }
	    next = 0;
	    if (cnt < 0) {
		print("Content length mismatch in message %d.\n", msg_no + 1);
		return -1;
	    }
	} else if (!text && !count_lines) {
	    /*
	     * This clause of this if statement is dead code.
	     * This case is handled as MIME now.
	     * 				CML Fri May 28 13:06:37 1993
	     */
	    errno = 0;
	    rd_len = Sread(buf, sizeof(char), min(msg_len, sizeof buf), ss);
	    if (rd_len < 1 || !rdonly &&
		    fwrite(buf, sizeof(char), rd_len, tmpf) < rd_len) {
		print(catgets(catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n"),
		    errno? "" : catgets(catalog, CAT_MSGS, 483, "Internal Error: "),
		    msg_no + 1,
		    errno? ": " : "",
		    errno? strerror(errno) : "");
		return rd_len < 1? -1 : 1;
	    }
	} else { /* text or line-counted attachment */
load_by_lines:
	    /* 
	     * some trickiness here.  This handles Sun format which has
	     * a line count.  Read one line, and continue with the loop,
	     * so that we re-execute the loop test after each line
	     * 				CML Fri May 28 13:06:37 1993
	     */
	    errno = 0;
	    if (Sgets(buf, sizeof buf, ss)) {
		*lines += 1;
		bodypart_lines++;
		if (!rdonly && fputs(buf, tmpf) == EOF) {
		    print(catgets(catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n"),
			errno? "" : catgets(catalog, CAT_MSGS, 483, "Internal Error: "),
			msg_no + 1,
			errno? ": " : "",
			errno? strerror(errno) : "");
		    return 1;
		}
		if (count_lines) {
		    if (!sun_kluge)
			attach->content_lines -= 1;
		} else
		    msg_len -= strlen(buf);
		if (next) {
		    if (!next->content_type)
			next->content_type = savestr("text");
		    if (!next->data_type)
			next->data_type = savestr(next->content_type);
		    if (next->content_lines)
			next->content_lines -= 1;
		    else
			next->content_length -= strlen(buf);
		    if (next->content_lines || next->content_length > 0)
			continue;
		} else
		    continue;
	    } else {
		print(catgets(catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n"),
		    errno? "" : catgets(catalog, CAT_MSGS, 483, "Internal Error: "),
		    msg_no + 1,
		    errno? ": " : "",
		    errno? strerror(errno) : "");
		return -1;
	    }
	}
	/* End of huge if clause; we get here after successfully 
	 * reading an attachment
	 */
	if (!count_lines)
	    msg_len -= rd_len;
load_zero_length:
	/* 
	 * Clean up the rest of this attachment and get ready to read
	 * the next one
	 */
	if (next && next->content_length <= 0) {
	    next->content_length =
		(rdonly? Stell(ss) : ftell(tmpf)) - next->content_offset;
	    /* Bart: Thu Feb  3 17:29:25 PST 1994
	     * This is for the DOS/Windows guys, who need to know how
	     * many CRs they output along with the LFs they counted
	     * when detaching/displaying-inline these attachment parts.
	     * If this is 0, it was a non-text attachment from an old
	     * zmail, loaded using the X-Zm-Content-Length: header ...
	     */
	    next->content_lines = bodypart_lines;
	}
	/* for mime messages, copy the epilogue and fix the 
	 * attachment content_length as well
	 */
	if (reachedMimeEndingBoundary) {
	    int end_of_msg = 0;
	    while (!end_of_msg && Sgets(buf, sizeof(buf), ss)) {
		if (!(end_of_msg = !!match_msg_sep(buf, folder_type))) {
		    if (!rdonly && fputs(buf, tmpf) == EOF) {
			print(catgets(catalog, CAT_MSGS, 482, "%sCannot load attachment, message %d%s%s.\n"),
			      errno? "" : catgets(catalog, CAT_MSGS, 483, "Internal Error: "),
			      msg_no + 1,
			      errno? ": " : "",
			      errno? strerror(errno) : "");
			return 1;
		    }
		    *lines += 1;
		    attach->content_length += strlen(buf);
		} else
		    Sungets(buf, ss);
	    }
	}
	/* There may be an additional "----------", not included in the
	 * X-Zm-Content-Length, separating each message or message part.
	 * 
	 * Skip any boundary and correct for small line count errors
	 */
	if (msg_len >= 0 && ((c = Sgetc(ss)) != EOF || sun_kluge)) {
	    int foundSunStyleBoundary = 0;

	    /* Bart: Tue Jan 26 14:31:12 PST 1993
	     * Be lenient in what we accept -- don't require the blank
	     * line between parts in RFC1154 encodings.
	     *
	     * This is actually a workaround for our own implementation
	     * of using an Encoding: to describe Sun attachment format.
	     * Strictly speaking, we're violating 1154 ourselves.  XXX
	     */
	    if (code && c != '\n') {
		Sungetc(c, ss);
		c = '\n';
	    } else if (sun_kluge && c != '-') {
		if (c != '\n') {
		    /* If it's an off-by-one error in the whole message,
		     * try to fix it, otherwise it's too far off to fix.
		     */
		    if (attach->content_lines == 0 &&
			    (!code || !*code) && (buf[0] = c) &&
			    (c == EOF ||
			    Sgets(buf + 1, sizeof buf - 1, ss) &&
			    match_msg_sep(buf, folder_type))) {
			/* We only unget on the success case because we
			 * will seek back after return of -1 on failure.
			 * This is a flaw in Source ...			XXX
			 */
			Sungets(buf, ss);
			print("Fixing line count mismatch in message %d.\n",
			    msg_no + 1);
			if (c == EOF)
			    sun_kluge = 0;	/* Break while() loop */
			/* Bart: Wed Nov  2 11:39:24 PST 1994
			 * There's something wrong here when we fix up
			 * an attachment that's at the end of a message
			 * but that isn't at the end of a folder (c != EOF).
			 * I'm not sure exactly what, though.
			 */
			c = '\n';
			if (!code)
			    attach->content_lines = 1;
			else if (!rdonly)
			    (void) fputc('\n', tmpf);
		    } else {
			print("Line count mismatch in message %d.\n",
			    msg_no + 1);
			return -1;
		    }
		} else {
		    if (!rdonly)
			(void) fputc(c, tmpf);
		    if (!code)
			break;
		}
	    } else {
		foundSunStyleBoundary = (c == '-');
		for (rd_len = 1; c == '-'; rd_len++) {
		    if (!rdonly)
			(void) fputc(c, tmpf);
		    c = Sgetc(ss);
		}
	    }
	    if (c != '\n') {
		if (count_lines) {
		    print(catgets( catalog, CAT_MSGS, 500, "Line count mismatch in message %d.\n" ), msg_no + 1);
		    return -1;
		} else
		    Sungetc(c, ss); /* Is this really good enough? */
	    } else if (!code) {
		if (count_lines && !sun_kluge) {
		    if (attach->content_lines)
			attach->content_lines -= 1;
		    else if (foundSunStyleBoundary) {
			print(catgets( catalog, CAT_MSGS, 500, "Line count mismatch in message %d.\n" ), msg_no + 1);
			return -1;
		    } /* else we've reached the end safely */
		} else if (!count_lines)
		    msg_len -= rd_len;
		if (!rdonly)
		    (void) fputc('\n', tmpf);
	    }
	}
	/* Bart: Tue Jan 26 15:02:28 PST 1993
	 * Throw away BOUNDARY parts which have been put in struct attaches, 
	 * for Sun or MIME encoded as RFC1154.
	 */
	if (next && next->content_type && next->encoding_algorithm &&
		ci_strcmp(next->encoding_algorithm, "BOUNDARY") == 0 &&
		ci_strcmp(next->content_type, "TEXT") == 0) {
	    remove_link(&attach, next);
	    free_attach(next, FALSE);
	}
	rd_len = 0;
	on_hdr = !code;
	next = (Attach *)0;
    } /* end main while loop */
    if (count_lines)
	attach->content_length =
	    (rdonly? Stell(ss) : ftell(tmpf)) - attach->content_offset;
    return 0;
}

void
parse_header(buf, mesg, attach, want_status, today, cnt)
char *buf;
Msg *mesg;
Attach **attach;
int want_status;
long today;
int	cnt; /* The message index */
{
    int get_status = want_status;
    char *p = buf;

    /* XXXX Need to strip comments here, and then enable MIME check
     */

#if defined( IMAP )
    unsigned long uid;

    if ( sscanf( p, "X-ZmUID: %08lx", &uid ) != 0 )
        mesg->uid = uid;
    p = buf;
#endif

    if (!ci_strncmp(buf, "Date:", 5))
	ZSTRDUP(mesg->m_date_sent, parse_date(p+5, today));
    else if (mesg->m_date_sent &&
	    !ci_strncmp(buf, "Resent-Date:", 12))
	ZSTRDUP(mesg->m_date_sent, parse_date(p+12, today));
    else if (!ci_strncmp(buf, "X-Zm-Folder-Index:", 18)) {
	turnon(mesg->m_flags, INDEXMSG);
	p += 18;
	while (isspace(*p)) p++;
	if ((strlen(p) < strlen(zmVersion(0))) || 
	    strncmp(p, zmVersion(0), strlen(p)-1))
	    turnon(folder_flags, IGNORE_INDEX);
    } else if (!ci_strncmp(buf, "MIME-version:", 13)) {
	turnon(mesg->m_flags, MIME);
#if 0
	p += 13;
	while (isspace(*p)) p++;
	if (strlen(p) < strlen(MIME_VERSION) ||
	    ci_strncmp(p, MIME_VERSION, strlen(p)-1)) {
	    if (ison(glob_flags, WARNINGS))
		print("Message %d: Treating MIME message of version %s\
as MIME version %s.\n",
		      cnt+1, 
		      /* Strip the newline */
		      zmVaStr("%s", p, strlen(p)-1), 
		      MIME_VERSION);
	}
#endif
    } else if (!ci_strncmp(buf, "Message-Id:", 11)) {
	/* This isn't correct when more than one Message-Id header XXX */
	p = buf + 11;
	skipspaces(0);
	ZSTRDUP(mesg->m_id, p);
	(void) no_newln(mesg->m_id);
    } else if (!ci_strncmp(buf, "Priority:", 9)) {
	if (!want_status)
	    return;
	MsgSetPri(mesg, parse_priorities(p+9));
    } else if (!ci_strncmp(buf, "X-Zm-Priority:", 14)) {
	if (!want_status)
	    return;
	MsgSetPri(mesg, parse_priorities(p+14));
    } else if (attach && content_header(buf, attach))
	turnon(mesg->m_flags, ATTACHED);
    else if (get_status && !(get_status = ci_strncmp(p, "Status:", 7))) {
	/* new mail should not have a Status: field! */
	turnon(mesg->m_flags, OLD|STATUS_LINE);
	for (p += 7 ; *p != '\n'; p++) {
	    if (isspace(*p))
		continue;
	    /* XXX Kluge! Using attach == 0 as a flag for reading index */
	    if (attach == 0) {
		switch(*p) {
		    case 'A':
			turnon(mesg->m_flags, ATTACHED);
			continue;
		    case 'D':
			turnon(mesg->m_flags, DELETE);
			continue;
		    case 'M':
			turnon(mesg->m_flags, MIME);
			continue;
		}
	    }
	    switch(*p) {
		case 'R': turnoff(mesg->m_flags, UNREAD);
		when 'P': turnon(mesg->m_flags, UNREAD);
		when 'N': turnon(mesg->m_flags, UNREAD);
			  turnoff(mesg->m_flags, OLD);
		when 'S': turnon(mesg->m_flags, SAVED);
		when 'r': turnon(mesg->m_flags, REPLIED);
		when 'O': ; /* do nothing */
		when 'f': turnon(mesg->m_flags, RESENT);
		when 'p': turnon(mesg->m_flags, PRINTED);
#if defined( IMAP )
                when 'D':
                        if ( current_folder->uidval )
                                turnon(mesg->m_flags, DELETE);
#endif
		otherwise :
		    if (ison(glob_flags, WARNINGS))
			print("unknown msg status flag: %c\n",
				*p);
	    }
	}
    }
}

/* Load the headers of a message and return 0 on success, nonzero on error.
 * Unfold long headers.
 *   					CML Sat May 29 17:30:32 1993
 */
int
load_headers(line, cnt, ss, lines)
    char *line;	/* Returns the last line read (usually "\n") */
    struct Source *ss;	/* The Source we're loading from */
    int cnt;	/* The message number we're loading */
    int *lines;	/* We add on the number of lines read (not
		 * counting the empty line at the end) */
{
    char	buf1[BUFSIZ], buf2[BUFSIZ];
    int		had_error = 0;
    Attach      *attach = 0;
    static long	today;
#ifndef LICENSE_FREE
    extern long license_expiration;
    
    /* Check time only on folder load or update */
    if (cnt == 0)
	today = license_expiration? license_expiration : time((time_t *)0);
#endif /* LICENSE_FREE */
    turnon(msg[cnt]->m_flags, UNREAD); /* initialize */
    /* we've read the "From " line(s), now read the rest of
     * the message headers till we get to a blank line.
     * 
     * use two buffers, in order to read ahead to know whether
     * there's continuation line before we parse the header
     */
    *buf2='\0';
    while (Sgets(buf1, sizeof (buf1), ss) && (*buf1 != '\n')) {
	if (isoff(folder_flags, READ_ONLY) && fputs(buf1, tmpf) == EOF) {
	    error(SysErrWarning, "%s: write failed", tempfile);
	    had_error++;
	    break;
	}
	if ((*buf1 == ' ') || (*buf1 == '\t')) {
	    if ((strlen(buf1) + strlen(buf2) + 1) < sizeof(buf2)) {
		*buf1 = ' ';
		(void) no_newln(buf2);
		(void) strcpy(buf2+strlen(buf2), buf1);
	    } else {
#ifdef NOT_NOW
		error(ZmErrWarning, 
			    /* This just disturbs users */
		      catgets( catalog, CAT_MSGS, 501, "Ignoring end of long incoming message header (probably ok)." ));
#endif
	    }
	} else {	    
	    if (*buf2) /* won't be the first time through */
		parse_header(buf2, msg[cnt], &attach, TRUE, today, cnt);
	    strcpy(buf2, buf1);
	}
	(*lines)++;
    }
    if (*buf2)
	parse_header(buf2, msg[cnt], &attach, TRUE, today, cnt);
    if (!msg[cnt]->m_date_sent || !*msg[cnt]->m_date_sent) {
	if (!msg[cnt]->m_date_recv || !*msg[cnt]->m_date_recv) {
	    print(catgets( catalog, CAT_MSGS, 502, "Message %d has *no* date!?\n" ), cnt+1);
	    ZSTRDUP(msg[cnt]->m_date_recv, "0000000000XXXGMT");
	}
	ZSTRDUP(msg[cnt]->m_date_sent, msg[cnt]->m_date_recv);
    } else if (!msg[cnt]->m_date_recv || !*msg[cnt]->m_date_recv)
	ZSTRDUP(msg[cnt]->m_date_recv, msg[cnt]->m_date_sent);
    if (ison(msg[cnt]->m_flags, INDEXMSG) && isoff(msg[cnt]->m_flags, SAVED)) {
	/* Discard indexes using old date format, regardless of version */
	turnon(folder_flags, IGNORE_INDEX);
    }
    if (attach)
	insert_link(&msg[cnt]->m_attach, attach);
    /* if our Sgets failed, this is wrong.  Oh, well */
    (void) strcpy(line, buf1);
    return had_error;
}

int
recover_folder()
{
    struct stat statb;
    long size;

    if (fseek(tmpf, 0L, 2) == 0)
	size = ftell(tmpf);
    else
	size = msg[msg_cnt]->m_offset;	/* Best guess, wrong if indexing */
    (void) rewind(tmpf);
    wprint(catgets( catalog, CAT_MSGS, 503, "Attempting restore ... " ));
    (void) file_to_fp(tempfile, tmpf, "w", 0);

    if (stat(tempfile, &statb) < 0 || statb.st_size < size) {
	wprint(catgets( catalog, CAT_MSGS, 504, "failed\n" ));
	(void) unlink(tempfile);	/* Don't accidentally decide it's ok */
	backup_folder();
	return -1;
    } else {
	(void) fclose(tmpf);
	tmpf = fopen(tempfile, FLDR_READ_MODE);
	return 0;
    }
}

static int
load_directory(file, append, last, list)
const char *file;
int append, last;
msg_group *list;
{
    char pat[MAXPATHLEN], *p, **files, **name;
    int n, x, ro = ison(folder_flags, READ_ONLY);
    struct stat statb;

    if (append == 0)
	return -1;
    (void) sprintf(pat, "%s/{[1-9],[1-9]*[0-9]}", file);
    n = filexp(pat, &files);
    /* Bart: Mon Jul 13 18:21:32 PDT 1992
     * We have to get the mtime here for check_new_mail().  There is a
     * tiny chance that we'll miss delivery of new mail between the time
     * we get the list of files and the time we do this stat, but there
     * isn't anything we can do about it.
     */
    if (stat(file, &statb) == 0)
	current_folder->mf_last_time = statb.st_mtime;
    if (n > 0) {
	for (name = files; *name; name++) {
	    (void) strcpy(pat, *name);
	    (void) strcpy(*name, basename(pat));
	}
	p = rindex(pat, '/') + 1;
	qsort(files, n, sizeof(char *),
	      (int (*) NP((CVPTR, CVPTR))) strnumcmp);
	last = msg_cnt;
	turnoff(folder_flags, READ_ONLY);
	for (name = files; *name; name++) {
	    x = atoi(*name);
	    if (x > current_folder->mf_last_size) {
		(void) strcpy(p, *name);
		if (load_folder(pat, 3, last, list) < 0)
		    n = -1;
		else
		    current_folder->mf_last_size = x;
	    }
	}
	if (ro)
	    turnon(folder_flags, READ_ONLY);
    }
    free_vec(files);
    return n > 0;
}

/*
 * Scan a file, parse messages from it and append them to the current folder
 *
 * If "append" is 1, start where we left off (held in msg[cnt]->m_offset)
 * and scan for messages.  Append all messages found until EOF.
 *
 * If "append" is 2, we're merging in a new file, so start at the end of
 * the present folder and append all messages found until EOF.
 *
 * If "append" is 3, assume the file contains exactly one message and
 * don't bother looking for separators.
 *
 * If "append" is 0, then the message separator must exist once and
 * only once.  All extra occurrences of the separator is preceded by a '>'.
 * The list argument will be the message number to replace in the current
 * folder with the message read in from other filename.
 */
int
load_folder(file, append, last, list)
const char *file;
int append, last;
msg_group *list;
{
    char	buf[BUFSIZ];
    int		lines = 0, msg_found = 0, had_error = 1;
    int		use_ix = 0, attachment_loaded = 0, cnt;
    long	flags, bytes = 0L, ftell();
    long	osize = 0L, size = 0L;
    Msg  	old;
    char	*p, date[64];
    FILE	*fp;
    Source	*ss = 0;
    int         warn = ison(glob_flags, WARNINGS);
    int		begin_sep = 0; /* track beginning vs ending separators */
#ifdef GUI
    long	intr_chk = 0L;
#endif /* GUI */
#ifdef BETTER_BUFSIZ
    char *iobuf;
#endif

#if defined( IMAP )
    unsigned long psize, dummy;
    int first;
    char buf1[BUFSIZ], buf2[BUFSIZ];
    char dchar;
#endif

#define BROKEN_REALLOC

#ifdef BROKEN_REALLOC
    int prealloc = 1;
#define PREALLOC_SIZE 256
#endif /* BROKEN_REALLOC */

    date[0] = '\0';
    
#ifndef LICENSE_FREE
    /* XXX There really ought to be a better way ... */
    if (ls_temporary(zlogin)) {
	/* Cannot load folders on a temp license */
	switch (append) {
	    case 0:
		break;		/* Reloading after an edit is OK */
	    case 1:
		if (msg_cnt)
		    return 1;	/* Don't trash existing messages */
		else
		    return -1;	/* Treat it as a complete failure */
	    case 2:
		return 1;	/* Merging, don't trash existing messages */
	    case 3:
		if (last == 0)	/* Don't die in mid-load */
		    return -1;
	}
    }
#endif /* LICENSE_FREE */

    if (append > 0 && append < 3 && (folder_type & FolderDirectory))
	    return load_directory(file, append, last, list);

    if (!(fp = lock_fopen(file, FLDR_READ_MODE))) {
	    error(ZmErrWarning,
	        catgets(catalog, CAT_MSGS, 505, "Unable to open %s: %s"),
	            file, (errno != 0 ? strerror(errno):"<unknown>"));
	    return -1;
    }
#if defined(MAC_OS) && defined(USE_SETVBUF)
    else (void) setvbuf(fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */

    if (append) {
	cnt = msg_cnt;
#if defined( IMAP ) 
	if ( IsLocalOpen() ) {
		osize = 0L;
	}
	else
#endif
	osize = append == 1 ? msg[cnt]->m_offset : 0L;
	(void) fseek(fp, osize, L_SET);
	if (append < 3) {
	    last = msg_cnt;
#if defined( IMAP )
	    if ((using_imap && IsLocalOpen()) || append == 1 && folder_type != FolderEmpty) {
#else
	    if (append == 1 && folder_type != FolderEmpty) {
#endif
		/* change_folder() initializes */
		size = current_folder->mf_last_size;
	    } else
		goto get_size;		/* Sigh */
	}
#if defined( IMAP )
        if ( osize == 0 )  {
                if ( fscanf( fp, "UIDVAL=%08lx%s %s %c\n", &dummy, buf1, buf2, &dchar ) == 4 ) {
                        current_folder->uidval = dummy;
                        if ( current_folder->imap_path )
                                free( current_folder->imap_path );
                        current_folder->imap_path = malloc( strlen( buf1 ) + 1
 );
                        strcpy( current_folder->imap_path, buf1 );
                        if ( current_folder->imap_user )
                                free( current_folder->imap_user );
                        current_folder->imap_user = malloc( strlen( buf2 ) +
1 );
                        strcpy( current_folder->imap_user, buf2 );

			if ( current_folder->imap_prefix )
				free( current_folder->imap_prefix );
			current_folder->imap_prefix = (char *) NULL;
			p = index( current_folder->imap_path, '}' );
			if ( p ) {
				p++;
				psize = (p - current_folder->imap_path);
				current_folder->imap_prefix = 
					calloc( psize + 1, sizeof( char ) );
#if 1
				strncat( current_folder->imap_prefix, 
					current_folder->imap_path, psize );
#else
				strncpy( current_folder->imap_prefix, 
					current_folder->imap_path, psize );
#endif
			}
                }
                else
                        (void) fseek(fp, osize, L_SET);
        }
#endif
	if (cnt == 0) {
#ifndef MAC_OS
	    const char *fname = file;
	    init_nointr_msg(zmVaStr(catgets( catalog, CAT_MSGS, 506, "Loading %s" ), fname), INTR_VAL(size/3000));
	    if (ix_init(ss, cnt, (long)(-1)) == 0)
		use_ix = 2; /* 2 means an external index exists */
	    end_intr_msg(zmVaStr(catgets( catalog, CAT_MSGS, 506, "Loading %s" ), fname));
#else /* MAC_OS */
	    if (ix_init(ss, cnt, (long)(-1)) == 0)
		use_ix = 2; /* 2 means an external index exists */
#endif /* !MAC_OS */
	}
    } else {
	struct stat statb;
	FolderType fotype;
get_size:
	if ((fotype = stat_folder(file, &statb)) != FolderInvalid)
	    size = statb.st_size;
	else
	    size = 0;
	if (fotype != folder_type) {
	    if (fotype == FolderEmpty) {
		close_lock(file, fp);
		return 0;
	    } else if (folder_type == FolderEmpty)
		folder_type = fotype;
	    else {
		close_lock(file, fp);
		if (append == 0) {
		    error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 508, "File not left in correct message format." ));
		    return -1;
		} else if (append != 2) {
		    turnon(folder_flags, CORRUPTED);
		    error(ZmErrWarning,
			    catgets( catalog, CAT_MSGS, 509, "Type of folder changed while in use" ));
		    return -1;
		} else {
		    error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 510, "Cannot merge folders of different types" ));
		    return -1;
		}
	    }
	}
    }
    if (!append) {
	cnt = last;
	old = *(msg[cnt]);
    } else if (append == 1 && cnt > 0)
	size -= msg[cnt]->m_offset;	/* Loading new mail, use the diff */

    if (isoff(folder_flags, READ_ONLY)) {
	/* XXX Should try to do this only on first call when append == 3 */
	if (tmpf) {
	    if (Access(tempfile, F_OK) < 0) { /* Sanity check */
		error(ZmErrWarning, catgets( catalog, CAT_MSGS, 511, "Somebody removed %s" ), tempfile);
		if (recover_folder() < 0 && append == 1)
		    (void) rewind(fp);
	    }
	    if (tmpf)	/* It may be NULL_FILE after recovery */
		(void) fclose(tmpf);
	}
	/* mask_fopen() handles "a+" even if fopen() doesn't */
	if (!(tmpf = mask_fopen(tempfile, FLDR_APLUS_MODE))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 512, "Unable to open %s for appending" ), tempfile);
	    close_lock(file, fp);
	    return -1;
	}
	(void) fseek(tmpf, 0L, 2); /* assure we're at the end of the file */
#ifdef BETTER_BUFSIZ
       iobuf = malloc(BETTER_BUFSIZ + 8);
       if (iobuf) FSETBUF(tmpf, iobuf, BETTER_BUFSIZ);
#endif
    } else if (append == 2) {
	/* you cannot merge in a folder to a read-only folder */
	close_lock(file, fp);
	return -1;
    }
    ss = Sinit(SourceFile, fp);

#ifndef _WINDOWS
    flags = INTR_ON | INTR_MSG | INTR_LONG;
#else
    flags = INTR_ON | INTR_MSG;
#endif
    if (size > 0)
	turnon(flags, INTR_RANGE);
    if (append != 1 || msg[cnt]->m_offset > 0)
	turnon(flags, INTR_NOOP);
#ifndef UNIX
    strcpy(buf, abbrev_foldername(file));
    handle_intrpt(flags, zmVaStr(catgets( catalog, CAT_MSGS, 513, "Loading %s (%ld bytes)" ), buf, size),
	INTR_VAL(size/3000));
#else /* UNIX */
    handle_intrpt(flags, zmVaStr(catgets( catalog, CAT_MSGS, 513, "Loading %s (%ld bytes)" ), file, size),
	INTR_VAL(size/3000));
#endif /* UNIX */
    turnoff(flags, INTR_ON);

    if ((append == 0 || append == 3) && folder_type == FolderDelimited) {
	(void) strcpy(buf, msg_separator);
	goto do_headers;
    } else if (append == 3 || folder_type == FolderRFC822 && cnt == 0) {
	if (folder_type != FolderRFC822 ||
		none_p(folder_flags, READ_ONLY|TEMP_FOLDER)) {
	    /* This mock From is not ideal */
	    sprintf(buf, "From %s ", zlogin);
	    (void) rfc_date(buf + strlen(buf));
	    (void) strcat(buf, "\n");
	    if (folder_type == FolderRFC822) {
		folder_type = FolderStandard;		/* Spoofing it */
		turnon(folder_flags, DO_UPDATE);	/* Write it */
	    }
	} else
	    buf[0] = 0;
	goto do_headers;
    }
    buf[0] = 0;

#if defined( IMAP )
    first = 1;
#endif

    while (Sgets(buf, sizeof (buf), ss)) {
#if defined( IMAP )
        /* blow by the UIDVAL line if present on line 1 only */

        if ( first ) {
                first = 0;
                if ( sscanf( buf, "UIDVAL=%08lx%s %s %c\n", &dummy, buf1, buf2, &dchar ) == 4 ) {
                        current_folder->uidval = dummy;
                        if ( current_folder->imap_path )
                                free( current_folder->imap_path );
                        current_folder->imap_path = malloc(
                                strlen( buf1 ) + 1 );
                        strcpy( current_folder->imap_path, buf1 );
                        if ( current_folder->imap_user )
                                free( current_folder->imap_user );
                        current_folder->imap_user = malloc(
                                strlen( buf2 ) + 1 );
                        strcpy( current_folder->imap_user, buf2 );
			if ( current_folder->imap_prefix )
				free( current_folder->imap_prefix );
			current_folder->imap_prefix = (char *) NULL;
			p = index( current_folder->imap_path, '}' );
			if ( p ) {
				p++;
				psize = (p - current_folder->imap_path);
				current_folder->imap_prefix = 
					calloc( psize + 1, sizeof( char ) );
#if 1
				strncat( current_folder->imap_prefix, 
					current_folder->imap_path, psize );
#else
				strncpy( current_folder->imap_prefix, 
					current_folder->imap_path, psize );
#endif
			}
                        continue;
                }
        }
#endif

	turnoff(glob_flags, WARNINGS);
	bytes = (ison(folder_flags, READ_ONLY)? Stell(ss) : ftell(tmpf));
	if ((cnt == 0 || !msg[cnt-1]->m_attach ||
		isoff(msg[cnt-1]->m_attach->a_flags, AT_BYTECOUNTED) ||
		bytes - msg[cnt-1]->m_attach->content_offset >=
		    msg[cnt-1]->m_attach->content_length) &&
		(p = match_msg_sep(buf, folder_type))) {
	    /* IF there was an index load awaiting confirmation, confirm it */
	    if (p != buf)
		(void) strcpy(date, p);
	    if (!append && (msg_found || folder_type == FolderDelimited))
		(void) fputc('>', tmpf);
	    else if (folder_type != FolderDelimited || (begin_sep = !begin_sep))
do_headers:
	    {
		/* check at each new message if the user wants to interrupt.
		 * Also gives us opportunity to update feedback.
		 */
		if (isoff(folder_flags, READ_ONLY))
		    bytes = Stell(ss);
#ifdef GUI
		if (istool > 1 && intr_chk < bytes / 3000) {
		    if (handle_intrpt(flags,
		       zmVaStr(use_ix == 1?
			   catgets( catalog, CAT_MSGS, 514, "Loading message %d via index" )
		         : catgets( catalog, CAT_MSGS, 515, "Loading message %d" ), msg_cnt+1),
			    size? (int)(100*(bytes-osize)/size) : -1))
			break;
		    intr_chk = bytes / 3000;
		} else
#endif /* GUI */
		if (check_intr())
		    break;

		msg_found++;
		had_error = attachment_loaded = 0;

		if (ison(folder_flags, READ_ONLY))
		    bytes -= strlen(buf);
		else {
		    if (append < 3 /* && !read_from_program */) {
			char path[256];
			parse_from(ss, path);
			if (path[0])
			    (void) sprintf(buf, "From %s %s", path,
						date_to_ctime(date));
		    }
		    bytes = ftell(tmpf);
		}

		/* finish up message structure from previous message.
		 * if this is incorporating new mail, check "lines" to
		 * see if previous message has already been set!
		 *
		 * Bart: Fri Oct 23 20:54:04 PDT 1992
		 * Checking use_ix and FINISHED_MSG here isn't the ideal
		 * way to handle this, but what we're doing is skipping
		 * over a bogus index that appears in the middle of a
		 * folder.  In that case, we already have the size of
		 * msg[cnt-1] set correctly (hence FINISHED_MSG) and all
		 * we want to do is start over with the message after the
		 * index.  Here, use_ix == 1 means we're in this state.
		 *
		 * Folder loading needs serious revamping.
		 */
		if (cnt && lines && (use_ix != 1 ||
			isoff(msg[cnt-1]->m_flags, FINISHED_MSG))) {
		    /* Debugging check */
		    if (ison(msg[cnt-1]->m_flags, FINISHED_MSG) &&
			    msg[cnt-1]->m_size != bytes - msg[cnt-1]->m_offset)
			error(ZmErrWarning,
			    catgets( catalog, CAT_MSGS, 516, "Recomputing size of existing message!" ));
		    turnon(msg[cnt-1]->m_flags, FINISHED_MSG);
		    /* End debug */
		    msg[cnt-1]->m_size = bytes - msg[cnt-1]->m_offset;
		    msg[cnt-1]->m_lines = lines;
		    /* 
		     * XXX Hack to handle MIME messages 
		     * We really only want to do this if load_attachments
		     * returned 2.
		     */
		    if (msg[cnt-1]->m_attach &&
			    (ison(msg[cnt-1]->m_flags, MIME) ||
			     ison(msg[cnt-1]->m_attach->a_flags,
				  AT_BYTECOUNTED))) {
			/* This should always have been set and cannot be
			 * 0; that would mean there would be no headers */
			if (!msg[cnt-1]->m_attach->content_offset)
			    msg[cnt-1]->m_attach->content_offset =
				msg[cnt-1]->m_offset;
			/* XXX This will be 0 if load_attachments
			 * returned 2 or if the size of the attachment
			 * really was 0 (in which case this fails to 
			 * work properly)
			 */
			if (!msg[cnt-1]->m_attach->content_length ||
				ison(msg[cnt-1]->m_attach->a_flags,
					AT_BYTECOUNTED)) {
			    if (ci_strcmp(msg[cnt-1]->m_attach->content_type,
					  "text") == 0) {
				free_attach(msg[cnt-1]->m_attach, TRUE);
				msg[cnt-1]->m_attach = 0;
			    } else {
				msg[cnt-1]->m_attach->content_length =
				    bytes - 
					msg[cnt-1]->m_attach->content_offset;
				if (folder_type == FolderDelimited)
				    msg[cnt-1]->m_attach->content_length -=
					strlen(msg_separator);
				turnoff(msg[cnt-1]->m_attach->a_flags,
					AT_BYTECOUNTED);
			    }
			}
		    }
		}
		if (isoff(folder_flags, READ_ONLY) && fputs(buf, tmpf) == -1) {
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 327, "%s: write failed" ), tempfile);
		    had_error++;
		    break;
		}
		if (append) {
#ifdef BROKEN_REALLOC
		    Msg **oldMsg = msg;
		    if (cnt+1 >= prealloc) {
			if (cnt < 10 * PREALLOC_SIZE)
			    prealloc = prealloc < 3? cnt + 2 : cnt * 2;
			else
			    prealloc = cnt + PREALLOC_SIZE;
			msg = (Msg **)realloc(msg,
					(unsigned)(prealloc*sizeof(Msg *)));
		    }
#ifdef MAC_OS
		    if (!msg) {
		    	MacHeapCrunch(prealloc*sizeof(Msg *));
			msg = (Msg **)realloc(oldMsg,
					(unsigned)(prealloc*sizeof(Msg *)));
		    }		    	
#endif /* MAC_OS */
#else /* !BROKEN_REALLOC */
		    /* XXX  This should be done in larger chunks!  XXX */
		    msg = (Msg **)realloc(msg, (unsigned)((cnt+2)*sizeof(Msg *)));
#endif /* !BROKEN_REALLOC */
		    if (!msg) {
			error(SysErrFatal,
			    catgets( catalog, CAT_MSGS, 518, "Cannot allocate space for more than %d messages" ),
			    cnt);
		    }
		    msg[cnt+1] = malloc(sizeof(Msg));
		}
		clearMsg(msg[cnt], FALSE);
		msg[cnt]->m_offset = bytes;
		if (folder_type == FolderDelimited)
		    lines = 0;
		else {
		    lines = 1; /* count the From_ line */
		    if (warn)
			turnon(glob_flags, WARNINGS);
		    if (date[0])
			msg[cnt]->m_date_recv = savestr(date);
		}
		had_error = load_headers(buf, cnt, ss, &lines);
		if (had_error)
		    break;
		if (append && list)
		    add_msg_to_group(list, cnt);
		if (append) {
		    cnt = ++msg_cnt;
		    if (use_ix < 0)
			use_ix = 1;
		} else /* Don't recognize an index on edit */
		    turnoff(msg[cnt]->m_flags, INDEXMSG);
	    }
	} else if (!msg_found) {
	    if (use_ix > 0) {
		had_error = ix_reset(ss, cnt) != 0;
		if (had_error)
		    break;
		msg_found = !had_error;	/* We've rewound */
		use_ix = -1;
		continue;
	    } else if (buf[0] != '\n') {
		/* Allow leading blank lines, but anything else is wrong */
		lines++;
		if (cnt == last || !append) {
		    had_error++;
		    break;
		} else {
		    /* Construct a phony message header */
		    Sungets(buf, ss);
		    phony_header(cnt-1, ss);
		}
	    }
	}
	if (msg_found) {
	    lines++;
	    if (isoff(folder_flags, READ_ONLY) && fputs(buf, tmpf) == EOF) {
		error(SysErrWarning, catgets( catalog, CAT_SHELL, 327, "%s: write failed" ), tempfile);
		had_error++;
		break;
	    }
	    if (append && append < 3 && ison(msg[cnt-1]->m_flags, INDEXMSG)) {
		bytes = Stell(ss);
		cnt = --msg_cnt;	/* Omit index from tempfile */
		old = *(msg[cnt]);
		clearMsg(msg[cnt], FALSE);
		if (isoff(folder_flags, READ_ONLY|IGNORE_INDEX))
		    turnon(folder_flags, RETAIN_INDEX);
		if (append != 1 || ix_init(ss, cnt, old.m_offset) != 0) {
		    /* Index parsing error, try to recover */
		    turnoff(old.m_flags, INDEXMSG);
		    had_error = Sseek(ss, bytes, 0) != 0;
		    *(msg[cnt]) = old;
		    cnt = ++msg_cnt;
		} else {
		    msg_found = begin_sep = 0;
		    if (use_ix < 2) /* Bart: Sat Aug 15 19:05:16 PDT 1992 */
			use_ix = isoff(folder_flags, IGNORE_INDEX);
		    clearMsg(&old, TRUE);
		    /* Bart: Sun Jan  3 05:34:47 PST 1993
		     * When IGNORE_INDEX, deal properly with indexes in the
		     * middle of folders -- that is, realize we skipped it.
		     */
		    if ((cnt = msg_cnt) /* Assign and test */ && use_ix) {
			lines = msg[cnt-1]->m_lines;	/* Taken from index */
			if (isoff(folder_flags, READ_ONLY))
			    (void) fseek(tmpf, msg[cnt]->m_offset, 0);
		    } else {
			if (use_ix)
			    had_error = 1; /* The index was empty, so error */
			lines = 0;	/* unless we find another message */
		    }
		    continue;
		}
	    } else if (use_ix > 0 &&
		    (use_ix < 2 || (use_ix = !ix_switch(FALSE)))) {
		bytes = Stell(ss);
		if (ix_load_msg(ss, cnt-1) == 0) {
		    msg_found = 0;
		    lines = msg[cnt-1]->m_lines;
		    continue;
		} else {
		    use_ix = -1;
		    had_error = Sseek(ss, bytes, 0) != 0;
		}
	    }
	    if (!attachment_loaded) {
		/* XXX NOTE: Currently, this assumes that append is never
		 * false for messages with attachments -- that is, that
		 * editing of messages with attachments is not permitted.
		 * However, editing a message that has no attachments may
		 * fix content length and suddenly cause it to have them ...
		 */
		if (!had_error && msg[cnt-!!append]->m_attach) {
		    if (append == 0 && !is_plaintext(old.m_attach))
			had_error++;
		    else {
			long tpos, pos = Stell(ss);
			if (isoff(folder_flags, READ_ONLY))
			    tpos = ftell(tmpf);
			/* cnt has already been incremented when append */
			had_error =
			    load_attachments(cnt-!!append, ss, &lines, FALSE);
			if (had_error < 0) {
			    free_attachments(&(msg[cnt-!!append]->m_attach),
				FALSE);
			    errno = 0;
			    if (Sseek(ss, pos, 0) == 0) {
				turnoff(msg[cnt-!!append]->m_flags, ATTACHED);
				/* Assume there was no attachment */
				had_error = 0;
			    } else {
				print(catgets(catalog, CAT_MSGS, 520, "Unable to rewind folder: %s.\n" ),
				    strerror(errno));
				break;
			    }
			    if (isoff(folder_flags, READ_ONLY))
				(void) fseek(tmpf, tpos, 0);
			} else if (had_error == 1)
			    break;
			else
			    attachment_loaded = 1;
			if (had_error == 2) {
			    /* The message is an attachment, but of a format
			     * that must be loaded like a normal message by
			     * searching for the next separator (e.g. MIME).
			     * We've set attachment_loaded, so we won't hit
			     * this block again; just clear the error and let
			     * the loop proceed, loading as usual.
			     */
			    had_error = 0;
			}
		    }
		}
	    }
	}
	/* We're already checking at the beginning of most messages; do an
	 * additional check here in case the message is long and the user
	 * wants to interrupt.
	 */
	if (!(lines % 500) &&
	    handle_intrpt(flags,
	      zmVaStr(use_ix == 1?
		      catgets( catalog, CAT_MSGS, 514, "Loading message %d via index" )
		    : catgets( catalog, CAT_MSGS, 515, "Loading message %d" ), msg_cnt),
	      size? (int)(100*(Stell(ss)-osize)/size) : -1)) {
	    had_error++;
	    break;
	}
    } /* end of main while loop */
    if (warn)
	turnon(glob_flags, WARNINGS);
#ifndef _WINDOWS
    warn = check_intr(); /* ("warn" isn't used anymore) save if WAS_INTR */
#endif
    (void) handle_intrpt(INTR_RANGE|INTR_MSG,
			 zmVaStr((cnt - last) == 1 ?
				 catgets( catalog, CAT_MSGS, 523, "Loaded %d message." ) :
				 catgets( catalog, CAT_MSGS, 524, "Loaded %d messages." ),
				 cnt - last),
			 warn && size ? (long)(100*(Stell(ss)-osize)/size) : 100L);
    (void) handle_intrpt(INTR_OFF|INTR_RANGE|INTR_MSG, zmVaStr(NULL), 100L);
    if (warn) {
	error(ForcedMessage, catgets( catalog, CAT_MSGS, 526, "Loading interrupted." ));
	turnon(folder_flags, CORRUPTED+READ_ONLY);
	had_error = 0;
    } else if (msg_found && append != 1)
	turnon(folder_flags, DO_UPDATE);
    if (!append && !had_error &&
	    folder_type == FolderDelimited && msg_separator)
	(void) fputs(msg_separator, tmpf);
    if (had_error) {
	if (!append)
	    *(msg[cnt]) = old;
	if (!msg_found) {
	    if (!append)
		error(UserErrWarning,
			catgets( catalog, CAT_MSGS, 508, "File not left in correct message format." ));
	    else if (cnt == 0) {
		if (buf[0]) {
#ifdef HAVE_BINMAIL
		    if (pathcmp(file, spoolfile) == 0 &&
			    strncmp(buf, catgets( catalog, CAT_MSGS, 528, "Forward to" ), 10) == 0)
			print(catgets( catalog, CAT_MSGS, 529, "No messages -- mail is forwarded to%s.\n" ),
			    buf+10);
		    else
#endif /* HAVE_BINMAIL */
		    error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 530, "\"%s\" does not seem to be a folder." ), file);
		} else
		    had_error = 0;	/* empty files are OK */
	    }
	}
    } else {
	if (append)
	    cnt--;
	else
	    clearMsg(&old, TRUE);
	if (isoff(folder_flags, READ_ONLY))
	    msg[cnt]->m_size = ftell(tmpf) - msg[cnt]->m_offset;
	else
	    msg[cnt]->m_size = Stell(ss) - msg[cnt]->m_offset;
	msg[cnt]->m_lines = lines;
	/* 
	 * XXX Hack to handle MIME messages 
	 * We really only want to do this if load_attachments
	 * returned 2.
	 */
	if (msg[cnt]->m_attach &&
		(ison(msg[cnt]->m_flags, MIME) ||
		 ison(msg[cnt]->m_attach->a_flags, AT_BYTECOUNTED))) {
	    /* This should always have been set and cannot be
	     * 0; that would mean there would be no headers */
	    if (!msg[cnt]->m_attach->content_offset)
		msg[cnt]->m_attach->content_offset =
		    msg[cnt]->m_offset;
	    /* XXX This will be 0 if load_attachments
	     * returned 2 or if the size of the attachment
	     * really was 0 (in which case this fails to 
	     * work properly)
	     */
	    if (!msg[cnt]->m_attach->content_length ||
		    ison(msg[cnt]->m_attach->a_flags, AT_BYTECOUNTED)) {
		if (ci_strcmp(msg[cnt]->m_attach->content_type,
			      "text") == 0) {
		    free_attach(msg[cnt]->m_attach, TRUE);
		    msg[cnt]->m_attach = 0;
		} else {
		    msg[cnt]->m_attach->content_length =
			msg[cnt]->m_size + msg[cnt]->m_offset - 
			    msg[cnt]->m_attach->content_offset;
		    if (folder_type == FolderDelimited)
			msg[cnt]->m_attach->content_length -=
			    strlen(msg_separator);
		    turnoff(msg[cnt]->m_attach->a_flags, AT_BYTECOUNTED);
		}
	    }
	}

#ifdef DEFUNCT  /* Bart: Mon Aug 17 17:28:18 PDT 1992
		 * cnt is not used below this point; why fool with it?
		 */
	/* remember where we were to seek to for when we append new mail */ 
	if (append)
	    cnt++;
#endif /* DEFUNCT */
    }
    if (append == 1) { /* merge_folders takes care of this for append == 2 */
	(void) Sseek(ss, 0L, 2); /* Position at end of file */
	msg[msg_cnt]->m_offset = Stell(ss);
	if (use_ix)
	    ix_complete(had_error);
    }
    close_lock(file, fp);
    xfree(ss);
    if (isoff(folder_flags, READ_ONLY)) {
	if (Access(tempfile, F_OK) < 0) { /* Sanity check */
	    error(ZmErrWarning, catgets( catalog, CAT_MSGS, 511, "Somebody removed %s" ), tempfile);
	    (void) recover_folder();
	}
	if (tmpf && fclose(tmpf) == EOF) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 532, "Close of %s failed" ), tempfile);
	    had_error = -1;
	}
#ifdef BETTER_BUFSIZ
	xfree(iobuf);
#endif
	if (!(tmpf = fopen(tempfile, FLDR_READ_MODE))) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 533, "Unable to open %s for reading" ), tempfile);
	    return -1;
	}
#if defined(MAC_OS) && defined(USE_SETVBUF)
	else (void) setvbuf(tmpf, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
    }
#if defined( MOTIF )
    if ( istool )
	    refresh_folder_menu();
#endif    
    return (had_error < 0)? -1 : !had_error;
}

static void
phony_header(cnt, ss)
int cnt;
Source *ss;
{
    char	buf[256];

    /* Build the header in reverse order, pushing on stack */
    sprintf(buf, catgets( catalog, CAT_MSGS, 534, "Subject: Extra text of %d\n\n" ), cnt+1);
    Sungets(buf, ss);
    sprintf(buf, "To: %s\n", zlogin);
    Sungets(buf, ss);
    sprintf(buf, "Date: %s", date_to_ctime(msg[cnt]->m_date_sent));
    Sungets(buf, ss);
    sprintf(buf, "From: %s (Z-Mail Error Correction)\n", zlogin);
    Sungets(buf, ss);
    if (folder_type == FolderDelimited) {
	/* Assumes msg_separator includes newline */
	Sungets(msg_separator, ss);
	Sungets(msg_separator, ss);
    } else if (folder_type == FolderStandard) {
	sprintf(buf, "From %s %s", zlogin, date_to_ctime(msg[cnt]->m_date_recv));
	Sungets(buf, ss);
    }
}

/* Stuff ">" in front of "From" during fioxlate() */
long
from_stuff(line, len, output, state)
char *line, **output, *state;
long len;
{
    if (def_fldr_type == FolderStandard) {
	if (*state) {	/* Never from_stuff the first line */
	    if (match_msg_sep(line, folder_type)) {
		*output = zmVaStr(">%s", line);
		return len + 1;
	    }
	} else
	    *state += 1;
    }
    *output = line;
    return len;
}

u_long
parse_priorities(str)
const char *str;
{
    char **vec, **vp, *pristr;
    u_long pri = (u_long) 0;
    int i;

    vp = vec = strvec(str, " ,\t\n", TRUE);
    if (!vec) return (u_long) 0;
    while (pristr = *vp++) {
	if (!*pristr) continue;
	if (!pristr[1]) {
	    int c = pristr[0];
	    if (c == '+') {
		turnon(pri, PRI_MARKED_BIT);
	    } else if (isalpha(c)) {
		turnon(pri, PRI_FROM_LETTER(c));
	    }
	    continue;
	}
	for (i = 0; i != PRI_NAME_COUNT; i++)
	    if (pri_names[i] && !ci_strcmp(pristr, pri_names[i])) break;
	if (i == PRI_NAME_COUNT && PRI_NAME_COUNT != PRI_UNDEF)
	    i = PRI_UNDEF;
	turnon(pri, M_PRIORITY(i));
    }
    free_vec(vec);
    return pri;
}
