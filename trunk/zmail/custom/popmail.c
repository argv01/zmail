/**************************************************************************
CREDITS: Brian Buhrow, September 17, 1991
PURPOSE: This file contains the interface between the Z-Mail program and the
	POP post-office calls which put mail into the system mailbox.  Mail
	will be placed in the user's mailbox when Z-Mail is first fired up
	and will be checked periodically as Z-Mail is run.
	Note that these routines will only be called if the pre-processor
	variable POP3_SUPPORT is defined.
**************************************************************************/

#include "config.h"

#ifdef POP3_SUPPORT

#ifndef lint
static char	popmail_rcsid[] =
    "$Id: popmail.c,v 2.76 2005/04/27 08:46:10 syd Exp $";
#endif

#ifdef NOT_ZMAIL
#define lock_fopen		fopen
#define close_lock(x, y)	fclose(y)
#define cleanup(x)		exit(x)
#define DELIVERYFILE		getenv("MAIL")
#ifdef MSDOS
#define LOGINNAME		getenv("USER")
#else /* !MSDOS */
#define LOGINNAME		getlogin()
#endif /* !MSDOS */
#define TRUE			1
#define FALSE			0
#include "catalog.h"
#include "error.h"
#include "zcalloc.h"
#include "zccmac.h"
#include "zcstr.h"
#include "zctime.h"
#include "zcunix.h"
char *get_name_n_addr P((const char *, char *, char *));
char *bang_form P((char *, const char *));
#ifdef MSDOS 
#include <conio.h>
#endif /* MSDOS */
#ifdef LANWP
#include <pwd.h>
#endif /* LANWP */
#else /* !NOT_ZMAIL */

#include "zmail.h"
#include "fsfix.h"
#include "refresh.h"
extern Ftrack *real_spoolfile;
#define DELIVERYFILE	\
		(real_spoolfile ? ftrack_Name(real_spoolfile) : spoolfile)
#define LOGINNAME		zlogin

#endif /* NOT_ZMAIL */

#undef msg
#include "pop.h"
#include "dputil.h"
#include "dynstr.h"
#include "excfns.h"
#include "strcase.h"
#include "mailserv.h"

#ifdef MAC_OS
#define lock_fopen		fopen
#define close_lock(x, y)	fclose(y)
#undef when
#undef otherwise
#include "mac-stuff.h"
extern MacGlobalsPtr gMacGlobalsPtr;
#endif /* MAC_OS */

#ifdef _WINDOWS
extern char *gUserPass;
#endif

struct mailserver mailserv_struct = {NULL, NULL};
mailserver_t mserv = &mailserv_struct;

#ifndef HAVE_STRSTR
/*
 * strstr - find first occurrence of wanted in s
 */

char *				/* found string, or NULL if none */
strstr(s, wanted)
const char *s;
const char *wanted;
{
    const char *scan;
    long len;
    char firstc;

    /*
     * The odd placement of the two tests is so "" is findable.
     * Also, we inline the first char for speed.
     * The ++ on scan has been moved down for optimization.
     */
    firstc = *wanted;
    len = strlen(wanted);
    for (scan = s; *scan != firstc || strncmp(scan, wanted, len) != 0; )
	if (*scan++ == '\0')
	    return NULL;
    return (char *) scan;
}
#endif /* !HAVE_STRSTR */

/* This routine forms the header line for the From and Date functions below.
 * It was written by John Kammens for use by UCSC's version of UCBmail running
 * on the ATHENA system.
 */
static char *
header(msg, tag)
char *msg, *tag;
{
    char *val, *ptr, *eoh;

    val = (char *) malloc(strlen(tag) + 3);
    if (!val)
	return (0);

    sprintf(val, "\n%s:", tag);

    eoh = strstr(msg, "\n\n");
    if (!eoh)
	eoh = msg + strlen(msg);

    ptr = strstr(msg, val);

    if ((!ptr) || (ptr > eoh)) {
	sprintf(val, "%s:", tag);
	if (!strncmp(val, msg, strlen(val)))
	    ptr = msg;
	else
	    ptr = 0;
    }
    if (!ptr) {
	free(val);
	return (0);
    }

    ptr += strlen(val);

    while (*ptr && ((*ptr == ' ') || (*ptr == '\t')))
	ptr++;

    eoh = (char *) index(ptr, '\n');
    if (!eoh)
	eoh = ptr + strlen(ptr);

    val = (char *) realloc(val, (eoh - ptr) + 1);
    strncpy(val, ptr, (eoh - ptr));
    val[eoh - ptr] = '\0';

    return (val);
}

