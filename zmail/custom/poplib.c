/*
 * pop.c: client routines for talking to a POP3-protocol post-office
 * server
 *
 * Jonathan Kamens
 * August 13, 1991
 */

#ifndef lint
static char	poplib_rcsid[] =
    "$Id: poplib.c,v 2.68 2005/04/27 08:46:10 syd Exp $";
#endif

#ifdef PCNFS /* standalone DOS PCNFS support */
#include <sys\tk_types.h>
#endif /* PCNFS */

#include "osconfig.h"

/* Akk: I have no idea. There's a note saying "see pop.h" but pop.h
 * has no mention of POPLIB_GETLINE_MAX, nor does any other file.
 * It's unique to this file.
 */
#define POPLIB_GETLINE_MAX BUFSIZ

#ifdef POP3_SUPPORT

#include <zcsock.h>
#include <zctype.h>
#include <zmstring.h>
#include <general.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include "zmail.h"
#include "pop.h"
#include "bfuncs.h"
#include "dputil.h"
#include "except.h"
#include "catalog.h"
#include "mailserv.h"

#ifdef MSDOS
#include <stdlib.h>
#else /* !MSDOS */
extern int errno;
#endif /* !MSDOS */

#ifdef KERBEROS
#include <krb.h>
#include <des.h>
#ifdef sun
#include <malloc.h>
#else /* !sun */
extern char *
malloc( /* unsigned */ );
extern void 
free( /* char * */ );
#endif /* sun */
#endif /* KERBEROS */

#ifdef HESIOD
#include <hesiod.h>

/*
 * It really shouldn't be necessary to put this declaration here, but
 * the version of hesiod.h that Athena has installed in release 7.2
 * doesn't declare this function; I don't know if the 7.3 version of
 * hesiod.h does.
 */
extern struct servent *
hes_getservbyname( /* char *, char * */ );
#endif /* HESIOD */

#ifndef MAC_OS
#include <pwd.h>
#endif /* !MAC_OS */
#include <zcstr.h>

#ifndef _WINDOWS
#include <netdb.h>
#endif /* !_WINDOWS */

#include <errno.h>
#include <stdio.h>

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#ifndef strerror
#define strerror(eno)	sys_errlist[eno]
#endif /* !strerror */
#endif /* !HAVE_STRERROR */

extern char * getenv P((const char *));
extern char * getlogin P((void));
extern char * getpass P((const char *));

#ifndef _WINDOWS
extern int socket P((int, int, int));
extern int close P((int));
#endif /* !_WINDOWS */

extern int atoi P((const char *));

#ifdef KERBEROS
extern int 
krb_sendauth P((long, int, KTEXT, char *, char *, char *, u_long, MSG_DAT **,
		CREDENTIALS *, Key_schedule, struct sockaddr_in *,
		struct sockaddr_in *, char *));
extern char *
krb_realmofhost P((char *));
#endif /* KERBEROS */

#ifndef MAC_OS
#ifndef _WINDOWS
#include "license/hosterr.h"
#endif /* !_WINDOWS */
#include "license/hostserv.h"
#endif /* !MAC_OS */

/* forward declarations */
int socket_connection P((const char *, int));
static int gettermination P((PopServer));

#define ERROR_MAX 80	/* a pretty arbitrary size */
#define POP_PORT 110
#define KPOP_PORT 1109
#define POP_SERVICE "pop3"
#define KPOP_SERVICE "kpop"

#ifdef MAC_OS
# define CRLF "\n\r"
#else /* !MAC_OS */
# define CRLF "\r\n"
#endif /* MAC_OS */
char pop_error[2048+ERROR_MAX+1];
int pop_debug = 0;

/*
 * Function: pop_open(char *host, char *username, char *password,
 * 		      int flags)
 *
 * Purpose: Establishes a connection with a post-office server, and
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
 * 	non-null PopServer will be returned.  Otherwise, null will be
 * 	returned, and the string variable pop_error will contain an
 * 	explanation of the error.
 */
