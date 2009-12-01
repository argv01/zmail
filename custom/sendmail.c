/* sendmail.c */
/* Copyright (c)  1993, 1994 Z-Code Software Corp. */

#ifndef lint
static char	sendmail_rcsid[] =
    "$Id: sendmail.c,v 2.40 2005/05/09 09:11:50 syd Exp $";
#endif

#include "config.h"
#include <zcsock.h>
#ifdef UNIX
#include <netdb.h>
#include <zmcomp.h>
#endif
#include "zmail.h"
#include <zmdebug.h>
#include "zmaddr.h"
#include "zcerr.h"
#include "vars.h"
#include "pop.h"
#include "sendmail.h"

#undef CR
#undef LF
#define CR 13
#define LF 10
#define cSendMail_Port 25
#define SMTP_ERRCODE	"500"
#define SMTP_OK		"25"

#ifdef UNIX
#define SOCKET_ERROR -1
#define closesocket close
#endif

#ifdef POP3_SUPPORT
int get_ip_addr P((struct sockaddr_in *, const char *));
static  PopServer gPostinfo = (PopServer) 0;

#ifndef MAC_OS


Socket
tcpcsocket(host, port)
char *host;
short port;
{
	Socket s;
	struct sockaddr_in sa;
	struct hostent *hent;

	if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) == SOCKET_ERROR)
	    return s;

	if (!get_ip_addr(&sa, host)) {
	    if (! (hent = (struct hostent *) gethostbyname(host))) {
		closesocket(s);
#ifndef UNIX
		WSASetLastError(WSAHOST_NOT_FOUND);
#endif
		return SOCKET_ERROR;
	    }
	    memcpy((char *) &sa.sin_addr, hent->h_addr, hent->h_length);
	}

	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);

	if (connect(s, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
	    closesocket(s);
	    return SOCKET_ERROR;
	}

	return s;
}

Socket open_port(ps, host, port, flags)
PopServer ps;
char *host;
short port;
int flags;
{
    Socket s;
    
    s = tcpcsocket(host, port);
    return s;
}

int close_port(stream)
Socket stream;
{
    return (closesocket(stream) != SOCKET_ERROR) ? 0 : -1;
}

# ifdef _WINDOWS
struct wsa_err {
    int errcode;
    char *string;
} wsa_errs[] = {
    { WSAEINTR, "interrupted" },
    { WSAEBADF, "bad file number" },
    { WSAEACCES, "permission denied" },
    { WSAEFAULT, "bad address" },
    { WSAEINVAL, "invalid argument" },
    { WSAEMFILE, "too many open files" },
    { WSAEWOULDBLOCK, "operation would block" },
    { WSAEINPROGRESS, "operation in progress" },
    { WSAEALREADY, "operation already in progress" },
    { WSAENOTSOCK, "not a socket" },
    { WSAEDESTADDRREQ, "destination address required" },
    { WSAEMSGSIZE, "message too long" },
    { WSAEPROTOTYPE, "protocol wrong type for socket" },
    { WSAENOPROTOOPT, "protocol not available" },
    { WSAEPROTONOSUPPORT, "protocol not supported" },
    { WSAESOCKTNOSUPPORT, "socket type not supported" },
    { WSAEOPNOTSUPP, "operation not supported" },
    { WSAEPFNOSUPPORT, "protocol family not supported" },
    { WSAEAFNOSUPPORT, "address family not supported" },
    { WSAEADDRINUSE, "address already in use" },
    { WSAEADDRNOTAVAIL, "can't assign requested address" },
    { WSAENETDOWN, "network is down" },
    { WSAENETUNREACH, "network is unreachable" },
    { WSAENETRESET, "network dropped connection on reset" },
    { WSAECONNABORTED, "connection aborted" },
    { WSAECONNRESET, "connection reset by peer" },
    { WSAENOBUFS, "no buffer space available" },
    { WSAEISCONN, "socket is already connected" },
    { WSAENOTCONN, "socket is not connected" },
    { WSAESHUTDOWN, "can't send after socket shutdown" },
    { WSAETOOMANYREFS, "too many references: can't splice" },
    { WSAETIMEDOUT, "connection timed out" },
    { WSAECONNREFUSED, "connection refused" },
    { WSAELOOP, "too many levels of symbolic links" },
    { WSAENAMETOOLONG, "file name too long" },
    { WSAEHOSTDOWN, "host is down" },
    { WSAEHOSTUNREACH, "no route to host" },
    { WSAENOTEMPTY, "directory not empty" },
    { WSAEDQUOT, "disc quota exceeded" },
    { WSAESTALE, "stale NFS file handle" },
    { WSAEREMOTE, "too many levels of remote in path" },
    { WSASYSNOTREADY, "network subsystem unusable" },
    { WSANOTINITIALISED, "winsock not initialized" },
    { WSAVERNOTSUPPORTED, "version not supported" },
    { WSAHOST_NOT_FOUND, "host not found" },
    { WSANO_DATA, "no data record of requested type" },
    { 0, NULL }
};