/* This routine comes up with the Unix Date line to insert into the mail
 * file.  It was written by John Kammens for use with UCSC's UCBMail on
 * Athena.
 *
 * Modified to get rid of dependence on that hideous B-news yacc parser.
 * Z-Mail already has a perfectly good date parser, let's use it.  -- Bart
 */
char *
date(msg)
char *msg;
{
    char *real = 0, *machine = 0;
    time_t t;
    int size;

    t = time((time_t *)0);
    machine = ctime(&t);
    size = strlen(machine);
    machine[size - 1] = '\0';	/* get rid of newline */
    real = (char *) malloc(size);
    if (!real)
	return 0;
    strcpy(real, machine);

    return real;
}

/* This routine comes up with the Unix From line to insert into the mail
 * file.  It was written by John Kammens for use with UCSC's UCBMail on
 * Athena.
 *
 * Modified to use get_name_n_addr() so we don't depend on regexec()
 * being available, which it usually isn't. -- Bart
 */
char *
from_line(msg)
char *msg;
{
    char *real = header(msg, "From"), *real2;
    char addr[256];

    if (real) {
	addr[0] = 0;
	(void) get_name_n_addr(real, NULL, addr);
	if (addr[0]) {
	    (void) bang_form(real, addr);
	    return real;
	}
    }

    if (!real)
	real = (char *) malloc(8);
    if (!real)
	return 0;
    strcpy(real, catgets( catalog, CAT_CUSTOM, 26, "unknown" ));
    return real;
}

/* Retrieves mail from the system post office using the POP interface
 * routines as put together by John Kammens for the UCBmail program under
 * ATHENA.  This routine will be called both when Z-Mail begins executing to
 * check for new mail and when Z-Mail is executing to see if new mail has
 * arrived during the Z-Mail session.
 */
static void 
loadmail(master, preserve, poplogin, poppword)
    int master, preserve;
    char *poplogin, *poppword;
{
    PopServer postinfo;
    struct dpipe *dp;
    int msgcount, msgsize, i, flags = 0, n;
    char mailbox[MAXPATHLEN], *tmp, *msgptr, *dateline, *fromline;
    FILE *mfstream;
    struct stat mfstat;
    static char pass[64] = { '\0' }; /* RJL ** 6.14.93 -these need to be inited */
    static char logn[64] = { '\0' }; /* since they are passed to pop_open */
    int first_new = 1, err = 0;
    off_t pre_retr_offset;
#if defined( IMAP )
    int	fd;
    FILE *fp;
    char *path, fldr[ 128 ], usr[ 128 ], junk, savepath[MAXPATHLEN];
    unsigned long readUID;
    AskAnswer answer = AskYes;
#endif

    errno = 0;
    
#ifndef NOT_ZMAIL
    if (!spoolfile)
	spoolfile = savestr(getenv("MAIL"));
    if (!zlogin)
	poplogin = value_of(VarUser);
#endif /* !NOT_ZMAIL */
    
    if (tmp = DELIVERYFILE)
	strcpy(mailbox, tmp);
    else {
	error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 27, "Error reading environment: no MAIL file.\n" ));
	return;
    }

    if (poppword)
	strcpy(pass, poppword);
#ifdef MAC_OS
    else strcpy(pass, gMacGlobalsPtr->pass);
#endif
#ifdef _WINDOWS
    else mailserver_SetPassword (mserv, gUserPass);
    /*strcpy (pass, gUserPass);*/
