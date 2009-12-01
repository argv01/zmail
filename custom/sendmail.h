/* sendmail.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.4 $
 * $Date: 1998/12/07 22:43:05 $
 * $Author: schaefer $
 */

#ifndef _SENDMAIL_H_
#define _SENDMAIL_H_


#include <stdio.h>
#include <general.h>
#include <zcsock.h>

struct _PopServer;

/*
 * function prototypes
 */
#ifdef MAC_OS
StreamPtr open_port P((struct _PopServer *ps, char *host, ip_port remotePort, int flags));
#else
Socket open_port P((struct _PopServer *ps, char *host, short port, int flags));
#endif /* MAC_OS */

Socket tcpcsocket P((char *host, short port));
int close_port P((Socket stream));
char *wsa_sockerror P((void));
char *wsa_sockerror_str P((int err));
#ifndef UNIX
int SMTP_close P((struct _PopServer **server));
#else
int SMTP_close P((PopServer server));
#endif
struct _PopServer *SMTP_open P((void));
int SMTP_sendfrom P((struct _PopServer *server));
int SMTP_sendto P((struct _PopServer *server, char *addr));
int SMTP_start_senddata P((struct _PopServer *server, FILE *fp));
int SMTP_senddata P((struct _PopServer *server, FILE *fp));
void SMTP_sendline P((struct _PopServer *server, char *s, int len));
int SMTP_sendend P((struct _PopServer *server));
int setup_sendmail P((char **addr_array));
int sendmail P((FILE *hdr_fp, FILE *body_fp));
int DSNetLookup P((const char *hostport, int max, const char *pat, char ***strs));
#ifdef CHECK_SMTP_ADDRS
int SMTP_testaddrs P((char **addr_array));

#endif /* CHECK_SMTP_ADDRS */

#endif /* _SENDMAIL_H_ */
