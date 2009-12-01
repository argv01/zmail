/* attach.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	attach_rcsid[] = "$Id: attach.c,v 2.267 2005/05/31 07:36:40 syd Exp $";
#endif

#include "zmail.h"
#include "attach.h"
#include "autotype.h"
#include "catalog.h"
#include "dputil.h"
#include "dppopen.h"
#include "except.h"
#include "general.h"
#include "glob.h"
#include "linklist.h"
#include "mimehead.h"
#include "pager.h"
#include "prune.h"
#include "strcase.h"
#include "zcstr.h"
#include "zmcomp.h"
#include "zmflag.h"
#include "zmopt.h"
#include "zmsource.h"

#ifdef VUI
# include "zmlite.h"
#endif /* VUI */

#if defined (MAC_OS) || defined (MSDOS)
#define USE_FILTER_ENCODERS
#endif /* MAC_OS || MSDOS */

#ifdef USE_FILTER_ENCODERS
#include "filtfunc.h"
#endif /* USE_FILTER_ENCODERS */

#ifndef MAC_OS
#include <c3/dyn_c3.h>
#else /* MAC_OS */
#include <dyn_c3.h>
#endif /* MAC_OS */

#include "fsfix.h"

#if defined(DARWIN)
#include <libgen.h>
#endif

extern struct dpipe *popen_coder();

#ifndef HAVE_STRING_H
# ifdef HAVE_PROTOTYPES
extern char *strcpy(char *to, const char *from);
extern int strcmp(const char *a, const char *b);
# else /* HAVE_PROTOTYPES */
#ifndef strcpy
extern char *strcpy();
#endif /* strcpy */
#ifndef strcmp
extern int strcmp();
#endif /* strcmp */
# endif /* HAVE_PROTOTYPES */
#endif /* HAVE_STRING_H */

static struct dpipe *detach_open_unencoded P ((Attach *, const char *));
extern char *attach_path, *dflt_encode;

/* Table used for parsing attachment headers in incoming messages */
DescribeAttach attach_table[] = {
    { HT_ulong,  "Content-Length:",            15, OffOf(content_length)     },
    /* MIME (Multipurpose Internet Mail Extensions) headers */
    { HT_string, "Content-Type:",           13, OffOf(mime_content_type_str) },
    { HT_string, "Content-Transfer-Encoding:", 26,OffOf(mime_encoding_str)   },
    { HT_encoded, "Content-Description:",      20, OffOf(content_abstract)   },
    { HT_string, "Content-Id",		       10, OffOf(mime_content_id)    },
    { HT_magic,  "Content-Disposition:",       20, OffOf(content_name)       },
    /* Z-Mail version 2.x attachment headers */
    { HT_string, "X-Zm-Content-Name:",         18, OffOf(content_name)       },
    { HT_string, "X-Zm-Content-Type:",         18, OffOf(content_type)       },
    { HT_ulong,  "X-Zm-Content-Length:",       20, OffOf(content_length)     },
    { HT_string, "X-Zm-Content-Abstract:",     22, OffOf(content_abstract)   },
    { HT_string, "X-Zm-Data-Type:",	       15, OffOf(data_type)          },
    { HT_string, "X-Zm-Encoding-Algorithm:",   24, OffOf(encoding_algorithm) },
    { HT_string, "X-Zm-Decoding-Hint:",        19, OffOf(decoding_hint)      },
    /* Temporarily understand Z-Mail beta version headers as well */
    { HT_string, "X-Zm-Target-Program:",       20, OffOf(data_type)          },
    /* Old Sun Mailtool attachment headers */
    { HT_ulong,  "Content-Lines:",             14, OffOf(content_lines)      },
    { HT_ulong,  "X-Lines:",                    8, OffOf(content_lines)      },
    { HT_string, "X-Sun-Data-Type:",           16, OffOf(data_type)          },
    { HT_string, "X-Sun-Data-Name:",           16, OffOf(content_name)       },
    { HT_string, "X-Sun-Data-Description:",    23, OffOf(content_abstract)   },
    { HT_string, "X-Sun-Data-Encoding-Info:",  25, OffOf(encoding_algorithm) },
    { HT_string, "X-Sun-Encoding-Info:",       20, OffOf(encoding_algorithm) },
    { HT_ulong,  "X-Sun-Content-Length:",      21, OffOf(content_length)     },
    { HT_ulong,  "X-Sun-Content-Lines:",       20, OffOf(content_lines)      },
    { HT_string, "X-Sun-Text-Type:",           16, -1 /* Unsupported */      },
    { HT_string, "X-Sun-Data-File:",           16, -1 /* OffOf(a_name) */    },
    { HT_string, "X-Sun-Reference-File:",      21, -1 /* Unsupported */      },
#ifdef SUPPORT_RFC1154
    /* Ignore the encoding header for now, because it is often wrong */
    /* Special handling for the RFC1154 "Encoding" header, see below */ 
    { HT_magic,  "Encoding:",                   9, OffOf(content_abstract)   },
#else /* SUPPORT_RFC1154 */
    { HT_magic,  "Encoding:",                   9, -1 /* Unsupported */      },
#endif /* SUPPORT_RFC1154 */
    { HT_end,    "",                            0,  0                        },
};

void
free_attach(tmp, clean)
Attach *tmp;
int clean;
{
    if (clean && tmp->a_name && ison(tmp->a_flags, AT_TEMPORARY)) {
	/* Unlinking like this is really dangerous.  If we don't have a
	 * full path, don't do the unlink, because we might have changed
	 * directories and we'd be unlinking the wrong file.  Better to
	 * leave a stray temp than to unlink something important ...
	 */
	if (is_fullpath(tmp->a_name))
	    (void) unlink(tmp->a_name);
    }
    xfree(tmp->a_name);
    xfree(tmp->content_type);
    xfree(tmp->content_name);
    xfree(tmp->content_abstract);
    xfree(tmp->encoding_algorithm);
    xfree(tmp->decoding_hint);
    xfree(tmp->data_type);
    xfree(tmp->mime_content_type_str);
    xfree(tmp->orig_mime_content_type_str);
    xfree(tmp->orig_content_name);
    FreeContentParameters(&tmp->content_params);
    xfree(tmp->mime_encoding_str);
    xfree(tmp->mime_content_id);
    xfree((char *)tmp);
}

void
free_attachments(a, clean)
Attach **a;
int clean;
{
    Attach *tmp;

    while (a && *a) {
	tmp = *a;
	remove_link(a, *a);
	free_attach(tmp, clean);
    }
}

DescribeAttach *
get_attach_description(buf, all)
char *buf;
int all;
{
    int i, len = strlen(buf);

    if (len == 0)
	return 0;

    for (i = 0; attach_table[i].header_type != HT_end; i++) {
	if ((all || attach_table[i].header_offset >= 0) &&
	    !ci_strncmp(buf,
		attach_table[i].header_name,
		all? len : attach_table[i].header_length)) {
	    return &attach_table[i];
	}
    }
    return 0;
}

/* 
 * Parse a content header and fill in the struct Attach.
 * If necessary, create a struct Attach.
 *
 * 				CML Fri May 28 12:40:45 1993
 */
int
content_header(buf, attach)
char *buf;
Attach **attach;
{
    DescribeAttach *at_ent;
    char *p, **s;
    u_long *u;

/* If the OffOf() macro cannot be made to work for a particular compiler, it
 * may be necessary to initialiaze the attach_table[] offsets at this point.
 */

    if (at_ent = get_attach_description(buf, FALSE)) {
	p = buf + at_ent->header_length;
	skipspaces(0);
	if (!*attach) {
	    *attach = zmNew(Attach);
	    bzero((char *) *attach, sizeof(struct Attach));
	}
	s = (char **)((char *)(*attach) + at_ent->header_offset);
	switch ((int)at_ent->header_type) {
	    case HT_magic :
		/* Currently, only one special case: Content-Disposition */
#ifdef SUPPORT_RFC1154
		if (at_ent->header_offset != OffOf(content_abstract))
#endif /* SUPPORT_RFC1154 */
		{
		    mimeContentParams tmp;
		    bzero((char *)&tmp, sizeof tmp);
		    ParseContentParameters(p, &tmp);
		    if (ci_strncmp(p, "inline", 6) == 0)
			turnon((*attach)->a_flags, AT_TRY_INLINE);
#ifdef NOT_NOW
		    else if (ci_strncmp(p, "attachment", 10) &&
			    !boolean_val(VarFirstPartPrimary))
			turnon((*attach)->a_flags, AT_NO_INLINE);
#endif /* NOT_NOW */
		    if (p = FindParam("filename", &tmp))
			ZSTRDUP(*s, p);
		    FreeContentParameters(&tmp);
		    break;
#ifdef SUPPORT_RFC1154
		}
		/* Possibly, one other special case: RFC1154 "Encoding" */
		else if (!*s) {
		    /* XXX This is less than perfect, because if the
		     * Content-Type header happens to precede the
		     * Encoding: header, we may set the content-type
		     * to RFC1154 incorrectly.  Need boolean flags.
		     */
		    ZSTRDUP((*attach)->content_type, RFC1154);
		/* Fall through */
#endif /* SUPPORT_RFC1154 */
	    case HT_string :
		    ZSTRDUP(*s, p);
		    (void) no_newln(*s);
		    *s = StripLeadingSpace(*s);
		}
	    when HT_encoded :
		ZSTRDUP(*s, decode_header(NULL, p));
		(void) no_newln(*s);
		*s = StripLeadingSpace(*s);
	    when HT_ulong :
		u = (u_long *)s;
		*u = atol(p);
	}
	return 1;
    }
    return 0;
}

/* 
 * Parse one portion of the "RFC1154 Encoding:" header, to get the info
 * about one part of the message.  Consider any part an attachment (including
 * a boundary part, for example).  Fill in the struct Attach.
 * If necessary, create a struct Attach.
 *						CML Fri May 28 12:40:57 1993
 */
char *
parse_encoding(buf, attach)
char *buf;
Attach **attach;
{
    int c = 0, n = 0;
    char *p, **vec;

    if (!*attach) {
	*attach = zmNew(Attach);
	bzero((char *) *attach, sizeof(struct Attach));
    }
    if (p = index(buf, ',')) {
	c = *p;
	*p = 0;
    } else
	p = buf + strlen(buf);
    /* XXX For completeness, strip (comment) fields here */
    vec = strvec(buf, " \t", TRUE);
    /* If not vec[0], we have an error */
    if (!vec || !vec[0])
	return NULL;
    if (((*attach)->content_lines = atoi(vec[n])) != 0 || vec[n][0] == '0')
	xfree(vec[n++]);
    if (vec[n]) {
	(*attach)->content_type = vec[n++];
	if (vec[n]) {
	    if (ci_strcmp(vec[n], "signature") != 0)
		(*attach)->encoding_algorithm = vec[n++];
	    (*attach)->content_abstract = joinv(NULL, &vec[n], " ");
	    free_elems(&vec[n]);
	}
    }
    xfree(vec);
    if (c)
	*p++ = c;
    skipspaces(0);
    return p;
}

/*
 * Your "signature" is of the type:
 *    file_or_path
 *    $variable
 *    \ literal string preceded by a backslash.
 *    [ literal string enclosed in square brackets ]
 * The variable will be expanded into its string value.
 * To sign the letter, the list of addresses is passed to this routine
 * (separated by whitespace and/or commas).  No comment fields!
 *
 * If "autosign2" is set, then it must be of the form:
 *    autosign2 = "*user user !host !some!path @dom.ain: ~/.sign2"
 *
 * The colon terminates the user/host lists from the "signature" to the right.
 *
 * Whitespace or commas separate tokens.  If everyone on the list exists in
 * the autosign2 list, the alternate signature is used. In case of syntax
 * error, the alternate signature is used without checks (e.g. if the colon
 * is missing).  The alternate signature == null is the same as not signing
 * the letter. An empty list forces signature2.
 *
 * If autosign2 is not set at all, then autosign is checked and used.
 * autosign = <signature>
 */
void
sign_letter(list, flags, fp)
register char *list; /* list of addresses -- no comment fields */
u_long flags;
FILE *fp;
{
    char buf[MAXPATHLEN], *signature;
    register char *p = NULL;
    FILE 	*pp2;
    int 	lines = 0, noisy;

    if (list) {
	while (isspace(*list))
	    list++;
    } else {
	list = "";
	turnoff(flags, DO_FORTUNE);
    }
    if (ison(flags, SIGN)) {
	noisy = !chk_option(VarQuiet, "autosign");
	if (!*list || !(p = value_of(VarAutosign2)))
	    buf[0] = 0;
	else {
	    if (!(signature = index(p, ':')))
		(void) strcpy(buf, p); /* No colon; use entire string as sig */
	    else {
		int ret_val = 0;
		*signature = 0;
		/* p now points to a list of addresses and p2 points to the
		 * signature format to use. Check that each address in the list
		 * provided (parameter) matches the "addrs" in autosign2.
		 */
		skipspaces(0);
		if (!*p)
		    /* autosign2 = " : <signature>"  send to all recipients */
		    ret_val = 1;
		else if (p = alias_to_address(p)) {
		    rm_cmts_in_addr(p);
		    ret_val = compare_addrs(list, p, NULL);
		}
		*signature++ = ':'; /* must reset first! */
		buf[0] = 0;
		if (ret_val) {
		    while (isspace(*signature))
			signature++;
		    /* Null signatures don't sign anything. */
		    if (!*strcpy(buf, signature))
			return;
		}
	    }
	}
	if (!buf[0]) {
	    if (!(p = value_of(VarAutosign)) || !*p) {
		char *home;
		if (!(home = value_of(VarHome)) || !*home)
		    home = ALTERNATE_HOME;
		(void) sprintf(buf, "%s%c%s", home, SLASH, SIGNATURE);
	    } else
		(void) strcpy(buf, p);
	    if (noisy)
		wprint(catgets( catalog, CAT_MSGS, 15, "Signing message... " ));
	} else if (noisy)
	    wprint(catgets( catalog, CAT_MSGS, 16, "Using alternate signature... " ));
	(void) fseek(fp, 0L, 2); /* guarantee position at end of file */
	(void) fputc('\n', fp);
	(void) fflush(fp);
	if (*buf == '$')
	    if (!(p = value_of(buf+1)))
		wprint(catgets( catalog, CAT_MSGS, 17, "(%s isn't set -- message not signed)\n" ), buf);
	    else {
		putstring(p+1, fp);
		if (noisy)
		    wprint("\n");
	    }
	else if (*buf == '\\') {
	    putstring(buf, fp);
	    if (noisy)
		wprint("\n");
	} else if (*buf == '[') {
	    char *rbr = index(buf, ']');
	    if (rbr)
		*rbr = 0;
	    putstring(buf + 1, fp);
	    if (noisy)
		wprint("\n");
	} else if (*buf == '|' || *buf == '!') {
#ifndef MAC_OS
	    (void) strcat(buf, " ");
	    (void) strcat(buf, list);
	    if (!(pp2 = popen(buf+1, "r")))
		error(SysErrWarning, buf+1);
	    else {
		int gotnl = 1;

		turnon(glob_flags, IGN_SIGS);
		do {	/* Reading from a pipe -- don't fail on SIGCHLD */
		    errno = 0;
		    while (fgets(buf, sizeof(buf), pp2)) {
			(void) fputs(buf, fp), lines++;
			gotnl = (buf[strlen(buf)-1] == '\n');
		    }
		} while (errno == EINTR && !feof(pp2));
		(void) pclose(pp2);
		if (gotnl == 0)
		    (void) fputc('\n', fp);
		(void) fflush(fp);
		turnoff(glob_flags, IGN_SIGS);
		if (noisy)
		  wprint(lines == 1 ? catgets( catalog, CAT_MSGS, 18, "added %d line\n" )
			            : catgets( catalog, CAT_MSGS, 22, "added %d lines\n" ),
			 lines);
	    }
#else /* MAC_OS */	    
	    wprint(catgets(catalog, CAT_MSGS, 883, "Mac Z-Mail can't use external programs to generate signatures."));
#endif /* !MAC_OS */
	} else {
	    /* precede _file_ signatures ONLY with "-- \n" */
	    (void) fputs("-- \n", fp);
	    (void) fflush(fp);
#ifndef MAC_OS
	    (void) file_to_fp(buf, fp, "r", 1);
#else /* MAC_OS */
	    if (file_to_fp(buf, fp, "r", 1) == -1 && noisy) {
	    	errno = 0;
	    	error(UserErrWarning, catgets(catalog, CAT_MSGS, 958, "Message was sent unsigned because the signature file was not found."));
	    }
#endif /* !MAC_OS */
	    if (noisy && istool) /* file_to_fp() supplies \n when !istool */
		wprint("\n");
	}
    }

    (void) fflush(stdout); /* for sys-v and older xenix */

    /* if fortune is set, check to see if fortunates is set. If so,
     * check to see if all the recipient are on the fortunates list.
     */
    if (ison(flags, DO_FORTUNE)) {
	noisy = !chk_option(VarQuiet, "fortune");
	if (p = value_of(VarFortunates)) {
	    if (!(p = alias_to_address(p)))
		return; /* no reason to hang around */
	    rm_cmts_in_addr(p);
	    if (!compare_addrs(list, p, buf)) {
		if (noisy) {
		    wprint(catgets( catalog, CAT_MSGS, 19, "\"fortunates\" does not contain \"%s\".\n" ), buf);
		    wprint(catgets( catalog, CAT_MSGS, 20, "No fortune added.\n" ));
		}
		return;
	    }
	}
	if (noisy)
	    wprint(catgets(catalog, CAT_MSGS, 840, "Adding fortune..."));
	if ((p = value_of(VarFortune)) && is_fullpath(p)) {
	    (void) strcpy(buf, p);
#ifndef MAC_OS
	} else
	    (void) sprintf(buf, "%s %s", FORTUNE, (p && *p == '-')? p: "-s");
	if (!(pp2 = popen(buf, "r")))
	    error(SysErrWarning, buf);
	else {
	    turnon(glob_flags, IGN_SIGS);
	    (void) fseek(fp, 0L, 2); /* go to end of file */
	    do {	/* Reading from a pipe -- don't fail on SIGCHLD */
		errno = 0;
		while (fgets(buf, sizeof(buf), pp2))
		    (void) fputs(buf, fp), lines++;
	    } while (errno == EINTR && !feof(pp2));
	    (void) pclose(pp2);
	    turnoff(glob_flags, IGN_SIGS);
#endif /* !MAC_OS */
	    (void) fflush(fp);
	    if (noisy)
	      wprint(lines == 1 ? catgets( catalog, CAT_MSGS, 18, "added %d line\n" )
		                : catgets( catalog, CAT_MSGS, 22, "added %d lines\n" ),
		     lines);
	}
#ifdef MAC_OS	
	else wprint (catgets(catalog, CAT_MSGS, 884, "couldn't find signature file!\n"));
#endif /* MAC_OS */
    }
    (void) fflush(stdout); /* for sys-v and older xenix */
}

