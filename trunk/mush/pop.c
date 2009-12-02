/*
 * pop.c: client routines for talking to a POP3-protocol post-office
 * server
 *
 * Jonathan Kamens
 * August 13, 1991
 */

#include "config.h"

#ifdef POP3_SUPPORT

#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "pop.h"

extern int errno;

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

#include <pwd.h>
#include <string.h>
#include <strings.h>

#include <netdb.h>
#include <errno.h>
#include <stdio.h>

#ifndef __linux__
extern char *sys_errlist[];
#define strerror(eno)	sys_errlist[eno]
#endif

#ifdef KERBEROS
extern int 
krb_sendauth( /* long, int, KTEXT, char *, char *, char *, u_long, MSG_DAT **,
		CREDENTIALS *, Key_schedule, struct sockaddr_in *,
		struct sockaddr_in *, char * */ );
extern char *
krb_realmofhost( /* char * */ );
#endif /* KERBEROS */

extern int h_errno;

static int socket_connection();
static char *getline();
static int sendline();
static int fullwrite();
static int getok();
static int gettermination();

#define ERROR_MAX 80	/* a pretty arbitrary size */
#define POP_PORT 110
#define KPOP_PORT 1109
#define POP_SERVICE "pop"
#define KPOP_SERVICE "kpop"

char pop_error[ERROR_MAX];
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
char *host, *username, *password;
int flags;
{
    int sock;
    PopServer server;

    char* p = 0;

    /* Determine the user name */
    if ((p = getenv("MAILUSER")) && *p) {
	if (username) strcpy(username, p);
	username = p;
    }

    if (!username || !*username) {
#ifdef USE_USER
	p = getenv("USER");
	if (!p) p = getenv("LOGNAME");
#endif

	if (!(p && *p)) {
	    p = getlogin();
	    if (!(p && *p)) {
		struct passwd *passwd;

		passwd = getpwuid(getuid());
		if (passwd && passwd->pw_name && *passwd->pw_name) {
		    p = passwd->pw_name;
		} else {
		    strcpy(pop_error, "Could not determine username");
		    return (0);
		}
	    }
	}
	if (p && username) {
	    (void) strcpy(username, p);
	}
	username = p;
    }
    /* Determine the mail host */
#ifdef HESIOD
    if ((!host || !*host) && (!(flags & POP_NO_HESIOD))) {
	struct hes_postoffice *office;

	office = hes_getmailhost(username);
	if (office && office->po_type && (!strcmp(office->po_type, "POP"))
		&& office->po_name && *office->po_name && office->po_host
		&& *office->po_host) {
	    p = office->po_host;
	    if (p && host) {
		strcpy(host, p);
	    }
	    host = p;
	    p = office->po_name;
	    if (p && username) {
		strcpy(username, p);
	    }
	    username = p;
	}
    }
#endif
    if (!host || !*host) {
	host = getenv("MAILHOST");
	if (!host) {
	    strcpy(pop_error, "Could not determine POP server");
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
	    p = getpass("Enter POP password: ");
	    if (p && password)
		(void) strcpy(password, p);
	    password = p;
	}
	if (!password) {
	    strcpy(pop_error, "Could not determine POP password");
	    return (0);
	}
    }
    if (password)
	flags |= POP_NO_KERBEROS;
    else
	password = username;

    if (flags & POP_NO_OPEN) return 0;

    sock = socket_connection(host, flags);
    if (sock == -1)
	return (0);

    server = (PopServer) malloc(sizeof(struct _PopServer));
    if (!server) {
	strcpy(pop_error, "Out of memory in pop_open");
	return (0);
    }
    server->file = sock;
    server->data = 0;
    server->dp = server->buffer;

    if (getok(server))
	return (0);

    /*
     * I really shouldn't use the pop_error variable like this, but.... 
     */
    if (strlen(username) > ERROR_MAX - 6) {
	pop_close(server);
	strcpy(pop_error,
		"Username too long; recompile pop.c with larger ERROR_MAX");
	return (0);
    }
    sprintf(pop_error, "USER %s", username);

    if (sendline(server, pop_error) || getok(server)) {
	return (0);
    }
    if (strlen(password) > ERROR_MAX - 6) {
	pop_close(server);
	strcpy(pop_error,
		"Password too long; recompile pop.c with larger ERROR_MAX");
	return (0);
    }
    sprintf(pop_error, "PASS %s", password);

    if (sendline(server, pop_error) || getok(server)) {
	return (0);
    }
    return (server);
}

