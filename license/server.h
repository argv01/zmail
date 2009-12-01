/* server.h	Copyright 1991, 1994 Z-Code Software Corp. */

#ifndef _LS_SERVER_H_
#define _LS_SERVER_H_

/*
 * $Revision: 2.42 $
 * $Date: 2005/04/27 08:46:11 $
 * $Author: syd $
 */

#include "osconfig.h"
#include "bfuncs.h"
#include "catalog.h"
#include "linklist.h"
#include "lsfile.h"
#include "lsutil.h"
#include "zctype.h"
#include "zmflag.h"

#undef howmany
#undef roundup
#define howmany(a,b)	(((int)((a)+(b)-1))/(b))
#define roundup(a,b)	(howmany(a,b)*(b))

#ifdef ZBITS
# undef ZBITS
#endif /* ZBITS */
#define ZBITS(x)	(sizeof(x)*8)

#ifdef BIT
# undef BIT
#endif /* BIT */
#define BIT(A, i)	    (((A)[(i)/ZBITS(*(A))] >> ((i)%ZBITS(*(A)))) & 1)

#undef BITOP
#define BITOP(A, i, op, b)   ((A)[(i)/ZBITS(*(A))] op ((b) << ((i)%ZBITS(*(A)))))
#define xdigit_to_int(d) \
		(isdigit(d) ? d-'0' : islower(d) ? (d)-'a'+10 : (d)-'A'+10)

typedef struct license_data {
    char *password;
    char *version;
    char *hostname;
    char *progname;
    unsigned long hostid;
    time_t expiration_date;
    unsigned long max_users;
    long flags;			/* Misc. boolean data */
    char **valid_users;
} license_data;

typedef struct license_float {
    struct link link;
    char password[17];
    unsigned long max_users;
    unsigned long current_users;
    unsigned long max_index_in_use;	/* actually 1 + max index */
    struct float_request {	/* Host/process id pairs for licensing */
	char user[16];
	unsigned long host;
	unsigned long pid;
	unsigned long key;	/* used for encrypting a give-back-token */
	long last_contact;
    } *accepted;		/* Allocated array max_users long */
} license_float;

#define fill_float_request_struct(request,_user,_host,_pid,_key,_last_contact) \
	       (strcpy((request)->user, _user), \
		(request)->host = _host, \
		(request)->pid = _pid, \
		(request)->key = _key, \
		(request)->last_contact = _last_contact)

extern void ls_freeentry(), ls_warn_expires();
#ifndef LICENSE_FREE
extern int ls_access(), ls_file_client(), ls_nls_client();
#endif
extern char *ls_encode(), *ls_decode(), *ls_password(),
					*ls_password_for_named_program();
extern char **ls_read_malloced_argv_of_malloced_in_addrs(),
	     *ls_delete_first_arg();
extern void ls_give_back_token();
extern char *GetPeerName();
extern int GetLocalPortOfSocket();

#define ls_temporary(user) \
	    (ison(license_flags, TEMP_LICENSE)? \
	    turnoff(license_flags, TEMP_LICENSE), ls_access(NULL, user) : 0)

extern long license_expiration;		/* Should be time_t */

#define LICENSE_DIR_HIDDEN     'l','i','c','e','n','s','e',0
#ifndef _WINDOWS
#define LICENSE_DATA_SUFFIX "/license/license.data"
#define LICENSE_FILE_HIDDEN    'l','i','c','e','n','s','e','.','d','a','t','a',0
#else /* _WINDOWS */
#define LICENSE_DATA_SUFFIX "\\license\\license.dat"
#define LICENSE_FILE_HIDDEN    'l','i','c','e','n','s','e','.','d','a','t',0
#endif /* _WINDOWS */
#define LICENSE_DIR	unhidestr(LICENSE_DIR_HIDDEN)
#define LICENSE_FILE	unhidestr(LICENSE_FILE_HIDDEN)

#define LICENSE_SEPARATOR "==\n"	/* '-' can be mistaken for an int */

/* Special keys for password creation */

#define LICENSE_MAGIC		((u_long)8161990L)
#define LICENSE_UNLIMITED (365 * 45 * 86400L + 674157728L)

/* Flags-- currently unused */

#define LICENSE_NOCODE	01	/* Do not encode in ls_wrentry() */

/* Error conditions */

extern int ls_verbose;
extern char *ls_err;
extern char ls_errbuf[80];

#define LICENSE_EXPIRED	catgets( catalog, CAT_LICENSE, 190, "Expiration date has passed." )
#define LICENSE_BADUSER	catgets( catalog, CAT_LICENSE, 191, "Not a registered user." )
#define LICENSE_BADHOST	catgets( catalog, CAT_LICENSE, 192, "Not a registered host." )
#define LICENSE_BADVERS	catgets( catalog, CAT_LICENSE, 193, "License does not match Z-Mail version." )
#ifndef _WINDOWS
#define LICENSE_MANGLED	catgets( catalog, CAT_LICENSE, 194, "Not a valid password." )
#else /* _WINDOWS */
#define LICENSE_MANGLED	catgets( catalog, CAT_LICENSE, 412, "License key is not valid.\nPlease make sure your license key has been entered correctly." )
#endif /* _WINDOWS */
#define LICENSE_BADHOST_OR_BADVERS \
	catgets( catalog, CAT_LICENSE, 195, "This Z-Mail version not registered on this host." )
