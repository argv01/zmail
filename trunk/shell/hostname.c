/*
 * hostname.c
 *
 * Contains the single function:
 *	zm_gethostname()
 * which returns static data that remains valid forever.
 */

#ifndef lint
static char	hostname_rcsid[] =
    "$Id: hostname.c,v 2.15 1995/09/06 03:54:58 liblit Exp $";
#endif

#include <general.h>
#include <zmstring.h>
#include "hostname.h"
#include "zcunix.h"	/* for getenv() */
#include "zcerr.h"	/* for errno */

#ifndef MAC_OS
#include <pwd.h>
#endif /* !MAC_OS */

#if defined(HAVE_GETHOSTNAME) && !defined(_WINDOWS)
#include <netdb.h>
#endif /* HAVE_GETHOSTNAME && !_WINDOWS */

#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif /* HAVE_UTSNAME */

#ifndef savestr
static char *__;        /* to avoid mentioning the argument of a macro twice */
#define savestr(s)  (__ = s, __ || (__ = ""), strcpy((char *)malloc(strlen(__)+1),__))
#endif /* !savestr */


extern char *
zm_gethostname()
{
#ifndef MSDOS
#if defined(HAVE_UTSNAME) && !defined(HAVE_GETHOSTNAME)
    struct utsname ourhost;
#else
    char ourhost[128];
#endif /* HAVE_UTSNAME && !HAVE_GETHOSTNAME */
#endif /* !MSDOS */

    static char *hostname;
    if (hostname)
	return hostname;
#if defined(MSDOS)
    hostname = savestr(getenv("ZMHOST"));
#else /* !MSDOS */
#ifdef HAVE_GETHOSTNAME
#ifdef MAC_OS
    if (mac_checktcp(FALSE)) {
#endif /* MAC_OS */
    (void) gethostname(ourhost, sizeof ourhost);
    hostname = savestr(ourhost);
#ifdef MAC_OS
    } else {
	hostname = NULL;
    }
#endif /* MAC_OS */
#else /* !HAVE_GETHOSTNAME */
#ifdef HAVE_UNAME
	if (uname(&ourhost) >= 0 && *ourhost.nodename)
	    hostname = savestr(ourhost.nodename);
	else
#endif /* HAVE_UNAME */
	{
	    /* Try to use uuname -l to get host's name if uname didn't work */
	    char buff[80];
	    char *p;
	    FILE *F;

#ifdef HAVE_UUNAME
	    (void) strcpy(buff, "exec uuname -l");
#else /* !HAVE_UUNAME */
#ifdef HOSTFILE
	    (void) sprintf(buff, "exec cat %s", HOSTFILE);
#endif /* HOSTFILE */
#endif /* !HAVE_UUNAME */
	    if (F = popen(buff, "r")) {
		do {	/* Don't fail on SIGCHLD */
		    if ((fgets(buff, sizeof buff, F) == buff) &&
			    (p = index(buff, '\n'))) {
			*p = '\0';		/* eliminate newline */
			hostname = savestr (buff);
		    }
		} while (errno == EINTR && !feof(F));
		(void)pclose(F);
	    }
	}
#endif /* !HAVE_GETHOSTNAME */
#endif /* MSDOS */
#ifdef MAC_OS
    if (!hostname || !*hostname)
	hostname = savestr(getenv("ZMHOST"));
#endif /* MAC_OS */

    return hostname;
}