char *
wsa_sockerror()
{
    int err = WSAGetLastError();
    return wsa_sockerror_str(err);
}

char *
wsa_sockerror_str(err)
int err;
{
    struct wsa_err *we;

    for (we = wsa_errs; we->errcode; we++)
	if (we->errcode == err)
	    return we->string;
    return "Unknown error";
}

# endif /* _WINDOWS */

#endif /* !MAC_OS */


/* get_status.  try to get an anticipated return code from a server.  return 0 if */
/* successful, -1 otherwise.  check for "-" after code -- indicates multi-line reply */
static int 
get_status(code, server)
char *code;
PopServer server;
{
    char *fromline;
    int l = strlen(code);

    do {
    	if (!(fromline = poplib_getline(server))) {
	    Debug(" Error getting line from SMTP server!");
	    return (-1);
	}
	Debug(" SMTP: %s\n", fromline);
    	if (strncmp(fromline, code, l)) {
	    Debug(" Error getting response code from SMTP server! Expected %s\n", code);
            return (-1);
	}
    } while (fromline[3] == '-');
    return 0;
}

int SMTP_close(server)
PopServer server;
{
    int ret = 0;

    Debug(" Sending QUIT to SMTP server.\n");
    strcpy(server->buffer, "QUIT");
    if (sendline(server, server->buffer) || get_status("221",server))
	ret = -1;

    close_port(server->file);
    if (server == gPostinfo)
#ifndef UNIX
    	gPostinfo = nil;
#else
    	gPostinfo = NULL;
#endif
    free(server);
    return (ret);
} /* SMTP_close */


PopServer SMTP_open()
{
    char *host, *us;
    Socket sock;
    PopServer server;
    int port = cSendMail_Port;
    char buf[255], *p;

    errno = 0;
    if (!(host = value_of(VarSmtphost))) {
        error(SysErrWarning, "Error sending mail: no SMTP server specified");
        return 0;
    } else {
        strcpy(buf, host);
        if (p = strrchr(buf, ':')) {
            *p++ = 0;
            host = buf;
            if (!(port = atoi(p)))
                port = cSendMail_Port;
        }
    }
	
    if (!(server = (PopServer) malloc(sizeof(struct _PopServer)))) {
        error(SysErrWarning, "Out of memory in SMTP_open");
        return 0;
    }
    server->ispop = POP3_NONE;

    sock = open_port(server, host, port, 0); /* no flags */

    if (sock == -1) {
        free(server);
        error(SysErrWarning, "Error opening SMTP connection\nCode = %s",
            SOERRORSTR());
        return 0;
    }

    server->file = sock;
    server->data = 0;
    server->dp = server->buffer;

    if (get_status("220", server)) {
        error(SysErrWarning, "SMTP:\nError opening connection to server:\n%s",server->buffer);
        SMTP_close(server);
        return 0;
    }
		
    strcpy (server->buffer, "HELO ");
    if ((us = get_full_hostname()) != NULL)
        strcat(server->buffer, us);
    
    Debug("Saying \"%s\" to SMTP server\n", server->buffer);
    if (sendline(server, server->buffer) || get_status(SMTP_OK, server)) {
        error(SysErrWarning, "SMTP:\nError saying 'HELO' to server:\n%s",server->buffer);
        SMTP_close(server);
        return 0;
    }
	
    return (server);
} /* SMTP_open */


