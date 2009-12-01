/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/types.h>
#include <stdio.h>
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */
#include <errno.h>
#include <sys/stat.h>
#include "zctime.h"
#include <ctype.h>
#include "zync_version.h"

/* #include <sys/socket.h> */ /* now in popper.h */
#include <netinet/in.h>
/* #include <netdb.h> */ /* now in popper.h */

#ifdef BIND43
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#include "popper.h"

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#include <bfuncs.h>
#include <general/strcase.h>
#include <general/excfns.h>

#define LIB_STRUCTURE_GLIST_GROWSIZE 5

static const char pop_init_rcsid[] =
    "$Id: pop_init.c,v 1.43 1996/04/22 23:43:46 spencer Exp $";

extern char *inet_ntoa();

/* Debug keys and associated values. */
struct {
    char* key;
    int val;
} debug_flags[] = {
    {"ERRORS", DEBUG_ERRORS},
    {"WARNINGS", DEBUG_WARNINGS|DEBUG_ERRORS},
    {"COMMANDS", DEBUG_COMMANDS|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"VERBOSE", DEBUG_VERBOSE|DEBUG_COMMANDS|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"CONNECTION", DEBUG_CONNECTION|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"CONFIG", DEBUG_CONFIG|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"MAILDROP", DEBUG_MAILDROP|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"LIBRARY", DEBUG_LIBRARY|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"EXEC", DEBUG_EXEC|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"UPLOAD", DEBUG_UPLOAD|DEBUG_WARNINGS|DEBUG_ERRORS},
    {"ALL", -1},
    {NULL, 0}
};

/*
 * Fetch the next word from an open file.  Skips initial whitespace, and
 * returns NULL if EOF is encountered during the skipping.  Otherwise
 * returns the same buffer that was passed in.
 */
static char *
getword(file, length, buffer)
    FILE *file;
    int length;
    char *buffer;
{
    char *ptr;
    char *end;
    int c;

    ptr = buffer;
    /* skip leading whitespace */
    do {
	if (EOF == (c = getc(file))) {
	    *ptr = 0;
	    return NULL;
	}
    } while (isspace(c));
    end = buffer + length - 1;
    do {
	*ptr++ = c;
	if (ptr >= end)
	    break;
	c = getc(file);
    } while (c != EOF && !isspace(c));
    *ptr = 0;
    return buffer;
}

/*  Debugging trace file specified */
int
trace(p, fullname)
    POP *p;
    const char *fullname;
{
    char *trace_prefix;

    if (p->trace) {
      fclose(p->trace);
      p->trace = NULL;
    }
    if ((p->trace = fopen(fullname, "a+")) == NULL) {
	if (p->debug & DEBUG_ERRORS)
	    pop_log(p, POP_PRIORITY, "Unable to open trace file %s: %s.",
		    fullname, strerror(errno));
	return -1;
    }
    trace_prefix = malloc(16);
    if (trace_prefix == NULL)
	return -1;
    sprintf(trace_prefix, "[%d] ", p->pid);
    p->trace_prefix = trace_prefix;
    return 0;
}


/* specify an idle timeout */
int
timeout(p, timeout_string)
    POP *p;
    const char *timeout_string;
{
    if (sscanf(timeout_string, "%u", &p->idle_timeout) != 1) {
	if (p->debug & DEBUG_ERRORS) 
	    pop_log(p, POP_PRIORITY,
		    "Invalid configuration-file timeout value: %s",
		    timeout_string);
	return -1;
    } else
	return 0;
}


void
setup_debug_flags(p, debug_keys)
     POP *p;
     const char *debug_keys;
{
    char *name;

    for (name=strtok(debug_keys, " \n\r\t");
	 name != NULL; name=strtok(NULL, " \n\r\t")) {
	int i;
	for (i=0; debug_flags[i].key; i++) {
	    if (ci_strcmp(name, debug_flags[i].key) == 0) {
		p->debug |= debug_flags[i].val;
		break;
	    }
	}
    }
}