PopServer 
pop_open(host, username, password, flags)
    const char *host, *username;
    char *password;
    int flags;
{
    int sock, port;
    PopServer server;
    char buf[255], *p;

    /* Determine the user name */
    if (!username) {
	username = getenv("USER");
#if defined(MSDOS) || defined(MAC_OS)
	if (!(username && *username)) 
	{
		strcpy(pop_error, "Could not determine username");
		return (0);
	}
#else /* !MSDOS && !MAC_OS */
	if (!(username && *username)) {
	    username = getlogin();
	    if (!(username && *username)) {
		struct passwd *passwd;

		passwd = getpwuid(getuid());
		if (passwd && passwd->pw_name && *passwd->pw_name) {
		    username = passwd->pw_name;
		} else {
		    strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 1, "Could not determine username" ));
		    return (0);
		}
	    }
	}
#endif /* MSDOS || MAC_OS */
    }

    /* Determine the mail host */
#ifdef HESIOD
    if ((!host) && (!(flags & POP_NO_HESIOD))) {
	struct hes_postoffice *office;

	office = hes_getmailhost(username);
	if (office && office->po_type && (!strcmp(office->po_type, "POP"))
		&& office->po_name && *office->po_name && office->po_host
		&& *office->po_host) {
	    host = office->po_host;
	    username = office->po_name;
	}
    }
#endif
    if (!host) {
	host = getenv("MAILHOST");
	if (!host) {
	    strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 2, "Could not determine POP server" ));
	    return (0);
	}
    }
    /* Determine the password */
#ifdef KERBEROS
#define DONT_NEED_PASSWORD (! (flags & POP_NO_KERBEROS))
#else
#define DONT_NEED_PASSWORD 0
#endif

    /* Modified to return password if possible -- Bart */
    if ((!password || !*password) && (!DONT_NEED_PASSWORD)) {
	if (!(flags & POP_NO_GETPASS)) {
#ifdef MSDOS  /* RJL ** 7.1.93 - get passwd from envir var for now */
	    char *p = getenv( "PASSWD" );
	    if (!p)
		p = mailserver_GetPassword(mserv);
# ifndef _WINDOWS
	    if( !p )
		p = getpass(catgets( catalog, CAT_SHELL, 881, "Enter password:" ));
# endif /* !_WINDOWS */
#else  /* !MSDOS */
	    char *p = getpass(catgets(catalog, CAT_CUSTOM, 218, "Enter POP password:"));
#endif /* !MSDOS */
	    if (p && password)
		(void) strcpy(password, p);
	    password = p;
	}
	if (!password) {
	    strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 4, "Could not determine POP password" ));
	    return (0);
	}
    }
    if (password)
	flags |= POP_NO_KERBEROS;
    else
	password = (char *) username;

    server = (PopServer) malloc(sizeof(struct _PopServer));
    if (!server) {
	strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 5, "Out of memory in pop_open" ));
	return (0);
    }
#ifdef MAC_OS
    strcpy(buf, host);
    if (p = strrchr(buf, ':'))
    {
	*p++ = 0;
	if (!(port = atoi(p)))
#ifdef KERBEROS
	    port = KPOP_PORT;
#else /* !KERBEROS */
	    port = POP_PORT;
#endif	/* !KERBEROS */
    } else port = POP_PORT;
    sock = open_port(server, buf, port, flags);
#else /* !MAC_OS */
    sock = socket_connection(host, flags);