int SMTP_sendfrom(server)
PopServer server;
{
    char *me;
    char *myhost;
	
    errno = 0;
    if (!(me = value_of(VarUser))) {
        error(SysErrWarning, "Error sending mail: no sender name specified");
        return (-1);
    }
    if (ison(comp_current->mta_flags, MTA_HIDE_HOST))
        myhost = NULL;
    else
        myhost = get_from_host(TRUE, FALSE);
    if (myhost && *myhost) {
        if (ison(comp_current->mta_flags, MTA_ADDR_UUCP))
            (void) sprintf(server->buffer, "MAIL FROM:<%s!%s>", myhost, me);
        else
            (void) sprintf(server->buffer, "MAIL FROM:<%s@%s>", me, myhost);
        xfree(myhost);
    } else
        (void) sprintf(server->buffer, "MAIL FROM:<%s>", me);
    Debug(" Sending STMP %s", server->buffer);
    if (sendline(server, server->buffer) || get_status(SMTP_OK, server)) {
        error(SysErrWarning, "SMTP:\nError sending 'MAIL FROM:' to server:\n%s",server->buffer);
        return (-1);
    }
    return 0;
} /* SMTP_sendfrom */

int SMTP_sendto(server, addr)
PopServer server;
char *addr;
{

    Debug(" SMTP RCPT TO:<%s>\n", addr);
    (void) sprintf(server->buffer, "RCPT TO:<%s>", addr);
    if (sendline(server, server->buffer) || get_status(SMTP_OK, server))
	return (-1);

    return 0;
} /* SMTP_sendto */


int SMTP_start_senddata(server, fp)
PopServer server;
FILE *fp;
{

    if (!fp)
        return (-1);
		
    (void) strcpy(server->buffer, "DATA");

    Debug(" Sending STMP DATA command\n");
    if (sendline(server, server->buffer) || get_status("354", server)) {
	error(SysErrWarning, "SMTP:\nError sending 'DATA' to server:\n%s",server->buffer);
	return (-1);
    }
    while (fgets(server->buffer, GETLINE_MAX, fp)) {
	int len = strlen(server->buffer);
	if (len && server->buffer[len-1] == '\n')
	    len--;
	SMTP_sendline(server, server->buffer, len);
    }

    return (0);
} /* SMTP_start_senddata */


#define STATE_START 0
#define STATE_MIDDLE 1
#define STATE_KEYWORD 2
#define NEWLINE '\n'
#define DOT '.'
#define KEYWORD "From "
#define KEYWORD_FIRST 'F'
#define QUOTE_CHAR '>'
#ifdef MAC_OS
# define BUFFER_SIZE BUFSIZ
#else /* !MAC_OS */
# define BUFFER_SIZE (BUFSIZ * 4)
#endif /* MAC_OS */