/*
 * Function: pop_stat
 *
 * Purpose: Issue the STAT command to the server and return (in the
 * 	value parameters) the number of messages in the maildrop and
 * 	the total size of the maildrop.
 *
 * Return value: 0 on success, or non-zero with an error in pop_error
 * 	in failure.
 *
 * Side effects: Closes the connection on failure.
 */
int 
pop_stat(server, count, size)
PopServer server;
int *count, *size;
{
    char *fromserver;

    if (sendline(server, "STAT") || (!(fromserver = getline(server))))
	return (-1);

    if (strncmp(fromserver, "+OK ", 4)) {
	strcpy(pop_error, "Unexpected response from POP server in pop_stat");
	pop_close();
	return (-1);
    }
    *count = atoi(&fromserver[4]);

    fromserver = index(&fromserver[4], ' ');
    if (!fromserver) {
	strcpy(pop_error,
		"Badly formatted response from server in pop_stat");
	pop_close(server);
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
 * Side effects: Closes the connection on error.
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
	strcpy(pop_error, "Out of memory in pop_list");
	pop_close(server);
	return (-1);
    }
    if (message) {
	sprintf(pop_error, "LIST %d", message);
	if (sendline(server, pop_error)) {
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	if (!(fromserver = getline(server))) {
	    free((char *) *IDs);
	    free((char *) *sizes);
	    return (-1);
	}
	if (strncmp(fromserver, "+OK ", 4)) {
	    if (!strncmp(fromserver, "-ERR", 4))
		strncpy(pop_error, fromserver, ERROR_MAX);
	    else
		strcpy(pop_error,
			"Unexpected response from server in pop_list");
	    free((char *) *IDs);
	    free((char *) *sizes);
	    pop_close(server);
	    return (-1);
	}
	(*IDs)[0] = atoi(&fromserver[4]);
	fromserver = index(&fromserver[4], ' ');
	if (!fromserver) {
	    strcpy(pop_error,
		    "Badly formatted response from server in pop_list");
	    free((char *) *IDs);
	    free((char *) *sizes);
	    pop_close(server);
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
	    if (!(fromserver = getline(server))) {
		free((char *) *IDs);
		free((char *) *sizes);
		return (-1);
	    }
	    (*IDs)[i] = atoi(fromserver);
	    fromserver = index(fromserver, ' ');
	    if (!fromserver) {
		strcpy(pop_error,
			"Badly formatted response from server in pop_list");
		free((char *) *IDs);
		free((char *) *sizes);
		pop_close(server);
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

/*
 * Function: pop_retrieve
 *
 * Purpose: Retrieve a specified message from the maildrop.
 *
 * Arguments:
 * 	server	The server to retrieve from.
 * 	message	The message number to retrieve.
 *
 * Return value: A string pointing to the message, if successful, or
 * 	null with pop_error set if not.
 *
 * Side effects: Closes the connection on error.
 */
char *
pop_retrieve(server, message)
PopServer server;
int message;
{
    int *IDs, *sizes;
    char *ptr, *cp;

    if (pop_list(server, message, &IDs, &sizes))
	return (0);

    sprintf(pop_error, "RETR %d", message);
    if (sendline(server, pop_error) || getok(server)) {
	return (0);
    }
    cp = ptr = (char *) malloc(sizes[0]+1);
    free((char *) IDs);
    free((char *) sizes);

    if (!ptr) {
	strcpy(pop_error, "Out of memory in pop_retrieve");
	pop_close(server);
	return (0);
    }
    *cp = '\0';

    while (1) {
	char *fromserver = getline(server);
	int size;

	if (!fromserver) {
	    free(ptr);
	    pop_close(server);
	    return (0);
	}
	if (fromserver[0] == '.') {
	    if (!fromserver[1]) {
		return (ptr);
	    }
	    fromserver++;
	}
	size = strlen(fromserver);
	bcopy(fromserver, cp, size + 1);
	cp += size;
	*cp++ = '\n';
	*cp = '\0';
    }
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
    sprintf(pop_error, "DELE %d", message);

    if (sendline(server, pop_error) || getok(server)) {
	pop_close(server);
	return (-1);
    }
    return (0);
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
 *
 * Side effects: Closes connection on error.
 */
int 
pop_noop(server)
PopServer server;
{
    if (sendline(server, "NOOP") || getok(server))
	return (-1);

    return (0);
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
 *
 * Side effects: Closes the connection on error.
 */
int 
pop_last(server)
PopServer server;
{
    char *fromserver;

    if (sendline(server, "LAST"))
	return (-1);

    if (!(fromserver = getline(server)))
	return (-1);

    if (!strncmp(fromserver, "-ERR", 4)) {
	strncpy(pop_error, fromserver, ERROR_MAX);
	pop_close(server);
	return (-1);
    } else if (strncmp(fromserver, "+OK", 3)) {
	strcpy(pop_error, "Unexpected response from server in pop_last");
	pop_close(server);
	return (-1);
    } else {
	return (atoi(&fromserver[4]));
    }
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
 *
 * Side effects: Closes the connection on error.
 */
int 
pop_reset(server)
PopServer server;
{
    if (sendline(server, "RSET") || getok(server)) {
	pop_close(server);
	return (-1);
    }
    return (0);
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

    close(server->file);
    free((char *) server);

    return (ret);
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
static int 
socket_connection(host, flags)
char *host;
int flags;
{
    struct hostent *hostent;
    struct servent *servent;
    struct sockaddr_in addr;
    char found_port = 0;
    char *service;
    int sock;

#ifdef KERBEROS
    KTEXT ticket;
    MSG_DAT msg_data;
    CREDENTIALS cred;
    Key_schedule schedule;
    int rem;

#endif

    int try_count = 0;

    do {
	hostent = gethostbyname(host);
	try_count++;
	if ((!hostent) && ((h_errno != TRY_AGAIN) || (try_count == 5))) {
	    strcpy(pop_error, "Could not determine POP server's address");
	    return (-1);
	}
    } while (!hostent);

    bzero((char *) &addr, sizeof(addr));
    addr.sin_family = AF_INET;

#ifdef KERBEROS
    service = (flags & POP_NO_KERBEROS) ? POP_SERVICE : KPOP_SERVICE;
#else
    service = POP_SERVICE;
#endif

#ifdef HESIOD
    if (!(flags & POP_NO_HESIOD)) {
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
#define SOCKET_ERROR "Could not create socket for POP connection: "

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
	strcpy(pop_error, SOCKET_ERROR);
	strncat(pop_error, strerror(errno),
		ERROR_MAX - sizeof(SOCKET_ERROR));
	return (-1);

    }
    while (*hostent->h_addr_list) {
	bcopy(*hostent->h_addr_list, (char *) &addr.sin_addr,
		hostent->h_length);
	if (!connect(sock, (struct sockaddr *) & addr, sizeof(addr)))
	    break;
	hostent->h_addr_list++;
    }

#define CONNECT_ERROR "Could not connect to POP server: "

    if (!*hostent->h_addr_list) {
	(void) close(sock);
	strcpy(pop_error, CONNECT_ERROR);
	strncat(pop_error, strerror(errno),
		ERROR_MAX - sizeof(CONNECT_ERROR));
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
#define KRB_ERROR "Kerberos error connecting to POP server: "
	    strcpy(pop_error, KRB_ERROR);
	    strncat(pop_error, krb_err_txt[rem],
		    ERROR_MAX - sizeof(KRB_ERROR));
	    (void) close(sock);
	    return (-1);
	}
    }
#endif	/* KERBEROS */

    return (sock);
}/* socket_connection */

/*
 * Function: getline
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
 * Notes: The line returned is overwritten with each call to getline.
 *
 * Side effects: Closes the connection on error.
 */
static char *
getline(server)
PopServer server;
{
#define GETLINE_ERROR "Error reading from server: "

    int ret;

    if (server->data) {
	char *cp = strstr(server->dp, "\r\n");

	if (cp) {
	    char *found;

	    *cp = '\0';
	    server->data -= (&cp[2] - server->dp);
	    found = server->dp;
	    server->dp = &cp[2];

	    if (pop_debug)
		fprintf(stderr, "<<< %s\n", found);
	    return (found);
	} else {
	    bcopy(server->dp, server->buffer, server->data);
	    server->dp = server->buffer;
	}
    } else {
	server->dp = server->buffer;
    }

    while (server->data < GETLINE_MAX) {
	ret = read(server->file, &server->buffer[server->data],
		GETLINE_MAX - server->data);
	if (ret < 0) {
	    strcpy(pop_error, GETLINE_ERROR);
	    strncat(pop_error, strerror(errno),
		    GETLINE_MAX - sizeof(GETLINE_ERROR));
	    pop_close(server);
	    return (0);
	} else if (ret == 0) {
	    strcpy(pop_error, "Unexpected EOF from server in getline");
	    pop_close(server);
	    return (0);
	} else {
	    char *cp = strstr(server->buffer, "\r\n");

	    server->data += ret;

	    if (cp) {
		*cp = '\0';
		server->data -= (&cp[2] - server->dp);
		server->dp = &cp[2];

		if (pop_debug)
		    fprintf(stderr, "<<< %s\n", server->buffer);
		return (server->buffer);
	    }
	}
    }

    strcpy(pop_error, "Line too long reading from server; compile pop.c with larger GETLINE_MAX");
    pop_close(server);
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
 *
 * Side effects: Closes the connection on error.
 */
static int 
sendline(server, line)
PopServer server;
char *line;
{
#define SENDLINE_ERROR "Error writing to POP server: "
    int ret;

    ret = fullwrite(server->file, line, strlen(line));
    if (ret >= 0) {	/* 0 indicates that a blank line was written */
	ret = fullwrite(server->file, "\r\n", 2);
    }
    if (ret < 0) {
	pop_close(server);
	strcpy(pop_error, SENDLINE_ERROR);
	strncat(pop_error, strerror(errno),
		ERROR_MAX - sizeof(SENDLINE_ERROR));
	return (ret);
    }
    if (pop_debug)
	fprintf(stderr, ">>> %s\n", line);

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
static int 
fullwrite(fd, buf, nbytes)
int fd;
char *buf;
int nbytes;
{
    char *cp;
    int ret;

    cp = buf;
    while ((ret = write(fd, cp, nbytes)) > 0) {
	cp += ret;
	nbytes -= ret;
    }

    return (ret);
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
 *
 * Side effects: Closes the connection on error.
 */
static int 
getok(server)
PopServer server;
{
    char *fromline;

    if (!(fromline = getline(server))) {
	return (-1);
    }
    if (!strncmp(fromline, "+OK", 3))
	return (0);
    else if (!strncmp(fromline, "-ERR", 4)) {
	strncpy(pop_error, fromline, ERROR_MAX);
	pop_error[ERROR_MAX - 1] = '\0';
	pop_close(server);
	return (-1);
    } else {
	strcpy(pop_error,
		"Unknown response from server; expecting +OK or -ERR");
	pop_close(server);
	return (-1);
    }
}

/*
 * Function: gettermination
 *
 * Purpose: Gets the next line and verifies that it is a termination
 * 	line (nothing but a dot).
 *
 * Return value: 0 on success, non-zero with pop_error set on error.
 *
 * Side effects: Closes the connection on error.
 */
static int 
gettermination(server)
PopServer server;
{
    char *fromserver;

    fromserver = getline(server);
    if (!fromserver)
	return (-1);

    if (strcmp(fromserver, ".")) {
	strcpy(pop_error,
		"Unexpected response from server in gettermination");
	pop_close(server);
	return (-1);
    }
    return (0);
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
    sendline(server, "RSET");
    sendline(server, "QUIT");

    close(server->file);
    free((char *) server);

    return;
}

#endif /* POP3_SUPPORT */