#endif
    if (pass[0])
	flags = POP_NO_GETPASS;
    if (poplogin)
	strcpy(logn, poplogin);
    if (logn[0] == 0)
	strcpy(logn, LOGINNAME);

    /* Get info about post drop */
    if (!(postinfo = pop_open(NULL, logn, pass, flags))) {
	error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 28, "Error opening connection with post office: %s\n" ),
	      pop_error);
	return;
    }
    if (preserve && !pop_services(postinfo, POP3_LAST)) {
	error(ForcedMessage,
	      catgets(catalog, CAT_CUSTOM, 45, "Can't keep mail on POP host: server does not support the\n\
'LAST' command.  To fix, you should turn off 'preserve'\n\
in the pop_options variable in the Variables dialog.\n\
Mail check aborted."));
	pop_close(postinfo);
	return;
    }
    if (pop_stat(postinfo, &msgcount, &msgsize)) {
	error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 29, "Error collecting status from post office: %s\n" ),
	      pop_error);
	pop_close(postinfo);
	return;
    }
    if (!msgcount) {
	pop_quit(postinfo);
	return;
    }
    if (preserve) {
    	first_new = pop_last(postinfo);
	if (first_new < 0 || first_new >= msgcount) {
	    pop_quit(postinfo);
	    return;
	} 
	++first_new;
    }
#ifdef MAC_OS
    else {
	unsigned long space = GetFreeSpace(mailbox);
	if (space == -1 || msgsize > space * 1024) {
	    char vol[63], *v;
	    strcpy((char *) vol, mailbox);
	    if (v = strchr(vol, SLASH))
	    	*v = 0;
	    error(ZmErrWarning, catgets(catalog, CAT_CUSTOM, 46, "There isn't enough space available on the volume \"%s\" to download your new mail."), vol);
	    pop_quit(postinfo);
	    return;
	}
    }
#endif /* MAC_OS */

    /* Begin loading mailbox */
    if (!(mfstream = lock_fopen(mailbox, FLDR_APPEND_MODE))) {
#ifdef NOT_ZMAIL
	perror(catgets( catalog, CAT_CUSTOM, 30, "Error opening mailbox in loadmail" ));
#else /* !NOT_ZMAIL */
	error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 30, "Error opening mailbox in loadmail" ));
#endif /* !NOT_ZMAIL */
	pop_close(postinfo);
	return;
    }

#if defined( IMAP )
	fp = fopen( mailbox, "r" );
	if ( fp ) {
	     	n = fscanf( fp, "UIDVAL=%08lx%s %s %c\n", &readUID, fldr, usr, &junk );
		if ( n == 4 ) {

			/* would you like to save??? */

			answer = AskYes;
			answer = ask(AskYes, catgets( catalog, CAT_MSGS, 1104, "The MAIL variable is pointing to a folder that contains IMAP4 messages\nread from the mailbox %s.\nLoading messages from the POP server into this folder is not advised.\nSave a copy of %s?\n\nSelect 'Yes' to save a copy of %s,\n'No' to overwrite %s,\nor 'Cancel' to cancel the POP session and open\n%s in IMAP4 disconnected mode." ), fldr, mailbox, mailbox, mailbox, mailbox );
again:
			if ( answer == AskYes ) {
				strcpy( savepath, value_of(VarHome) );
				strcat( savepath, "/Mail" );
				path = tempnam( savepath, "ZmIMAP4Save" );
				if ( CopyIMAPFolder( mailbox, path ) == 0 ) {
					fclose( fp );
					return;
				}
			} else if ( answer == AskCancel ) {
				using_pop = 0;
				fclose( fp );
				return;
			} else {
				answer = AskYes;
				answer = ask(AskYes, catgets( catalog, CAT_MSGS, 1105, "Contents of %s will be overwritten. Please verify your selection.\n\nSelect 'Yes' to save a copy of %s.\nSelect 'No' to overwrite %s.\nSelect 'Cancel' to cancel the POP session and operate on\n%s in IMAP4 disconnected mode." ), mailbox, mailbox, mailbox, mailbox );
				if ( answer == AskYes || answer == AskCancel ) {
					goto again;
				}
			}

			/* Yes (save) or No (overwrite) causes a truncation 
                           of the file to zero */

			fd = open( mailbox, O_CREAT | O_RDWR | O_TRUNC, 0600 );
			close( fd );
		}
		fclose( fp );
	}