#endif /* !MAC_OS */
    if (sock == -1) {
    	free(server);
	return (0);
    }
    server->ispop = POP3_MINIMAL;
    server->pop_rset_flag = FALSE;
    server->file = sock;
    server->data = 0;
    server->dp = server->buffer;

    if (getok(server)) {
    	pop_close(server);
	return (0);
    }

    /* Detect UCB POP server, assume LAST works if this is NOT */
    if (strncmp(pop_error, "UCB", 3) != 0) {
	if (strstr(pop_error, "ZPOP")) {
	    /* XXX This should not be hard-coded! */
	    if (strstr(pop_error, "version 1.0")) {
		server->ispop = POP3_ZPOP;
	    } else {
		server->ispop = POP3_LAST;
	    }
	} else
	    server->ispop = POP3_LAST;
    }
    /* XXX Detect other POP servers and set UIDL?? */

    sprintf(buf, "USER %s", username);
    if (sendline(server, buf) || getok(server)) {
    	pop_close(server);
	return (0);
    }
    if (strlen(password) > ERROR_MAX - 6) {
	pop_close(server);
	strcpy(pop_error,
		catgets( catalog, CAT_CUSTOM, 7, "Password too long; recompile pop.c with larger ERROR_MAX" ));
	return (0);
    }
    sprintf(buf, "PASS %s", password);
    mailserver_SetPassword(mserv, password);
    if (sendline(server, buf) || getok(server)) {
	/* pf Fri Feb 18 11:36:16 1994
	 * this is a hack but it works.. make sure the error string
	 * is the same as what pop_init is expecting.  Do not localize
	 * this.
	 */
	if (strstr(pop_error, "assword")) {
	    strcpy(pop_error, "-ERR invalid password");
	    mailserver_UnsetPassword(mserv);
	}
    	pop_close(server);
	return (0);
    }
    return (server);
}

int pop_top P((PopServer pserv, char *tdata, int msgnum, int lines));

/*
 * pop_top:  issue TOP command to pop server, with given msg num & # of
 *           lines.  assumes open connection.  puts results into pserv.
 *           doesn't report pop errs
 */
int pop_top(pserv, tdata, msgnum, lines)
    PopServer pserv;
    char *tdata;
    int msgnum, lines;
{
    char buf[32];
    char *p;
    int size;
    
    pserv->data = 0;
    if (!tdata)
        return (-1);
    *tdata = 0;
    sprintf(buf, "TOP %d %d", msgnum, lines);
    if (sendline(pserv, buf) || getok(pserv))
        return (-1);
    while (TRUE) {
        if (!(p = poplib_getline(pserv)))
            return (-1);
        if (p[0] == '.' && !p[1])
            break;
        size = strlen(p);
        strncpy(tdata, p, size + 1);
        tdata += size;
        *tdata++ = '\n';
        *tdata = '\0';
    }
    return 0;
} /* pop_top */

/*
 * Function: pop_stat
 *
 * Purpose: Issue the STAT command to the server and return (in the
 * 	value parameters) the number of messages in the maildrop and
 * 	the total size of the maildrop.
 *
 * Return value: 0 on success, or non-zero with an error in pop_error
 * 	in failure.
 */
int 
pop_stat(server, count, size)
PopServer server;
int *count, *size;
{
    char *fromserver;

    if (sendline(server, "STAT") || (!(fromserver = poplib_getline(server))))
        return (-1);

    if (strncmp(fromserver, "+OK ", 4)) {
        strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 8, "Unexpected response from POP server in pop_stat" ));
        return (-1);
    }
    *count = atoi(&fromserver[4]);

    fromserver = index(&fromserver[4], ' ');
    if (!fromserver) {
        sprintf(pop_error, catgets(catalog, CAT_CUSTOM, 9, "Badly formatted response from server in %s."), "pop_stat");
        return (-1);
    }
    *size = atoi(fromserver + 1);

    return (0);
}

/*
 * Function: pop_list
 *
 * Purpose: Performs the POP "list" command and returns (in value
 * 	parameters) two malloc'd zero-terminated arrays -- one of
 * 	message IDs, and a parallel one of sizes.
 *
 * Arguments:
 * 	server	The pop connection to talk to.
 * 	message	The number of the one message about which to get
 * 		information, or 0 to get information about all
 * 		messages.
 *
 * Return value: 0 on success, non-zero with error in pop_error on
 * 	failure.
 *
 */

