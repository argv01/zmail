/* features.h	Copyright 1991 Z-Code Software Corp. */

#ifndef _ZFEATURES_H
#define _ZFEATURES_H

/**************************************************************************
 *
 * Z-Mail Features
 *
 **************************************************************************/

#include "osconfig.h"

#ifdef SGI_CUSTOM
# ifndef MEDIAMAIL
#  define MEDIAMAIL
# endif
#else
# if defined(MEDIAMAIL) && !defined(ZMAIL_BASIC)
#  define ZMAIL_BASIC
# endif /* MEDIAMAIL && !ZMAIL_BASIC */
#endif /* SGI_CUSTOM */

/* #define GUI		/* Supply X Windows GUI interface */
/* #undef  VUI		/* Supply "Lite" Curses-based GUI interface */
/* #define MOTIF		/* X Windows interface uses Motif toolkit */
/* #undef  OLIT		/* X Windows interface uses olit (OpenLook) toolkit */
#define FONTS_DIALOG	/* Font editing dialog */
#if !defined(MEDIAMAIL) || defined(SGI_CUSTOM)
#define PAINTER		/* Color editing dialog */
#endif /* !MEDIAMAIL || SGI_CUSTOM */
#ifndef MEDIAMAIL
#define CREATE_ATTACH	/* Create and attach new files using attach.types rules */
#endif /* !MEDIAMAIL */
#ifndef ZMAIL_BASIC
#define DSERV		/* Include directory services */
#define INTERPOSERS	/* Include interposers */
#define FILTERS_DIALOG	/* Include filters dialog */
#define FUNC_DIALOG	/* Include functions dialog */
#define DYNAMIC_HEADERS /* Include dynamic headers dialog */
#ifdef NOT_NOW
#define FOLDER_DIALOGS 	/* Include folder manager, open folders dialog */
#endif
#define SORT_DIALOG	/* Include sort dialog */
#define MENUS_DIALOG	/* Includes buttons and menus dialog */
#endif /* !ZMAIL_BASIC */
#ifndef ZMAIL_BASIC
#define FUNCTIONS_HELP	/* Include functions help dialog */
#endif /* !ZMAIL_BASIC */
#define FILTERS		/* Include filters */
#undef  AUDIO		/* Sound FX */
#define SPELLER		/* Enable spell checker in text search dialog */
/* #define SELECT_POS_LIST	/* Supply enhanced Motif list widget functions */
#define GUI_INTRPT	/* The gui-version of the interrupt notifier */
#undef  USE_XV_NOTICE	/* Use the xv_notice program for error and ask */
#undef  USE_ASK_NO_ECHO	/* use the no-echo option for zm_ask() [broken] */
#undef  RECORD_ACTIONS  /* event/action record/playback */
#undef  SCRIPT		/* Include buttons and functions dialog */

#if defined(MOTIF) || !defined(GUI)
#define TIMER_API
#endif /* MOTIF || !GUI */

#if defined(TIMER_API) && defined(FAM_OPEN)
#define USE_FAM		/* Use File Alteration Monitor instead of polling */
#endif /* TIMER_API && FAM_OPEN */

#if !defined(MAC_OS) && !defined (_WINDOWS)
#define PARTIAL_SEND		/* outbound partials; requires Unix filtering */
#undef PARTIAL_SEND_DIALOG	/* custom prompting dialog */
#endif /* Unix */

#define ZM_CHILD_MANAGER/* Improved child process handling */
#undef  ZM_JOB_CONTROL	/* Let Z-Mail job-control composition editors */
#undef  USA_TIMEZONES	/* Use USA timezone abbreviations */
#define USE_ZLOCK	/* Replacement for flock(2) using fcntl(2) */

#undef  MSTATS		/* Generate malloc statistics in INTERNAL_MALLOC */
#undef  RCHECK		/* Do range (overflow) checks in INTERNAL_MALLOC */
#undef  MALLOC_TOUCHY	/* Die gruesomely if malloc gets errors */
#undef  MALLOC_UTIL	/* Compile in extensive malloc/free tracing */
#undef  STACKTRACE	/* Compile in stack tracing (not all systems) */
#undef  MALLOC_TRACE	/* Trace malloc/free via stacktrace routines */

#ifndef REGEXPR
# define REGEXPR		/* Use extended regular expression library */
#endif /* REGEXPR */

#ifdef ZMAIL_INTL
# define C3			/* Use C3 character-set translation */
#endif /* ZMAIL_INTL */

/**************************************************************************
 *
 * License Server Stuff
 *
 **************************************************************************/

#ifndef NETWORK_LICENSE
#define NETWORK_LICENSE	/* Allow connections to ZC Network License Server */
#endif

#ifndef FREEWARE
#undef  LICENSE_FREE	/* No license server is used */
#endif

#undef  DEMO_YEAR	/* 1993 -- Time bomb for demo copies */

/**************************************************************************
 *
 * Miscellaneous Stuff
 *
 **************************************************************************/

#define MSG_HEADER_CACHE /* new way to cache message headers; includes RFC-1522 */
#define FAILSAFE_LOCKS	/* Remove stray lockfiles when detected */
#undef  DEBUG		/* Compile in additional debugging */
#define NDEBUG		/* Ignore assert statements */
#undef  ZMVADEBUG	/* Debug the zmVaStr() utility */
#undef  OLD_BEHAVIOR	/* Revert to older versions for some features */
#undef  OLDEBUG		/* Compile in additonal OpenLook debugging */
#undef  OLIT_MENU_FIX	/* Attempt to repair olit menu mismanagement */

#endif /* _ZFEATURES_H */
/* DON'T ADD STUFF AFTER THIS #endif */

