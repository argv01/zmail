/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 *
 * static char copyright[] = 
 * "Copyright (c) 1990 Regents of the University of California.\n\
 * All rights reserved.\n";
 * static char SccsId[] = "@(#)@(#)popper.h	2.2  2.2 4/2/91";
 *
 */

/*  LINTLIBRARY */

#include "osconfig.h"
#include "config.h"
#include <stdio.h>

/* 
 *  Header file for the POP programs
 */

#include <sys/param.h>
#include <syslog.h>
#include "zctime.h"
#include <sys/types.h>
#ifdef HAVE_STRINGS_H
# include <strings.h>
#endif /* HAVE_STRINGS_H */
#ifdef HAVE_STRING_H
# include <string.h>
#endif /* HAVE_STRING_H */
#include <glist.h>   /* gets general.h, too! */
#include <dynstr.h>

#include "version.h"
#include "plist.h"

#include <mstore/mfolder.h>	/* for struct mailhash */

#include "zcsyssel.h" /* for sys/select.h where appropriate */

#ifdef REALLOC_NULL_OK
# define reallocate(pointer, size)  (realloc((pointer), (size)))
#else
# define reallocate(pointer, size)  ((pointer) ? realloc((pointer), (size)) : malloc(size))
#endif /* REALLOC_NULL_OK */

/*
 * whatever headers define MAXHOSTNAMELEN etc. *must* be included before
 * the constant definitions below, otherwise really nasty stuff starts
 * happening in the POP struct where one .c file thinks the host char[]
 * is 256 chars and another thinks it is only 64 chars
 */
#include <netdb.h> /* for MAXHOSTNAMELEN on Motorola R40 */
#include <sys/socket.h> /* for MAXHOSTNAMELNE on SCO, believe it or not */

#ifndef MAXPATHLEN
# define MAXPATHLEN	256
#endif /* !MAXPATHLEN */

#ifndef MAXHOSTNAMELEN
# define MAXHOSTNAMELEN  256
#endif /* !MAXHOSTNAMELEN */

#ifndef WEXITSTATUS
# define WEXITSTATUS(stat)  ((stat).w_retcode)
#endif /* WEXITSTATUS */

#ifdef USE_UNION_WAIT
typedef union wait wait_status;
#else /* USE_UNION_WAIT */
typedef int wait_status;
#endif /* USE_UNION_WAIT */

#include "zcerr.h"

#ifdef HAVE_LOCKF
# define ZLOCKF(fd) (lockf((fd), F_LOCK, 0))
# define ZLOCKF_NB(fd) (lockf((fd), F_TLOCK, 0))
# define ZLOCKF_WOULDBLOCK EAGAIN
# define ZUNLOCKF(fd) (lockf((fd), F_ULOCK, 0))
#else /* !HAVE_LOCKF */
# ifdef HAVE_FLOCK
#  define ZLOCKF(fd) (flock((fd), LOCK_EX))
#  define ZLOCKF_NB(fd) (flock((fd), LOCK_EX|LOCK_NB))
#  define ZLOCKF_WOULDBLOCK EWOULDBLOCK
#  define ZUNLOCKF(fd) (flock((fd), LOCK_UN))
# else /* !HAVE_FLOCK */
#  include "platform has no lockf or flock"
# endif /* HAVE_FLOCK */
#endif /* HAVE_LOCKF */

#ifndef HAVE_STRDUP
extern char *strdup();
#endif /* HAVE_STRDUP */

#define DEFTMPDIR	"/tmp"
#define DEFAULT_NEWMSGPATH  "/tmp"

