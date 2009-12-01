/*
 Copyright (c) 1993 - 1995 Z-Code Software Corporation, All rights reserved.
 This is proprietary and confidential information, and a trade secret of Z-Code
 Software Corporation.  Unauthorized use, disclosure, distribution or
 reproduction is strictly prohibited.
*/

/* 8/25/93 GF : created */
/* 2/11/95 GF : gutted of old synchronization code */

#include "zmail.h"
#ifdef MAC_OS
# undef when
# undef otherwise
#endif /* MAC_OS */
#include "fsfix.h"
#include "pop.h"
#include "glist.h"
#include "dputil.h"
#include "dynstr.h"
#include "excfns.h"
#include "mailserv.h"
#include <general.h>

#ifdef _WINDOWS
extern char *gUserPass;
#endif

/* pop_check_connect:  take all the necessary connection info, try to make a connection, */
/* quit, and return the results */
int
pop_check_connect(host, user, pword)
const char *host, *user, *pword;
{
    PopServer pserv;
    int ret = -1;

    if (!host || !*host || !user || !*user || !pword || !*pword)
	return ret;
    errno = 0;
    pop_initialized = 1;	/* gets 0 if pword failed */
    pserv = pop_open(host, user, pword, POP_NO_GETPASS);
    if (!pserv)
    {
    	if (pop_initialized)
	    error(SysErrWarning, "POP:\nError opening connection with post office: \n\n%s", pop_error);
	else error(WarnOk, "POP:\nYou did not enter the correct password.");
	pop_initialized = 0;
    } else
    {
    	ret = 0;
	if (pop_quit(pserv) == -1)
	    error(SysErrWarning, "POP:\nError closing connection with post office: \n\n%s", pop_error);
    }
    return ret;
} /* pop_check_connect */

PopServer
zync_open(host, uname, pword, flags)
const char *host, *uname, *pword;
unsigned long flags;
{
    long sock;
    int port = ZYNCPORT;
    PopServer server;
    char buf[64], *p;

    errno = 0;
    if (!host || !*host || !uname || !*uname || !pword || !*pword)
	return NULL;
    
    strcpy(buf, host);
    if (p = strrchr(buf, ':')) {
	*p++ = 0;
	if (!(port = atoi(p)))
	    port = ZYNCPORT;
    }

    server = (PopServer) malloc(sizeof(struct _PopServer));
    if (!server) {
	error(SysErrWarning, "Z-POP:\nCouldn't allocate memory for connection");
	return NULL;
    }
    server->ispop = True;
    server->pop_rset_flag = False;
#ifndef UNIX
    sock = open_port(server, buf, port, flags);
#else /* UNIX */
    sock = socket_connection(buf, flags);
#endif /* UNIX */
    if (sock == -1) {
    	error(SysErrWarning, "Z-POP:\nCouldn't connect user '%s' to server %s at port %d.  Please check your configuration",
			uname, buf, port);
	free(server);
	return NULL;
    }

    server->file = sock;
    server->data = 0;
    server->dp = server->buffer;

    if (getok(server)) {
	error(SysErrWarning, "Z-POP:\nDidn't get proper handshake from server");
	goto Bail;
    }
    
    sprintf(buf, "USER %s", uname);
    if (sendline(server, buf) || getok(server)) {
	error(SysErrWarning, "Z-POP:\nError issuing USER command");
	goto Bail;
    }

    sprintf(buf, "PASS %s", pword);
    if (sendline(server, buf) || getok(server)) {
	/* look for "lock" problem, else assume pword bad */
	if (!strstr(pop_error, "lock")) {
	    error(SysErrWarning, "Z-POP:\nYou did not enter the correct password");
	    pop_initialized = 0;
	} else error(SysErrWarning, "Z-POP:\nError establishing connection:\n\n%s", pop_error);
	goto Bail;
    } else if (!pop_initialized) {
	pop_initialized = 1;
	mailserver_SetPassword(mserv, pword);
    }

    zync_moi(server);
    return (server);

Bail:
    pop_quit(server);
    return NULL;

} /* zync_open */


