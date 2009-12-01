/*
 * imaplib.c: client routines for talking to a IMAP-protocol server 
 *
 * Syd 
 * 23Jan97 
 */

#if defined( SUN414 )
#if !defined( const )
#define const
#endif
#endif

#include <limits.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

#if defined( IMAP )

/* c-client include file */

#include "c-clientmail.h"	

#include "bfuncs.h"
#include "dputil.h"
#include "except.h"
#include "catalog.h"
#include "mailserv.h"

#include "error.h"

extern int errno;

#include <pwd.h>
#include <zcstr.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <errno.h>
#include <stdio.h>

#include <zmail.h>
#include <zfolder.h>

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#ifndef strerror
#define strerror(eno)	sys_errlist[eno]
#endif /* !strerror */
#endif /* !HAVE_STRERROR */

extern char * getenv P((const char *));
extern char * getlogin P((void));
extern char * getpass P((const char *));

extern int atoi P((const char *));

#include "license/hosterr.h"

#if 0
/* forward declarations */
int socket_connection P((const char *, int));
static int gettermination P((PopServer));
#endif

#define ERROR_MAX 80	/* a pretty arbitrary size */

# define CRLF "\r\n"

char imap_error[2048+ERROR_MAX+1];
int imap_debug = 0;

int zimap_timeout();

MAILSTREAM *gIMAPh = (MAILSTREAM *) NULL;
static MAILSTREAM *prev = (MAILSTREAM *) NULL;
static char remoteHost[MAXHOSTNAMELEN] = "";	

void
ClearPrev()
{
	prev = (MAILSTREAM *) NULL;
}

/* Various c-client timeouts, in seconds. The following never
   timeout by default in c-client, so we need to adjust them. */

/* XXX might want to make these configurable */

#define CCREAD_TIMEOUT	120
#define CCWRITE_TIMEOUT	120
#define CCRSH_TIMEOUT	120

void zimap_close();

/*
 * Function: zimap_open(char *host, char *username, char *password,
 * 	int flags)
 *
 * Purpose: Establishes a connection with an IMAP server, and
 * 	completes the authorization portion of the session.
 *
 * Arguments:
 * 	host	The server host with which the connection should be
 * 		established.  Optional.  If omitted, internal
 * 		heuristics will be used to determine the server host,
 * 		if possible.
 * 	username
 * 		The username of the mail-drop to access.  Optional.
 * 		If omitted, internal heuristics will be used to
 * 		determine the username, if possible.
 * 	password
 * 		The password to use for authorization.  If omitted,
 * 		internal heuristics will be used to determine the
 * 		password, if possible.
 * 	flags	A bit mask containing flags controlling certain
 * 		functions of the routine.  Valid flags are defined in
 * 		the file pop.h
 *
 * Return value: Upon successful establishment of a connection, a
 * 	non-null MAILSTREAM pointer will be returned. Otherwise, null 
 *      will be returned, and the string variable imap_error will 
 *      contain an explanation of the error.
 */