int 
pop_list(server, message, IDs, sizes)
PopServer server;
int message, **IDs, **sizes;
{
    int how_many, i;
    char *fromserver;

    if (message)
	how_many = 1;
    else {
	int count, size;

	if (pop_stat(server, &count, &size))
	    return (-1);
	how_many = count;
    }

    *IDs = (int *) malloc((how_many + 1) * sizeof(int));
    *sizes = (int *) malloc((how_many + 1) * sizeof(int));
    if (!(*IDs && *sizes)) {
	strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 10, "Out of memory in pop_list" ));
	return (-1);
    }
    if (message) {
	sprintf(pop_error, "LIST %d", message);
	if (sendline(server, pop_error)) {
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	if (!(fromserver = poplib_getline(server))) {
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	if (strncmp(fromserver, "+OK ", 4)) {
	    if (!strncmp(fromserver, "-ERR", 4)) {
		pop_error[0] = 0;
		strncat(pop_error, fromserver+4, ERROR_MAX);
	    } else
		strcpy(pop_error,
			catgets( catalog, CAT_CUSTOM, 11, "Unexpected response from server in pop_list" ));
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	(*IDs)[0] = atoi(&fromserver[4]);
	fromserver = index(&fromserver[4], ' ');
	if (!fromserver) {
	    sprintf(pop_error, catgets(catalog, CAT_CUSTOM, 9, "Badly formatted response from server in %s."), "pop_list");
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	(*sizes)[0] = atoi(fromserver);
	(*IDs)[1] = (*sizes)[1] = 0;
	return (0);
    } else {
	if (sendline(server, "LIST") || getok(server)) {
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	for (i = 0; i < how_many; i++) {
	    if (!(fromserver = poplib_getline(server))) {
		free((char *) *IDs);
		free((char *) *sizes);
		return (-1);
	    }
	    (*IDs)[i] = atoi(fromserver);
	    fromserver = index(fromserver, ' ');
	    if (!fromserver) {
	        sprintf(pop_error, catgets(catalog, CAT_CUSTOM, 9, "Badly formatted response from server in %s."), "pop_list");
		free((char *) *IDs);
		free((char *) *sizes);
		return (-1);
	    }
	    (*sizes)[i] = atoi(fromserver);
	}
	if (gettermination(server)) {
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	(*IDs)[i] = (*sizes)[i] = 0;
	return (0);
    }
}

void
pop_read_server(dp, rddata)
struct dpipe *dp;
GENERIC_POINTER_TYPE *rddata;
{
    PopServer server = (PopServer)rddata;
    char *fromserver = poplib_getline(server);
    int size;

    if (!fromserver) {
	dpipe_Close(dp);
	RAISE(strerror(ENOMEM), "pop_read_server");
    }
    if (fromserver[0] == '.') {
	if (!fromserver[1]) {
	    dpipe_Close(dp);
	    return;
	}
	fromserver++;
    }
    size = strlen(fromserver);
    /* 11/23/93 GF; poplib_getline() stripped our CRLF;  put the newline back */
    /* (DOS folks probably want to #ifdef this).  To do this, the PopServer */
    /* buf *MUST* be larger than POPLIB_GETLINE_MAX -- see pop.h */
    fromserver[size++] = '\n';
    fromserver[size] = 0;
    dpipe_Write(dp, fromserver, size);
}

/*
 * Function: pop_retrieve
 *
 * Purpose: Retrieve a specified message from the maildrop.
 *
 * Arguments:
 * 	server	The server to retrieve from.
 * 	message	The message number to retrieve.
 *
 * Return value: A dpipe pointing to the message, if successful, or
 * 	null with pop_error set if not.
 */
struct dpipe *
pop_retrieve(server, message)
PopServer server;
int message;
{
    int *IDs, *sizes;
    struct dpipe *dp;

    if (!(dp = dputil_Create((dpipe_Callback_t)0,
			     (GENERIC_POINTER_TYPE *)0,
			      pop_read_server,
			     (GENERIC_POINTER_TYPE *)server, 0))) {
        strcpy(pop_error,
	    catgets(catalog, CAT_CUSTOM, 14, "Out of memory in pop_retrieve"));
	return (0);
    }
    if (pop_list(server, message, &IDs, &sizes))
	return (0);

    sprintf(pop_error, "RETR %d", message);
    if (sendline(server, pop_error) || getok(server)) {
	return (0);
    }
    server->pop_rset_flag = TRUE;
    free((char *) IDs);
    free((char *) sizes);

    return dp;
}

/* Function: pop_delete
 *
 * Purpose: Delete a specified message.
 *
 * Arguments:
 * 	server	Server from which to delete the message.
 * 	message	Message to delete.
 *
 * Return value: 0 on success, non-zero with error in pop_error
 * 	otherwise.
 */
int 
pop_delete(server, message)
PopServer server;
int message;
{
    int ret = 0;
    char buf[255];

    sprintf(buf, "DELE %d", message);
    if (sendline(server, buf) || getok(server)) {
	ret = -1;
    } else server->pop_rset_flag = TRUE;
    return ret;
}

/*
 * Function: pop_noop
 *
 * Purpose: Send a noop command to the server.
 *
 * Argument:
 * 	server	The server to send to.
 *
 * Return value: 0 on success, non-zero with error in pop_error
 * 	otherwise.
 */
int 
pop_noop(server)
PopServer server;
{
    int ret = 0;
    if (sendline(server, "NOOP") || getok(server))
	ret = -1;

    return ret;
}

/*
 * Function: pop_last
 *
 * Purpose: Find out the highest seen message from the server.
 *
 * Arguments:
 * 	server	The server.
 *
 * Return value: If successful, the highest seen message, which is
 * 	greater than or equal to 0.  Otherwise, a negative number with
 * 	the error explained in pop_error.
 */
int 
pop_last(server)
PopServer server;
{
    char *fromserver;
    int ret = -1;

    if (sendline(server, "LAST") == 0)
    {
	fromserver = poplib_getline(server);
	if (fromserver)
	{
	    if (!strncmp(fromserver, "-ERR", 4))
	    {
		pop_error[0] = 0;
		strncat(pop_error, fromserver+4, ERROR_MAX);
	    } else if (strncmp(fromserver, "+OK", 3))
		{
		    strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 15, "Unexpected response from server in pop_last" ));
		} else {
		    ret = atoi(&fromserver[4]);
		}
	}
    }
    return ret;
}

/*
 * Function: pop_reset
 *
 * Purpose: Reset the server to its initial connect state
 *
 * Arguments:
 * 	server	The server.
 *
 * Return value: 0 for success, non-0 with error in pop_error
 * 	otherwise.
 */
int 
pop_reset(server)
PopServer server;
{
    int ret = 0;
    if (sendline(server, "RSET") || getok(server))
	ret = -1;

    return ret;
}

/*
 * Function: pop_quit
 *
 * Purpose: Quit the connection to the server,
 *
 * Arguments:
 * 	server	The server to quit.
 *
 * Return value: 0 for success, non-zero otherwise with error in
 * 	pop_error.
 *
 * Side Effects: The PopServer passed in is unuseable after this
 * 	function is called, even if an error occurs.
 */
int 
pop_quit(server)
PopServer server;
{
    int ret = 0;

    if (sendline(server, "QUIT") || getok(server))
	ret = -1;

    SOCLOSE(server->file);
    free((char *) server);

    return (ret);
}

#ifndef MAC_OS
int get_ip_addr P((struct sockaddr_in *, const char *));

int
get_ip_addr(addr, ip)
    struct sockaddr_in *addr;
    const char *ip;
{
    int a, b, c, d;
    unsigned long e;
    
    if (sscanf(ip, "%d.%d.%d.%d", &a, &b, &c, &d) != 4)
	return 0;
    e = (a<<24)|(b<<16)|(c<<8)|d;
    addr->sin_addr.s_addr = ntohl(e);
    return 1;
}

/*
 * Function: socket_connection
 *
 * Purpose: Opens the network connection with the mail host, without
 * 	doing any sort of I/O with it or anything.
 *
 * Arguments:
 * 	host	The host to which to connect.
 *	flags	Option flags.
 * 	
 * Return value: A file descriptor indicating the connection, or -1
 * 	indicating failure, in which case an error has been copied
 * 	into pop_error.
 */
int 
socket_connection(host, flags)
const char *host;
int flags;
{
    struct hostent *hostent = NULL;
    struct hostserv *hserv = NULL;
    struct servent *servent;
    struct sockaddr_in addr;
    char found_port = 0;
    char *service;
    const char *ptr;
    int sock, i;
    int connected = 0;
    char hostnambuf[100];

#ifdef KERBEROS
    KTEXT ticket;
    MSG_DAT msg_data;
    CREDENTIALS cred;
    Key_schedule schedule;
    int rem;

#endif

    int try_count = 0;

    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;

    /* get port spec from host:port */
    ptr = index(host, ':');
    if (ptr) {
	addr.sin_port = htons(atoi(ptr+1));
	strcpy(hostnambuf, host);
	hostnambuf[ptr-host] = '\0';
	host = hostnambuf;
	found_port = 1;
    }

    if (!(flags & POP_NO_KERBEROS) || !get_ip_addr(&addr, host))
	do {
	    hostent = gethostbyname(host);
	    try_count++;
	    if ((!hostent) && ((h_errno != TRY_AGAIN) || (try_count == 5))) {
		strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 16, "Could not determine POP server's address: " ));
		strncat(pop_error, SOERRORSTR(),
		    ERROR_MAX - strlen(pop_error));
		return (-1);
	    }
	} while (!hostent);

    /* hostent may be NULL here! */
    
    /* on windows, we need to copy the hostent structure, because it
     * is not guaranteed to persist.
     */
    if (hostent)
	    hserv = hostserv_build(hostent, NULL);
    
#ifdef KERBEROS
    service = (flags & POP_NO_KERBEROS) ? POP_SERVICE : KPOP_SERVICE;
#else
    service = POP_SERVICE;
#endif

#ifdef HESIOD
    if (!found_port && !(flags & POP_NO_HESIOD)) {
        servent = hes_getservbyname(service, "tcp");
        if (servent) {
            addr.sin_port = servent->s_port;
            found_port = 1;
        }
    }
#endif
    if (!found_port) {
	servent = getservbyname(service, "tcp");
	if (servent) {
	    addr.sin_port = servent->s_port;
	} else {
#ifdef KERBEROS
	    addr.sin_port = htons((flags & POP_NO_KERBEROS) ?
		    POP_PORT : KPOP_PORT);
#else
	    addr.sin_port = htons(POP_PORT);
#endif
	}
    }
#define SOCKET_ERROR_MSG catgets( catalog, CAT_CUSTOM, 17, "Could not create socket for POP connection: " )

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == sock_Error) {
	strcpy(pop_error, SOCKET_ERROR_MSG);
	strncat(pop_error, SOERRORSTR(),
		ERROR_MAX - strlen(SOCKET_ERROR_MSG));
	if (hserv)
	    hostserv_destroy(hserv);
	return (-1);

    }
    i = 0;
    if (hserv)
	hostent = hserv->host;
    while (!hostent || hostent->h_addr_list[i]) {
	if (hostent)
	    bcopy(hostent->h_addr_list[i], (char *) &addr.sin_addr,
		hostent->h_length);
	if (!connect(sock, (struct sockaddr *) & addr, sizeof(addr))) {
	    connected = 1;
	    break;
	}
	if (!hostent) break;
	i++;
    }
    if (hserv)
	hostserv_destroy(hserv);
    hostent = NULL;

#define CONNECT_ERROR catgets( catalog, CAT_CUSTOM, 18, "Could not connect to POP server: " )

    if (!connected) {
	strcpy(pop_error, CONNECT_ERROR);
	strncat(pop_error, SOERRORSTR(),
		ERROR_MAX - strlen(CONNECT_ERROR));
	(void) SOCLOSE(sock);
	return (-1);

    }
#ifdef KERBEROS
    if (!(flags & POP_NO_KERBEROS)) {
	ticket = (KTEXT) malloc(sizeof(KTEXT_ST));
	rem = krb_sendauth(0L, sock, ticket, "pop", hostent->h_name,
		(char *) krb_realmofhost(hostent->h_name),
		(unsigned long) 0, &msg_data, &cred, schedule,
		(struct sockaddr_in *) 0,
		(struct sockaddr_in *) 0,
		"KPOPV0.1");
	free((char *) ticket);
	if (rem != KSUCCESS) {
#define KRB_ERROR catgets( catalog, CAT_CUSTOM, 19, "Kerberos error connecting to POP server: " )
	    strcpy(pop_error, KRB_ERROR);
	    strncat(pop_error, krb_err_txt[rem],
		    ERROR_MAX - sizeof(KRB_ERROR));
	    (void) SOCLOSE(sock);
	    return (-1);
	}
    }
#endif	/* KERBEROS */

    return (sock);
}/* socket_connection */