/*
 * Rudimentary test to decide if a file should be sent as a binary file.
 * Grab 8kb of characters, and if any of them are beyond the range of
 * 7-bit ASCII or within NUL through BEL, assume the file to be binary.
 */
int
test_binary(path)
const char *path;
{
    FILE *fp;
    char buf[8192];
    int i, n;

    if (!(fp = fopen(path, "r")))
	return 0;
    n = fread(buf, sizeof(char), sizeof buf, fp);
    (void) fclose(fp);
    if (!n)
	return 0;
    for (i = 0; i < n; i++)
	if ((unsigned int)buf[i] < '\010' ||
	    (unsigned int)buf[i] > (unsigned int)'\176')
	    return 1;
    return 0;
}

/* We're inventing a new file to attach */
char *
new_attach_filename(compose, name)
struct Compose *compose;
char *name;
{
    int n = number_of_links(compose->attachments);

    if (!name)
	return NULL;

    do {
	(void) sprintf(name, "%s/Z%dM%s.A%d",
		    getdir(value_of(VarTmpdir), FALSE),
		    getpid(),	/* XXX DOS problems? */
		    compose->link.l_name,
		    n++);
    } while (Access(name, F_OK) == 0);

    return name;
}

#ifdef NOT_NOW
/*
 * Return the string best naming the type of a given attachment.
 * The "best" parameter tells whether the most MIME-correct name
 * should be chosen (best == 1) or whether an alias name should
 * override better choices when available (best == 0).
 *
 * This is not suitable for outgoing Content-Type headers because
 * it doesn't convert to lower case and it derefs aliases.
 */
char *
attach_get_type_name(attach, best)
Attach *attach;
int best;
{
    static char *buf;
    mimeType type = attach->mime_type;
    char *name = attach->data_type ? attach->data_type : attach->content_type; 
    char *aname = get_type_from_alias(name);

    if (type < ExtendedMimeType) {
	if (type == UnknownMimeType)
	    type = GetMimeType(best ? name : (name = aname));
	if (name != aname && type <= ExtendedMimeType) {
	    mimeType atype = GetMimeType(aname);
	    if (atype > type) {
		type = atype;
		name = aname;
	    }
	}
    }
    if (best == 0 || type == ExtendedMimeType)
	return name;
    else if (type > ExtendedMimeType)
	return MimeTypeStr(type);
    else
	ZSTRDUP(buf, zmVaStr("X-Zm-%s", name));

    return buf;
}
#endif /* NOT_NOW */

/*
 * Set the attachment's charset= content-parameter to the character set
 * in which the attachment will ultimately be sent (after C3 conversion,
 * if that's applicable).
 */
static void
attach_find_charset(attach, primaryType, bin)
Attach *attach;
mimePrimaryType primaryType;
int bin;		/* Boolean -- 8bit binary data in this attachment? */
{
    int bestCharSet;

    /*
     * fileCharSet here should eventually be replaced with charset
     * information that is truly specific to the attachment.	XXX
     *
     * Test of (bestCharSet == UsAscii) should become a test for whether
     * the character set is expected to be 7-bit only.		XXX
     */

    if (ison(attach->a_flags, AT_CHARSET_OK))
	return;

#ifdef C3
    bestCharSet = C3_TRANSLATION_REQUIRED(fileCharSet, outMimeCharSet) ?
			outMimeCharSet : fileCharSet;
#else /* !C3 */
    bestCharSet = fileCharSet;
#endif /* !C3 */

    attach->mime_char_set = NoMimeCharSet;

    if (!bin && (primaryType == MimeText) &&
	    IsAsciiSuperset(fileCharSet)) {
	attach->mime_char_set = UsAscii;
    } else if (bin && (primaryType == MimeText) &&
	    (bestCharSet == UsAscii) && (fileCharSet == bestCharSet)) {
	attach->mime_char_set = UnknownMimeCharSet;
    } else if (primaryType == MimeText) {
	attach->mime_char_set = bestCharSet;
    }
    if ((attach->mime_char_set == NoMimeCharSet) ||
        (attach->mime_char_set == UnknownMimeCharSet)) 
	DeleteContentParameter(&attach->content_params,
	    MimeTextParamStr(CharSetParam));
    else
	AddContentParameter(&attach->content_params,
	    MimeTextParamStr(CharSetParam),
	    GetMimeCharSetName(attach->mime_char_set));
}

/*
 * Prepare to attach a file to a message.  Return 0 if successful, else
 * return a pointer to an error message.  (So the GUI can display it.)
 * Return a pointer to the new attachment in address specified in the 
 * last parameter.
 */
const char *
add_attachment(compose, file, type, desc, encode, flags, newAttachPtrPtr)
struct Compose *compose;
const char *file, *type, *desc, *encode;
u_long flags;
Attach	**newAttachPtrPtr;
{
    const char *p = NULL, *cancel = catgets( catalog, CAT_SHELL, 160, "Cancelled" );
    Attach tmp, *new = 0;
    AttachKey *akey = 0;
    int n, bin = FALSE;
    int be_quiet = (istool && isoff(compose->send_flags, SEND_NOW));
    char full[MAXPATHLEN];
    mimePrimaryType	primaryType;
    char 		*autoencode = NULL;
    if (file && !*file) {
	if (be_quiet)
	    return catgets( catalog, CAT_MSGS, 24, "No file specified." );
	if (compose->attachments)
	    (void) list_attachments(compose->attachments, (Attach *)NULL,
				    TRPL_NULL, TRUE, TRUE);
	else
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 25, "There are no attachments to list." ));
	return NULL;
    }
    if (fullpath(strcpy(full, file), FALSE) == NULL)
	return NULL;

    if (interpose_on_msgop("attach", -1, full) < 0)
	return catgets( catalog, CAT_MSGS, 26, "Interposer cancelled attachment." );

    bzero((char *) &tmp, sizeof(Attach));

    if (new = (Attach *)retrieve_link(compose->attachments, full, strcmp)) {
	AskAnswer answer =
	    ask(AskYes, catgets( catalog, CAT_MSGS, 27, "%s already attached -- replace? " ), full);
	if (answer == AskCancel)
	    return cancel;
	if (answer == AskNo)
	    new = (Attach *)NULL;
    }

    while (!p)
	if ((p = full) && *p || istool < 2 &&
	    none_p(glob_flags, REDIRECT|NO_INTERACT) && !check_intr() &&
	    (p = set_header(catgets(catalog, CAT_MSGS, 28, "Attach file: "),
			    "", TRUE))) {
#ifndef MAC_OS
	    skipspaces(0);
#endif /* !MAC_OS */
	    if (!*p)
		return cancel;
	    n = (istool > 1); /* Ignore no-such-file in GUI mode */
	    if (!(p = varpath(p, &n)) || n != 0) {
		if (n < 0) {
		    if (!be_quiet)
			error(UserErrWarning, "%s\n", p);
		    if (istool)
			return p;
		} else {
		    if (!be_quiet)
			error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), p);
		    if (istool)
			return catgets( catalog, CAT_MSGS, 30, "Cannot attach directories!" );
		}
		if (*full)
		    return file; /* Anything but NULL */
		p = NULL;
	    } else {
		bin = test_binary(p);
		tmp.a_name = savestr(p);
	    }
	} else
	    return cancel;
    p = NULL;
    /* Autotyping only in GUI-mode */
    if (!type && istool>1) {
	type = autotype(tmp.a_name);
	if (!desc && (akey = get_attach_keys(FALSE, (Attach *) 0, type)))
	    desc = akey->description;
    }
    while (!p) {
	if ((p = type) && *p || istool < 2 &&
	    none_p(glob_flags, REDIRECT|NO_INTERACT) && !check_intr() && 
	    (p = set_header(catgets(catalog, CAT_MSGS, 31, "Attachment type (? for list): "),
			    autotype(tmp.a_name),
			    TRUE))) {
	    skipspaces(0);
	    if (!*p) {
		xfree(tmp.a_name);
		return cancel;
	    }
	    if (akey = get_attach_keys(FALSE, &tmp, p)) {
		if (GetMimeType(akey->key_string) < ExtendedMimeType &&
			!boolean_val("sun_attachment")) {
		    if (istool) {
			xfree(tmp.a_name);
			return catgets( catalog, CAT_MSGS, 32, "Type not supported for sending." );
		    } else
			error(UserErrWarning,
			      catgets( catalog, CAT_MSGS, 33, "Type %s not supported for sending." ),
			      akey->key_string);
		    p = type = NULL; /* Loop */
		}
		tmp.data_type = 
		    savestr(((!ci_strcmp(akey->key_string,
				      MimeTypeStr(TextPlain))) ?
			     "text":
			     (GetMimeType(akey->key_string) > ExtendedMimeType)? 
			     MimeTypeStr(GetMimeType(akey->key_string)) :
			     akey->key_string));
	    } else if (!be_quiet) {
		if (*p != '?')
		    error(UserErrWarning, catgets( catalog, CAT_MSGS, 34, "Unknown attachment type: %s." ), p);
		if (istool) {
		    xfree(tmp.a_name);
		    return catgets( catalog, CAT_MSGS, 35, "Unknown attachment type." );
		}
		p = type = NULL; /* Loop */
	    } else {
		xfree(tmp.a_name);
		return catgets( catalog, CAT_MSGS, 35, "Unknown attachment type." );
	    }
	} else {
	    xfree(tmp.a_name);
	    return cancel;
	}
    } /* end while */
    if ((p = desc) && *p || istool < 2 &&
	none_p(glob_flags, REDIRECT|NO_INTERACT) && !check_intr() &&
	(p = set_header(catgets(catalog, CAT_MSGS, 37, "Attachment description: "),
			"", TRUE))) {
	skipspaces(0);
	if (*p)
	    tmp.content_abstract = savestr(p);
    }
    if (akey && akey->use_code && *(akey->use_code) && 
	    (!encode || ci_strcmp(akey->use_code, encode) != 0) &&
	    (!encode || !*encode || !ci_strcmp(encode, "none")))
	encode =
	    (istool > 1 || any_p(glob_flags, REDIRECT|NO_INTERACT))?
		akey->use_code : (char *) NULL;

    primaryType = GetMimePrimaryType(akey->key_string);

    attach_find_charset(&tmp, primaryType, bin);

    if (bin && (!((primaryType == MimeText) && BinaryTextOk(compose))) && 
	    (!encode || !*encode) &&
	    (istool > 1 || any_p(glob_flags, REDIRECT|NO_INTERACT)))
	encode = dflt_encode;
    p = NULL;
    while (!p) {
	AttachKey *ekey;
	if ((p = encode) && *p || istool < 2 &&
	    none_p(glob_flags, REDIRECT|NO_INTERACT) && !check_intr() &&
	    (p = set_header(catgets(catalog, CAT_MSGS, 39, "Encoding (? for list): "),
			    akey && akey->use_code ? 
			    akey->use_code : 
			    (!bin ||
			     ((primaryType == MimeText) &&
				BinaryTextOk(compose))) &&
			    !ci_strcmp(tmp.data_type,"text") ? 
			    "none" : dflt_encode,  
			    TRUE))) {
	    skipspaces(0);
	    /* This is tricky code here; be wary of making changes */
	    if (bin && ((!*p || (*p && ci_strcmp(p, "none") == 0)) &&
			(!((primaryType == MimeText) &&
			    BinaryTextOk(compose))))) {
		if (primaryType == MimeText) {
		    p = encode = autoencode = MimeEncodingStr(QuotedPrintable);
		} else {
		    p = encode = autoencode = MimeEncodingStr(Base64);
		}
		continue;
	    } else if (!*p || (*p && ci_strcmp(p, "none") == 0)) {
		/* Do nothing; this is fine */;
	    } else if (*p && (ekey = get_attach_keys(TRUE, &tmp, p))) {
		if (check_attach_prog(&ekey->encode, TRUE) == 0)
		    encode = tmp.encoding_algorithm = 
			savestr(ekey->key_string);
		else {
		    if (!be_quiet)
			error(UserErrWarning,
			    ekey->encode.program,
			    ekey->key_string);
		    p = encode = NULL;
		}
	    } else if (*p) {
		tmp.mime_encoding = GetMimeEncoding(p);
		if (tmp.mime_encoding <= ExtendedMimeEncoding) {
		    if (!be_quiet) {
			if (*p != '?' || istool)
			    error(UserErrWarning, catgets( catalog, CAT_MSGS, 44, "Unknown encoding: %s." ), p);
		    }
		    p = encode = NULL;
		} else
		    encode = tmp.encoding_algorithm = savestr(p);
#ifdef GUI
		if (istool && p == NULL) {
		    xfree(tmp.a_name);
		    xfree(tmp.content_abstract);
		    return catgets( catalog, CAT_MSGS, 45, "Unknown encoding." );
		}
#endif /* GUI */
	    }
	} else if (bin &&
		(!(primaryType == MimeText && BinaryTextOk(compose)))) {
	    xfree(tmp.a_name);
	    xfree(tmp.content_abstract);
	    return catgets( catalog, CAT_MSGS, 46, "Binary attachment requires encoding" );
	} else if (check_intr()) {
	    xfree(tmp.a_name);
	    xfree(tmp.content_abstract);
	    return cancel;
	} else {
	    xfree(tmp.a_name);
	    xfree(tmp.content_abstract);
	    return catgets( catalog, CAT_MSGS, 45, "Unknown encoding." );
	}
    }
    if (autoencode && *autoencode) {
	encode = autoencode;	/* This should already be the case */
    	tmp.encoding_algorithm = savestr(autoencode);
    } else if (!encode && akey->use_code) {
	encode = akey->use_code;
    	tmp.encoding_algorithm = savestr(encode);
    }
    if (encode && (akey = get_attach_keys(TRUE, &tmp, encode)) &&
	    check_attach_prog(&akey->decode, TRUE) == 0)
	tmp.decoding_hint = savestr(akey->decode.program);
    if (new) {
	xfree(new->a_name);
	xfree(new->content_type);
	xfree(new->content_abstract);
	xfree(new->encoding_algorithm);
	xfree(new->decoding_hint);
	xfree(new->data_type);
	xfree(new->mime_content_type_str);
	FreeContentParameters(&new->content_params);
	xfree(new->mime_encoding_str);
	xfree(new->orig_mime_content_type_str);
	xfree(new->orig_content_name);
	xfree(new->mime_content_id);
    } else if (!(new = zmNew(Attach))) {
	if (!istool)
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 47, "Cannot attach file" ));
	xfree(tmp.a_name);
	xfree(tmp.content_type);
	xfree(tmp.content_abstract);
	xfree(tmp.encoding_algorithm);
	xfree(tmp.decoding_hint);
	xfree(tmp.data_type);
	xfree(tmp.mime_content_type_str);
	FreeContentParameters(&tmp.content_params);
	xfree(tmp.mime_encoding_str);
	xfree(tmp.orig_mime_content_type_str);
	xfree(tmp.orig_content_name);
	xfree(tmp.mime_content_id);
	return catgets( catalog, CAT_MSGS, 47, "Cannot attach file" );
    } else {
	bzero((char *) new, sizeof(struct Attach));
	insert_link(&compose->attachments, new);
    }

    /* For the present, all attachments are assumed to be sent with
     * type text.  That means that the encoding algorithm must produce
     * 7-bit ASCII as its output.
     */
    tmp.content_type = savestr("text");
    /* XXX Piercing the veil so we can modify in place ... */
    new->a_name = tmp.a_name;
    tmp.a_link = new->a_link;
    *new = tmp;
    turnon(new->a_flags, flags);

