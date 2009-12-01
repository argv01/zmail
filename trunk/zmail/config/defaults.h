/* defaults.h	Copyright 1992 Z-Code Software Corp. */

/* Default sizes and other constants */
/* RJL ** 4.13.93 - special ifdef MSC HDRSIZ removed */
/* should not be < BUFSIZ! (but can be >) */
#define HDRSIZ  min(40*BUFSIZ,40*1024)

/* Default names and locations for files */
#define DEFAULT_LIB	unhidestr(DEFAULT_LIB_HIDDEN)
#ifdef MSDOS
#define DEFAULT_LIB_HIDDEN 	/* "\ZMAIL\LIB" */ \
		'\\','Z','M','A','I','L','\\','L','I','B',0
#define ZMDOS_LIBDIR "LIB"	 /* name of default lib subdirectory */
/* **RJL 12.14.92 - BIN_CAT was #syntax error# - redefined to "type" */
#define BIN_CAT		"type"
#define MAILRC		"zmail.rc"
#define ALTERNATE_RC	"mush.rc"
#define DEFAULT_RC	"system.rc"
#define ALT_DEF_RC	"\\usr\\lib\\Mushrc"
#define SIGNATURE	"signatur.txt"
#define FORTUNE		"\\usr\\games\\fortune"	/* Probably not there */
#define SPELL_CHECKER	"\\usr\\bin\\spell"	/* Probably not there */
#define FORM_TEMPL_DIR  "forms"
#define VARIABLES_FILE	"variable"
#define ENCODINGS_FILE	"attach.typ"
#define COMMAND_HELP	"command.hlp"
#ifdef ZMAIL_INTL
#define FUNCTION_HELP	"function.hlp"
#endif /* ZMAIL_INTL */
#ifdef GUI
# define COLORS_FILE	"~\\colors.zm"
# define FONTS_FILE	"~\\fonts.zm"
# ifdef MOTIF
#  define TOOL_HELP	"motif.hlp"
# else /* MOTIF */
#  ifdef VUI
#   define TOOL_HELP     "lite.hlp"
#  else /* VUI */
#   define TOOL_HELP	"tool.hlp"
#  endif /* VUI */
# endif /* MOTIF */
#endif /* GUI */
#define ALTERNATE_HOME	"\\tmp"  /* Path must be read/write to EVERYONE */
#define EDFILE  	"ed"    /* file/pathname added to user's "home" */

#define LS_COMMAND	"dir"
#define LPR		"print"

#define DEF_DEAD	"~\\dead.ltr"	/* default dead.letter */
#define DEF_MBOX	"~\\mbox"	/* default mbox */
#define DEF_FOLDER	"~\\Mail"        /* default Mail folder */

#else /* ! MSDOS */
#ifdef MAC_OS
/* How do we refer to the root?   Is there a root? */
#define DEFAULT_LIB_HIDDEN 	/* "usr:lib:Zmail" */ \
		'u','s','r',':','l','i','b',':','Z','m','a','i','l',0
#define BIN_CAT		#syntaxerror#
#define MAILRC		"zmail.rc"
#define ALTERNATE_RC	"mush.rc"
#define DEFAULT_RC	"system.zrc"
#define ALT_DEF_RC	"usr:lib:Mushrc"
#define SIGNATURE	"signatur.e"
#define FORTUNE		"usr:games:fortune"	/* Probably not there */
#define SPELL_CHECKER	"usr:bin:spell"		/* Probably not there */
#define FORM_TEMPL_DIR  "forms"
#define VARIABLES_FILE	"variable"
#define ENCODINGS_FILE	"attach.typ"
#define COMMAND_HELP	"command.hlp"
#ifdef ZMAIL_INTL
#define FUNCTION_HELP	"function.hlp"
#endif /* ZMAIL_INTL */
#ifdef GUI
# define COLORS_FILE	"~:.zmcolors"
# define FONTS_FILE	"~:.zmfonts"
# ifdef MOTIF
#  define TOOL_HELP	"motif.hlp"
# else /* MOTIF */
#  ifdef VUI
#   define TOOL_HELP     "lite.hlp"
#  else /* VUI */
#   define TOOL_HELP	"tool.hlp"
#  endif /* VUI */
# endif /* MOTIF */
#endif /* GUI */
#define ALTERNATE_HOME	":tmp"  /* Path must be read/write to EVERYONE */
#define EDFILE  	"ed"    /* file/pathname added to user's "home" */