#endif /* MAC_OS */

/*
 * Function: poplib_getline
 *
 * Purpose: Get a line of text from the connection and return a
 * 	pointer to it.  The carriage return and linefeed at the end of
 * 	the line are stripped, but periods at the beginnings of lines
 * 	are NOT dealt with in any special way.
 *
 * Arguments:
 * 	server	The server from which to get the line of text.
 *
 * Returns: A non-null pointer if successful, or a null pointer on any
 * 	error, with an error message copied into pop_error.
 *
 * Notes: The line returned is overwritten with each call to poplib_getline.
 */
char *
poplib_getline(server)
PopServer server;
{
#define POPLIB_GETLINE_ERROR catgets( catalog, CAT_CUSTOM, 20, "Error reading from server: " )
    int ret;

    if (server->data) {
        char *cp = strstr(server->dp, CRLF);

        if (cp) {
            char *found;

            *cp = '\0';
            server->data -= (&cp[2] - server->dp);
            found = server->dp;
            server->dp = &cp[2];
#ifndef MAC_OS
            if (pop_debug)
                fprintf(stderr, "<<< %s\n", found);
#endif /* !MAC_OS */
            return (found);
        } else {
            bcopy(server->dp, server->buffer, server->data);
            server->dp = server->buffer;
        }
    } else {
        server->dp = server->buffer;
    }

    while (server->data < POPLIB_GETLINE_MAX) {
        ret = SOREAD(server->file, &server->buffer[server->data],
            POPLIB_GETLINE_MAX - server->data);
        if (ret < 0) {
            strcpy(pop_error, POPLIB_GETLINE_ERROR);
            strncat(pop_error, SOERRORSTR(),
                ERROR_MAX - strlen(POPLIB_GETLINE_ERROR));
            return (0);
        } else if (ret == 0) {
            strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 21, "Unexpected EOF from server in poplib_getline" ));
            return (0);
        } else {
            char *cp;

            server->data += ret;
            server->buffer[server->data] = 0;

            if ((cp = strstr(server->buffer, CRLF)) != 0) {
                *cp = '\0';
                server->data -= (&cp[2] - server->dp);
                server->dp = &cp[2];
#ifndef MAC_OS
                if (pop_debug)
                    fprintf(stderr, "<<< %s\n", server->buffer);
#endif /* MAC_OS */
                return (server->buffer);
            }
        }
    }

    // should never get here now

    strcpy(pop_error, catgets( catalog, CAT_CUSTOM, 22, "Line too long reading from server; compile pop.c with larger POPLIB_GETLINE_MAX" ));
    return (0);
}