MAILSTREAM *
zimap_open(host, username, password, flags, new, isspool)
    const char *host, *username;
    char *password;
    int flags;
    int *new;
    int isspool;
{
    MAILSTREAM *mstream; 
    char errbuf[255], hostbuf[255], buf[255], **p1, **p2;
    int len;
    void setremotefolder();
    void setremoteuser();
    struct hostent *hp;
    struct in_addr in1, in2;
    int localhost;
    char *b1, *b2;
    int port = -1;
    char lhs[128];

    *new = 0;
    if ( gIMAPh ) 
        return( gIMAPh );

    if ( !prev )
        *new = 1;

#include <linkage.c>

    /* Determine the user name */
    if (!username) {       
        username = getenv("USER");
        if (!(username && *username)) {
            username = getlogin();
            if (!(username && *username)) {
                struct passwd *passwd;

                passwd = getpwuid(getuid());
                if (passwd && passwd->pw_name && *passwd->pw_name) {
                    username = passwd->pw_name;
                } else {
                    strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 1, "Could not determine username" ));
                    return (0);
                }
            }
        }
    }

    /* Determine the mail host */

    if (!host) {
        host = getenv("MAILHOST");
        if (!host) {
            strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 302, "Could not determine IMAP server" )); 
            return (0);
        }
    }

    /* munge the host value to the form {host}mailbox */

    if ( !FixHost( host, buf ) ) {
        strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 303, "Malformed MAILHOST" ));
        return (0);
    }

    /* see if we are on the same host */

    sscanf(buf, "{%[^}]}%s", hostbuf, errbuf);

    strcpy(remoteHost, hostbuf);

    // see naming.txt in the c-client docs

    // check for port

    if (strstr(remoteHost, ":")) {
        char *buf = strtok(remoteHost, ":");
        if (buf) {
            strcpy(lhs, buf);
            buf = strtok(remoteHost, NULL);
            if (buf) {
                port = atoi(buf);
            }
        }
    } else 
        strcpy(lhs, remoteHost);
    if (strstr(lhs, "/")) {
        strcpy(remoteHost, strtok(lhs, "/")); 
        if (strstr(lhs, "/service=")) {
        } else if (strstr(lhs, "/user=")) {
        } else if (strstr(lhs, "/authuser=")) {
        } else if (strstr(lhs, "/anonymous")) {
        } else if (strstr(lhs, "/debug")) {
        } else if (strstr(lhs, "/secure")) {
        } else if (strstr(lhs, "/imap")) {
        } else if (strstr(lhs, "/pop3")) {
        } else if (strstr(lhs, "/nntp")) {
        } else if (strstr(lhs, "/norsh")) {
        } else if (strstr(lhs, "/ssl")) {
        } else if (strstr(lhs, "/validate-cert")) {
        } else if (strstr(lhs, "/novalidate-cert")) {
        } else if (strstr(lhs, "/tls")) {
        } else if (strstr(lhs, "/readonly")) {
        }
    }

    localhost = 0;
    hp = gethostbyname(remoteHost);
    if ( !hp ) {
	    strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 304, "Unable to get host information" ));
        return (0);
    }

    p1 = hp->h_addr_list;
    if ( *p1 ) {
        (void) memcpy(&in1.s_addr, *p1, sizeof (in1.s_addr));
        gethostname( hostbuf, sizeof( hostbuf ) );
        hp = gethostbyname( hostbuf );
    	if ( !hp ) {
            strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 304, 
			    "Unable to get host information" ));
            return (0);
        }
    	p2 = hp->h_addr_list;
        if ( *p2 ) {
            (void) memcpy(&in2.s_addr, *p2, sizeof (in2.s_addr));
            strcpy( errbuf, inet_ntoa( in1 ) );
            b2 = inet_ntoa( in2 );
            localhost = !strcmp( errbuf, b2 );
        }
    }

    if ( localhost /*&& isspool */) {
        sprintf( buf, catgets( catalog, CAT_CUSTOM, 305, 
		    "Connection to local IMAP server is not possible.\n\nTo fix this problem, set your MAILHOST variable to a host other than '%s',\nor set your MAIL variable to a folder other than '%s', and then restart Z-Mail." ),
            hostbuf, spoolfile );
        strcpy(imap_error, buf );
        return( 0 );
    }

    /* Determine the password */

#define DONT_NEED_PASSWORD 0

    /* Modified to return password if possible -- Bart */
    if ((!password || !*password) && (!DONT_NEED_PASSWORD)) {
        char *p = getpass(catgets(catalog, CAT_CUSTOM, 306, 
		    "Enter IMAP password:"));
        if (p && password)
            (void) strcpy(password, p);
        password = p;
        if (!password) {
            strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 307, 
                "Could not determine IMAP password" ));
            return (0);
        }
    }

    if ( !password )
        password = (char *) username;

    mailserver_SetPassword(mserv, password);
    mailserver_SetUsername(mserv, username);	/* XXX */

    /* do the c-client open here */

    mstream = mail_open( NIL, buf, NIL );
 
    if ( mstream == (MAILSTREAM *) NULL ) {
        sprintf(errbuf, catgets(catalog, CAT_CUSTOM, 308,
            "Could not open IMAP server \"%s\""), buf );
        strcpy(imap_error, errbuf);
        mailserver_UnsetPassword(mserv);
    }

    setremotefolder( buf );
    setremoteuser( username );

    /* set timeouts */

    mail_parameters( mstream, SET_READTIMEOUT, (void *)CCREAD_TIMEOUT );
    mail_parameters( mstream, SET_WRITETIMEOUT, (void *)CCWRITE_TIMEOUT );
    mail_parameters( mstream, SET_RSHTIMEOUT, (void *)CCRSH_TIMEOUT );

    /* register a timeout callback */

    mail_parameters( mstream, SET_TIMEOUT, zimap_timeout );

    gIMAPh = prev = mstream;
    last_spool_size = 0;

    if ( !*new ) 

	/* We are probably recovering from a broken connection. Send
	   all flags since then */

	    ImapSyncFolder( mstream, 0 /* no expunge */ );

    return ( mstream );
}