#endif

#if defined(MAC_OS) && defined(USE_SETVBUF)
    (void) setvbuf(mfstream, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
#ifndef NOT_ZMAIL
    if (msgcount > 0) {
	char *gotfrom = value_of(VarMailhost);
	init_intr_mnr(zmVaStr(msgcount - first_new ? catgets( catalog, CAT_CUSTOM, 32, "Retrieving %d messages from %s" )
			      : catgets( catalog, CAT_CUSTOM, 31, "Retrieving %d message from %s" ),
			      1 + msgcount - first_new, gotfrom),
		      100);
    }
#endif /* NOT_ZMAIL */
    err = 0;
    for (i = first_new; i <= msgcount && !err; i++) {	/* Load new messages */
#ifndef NOT_ZMAIL
	zmVaStr(catgets( catalog, CAT_CUSTOM, 33, "Retrieving %d ..." ), 1 + i - first_new);
	if (check_intr_mnr(zmVaStr(NULL), ((1 + i - first_new) * 100)/(1 + msgcount - first_new)))
	    break;
	if (!istool)
	    print("%s\n", zmVaStr(NULL));
#endif /* NOT_ZMAIL */
	TRY {
	    dp = pop_retrieve(postinfo, i);
	} EXCEPT(ANY) {
	    error(ZmErrWarning, catgets( catalog, CAT_CUSTOM, 14, "Out of memory in pop_retrieve" ));
	    err = TRUE;	    
	} FINALLY {
	    if (!dp)
	    	err = TRUE;
	} ENDTRY;
	if (err)
	    continue;
	pre_retr_offset = ftell(mfstream);
	if (FolderDelimited == def_fldr_type)
	    efprintf(mfstream, "%s", msg_separator);
	TRY {
	    msgptr = NULL;
	    n = dpipe_Get(dp, &msgptr);
	    if (n && strncmp(msgptr, "From ", 5) != 0) {
		struct dynstr ds;

		dynstr_Init(&ds);
		TRY {
		    dynstr_AppendN(&ds, msgptr, n);
		    free(msgptr);
		    msgptr = NULL;
		    /* Now we need to get ALL the headers to phony one up */
		    while (!strstr(dynstr_Str(&ds), "\n\n") &&
			   (n = dpipe_Get(dp, &msgptr))) {
			dynstr_AppendN(&ds, msgptr, n);
			free(msgptr);
		    }
		    msgptr = NULL;
		    dateline = date(dynstr_Str(&ds));
		    fromline = from_line(dynstr_Str(&ds));
		    TRY {
			efprintf(mfstream,
				 (FolderDelimited == def_fldr_type) ?
				 "From %s %s\n%s" : "\nFrom %s %s\n%s",
				 fromline, dateline, dynstr_Str(&ds));
		    } FINALLY {
			free(dateline);
			free(fromline);
		    } ENDTRY;
		} FINALLY {
		    dynstr_Destroy(&ds);
		} ENDTRY;
	    } else if (n) {
		efwrite(msgptr, sizeof(char), n, mfstream, "loadmail");
		free(msgptr);
		msgptr = NULL;
	    }
	    while (n = dpipe_Get(dp, &msgptr)) {
		efwrite(msgptr, sizeof(char), n, mfstream, "loadmail");
		free(msgptr);
		msgptr = NULL;
	    }
	} EXCEPT(ANY) {
	    xfree(msgptr);
	    ftruncate(fileno(mfstream), pre_retr_offset);
	    error(SysErrWarning, catgets(catalog, CAT_CUSTOM, 34, "Error writing mailbox file\n"));
	    err = TRUE;
#ifdef NOT_ZMAIL
	    pop_close(postinfo);
	    if (master)
		cleanup(-1);
	    else
		exit(1);
#endif /* NOT_ZMAIL */
	} FINALLY {
	    dputil_Destroy(dp);
	} ENDTRY;

	if (!err) {
	    if (FolderDelimited == def_fldr_type)
		efprintf(mfstream, "%s", msg_separator);
	    if (!preserve) {
		if (pop_delete(postinfo, i)) {
		    error(ZmErrWarning,
			  catgets(catalog, CAT_CUSTOM, 36,
				  "Error deleting message from post office: %s\n"),
			  pop_error);
		}
	    }
	}
    }
#ifndef NOT_ZMAIL
    if (msgcount > 0) {
	i -= first_new;
	end_intr_mnr(zmVaStr(ison(glob_flags, WAS_INTR) ?
			     i == 1 ?
			     catgets(catalog, CAT_CUSTOM, 37, "Stopped, retrieved %d messages.")
			     : catgets(catalog, CAT_CUSTOM, 42, "Stopped, retrieved %d message.")
			     : i == 1 ?
			     catgets(catalog, CAT_CUSTOM, 43, "Done, retrieved %d message.")
			     : catgets(catalog, CAT_CUSTOM, 44, "Done, retrieved %d messages."),
			     i),
		     100L);
    }
#endif /* NOT_ZMAIL */
    if (close_lock(mailbox, mfstream) == EOF) {
#ifdef NOT_ZMAIL
	perror(catgets(catalog, CAT_CUSTOM, 38, "Error closing mailbox file in loadmail"));
#else /* !NOT_ZMAIL */
	error(SysErrWarning, catgets(catalog, CAT_CUSTOM, 38, "Error closing mailbox file in loadmail"));
#endif /* !NOT_ZMAIL */
	pop_close(postinfo);
	return;
    }
    if (pop_quit(postinfo)) {
	error(ZmErrWarning, catgets(catalog, CAT_CUSTOM, 39, "Error closing post office: %s\n"), pop_error);
    }
    return;
}