/*
 * Function: sendline
 *
 * Purpose: Sends a line of text to the POP server.  The line of text
 * 	passed into this function should NOT have the carriage return
 * 	and linefeed on the end of it.  Periods at beginnings of lines
 * 	will NOT be treated specially by this function.
 *
 * Arguments:
 * 	server	The server to which to send the text.
 * 	line	The line of text to send.
 *
 * Return value: Upon successful completion, a value of 0 will be
 * 	returned.  Otherwise, a non-zero value will be returned, and
 * 	an error will be copied into pop_error.
 */
int 
sendline(server, line)
PopServer server;
const char *line;
{
#define SENDLINE_ERROR catgets( catalog, CAT_CUSTOM, 23, "Error writing to POP server: " )
    int ret = 0;

    if (*line)
	ret = fullwrite(server->file, line, strlen(line));
    if (ret >= 0) {	/* 0 indicates that a blank line was written */
	ret = fullwrite(server->file, CRLF, 2);
    }
    if (ret < 0) {
	strcpy(pop_error, SENDLINE_ERROR);
	strncat(pop_error, SOERRORSTR(),
		ERROR_MAX - strlen(SENDLINE_ERROR));
	return (ret);
    }
#ifndef MAC_OS
    if (pop_debug)
	fprintf(stderr, ">>> %s\n", line);
#endif
    return (0);
}