char *
GetRemoteHost()
{
	return( remoteHost );
}

int
FixHost( host, buf )
char 	*host;
char	*buf;	
{
	int	len, retval = 1;;

    	len = strlen( host );

    	if ( *host == '{' ) {
		if ( host[ len - 1 ] == '}' ) {

			/* {host} --> {host}INBOX */

			sprintf( buf, "%sINBOX", host );
		}
		else if ( index( host, '}' ) ) {

			/* {host}mailbox, just copy */

			strcpy( buf, host );
		}
		else {

			/* malformed. complain and bail */

			strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 303, 
				"Malformed MAILHOST" ));
			retval = 0;
		}
	}
        else if ( index( host, '{' ) || index( host, '}' ) ) {
		strcpy(imap_error, catgets( catalog, CAT_CUSTOM, 303, 
			"Malformed MAILHOST" ));
		retval = 0;
    	}
        else {
		/* assume it's a host, copy and concatentate INBOX. However, 
	   	   if INBOX, then call gethostname() and use that as the host */

		if ( !strcmp( host, "INBOX" ) ) {
			gethostname( host, MAXHOSTNAMELEN );
			if ( host == (char *) NULL ) {
				strcpy(imap_error, catgets( catalog, 
					CAT_CUSTOM, 309, "Unable to get hostname" ));
				retval = 0;
			}
		}
		sprintf( buf, "{%s}INBOX", host );
	} 
	return( retval );
}

/*
 * Function: imap_stat
 *
 * Purpose: Issue the STAT command to the server and return (in the
 * 	value parameters) the number of messages in the maildrop and
 * 	the total size of the maildrop.
 *
 * Return value: 0 on success
 */

int 
imap_stat(server, count, size)
MAILSTREAM *server;
int *count, *size;
{
    if ( server != (MAILSTREAM *) NULL ) 
	    mail_status ( server, server->mailbox, SA_MESSAGES );
    *count = mm_getCount(); 
    return (0);
}

int
zimap_copymsg( file, uid )
char *file;
unsigned long uid;
{
	MAILSTREAM *server;
	char 	buf[64];
	int	retval = 0;
	char	*p;

	/* validate file arg */

	if ( !file || strlen( file ) == 0 ) return( 0 );

	/* strip off { host } part, if any */

	p = file + (strlen( file ) - 1);
	while ( p != file && *p != '}' ) p--;
	if ( *p == '}' ) p++;

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
	if ( server ) {
		sprintf( buf, "%ld", uid );
		retval = (int) mail_copy_full( server, buf, p, CP_UID );
	}
	return( retval );
}

int
zimap_writemsg( fp, file, offset, size )
FILE 	*fp;
char 	*file;
unsigned long offset;
unsigned long size;
{
	MAILSTREAM *server;
	STRING 	message;
	int	retval, n;
	char	*buf;

	buf = malloc( size );
	fseek( fp, offset, SEEK_SET );
	n = fread( buf, sizeof( char ), size, fp );
	if ( n != size )
		n = 0;
	else {
		server = (MAILSTREAM *) zimap_connect( NULL, NULL );
		if ( server ) {
#if 0
			mail_string_init( &message, buf, size );
#else
            INIT(&message, mail_string, buf, size);
#endif
			message.dtb = 0;
			retval = (int) mail_append( server, file, &message );
			if ( !retval )
				n = 0;
		}
		else
			n = 0;
	}
	free( buf );
	return( n );
}

int
zimap_rename( src, dst )
char	*src;
char	*dst;
{
	MAILSTREAM *server;
	int	retval = 0;
	char	buf1[MAXPATHLEN], buf2[MAXPATHLEN];

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
	if ( server ) {
		sprintf( buf1, "%s%s", 
			current_folder->imap_prefix, src );
		sprintf( buf2, "%s%s", 
			current_folder->imap_prefix, dst );
		retval = (int) mail_rename( server, buf1, buf2 ); 
	}
	return( retval );
}