#ifdef GUI
    if ((istool > 1) && (istool != 3))
	gui_refresh(current_folder, NO_FLAGS);
#endif /* GUI */
    if (newAttachPtrPtr)
	*newAttachPtrPtr = new;
    return NULL;
}

int
handle_coder_err(val, program, errfile)
int val;
char *program, *errfile;
{
    if (errfile && *errfile) {
	FILE *fp;
	struct stat s_buf;

	if (stat(errfile, &s_buf) == 0 && (val || s_buf.st_size != 0) &&
		(fp = fopen(errfile, "r"))) {
	    ZmPager pager;

	    pager = ZmPagerStart(PgText);
	    ZmPagerSetTitle(pager, catgets(catalog, CAT_MSGS, 49, "Error Processing Attachment" ));
	    if (program) {
		zmVaStr(catgets( catalog, CAT_MSGS, 50, "The command\n\t%s\nexited with status %d.\n\n\
%s\n\n" ), program, val, s_buf.st_size != 0? catgets( catalog, CAT_MSGS, 51, "Output:" ) : "");
		(void) ZmPagerWrite(pager, zmVaStr(NULL));
	    }
	    (void) fioxlate(fp, -1, -1, NULL_FILE, fiopager, (char *)pager);
	    ZmPagerStop(pager);
	    (void) fclose(fp);
	} else if (val) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 52, "Cannot open error file \"%s\"" ), errfile);
	}
    }
    return val;
}

/*
 * Examine a pending attachment of a composition.
 */
void
preview_attachment(compose, part)
struct Compose *compose;
const char *part;
{
    static char *errfile = NULL;
    FILE *efp;

#if defined(MSDOS) || defined (MAC_OS)
    error(UserErrWarning, catgets(catalog, CAT_MSGS, 841, "Previewing attachments is not supported (yet)" ));
#else /* !MSDOS && !MAC_OS */
    Attach *tmp;
    int n;

    if (!compose->attachments) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 54, "There are no attachments to preview." ));
	return;
    }
    if (!part || !*part) {
	if (istool)
	    return;
	(void) list_attachments(compose->attachments, (Attach *)NULL,
				TRPL_NULL, TRUE, TRUE);
	part = set_header(catgets( catalog, CAT_MSGS, 55, "Preview part (number or name): " ), "", 1);
	if (!part || !*part)
	    return;
    }
    if ((tmp = (Attach *)retrieve_link(compose->attachments, part, strcmp)) ||
	    (n = atoi(part)) &&
	    (tmp = (Attach *)retrieve_nth_link(compose->attachments, n))) {
	AttachProg *ap;

	if (efp = open_tempfile("err", &errfile))
	    (void) fclose(efp);

	(void) popen_coder(ap = coder_prog(FALSE, tmp, NULL, NULL, "x", FALSE),
			    NULL, errfile, "x");
	(void) handle_coder_err(ap->exitStatus, ap->program, errfile);
	(void) unlink(errfile);
    } else if (n)
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 56, "Cannot find part number %s." ), part);
    else
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 57, "Cannot find part named \"%s\"." ), part);
#endif /* MSDOS || MAC_OS */
}

/*
 * Remove a pending attachment from a composition.
 */
void
del_attachment(compose, part)
struct Compose *compose;
char *part;
{
    Attach *tmp;
    int n;

    if (!compose->attachments) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 59, "There are no attachments to remove." ));
	return;
    }
    if (!part || !*part) {
	if (istool)
	    return;
	(void) list_attachments(compose->attachments, (Attach *)NULL,
				TRPL_NULL, TRUE, TRUE);
	part = set_header(catgets( catalog, CAT_MSGS, 60, "Remove part (number or name): " ), "", 1);
	if (!part || !*part)
	    return;
    }
    if ((tmp = (Attach *)retrieve_link(compose->attachments, part, strcmp)) ||
	    (n = atoi(part)) &&
	    (tmp = (Attach *)retrieve_nth_link(compose->attachments, n))) {
	remove_link(&compose->attachments, tmp);
	free_attach(tmp, TRUE);
#ifdef GUI
	if ((istool > 1) && (istool != 3))
	    gui_refresh(current_folder, NO_FLAGS);
#endif /* GUI */
    } else if (n)
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 56, "Cannot find part number %s." ), part);
    else
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 57, "Cannot find part named \"%s\"." ), part);
}

/*
 * Attach the pending attachments to the outgoing message, in MailTool format.
 * See attach_files() for details.
 */
int
sunstyle_attach_files(compose)
Compose *compose;
{
    AttachProg *saveap;
    Attach *tmp = compose->attachments;
    char *infile, buf[BUFSIZ];
#ifdef SUPPORT_RFC1154
    char *rfc1154 = NULL;
#endif /* SUPPORT_RFC1154 */
    FILE *encoded, *input;
    struct dpipe *dpinput;
    int n, had_error = 0;
    long start, offset;
    FMODE_SAVE();

    if (!compose->attachments)
	return 0;

    /* The assumptions made about the iostreams in this function
     * depend on the files being open as _O_BINARY. There is code in
     * file.c and filtfunc.c which tests the "direction" of the file
     * or pipe (this is a poor naming convention!) explicitly for the
     * character 'r', or 'w'. Because of this, we cannot substitute
     * "rb" for "direction", as we might like. As a result, instead of
     * special casing every fopen(), popen(), and dpipe open for DOS,
     * we instead change tha value of the _fmode variable. This sucks,
     * since we must restore is value when we exit this function, at
     * every of its MANY function exit points (argh!)  -ABS 12/22/93
     */

    FMODE_BINARY();

    start = compose->body_pos;
    if (fseek(compose->ed_fp, 0L, 2) < 0) {
	error(SysErrWarning,
	    catgets(catalog, CAT_MSGS, 872, "Can't seek to end of %s"),
	    compose->edfile);
	FMODE_RESTORE();
	return -1;
    }
    offset = ftell(compose->ed_fp);
    do {
	infile = NULL;
	/* Unencoded binary files may be passed off to an mta filter 
	 * for encoding (NOT!!)					XXX
	 */
	if ((!tmp->encoding_algorithm || !*tmp->encoding_algorithm ||
		ci_strcmp(tmp->encoding_algorithm, "none") == 0) &&
		test_binary(tmp->a_name))
	    ZSTRDUP(tmp->encoding_algorithm, dflt_encode);
	tmp->content_lines = 0;
	if (!tmp->encoding_algorithm || !*tmp->encoding_algorithm ||
		ci_strcmp(tmp->encoding_algorithm, "none") != 0) {
	    saveap = coder_prog(FALSE, tmp, NULL, NULL, "r", FALSE);
	    /* Bart: Fri Jan  8 17:06:06 PST 1993
	     * Don't call popen_coder() if an encoding of NULL
	     * or "None" mapped to an ENCODE of "None" as well.
	     */
	    if (saveap->checked || saveap->program)
		dpinput = popen_coder(saveap, tmp->a_name, NULL, "r");
	    else
		dpinput = 0;
	    if (!dpinput && tmp->encoding_algorithm) {
		error(SysErrWarning,
		    catgets(catalog, CAT_MSGS, 66, "Unable to execute encoding program for \"%s\""),
		    tmp->encoding_algorithm);
		had_error = 1;
		break;
	    } else if (dpinput) {
		encoded = open_tempfile(EDFILE, &infile);
		if (!encoded) {
		    error(SysErrWarning, catgets(catalog, CAT_MSGS, 67, "Unable to create tempfile"));
		    (void) pclose_coder(saveap);
		    had_error = 1;
		    break;
		}
		while (dputil_Dgets(dpinput, buf, sizeof(buf))) {
		    n = strlen(buf);
		    if (fwrite(buf, sizeof(char), n, encoded) != n ||
			    fflush(encoded) < 0) {
			error(SysErrWarning,
			    catgets(catalog, CAT_MSGS, 68, "Write failed while encoding \"%s\""),
			    tmp->a_name);
			(void) unlink(infile);
			xfree(infile);
			had_error = 1;
			break;
		    }
		    tmp->content_lines += (buf[n-1] == '\n');
		}
		(void) pclose_coder(saveap);	/* Closes "input" */
		if (had_error) {
		    (void) fclose(encoded);
		    break;
		}
		if ((tmp->content_length = ftell(encoded)) == 0) {
		    error(ZmErrWarning,
			catgets(catalog, CAT_MSGS, 876, "Zero-length output from \"%s\""),
			tmp->encoding_algorithm);
		    (void) fclose(encoded);
		    (void) unlink(infile);
		    xfree(infile);
		    had_error = 1;
		    break;
		}
		(void) rewind(input = encoded);
	    } else
		goto getfile; /* Icky, but direct */
	} else {
getfile:
	    input = fopen(tmp->a_name, "r");
	    /* Count lines for RFC1154 Encoding: header */
	    if (!input || fioxlate(input, 0, -1, NULL_FILE, xlcount,
					(char *)&tmp->content_lines) < 0) {
		error(SysErrWarning,
		    catgets(catalog, CAT_MSGS, 70, "Unable to read \"%s\""), tmp->a_name);
		if (input)
		    (void) fclose(input);
		had_error = 1;
		break;
	    }
	    tmp->content_length = ftell(input);
	    (void) rewind(input);
	}

	if (isoff(tmp->a_flags, AT_TEMPORARY))
	    tmp->content_name = savestr(basename(tmp->a_name));

	fseek(compose->ed_fp, 0L, 2);
	n = tmp->content_lines;	/* Output below at ZZZ */
	fputs("----------\n", compose->ed_fp);
#ifdef SUPPORT_RFC1154
	fprintf(compose->ed_fp, "Encoding: %lu %s%s\n", tmp->content_lines,
		(tmp->encoding_algorithm ||
		    ci_strcmp(tmp->data_type, "text"))? "X-Zm-" : "",
		tmp->encoding_algorithm?
		    tmp->encoding_algorithm : tmp->content_type);
	tmp->content_lines += 1;
#endif /* SUPPORT_RFC1154 */
	if (tmp->content_name) {
	    fprintf(compose->ed_fp, "X-Sun-Data-Name: %s\n",
		tmp->content_name);
	    tmp->content_lines += 1;
	}
	fprintf(compose->ed_fp, "X-Sun-Content-Lines: %lu\n", n); /* ZZZ */
	tmp->content_lines += 1;
	if (tmp->data_type) {
	    if (ci_strcmp(tmp->data_type, "none") != 0) {
		fprintf(compose->ed_fp,
			"X-Sun-Data-Type: %s\n", tmp->data_type);
	    } else {
		fprintf(compose->ed_fp,
		    "X-Sun-Data-Type: default\n");
	    }
	    tmp->content_lines += 1;
	}
	if (tmp->content_abstract) {
	    char **lines = strvec(tmp->content_abstract, "\r\n", TRUE);
	    if (lines) {
		char *abstract = joinv(NULL, lines, "\t\n");
		if (abstract) {
		    xfree(tmp->content_abstract);
		    tmp->content_abstract = abstract;
		}
		free_vec(lines);
	    }
	    fprintf(compose->ed_fp,
		"X-Sun-Data-Description: %s\n", tmp->content_abstract);
	    tmp->content_lines += 1;
	}
	if (tmp->encoding_algorithm &&
		ci_strcmp(tmp->encoding_algorithm, "none") != 0) {
	    fprintf(compose->ed_fp,
		    "X-Sun-Encoding-Info: %s\n", tmp->encoding_algorithm);
	    tmp->content_lines += 1;
	} else if (IsKnownMimeCharSet(tmp->mime_char_set)) {
	    fprintf(compose->ed_fp,
		    "X-Sun-Charset: %s\n",
		    GetMimeCharSetName(tmp->mime_char_set));
	    tmp->content_lines += 1;
	}
	if (tmp->decoding_hint &&
		ci_strcmp(tmp->decoding_hint, "none")) {
	    fprintf(compose->ed_fp,
		"X-Zm-Decoding-Hint: %s\n", tmp->decoding_hint);
	    tmp->content_lines += 1;
	}
	fputc('\n', compose->ed_fp);
	if (fp_to_fp(input, 0, -1, compose->ed_fp) < tmp->content_length) {
	    error(SysErrWarning, catgets(catalog, CAT_MSGS, 878, "Can't load attachment \"%s\""),
		tmp->a_name);
	    had_error = 1;
	}
	(void) fclose(input);
	if (infile) {
	    (void) unlink(infile);
	    xfree(infile);
	}
	if (had_error)
	    break;
#ifdef SUPPORT_RFC1154
	/* The Encoding: header name and the information about the first text
	 * part (what the user actually typed in) are added by add_headers().
	 */
	if (rfc1154)
	    (void) strapp(&rfc1154, ",\n\t");
	(void) strapp(&rfc1154, zmVaStr("1 TEXT BOUNDARY, %d MESSAGE",
					tmp->content_lines + 1));
#endif /* SUPPORT_RFC1154 */
    } while ((tmp = (Attach *)tmp->a_link.l_next) != compose->attachments);

    if (had_error) {
#ifdef SUPPORT_RFC1154
	xfree(rfc1154);
#endif /* SUPPORT_RFC1154 */
	truncate_edfile(compose, offset);
	FMODE_RESTORE();
	return -1;
    }
    /* The rfc1154 header won't help Z-Mail, as new Z-Mail will use the
     * MIME info, and older Z-Mail will be confused by the MIME
     * Content-Type header.  But, it might help somebody, somewhere, 
     * someday.  It could be useful in the unlikely event that some 
     * sicko gateway srips off the Content-Type header and all the X-headers.
     */
    free_attachments(&compose->attachments, TRUE);
    fflush(compose->ed_fp);
    tmp = zmNew(Attach);
    tmp->content_type = savestr("multipart");
#ifdef SUPPORT_RFC1154
    /* XXX May need to add 1 TEXT here for MIME ending boundary */
    tmp->content_abstract = rfc1154;	/* Special case */
#endif /* SUPPORT_RFC1154 */
    tmp->content_length = ftell(compose->ed_fp) - start;
    tmp->content_offset = offset - start;	/* Return pre-attached size */
    insert_link(&compose->attachments, tmp);
    fseek(compose->ed_fp, start, 0);
    FMODE_RESTORE();
    return 0;
}

