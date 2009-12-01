/* client.h	Copyright 1993 Z-Code Software Corp. */

#ifndef _LS_CLIENT_H_
#define _LS_CLIENT_H_

/*
 * $RCSfile: client.h,v $
 * $Revision: 2.11 $
 * $Date: 1995/10/26 20:58:21 $
 * $Author: liblit $
 */

#include <config.h>		/* for UNIX macro */
#include <general.h>		/* for P() macro */

#include <zcsock.h>

#include <stdio.h>
#include <ctype.h>

#ifdef MAC_OS
# include "server.h"
#else /* !MAC_OS */
# include "license/server.h"
#endif /* !MAC_OS */

#ifdef HAVE_HOSTENT
#include "hostserv.h"
#endif /* HAVE_HOSTENT */

#include "catalog.h"
#include "hostname.h"
#include "lsutil.h"

/* 7/11/93 GF
 * Mac, and maybe other platforms, return time() from 1900, not 1970
 * (as unix does).  Add the appropriate offset so we're all comparing
 * the same date.
 */
#ifdef UNIX
# define SYS_TIME_OFFSET 0
#else
# ifdef MAC_OS
#  define SYS_TIME_OFFSET 2082844800
# else
#  define SYS_TIME_OFFSET 0
# endif
#endif /* UNIX */

#include "zcstr.h"
#include "zmalloc.h"

extern int ls_verbose;
extern char *ls_err;
extern char ls_errbuf[80];

extern void ls_long_to_key(), ls_makekey();
extern int AtoUL(), ls_is_a_password(), ls_argvlen();
extern void ls_append_to_malloced_argv(), ls_append_to_malloced_ulv();

extern char *ls_password_for_named_program P ((char *, unsigned long, unsigned long, long, char *, char *));
extern int ls_license_for_named_program P((char *, char *, unsigned long, unsigned long *, long *, char *, char *));
extern void ls_nls_client_give_back_token P((int));

extern int swrite P ((char *, unsigned int, unsigned int, sock_t));
extern int swritevar P ((char *, unsigned int, unsigned int, sock_t));
extern int swriterequest P ((char *, sock_t));
extern sock_t ls_call_server P ((char *, int, int));
extern int ls_recheck P ((char *, int));
extern int ls_access P ((char *, char *));
#ifndef LICENSE_FREE
extern void den_crypt P ((char [8], unsigned char [8], int));
#endif
extern int ls_is_custom_license P ((char *));
extern int ls_nls_client_connect_timeout P ((void));
extern int ls_nls_client_gethello_timeout P ((void));
extern int ls_nls_client_convers_timeout P ((void));

#endif /* _LS_CLIENT_H_ */