#ifdef STANDALONE

main()
{
#ifdef MSDOS
   loadmail(TRUE, (getenv("POPKEEP")!=NULL), (char*)0, (char*) 0);
#else /* !MSDOS */ 
   loadmail(TRUE, TRUE, (char *)0, (char *)0);
#endif /* !MSDOS */
}

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

#define MAXPRINTLEN	4096

#ifdef HAVE_VPRINTF

#define VPrintf(f, a)		(void) vprintf(f, a)
#define VSprintf(b, f, a)	(void) vsprintf(b, f, a)

#else /* !HAVE_VPRINTF */

#define VPrintf(f, a)		(void) _doprnt(f, a, stdout)
    /* This macro is not ideal because it must be called with an
     * actual array name rather than a pointer.  It may also be
     * necessary to cast b to unsigned char when assigning.
     */
#define VSprintf(b, f, a) do {\
	    FILE VS_file;\
	    VS_file._cnt = (1L<<30) | (1<<14); /* was sizeof(b); */\
	    VS_file._base = VS_file._ptr = (b);\
	    VS_file._flag = _IOWRT+_IOSTRG;\
	    (void) _doprnt(f, a, &VS_file);\
	    *VS_file._ptr = '\0';\
	} while (0)

#endif /* HAVE_VPRINTF */

/* Print an error or other warning message.  First argument is the reason
 * for the message, second is printf formatting, rest are to be formatted.
 * If the reason is "Message", automatic appending of "\n" is supressed.
 */
void
#ifdef HAVE_STDARG_H
error (PromptReason ignored, char *fmt, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
error(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    char buf[MAXPRINTLEN];
    va_list args;
#ifndef HAVE_STDARG_H
    PromptReason ignored;
    char *fmt;

    va_start(args);
    ignored = va_arg(args, PromptReason);
    fmt = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, fmt);
#endif /* !HAVE_STDARG_H */
    VSprintf(buf, fmt, args);
    va_end(args);

    (void) fprintf(stderr, "%s\n", buf);
}

#if defined( MSDOS ) 
#if !defined( LANWP )
void bcopy(const void *src, void *dst, int length)
{
	memcpy( dst, src, length );
}

void bzero(void *b, int length)
{	 
	memset( b, 0, length );
}

#endif /* !LANWP */


char * getpass( char * prompt )
{
	static char buf[80];
	int ch;
	int i = 0;
	printf( prompt );
	
	/* non-echoing gets */
	while(1) 
	{
		ch = getch();
		if( ch == '\r' )
		{
			buf[i] = '\0';
			break;
		}
		buf[i++]=(char)ch;
		if( i >= 80 )
		{
			buf[i] = '\0';
			break;
		}
	}
	printf("\n");
	return buf;
}