int
zimap_newfolder(file, dir)
char	*file;
int	dir;		/* if 1, append a delimiter */
{
	MAILSTREAM *server;
	int	retval = 0;
	char	buf[MAXPATHLEN];

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
		
	if ( server ) {
		sprintf( buf, "%s%s%c", 
			current_folder->imap_prefix, file,
			( dir ? GetDelimiter() : '\0' ) );
		retval = (int) mail_create( server, buf ); 
	}
	return( retval );
}

int
zimap_rmfolder(file, dir)
char    *file;		/* note, already has {host} prefixed */
int	dir;		/* if 1, append a delimiter */
{
        MAILSTREAM *server;
        int     retval = 0;
        char    buf[MAXPATHLEN];

        server = (MAILSTREAM *) zimap_connect( NULL, NULL );

        if ( server ) {
                sprintf( buf, "%s%c", file,
                        ( dir ? GetDelimiter() : '\0' ) );
                retval = (int) mail_delete( server, buf );
        }
        return( retval );
}

int
zimap_list_dir(dir)
char *dir;
{
	MAILSTREAM *server;
	char	hostbuf[255], errbuf[255], buf[255];

	/* ensure we have an open stream */

	if ( HaveTree() ) 
		return;

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
		
	if ( server ) {

		AddFolder( "", 0, ROOT );

    		sscanf( open_folders[0]->imap_path, "{%[^}]}%s", hostbuf, errbuf );
		sprintf( buf, "{%s}*", hostbuf );

		if ( !dir )
			mail_list( server, "", buf );
		else
			mail_list( server, dir, buf );
		mail_ping( server );
	}
}

void
zimap_expunge( uidval )
unsigned long uidval;
{
	MAILSTREAM *server;

	/* ensure we have an open stream */

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
			
	if ( server && using_imap ) {

		/* check UIDVAL */

		if ( server->uid_validity != uidval )
			return;

		mail_expunge( server );
			
	}
}

void
zimap_turnon( uidval, uid, flags )
unsigned long uidval;
unsigned long uid;
unsigned long flags;
{
	MAILSTREAM *server;
	char buf[ 64 ];	

	/* ensure an open stream */

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
			
	if ( server && using_imap ) {

		/* check UIDVAL */

		if ( server->uid_validity != uidval )
			return;

		sprintf( buf, "%d", uid );

		/* process flags */

		if ( flags & ZMF_DELETE )
			mail_setflag_full( server, buf, 
				"\\DELETED", ST_UID );
		if ( flags & ZMF_REPLIED )
			mail_setflag_full( server, buf, 
				"\\ANSWERED", ST_UID );
		if ( flags & ZMF_UNREAD )
			mail_setflag_full( server, buf, 
				"\\SEEN", ST_UID );
	        mail_ping( server );
	        HandleFlagsAndDeletes( 2 );
	}
}

void
zimap_turnoff( uidval, uid, flags )
unsigned long uidval;
unsigned long uid;
unsigned long flags;
{
	MAILSTREAM *server;
	char buf[ 64 ];

	/* ensure an open stream */

	server = (MAILSTREAM *) zimap_connect( NULL, NULL );
			
	if ( server && using_imap ) {

		/* check UIDVAL */

		if ( server->uid_validity != uidval )
			return;

		sprintf( buf, "%d", uid );

		/* process flags */

		if ( flags & ZMF_DELETE )
			mail_clearflag_full( server, buf, 
				"\\DELETED", ST_UID );
		if ( flags & ZMF_REPLIED )
			mail_clearflag_full( server, buf, 
				"\\ANSWERED", ST_UID );
		if ( flags & ZMF_UNREAD )
			mail_clearflag_full( server, buf, 
				"\\SEEN", ST_UID );
	        mail_ping( server );
	        HandleFlagsAndDeletes( 2 );
	}
}

unsigned long
zimap_UID( server, which )
MAILSTREAM *server;
unsigned long which;
{
	if ( server != (MAILSTREAM *) NULL ) {
		if ( which > server->nmsgs )
			return( 0L );

		return( mail_uid( server, which ) ); 
	}
	else
		return( 0L );
}

static unsigned long uidval = 0L;