#define LICENSE_BADNAME catgets( catalog, CAT_LICENSE, 196, "Server host name unknown." )
#define LICENSE_TOOFAR  catgets( catalog, CAT_LICENSE, 197, "Server host not on this subnet." )
#define LICENSE_NOSOCK  catgets( catalog, CAT_LICENSE, 198, "Cannot create server socket." )
#define LICENSE_NOCONN	catgets( catalog, CAT_LICENSE, 199, "Cannot connect to server socket." )
#ifdef NETWORK_LICENSE
#define LICENSE_NODATA	catgets( catalog, CAT_LICENSE, 200, "Cannot access license server." )
#else /* !NETWORK_LICENSE */
#if !defined(MAC_OS) && !defined(_WINDOWS)
# define LICENSE_NODATA	catgets(catalog, CAT_LICENSE, 409, "No license found.")
#else /* MAC_OS or Windows*/
#ifdef MAC_OS
# define LICENSE_NODATA	catgets(catalog, CAT_LICENSE, 410, "This copy of Z-Mail is not registered.  Opening folders has been disabled.")
#else /* WINDOWS */
# define LICENSE_NODATA	catgets(catalog, CAT_LICENSE, 413, "Valid license information could not be found.\nThis copy of Z-Mail is not registered and opening folders has been disabled.\n\n")
#endif
#endif /* !MAC_OS && !WINDOWS */
#endif /* !NETWORK_LICENSE */
#define LICENSE_NODEMON	catgets( catalog, CAT_LICENSE, 201, "Cannot access server for validation." )
#define LICENSE_TIMEOUT	catgets( catalog, CAT_LICENSE, 202, "Connection to server timed out." )
#define LICENSE_NOZCNLS	catgets( catalog, CAT_LICENSE, 203, "No ZCNLSERV environment variable found." )
#define LICENSE_NOHOSTS	catgets( catalog, CAT_LICENSE, 204, "Cannot look up host address for server." )
#define LICENSE_NOFLOAT	catgets( catalog, CAT_LICENSE, 205, "No floating license tokens available." )
#define LICENSE_NORETRY	catgets( catalog, CAT_LICENSE, 206, "Unable to complete conversation with server." )
#define LICENSE_TOOBIG	catgets( catalog, CAT_LICENSE, 207, "Not enough memory." )
#define LICENSE_ALTERED	catgets( catalog, CAT_LICENSE, 208, "License data was altered." )
#define LICENSE_EOF	catgets( catalog, CAT_LICENSE, 209, "No more entries." )
#define LICENSE_BADENT	catgets( catalog, CAT_LICENSE, 210, "Bad license entry." )
#define LICENSE_BADDATE	catgets( catalog, CAT_LICENSE, 211, "System clock set too far in the past." )
#define LICENSE_BADDEMO	catgets( catalog, CAT_LICENSE, 212, "Demonstration period has expired." )
#define LICENSE_NOMEM	catgets( catalog, CAT_LICENSE, 95, "Out of memory." )

#ifdef SINGLE_LICENSE_KEY_REQUIRED
#define SINGLE_LICENSE_MODE
#endif /* SINGLE_LICENSE_KEY_REQUIRED */

#ifdef SINGLE_LICENSE_MODE
extern catalog_ref single_license_errstr[];
#define LICENSE_SINGLE_UNUSED  0
#define LICENSE_SINGLE_IN_USE  1
#define LICENSE_SINGLE_BADPORT 2
#define LICENSE_SINGLE_BADSOCK 3

extern int single_license_errno();
#define single_license_error() catgetref(single_license_errstr[single_license_errno()])

#define SINGLE_LICENSE_ERROR_TEXT	catgets(catalog, CAT_LICENSE, 395, "%s\n\n\
Please make sure that TCP port %d is available for use by %s."),\
single_license_error(), NLS_REGISTERED_PORT, zmName()

#define SINGLE_LICENSE_ADVERTISEMENT	catgets(catalog, CAT_LICENSE, 396, "%s\n\n\
This machine is licensed to run only one copy of %s at a time.\n"),\
single_license_error(), zmName()
#endif /* SINGLE_LICENSE_MODE */

/* How far can we time-travel before it's suspicious? */
#define LICENSE_DATE_FUDGE	(5 * 24*60*60L) /* 5 days */

/* Who ya gonna call? */
#define GHOSTBUSTERS	catgets( catalog, CAT_LICENSE, 214, "Call for licensing." )

#define LICENSE_PHONE	GHOSTBUSTERS

/* License server control */
extern unsigned long license_flags;