int SMTP_senddata(server, fp)
PopServer server;
FILE *fp;
{
    static char key_buffer[] = KEYWORD;
    char *key_ptr;
    char *key_last;
    char *in_buffer;
    char *out_buffer;
    int state;
    char *match_ptr;
    int ret;

    in_buffer = malloc(BUFFER_SIZE);
    if (in_buffer == NULL)
    {
	Debug("can't allocate input buffer");
	return 1;
    }
    out_buffer = malloc(2 * BUFFER_SIZE);
    if (out_buffer == NULL)
    {
	Debug("can't allocate output buffer");
	free(in_buffer);
	return 1;
    }

    state = STATE_START;
    key_ptr = key_buffer;
    key_last = key_ptr + strlen(key_ptr) - 1;

    for (;;)
    {
	int in_length;
	register char *in_ptr;
	register char *in_last;
	register char *out_ptr;
	register int c;
	char *tmp_ptr;
	int out_length;
	int written;

	in_length = (int) fread(in_buffer, sizeof (char), BUFFER_SIZE, fp);
	if (in_length <= 0)
	{
	    if (in_length == 0)
		goto done;
	    Debug("error in read");
	    goto fail;
	}

	in_ptr = in_buffer;
	in_last = in_ptr + in_length - 1;
	out_ptr = out_buffer;

	switch (state)
	{
	  case STATE_START:
	    goto start;
	  case STATE_MIDDLE:
	    goto in_middle;
	  case STATE_KEYWORD:
	    goto in_keyword;
	  default:
	    Debug("bad state");
	    goto fail;
	}

      start:
	if (in_ptr > in_last)
	    goto save_start;
	c = *in_ptr++;
	switch (c)
	{
	  default:
	    goto start_middle;
	  case NEWLINE:
	    goto newline;
	  case DOT:
	    goto dot;
	  case KEYWORD_FIRST:
	    goto start_keyword;
	}

      start_middle:
	*out_ptr++ = c;

      in_middle:
	/* the real inner loop */
	for (;;)
	{
	    if (in_ptr >= in_last)
		goto crunch;
	    c = *in_ptr++;
	    if (c == NEWLINE)
		goto newline;
	    *out_ptr++ = c;
	    c = *in_ptr++;
	    if (c == NEWLINE)
		goto newline;
	    *out_ptr++ = c;
	}

      newline:
	*out_ptr++ = CR;
	*out_ptr++ = LF;
	goto start;

      dot:
	*out_ptr++ = DOT;
	*out_ptr++ = DOT;
	goto in_middle;

      start_keyword:
	match_ptr = key_ptr + 1;

      in_keyword:
	if (in_ptr > in_last)
	    goto save_keyword;
	c = *in_ptr++;
	if (c != *match_ptr)
	    goto mismatch;
	if (match_ptr >= key_last)
	    goto match;
	match_ptr++;
	goto in_keyword;

      mismatch:
	tmp_ptr = key_ptr;
	while (tmp_ptr < match_ptr)
	    *out_ptr++ = *tmp_ptr++;
	if (c != NEWLINE)
	    goto start_middle;
	goto newline;

      match:
	*out_ptr++ = QUOTE_CHAR;
	tmp_ptr = key_ptr;
	while (tmp_ptr <= key_last)
	    *out_ptr++ = *tmp_ptr++;
	if (c != NEWLINE)
	    goto in_middle;
	goto newline;

      crunch:
	if (in_ptr > in_last)
	    goto save_middle;
	c = *in_ptr++;
	if (c == NEWLINE)
	    goto newline;
	*out_ptr++ = c;
	goto save_middle;

      save_start:
	state = STATE_START;
	goto write_it;

      save_middle:
	state = STATE_MIDDLE;
	goto write_it;

      save_keyword:
	state = STATE_KEYWORD;
	goto write_it;

      write_it:
	out_length = out_ptr - out_buffer;
	written = fullwrite(server->file, out_buffer, out_length);
	if (written != out_length)
	{
	    Debug("error in write");
	    goto fail;
	}
    }

  done:
    if (state != STATE_START)
    {
	static char crlf[2] = { CR, LF };

	(void) fullwrite(server->file, crlf, 2);
	Debug("message ends in middle of line");
    }
    ret = 0;
    goto frees;

  fail:
    ret = 1;
    goto frees;

  frees:
    free(in_buffer);
    free(out_buffer);

    return ret;

}

#ifdef CHECK_SMTP_ADDRS
int SMTP_testaddrs(addr_array)
char **addr_array;
{
    int ret = -1;
    PopServer psPtr;
    errno = 0;

    if (!addr_array)
    	return ret;
    timeout_cursors(TRUE);
    if (psPtr = SMTP_open()) {
	if (SMTP_sendfrom(psPtr) == 0) {
	    while (*addr_array) {
		if (SMTP_sendto(psPtr, *addr_array)) {
		    error(SysErrWarning, "SMTP:\nError sending to address\n  %s\nThis message has not been sent",*addr_array);
		    break;
		}
		++addr_array;
	    }
	    if (!*addr_array)
	    	ret = 0;
	}
	if (sendline(psPtr, "RSET") == 0)
	    get_status(SMTP_OK, psPtr);
	SMTP_close(psPtr);
    }
    timeout_cursors(FALSE);
    return ret;	
} /* SMTP_testaddrs */
#endif /* CHECK_SMTP_ADDRS */

void
SMTP_sendline(server, s, len)
PopServer server;
char *s;
int len;
{
    char sav = s[len];

    s[len] = 0;
    if (*s == '.')
	fullwrite(server->file, ".", 1);
    sendline(server, s);
    s[len] = sav;
}