/*
 * Procedure: fullwrite
 *
 * Purpose: Just like write, but keeps trying until the entire string
 * 	has been written.
 *
 * Return value: Same as write.  Pop_error is not set.
 */
int 
fullwrite(fd, buf, nbytes)
int fd;
const char *buf;
int nbytes;
{
    const char *cp;
    int ret;

    if (nbytes == 0)	/* Write of 0 bytes may fail with -1, be we don't */
        return 0;

    cp = buf;
    while ((ret = SOWRITE(fd, cp, nbytes)) > 0) {
        cp += ret;
        nbytes -= ret;
        if (nbytes == 0) {
            ret = 0;	/* The next write, of 0 bytes, could fail with -1 */
            break;	/* So don't go around the loop another time */
        }
    }
    return ret == 0 ? cp - buf : ret;	/* Return bytes copied, or -1 */
}

/*
 * Procedure getok
 *
 * Purpose: Reads a line from the server.  If the return indicator is
 * 	positive, return with a zero exit status.  If not, return with
 * 	a negative exit status.
 *
 * Arguments:
 * 	server	The server to read from.
 * 
 * Returns: 0 for success, else for failure and puts error in pop_error.
 */
int 
getok(server)
PopServer server;
{
    char *fromline;
    int ret = -1;

    if (fromline = poplib_getline(server)) {
        if (!strncmp(fromline, "+OK", 3)) {
            ret = 0;
            pop_error[0] = '\0';
            if (fromline[3])
                strncat(pop_error, fromline+4, ERROR_MAX);
        }
        else if (!strncmp(fromline, "-ERR", 4)) {
            pop_error[0] = '\0';
            strncat(pop_error, fromline+4, ERROR_MAX);
        } else {
            strcpy(pop_error,
                catgets( catalog, CAT_CUSTOM, 24, "Unknown response from server; expecting +OK or -ERR" ));
        }
    }
    return ret;
}