static void
fput_mime_headers(attach, boundary, fp)
Attach *attach;
char *boundary;
FILE *fp;
{
    char buf[BUFSIZ];
    char *cname = 0;

    fprintf(fp, "\n--%s\n", boundary);
    if (attach->content_name && isoff(attach->a_flags, AT_NAMELESS)) {
	cname = backwhack(attach->content_name);
	if (!cname) cname = attach->content_name;
    }
    if (attach->content_abstract) {
	/* Sometimes the user enters this; sometimes it's from 
	  attach.types */
	char **lines = strvec(attach->content_abstract, "\r\n", TRUE);
	if (lines) {
	    char *abstract = joinv(NULL, lines, "\t\n");
	    if (abstract) {
		xfree(attach->content_abstract);
		attach->content_abstract = abstract;
	    }
	    free_vec(lines);
	}
	fprintf(fp,
		"Content-Description: %s\n",
		encode_header("Content-Description",
				attach->content_abstract,
				outMimeCharSet));
    }
    if (attach->mime_content_id) {
	fprintf(fp,
		"Content-Id: %s\n", attach->mime_content_id);
    }
    if (attach->data_type) {
	if (ci_strcmp(attach->data_type, "none" )) {
	    /* MIME content type */
	    if (!attach->mime_type)
		attach->mime_type = GetMimeType(attach->data_type);
	    if (attach->mime_type > ExtendedMimeType)
		fprintf(fp,
			"Content-Type: %s",
			MimeTypeStr(attach->mime_type));
	    else if (attach->mime_type == ExtendedMimeType)
		fprintf(fp,
			"Content-Type: %s",
			ci_strcpy(attach->data_type, attach->data_type));
	    else
		fprintf(fp,
			"Content-Type: X-Zm-%s", attach->data_type);
	    if (cname)
		fprintf(fp, " ; name=\"%s\"", cname);
	    PrintContentParameters(buf, &attach->content_params);
	    fprintf(fp, buf);
	    if (((attach->mime_type > ExtendedMimeType) &&
		 (MimePrimaryType(attach->mime_type) == MimeText)) ||
		((attach->mime_type == ExtendedMimeType) &&
		 (GetMimePrimaryType(attach->data_type) == MimeText))) {
		/* The charset should have already been in the content
		 * parameters; if it wasn't, print it here
		 */
		if (!FindParam(MimeTextParamStr(CharSetParam),
			       &attach->content_params))  {
		    fprintf(fp, "; %s=%s",
			    MimeTextParamStr(CharSetParam),
			    GetMimeCharSetName(
				IsKnownMimeCharSet(attach->mime_char_set) ?
					attach->mime_char_set : 
					outMimeCharSet));
		}
	    }
	    fprintf(fp, "\n");
	}
    }
    if (attach->encoding_algorithm &&
	ci_strcmp(attach->encoding_algorithm, "none") &&
	/* Special hack to handle binhex content type correctly */
	ci_strcmp(attach->data_type, ApplicationBinHexStr)) {
	if (!attach->mime_encoding)
	  attach->mime_encoding = GetMimeEncoding(attach->encoding_algorithm);
	if (attach->mime_encoding > ExtendedMimeEncoding)
	    fprintf(fp,
		    "Content-Transfer-Encoding: %s\n", 
		    MimeEncodingStr(attach->mime_encoding));
	else if (attach->mime_encoding == ExtendedMimeEncoding)
	    fprintf(fp,
		    "Content-Transfer-Encoding: %s\n", 
		    ci_strcpy(attach->encoding_algorithm,
				attach->encoding_algorithm));
	else
	    fprintf(fp,
		    "Content-Transfer-Encoding: X-Zm-%s\n", 
		    attach->encoding_algorithm);
    }
#ifdef NOT_NOW	/* The field is optional if the value is 7bit */
    else {
	fprintf(fp,
		"Content-Transfer-Encoding: %s\n", 
		MimeEncodingStr(SevenBit));
    }
#endif
    if (ison(attach->a_flags, AT_TRY_INLINE))
	fprintf(fp, "Content-Disposition: inline");
    else
	fprintf(fp, "Content-Disposition: attachment");
    if (cname)
	fprintf(fp, " ; filename=\"%s\"", cname);
    fputc('\n', fp);
    if (cname)
	fprintf(fp, "X-Zm-Content-Name: %s\n", attach->content_name);
    if (attach->decoding_hint &&
	    ci_strcmp(attach->decoding_hint, "none")) {
	fprintf(fp,
	    "X-Zm-Decoding-Hint: %s\n", attach->decoding_hint);
    }
    fputc('\n', fp);
}

static int
fput_mime_bodypart(attach, fp)
Attach *attach;
FILE *fp;
{
    AttachProg *saveap;
    struct dpipe *dpinput;
    int n, had_error = 0;
    char *buf;
    FILE *input;

    if (!attach->encoding_algorithm || !*attach->encoding_algorithm ||
	   (attach->mime_encoding != SevenBit &&
	    attach->mime_encoding != EightBit &&
	    attach->mime_encoding != Binary &&
	    ci_strcmp(attach->encoding_algorithm, "none" ) != 0)) {
	saveap = coder_prog(FALSE, attach, NULL, NULL, "r", FALSE);
	/* Bart: Fri Jan  8 17:06:06 PST 1993
	 * Don't call popen_coder() if an encoding of NULL
	 * or "None" mapped to an ENCODE of "None" as well.
	 */
	if (saveap->checked || saveap->program)
	    dpinput = popen_coder(saveap, attach->a_name, NULL, "rb");
	else
	    dpinput = 0;
	if (!dpinput && attach->encoding_algorithm) {
	    error(SysErrWarning,
		catgets( catalog, CAT_MSGS, 66, "Unable to execute encoding program for \"%s\"" ),
		attach->encoding_algorithm);
	    return 1;
	}
	if (dpinput) {
	    while (n = dpipe_Get(dpinput, &buf)) {
		if (fwrite(buf, sizeof(char), n, fp) != n ||
			fflush(fp) < 0) {
		    error(SysErrWarning,
			catgets( catalog, CAT_MSGS, 68, "Write failed while encoding \"%s\"" ),
			attach->a_name);
		    had_error = 1;
		}
		xfree(buf);
		if (had_error)
		    break;
	    }
	    (void) pclose_coder(saveap);	/* Closes "dpinput" */
	    if (had_error) {
		return 1;
	    }
	    if ((attach->content_length = ftell(fp)) == 0) {
		error(ZmErrWarning,
		    catgets( catalog, CAT_MSGS, 69, "Zero-length output from \"%s\"" ),
		    attach->encoding_algorithm);
		return 1;
	    }
	} else
	    goto getfile; /* Icky, but direct */
    } else {
getfile:
	if (!(input = fopen(attach->a_name, "r")) ||
		fp_to_fp(input, 0, -1, fp) < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 75, "Cannot load attachment \"%s\"" ),
		attach->a_name);
	    return 1;
	}
	if (input && fclose(input) != 0 && !had_error) {
	    error(SysErrWarning, catgets( catalog, CAT_MSGS, 75, "Cannot load attachment \"%s\"" ),
		attach->a_name);
	    return 1;
	}
    }

    return 0;
}

#ifdef C3
static void
attach_c3_translate(attach, in_charset, out_charset, in_place)
Attach *attach;
mimeCharSet in_charset, out_charset;
int in_place;
{
    if (!IsKnownMimeCharSet(in_charset))
	in_charset = in_place ? inMimeCharSet : fileCharSet;
    if (!IsKnownMimeCharSet(out_charset))
	out_charset = in_place ? fileCharSet : outMimeCharSet;

    /*
     * We can always safely C3-convert text/plain objects.
     *
     * If the type is NOT text/plain, but it is text/* for some *, then we
     * MIGHT be able to safely C3-convert.  We can convert safely only if
     * both the input and output character sets are ASCII supersets.  Other
     * conversions are unsafe because a text/* object may contain format
     * directives -- consider text/enriched or text/x-html -- which would
     * be damaged by conversion to or from a non-ASCII character set.
     */
    if (isoff(attach->a_flags, AT_CHARSET_OK) &&
	    (attach->mime_type == TextPlain ||
		(MimePrimaryType(attach->mime_type) == MimeText &&
			(IsAsciiSuperset(in_charset) &&
			IsAsciiSuperset(out_charset)))) &&
	    C3_TRANSLATION_REQUIRED(in_charset, out_charset)) {
	/* Convert the attachment file into the out_charset */
	char *xname = 0;
	FILE *xfp;
	FILE *afp = fopen(attach->a_name, "r");

	if (in_place) {
	    char *str, dir[MAXPATHLEN+1];
	    (void) strcpy(dir, attach->a_name);
	    str = last_dsep(dir);
	    if (str) {
		*str++ = 0;
	    } else {
		dir[0] = 0;	/* Windows */
		GetCwd(dir, MAXPATHLEN);
		str = attach->a_name;
	    }
	    /* use old extension if it is valid */
	    if (str && (str = rindex(str, '.')) && strlen(str) <= 4) {
		str++;
	    } else {
		str = "msg";
	    }
	    if (dir[0])
		xname = alloc_tempfname_dir(str, dir);
	    else
		xname = alloc_tempfname(str);
	    xfp = fopen(xname, "w+");
#if defined(MAC_OS) && defined(USE_SETVBUF)
	    if (xfp)
		(void) setvbuf(xfp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
	} else {
	    xfp = open_tempfile(EDFILE, &xname);
	}

	if (xfp) {
	    if (fp_c3_translate(afp, xfp, in_charset, out_charset) == 0) {
		fclose(afp);
		fclose(xfp);
		if (ison(attach->a_flags, AT_TEMPORARY) || in_place) {
		    unlink(attach->a_name);	/* XXX Data loss possible! */
		    if (in_place && rename(xname, attach->a_name) != 0) {
			error(SysErrWarning,
			    catgets(catalog, CAT_MSGS, 991, "Charset conversion failed: rename"));
			in_place = FALSE;
		    }
		} else if (!in_place && !attach->content_name)
		    attach->content_name = savestr(basename(attach->a_name));
		if (in_place) {
		    xfree(xname);
		    /* Not safe to do this:
		    attach->mime_char_set = out_charset;
		    turnon(attach->a_flags, AT_CHARSET_OK);
		     * because it screws up later not-in-place detaches of
		     * this attachment, e.g. from copy_attachments().
		     */
		} else {
		    xfree(attach->a_name);
		    attach->a_name = xname;
		    turnon(attach->a_flags, AT_TEMPORARY);
		}
	    } else {
		fclose(afp);
		fclose(xfp);
		unlink(xname);
		xfree(xname);
	    }
	}
    }
}
#endif /* C3 */

/*
 * Attach the pending attachments to the outgoing message, 
 * by appending them to the file being constructed with the appropriate
 * encapsulation boundaries and headers.
 *					CML Wed May 26 17:19:23 1993
 * When this function starts, the main message text (i.e. first body part)
 * is in compose->ed_fp.
 *					CML Wed Sep  1 14:27:56 1993
 *
 * The assumptions made about the iostreams in this function
 * depend on the files being open as _O_BINARY. There is code in
 * file.c and filtfunc.c which tests the "direction" of the file
 * or pipe (this is a poor naming convention!) explicitly for the
 * character 'r', or 'w'. Because of this, we cannot substitute
 * "rb" for "direction", as we might like. As a result, instead of
 * special casing every fopen(), popen(), and dpipe open for DOS,
 * we instead change the value of the _fmode variable. This sucks,
 * since we must restore is value when we exit this function, at
 * every of its MANY function exit points (argh!)  -ABS 12/22/93
 */
int
attach_files(compose)
Compose *compose;
{
    Attach *tmp = compose->attachments;
    char *buf, buf2[BUFSIZ];
    int had_error = 0;
    long start, offset;
    FMODE_SAVE();

    if (boolean_val("sun_attachment"))
	return sunstyle_attach_files(compose);

    FMODE_BINARY();

    /* Encode the first part if it's binary and binary text is not ok.
     * It would be better not to treat the first part as a special case,
     * but if we treated it like the others, its headers couldn't be put
     * before the status header.  Who knows what the side effects would be;
     * this could break other mailers, so for now, the conservative route.
     * XXX This is quite a hack.
     *					CML Wed Sep  1 14:48:00 1993
     *
     * Make the hack a little better (or a little worse, depending on your
     * point of view).  If we're already doing a multipart/mixed, go ahead
     * and encode the first part.  Otherwise, if the MTA will accept 8bit
     * data (or at least we've been told it will) don't muck with it.
     *					Bart: Thu Apr 14 16:04:15 PDT 1994
     *
     * Now we're finally doing this right.  We don't encode, here or in the
     * multipart, if the mailer supports "binary text".  If the mailer does
     * not support binary text, we encode, but we don't add any extra level
     * of multipart wrapping.		Bart: Mon Sep 18 16:00:43 PDT 1995
     */
    if (!BinaryTextOk(compose) && test_binary(compose->edfile)) {
	Attach newAttach;
	AttachKey *akey;
	char *description;
	char *encoding = NULL;
	char *xname = 0;
	FILE *xfp = open_tempfile(EDFILE, &xname);
	u_long flags = 0;

	if (akey = 
	    get_attach_keys(FALSE, (Attach *) 0, MimeTypeStr(TextPlain))) {
	    description = akey->description;
	    encoding = akey->use_code;
	} else
	    description = catgets(catalog, CAT_MSGS, 476, "plain text");
	if (!encoding || !*encoding || !ci_strcmp(encoding, "none"))
	    encoding = MimeEncodingStr(QuotedPrintable);
	if (!compose->ed_fp)
	    error(ZmErrFatal, catgets(catalog, CAT_MSGS, 842, "editor file improperly closed"));
	if (ison(compose->flags, EDIT_HDRS)) {
	    /* By the time we get here, we've already reloaded all the
	     * headers from the editor file, if any -- and now we need
	     * to dispose of them so they don't show up in the text part.
	     */
	    turnoff(compose->flags, EDIT_HDRS);
	    prepare_edfile();
	} else {
	    (void) fclose(compose->ed_fp);	/* We'll reopen it */
	}
	turnon(flags, AT_NAMELESS);
	turnon(flags, AT_TEMPORARY);
	if (xfp) {
	    bzero((char *)&newAttach, sizeof newAttach);
	    newAttach.a_name = compose->edfile;
	    newAttach.a_flags = flags;
	    newAttach.mime_type = TextPlain;
	    newAttach.data_type = MimeTypeStr(newAttach.mime_type);
	    newAttach.content_type = "text";
	    newAttach.content_abstract = description;
	    newAttach.encoding_algorithm = encoding;
	    if (fput_mime_bodypart(&newAttach, xfp) == 0) {
		if (fclose(xfp) == 0) {
		    unlink(compose->edfile);
		    xfree(compose->edfile);
		    compose->edfile = xname;
		    turnon(compose->send_flags, SEND_Q_P);
		} else {
		    xfree(xname);
		}
	    } else {
		fclose(xfp);
		xfree(xname);
	    }
	}
	compose->ed_fp = fopen(compose->edfile, "r+");
#if defined(MAC_OS) && defined(USE_SETVBUF)
	if (compose->ed_fp)
	    (void) setvbuf(compose->ed_fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
#ifdef MSDOS
	/* the compose structure 's ed_fp always has to be in text mode.
	 * if this is not done, quoted-printable encoded attachments
	 * (for example) end up with two newlines after every line.
	 */
	_setmode(fileno(compose->ed_fp), _O_TEXT);
#endif /* MSDOS */
    }
    
    if (!compose->attachments) {
	FMODE_RESTORE();
	return 0;
    }

    start = compose->body_pos;
    if (fseek(compose->ed_fp, 0L, 2) < 0) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 63, "Cannot seek to end of %s" ), compose->edfile);
	FMODE_RESTORE();
	return -1;
    }
    offset = ftell(compose->ed_fp);
    do {
	mimePrimaryType	primaryType;
#ifdef C3
	int is_binary = -1;
#define TEST_BINARY(tmp) \
	    (is_binary < 0 ? (is_binary = \
	    (!(primaryType == MimeText && BinaryTextOk(compose)) && \
	    test_binary(tmp->a_name))) : is_binary)
#else /* !C3 */
#define TEST_BINARY(tmp) \
	    (!(primaryType == MimeText && BinaryTextOk(compose)) && \
	    test_binary(tmp->a_name))
#endif /* !C3 */

	if (isoff(tmp->a_flags, AT_TEMPORARY))
	    tmp->content_name = savestr(basename(tmp->a_name));
	ValidateMimeInfo(tmp);

	primaryType = tmp->mime_type ? MimePrimaryType(tmp->mime_type) :
	    (tmp->content_type ? GetMimePrimaryType(tmp->content_type) :
	     GetMimePrimaryType(tmp->data_type));
#ifdef C3
	/* This overrides the setting established in add_attachment()
	 * so that changes in the composition's outgoing character set
	 * will apply to all attachments.  This call should be removed
	 * or modified when/if the UI supports changing the charset of
	 * individual attachments.
	 */
	attach_find_charset(tmp, primaryType, TEST_BINARY(tmp));

	/* This assumes that attach_find_charset() has set the content
	 * parameter to the character set in which we ultimately want
	 * to send the attachment (a correct assumption).  We still need
	 * code to identify the actual charset of the file if it is not
	 * the same as fileCharSet.				XXX
	 */
	attach_c3_translate(tmp, fileCharSet, tmp->mime_char_set, 0);
#endif /* C3 */

#ifdef MAC_OS
	/* Unencoded binary files may be passed off to an mta filter 
	 * for encoding
	 */
	if (tmp->encoding_algorithm &&
		ci_strcmp(tmp->encoding_algorithm, "none") != 0 &&
		ison(tmp->a_flags, AT_UUCODECOMPAT)) {
	    str_replace(&tmp->encoding_algorithm, "x-uuencode");
	    tmp->mime_encoding = ExtendedMimeEncoding;
	    if (ison(tmp->a_flags, AT_MACBINCOMPAT)) {
	    	str_replace(&tmp->data_type, "application/x-macbinary");
		tmp->mime_type = ExtendedMimeType;
	    }
	    if (ci_strcmp(tmp->data_type, ApplicationBinHexStr) == 0) {
	    	str_replace(&tmp->data_type, "application/octet-stream");
		tmp->mime_type = ExtendedMimeType;
	    }
	} else
#endif /* MAC_OS */
	if ((!tmp->encoding_algorithm || !*tmp->encoding_algorithm ||
	     tmp->mime_encoding == EightBit || tmp->mime_encoding == Binary ||
	     ci_strcmp(tmp->encoding_algorithm, "none") == 0) &&
		TEST_BINARY(tmp)) {
	    ZSTRDUP(tmp->encoding_algorithm, dflt_encode);
	    tmp->mime_encoding = GetMimeEncoding(dflt_encode);
	}
	if (!tmp->encoding_algorithm || !*tmp->encoding_algorithm) {
	    ZSTRDUP(tmp->encoding_algorithm, "none");
	    tmp->mime_encoding = SevenBit;
	}
	tmp->content_lines = 0;

	fseek(compose->ed_fp, 0L, 2);
	if (!compose->boundary)
	  compose->boundary = message_boundary();
	fput_mime_headers(tmp, compose->boundary, compose->ed_fp);
	had_error = fput_mime_bodypart(tmp, compose->ed_fp);

	if (had_error)
	    break;
    } while ((tmp = (Attach *)tmp->a_link.l_next) != compose->attachments);

    if (had_error) {
	truncate_edfile(compose, offset);
	FMODE_RESTORE();
	return -1;
    }
    /* Add an empty line afterwards so that if this message is
     * blindly encapsulated in another, there will be at least
     * one CRLF after the encapsulation boundary and one CRLF 
     * after the body part, as strictly required by MIME
     */
    fprintf(compose->ed_fp, "\n--%s--\n\n", compose->boundary);
    free_attachments(&compose->attachments, TRUE);
    fflush(compose->ed_fp);
    tmp = zmNew(Attach);
    bzero((char *) tmp, sizeof(struct Attach));
    tmp->content_type = savestr("multipart");
    tmp->content_length = ftell(compose->ed_fp) - start;
    tmp->content_offset = offset - start;	/* Return pre-attached size */
    insert_link(&compose->attachments, tmp);
    fseek(compose->ed_fp, start, 0);
    FMODE_RESTORE();
    return 0;
}

