/* version.c    Copyright 1991-95 Z-Code Software Corp. */

#include "catalog.h"

#ifndef _ifdef_sh_
#include "config/features.h"
# include "version.h"
# include "zmstring.h"
#endif /* _ifdef_sh_ */

#ifndef lint
static char     version_rcsid[] =
    "$Id: version.c,v 2.400 2005/05/09 09:15:23 syd Exp $";
#endif

#ifdef _WINDOWS
#define COPYRIGHT \
"Copyright \251 1996-1998 NetManage, Inc.  All rights reserved."
#else
#define COPYRIGHT \
"Copyright 2005 OpenZMail.org. All rights reserved."
#endif

/* The following comment is parsed by "doc.sh" to determine the version
 * numbers for each of lite, motif, and olit.  Please keep it up to
 * date and in the same format.  Patchlevel is not significant.  --bg
 * [Fri Jul 16 16:14:47 1993]
 *
 * The numbering convention is:
 *    Major number
 *    Dot
 *    Minor number plus optional "a" or "b" (alpha/beta)
 *    Dot
 *    If alpha or beta
 *        Revision number (month and day as mmdd, counting from the
 *            introduction of the minor number).
 *    Else
 *        Patch release number
 */

/*
 * motif version <5.0>
 * lite version <5.0>
 */

#ifdef VUI
# define ZM_NAME                "OpenZMail Classic Lite"
# define RELEASE_DATE           "24April2005"
# define RELEASE                0
# define REVISION               "9"
# define PATCHLEVEL             2
# define LICENSING_VERSION      "0.9"
#else /* !VUI */
# if defined(ZMAIL_BASIC) && !defined(MEDIAMAIL)
#  define ZM_NAME               "Z-Mail_Basic"
# else /* !ZMAIL_BASIC */
# ifdef SGI_CUSTOM
#  define ZM_NAME               "Z-Mail-SGI"
# else /* !SGI_CUSTOM */
#  define ZM_NAME               "OpenZMail Classic"		/* Ordinary Z-Mail */
# endif /* SGI_CUSTOM */
# endif /* !ZMAIL_BASIC */

# ifdef CRAY_CUSTOM
#  define RELEASE_DATE          "28nov93 Cray"
# else
#  ifdef MEDIAMAIL
#   define RELEASE_DATE         "13Jan97 MediaMail"
#  else /* !MEDIAMAIL */
#    define RELEASE_DATE        "24April2005"		/* Ordinary Z-Mail */
#  endif /* !MEDIAMAIL */
# endif
# define RELEASE                0
# ifdef SGI_CUSTOM
#  define REVISION              "0S"
#  define LICENSING_VERSION     "5.0S"
# else
#  ifdef MEDIAMAIL
#   define REVISION             "0"
#  else /* !MEDIAMAIL */
#   define REVISION             "9"			/* Ordinary Z-Mail */
#  endif /* !MEDIAMAIL */
#  define LICENSING_VERSION     "0.9"			/* Ordinary Z-Mail */
# endif /* !SGI_CUSTOM */
# ifdef MEDIAMAIL
#  define PATCHLEVEL            1
# else /* !MEDIAMAIL */
#  define PATCHLEVEL            2			/* Ordinary Z-Mail */
# endif /* MEDIAMAIL */
# ifdef MAC_OS
#  undef ZM_NAME
#  undef RELEASE_DATE
#  undef REVISION
#  undef PATCHLEVEL
#  define ZM_NAME               "ZM-Mac"
#  define RELEASE_DATE          "15Mar95"
#  define RELEASE               3
#  define REVISION              "2"
#  define PATCHLEVEL            1
# endif /* MAC_OS */
# ifdef _WINDOWS
#  undef ZM_NAME
#  undef RELEASE_DATE
#  undef RELEASE
#  undef REVISION
#  undef PATCHLEVEL
#  define ZM_NAME               "ZM-Win"
#  define ZM_PRODUCTNAME  VER_FILEDESCRIPTION_STR
#  define RELEASE_DATE  __DATE__ " - " __TIME__
#  define RELEASE               ZMWIN_MAJOR_STR
#  define REVISION      ZMWIN_MINOR_STR
#  define PATCHLEVEL    ZMWIN_PATCH_STR
# endif /* _WINDOWS */
#endif /* VUI */

char *
zmCopyright()
{
    return COPYRIGHT;
}

char *
zmName()
{
    return ZM_NAME;
}

/* Allow "Z-Mail Lite", for instance, to look like "Z-Mail" to the NLS */
char *
zmMainName()
{
    static char buf[sizeof(ZM_NAME)];

    if (!*buf) {
	    char *p;

	    strcpy(buf, ZM_NAME);
	    if (p = any(buf, " \t")) {
	        *p = '\0';
	    }
    }
    return (buf);
}

char *
zmVersion(full)
int full;
{
    static char maj_min_pat[64];
    static char maj_min_only[64];

    if (full == 2) {	/* Hack for licensing */
#ifdef _WINDOWS
	full = 0;
#else /* !_WINDOWS */
	return LICENSING_VERSION;
#endif /* !_WINDOWS */
    }
    if (!maj_min_only[0])
	(void) sprintf(maj_min_only, "%d.%s", RELEASE, REVISION);
    if (full && !maj_min_pat[0])
	(void) sprintf(maj_min_pat, "%s.%d", maj_min_only, PATCHLEVEL);

    return full? maj_min_pat : maj_min_only;
}

char *
zmRelease()
{
    return RELEASE_DATE;
}