#endif /* MSDOS */

/* this strcpy returns number of bytes copied */
Strcpy(dst, src)
     register char *dst;
     register const char *src;
{
    register char *d = dst;

    if (!dst || !src)
	return 0;
    while (*dst++ = *src++)
	;
    return dst - d - 1;
}

/* Find any character in s1 that's in s2;
 * return pointer to that char in s1.
 */
char *
any(s1, s2)
register char *s1, *s2;
{
    register char *p;

    if (!s1 || !*s1 || !s2 || !*s2)
	return NULL;
    for( ; *s1; s1++) {
	for(p = s2; *p; p++)
	    if (*p == *s1)
		return s1;
    }
    return NULL;
}

/*
 * Convert RFC822 or mixed addresses to RFC976 `!' form,
 *  copying the new address to d.  The source address is
 *  translated according to RFC822 rules.
 * Return a pointer to the end (nul terminus) of d.
 */
char *
bang_form (d, s)
char *d;
const *s;
{
    char *r, *t, *ab = NULL;

    *d = '\0';
    /* If nothing to do, quit now */
    if (!s || !*s) {
	return d;
    }
    /* Avoid any angle braces */
    if (*s == '<') {
	if (ab = index(s + 1, '>'))
	    s++, *ab = '\0';
	else
	    return NULL;
    }
    /*
     * Look backwards for the first `@'; this gives us the
     * primary domain of the RFC822 address
     */
    if (*s == '@') {
	/* An RFC-822 "@domain1,@domain2:" routing */
	if (t = any(++s, ",:")) {
	    char c = *t;
	    *t = '\0';
	    d += Strcpy(d, s);
	    *d++ = '!';
	    *t++ = c;
	    r = bang_form(d, t);
	} else
	    r = NULL;
    } else if ((t = rindex(s, '@')) && t != s) {
	/* Copy the RFC822 domain as the UUCP head */
	d += Strcpy(d, t + 1);
	*d++ = '!';
	*t = '\0';
	r = bang_form(d, s);
	*t = '@';
    } else if (t = index(s, '!')) {
	/* A normal UUCP path */
	*t = '\0';
	d += Strcpy(d, s);
	*t++ = *d++ = '!';
	r = bang_form(d, t);
    } else if (t = rindex(s, '%')) {
	/* An imbedded `%' -- treat as low-priority `@' */
	*t = '@';
	r = bang_form(d, s);
	*t = '%';
    } else
	r = d + Strcpy(d, s);  /* No `@', `!', or `%' */
    if (ab)
	*ab = '>';
    return r;
}

/*
 * Get address and name from a string (str) which came from an address header
 * in a message or typed by the user.  The string may contain one or more
 * well-formed addresses.  Each must be separated by a comma.
 *
 * address, address, address
 * address (comment or name here)
 * comment or name <address>
 * "Comment, even those with comma's!" <address>
 * address (comma, (more parens), etc...)
 *
 * This does *not* handle cases like:
 *    comment <address (comment)>
 *
 * find the *first* address here and return a pointer to the end of the
 * address (usually a comma).  Return NULL on error: non-matching parens,
 * brackets, quotes...
 *
 * Suppress error mesages if BOTH name and addr are NULL.
 */
