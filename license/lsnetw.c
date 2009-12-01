/* lsnetw.c	Copyright 1991-93 Z-Code Software Corp. */

#include "config.h"

#ifdef MAC_OS
# include "client.h"
# include "netdb.h"
#else
# include "license/client.h"
#endif /* MAC_OS */
#include <zctype.h>

#if defined(_XOPEN_SOURCE_EXTENDED)
#include <arpa/inet.h>
#endif

/* XXX why is this not unsigned? */
long
ls_gethostid(use_net_id)
int use_net_id;
{
    static unsigned long id = 0;

    if (id)
	return id;
#ifdef _SC_BSD_NETWORK
    /* Hope this is right...  pf Tue Aug 31 20:14:14 1993 */
    if (sysconf(_SC_BSD_NETWORK) < 0)
	use_net_id = 0;
#endif /* _SC_BSD_NETWORK */
#ifdef HAVE_HOSTENT
    if (use_net_id) {
	struct hostserv *hostserv;
	if ((hostserv =
		hostserv_getbyname(zm_gethostname(), HS_HOST_REQUIRED,
						     HS_SERV_NOT_REQUIRED,
						     HS_STATIC))) {
	    return (long)(
	    ((unsigned long)(unsigned char)(hostserv->host->h_addr[0]) << 24) +
	    ((unsigned long)(unsigned char)(hostserv->host->h_addr[1]) << 16) +
	    ((unsigned long)(unsigned char)(hostserv->host->h_addr[2]) <<  8) +
	    ((unsigned long)(unsigned char)(hostserv->host->h_addr[3]) <<  0)
	    );
	}
    }
#endif /* HAVE_HOSTENT */
    id = gethardwareid();
    if (id && id >= 100)
	return id;
    return id = ls_concoct_hostid(zm_gethostname());
}

long
ls_concoct_hostid(name)
char *name;
{
    Int32 it = 0;
    if (!name)
	return 0;
    for (;*name; name++)
	it = 127*it + (u_char)(isupper(*name) ? tolower(*name) : *name);
    return it;
}

#ifndef LICENSE_FREE

#ifdef HAVE_HOSTENT

#ifndef INRANGE
#define INRANGE(foo,bar,baz) ((foo(bar))&&((bar)baz))
#endif /* INRANGE */

char **
ls_read_malloced_argv_of_malloced_in_addrs()
{
    char buf[128];
    char **in_addrs = (char **)NULL, *latest;
    struct hostserv *hostserv;

    while (1) {
	if (fgets(buf, sizeof buf, stdin) == 0)
	    break;	/* you can hit ^D to finish if the os allows it*/
	buf[strlen(buf)-1] = '\0';
	if (strcmp(buf, ".") == 0)
	    break;

	if (!(hostserv = hostserv_getbyname(buf, HS_HOST_REQUIRED,
						 HS_SERV_REQUIRED,
						 HS_STATIC))) {
	    printf(catgets( catalog, CAT_LICENSE, 108, "\"%s\" is not a valid address:port (%s).\n" ), buf,
							    hostserv_errstr);
	    continue;
	}

	latest = (char *) malloc(30);
	if (!latest)
#ifndef WIN16
	    perror("malloc"), exit(1);
#else
	    exit(1);
#endif
	/*
	 * Convert back into a string in canonical form
	 */
	sock_AddrPortToStr(latest, hostserv->host->h_addr,
#ifdef WIN16
			   lpfnntohs(hostserv->serv->s_port));
#else
			   ntohs(hostserv->serv->s_port));
#endif
	ls_append_to_malloced_argv(&in_addrs, latest);
    }
    return in_addrs;
}

#endif /* HAVE_HOSTENT */

#endif /* LICENSE_FREE */
