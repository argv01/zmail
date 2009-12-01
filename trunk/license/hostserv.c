/*
 * hostserv.c
 *
 * Interface to gethostby{name,addr}() and getservby{name,port}()
 * that always works constently and digests a string
 * of the form host:port all at once.
 *
 * See hostserv.h for list of external functions and variables.
 *
 * See text of hostserv_getbyname() below for list of valid formats.
 *
 * Programs other than zmail who want to use this stuff
 * will have to compile with -DTHERE_IS_LIFE_BEYOND_ZMAIL.
 *
 * Don't you dare put anything else zmail-specific in this file.
 */

#ifndef lint
static char	hostserv_rcsid[] =
    "$Id: hostserv.c,v 2.21 1998/12/07 23:13:01 schaefer Exp $";
#endif

#ifdef MAC_OS
# include <netdb.h>
#endif /* MAC_OS*/

#ifdef THERE_IS_LIFE_BEYOND_ZMAIL
#define HAVE_HOSTENT
#else /* ZMAIL */
#include "config.h"	/* so HAVE_HOSTENT will be defined or not */
#include "catalog.h"
#endif /* ZMAIL */

#ifdef HAVE_HOSTENT

#include <zcsock.h>
#include <zctype.h>
#include <stdio.h>
#include <ctype.h>

#include "hostserv.h"

#include <bfuncs.h>

#include <zcalloc.h>
#include <zcunix.h>

#if defined(_XOPEN_SOURCE_EXTENDED)
#include <arpa/inet.h>
#endif

#ifndef INRANGE
#define INRANGE(foo,bar,baz) ((foo(bar))&&((bar)baz))
#endif /* INRANGE */

/*===========================================================================*/
/* String routines.
 * Zmail has equivalent routines,
 * but in the interest of making this able to stand alone...
 */

static void
str_free(p)	/* free something returned by str_dup */
char *p;
{
    if (p)
	free(p);
}

static char *
str_dup(p)
char *p;
{
    char *q;
    if (!p || !(q = (char *) malloc(strlen(p)+1)))
	return (char *)NULL;
    return ((char *) strcpy(q, p));
}

static int
argv_len(p)
char **p;
{
    int len = 0;
    if (p)
	for (; *p; p++)
	    len++;
    return len;
}

static void
argv_free(p)	/* free something returned by argv_dup or argvn_dup */
char **p;
{
    int i;
    if (p) {
	for (i = 0; p[i]; ++i)
	    free(p[i]);
	free((char *)p);
    }
}

static char **
argv_dup(p)
char **p;
{
    int i;
    char **q;
    if (!p || !(q = (char **)calloc(argv_len(p)+1, sizeof(char *))))
	return (char **)NULL;
    for (i = 0; p[i]; ++i) {
	if (!(q[i] = str_dup(p[i]))) {
	    argv_free(q);
	    return (char **)NULL;
	}
    }
    return q;
}

static char **
argvn_dup(p, n)	/* null-terminated array of char arrays each of length n */
char **p;
int n;
{
    int i;
    char **q;
    if (!p || !(q = (char **)calloc(argv_len(p)+1, sizeof(char *))))
	return (char **)NULL;
    for (i = 0; p[i]; ++i) {
	if (!(q[i] = (char *) malloc(n))) {
	    argv_free(q);
	    return (char **)NULL;
	}
	(void) bcopy(p[i], q[i], n);
    }
    return q;
}
/*===========================================================================*/


char *hostserv_errstr;


extern struct hostserv *
hostserv_build(host, serv)
struct hostent *host;
struct servent *serv;
{
    struct hostserv *hostserv;
    hostserv = (struct hostserv *) calloc(1, sizeof(struct hostserv));
    if (!hostserv)
	goto abort;

    hostserv->allocated = 1;

    if (host) {
	if (!(hostserv->host = (struct hostent *)
			       calloc(1, sizeof(struct hostent))))
	    goto abort;
	if (host->h_name)
	    if (!(hostserv->host->h_name = str_dup(host->h_name)))
		goto abort;
	if (host->h_aliases)
	    if (!(hostserv->host->h_aliases = argv_dup(host->h_aliases)))
		goto abort;
	if (host->h_addr_list)
	    if (!(hostserv->host->h_addr_list =
			   argvn_dup(host->h_addr_list, host->h_length)))
		goto abort;

	hostserv->host->h_addrtype = host->h_addrtype;
	hostserv->host->h_length = host->h_length;
    }

    if (serv) {
	if (!(hostserv->serv = (struct servent *)
			       calloc(1, sizeof(struct servent))))
	    goto abort;
	if (serv->s_name)
	    if (!(hostserv->serv->s_name = str_dup(serv->s_name)))
		goto abort;
	if (serv->s_aliases)
	    if (!(hostserv->serv->s_aliases = argv_dup(serv->s_aliases)))
		goto abort;
	if (serv->s_proto)
	    if (!(hostserv->serv->s_proto = str_dup(serv->s_proto)))
		goto abort;
	hostserv->serv->s_port = serv->s_port;
    }