#ifdef D4I
# define DEFAULT_PREFPATH    "/usr/local/etc/delpoplib/prefs"
# define DEFAULT_LIBPATH	    "/usr/local/etc/delpoplib/lib"
# define DEFAULT_UIDPATH     "/usr/local/etc/delpoplib/uid-record"
# define DEFAULT_CONFIG "/usr/local/etc/delpoplib/delpop.config"
#else /* D4I */
# define DEFAULT_PREFPATH    "/usr/local/etc/zpoplib/prefs"
# define DEFAULT_LIBPATH	    "/usr/local/etc/zpoplib/lib"
# define DEFAULT_UIDPATH     "/usr/local/etc/zpoplib/uid-record"
# define DEFAULT_CONFIG "/usr/local/etc/zpoplib/zpop.config"
#endif /* D4I */

#define DEFAULT_IDLE_TIMEOUT 600
#define DEFAULT_LIBRARY_STRUCTURE "PRODUCT PLATFORM VERSION"

#define NO_SEQNO -1

#define NULLCP          ((char *) 0)
#define SPACE           32
#define TAB             9
#define TRUE            1
#define FALSE           0

#define MAXUSERNAMELEN  65
#define MAXDROPLEN      64
#define MAXLINELEN      1024
#define MAXMSGLINELEN   1024
#define MAXCMDLEN       4
#define MAXPARMCOUNT    5
#define ALLOC_MSGS  20
#define MAIL_COMMAND    "/usr/lib/sendmail"

#define POP_FACILITY    LOG_LOCAL0
#define POP_PRIORITY    LOG_NOTICE
#define POP_DEBUG       LOG_DEBUG
#define POP_LOGOPTS     0
#ifdef USE_SPOOL_MAIL
# define POP_MAILDIR	"/usr/spool/mail"
#else /* !USE_SPOOL_MAIL */
# define POP_MAILDIR	"/usr/mail"
#endif /* USE_SPOOL_MAIL */
/* POP_TMPSIZE needs to be big enough to hold the string
	 * defined by POP_TMPDROP.  POP_DROP and POP_TMPDROP
	 * must be in the same filesystem.
	 */
/* GF 9.30.93 -- convert POP_TMPDROP from the full path & file name
 * to just the file name;  mod code to construct path from POP_MAILDIR
 */
#define POP_TMPSIZE	256
#define POP_TMPXMIT     "/tmp/xmitXXXXXX"
#define POP_OK          "+OK"
#define POP_ERR         "-ERR"
#define POP_ABORT       2
#define POP_SUCCESS     1
#define POP_FAILURE     0

#define CLIENT_PROPS_GROWSIZE 10

extern int errno;

#define pop_command         pop_parm[0]     /*  POP command is first token */
#define pop_subcommand      pop_parm[1]     /*  POP XTND subcommand is the 
                                                second token */

typedef enum {			/*  POP processing states */
    auth1,			/*  Authorization: waiting for USER command */
    auth2,			/*  Authorization: waiting for PASS command */
    pretrans,			/*  Like trans, but spool isn't copied or
				    parsed */
    trans,			/*  Transaction */
    down,			/*  ready to download updated files */
    halt,			/*  (Halt):  stop processing and exit */
    error			/*  (Error): something really bad happened */
} pop_state;


typedef struct {                 /* State information for each POP command */
    pop_state ValidCurrentState; /* The operating state of the command */
    char *command;               /* The POP command */
    int min_parms;               /* Minimum number of parms for the command */
    int max_parms;               /* Maximum number of parms for the command */
    int (*function) ();          /* The function that processes the command */
    pop_state result[2];         /* The resulting state after cmd processing */
} state_table;

#define success_state result[POP_SUCCESS] /*  State when a command succeeds */


typedef struct {         /*  Table of extensions */
    char *subcommand;      /*  The POP XTND subcommand */
    int min_parms;         /*  Minimum number of parms for the subcommand */
    int max_parms;         /*  Maximum number of parms for the subcommand */
    int (*function) ();    /*  The function that processes the subcommand */
} xtnd_table;