/* zync_get_prefs:  get or make a connection, issue GPRF command to Zync */
/* return -1 for error, or stuff prefs into file, source & return 0 */
zync_get_prefs(PopServer ps) {
    char buf[MAXPATHLEN], cmd[32], *p;
    Boolean make_connect = !ps;
    FILE *pref_fp = NULL;
    int i, res;
    char *pass = NULL;

    res = -1;
    errno = 0;
    if (!(p = alloc_tempfname("prf"))) {
    	error(SysErrWarning, "Z-POP:\nCouldn't find temp directory");
	return res;
    }
    strcpy(buf, p);
    xfree(p);
    if (!(pref_fp = fopen(buf, "w"))) {
    	error(SysErrWarning, "Z-POP:\nCouldn't open temp preferences file");
	return res;
    }
    if (make_connect) {
	pass = mailserver_GetPassword(mserv);
#ifdef _WINDOWS
	if ((!pass) && (*gUserPass))
	{
		mailserver_SetPassword (mserv, gUserPass);
		pass = &gUserPass[0];
	}
#endif
	if (!(ps = zync_open(getenv("ZYNCHOST"), getenv("USER"), pass, 0))) {
	    fclose(pref_fp);
	    return res;
	}
    }
    strcpy(cmd, "GPRF");
    if (sendline(ps, cmd) || getok(ps)) {
	error(SysErrWarning, "Z-POP:\nError sending 'get preferences' command:\n\n%s", pop_error);
	goto Bail;
    }
    	/* Are there prefs to be had? */
    if (strncmp((char *)(ps->buffer + 4), "Pref", 4))  /* "+OK " */
	goto Bail;  /* nothing to do */

    while (TRUE) {
	if (!(p = getline(ps)))
	    goto Bail;
	if (p[0] == '.' && !p[1])
	    break;
	if (!fprintf(pref_fp, "%s\n", p) || feof(pref_fp))
	    goto Bail;
    }
    fclose(pref_fp);
    pref_fp = NULL;
    (void) cmd_line(zmVaStr("source %s", quotezs(buf, 0)), NULL);
    res = 0;

Bail:
    if (pref_fp)
	(void) fclose(pref_fp);
    (void) remove(buf);
    if (make_connect)
	(void) pop_quit(ps);
    return res;

} /* zync_get_prefs */



/* zync_set_prefs */
int zync_set_prefs() {
    char *argv[5], buf[32], tmp[MAXPATHLEN], *t;
    int i, n, ret = 0;
    PopServer ps;
    FILE *FP;
    char *pass = NULL;

    pass = mailserver_GetPassword(mserv);

    errno = 0;
    ps = zync_open(getenv("ZYNCHOST"), getenv("USER"), pass, 0);
    if (!ps) {
   	error(SysErrWarning, "Z-POP:\nError opening connection for remote preferences");
	return ret;    
    }
    strcpy(tmp, alloc_tempfname("prf"));

    argv[0] = "saveopts";
    argv[1] = "-f";
    argv[2] = tmp;
    argv[3] = NULL;

    i = 2;
    if (save_opts(i, argv) || !(FP = fopen(tmp, "r"))) {
	i = remove(tmp);
   	error(SysErrWarning, "Z-POP:\nError writing remote preferences in temp folder");
	return ret;
    }
    
    strcpy(buf, "SPRF");
    if (!sendline(ps, buf) && !getok(ps)) {
	ret = 1;
	do {
	    i = fread(ps->buffer, sizeof(char), BUFSIZ - 1, FP);
	    if (i) {
#ifdef MAC_OS
		MAC_CRTONL(ps->buffer, i);
#endif /* MAC_OS */
		n = fullwrite(ps->file, ps->buffer, i);
		if (n < 0) {
		    error(SysErrWarning, "Z-POP:\nError sending remote preferences");
		    ret = -1;
		    break;
		}
	    }
        } while (i && !feof(FP));

	strcpy(buf, ".");	/* smtp-like EOF */
	if ((sendline(ps, buf) ||  getok(ps)) && ret != -1)
	    error(SysErrWarning, "Z-POP:\nError ending remote preferences transmission");
    } else {
	error(UserErrWarning, "Z-POP:\nError sending remote preferences:\n\n%s", pop_error);
    }
    (void) fclose(FP);
    (void) remove(tmp);
    pop_quit(ps);
    if (ret > 0)
	wprint("Saved preferences on server.\n");
    return ret;    
} /* zync_set_prefs */


/* once we know we've got a good connection to the server, do Z-POP config stuff */
int zync_do_config() {
    char *pass, *p;
    PopServer ps;
    int ret = -1;
    char buf[32];

    pass = mailserver_GetPassword(mserv);

    errno = 0;
    if (!(p = getenv("ZMLIB")))
    	return ret;
    if (!(ps = zync_open(getenv("ZYNCHOST"), getenv("USER"), pass, 0)))
    	return ret;
    strcpy(buf, "ZHAV");
    if (sendline(ps, buf) || getok(ps))
    	goto Bail;    
    zync_describe_files(ps, p);
    if (getok(ps)) {
	errno = 0;
	error(SysErrWarning, "Z-POP:\nError sending server configuration files:\n\n%s", pop_error);
	goto Bail;
    }
    TRY {
	strcpy(buf, "ZGET");
	if (sendline(ps, buf) || getok(ps))
	    goto Bail;
	zync_update_files(ps, p, NULL);
	ret = 0;
    } EXCEPT (except_ANY) {
	goto Bail;
    } ENDTRY;
Bail:
    pop_quit(ps);
    return ret;        
}