#define ATTACH_TBL_HDR \
	catgets( catalog, CAT_MSGS, 76, "Part" ), \
	catgets( catalog, CAT_MSGS, 77, "Name" ), \
	catgets( catalog, CAT_MSGS, 78, "Type Key" ), \
	catgets( catalog, CAT_MSGS, 79, "Encoding" ), \
	catgets( catalog, CAT_MSGS, 80, "Size" ), \
	catgets( catalog, CAT_MSGS, 81, "Description" )
#define ATTACH_TBL_FMT \
	catgets(catalog, CAT_MSGS, 825, "%-4.4s  %-14.14s  %-12.12s  %-12.12s  %-10.10s  %-16.16s\n")

/*
 * attach_head is the head of the list of attachments and must always be passed
 * in.  attach is the specific part we want listed, or NULL to list all parts
 */
int
list_attachments(attach_head, attach, vecp, all, pending)
Attach *attach_head, *attach;
char ***vecp;
int all, pending;
{
    int i, j, n = 0;
    Attach *a;
    char *p;

    if (!attach_head)
	return 0;
    if (!vecp)
	wprint(ATTACH_TBL_FMT, ATTACH_TBL_HDR);
    i = j = is_multipart(attach_head);

    if (attach == attach_head)
	a = attach;
    else
	a = (i++) ? link_next(Attach, a_link, attach_head) : attach_head;
    do {
	char buf[32];

	if (!attach || (attach == a)) {
	    if (pending)
		(void) strcpy(buf, catgets( catalog, CAT_MSGS, 83, "(pending)" ));
	    else
		(void) sprintf(buf, "%lu", a->content_length);
	    p = zmVaStr(ATTACH_TBL_FMT, itoa(i-j),
			a->content_name? basename(a->content_name) :
			(a->a_name? basename(a->a_name) : catgets( catalog, CAT_MSGS, 84, "(no name)" )),
			a->data_type? a->data_type : catgets( catalog, CAT_MSGS, 85, "(unknown)" ),
			a->encoding_algorithm? a->encoding_algorithm : catgets( catalog, CAT_SHELL, 822, "(none)" ),
			buf,
			a->content_abstract? a->content_abstract : catgets( catalog, CAT_MSGS, 87, "(no description)" ));
	    if (vecp) {
		if ((n = catv(n, vecp, 1, unitv(p))) < 0) {
		    free_vec(*vecp);
		    return -1;
		}
	    } else {
		wprint("%s", p); /* Should page this eventually */
		n++;
	    }
	    if (attach)
		break;
	}
	++i;
	a = link_next(Attach, a_link, a);
    } while (a != attach_head);

    return n;
}

/* Return NULL on success, pointer to error message on failure */
char *
check_attach_prog(program, is_coder)
AttachProg *program;
int is_coder;
{
    char *p, prog[MAXPATHLEN];

    if (program->checked < 0)
	return program->program;
    else if (program->checked > 0)
	return NULL;

    if (!(p = program->program)) {
	program->checked = -1;
	return (program->program =
		    savestr(catgets( catalog, CAT_MSGS, 88, "No program provided for \"%s\"." )));
    }
    skipspaces(0);
    program->pipein = (*p == '|');
    program->zmcmd = (*p == ':');
    skipspaces(program->pipein || program->zmcmd);
    (void) strcpy(prog, p);
    p = no_newln(prog); /* Set p to end of prog, strip spaces */
    if (program->pipeout = (*p == '|'))
	*p = 0;
    for (p = prog; (p = index(p, '%')) && p[1] != 's'; p++)
	;
    program->givename = !!p;
    p = NULL;
    ZSTRDUP(program->program, prog);
    if (program->givename &&
	    program->pipein && (program->pipeout || !is_coder)) {
	p = ZSTRDUP(program->program,
		catgets( catalog, CAT_MSGS, 89, "Ambiguous redirection in program for \"%s\"." ));
	program->checked = -1;
    } else if (is_coder) {
	if (!program->givename &&
		(!program->pipein != !program->pipeout || /* ! != ! is XOR */
		!program->pipein && !program->pipeout)) {
	    p = ZSTRDUP(program->program,
		    catgets( catalog, CAT_MSGS, 90, "Cannot read from program for \"%s\"." ));
	    program->checked = -1;
	} else if (program->zmcmd) {
	    p = ZSTRDUP(program->program,
		catgets( catalog, CAT_MSGS, 91, "Cannot use Z-Mail commands in program for \"%s\"." ));
	    program->checked = -1;
	/* Bart: Tue Aug 18 11:02:50 PDT 1992
	 * The following used to be is_coder && ...
	 * Hope it wasn't a typo for !is_coder && ...
	 * It was originally outside this surrounding "if" block.
	 */
	} else if (!program->givename && !program->pipein) {
	    p = ZSTRDUP(program->program,
		    catgets( catalog, CAT_MSGS, 92, "Undefined file access in program for \"%s\"." ));
	    program->checked = -1;
	} else
	    program->checked = 1;
    } else if (program->pipeout) {
	p = ZSTRDUP(program->program,
		catgets( catalog, CAT_MSGS, 93, "Undefined output in program for \"%s\"." ));
	program->checked = -1;
    } else
	program->checked = 1;
    return p;
}

/*
 * Figure out the shell command needed to do encoding/decoding/viewing
 * and return a pointer to a static AttachProg containing the necessary
 * information.  On error, set the "checked" field to -1, the "zmcmd"
 * field to the PromptReason of the error, and the "program" field to
 * the error message.  Always return a valid pointer.
 *
 * If creating is TRUE and direction is not "x", then code_keys are looked
 * up, else type_keys.  If creating is FALSE but direction is "r", then any
 * automatic encoding specified for the given type is looked up and used.
 *
 * If keyword is NULL, the one found in the attachment's encoding_algorithm
 * or data_type (as appropriate) is used.
 *
 * If the "pipeline" parameter is TRUE, attempt to create a program string
 * whose other end (i.e., the end not connected to Z-Mail, as indicated by
 * the "direction") can be connected to another program.  However, do not
 * introduce additional processing to accomplish this, i.e. if the output
 * of the defined program is a file, piping out is assumed to be impossible.
 * The "pipein" and "pipeout" fields indicate what pipelining is possible.
 *
 * In all non-error cases, "program" is the string to be executed.  If the
 * string can be passed directly to popen() with the same "direction" as
 * was indicated, then the "checked" field is 1.  If the string can be
 * executed via system() [or cmd_line() if "zmcmd"] either after writing
 * (if direction is "w") or before reading (if direction is "r") the "file",
 * then "checked" is 0.  If no program is needed (e.g. the file should be
 * opened directly) then "checked" is 0 and "program" is NULL.
 *
 * The "givename" field is -1 if the file will be destructively overwritten
 * by executing the program.  (This is not considered to be the case if the
 * file that will be overwritten is a temp file created by this function.)
 * The "givename" field is 0 if it is not possible to tell whether the file
 * will be overwritten, and 1 if the file is believed to be safe.
 *
 * NOTE: If "checked" is returned as 0 and a temp file will be needed to
 * do the write or read before calling system(), then the temp file is
 * created (but left empty) and the "file" parameter is -overwritten- with
 * the name of that temp file!
 */
AttachProg *
coder_prog(creating, attach, keyword, file, direction, pipeline)
int creating, pipeline;
Attach *attach;
const char *keyword;
char *file;
const char *direction;
{
    static AttachProg aprog;
    static char prog[MAXPATHLEN * 4];
    AttachKey *akey;
    char buf[MAXPATHLEN * 4], buf2[MAXPATHLEN * 4], *p;
    int need_tempfile = FALSE, tempfile_ok = TRUE;
    int want_encoding = (creating && *direction != 'x');

    aprog.checked = aprog.givename = -1; /* Default to error state */

    if (*direction == 'x')
	pipeline = FALSE;	/* Ignore pipelines for execute-only */

    if (!keyword)
	if (want_encoding)
	    keyword = attach->encoding_algorithm;
	else
	    keyword = attach_data_type(attach);
    if (!file) {
	file = attach->a_name? attach->a_name : attach->content_name;
	tempfile_ok = FALSE;	/* Not safe to use tempfile */
    }
    if (!(akey = get_attach_keys(want_encoding, attach, keyword))) {
	if (*direction == 'x') {
	    aprog.zmcmd = (int)UserErrWarning;
	    aprog.program = strcpy(prog, zmVaStr(catgets( catalog, CAT_MSGS, 135, "Unrecognized %s: %s." ),
		creating? catgets( catalog, CAT_MSGS, 105, "encoding" ) : catgets( catalog, CAT_MSGS, 137, "file type" ), keyword));
	} else {
	    aprog.program = NULL;
	    aprog.checked = 0;
	}
	return &aprog;
    }
    if (*direction == 'r' && !creating) {
	if (!(keyword = attach->encoding_algorithm))
	    keyword = akey->use_code;
	if (!(akey = get_attach_keys(TRUE, attach, keyword))) {
	    aprog.program = NULL;
	    aprog.checked = 0;
	    return &aprog;
	} else if (check_attach_prog(&akey->encode, creating = TRUE) == 0 &&
		keyword != attach->encoding_algorithm)
	    ZSTRDUP(attach->encoding_algorithm, keyword);
    }

    if (*direction == 'r' || creating && *direction == 'x') {
	if (p = check_attach_prog(&akey->encode, want_encoding)) {
	    aprog.zmcmd = (int)ZmErrWarning;
	    aprog.program = strcpy(prog, zmVaStr(p, keyword));
	    return &aprog;
	}
	if (akey->encode.zmcmd && *direction == 'r') {
	    /* Never read from a zmcmd */
	    aprog.zmcmd = (int)UserErrWarning;
	    aprog.program = strcpy(prog,
		zmVaStr(catgets( catalog, CAT_MSGS, 138, "Cannot read from command: %s" ), akey->encode.program));
	    return &aprog;
	}
	aprog = akey->encode;
    } else {
	if (p = check_attach_prog(&akey->decode, want_encoding)) {
	    aprog.zmcmd = (int)ZmErrWarning;
	    aprog.program = strcpy(prog, zmVaStr(p, keyword));
	    return &aprog;
	}
	aprog = akey->decode;
    }
    aprog.checked = -1;		/* We still aren't sure we're ok */
    aprog.program = strcpy(prog, aprog.program);
    /* Bart: Tue Aug 18 11:04:26 PDT 1992
     * Can't run the program if it is "none" ...
     */
    if (ci_strcmp(prog, "none") == 0) {
	aprog.zmcmd = (int)UserErrWarning;
	aprog.program = strcpy(prog,
	    zmVaStr(catgets( catalog, CAT_MSGS, 88, "No program provided for \"%s\"." ), keyword));
	return &aprog;
    }
    if (aprog.givename && (!file || !*file)) {
	aprog.zmcmd = (int)ZmErrWarning;
	aprog.program = strcpy(prog,
	    zmVaStr(catgets( catalog, CAT_MSGS, 141, "File name required in program for \"%s\"." ), keyword));
	return &aprog;
    }
#ifdef USE_FILTER_ENCODERS
#ifdef MAC_OS
    if (ison(attach->a_flags, AT_UUCODECOMPAT)) {
	if (ison(attach->a_flags, AT_MACBINCOMPAT))
	    aprog.program = strcpy(prog, "uuenmacbin");
	else aprog.program = strcpy(prog, "uuenpipe");
    }
#endif /* MAC_OS */
    (void) no_newln(aprog.program);
    want_encoding = (creating && *direction != 'x');
    if (want_encoding && !LookupFilterFunc(aprog.program) ||
    	    !want_encoding && !aprog.zmcmd) {
	aprog.program = want_encoding?
		strcpy(prog, zmVaStr(catgets(catalog, CAT_MSGS, 950, "No internal encoder: %s"), aprog.program)):
		strcpy(prog, zmVaStr(catgets(catalog, CAT_MSGS, 951, "No internal viewer: %s"), akey->decode.program));
	aprog.checked = -1;
	aprog.zmcmd = (int)UserErrWarning;
	return &aprog;
    } else aprog.checked = want_encoding;
#endif /* USE_FILTER_ENCODERS */
    p = NULL;
    /* Now see if we need to use temporary files */
    if (creating && aprog.givename) {
	if (*direction == 'r')
	    need_tempfile = (aprog.pipein && !pipeline || !aprog.pipeout);
	else if (*direction == 'w')
	    need_tempfile = (!aprog.pipein || aprog.pipeout && !pipeline);
	if (need_tempfile && !tempfile_ok) {
	    aprog.zmcmd = (int)ZmErrWarning;
	    aprog.program = catgets( catalog, CAT_MSGS, 142, "Cannot create tempfile" );
	    return &aprog;
	}
	if (need_tempfile) {
	    FILE *pp;
	    if (!(pp = open_tempfile(EDFILE, &p))) {
		aprog.zmcmd = (int)SysErrWarning;
		aprog.program = catgets( catalog, CAT_MSGS, 142, "Cannot create tempfile" );
		return &aprog;
	    }
	    (void) fclose(pp);
	}
    }
    if (attach) {
	InsertContentParameters(buf, aprog.program, attach);
	strcpy(prog, buf);
    }

    if (aprog.givename) {
#ifndef USE_FILTER_ENCODERS
	if (creating && !aprog.zmcmd) {
	    if (*direction == 'r') {
		if (need_tempfile) {
		    /* This code assumes p points to a temp file name */
		    (void) sprintf(buf, prog, p);
		    if (aprog.pipein) {
			(void) sprintf(prog, "(%s) < %s", buf,
					quotesh(file, 0, FALSE));
			aprog.pipein = FALSE;
			aprog.givename = 1;	/* Overwrite is unlikely */
		    }  else {
			aprog.zmcmd = (int)ZmErrWarning;
			aprog.program = strcpy(prog,
					       zmVaStr(catgets( catalog, CAT_MSGS, 147, "Cannot do I/O with program for \"%s\"." ),
						       keyword));
			xfree(p);
			return &aprog;
		    }
		    (void) strcpy(file, p);	/* Return the temp name */
		    aprog.checked = 0;
		} else {
		    /* Bart: Sat Apr  3 17:04:01 PST 1993
		     * Would be nice to use quotesh(file) here, but we don't
		     * know what the user may have done to quote the %s in
		     * the attach.types definition.  
		     * Carlyn: Fri Aug 13 02:46:23 1993
		     * Using strcpyStrip means that filenames
		     * with shell metacharacters in them will be altered.
		     */
		    if (strcpyStrip(buf2, file, TRUE))
			error(UserErrWarning,
			      catgets(catalog, CAT_MSGS, 829, "Shell metacharacters included in attachment filename %s.\nUsing: %s.\n"),
			      file, buf2);
		    (void) sprintf(buf, prog, buf2);
		    (void) strcpy(prog, buf);
		    aprog.checked = (!aprog.pipein == !pipeline);
		    aprog.givename = 0;		/* Cannot determine safety */
		}
	    } else {
		if (need_tempfile) {
		    /* This code assumes p points to a temp file name */
		    (void) sprintf(buf, prog, p);
		    (void) sprintf(prog, "(%s) > %s", buf,
				   quotesh(file, 0, FALSE));
		    (void) strcpy(file, p);	/* Return the temp name */
		    aprog.pipeout = FALSE;
		    aprog.givename = -1;	/* Guaranteed destruction */
		    aprog.checked = 0;
		} else {
		    /* Bart: Sat Apr  3 17:04:01 PST 1993
		     * Would be nice to use quotesh(file) here, but we don't
		     * know what the user may have done to quote the %s in
		     * the attach.types definition. 
		     * Carlyn: Fri Aug 13 02:46:23 1993
		     * Using strcpyStrip means that filenames
		     * with shell metacharacters in them will be altered.
		     */
		    if (strcpyStrip(buf2, file, TRUE))
			error(UserErrWarning,
			      catgets(catalog, CAT_MSGS, 829, "Shell metacharacters included in attachment filename %s.\nUsing: %s.\n"),
			      file, buf2);
		    (void) sprintf(buf, prog, buf2);
		    (void) strcpy(prog, buf);
		    aprog.givename = 0;		/* Can't determine safety */
		    aprog.checked = (!aprog.pipeout == !pipeline);
		}
	    }
	}
	else
#endif /* !USE_FILTER_ENCODERS */
	{   /* Bart: Sat Apr  3 17:04:01 PST 1993
	     * Would be nice to use quotesh(file) here, but we don't
	     * know what the user may have done to quote the %s in
	     * the attach.types definition.  
	     * Carlyn: Fri Aug 13 02:46:23 1993
	     * Using strcpyStrip means that filenames
	     * with shell metacharacters in them will be altered.
	     */
	    if (strcpyStrip(buf2, file, TRUE))
		error(UserErrWarning,
		      catgets(catalog, CAT_MSGS, 829, "Shell metacharacters included in attachment filename %s.\nUsing: %s.\n"),
		      file, buf2);
	    (void) sprintf(buf, prog, buf2);
	    (void) strcpy(prog, buf);
	    aprog.givename = 0;		/* Can't determine safety */
	    aprog.checked = !aprog.zmcmd;
	}
    } 
#ifndef USE_FILTER_ENCODERS
    else if (!aprog.zmcmd) {
	(void) strcpy(buf, prog);
	/* XXX I'm not sure this is right -- why never check aprog.pipein
	 *     unless not creating?
	 */
	if (creating) {
	    if (*direction == 'r') {
		if (pipeline) /* Ignore the file name */
		    (void) sprintf(prog, "(%s)", buf);
		else {
		    (void) sprintf(prog, "(%s) < %s", buf,
				    quotesh(file, 0, FALSE));
		    aprog.pipein = FALSE;
		}
		aprog.givename = 1;	/* Overwrite is unlikely */
	    } else {
		if (pipeline) {	/* Ignore the file name */
		    (void) sprintf(prog, "(%s)", buf);
		    aprog.givename = 1;		/* Overwrite is unlikely */
		} else {
		    (void) sprintf(prog, "(%s) > %s", buf,
				    quotesh(file, 0, FALSE));
		    aprog.pipeout = FALSE;
		    aprog.givename = -1;	/* Guaranteed destruction */
		}
	    }
	} else if (aprog.pipein) {
	    (void) sprintf(prog, "(%s) < %s", buf, quotesh(file, 0, FALSE));
	    aprog.givename = 1;		/* Overwrite is unlikely */
	} else {
	    (void) sprintf(prog, "exec %s", buf);
	    aprog.givename = 0;		/* Can't determine safety */
	}
	aprog.checked = 1;
    }
#endif /* !USE_FILTER_ENCODERS */
    if (aprog.checked > 0 && *direction == 'x')
	aprog.checked = 0;	/* Never popen() on direction "x" */
    xfree(p);
    Debug("coder_prog returning %s.\n", aprog.program);
    return &aprog;
}