struct msg_info {
    int number;			/* Message number */
    off_t header_offset;	/* Offset of RFC 822 message headers */
    off_t header_length;	/* Length in bytes of message header */
    off_t body_offset;		/* Offset of message body */
    off_t body_length;		/* Length in bytes of message body */
    off_t header_adjust;	/* Number of bytes silently eaten, e.g. `>' */
    struct mailhash key_hash, header_hash;
    unsigned long status, date;
    char *summary, *unique_id;
    struct dynstr from_line;	/* The "From " line, for ZFRL */
    int have_key_hash:1, have_header_hash:1, have_date:1, had_from_line:1;
};

struct number_list {
    int list_count;
    int *list_numbers;
};

typedef struct {
  char *name;                /* Name of this node */
  struct glist *file_list;   /* sorted glist of files available at this node 
			        (including "inherited" files) */
  struct glist *child_list;  /* sorted glist of children nodes */
} downloadNode;


typedef struct {
    char *name;	             /* Name of file to download */
    char *fullname;            /* Pathname of file to download */
    char *version_str;	     /* Version string */
    struct version version;    /* Parsed version string */
    int seqno;                 /* Sequence number available on server */
    off_t	bytes;		     /* Size of file in bytes */
    int outdated;		     /* Flagged for download */
    struct plist *attributes;  /* plist of file attributes */
} downloadFile;


typedef struct {
    dev_t st_dev;           /* Device ID of a downloadNode */
    ino_t st_ino;           /* Inode # of a downloadNode */
} nodeHashKey;


typedef struct {		/* Attribute from file attribute line */
    char *identifier;		/* Attribute identifier */
    char *value;		/* Attribute value */
} file_attribute;

typedef struct {                /*  POP parameter block */
    int debug;			/*  Debugging requested */
    char *myname;		/*  The name of this POP daemon program */
    char myhost[MAXHOSTNAMELEN]; /*  The name of our host computer */
    char *client;		/*  Canonical name of client computer */
    char *ipaddr;		/*  Dotted-notation format of client IP
				    address */
    unsigned short ipport;	/*  Client port for privileged operations */
    char user[MAXUSERNAMELEN];	/*  Name of the POP user */
    char homedir[MAXPATHLEN];	/* user's home dir */
    pop_state CurrentState;	/*  The current POP operational state */
    struct glist minfo;		/* message information list */
    int msgs_deleted;		/*  Number of messages flagged for deletion */
    int last_msg;		/*  Last message touched by the user */
    int orig_last_msg;		/*  Last message read when folder was parsed */
    long bytes_deleted;		/*  Number of maildrop bytes flagged for
				    deletion */
    off_t orig_size;		/* when we first parsed the spool */
    long drop_size;		/*  Size of the maildrop in bytes */
    FILE *drop;			/*  (Temporary) mail drop */
    char dropname[MAXPATHLEN];	/*  The name of the user's maildrop */
    char tmpdropname[MAXPATHLEN];
    FILE *input;		/*  Input TCP/IP communication stream */
    FILE *output;		/*  Output TCP/IP communication stream */
    FILE *trace;		/*  Debugging trace file */
    char *trace_prefix;		/* Prefix for lines in trace file */
    char *pop_parm[MAXPARMCOUNT]; /*  Parse POP parameter list */
    int  parm_count;		/*  Number of parameters in parsed list */
    char raw_command[MAXMSGLINELEN]; /* unparsed command line */
    char pref_path[MAXPATHLEN];	/* 9/7/93 GF  path of prefs */
    struct plist *client_props;	/* plist of client (from ZMOI command) */
    char lib_path[MAXPATHLEN];	/* path of library directory */
    struct glist *lib_structure; /* ordered list of client prop names to use
				    to find position in download tree */
    downloadNode *download_tree; /* tree of downloadable files */
    downloadNode *downloads;	/* relevant node in download tree */
    unsigned outdatedCount;	/* Number of outdated client files */
    unsigned long outdatedBytes; /* Byte count of all outdated client files */
    unsigned idle_timeout;	/* Idle timeout in seconds */
    unsigned additional_timeout; /* To be added to idle_timeout */
    int speed;			/* connection speed (if given in ZMOI) */
    fd_set input_fdset;		/* fdset for select() on input stream */
    struct glist *errGlist;	/* pointer to error glist */
    pid_t pid;			/* zyncd process id */
    char newmsgpath[MAXPATHLEN]; /* path to message upload directory */
    unsigned char spool_format;	/* bit field */
    struct dynstr msg_separator; /* for MMDF */
    char time_stamp[MAXHOSTNAMELEN+25]; /* <PID.TIME@HOSTNAME> for APOP/UIDL */
    int time_stamp_len;
} POP;