    return hostserv;

abort:
    hostserv_destroy(hostserv);
    hostserv_errstr = catgets( catalog, CAT_LICENSE, 95, "Out of memory." );
    return (struct hostserv *) NULL;
}


extern struct hostserv *
hostserv_dup(hostserv)
struct hostserv *hostserv;
{

    return hostserv ? hostserv_build(hostserv->host, hostserv->serv) : 0;
}


/*
 * Valid formats:
 *	host:serv
 *	host:
 *	:serv
 *	host
 * where host is a hostname or an address in dot notation,
 * and serv is a service name or port number.
 * If either of host or serv is omitted, the corresponding field in
 * the returned hostserv will be set to NULL.
 *
 * The call to getservby{name,port}() uses proto="tcp".
 */
extern struct hostserv *
hostserv_getbyname(name, host_required, serv_required, allocate)
char *name;
int host_required;	/* if true, return NULL if can't get hostent */
int serv_required;	/* if true, return NULL if can't get servent */
int allocate;		/* whether to allocate and not just use static */
{
    static struct hostserv hostserv;
    static struct hostent hs_gbn_host;
    static struct servent hs_gbn_serv;
    char hostname[40], servname[40];
    int i;

    if (strlen(name)+1 > sizeof(hostname)) {
	hostserv_errstr = catgets( catalog, CAT_LICENSE, 96, "String too long" );
	return (struct hostserv *) NULL;
    }

    hostname[0] = '\0';
    servname[0] = '\0';
    if (sscanf(name, ":%s", servname) != 1)
        (void) sscanf(name, "%[^:]:%s", hostname, servname);

    if (host_required && !hostname[0]) {
	hostserv_errstr = catgets( catalog, CAT_LICENSE, 97, "host not specified" );
	return (struct hostserv *) NULL;
    }
    if (serv_required && !servname[0]) {
	hostserv_errstr = catgets( catalog, CAT_LICENSE, 98, "port not specified" );
	return (struct hostserv *) NULL;
    }


    if (hostname[0]) {
	int intaddr[4];
	char addr[4], dummy;
	if (sscanf(hostname, "%d.%d.%d.%d%c",
	       &intaddr[0], &intaddr[1], &intaddr[2], &intaddr[3], &dummy) == 4
		 && INRANGE(0 <=, intaddr[0], < 256)
		 && INRANGE(0 <=, intaddr[1], < 256)
		 && INRANGE(0 <=, intaddr[2], < 256)
		 && INRANGE(0 <=, intaddr[3], < 256)) {
	    for (i = 0; i < 4; ++i)
		addr[i] = intaddr[i];
	    if (!(hostserv.host =
#ifndef _WINDOWS
		gethostbyaddr((void *)addr, 4, AF_INET)
#else /* _WINDOWS */
		/* gethostbyaddr() causes a hang sometimes on PC-NFS's
		 * winsock, so just don't bother
		 */
	        NULL
#endif /* _WINDOWS */
		)) {
		/*
		 hostserv_errstr = catgets( catalog, CAT_LICENSE, 99, "gethostbyaddr() failed" );
		 return (struct hostserv *) NULL;
		*/
		/*
		 * I don't give a rat's ass if there is no name
		 * for the address, that's no reason for gethostbyaddr
		 * to fail.
		 */
		static char _name[16];
		static char *_aliases[] = { NULL };
		static char _addr[4];
		static char *_addr_list[] = { _addr, NULL };
		static struct hostent host =
			{ _name, _aliases, AF_INET, 4, _addr_list };
		/* don't strcpy, or might overflow if input had extra zeros */
		sprintf(host.h_name, "%d.%d.%d.%d",
			intaddr[0], intaddr[1], intaddr[2], intaddr[3]);
		bcopy(addr, host.h_addr_list[0], 4);
		hostserv.host = &host;
	    }
	} else {
	    if (!(hostserv.host = gethostbyname(hostname))) {
		if (host_required) {
		    hostserv_errstr = catgets( catalog, CAT_LICENSE, 100, "gethostbyname() failed" );
		    return (struct hostserv *) NULL;
		}
	    }
	}
    } else
	hostserv.host = (struct hostent *) NULL;

    /* Bart: Tue Jun 22 15:53:49 PDT 1993
     * It appears that gethost*() and getserv*() may use overlapping
     * storage on some machines.  Copy the hostent so it doesn't get
     * clobbered before we manage to return it.
     */
    if (hostserv.host) {
	hs_gbn_host = *(hostserv.host);
	hostserv.host = &hs_gbn_host;
    }

    if (servname[0]) {
	for (i = 0; servname[i]; i++)
	    if (!isdigit(servname[i]) && !(i == 0 && servname[i] == '-'))
		break;
	if (!servname[i]) {	/* it's a number */
	    if (!(hostserv.serv = getservbyport(atoi(servname), "tcp"))) {
		/*
		 hostserv_errstr = catgets( catalog, CAT_LICENSE, 101, "getservbyport() failed" );
		 return (struct hostserv *) NULL;
		*/
		/*
		 * I don't give a rat's ass if there is no name
		 * for the service, that's no reason for getservbyport
		 * to fail.
		 */
		static char _name[16];
		static char *_aliases[] = { NULL };
		static struct servent serv = { _name, _aliases, 0, "tcp" };

		strcpy(serv.s_name, servname);
		serv.s_port = htons(atoi(servname));
		hostserv.serv = &serv;
	    }
	} else {
	    if (!(hostserv.serv = getservbyname(servname, "tcp"))) {
		if (serv_required) {
		    hostserv_errstr = catgets( catalog, CAT_LICENSE, 102, "getservbyname() failed" );
		    return (struct hostserv *) NULL;
		}
	    }
	}
    } else
	hostserv.serv = (struct servent *) NULL;

    /* Bart: Tue Jun 22 15:53:49 PDT 1993
     * It appears that gethost*() and getserv*() may use overlapping
     * storage on some machines.  Copy the servent so it doesn't get
     * clobbered before the caller can dereference it.
     */
    if (hostserv.serv && !allocate) {
	hs_gbn_serv = *(hostserv.serv);
	hostserv.serv = &hs_gbn_serv;
    }

    return allocate ? hostserv_dup(&hostserv)
		    : &hostserv;
}