void
get_spool_format(p, opts)
    POP *p;
    const char *opts;
{
    char *name;

    for (name = strtok(opts, " \n\r\t");
	 name != NULL; name = strtok(NULL, " \n\r\t")) {
	if (ci_strcmp(name, "none") == 0)
	    p->spool_format = (unsigned long)0;
	else if (ci_strcmp(name, "content-length") == 0)
	    p->spool_format = /* this is unnecessary now, but just... */
		(p->spool_format | CONTENT_LENGTH) & ~MMDF_SEPARATORS;
	else if (ci_strcmp(name, "mmdf") == 0)
	    p->spool_format = /* ... in case we add other options later. */
		(p->spool_format | MMDF_SEPARATORS) & ~CONTENT_LENGTH;
	else
	    pop_log(p, POP_PRIORITY,
		    "Unrecognized argument to spool-format: %s", name);
    }
}

void
get_message_separator(p, sep)
    POP *p;
    const char *sep;
{
    char *p2;

    dynstr_Set(&p->msg_separator, 0);
    p2 = sep;
    while (*p2 && (' ' == *p2 || '\t' == *p2))
	++p2;
    while (*p2 && *p2 != '\n') {
	if ('\\' == *p2) {
	    ++p2;
	    if (strspn(p2, "0123456789") > 2) {
		int c;
		sscanf(p2, "%3o", &c);
		dynstr_AppendChar(&p->msg_separator, (char)c);
		p2 += 3;
	    } else if (*p2)
		dynstr_AppendChar(&p->msg_separator, *p2++);
	} else
	    dynstr_AppendChar(&p->msg_separator, *p2++);
    }
}
	    
    
  
#ifndef HAVE_STRDUP
char *
strdup(string)
    const char *string;
{
    char *ret;

    ret = (char *)emalloc(strlen(string) + 1, "zpop_strdup");
    strcpy(ret, string);
    return ret;
}
#endif /* !HAVE_STRDUP */

extern struct glist *newErrGlist();

#define copy_optarg(x) ((x)[sizeof(x)-1]='\0', strncpy((x), optarg, sizeof(x)-1))

extern char *any P((const char *, const char *)); /* pop_dropinfo.c */