char *
get_name_n_addr(str, name, addr)
const char *str;
char *name, *addr;
{
    register char *p, *p2, *beg_addr = addr, *beg_name = name, c;
    static char *specials = "<>@,;:\\.[]";	/* RFC822 specials */
    int angle = 0;

    if (addr)
	*addr = 0;
    if (name)
	*name = 0;
    if (!str || !*str)
	return NULL;

    while (isspace(*str))
	str++;

    /* first check to see if there's something to look for */
    if (!(p = any(str, ",(<\""))) {
	/* no comma or indication of a quote character. Find a space and
	 * return that.  If nothing, the entire string is a complete address
	 */
	if (p = any(str, " \t"))
	    c = *p, *p = 0;
	if (addr)
	    (void) strcpy(addr, str);
	if (p)
	    *p = c;
	return p? p : (char *) str + strlen(str);
    }

    /* comma terminated before any comment stuff.  If so, check for whitespace
     * before-hand cuz it's possible that strings aren't comma separated yet
     * and they need to be.
     *
     * address address address, address
     *                        ^p  <- p points here.
     *        ^p2 <- should point here.
     */
    if (*p == ',') {
	c = *p, *p = 0;
	if (p2 = any(str, " \t"))
	    *p = ',', c = *p2, p = p2, *p = 0;
	if (addr)
	    (void) strcpy(addr, str);
	*p = c;
	return p;
    }

    /* starting to get hairy -- we found an angle bracket. This means that
     * everything outside of those brackets are comments until we find that
     * all important comma.  A comment AFTER the <addr> :
     *  <address> John Doe
     * can't call this function recursively or it'll think that "John Doe"
     * is a string with two legal address on it (each name being an address).
     *
     * Bart: Wed Sep  9 16:42:48 PDT 1992 -- CRAY_CUSTOM
     * Having the comment after the address is incorrect RFC822 syntax anyway.
     * Just to be forgiving, fudge it so that we accept a comment after an <>
     * address if and only if the comment doesn't contain any 822 specials.
     * Otherwise, treat it as another address (which is also more forgiving
     * than strict RFC822, but maintains a little backward compatibility).
     */
    if (*p == '<') { /* note that "str" still points to comment stuff! */
	angle = 1;
	if (name && *str) {
	    *p = 0;
	    name += Strcpy(name, str);
	    *p = '<';
	}
	if (!(p2 = index(p+1, '>'))) {
	    return NULL;
	}
	if (addr) {
	    /* to support <addr (comment)> style addresses, add code here */
	    *p2 = 0;
	    skipspaces(1);
	    addr += Strcpy(addr, p);
	    while (addr > beg_addr && isspace(*(addr-1)))
		*--addr = 0;
	    *p2 = '>';
	}
	/* take care of the case "... <addr> com (ment)" */
	{
	    int p_cnt = 0; /* parenthesis counter */
	    int inq = 0; /* Quoted-string indicator */
	    char *orig_name = name;

	    p = p2;
	    /* don't recurse yet -- scan till null, comma or '<'(add to name) */
	    for (p = p2; p[1] && (p_cnt || p[1] != ',' && p[1] != '<'); p++) {
		if (p[1] == '(')
		    p_cnt++;
		else if (p[1] == ')')
		    p_cnt--;
		else if (p[1] == '"' && p[0] != '\\')
		    inq = !inq;
		else if (!inq && !p_cnt && p[0] != '\\' &&
			index(specials, p[1])) {
		    if (orig_name)
			*(name = orig_name) = 0;
		    return p2 + 1;
		}
		if (name)
		    *name++ = p[1];
	    }
	    if (p_cnt) {
		return NULL;
	    }
	}
	if (name && name > beg_name) {
	    while (isspace(*(name-1)))
		--name;
	    *name = 0;
	}
    }

    /* this is the worst -- now we have parentheses/quotes.  These guys can
     * recurse pretty badly and contain commas within them.
     */
    if (*p == '(' || *p == '"') {
	char *start = p;
	int comment = 1;
	c = *p;
	/* "str" points to address while p points to comments */
	if (addr && *str) {
	    *p = 0;
	    while (isspace(*str))
		str++;
	    addr += Strcpy(addr, str);
	    while (addr > beg_addr && isspace(*(addr-1)))
		*--addr = 0;
	    *p = c;
	}
	while (comment) {
	    if (c == '"' && !(p = index(p+1, '"')) ||
		    c == '(' /*)*/ && !(p = any(p+1, "()"))) {
		return NULL;
	    }
	    if (p[-1] != '\\') {
		if (*p == '(')	/* loop again on parenthesis. */
		    comment++;
		else		/* quote or close paren may end loop */
		    comment--;
	    }
	}
	/* Hack to handle "address"@domain */
	if (c == '"' && *(p + 1) == '@') {
	    c = *++p; *p = 0;
	    addr += Strcpy(addr, start);
	    *p-- = c;
	} else if ((p2 = any(p+1, "<,")) && *p2 == '<') {
	    /* Something like ``Comment (Comment) <addr>''.  In this case
	     * the name should include both comment parts with the
	     * parenthesis.   We have to redo addr.
	     */
	    angle = 1;
	    if (!(p = index(p2, '>'))) {
		return NULL;
	    }
	    if (addr = beg_addr) { /* reassign addr and compare to null */
		c = *p; *p = 0;
		addr += Strcpy(addr, p2+1);
		while (addr > beg_addr && isspace(*(addr-1)))
		    *--addr = 0;
		*p = c;
	    }
	    if (name) {
		c = *p2; *p2 = 0;
		name += Strcpy(name, str);
		while (name > beg_name && isspace(*(name-1)))
		    *--name = 0;
		*p2 = c;
	    }
	} else if (name && start[1]) {
	    c = *p, *p = 0; /* c may be ')' instead of '(' now */
	    name += Strcpy(name, start+1);
	    while (name > beg_name && isspace(*(name-1)))
		*--name = 0;
	    *p = c;
	}
    }
    p2 = ++p;	/* Bart: Fri May 14 19:11:33 PDT 1993 */
    skipspaces(0);
    /* this is so common, save time by returning now */
    if (!*p)
	return p;
    /* Bart: Fri May 14 19:11:40 PDT 1993
     * If we hit an RFC822 special here, we've already scanned one
     * address and are looking at another.  Since we're supposed to
     * return the end of the address just parsed, back up.  See the
     * CRAY_CUSTOM comment above for rationales.
     *
     * pf Tue Aug 10 16:51:06 1993
     * Only do this if we actually read a '<'.  (xxx"foobar"@zen doesn't
     * work otherwise)
     */
    if (*p == '[' || angle && index(specials, *p))
	return p2;
    return get_name_n_addr(p, name, addr);
}