/*
 * Function: gettermination
 *
 * Purpose: Gets the next line and verifies that it is a termination
 * 	line (nothing but a dot).
 *
 * Return value: 0 on success, non-zero with pop_error set on error.
 */
static int 
gettermination(server)
PopServer server;
{
    char *fromserver;
    int ret = -1;

    fromserver = poplib_getline(server);
    if (fromserver) {
        if (strcmp(fromserver, ".")) {
            strcpy(pop_error,
            catgets( catalog, CAT_CUSTOM, 25, "Unexpected response from server in gettermination" ));
        }
        else 
            ret = 0;
    }
    return (ret);
}

/*
 * Function pop_close
 *
 * Purpose: Close a pop connection, sending a "RSET" command to try to
 * 	preserve any changes that were made and a "QUIT" command to
 * 	try to get the server to quit, but ignoring any responses that
 * 	are received.
 *
 * Side effects: The server is unuseable after this function returns.
 * 	Changes made to the maildrop since the session was started (or
 * 	since the last pop_reset) may be lost.
 */
void 
pop_close(server)
PopServer server;
{
    static int closing;

    if (closing)
        return;
    closing = 1;
    if (server->ispop) {
	/* only send RSET and QUIT if this is a pop server
	 * (this routine might be called from poplib_getline(); if poplib_getline()
	 * is used to get data from an SMTP or directory server, we don't
	 * want to send RSET and QUIT to it; it causes problems with
	 * some winsocks (e.g. hangs when trying to close the connection)
	 * 
	 * Also note that sending RSET is an error if the pop server
         * is not in the "trans" (logged-in) state. The QUIT will still
	 * do the job, though.
	 */
	
        if (server->pop_rset_flag)
            sendline(server, "RSET");
        sendline(server, "QUIT");
    }

    SOCLOSE(server->file);
    free((char *) server);
    closing = 0;

    return;
}

#endif /* POP3_SUPPORT */