#define LS_COMMAND	"dir"
#define LPR		"print"

#define DEF_DEAD	"~:dead.ltr"	/* default dead.letter */
#define DEF_MBOX	"~:mbox"	/* default mbox */
#define DEF_FOLDER	"~:Mail"        /* default Mail folder */

#else /* ! MAC_OS */
#if defined(AIX) || defined(AIX322) || defined(AIX4)
#define DEFAULT_LIB_HIDDEN	/* "/usr/lpp/Zmail" */ \
		'/','u','s','r','/','l','p','p','/','Z','m','a','i','l',0
#else /* normal */
#define DEFAULT_LIB_HIDDEN 	/* "/usr/lib/Zmail" */ \
		'/','u','s','r','/','l','i','b','/','Z','m','a','i','l',0
#endif /* AIX */
#define BIN_CAT		"/bin/cat"
#define MAILRC		".zmailrc"
#define ALTERNATE_RC	".mushrc"
#define DEFAULT_RC	"system.zmailrc"
#define ALT_DEF_RC	"/usr/lib/Mushrc"
#define SIGNATURE	".signature"
#define FORTUNE		"/usr/games/fortune"
#define SPELL_CHECKER	"/usr/bin/spell"
#define FORM_TEMPL_DIR  "forms"
#define VARIABLES_FILE	"variables"
#define ENCODINGS_FILE	"attach.types"
#define COMMAND_HELP	"command.hlp"
#ifdef ZMAIL_INTL
#define FUNCTION_HELP	"function.hlp"
#endif /* ZMAIL_INTL */
#ifdef GUI
# define COLORS_FILE	"~/.zmcolors"
# define FONTS_FILE	"~/.zmfonts"
# ifdef MOTIF
#  define TOOL_HELP	"motif.hlp"
# else
#  ifdef VUI
#   define TOOL_HELP	"lite.hlp"
#  else /* VUI */
#   define TOOL_HELP	"openlook.hlp"
#  endif /* VUI */
# endif /* MOTIF */
#endif /* GUI */
#define ALTERNATE_HOME	"/tmp"  /* Path must be read/write to EVERYONE */
#define EDFILE  	"ed"    /* file/pathname added to user's "home" */

#define LS_COMMAND	"ls"
#ifdef BSD
#define LPR		"lpr"
#else /* !BSD */
#define LPR		"lp"
#endif /* BSD */

#define DEF_DEAD	"~/dead.letter"	/* default dead.letter */
#define DEF_MBOX	"~/mbox"	/* default mbox */
#define DEF_FOLDER	"~/Mail"        /* default Mail folder */

#endif /* MAC_OS */
#endif /* MSDOS */

/* default settings for some variable strings */
#define DEF_PROMPT	"Msg %m of %t: "
#define DEF_PAGER	"more" /* set to "internal" to use internal pager */
#ifdef GUI
# define DEF_WIN_SHELL	"xterm -e"
#endif /* GUI */
#define DEF_SHELL	"csh"
#define DEF_EDITOR	"vi"
#define DEF_IXDIR	"index.dir"     /* default folder index subdir */
#define DEF_DETACH	"detach.dir"    /* default attachment detach subdir */
#define DEF_INDENT_STR	"> "		/* indent included mail */
#define DEF_PRINTER	"lp"
#define DEF_ESCAPE	"~"
#define DEF_HDR_FMT	"%22n %M %-2N %T (%l) %.23s" /* default hdr_format */
#define DEF_CURSES_HELP	\
    "display save mail reply next-msg back-msg screen-next screen-back"

/* Headers that will NOT be included when forwarding mail */
#define IGNORE_ON_FWD	\
"status,priority,precedence,x-zm-priority,\
return-receipt-to,x-replied,x-forwarded,\
x-key-digest"

/* Maximum size in bytes of an attachment that we show in-line */
#define MAX_ATTACH_SHOW	(10 * 1024)