extern void
hostserv_destroy(hostserv)
struct hostserv *hostserv;
{
    struct hostent *host;
    struct servent *serv;

    if (!hostserv || !hostserv->allocated)
	return;

    if (host = hostserv->host) {
	str_free(host->h_name);
	argv_free(host->h_aliases);
	argv_free(host->h_addr_list);
	free((char *)host);
    }

    if (serv = hostserv->serv) {
	str_free(serv->s_name);
	argv_free(serv->s_aliases);
	str_free(serv->s_proto);
	free((char *)serv);
    }

    free((char *)hostserv);
}

#if defined(MAIN) || defined(HOSTSERV_PRINT)

extern void
hostent_fprint(fp, hostent)
FILE *fp;
struct hostent *hostent;
{
    char **charp;

    if (!hostent) {
	(void) fprintf(fp, "\t(null)\n");
	return;
    }

    (void) fprintf(fp, "\th_name: \"%s\"\n", hostent->h_name);
    (void) fprintf(fp, "\th_aliases:\n");
    for (charp = hostent->h_aliases; *charp; charp++)
	(void) fprintf(fp, "\t\t\"%s\"\n", *charp);
    (void) fprintf(fp, "\th_addrtype: %d\n", hostent->h_addrtype);
    (void) fprintf(fp, "\th_length: %d\n", hostent->h_length);
    (void) fprintf(fp, "\th_addr_list:\n");
    for (charp = hostent->h_addr_list; *charp; charp++)
	(void) fprintf(fp, "\t\t%d.%d.%d.%d\n",
	    (int)(unsigned char)charp[0][0],
	    (int)(unsigned char)charp[0][1],
	    (int)(unsigned char)charp[0][2],
	    (int)(unsigned char)charp[0][3]);
}

extern void
servent_fprint(fp, servent)
FILE *fp;
struct servent *servent;
{
    char **charp;

    if (!servent) {
	(void) fprintf(fp, "\t(null)\n");
	return;
    }

    (void) fprintf(fp, "\ts_name: \"%s\"\n", servent->s_name);
    (void) fprintf(fp, "\ts_aliases:\n");
    for (charp = servent->s_aliases; *charp; charp++)
	(void) fprintf(fp, "\t\t\"%s\"\n", *charp);
    (void) fprintf(fp, "\ts_port: %d\n", ntohs(servent->s_port));
    (void) fprintf(fp, "\ts_proto: \"%s\"\n", servent->s_proto);
}

extern void
hostserv_fprint(fp, hostserv)
FILE *fp;
struct hostserv *hostserv;
{
    if (!hostserv) {
	(void) fprintf(fp, "(null)\n");
	return;
    }
    hostent_fprint(fp, hostserv->host);
    fprintf(fp, "\n");
    servent_fprint(fp, hostserv->serv);
    fprintf(fp, "\n");
}


extern void
hostent_print(hostent)
struct hostent *hostent;
{
    hostent_fprint(stdout, hostent);
}

extern void
servent_print(servent)
struct servent *servent;
{
    servent_fprint(stdout, servent);
}

extern void
hostserv_print(hostserv)
struct hostserv *hostserv;
{
    hostserv_fprint(stdout, hostserv);
}

#endif /* MAIN || HOSTSERV_PRINT */


#ifdef MAIN	/* little test program */

main(argc, argv)
char **argv;
{
    char buf[100];
    struct hostserv *hostserv;

    while ((void) printf("Enter a host and port: "), gets(buf)) {
	hostserv = hostserv_getbyname(buf, 0, 0, 1);
	hostserv_print(hostserv);
	if (!hostserv)
	    (void) printf("hostserv_getbyname(\"%s\") failed: %s\n",
					buf, hostserv_errstr);
	hostserv_destroy(hostserv);
    }
}
#endif /* MAIN */

#endif /* HAVE_HOSTENT */