int
pop_init(p, argcount, argmessage, progname)
    POP *p;
    int argcount;
    char **argmessage;
    char *progname;
{

    struct sockaddr_in cs;
    struct hostent *ch;
    int errflag = 0;
    int c;
    int len;
    extern char *optarg;
    int options = 0;
    int sp = 0;
    char *t;
    FILE *config_file;
    struct stat statbuf;
    char config_name[MAXPATHLEN];
    char lib_structure[MAXLINELEN];
    char *name;

    /* Initialize the POP parameter block */
    bzero(p, (sizeof (POP)));

    p->errGlist = newErrGlist();

    /* Save my name in a global variable */
    p->myname = argmessage[0];

    /* Get the name of our host */
    gethostname(p->myhost,MAXHOSTNAMELEN);

    /* create time stamp for APOP/UIDL */
    sprintf(p->time_stamp, "<%lu.%lu@%s>", (unsigned long)getpid(),
	    (unsigned long)time((time_t *)0), zm_gethostname ());
    p->time_stamp_len = strlen(p->time_stamp);

    /*  Open the log file */
#ifdef HAVE_SYSLOG_43
    openlog(p->myname,POP_LOGOPTS,POP_FACILITY);
#else /* !HAVE_SYSLOG_32 */
    openlog(p->myname,0);
#endif /* !HAVE_SYSLOG_32 */

    /* set default library path */
    strcpy(p->lib_path,  DEFAULT_LIBPATH);

    /* set default idle timeout */
    p->idle_timeout = DEFAULT_IDLE_TIMEOUT;
    p->additional_timeout = 0;
    p->speed = -1;

    /* set default config file */
    strcpy(config_name, DEFAULT_CONFIG);
    
    /* set default library structure */
    strcpy(lib_structure, DEFAULT_LIBRARY_STRUCTURE);

    /* set default debug level */
    setup_debug_flags(p, DEFAULT_DEBUG_KEYS);

    /* set MTA options */
    p->spool_format = (unsigned long)0;
    dynstr_Init(&p->msg_separator);
    get_message_separator(p, MSG_SEPARATOR);

    /* record process id */
    p->pid = getpid();

    /*  Process command line arguments */
    while ((c = getopt(argcount,argmessage,"d:t:s:S:m:T:p:l:c:z:o:")) != EOF) {
	switch (c) {
	    case 'd': /* Debugging requested */
		setup_debug_flags(p, optarg);
		break;
	    case 't': /* Debugging trace file specified */
		trace(p, optarg);
		break;
	    case 's': /* spool format */
		get_spool_format(p, optarg);
		break;
	    case 'S': /* message separator */
		get_message_separator(p, optarg);
		break;
	    case 'm': /* 9/5/93 GF specify spool path */
		copy_optarg(p->dropname);
		break;
	    case 'T':
		copy_optarg(p->tmpdropname);
		break;
	    case 'p': /* 9/7/93 GF specify pref path */
		copy_optarg(p->pref_path);
		break;
	    case 'l': /* specify library path */
		copy_optarg(p->lib_path);
		break;
	    case 'c': /* specify location of config file */
		copy_optarg(config_name);
		break;
	    case 'z': /* specify idle timeout */
		timeout(p, optarg);
		break;
	    case 'o': /* specify library organization */
		copy_optarg(lib_structure);
		break;
	    case 'n': /* specify message upload dir */
		copy_optarg(p->newmsgpath);
		break;
	    default: /* Unknown option received */
		if (p->debug & DEBUG_WARNINGS) 
		    pop_log(p, POP_PRIORITY, 
			    "Warning: unrecognized command-line option: -%c",
			    c);
		errflag++;
		break;
	}
    }

    /* fetch configuration options */
    if (config_file = fopen(config_name, "r")) {
	char word[MAXLINELEN];
	while (!feof(config_file)) {
	    if (getword(config_file, sizeof word, word) == NULL)
		break;
	    if (word[0] == '#')				
		for (;;) {
		    c = getc(config_file);
		    if (c == '\n' || c == EOF)
			break;
		}
	    else if (ci_strcmp(word, "debug") == 0) {
		fgets(word, sizeof word, config_file);
		setup_debug_flags(p, word);
	    } else if (ci_strcmp(word, "trace") == 0) {
		getword(config_file, sizeof word, word);
		trace(p, word);
	    }
	    else if (ci_strcmp(word, "spool") == 0)
		getword(config_file, sizeof p->dropname, p->dropname);
	    else if (ci_strcmp(word, "tempcopy") == 0)
		getword(config_file, sizeof p->tmpdropname, p->tmpdropname);
	    else if (ci_strcmp(word, "prefs") == 0)
		getword(config_file, sizeof p->pref_path, p->pref_path);
	    else if (ci_strcmp(word, "library") == 0)
		getword(config_file, sizeof p->lib_path, p->lib_path);
	    else if (ci_strcmp(word, "timeout") == 0) {
		getword(config_file, sizeof word, word);
		errflag += !!timeout(p, word);
	    }
	    else if (ci_strcmp(word, "library-structure") == 0)
		fgets(lib_structure, sizeof lib_structure, config_file);
	    else if (ci_strcmp(word, "message-upload") == 0)
		getword(config_file, sizeof p->newmsgpath, p->newmsgpath);
	    else if (ci_strcmp(word, "spool-format") == 0) {
		fgets(word, sizeof word, config_file);
		get_spool_format(p, word);
	    } else if (ci_strcmp(word, "message-separator") == 0) {
		fgets(word, sizeof word, config_file);
		get_message_separator(p, word);
	    } else if (p->debug & DEBUG_ERRORS) {
		pop_log(p, POP_PRIORITY, 
			"Unrecognized configuration-file option:  %s", word);
		errflag++;
	    }
	}
	fclose(config_file);
    }

    /* Don't allow timeouts shorter than 10 minutes, per the spec */
    if (p->idle_timeout && (p->idle_timeout < 600))
	p->idle_timeout = 600;

    /* GF 9/5/93  set default spool path */
    if (!(p->dropname[0]))
	strcpy(p->dropname, POP_MAILDIR);

    if (!(p->tmpdropname[0]))
	strcpy(p->tmpdropname, "%s/.%u.pop");

    /* GF 9/7/93  set default pref path */
    if (!(p->pref_path[0]))
	strcpy(p->pref_path,  DEFAULT_PREFPATH);
    if (stat((char *)p->pref_path, &statbuf)) {
	if (!(t = getenv("TMPDIR")))
	    t = DEFTMPDIR;
	if (p->debug & DEBUG_ERRORS) {
	  pop_log(p, POP_PRIORITY, "Can't stat prefs directory %s: %s",
		  p->pref_path, strerror(errno));
	  pop_log(p, POP_PRIORITY, "Using %s instead.\n", t);
	}
	strcpy(p->pref_path, t);
    }

    /* GF 9/7/93  set default message upload path */
    if (!(p->newmsgpath[0]))
	strcpy(p->newmsgpath,  DEFAULT_NEWMSGPATH);
    if (stat((char *)p->newmsgpath, &statbuf)) {
	if (!(t = getenv("TMPDIR")))
	    t = DEFTMPDIR;
	if (p->debug & DEBUG_ERRORS) {
	  pop_log(p, POP_PRIORITY, "Can't stat msg upload directory %s: %s",
		  p->pref_path, strerror(errno));
	  pop_log(p, POP_PRIORITY, "Using %s instead.\n", t);
	}
	strcpy(p->newmsgpath, t);
    }

    /* setup library structure */
    p->lib_structure = (struct glist *)malloc(sizeof(struct glist));
    glist_Init(p->lib_structure, sizeof(char **), 
	       LIB_STRUCTURE_GLIST_GROWSIZE);
    for (name=strtok(lib_structure, " \n\r\t"); 
	 name!=NULL; 
	 name=strtok(NULL, " \n\r\t")) {
	name = strdup(name);
	glist_Add(p->lib_structure, &name);
    }
    
#ifdef SANS_SOCKETS

    p->client = p->ipaddr = "<unknown>";
    p->input = stdin;
    p->output = stdout;

#else /* !SANS_SOCKETS */

    /* Get the address and socket of the client to whom I am speaking */
    len = sizeof(cs);
    if (getpeername(sp, (struct sockaddr *)&cs, &len) < 0) {
	pop_log(p, POP_PRIORITY,
		"Unable to obtain socket and address of client, err = %d",errno);
	exit(-1);
    }

    /* Save the dotted decimal form of the client's IP address 
       in the POP parameter block */
    p->ipaddr = inet_ntoa(cs.sin_addr);

    /* Save the client's port */
    p->ipport = ntohs(cs.sin_port);

    /* Get the canonical name of the host to whom I am speaking */
    ch = gethostbyaddr((char *)&cs.sin_addr, sizeof(cs.sin_addr), AF_INET);
    if (ch == NULL) {
	if (p->debug & DEBUG_CONNECTION)
	    pop_log(p, POP_PRIORITY,
		    "Unable to get canonical name of client, err = %d",errno);
	p->client = p->ipaddr;
    }
   
    /* Save the cannonical name of the client host in the POP parameter block */
    else {

#ifndef BIND43

	p->client = strdup(ch->h_name);

#else

	/* Distrust distant nameservers */
#if 0
	extern struct RES_STATE _res;
#endif
	struct hostent * ch_again;
	char **addrp;

	p->client = strdup(ch->h_name);

	/* We already have a fully-qualified name */
	/* _res.options &= ~RES_DEFNAMES; */
	/* Yeah, well, bite me.  Better to have this commented out. */

	/* See if the name obtained for the client's IP 
	   address returns an address */
	if ((ch_again = gethostbyname(p->client)) == NULL) {
	    if (p->debug & DEBUG_CONNECTION)
		pop_log(p, POP_PRIORITY,
			"Client at \"%s\" resolves to an unknown host name \"%s\"",
			p->ipaddr, ch->h_name);
	    p->client = p->ipaddr;
	} else {
      
	    /* Look for the client's IP address in the list returned 
	       for its name */
	    for(addrp=ch_again->h_addr_list; *addrp; ++addrp)
		if (bcmp(*addrp, &(cs.sin_addr), sizeof(cs.sin_addr)) == 0) 
		    break;

	    if (!*addrp) {
		if (p->debug & DEBUG_CONNECTION)
		    pop_log (p, POP_PRIORITY,
			     "Client address \"%s\" not listed for its host name \"%s\"",
			     p->ipaddr, p->client);
		p->client = p->ipaddr;
	    }
	}
#endif /* BIND43 */
    }

    /* Create input file stream for TCP/IP communication */
    if ((p->input = fdopen(sp, "r")) == NULL) {
	pop_log(p, POP_PRIORITY,
		"Unable to open communication stream for input, err = %d",errno);
	exit (-1);
    }
    
    /*  Create output file stream for TCP/IP communication */
    if ((p->output = fdopen(sp, "w")) == NULL) {
	pop_log(p, POP_PRIORITY,
		"Unable to open communication stream for output, err = %d",
		errno);
	exit (-1);
    }

#endif /* !SANS_SOCKETS */

    setbuf(p->input, NULL);

    /* moved here from popper.c, so that the "starting session" message
       precedes any other log output */
    if (p->debug) {
	time_t tod = time(NULL);
	char *todstr = ctime(&tod);
	*(todstr + strlen(todstr) - 1) = '\0';
	pop_log(p, POP_PRIORITY,
		"%s %s (pl %s) starting session from \"%s\" [%s] on %s.",
		progname, VERSION, ZPOP_PATCHLEVEL, p->client, p->ipaddr, todstr);
    }

    if (p->debug & DEBUG_CONFIG) {
	char **strptr;
	int i;
	FILE *trace_fp;
	char *trace_prefix;
	int idle_timeout;

	trace_fp = p->trace;
	trace_prefix = p->trace_prefix;
	fputs(trace_prefix, trace_fp);
	fprintf(trace_fp, "Spool path: \"%s\"\n", p->dropname);
	fputs(trace_prefix, trace_fp);
	fprintf(trace_fp, "Preferences path: \"%s\"\n", p->pref_path);
	fputs(trace_prefix, trace_fp);
	fprintf(trace_fp, "Library path: \"%s\"\n", p->lib_path);
	fputs(trace_prefix, trace_fp);
	idle_timeout = p->idle_timeout;
	if (idle_timeout == 0)
	    fputs("No idle timeout.\n", trace_fp);
	else
	    fprintf(trace_fp, "Idle timeout is %d seconds.\n", idle_timeout);

	fputs(trace_prefix, trace_fp);
	fputs("Library structure:", trace_fp);
	glist_FOREACH(p->lib_structure, char *, strptr, i) {
	    putc(' ', trace_fp);
	    fputs(*strptr, trace_fp);
	}
	putc('\n', trace_fp);
	fputs(trace_prefix, trace_fp);
	fputs("Debug flags:", trace_fp);
	for (i=0;debug_flags[i].key;i++)
	    if ((p->debug & debug_flags[i].val) == debug_flags[i].val) {
		putc(' ', trace_fp);
		fputs(debug_flags[i].key, trace_fp);
	    }
	putc('\n', trace_fp);
	fflush(trace_fp);
    }

    return errflag ? POP_FAILURE : POP_SUCCESS;
}