int SMTP_sendend(server)
PopServer server;
{

    (void) strcpy(server->buffer, ".");

    if (sendline(server, server->buffer) || get_status(SMTP_OK, server)) {
	error(SysErrWarning, "SMTP:\nError terminating conversation with server:\n%s",server->buffer);
	return (-1);
    }
    return 0;
} /* SMTP_sendend */


int setup_sendmail(addr_array)
char **addr_array;
{
    errno = 0;
    
    
    Debug("*** setup_sendmail():\n");

    if (!(gPostinfo = SMTP_open()))
	return (-1);
    if (SMTP_sendfrom(gPostinfo)) {
	SMTP_close(gPostinfo);
	return (-1);
    }

    while (*addr_array) {
	if (SMTP_sendto(gPostinfo, *addr_array)) {
	    error(SysErrWarning, "SMTP:\nError sending to address\n  %s\nThis message has not been sent",*addr_array);
	    SMTP_close(gPostinfo);
	    return (-1);
	}
	++addr_array;
    }
    return 0;	
} /* setup_sendmail */


int sendmail(hdr_fp, body_fp)
FILE *hdr_fp;
FILE *body_fp;
{
    int ret = -1;
    FMODE_SAVE();
    FMODE_BINARY();

    errno = 0;

    Debug("*** sendmail():\n");

#ifndef UNIX
    if (!(gPostinfo = SMTP_open()))
#endif
    if (gPostinfo) {
        if (SMTP_start_senddata(gPostinfo, hdr_fp) == 0)
	    if (SMTP_senddata(gPostinfo, body_fp) == 0)
		if (SMTP_sendend(gPostinfo) == 0)
		    ret = 0;
    }
    SMTP_close(gPostinfo);
    FMODE_RESTORE();
    return ret;

} /* sendmail */


#endif /* POP3_SUPPORT */



#define kDefaultDSPort 15213

#ifdef MAC_OS
# include "dirserv.seg"
#endif /* MAC_OS */
/* GF 4/26/94 -- directory service lookup across the net;  mostly for Mac & Win */
/* expect hostport format: "<host>:<port>"; default to kDefaultDSPort. */
/* send command in the form: "<max hit #> <pattern>" */
int DSNetLookup(hostport, maxx, pat, strs)
const char *hostport;
int maxx;
const char *pat;
char ***strs;
{
    Socket aStream;
    PopServer server;
    char *p, *s;
    int port = 0, ret = -1, n = 0;
    char host[255], cmd[255];

    if (!hostport)
	return (-1);
	/* wouldn't want to poke a static buffer */
    strcpy(host, hostport);
    if (p = strchr(host, ':')) {
	*p++ = 0;
	if (sscanf(p, "%ud", &port) != 1)
	    port = kDefaultDSPort;
    } else port = kDefaultDSPort;
    sprintf(cmd, "%d %s", maxx, pat);

    Debug("Performing address check.\n host=%s, port=%d\n", host, port);
    if (!(server = (PopServer) malloc(sizeof(struct _PopServer)))) {
	Debug(" couldn't allocate memory for lookup connection!\n");
	return (-1);
    }
    server->ispop = POP3_NONE;
    aStream = open_port(server, host, port, 0);
    if (aStream == -1) {
	Debug("  address server connection failed!\n");
	free(server);
	return (-1);
    }
    server->file = aStream;
    server->data = 0;
    server->dp = server->buffer;

    Debug("  Sending lookup string = %s\n", cmd);
    if (sendline(server, cmd)) {
	close_port(aStream);
	free(server);
	Debug("  error sending lookup string!\n    %s\n", pop_error);
	return (-1);
    }	
    Debug("  lookup returned:\n\n");
    if (s = poplib_getline(server)) {
        sscanf(s, "%ud", &ret);
	if (debug)
	    Debug("  exit status: %d\n", ret);
	do {
	    if (!(s = poplib_getline(server)))
		break;
	    n = catv(n, strs, 1, unitv(s));
	    Debug("   addr: %s\n\n", s);
	} while (n != -1);
    }

    Debug("closing port.\n");
    close_port(aStream);
    Debug("freeing server.\n");
    free(server);
    Debug("lookup completed.\n");

    return ret;
} /* DSNetLookup */