int
internal_coder(program)
char *program;
{
    msg_folder *save_folder = current_folder;
    int ok = cmd_line(program, NULL_GRP);
    if (current_folder != save_folder)
	wprint(catgets( catalog, CAT_MSGS, 150, "Returning to folder %s\n" ), save_folder->mf_name);
#ifdef GUI
    if (istool) {
	if (ok > -1 || save_folder != current_folder) {
	    gui_refresh(current_folder = save_folder, REDRAW_SUMMARIES);
# ifdef VUI
	    spSend(Dialog(&MainDialog), m_dialog_setfolder, current_folder);
# endif /* VUI */
	} else {
	    clear_msg_group(&current_folder->mf_group);
	}
    } else
#endif /* GUI */
	current_folder = save_folder;
    return ok;
}

int
pclose_coder(aprog)
const AttachProg *aprog;
{
    int ret = 0;

#ifdef USE_FILTER_ENCODERS
     TRY {
	ret = dpipeline_filtfunc_close(aprog->stream);
    } EXCEPT(except_ANY) {
	ret = -1;
    } ENDTRY;
   return ret;
#else /* !USE_FILTER_ENCODERS */

    if (index(aprog->program, '&')) { /* Bart: Sun Sep  6 15:51:07 PDT 1992 */
	/* We take a guess that the program was run in the background.
	 * In that case, wait a couple of seconds in case the child is
	 * going to die with "command not found" or some such, so that
	 * the error message has a chance to be written into the file
	 * where we will see it and display it.
	 */
	(void) sleep(2);
    }

    TRY {
	if (aprog->processp)
	    ret = ((dputil_pclose(aprog->stream) >> 8) & 0xff);
	else
	    dputil_fclose(aprog->stream);
    } EXCEPT(except_ANY) {
	ret = -1;		/* Ugh */
    } ENDTRY;

    return ret;
#endif /* !USE_FILTER_ENCODERS */
}

struct dpipe*
popen_coder(aprog, file, errFile, direction)
AttachProg *aprog;
const char *file, *errFile, *direction;
{
    static char *cmdbuf;
    static int allocated = 0;
    char *newprog, *p, *nenv[4];
    int len;

    if (aprog->checked < 0) {	/* error occurred in coder_prog */
	error(aprog->zmcmd, "%s", aprog->program);
	return 0;
    }
    if (!aprog->program) {
	aprog->processp = 0;
	if (!file) {
	    error(ZmErrWarning,
		catgets( catalog, CAT_MSGS, 151, "popen_coder: Neither file nor program given!" ));
	    return 0;
	}
	return (aprog->stream = dputil_fopen(file, direction));
    }
    if (aprog->zmcmd) {
	aprog->exitStatus = internal_coder(aprog->program);
	return 0;
    }

#ifndef USE_FILTER_ENCODERS
    aprog->processp = 1;
    if (!errFile)
	errFile = "";
    len = (strlen(aprog->program) + (file ? strlen(file) : 0) +
	   strlen(errFile) + 100);
    if (len > allocated) {
	if (allocated) {
	    if (!(cmdbuf = (char *) realloc(cmdbuf, len)))
		return 0;
	}
	else {
	    if (!(cmdbuf = (char *) malloc(len)))
		return 0;
	}
	allocated = len;
    }
    switch (*direction) {
      case 'r':
	if (!(aprog->checked))	/* program won't write stdout */
	    sprintf(cmdbuf, "( %s ; %s < %s ) %s %s",
		    aprog->program, BIN_CAT, quotesh(file, 0, FALSE),
		    *errFile ? "2>" : "", errFile);
	else
	    sprintf(cmdbuf, "( %s ) %s %s", aprog->program,
		    *errFile ? "2>" : "", errFile);
	break;
      case 'w':
	if (!(aprog->checked))	/* program won't read stdin */
	    sprintf(cmdbuf, "( %s > %s ; %s ) %s %s",
		    BIN_CAT, quotesh(file, 0, FALSE), aprog->program,
		    *errFile ? "2>" : "", errFile);
	else
	    sprintf(cmdbuf, "( %s ) %s %s", aprog->program,
		    *errFile ? "2>" : "", errFile);
	break;
      case 'x':
	sprintf(cmdbuf, "( %s ) %s %s", aprog->program,
		*errFile ? "2>" : "", errFile);
	break;
    }

#ifdef NOSETENV
    p = "PATH=%s; export PATH; %s";
    n = strlen(cmdbuf) + strlen(attach_path) + strlen(p);
    if (newprog = malloc(n))
	(void) sprintf(newprog, p, attach_path, cmdbuf);
    else {
	error(SysErrWarning, cmdbuf);
	return 0;
    }

#else /* NOSETENV */

    nenv[0] = NULL;
    nenv[1] = "PATH";
    nenv[2] = attach_path;
    nenv[3] = NULL;

    p = savestr(getenv("PATH"));
    (void) Setenv(3, nenv);
    newprog = cmdbuf;
#endif /* NOSETENV */

    on_intr();

    switch (*direction) {
      case 'r':
      case 'w':
	aprog->stream = dputil_popen(newprog, direction);
	break;
      case 'x':
	aprog->exitStatus = system(newprog);
	/* We take a guess that the program was run in the background.
	 * In that case, wait a couple of seconds in case the child is
	 * going to die with "command not found" or some such, so that
	 * the error message has a chance to be written into the file
	 * where we will see it and display it.
	 */
	if (aprog->exitStatus == 0)
	    (void) sleep(2);
	break;
    }

#ifdef NOSETENV
    xfree(newprog);
#else /* NOSETENV */
    nenv[2] = p;
    (void) Setenv(3, nenv);
    xfree(p);
#endif /* NOSETENV */

    if (aprog->stream && check_intr()) {
	(void) dputil_pclose(aprog->stream);
	aprog->stream = 0;
    }
#else /* USE_FILTER_ENCODERS */
    on_intr();
    TRY {
	aprog->stream = (struct dpipe *) dpipeline_filtfunc_open(aprog->program, file, direction);
    } EXCEPT(except_ANY) {
	aprog->stream = 0;
    } ENDTRY;

    if (aprog->stream && check_intr()) {
	(void) dpipeline_filtfunc_close(aprog->stream);
	aprog->stream = 0;
    }
#endif /* USE_FILTER_ENCODERS */

    off_intr();

    return aprog->stream;
}

static char *detach_dir_cache = 0;

void
check_detach_dir()
{
    if (detach_dir_cache) {
	free(detach_dir_cache);
	detach_dir_cache = 0;
    }
}

char *
get_detach_dir()
{
    if (detach_dir_cache)
	return detach_dir_cache;
    else {
	char *p, buf[MAXPATHLEN], *path;
	char *dir = value_of(VarDetachDir);
	int eno;

	/* Make sure the $folder directory exists */
	p = getdir("+", TRUE);
	eno = errno;	/* Save errno for reporting later */

	if (!dir)
	    dir = DEF_DETACH;

	if (dgetstat(p, dir, buf, NULL) != 0) {
	    error(SysErrWarning, "%s", dir);
	    if (p && strcmp(dir, DEF_DETACH) != 0) {
		(void) sprintf(buf, "%s%c%s", p, SLASH, DEF_DETACH);
		p = buf;
	    } else {
		errno = eno;
		p = NULL;
	    }
	} else
	    p = buf;
	if (!p || !(path = getdir(p, TRUE))) {
	    error(SysErrWarning, "%s", p? p : "+");
	    path = value_of(VarTmpdir);
	    if (!path)
		path = ALTERNATE_HOME;
	    (void) pathcpy(buf, path);
	    /* This should always succeed unless the tmpdir doesn't exist,
	     * in which case lots of other functions are already complaining.
	     */
	    path = getdir(buf, FALSE);
	}
	return detach_dir_cache = savestr(path);
    }
}

static int
basecmp(s1, s2)
const char *s1, *s2;
{
    return strcmp(basename(s1), s2);
}

Attach *
lookup_part(list, number, name)
const Attach *list;
int number;
const char *name;
{
    Attach *a;
    int x;

    if (number) {
	x = is_multipart(list);
	a = (Attach *)retrieve_nth_link((struct link *) list, number+x);
    } else if (name) {
	if (!(a = (Attach *)retrieve_link((struct link *) list, name, basecmp))) {
	    /* Bart: Sat Oct  3 13:23:56 PDT 1992 */
	    while (a = (Attach *)retrieve_nth_link((struct link *) list, ++number))
		if (a->content_name && basecmp(a->content_name, name) == 0)
		    break;
	}
    }
    return a;
}

/*
 * Detach one or more attachment parts from a multipart message.
 */
int
detach_parts(i, number, name, out, decoding, program, disposition)
    int		i;		/* The message number to detach from */
    int		number;		/* The part number to detach (if known) */
    const char 	*name;		/* Part name if number not known */
    const char	*out;		/* Output file name (if different) */
    const char	*decoding;	/* Decoding keyword if not that in the part */
    const char	*program;	/* Display keyword if not that in the part */
    unsigned long disposition;	/* What to do with the parts. */