#ifndef ULBIT
#define ULBIT(bit)		((u_long)1 << (u_long)(bit))
#endif /* ULBIT */

#define DEMO_LICENSE  ULBIT(0) /* require expiration checks */
#define DATE_LICENSE  ULBIT(1) /* require expiration checks */
#define USER_LICENSE  ULBIT(2) /* require user id checks */
#define HOST_LICENSE  ULBIT(3) /* require host id checks */
#define NLS_LICENSE   ULBIT(4) /* attempt NLS conversation */
#define TEMP_LICENSE  ULBIT(5) /* run, but in restricted mode */
#define LOCAL_ONLY_LICENSE	ULBIT(6)
#define NETWORK_ONLY_LICENSE	ULBIT(7)
#define ALLOW_FLOAT_LICENSE	ULBIT(8)
#define NEED_LICENSE  ULBIT(9) /* voluntarily gave up license */

#define CLASS_B_LICENSE		ULBIT(10)			/* 1024 */
#define CLASS_C_LICENSE		ULBIT(11)			/* 2048 */
#define CLASS_D_LICENSE		(CLASS_B_LICENSE|CLASS_C_LICENSE) /* 3072 */

#define CLASS_A_LICENSE		(ULBIT(12)|CLASS_D_LICENSE)	/* 7168 */

#define FULL_LICENSE  (DATE_LICENSE|USER_LICENSE|HOST_LICENSE|NLS_LICENSE)

long ls_gethostid(),
#ifdef DECLARE_GETHOSTID
     gethostid(),
#endif /* DECLARE_GETHOSTID */
     ls_concoct_hostid();

/* Network License server stuff */

#define NLS_SERVNAME	"zcnls"	/* Z-Code Network License Server */
#define NLS_PROTOCOL	"tcp"	/* Allowable are "tcp" or NULL */
#define NLS_WELLKNOWN_PORT	722
#define NLS_REGISTERED_PORT	2722
#define NLS_DEFAULT_PORT	NLS_REGISTERED_PORT

#define NLS_PROGNAME	"ZCNLSD"	/* name that appears in license entries */

#define NLS_GREETING	"HELLO"
#define NLS_FUCKOFF	"GOWAY"	       /* must be same length as greeting */
#define NLS_TRYAGAIN	"RETRY"
#define NLS_WHY		"WHY??"	       /* length must be same as NLS_TRYAGAIN */
#define NLS_GOAHEAD	"OK"
/* client requests other than access... */
#define NLS_SENDFILE	"SEND"
#define NLS_PING	"PING"
#define NLS_PONG	"PONG"		/* server's reply to ping */
#define NLS_ECHO	"ECHO"		/* echo up to 1024 bytes */
#define NLS_REFUND	"REFUND"	/* give back a token */
#define NLS_BYE		"BYE"
#define NLS_GOTTAGO	"GOTTAGO"

#define ONE_MINUTE	60L
#define ONE_HOUR	(60L*ONE_MINUTE)
#define ONE_DAY		(24L*ONE_HOUR)

#define NLS_NOW		time((time_t *)0)
#define LS_ERRBUFSIZ	512

/* Return values from ls_access() and ls_retry() */
#define LS_UNSUCCESSFUL		(-1)
#define LS_SUCCESSFUL		( 0)
#define LS_FIRST_FAILURE	( 1)
#define LS_SECOND_FAILURE	( 2)

/* A randomizing factor for the time between license checks */
#define NLS_RANDOMIZE	ONE_MINUTE
/* Expiration time of floating license */
#define NLS_FLOAT_TIME	(15*ONE_MINUTE)
/* How often the server checks for other servers running */
#define NLS_CHECK_SUPERIORS_THIS_OFTEN (5*ONE_MINUTE + 7)
/* Allowable error in last contact time */
#define NLS_FLOAT_FUDGE	(NLS_FLOAT_TIME+(5*ONE_MINUTE))
/* Allowable timestamp error from client */
#define NLS_CTS_FUDGE	(24*ONE_HOUR)
/* Allowable timestamp error from server */
#define NLS_STS_FUDGE	(NLS_CTS_FUDGE+NLS_FLOAT_FUDGE)
/* Allowed time of a conversation, enforced by both server and client */
/*
 * Note: if you change any of the following, change README.NLS too!
 */
#define NLS_CLIENT_CONNECT_TIMEOUT	3
#define NLS_CLIENT_GETHELLO_TIMEOUT	20
#define NLS_CLIENT_CONVERS_TIMEOUT	29
#define NLS_CLIENT_REFUND_TIMEOUT	10
#define NLS_SERVER_CONVERS_TIMEOUT	31
#define NLS_SUPCHECK_CONNECT_TIMEOUT	2
#define NLS_SUPCHECK_GETHELLO_TIMEOUT	10

extern int nls_fuckoff_mode;
extern char nls_fuckoff_who[80];

/* Backwards compatibility support */
extern u_long nls_backwards_compat;

#define ZMAIL_2_1_COMPAT		ULBIT(0)

#endif /* _LS_SERVER_H_ */