#define NTHMSG(p,n) ((struct msg_info *) \
		     glist_Nth(&((p)->minfo),((n)-1)))
#define NUMMSGS(p) (glist_Length(&((p)->minfo)))

extern void do_drop P((POP *));
extern void allow P((POP *, size_t));

extern int  pop_dele();
extern int  pop_last();
extern int  pop_list();
extern int  pop_pass();
extern int  pop_quit();
extern int  pop_rset();
extern int  pop_send();
extern int  pop_stat();
extern int  pop_updt();
extern int  pop_user();
extern int  pop_xtnd();
extern int  pop_xmit();

extern int  zync_vers();
extern int  zync_mbox();
extern int  zync_gprf();
extern int  zync_sprf();
extern int  zync_init();
extern int  zync_msgs();
extern int  zync_get_remote_file();
extern int  zync_who();
extern int  zync_have();
extern int  zync_get();
extern int  zync_zmyp();
extern int  zync_moi();
extern int  zync_uidl();

extern int zync_zpsh();
extern int zync_zpgk();
extern int zync_ztkn();
extern int zync_zshk();
extern int zync_zshq();
extern int zync_zshf();
extern int zync_zmhk();
extern int zync_zmhq();
extern int zync_zmhf();
extern int zync_zfmk();
extern int zync_zfmh();
extern int zync_zfmb();
extern int zync_zfmf();
extern int zync_zuph();
extern int zync_zupb();
extern int zync_zupf();
extern int zync_zhbm();
extern int zync_zhb2();
extern int zync_zdat();
extern int zync_zsiz();
extern int zync_zsts();
extern int zync_zst2();
extern int zync_zsmy();
extern int zync_zsst();
extern int zync_zfrl();
#ifdef D4I
extern int zync_d4ip();
#endif /* D4I */

extern char *zync_fgets();
extern int parse_number_list();
extern int compute_extras();

#define FILE_ID_PROP "FILE-ID"
#define SEQNO_PROP "SEQ-NUM"
#define VERSION_PROP "CLIENT-REV"

#define DEBUG_ERRORS        1 
#define DEBUG_WARNINGS      2
#define DEBUG_COMMANDS      4
#define DEBUG_VERBOSE       8
#define DEBUG_CONNECTION   16
#define DEBUG_CONFIG       32
#define DEBUG_MAILDROP     64
#define DEBUG_LIBRARY     128
#define DEBUG_EXEC        256
#define DEBUG_UPLOAD      512

#ifndef ULBIT
# define ULBIT(bit)     ((u_long)1 << (u_long)(bit))
#endif /* !ULBIT */

/* struct POP.spool_format */
#define MMDF_SEPARATORS	ULBIT(0)
#define CONTENT_LENGTH 	ULBIT(1)

#define DEFAULT_DEBUG_KEYS "WARNINGS"

#define ATTR_PLIST_GROWSIZE 5

DECLARE_EXCEPTION(zpop_err_getpwnam);

#define Verbose (p->trace && (p->debug & DEBUG_VERBOSE))

#ifndef HAVE_FTRUNCATE
# ifdef HAVE_CHSIZE
#  define ftruncate(f, off)  chsize((f), (off))
# endif /* HAVE_CHSIZE */
#endif /* HAVE_FTRUNCATE */