EXC_BEGIN
{
    const int do_all = (int) (disposition & DetachAll);
    char buf[MAXPATHLEN], *errfile = NULL, *path, *filemode;
    struct dpipe *pp;
    FILE *efp;
    int x, check_tmp, do_auto = 0;
    Attach *a;
    AttachProg *pap, ap;
    AskAnswer answer;
#if defined(MAC_OS) || defined (MSDOS)
    char *tmp;
#endif /* MAC_OS || MSDOS */

    if (do_all) {
	do_auto = (number < 0);
	number = 1;
	name = NULL;
    }

    if (do_auto && !msg[i]->m_attach)
	return 0;

    on_intr();

    if (!(efp = open_tempfile("err", &errfile))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 152, "Cannot open temporary file" ));
	off_intr();
	return (-1);
    }
    fclose(efp);

    if (disposition & DetachCopy)
	disposition |= DetachSave;

    do {
	check_tmp = !(disposition & DetachSave);
	if (!(a = lookup_part(msg[i]->m_attach, number, name))) {
	    if (do_auto || do_all && number > 1)
		break;
	    if (number > 0)
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 153, "Message %d has fewer than %d attachments." ), i+1, number);
	    else
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 154, "Cannot find attachment \"%s\" in message %d." ), name, i+1);
	    (void) unlink(errfile);
	    xfree(errfile);
	    off_intr();
	    return -1;
	}
	if (do_all)
	    ++number;

	if (disposition & DetachExplode) {
	    x = explode_multipart(i, a, (Source *)0, (int *)0, FALSE);
	    if (x >= 0) {
		/* Call the message_panes callbacks to redraw GUI windows */
		ZmCallbackCallAll(VarMessagePanes, ZCBTYPE_VAR, ZCB_VAR_SET,
				  get_var_value(VarMessagePanes));
	    }
	    if (x > 0)
		break;	/* We were interrupted via the task meter? */
	    continue;
	}

	/* Figure out how to perform decoding and display */
	if (do_all || !decoding && a->encoding_algorithm)
	    decoding = a->encoding_algorithm;
	if (decoding && ci_strcmp(decoding, "none") == 0)
	    decoding = NULL;
	if ((disposition & DetachDisplay) && (do_all || !program && a->data_type))
	    program = attach_data_type(a);
	if (program && ci_strcmp(program, "none") == 0)
	    program = NULL;

	if (do_auto && (!program || !chk_option(VarAutodisplay, program)))
	    continue;

	/* Figure out where to put the file */
	if (do_all || !out || !*out) {
	    out = (!do_all && name && *name)? name :
		    (a->a_name? a->a_name : a->content_name);
	    if (!(disposition & DetachSave) && !(name && *name)) {
		if (!is_fullpath(out)) {
		    if (path = get_detach_dir()) {
			(void) sprintf(buf, "%s%c%s", path, SLASH, out);
			out = buf;
		    } else {
			error(SysErrWarning, "%s", buf);
			(void) unlink(errfile);
			xfree(errfile);
			off_intr();
			return -1;
		    }
		} else
		    out = pathcpy(buf, out);
		check_tmp = 2;
	    }
	}
	if (!out || !*out) {
	    error(UserErrWarning,
		catgets( catalog, CAT_MSGS, 155, "No file name for detach, message %d, part %d." ), i+1, number);
	    (void) unlink(errfile);
	    xfree(errfile);
	    off_intr();
	    return -1;
	}

	x = 1;
	path = getpath(out, &x);
	if (x != 0) {
	    if (x == 1) {
		name = basename(a->content_name);
		if (ask(AskYes, catgets( catalog, CAT_MSGS, 158, "\"%s\" is a directory.\nUse %s%c%s?" ),
			trim_filename(path), trim_filename(path),
			/* XXX The above works only because the same path
			 * is passed to both trim_filename() calls!  XXX */
			SLASH, name) != AskYes) {
		    (void) unlink(errfile);
		    xfree(errfile);
		    off_intr();
		    return -1;
		}
		(void) strcat(path, SSLASH);
		(void) strcat(path, name);
	    } else if (x == -1) {
		error(UserErrWarning, "%s: %s", out, path);
		(void) unlink(errfile);
		xfree(errfile);
		off_intr();
		return -1;
	    }
	}
	path = fullpath(strcpy(buf, path), FALSE);

	/* Create the decode and display program templates */
	if (decoding) {
	    /*
	    pap = coder_prog(TRUE, a, decoding, path, "w", !!program)
	    */
	    if (pap = coder_prog(TRUE, a, decoding, path, "w", FALSE)) {
		ap = *pap;
		ap.program = savestr(ap.program);
	    }
	    else
		decoding = NULL;
	}
	if (program) {
	    /*
	    pap = coder_prog(FALSE, a, program, path, "x", !!decoding);
	    */
	    pap = coder_prog(FALSE, a, program, path, "x", FALSE);
	    if (!pap)
		program = NULL;
	    /*
	    else if (decoding) {
		combine_coders(&ap, pap);
		program = NULL;
	    } else {
		error();
		(void) unlink(errfile);
		xfree(errfile);
		if (decoding)
		    free(ap.program);
		off_intr();
		return -1;
	    }
	    */
	}

	/* Bart: Mon Jan  4 12:40:31 PST 1993 */
	if ((disposition & DetachDisplay) && !program || program && pap->checked < 0) {
	    switch (ask(AskYes, program? catgets( catalog, CAT_MSGS, 159, "%s\nDetach anyway?" ) :
			catgets( catalog, CAT_MSGS, 160, "Cannot display \"%s\".\nDetach anyway?" ),
			program? pap->program : attach_data_type(a))) {
		case AskYes:
		    program = NULL;
		    break;
		case AskNo:
		    if (decoding)
			free(ap.program);
		    continue;
		case AskCancel:
		    (void) unlink(errfile);
		    xfree(errfile);
		    if (decoding)
			free(ap.program);
		    off_intr();
		    return -1;
	    }
	}

	/* we've ignored ENOENT, so test to see if file exists for overwrite */
	if (isoff(disposition, DetachOverwrite) && Access(path, F_OK) == 0) {
	    if (check_tmp > 1 && ison(a->a_flags, AT_NAMELESS))
		answer = AskYes;
	    else if (check_tmp > 1 && a->a_name)
		answer = (pathcmp(a->a_name, path) == 0)? AskNo : AskYes;
	    else
		answer = ask(AskYes, catgets( catalog, CAT_MSGS, 161, "\"%s\": File exists. Overwrite?" ),
			trim_filename(path));
	    if (answer == AskCancel) {
		if (do_all)
		    continue;
		else {
		    (void) unlink(errfile);
		    xfree(errfile);
		    if (decoding)
			free(ap.program);
		    off_intr();
		    return -1;
		}
	    } else if (answer == AskNo)
		goto ShowItToMe;
	}

	/* Use efp as a dummy to test that we can write path */
	if (!(efp = fopen(path, "w"))) {
	    error(SysErrWarning,
		catgets(catalog, CAT_SHELL, 335, "Cannot write to \"%s\""),
		trim_filename(path));
	    (void) unlink(errfile);
	    xfree(errfile);
	    if (decoding)
		free(ap.program);
	    off_intr();
	    return -1;
	}
	(void) fclose(efp);

	/* Reset the current values for the attachment */
	if (check_tmp && (!a->a_name || !is_fullpath(a->a_name))) {
	    ZSTRDUP(a->a_name, path);
	    turnon(a->a_flags, AT_TEMPORARY);
	} else if (a->a_name && !(disposition & DetachCopy) &&
		pathcmp(a->a_name, path) != 0) {
	    if (ison(a->a_flags, AT_TEMPORARY))
		(void) unlink(a->a_name);	/* We'll rewrite it */
	    ZSTRDUP(a->a_name, path);
	    if (ison(a->a_flags, AT_NAMELESS))
		ZSTRDUP(a->content_name, basename(path));
	}
	if (check_tmp == 0)
	    turnoff(a->a_flags, AT_TEMPORARY);
	else if (check_tmp > 1)
	    turnon(a->a_flags, AT_TEMPORARY);
	if (!a->a_name && !(disposition & DetachCopy))
	    ZSTRDUP(a->a_name, path);

	if (decoding && *decoding && decoding != a->encoding_algorithm)
	    ZSTRDUP(a->encoding_algorithm, decoding);
	if (program && *program && program != a->data_type)
	    ZSTRDUP(a->data_type, program);

#ifdef MSDOS
	/* folder attachments must be written out without CRLF's */
	filemode = (a->data_type &&
	    (!ci_strcmp(attach_data_type(a), "text/x-zm-folder") ||
	    (!ci_strcmp(attach_data_type(a), "application/x-zm-folder")))) ?
	    "wb" : "w";
#else /* !MSDOS */
	filemode = "w";
#endif /* !MSDOS */
	if (!decoding || !*decoding ||
		!(pp = popen_coder(&ap, path, errfile, filemode))) {
	    if (a->encoding_algorithm &&
		    ci_strcmp(a->encoding_algorithm, "none") != 0) {
		wprint(catgets( catalog, CAT_MSGS, 164, "Warning: Attachment is encoded with \"%s\".\n\
Output file \"%s\" will be in encoded form.\n" ), a->encoding_algorithm, path);
	    }
	    decoding = NULL;
	    pp = detach_open_unencoded(a, path);
	}
#if defined (MAC_OS)
	if (pp && (tmp = get_type_from_mactype(a->content_type)))
	    gui_set_filetype(AttachmentFile, path, (tmp != a->content_type) ? tmp : nil);
#endif /* MAC_OS */
	if (pp) {
	    long bytes =
		fp_to_dp(tmpf, a->content_offset, a->content_length, pp);
	    if (bytes != a->content_length) {
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 165, "%s: wrote %ld bytes, expected %ld" ),
		    path, bytes, a->content_length);
		program = NULL;
	    } else
		wprint(catgets( catalog, CAT_MSGS, 166, "%s: wrote %ld bytes.\n" ), path, bytes);
	    if (decoding && *decoding) {
		if (handle_coder_err(pclose_coder(&ap), ap.program, errfile)) {
		     /* Bart: Fri Aug 28 13:07:12 PDT 1992
		      * Make sure we write the file again next time in case
		      * the user fixed whatever error we just had.
		      */
		    if (ison(a->a_flags, AT_TEMPORARY))
			(void) unlink(path);
		     /* Bart: Fri Aug  7 18:24:55 PDT 1992
		      * Could ask() here whether we should continue anyway ...
		      */
		    (void) unlink(errfile);
		    xfree(errfile);
		    free(ap.program);
		    off_intr();
		    return -1;
		}
#ifdef POST_UUCODE_DECODE
/* 3/8/95 GF -- catch macbinaried, etc., encoded files from non-mime mailers -- just like from */
/*	zm_uudecode -- but only if the original transfer encoding was uuencode.  */
		if (a->mime_encoding <= ExtendedMimeEncoding  &&
			!(disposition & DetachCopy) &&
			(ci_strncmp(a->encoding_algorithm, "x-uue", 5) == 0 || 
			ci_strncmp(a->encoding_algorithm, "uue", 3) == 0))
		{
		    CheckNAryEncoding(&a->a_name);
		}
#endif /* POST_UUCODE_DECODE */
	    } else {
		TRY {
		    dputil_fclose(pp);
		} EXCEPT(ANY) {
		    error(SysErrWarning, catgets(catalog, CAT_MSGS, 992, "Cannot save attachment"));
		    if (ison(a->a_flags, AT_TEMPORARY))
			(void) unlink(path);
		    (void) unlink(errfile);
		    xfree(errfile);
		    off_intr();
		    EXC_RETURNVAL(int, -1);
		} ENDTRY;
	    }
	} else {
	    (void) unlink(errfile);
	    xfree(errfile);
	    error(SysErrWarning, decoding? decoding: path);
	    if (decoding)
		free(ap.program);
	    off_intr();
	    return -1;
	}

#ifdef C3
	if (!(disposition & (DetachCharsetOk|DetachCopy)))
	    attach_c3_translate(a, a->mime_char_set, fileCharSet, 1);
#endif /* C3 */

ShowItToMe:
	if (program && *program && !check_intr()) {
	    /* Nothing useful returned when "x" */
	    (void) popen_coder(pap, path, errfile, "x");
	    if (pap->checked >= 0)
		handle_coder_err(pap->exitStatus, pap->program, errfile);
	}
	if (decoding)
	    free(ap.program);
    } while (do_all && !check_intr());
    (void) unlink(errfile);
    xfree(errfile);
    off_intr();
    return 0;
} EXC_END

static struct dpipe *
detach_open_unencoded(a, file)
Attach *a;
const char *file;
{
    struct dpipe *pp;
    FMODE_SAVE();

#ifdef MSDOS
    /* hack for DOS; write out messages or folders in binary mode. */
    if (a && (a->mime_type == MessageRfc822 ||
	      a->mime_type == MessageNews ||
	      !ci_strcmp(a->content_type, "application/x-zm-folder")))
	FMODE_BINARY();
    else
	FMODE_TEXT();
#endif /* MSDOS */
    pp = dputil_fopen(file, "w");
    FMODE_RESTORE();
    return pp;
}

char *detach_flags[][2] = {
    { "-all",		      	"-a" },
    { "-charset",		"-C" },
    { "-code",			"-c" },
    { "-delete",		"-D" },
    { "-display",		"-d" },
    { "-encoding",		"-e" },
    { "-explode",		"-x" },
    { "-interactive",		"-i" },
    { "-info",			"-I" },
    { "-list", 			"-l" },
    { "-name",			"-n" },
    { "-part",		      	"-p" },
    { "-prune",		      	"-D" },
    { "-rehash",		"-r" },
    { "-reset",			"-r" },
    { "-temporary",		"-T" },
    { "-type",			"-t" },
    { "-undelete",		"-U" },
    { "-use",			"-u" },
    { NULL, NULL }
};

/*
 * Usage:
 * detach [-name file] [-part number | -all] [-encoding encode_key]
 *        [-display | -use program_key | -delete | -undelete] [-charset set]
 *	  [msg-number] [file]
 * detach [-interactive] [-list | -info item var] [msg-number]
 */
int
zm_detach(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    const char *name = NULL, *decoding = NULL, *program = NULL, *out = NULL;
    int i, number = -1;
    int do_all = TRUE, do_list = FALSE, interact = FALSE;
    unsigned long disposition = DetachSave;
    int info = FALSE;
    const char *infoitem, *infovar, *charsetname;
    mimeCharSet charset = fileCharSet;
    int delete = FALSE, undelete = FALSE;

    if (!argv[1]) {
	do_list = TRUE;
	++argv;
    } else {
	while (*++argv && **argv == '-') {
	    fix_word_flag(argv, detach_flags);
	    switch (argv[0][1]) {
		case 'a':
		    do_all = TRUE;
		when 'e': /* encoding keyword */
		case 'c': /* code [see zm_attach()] */
		    if (!(decoding = *++argv)) {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 167, "detach: no encoding-key specified." ));
			return -1;
		    } else if (!strcmp(decoding, "?")) {
			(void) get_attach_keys(TRUE, (Attach *)0, decoding);
			return 0;
		    }
		when 'n': /* name name */
		    if (!(name = *++argv)) {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 168, "detach: no attachment name specified." ));
			return -1;
		    }
		    do_all = FALSE;
		    if (number < 0)
			number = 0;
		when 'i': /* interactive */
		    interact = TRUE;
		when 'I': /* info */
		    info = TRUE;
		when 'l': /* list */
		    do_list = TRUE;
		when 'p': /* part number */
		    if (number > 0) {
			error(UserErrWarning, catgets(
			    catalog, CAT_MSGS, 885, "detach: specify at most one part."));
			return -1;
		    }
		    if (!*++argv ||
			    (number = atoi(*argv)) < 1 && !isdigit(**argv)) {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 169, "detach: no attachment number specified." ));
			return -1;
		    }
		    do_all = FALSE;
		when 'r': /* rehash or reset */
		    (void) get_attach_keys(-1, (Attach *)0, NULL);
		    return 0;
		    break;
		when 'D':
		    delete = TRUE;
		when 'U':	/* undelete */
		    undelete = TRUE;
		when 'u': /* use program (for type named) */
		case 't': /* type */
		    if (!(program = *++argv)) {
			error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 170, "detach: no type-key specified" ));
			return -1;
		    } else if (!strcmp(program, "?")) {
			(void) get_attach_keys(FALSE, (Attach *)0, program);
			return 0;
		    }
		    do_all = FALSE;
		    /* Fall through */
		case 'd': /* display */
		    turnon(disposition, DetachDisplay);
		    /* Fall through */
		case 'T':
		    turnoff(disposition, DetachSave);
		case 'O':
		    turnon(disposition, DetachOverwrite);
		when 'C':
		    if (!(charsetname = *++argv)) {
			error(UserErrWarning,
			    catgets(catalog, CAT_MSGS, 993, "detach: no character set specified"));
			return -1;
		    } else if (!strcmp(charsetname, "-")) {
			turnon(disposition, DetachCharsetOk);
		    } else {
			charset = GetMimeCharSet(charsetname);
		    }
		when 'x':
		    turnon(disposition, DetachExplode);
		    turnoff(disposition, DetachSave);
		    turnoff(disposition, DetachDisplay);
		when '?':
		    return help(0, "detach", cmd_help);
		otherwise:
		    error(UserErrWarning,
			    catgets( catalog, CAT_MSGS, 171, "detach: unrecognized option: %s" ), argv[0]);
		    return -1;
	    }
	}
    }
    if (do_all)
	disposition |= DetachAll;
    
    if (info) {
	if (!*argv) {
	    error(UserErrWarning, "Specify the item of information wanted.");
	    return -1;
	}
	infoitem = *argv++;
	if (!*argv) {
	    error(UserErrWarning, "No information variable specified.");
	    return -1;
	}
	infovar = *argv++;
    }
    if ((i = get_msg_list(argv, list)) < 0)
	return -1;
    argv += i;
    if (*argv) {
	if (!info) {
	    out = *argv;
	    if (!name && number < 0) {
		name = *argv;
		do_all = FALSE;
	    }
	    if (*++argv) {
		error(UserErrWarning, catgets( catalog, CAT_MSGS, 172, "detach: extra arguments after file name." ));
		return -1;
	    }
	} else {
	    error(UserErrWarning, "Extra arguments after variable.");
	    return -1;
	}
    }
    if (do_all) {
	if (name) {
	    error(UserErrWarning,
		catgets( catalog, CAT_MSGS, 173, "detach -all: cannot detach all parts to the same file." ));
	    return -1;
	}
	if (program) {
	    error(UserErrWarning,
		catgets( catalog, CAT_MSGS, 174, "detach -all: won't use same program for all parts." ));
	    return -1;
	}
	if (number > 0)
	    print(catgets( catalog, CAT_MSGS, 175, "detach -all: ignoring option \"-part %d\"\n" ), number);
	number = 1;
    }
    for (i = 0; i < msg_cnt; i++)
	if (msg_is_in_group(list, i))
	    break;
    if (i == msg_cnt)
	return 0;
    if (count_msg_list(list) > 1 &&
	ask(WarnCancel,
catgets( catalog, CAT_MSGS, 176, "Attachments can be detached from only one message at a time.\n\
More than one message was provided.  Detach using message %d?" ), i+1) != AskYes)
	return -1;

    if (info) {
	if (!strcmp(infoitem, "parts-list"))
	    return get_attachments_list(msg[i]->m_attach, infovar);
    }
    if (!msg[i]->m_attach) {
	if ((do_all || number < 2) && !do_list && !info) {
	    display_msg(i, NO_HEADER);
	    return 0;
	}
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 177, "Message %d has no attachments." ), i+1);
	return -1;
    }
    if (do_list && do_all)
	return list_attachments(msg[i]->m_attach, (Attach *)NULL,
				TRPL_NULL, TRUE, FALSE);
    else if (interact) {
	wprint(catgets( catalog, CAT_MSGS, 178, "Interactive detach not yet supported.\n" ));
	return -1;
    }
    if ((!name || !*name) && (number < 0 ||
	    number == 0 && (delete || undelete || do_list))) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MSGS, 179, "Specify the part to detach (-part # or -name name)." ));
	return -1;
    }
    if (do_list || info || delete || undelete) {
	Attach *a = msg[i]->m_attach;
	if (number == 0 || (a = lookup_part(a, number, name))) {
	    return do_list ?
		list_attachments(msg[i]->m_attach, a, TRPL_NULL, FALSE, FALSE) :
	    info ?
		get_attachments_info(a, infovar, infoitem) :
	    delete ?
		prune_part_delete(current_folder, msg[i], a) :
	    prune_part_undelete(current_folder, msg[i], a);
	} else {
	    if (number > 0)
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 153, "Message %d has fewer than %d attachments." ), i+1, number);
	    else
		error(UserErrWarning,
		    catgets( catalog, CAT_MSGS, 154, "Cannot find attachment \"%s\" in message %d." ), name, i+1);
	    return -1;
	}
    } else {
	mimeCharSet oldFileCharSet = fileCharSet;
	fileCharSet = charset;
	if (name) {
	    if (number < 0)
		number = 0;
	} else if (number == 0)
	    name = msg[i]->m_attach->content_name;
	TRY {
	    i = detach_parts(i, number, name, out,
			     decoding, program, disposition);
	} FINALLY {
	    fileCharSet = oldFileCharSet;
	} ENDTRY;
	return i;
    }
}