unsigned long
getuidval()
{
	return uidval;
}

static char *remotefolder = (char *) NULL;
static char *remoteuser = (char *) NULL;

void 
freeremotefolder()
{
	if ( remotefolder ) {
		free( remotefolder );
		remotefolder = (char *) NULL;
	}
}

char *
getremotefolder()
{
	return remotefolder;
}

void
setremotefolder( path )
char *path;
{
	freeremotefolder();
	if ( path ) {
		remotefolder = malloc( strlen( path ) + 1 );
		strcpy( remotefolder, path );
	}
}

void 
freeremoteuser()
{
	if ( remoteuser ) {
		free( remoteuser );
		remoteuser = (char *) NULL;
	}
}

char *
getremoteuser()
{
	return remoteuser;
}

void
setremoteuser( name )
char *name;
{
	freeremoteuser();
	if ( name ) {
		remoteuser = malloc( strlen( name ) + 1 );
		strcpy( remoteuser, name );
	}
}

unsigned long
zimap_getuidval( server )
MAILSTREAM *server;
{
	if ( server == (MAILSTREAM *) NULL )
		server = gIMAPh;

	return( ( uidval = ( server ? server->uid_validity : 0 ) ) );
}

void 
zimap_close(server, hasError)
MAILSTREAM *server;
unsigned long hasError;
{
    if ( server == (MAILSTREAM *) NULL ) 
	server = gIMAPh;
    if ( server != (MAILSTREAM *) NULL ) 
	    mail_close( server );
    gIMAPh = (MAILSTREAM *) NULL;
#if 0
    PurgeTree();
#endif
    imap_initialized = 0;
}

/* the following is called from mm_notify */

void
zimap_display_info()
{
   error(Message,
	catgets( catalog, CAT_CUSTOM, 310, "IMAP: %s\n" ), imap_error);
}

void
zimap_display_warning()
{
   error(Message,
	catgets( catalog, CAT_CUSTOM, 310, "IMAP: %s\n" ), imap_error);
}

void
zimap_display_error()
{
   error(ZmErrWarning,
	catgets( catalog, CAT_CUSTOM, 310, "IMAP: %s\n" ), imap_error);
}

void
zimap_shutdown( folder )
char *folder;
{
	if ( folder )
	    TruncIfNoCache( folder );
	zimap_close(gIMAPh, 0L);
}

static int ret_msg = -1;

static void
zimap_retrieve_setmsg( message )
int message;
{
	ret_msg = message;
}

static int
zimap_retrieve_getmsg()
{
	return( ret_msg ); 
}

