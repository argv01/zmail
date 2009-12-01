/* poplib.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.6 $
 * $Date: 1995/10/05 04:53:34 $
 * $Author: liblit $
 */

#ifndef _POPLIB_H_
#define _POPLIB_H_

/*
 * Header file for the "poplib.c" client POP3 
 * protocol implementation.
 */
#ifdef POP3_SUPPORT

#include <general.h>
#ifndef ZC_INCLUDED_STDIO_H
# define ZC_INCLUDED_STDIO_H
# include <stdio.h>
#endif /* ZC_INCLUDED_STDIO_H */
#include "dpipe.h"

#define GETLINE_MAX 1024+128	/* a pretty arbitrary value */

#ifdef WIN16
extern char __far pop_error[];
#else
extern char pop_error[];
#endif

extern int pop_debug;

typedef struct _PopServer {
     int file, data, ispop;
     int pop_rset_flag;
     char buffer[GETLINE_MAX + 2], *dp;
} *PopServer;

/*
 * Valid flags for the pop_open function.
 */

#define POP_NO_KERBEROS	(1<<0)
#define POP_NO_HESIOD	(1<<1)
#define POP_NO_GETPASS 	(1<<2)

/*
 * Valid flags for the popchkmail function.
 */
#define POP_BACKGROUND	(1<<0)
#define POP_PRESERVE	(1<<1)

struct sockaddr_in;

/*
 * function prototypes
 */
PopServer pop_open P((const char *host, const char *username, char *password, int flags));
int pop_stat P((PopServer server, int *count, int *size));
int pop_list P((PopServer server, int message, int **IDs, int **sizes));
void pop_read_server P((struct dpipe *dp, GENERIC_POINTER_TYPE *rddata));
struct dpipe *pop_retrieve P((PopServer server, int message));
int pop_delete P((PopServer server, int message));
int pop_noop P((PopServer server));
int pop_last P((PopServer server));
int pop_reset P((PopServer server));
int pop_quit P((PopServer server));
void pop_close P((PopServer server));
int get_ip_addr P((struct sockaddr_in *addr, const char *ip));

/* additional, formerly static declarations from poplib.c */
char *poplib_getline P((PopServer server));
int sendline P((PopServer server, const char *line));
int fullwrite P((int fd, const char *buf, int nbytes));
int getok P((PopServer server));
#ifndef MAC_OS
int socket_connection P((const char *, int));
#endif /* !MAC_OS */

#endif /* POP3_SUPPORT */

#endif /* _POPLIB_H_ */