int
has_simple_encoding(attach)
const Attach *attach;
{
    return (!attach->mime_encoding ||
	    attach->mime_encoding == SevenBit ||
	    attach->mime_encoding == EightBit ||
	    attach->mime_encoding == QuotedPrintable);
}

/*
 * Given a pointer to the first of a possible chain of attachment 
 * structures, return true if they represent a single, unencoded text part,
 * in our character set (or it's ascii and our charset is a superset).
 * Also return true if the attach ptr is NULL.
 * Return false otherwise.
 */
int
is_plaintext(attach)
const Attach *attach;
{
    if (!attach || (attach->mime_type < ExtendedMimeType &&
	   (!ci_strncmp(attach->content_type, "text", 4) &&
	    (!attach->encoding_algorithm || 
	     !ci_strcmp(attach->encoding_algorithm, "none"))))) {
	return TRUE;
    }
    if (attach->mime_type == TextPlain) {
	if (has_simple_encoding(attach)) {
#ifdef C3
	    char *p = value_of(OverrideCharSet);
	    mimeCharSet charSet;

	    if (p && *p) {
		charSet = GetMimeCharSet(p);

		if (!IsKnownMimeCharSet(charSet))
		    charSet = attach->mime_char_set;
	    } else {
		charSet = attach->mime_char_set;
	    }

	    if (!IsKnownMimeCharSet(charSet))
		charSet = GetMimeCharSet(
				FindParam(
					MimeTextParamStr(CharSetParam),
					&attach->content_params));
	    if (!C3_TRANSLATION_WANTED(charSet, currentCharSet) ||
	        C3_TRANSLATION_POSSIBLE(charSet, currentCharSet))
#endif /* C3 */
            {
		return TRUE;
	    }
	}
    }
    return FALSE;
}

/*
 * Given a pointer to the first of a possible chain of attachment 
 * structures, return true if the structure represents a text part,
 * regardless of character set, which is either unencoded or encoded
 * as quoted-printable.  These parts are to be shown in line, but should
 * still have attachment icons if they are not plaintext, as evaluated by
 * the above "is_plaintext" function.
 * Also return true if the attach ptr is NULL.
 * Return false otherwise.
 */
int
is_readabletext(attach)
const Attach	*attach;
{
    if (!attach || 
	(!ci_strcmp(attach->content_type, "text") &&
	 (!attach->encoding_algorithm || 
	  !ci_strcmp(attach->encoding_algorithm, "none"))) ||
	((attach->mime_type == TextPlain) &&
	  has_simple_encoding(attach)))
	return TRUE;
    else
	return FALSE;
}

/*
 * Given a pointer to the first of a possible chain of attachment 
 * structures, return true if they represent a multipart object.
 * Return false otherwise.
 */
int
is_multipart(attach)
const Attach	*attach;
{
    /* For now, treat MultipartAlternative as a single part to be passed
     * to metamail
     */
    if (attach && /* (attach->mime_type != MultipartAlternative) && */
	((MimePrimaryType(attach->mime_type) == MimeMultipart) ||
	 (!attach->mime_type &&
	   (!(ci_strcmp(attach->content_type, "multipart")) ||
	    !(ci_strcmp(attach->content_type, "x-zm-multipart"))) ||
	    /* The mime info may not yet have been derived */
	    (attach->mime_type == ExtendedMimeType &&
             GetMimePrimaryType(attach->content_type) == MimeMultipart))))
	return TRUE;
    else
	return FALSE;
}

int
get_attachments_info(a, var, item)
const Attach *a;
const char *var, *item;
{
    const char *data;
    char buf[32];
    
    if (!strcmp(item, "encoding"))
	data = a->encoding_algorithm;
    else if (!strcmp(item, "type"))
	data = a->data_type;
    else if (!strcmp(item, "size")) {
	sprintf(buf, "%lu", a->content_length);
	data = buf;
    } else if (!strcmp(item, "description"))
	data = a->content_abstract;
    else if (!strcmp(item, "name"))
	/* used to just be content_name, which never even seems to be set */
	data = a->content_name ? a->content_name : a->a_name;
    else if (!ci_strcmp(item, "id"))
	data = a->mime_content_id;
    else if (!strcmp(item, "pruning"))
	data = ison(a->a_flags, AT_DELETE) ? "to-be-deleted" :
	pruned(a) ? "gone" : "none";
    else if (!strcmp(item, "unparsed-type"))
	data = a->orig_mime_content_type_str;
    else if (!strncmp(item, "param-", 6)) {
	if (!ci_strcmp(item+6, "charset"))
	    data = (char *) GetMimeCharSetName(a->mime_char_set); /* cast
								   * away
								   * const */
	else
	    data = FindParam(item+6, &a->content_params);
    } else {
	error(UserErrWarning, "Bad info item name: %s.", item);
	return -1;
    }
    if (!data) data = "";
    set_var(var, "=", data);
    return 0;
}

int
get_attachments_list(att, var)
Attach *att;
const char *var;
{
    int i, base;
    char buf[10];
    Attach *a;

    if (!att) {
	set_var(var, "=", "");
	return 0;
    }
    /* this will need to become more sophisticated when we can
     * poke around in multiparts.
     */
    base = is_multipart(att);
    for (i = 1; a = (Attach *) retrieve_nth_link(att, i+base); i++) {
	set_var(var, (i != 1) ? "+=" : "=", (sprintf(buf, " %d", i), buf));
    }
    return 0;
}

int
get_comp_attachments_info(compose, argv)
Compose *compose;
char **argv;
{
    char *name = 0;
    int number;
    Attach *a;
    
    if (!strcmp(argv[0], "parts-list"))
	return get_attachments_list(compose->attachments, argv[1]);
    if (!argv[2]) {
	error(UserErrWarning, "No part specified.");
	return -1;
    }
    number = atoi(argv[2]);
    if (!number)
	name = argv[2];
    if (!(a = lookup_part(compose->attachments, number, name))) {
	error(UserErrWarning, catgets(catalog, CAT_MSGS, 847, "Cannot find part %s."), argv[2]);
	return -1;
    }
    return get_attachments_info(a, argv[1], argv[0]);
}

int
list_templates(names, pattern, quiet)
char ***names;
const char *pattern;
int quiet;
{
    char buf[MAXPATHLEN], *p;
    int x, y, z;

    if (!pattern)
	pattern = "*";

    if (p = value_of(VarTemplates)) {
#ifndef MAC_OS
	skipspaces(0);
	(void)no_newln(p); /* Strips trailing spaces */
	(void)strcpy(buf, p);
	for (p = buf; p = any(p, " \t"); ) {
	    *p = ',';
	    skipspaces(1);
	}
	(void)strcpy(buf,
	    zmVaStr("{%s,%s}%c%s", form_templ_dir, buf, SLASH, pattern));
#else /* MAC_OS */
	strcpy (buf, zmVaStr("%s%c%s", p, SLASH, pattern));
#endif /* !MAC_OS */
    } else
	(void)sprintf(buf, "%s%c%s", form_templ_dir, SLASH, pattern);
    x = filexp(buf, names);
    /* filexp() just expands the expression; we don't know if the files
     * actually exist unless there are metacharacters in the pattern.
     */
    for (y = 0; y < x; y++) {
	int is_dir = 0;
	getpath((*names)[y], &is_dir);
	if (Access((*names)[y], R_OK) == -1 || is_dir) {
	    /* the glob matched a non-readable file or a directory. */
	    xfree((*names)[y]);
	    /* move down all the rest. */
	    for (z = y; z < x; z++)
		(*names)[z] = (*names)[z+1];
	    x--; /* decrement the total number of names we're returning */
	    y--;
	}
    }
    if (x == 0 && *names)
      free_vec(*names);
    if (quiet) return x;
    if (x == 0) {
	if (strcmp(pattern, "*") != 0)
	    error(UserErrWarning,
		catgets( catalog, CAT_MSGS, 190, "No templates matching \"%s\" found." ), pattern);
	else
	    error(UserErrWarning, catgets( catalog, CAT_MSGS, 191, "No templates available." ));
    }
    if (x < 0)
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 192, "Cannot list %s" ), buf);
    return x;
}

/* 
 * Copy the attachments from the specified message to the current 
 * composition.
 */
/*
static int
copy_attachments(list)
    msg_group	*list;
*/
void
copy_attachments(i)
    int		i; /* source message */
{
    int		j, base;
    Attach	*attachPtr;
    Attach	*newAttachPtr;
    char buf[HDRSIZ];
    char	*tempFileName = NULL;
    char	*encoding = NULL;
    
    /*
      for (i = 0; i < msg_cnt; i++)
      if (msg_is_in_group(list, i) && !is_plaintext(msg[i]->m_attach))
      ;
      */
    if (is_plaintext(msg[i]->m_attach))
	return;
    base = is_multipart(msg[i]->m_attach);
    (void) reply_to(i, FALSE, buf);
    for (j = 1; 
	 attachPtr = (Attach *)retrieve_nth_link(msg[i]->m_attach,j+base); 
	 j++) {
	/* XXX Perhaps should only continue if the part is not displayed 
	 * inline */
	if (attachPtr) {
	    char *str = attachPtr->content_name;
	    if (!str)
		str = attachPtr->a_name;
	    /* use old extension if it is valid */
	    if (str && (str = rindex(str, '.')) && strlen(str) <= 4)
		str++;
	    else
		str = "msg";
	    tempFileName = alloc_tempfname(str);
	    if (!tempFileName) {
		error(SysErrWarning, catgets( catalog, CAT_MSGS, 152, "Cannot open temporary file" ));
		continue;
	    }
	    /* Use DetachSave here so that closing the folder won't
	     * delete the attachment files out from under the composition
	     * to which they have been reattached.  The composition can
	     * take care of deleting the temp files.
	     */
	    if (!detach_parts(i, j, tempFileName, NULL,
			      NULL, NULL, DetachCharsetOk|DetachCopy)) {
		if (attachPtr->encoding_algorithm)
		    encoding = GetDefaultEncodingStr((attachPtr->data_type) ?
						     attachPtr->data_type :
						     attachPtr->content_type);
		if (!encoding)
		    encoding = "none";
		if (!(add_attachment(comp_current,
				     tempFileName,
				     (attachPtr->data_type?
				      attach_data_type(attachPtr) :
				      get_type_from_alias(attachPtr->content_type)),
				     attachPtr->content_abstract,
				     encoding,
				     attachPtr->a_flags|AT_TEMPORARY,
				     &newAttachPtr))) {
		    if (newAttachPtr) {
			turnon(newAttachPtr->a_flags, AT_CHARSET_OK);
			/* Use the real name of the original file */
			if (attachPtr->content_name &&
			    isoff(attachPtr->a_flags, AT_NAMELESS))
			    ZSTRDUP(newAttachPtr->content_name, 
				   attachPtr->content_name);
			if (attachPtr->content_abstract)
			    newAttachPtr->content_abstract = 
				ison(comp_current->send_flags, SEND_ENVELOPE)?
				savestr(attachPtr->content_abstract):
				savestr(zmVaStr(catgets(catalog, CAT_MSGS, 836, "Transferred from mail from %s: %s"), 
					buf, 
					attachPtr->content_abstract));
			if (encoding)
			    ZSTRDUP(newAttachPtr->mime_encoding_str,
				   encoding);
			if (attachPtr->mime_content_id)
			    ZSTRDUP(newAttachPtr->mime_content_id,
				   attachPtr->mime_content_id);
			newAttachPtr->mime_type = attachPtr->mime_type;
			if (encoding)
			    newAttachPtr->mime_encoding = 
				GetMimeEncoding(encoding);
			newAttachPtr->mime_char_set = 
			    attachPtr->mime_char_set;
			FreeContentParameters(&newAttachPtr->content_params);
			CopyContentParameters(&newAttachPtr->content_params,
					      &attachPtr->content_params);
		    }
		}
	    }
	    xfree(tempFileName);
	}
    }
}

#ifdef GUI
const char *
get_attach_label(a, is_compose)
Attach *a;
int is_compose;
{
    char *varname = is_compose ? VarCompAttachLabel : VarMsgAttachLabel;

    /* Default to the most specific equivalent type */
    const char *text = attach_data_type(a);
    /* If that's not known, use the sender's type */
    if (!text) text = a->content_type;

    /* User asked to see the description, override the type */
    if (chk_option(varname, "description"))
	text = a->content_abstract;
    /* Description was empty or user asked for the comment */
    if (!text || chk_option(varname, "comment")) {
	AttachKey *k = get_attach_keys(0, a, (char *)0);
	if (k && k->description)
	    text = k->description;
    }
    /* Nothing found yet and sender supplied a filename */
    if (!text || (a->content_name && bool_option(varname, "content")))
	text = a->content_name;
    /* Still nothing found, but user has supplied a filename */
    if (!text || bool_option(varname, "filename")) {
	if (a->content_name)
	    text = basename(a->content_name);
	else if (a->a_name)
	    text = basename(a->a_name);
    }
    /* If all else fails, or user asked for the type */
    /* Bart: Mon Mar  7 16:03:52 PST 1994
     * I don't really like the || here.  The way this function is set up,
     * anything other than "type" will be overridden if "type" is also set.
     */
    if (!text || bool_option(varname, "type")) {
	text = attach_data_type(a);
	if (!text) text = a->content_type;
    }
    /* If we still didn't get anything, label it unknown */
    if (!text) text = catgets(catalog, CAT_MSGS, 827, "unknown");
    return text;
}
#endif /* GUI */