void
zimap_read_server(dp, rddata)
struct dpipe *dp;
GENERIC_POINTER_TYPE *rddata;
{
    MAILSTREAM *server = (MAILSTREAM *)rddata;
    char *p, *fromserver = NULL;
    unsigned long size, textsize, newsize;
    unsigned long uid, message;
    char uidbuf[ 64 ], statbuf[ 64 ];
    int	i, toSend;
    unsigned long flags;
    char *buf;
    MESSAGECACHE *foo;
    ENVELOPE *env;
    BODY *body;

    if ( server == (MAILSTREAM *) NULL ) {
	dpipe_Close(dp);
        return;
    }

    /* do the header first */

    message = (unsigned long) zimap_retrieve_getmsg();

#if 1
    fromserver = mail_fetchheader_full( server, message, NIL, 
	&size, FT_PREFETCHTEXT );
#else
    fromserver = mail_fetchheader_full( server, message, NIL, 
	&size, 0 );
#endif

    if ( fromserver != (char *) NULL ) {

	buf = (char *) malloc( size );
	if ( !buf ) {
		dpipe_Close(dp);
        	return;
	}

	p = fromserver;
	newsize = 0;

	if ( strchr( p, '\r' ) )
		for ( i = 0; i < size; i++ ) {
			if ( *p != '\r' )
				buf[ newsize++ ] = *p;
			p++;
		}

	p = buf;
	size = newsize;

        while ( !isprint(*(p + size - 1)) )size--; 

	while ( size ) {
		toSend = (size < INT_MAX ? size : INT_MAX);
		dpipe_Write(dp, p, toSend ); 
		size -= toSend;
		p += toSend;
	}

	free( buf );
    }
    else {
	dpipe_Close(dp);
        return;
    }

#if 1
    env = mail_fetchstructure( server, message, &body );
#endif

    /* now the text */

#if 1
    fromserver = mail_fetchtext_full( server, message, &textsize, 0 ); 
#else
    textsize = body->size.bytes;	
    fromserver = malloc( textsize );   /* XXX remember to free */
    memset( fromserver, (int)  ' ', textsize );
#endif

    /* reading the env from the server causes the status flags to be
       updated. We can the write the Status: line, if needed, based 
       upon the IMAP4 status. */

    env = mail_fetchstructure( server, message, NIL );

    uid = zimap_UID(server, message);

    sprintf( uidbuf, "\nX-ZmUID: %08lx\n", uid );
    p = uidbuf;
    size = strlen( uidbuf );
    while ( size ) {
	toSend = (size < INT_MAX ? size : INT_MAX);
	dpipe_Write(dp, p, toSend ); 
	size -= toSend;
	p += toSend;
    }

    foo = mail_elt( server, message );

    strcpy( statbuf, "Status: " ); 

    if ( !foo->recent ) { 
#if 0
	strcat( statbuf, "O" );

	if ( foo->seen )
		strcat( statbuf, "R" );
#endif
    }

    if ( foo->deleted )
	strcat( statbuf, "D" );

    if ( foo->answered )
	strcat( statbuf, "r" );

    if ( strcmp(statbuf, "Status: ") ) 
	strcat( statbuf, "\n\n" );
    else
	strcpy( statbuf, "\n" );
    size = strlen( statbuf );
    p = statbuf;
		
    while ( size ) {
	toSend = (size < INT_MAX ? size : INT_MAX);
	dpipe_Write(dp, p, toSend); 
	size -= toSend;
	p += toSend;
    }

    /* now, dump the mail message text */

    size = textsize;
    if ( fromserver != (char *) NULL ) {

	buf = (char *) malloc( size );
	if ( !buf ) {
		dpipe_Close(dp);
        	return;
	}

	p = fromserver;
	newsize = 0;

	if ( strchr( p, '\r' ) )
		for ( i = 0; i < size; i++ ) {
			if ( *p != '\r' )
				buf[ newsize++ ] = *p;
			p++;
		}

	p = buf;
	size = newsize;

	while ( size ) {
		toSend = (size < INT_MAX ? size : INT_MAX);
		dpipe_Write(dp, p, toSend); 
		size -= toSend;
		p += toSend;
	}

	free( buf );
    }
    dpipe_Close(dp);
}

/*
 * Function: zimap_retrieve
 *
 * Purpose: Retrieve a specified message from the maildrop.
 *
 * Arguments:
 * 	server	The server to retrieve from.
 * 	message	The message number to retrieve.
 *
 * Return value: A dpipe pointing to the message, if successful, or
 * 	null with zimap_error set if not.
 */

struct dpipe *
zimap_retrieve(server, message)
MAILSTREAM *server;
int message;
{
    int *IDs, *sizes;
    struct dpipe *dp;

    if (!(dp = dputil_Create((dpipe_Callback_t)0,
			     (GENERIC_POINTER_TYPE *)0,
			     zimap_read_server,
			     (GENERIC_POINTER_TYPE *)server, 0))) {
        strcpy(imap_error,
	    catgets(catalog, CAT_CUSTOM, 311, "Out of memory in zimap_retrieve"));
	return (0);
    }

    zimap_retrieve_setmsg( message );
    return dp;
}

/* Function: zimap_delete
 *
 * Purpose: Delete a specified message.
 *
 * Arguments:
 * 	server	Server from which to delete the message.
 * 	message	Message to delete.
 *
 * Return value: 0 on success, non-zero with error in zimap_error
 * 	otherwise.
 */

int 
zimap_delete(server, message)
MAILSTREAM *server;
int message;
{
    char buf[ 16 ];

    if ( server != (MAILSTREAM *) NULL ) {
	    sprintf( buf, "%ld", message );
	    mail_setflag (server,buf,"\\Deleted");
    }
    return( 0 );
}

unsigned long
zimap_last(server)
MAILSTREAM *server;
{
    if ( server != (MAILSTREAM *) NULL ) 
	    mail_status ( server, server->mailbox, SA_RECENT );
    return( mm_getRecent() ); 
}

#endif /* IMAP */