#else /* !STANDALONE */

/* This function calls loadmail, after first forking, releasing stdin,
 * stdout and stderr and ignoring any signals that might be generated.
 *
 * The above behavior is now controlled by the POP_BACKGROUND item in
 * the flags parameter.  If it is false, the check is carried on in the
 * foreground and can be interrupted (this may be undesirable).
 */
void 
popchkmail(flags, poplogin, poppword)
int flags;
char *poplogin, *poppword;
{
#ifdef POP3_FORK_OK
    static int cpid = 0, numtries = 0;

    if ((flags & POP_BACKGROUND) == 0) {
	loadmail(TRUE, flags & POP_PRESERVE, poplogin, poppword);
	return;
    }

    if (cpid > 0) {
	if (!kill(cpid, 0)) {
	    numtries++;
	    if (numtries > 10) {
		kill(cpid, SIGKILL);
	    }
	    return;
	} else {
	    numtries = 0;
	}
    }
    if (!(cpid = fork())) {
	/* Ignore several signals for workability */
	signal(SIGCONT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
	signal(SIGTSTP, SIG_IGN);
	signal(SIGSTOP, SIG_IGN);
	signal(SIGTERM, SIG_IGN);
	signal(SIGIO, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
	loadmail(FALSE, flags & POP_PRESERVE, poplogin, poppword);
	_exit(0);
    } else {
	if (cpid == -1) {
#ifdef NOT_ZMAIL
	    perror(catgets( catalog, CAT_CUSTOM, 41, "Unable to fork in popchkmail\n" ));
#else /* !NOT_ZMAIL */
	    error(SysErrWarning, catgets( catalog, CAT_CUSTOM, 41, "Unable to fork in popchkmail\n" ));
#endif /* !NOT_ZMAIL */
	}
	return;
    }
#else /* !POP3_FORK_OK */
#ifdef MAC_OS
    gui_mailcheck_feedback(TRUE);
#endif
    loadmail(TRUE, flags & POP_PRESERVE, poplogin, poppword);
#ifdef MAC_OS
    gui_mailcheck_feedback(FALSE);
#endif
#endif /* POP3_FORK_OK */
}

#endif	/* !STANDALONE */

#endif	/* POP3_SUPPORT */
