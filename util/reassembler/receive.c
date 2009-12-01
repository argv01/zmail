#include <dpipe.h>
#include <dputil.h>
#include <dynstr.h>
#include <except.h>
#include <excfns.h>
#include <glist.h>
#include <mime-api.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "announce.h"
#include "fatal.h"
#include "files.h"
#include "headers.h"
#include "options.h"
#include "receive.h"
#include "lock.h"

static gid_t user_group;
static gid_t mail_group;


void
init_groups()
{
    user_group = getgid();
    mail_group = getegid();
    setgid(user_group);
}


unsigned
receive(char **waiting_room, struct glist *headers)
{
    const struct passwd * const passwd = getpwuid(getuid());
    FILE *sink;
    int (*closer)(FILE *);
    const char *total = 0;

    ASSERT(passwd && passwd->pw_name, "no such user", WHERE(receive));
    
    /* don't work for root - should probably use dpipes instead */
    if (passwd->pw_uid == 0) {
	sink = popen_delivery();
	closer = pclose;
	*waiting_room = 0;
	
    } else {
	
	struct dpipe source;
	struct dynstr from_;
	struct glist *parameters = 0;
	char *type, *subtype;
	char *id = 0;
	const char *number = 0;
	char *leftovers;

	/* put stdin -> dpipe */
	dpipe_Init(&source, 0, 0, dputil_FILEtoDpipe, stdin, 0);
	
	dynstr_Init(&from_);
	mime_Readline(&source, &from_);
	headers_read(headers, &source);
	mime_AnalyzeHeaders(headers, &parameters, &type, &subtype, 0, 0);
	
	if (type && subtype
	    && !strcasecmp(type, "message")
	    && !strcasecmp(subtype, "partial")) {

	    int counter;
	    struct mime_pair *parameter;

	    glist_FOREACH(parameters, struct mime_pair, parameter, counter) {
		if (!strcasecmp(dynstr_Str(&parameter->name), "id"))
		    id = dynstr_Str(&parameter->value);
		else if (!strcasecmp(dynstr_Str(&parameter->name), "number"))
		    number = dynstr_Str(&parameter->value);
		if (!strcasecmp(dynstr_Str(&parameter->name), "total"))
		    total = dynstr_Str(&parameter->value);

		if (id && number && total)
		    break;
	    }
	}
	
	if (id && *id && number && strtoul(number, &leftovers, 10) && !*leftovers) {
	    struct dynstr accumulate;
	    dynstr_Init(&accumulate);

	    if (partsDir)
		dynstr_Append(&accumulate, partsDir);
	    else {
		dynstr_Append(&accumulate, "/usr/mail/");
		dynstr_Append(&accumulate, passwd->pw_name);
		dynstr_Append(&accumulate, ".partials");
	    }

	    TRY {
		if (!partsDir) setgid(mail_group);
		ASSERT(!mkdirhier(dynstr_Str(&accumulate), S_IRWXU), strerror(errno), WHERE(receive));
	    } FINALLY {
		if (!partsDir) setgid(user_group);
	    } ENDTRY;
	    
	    dynstr_AppendChar(&accumulate, '/');
	    filename_disarm(id);
	    dynstr_Append(&accumulate, id);
	    *waiting_room = strdup(dynstr_Str(&accumulate));
	    ASSERT(*waiting_room, strerror(ENOMEM), WHERE(receive));
	    {
		struct stat status;

		if (stat(*waiting_room, &status)) {
		    ASSERT(!mkdir(*waiting_room, S_IRWXU), strerror(errno), WHERE(receive));
		    lock(*waiting_room);
		    announce(*waiting_room, total, headers);
		} else {
		    lock(*waiting_room);
		    ASSERT(S_ISDIR(status.st_mode), strerror(ENOTDIR), WHERE(receive));
		}
	    }

	    dynstr_AppendChar(&accumulate, '/');
	    dynstr_Append(&accumulate, number);
	    sink = efopen(dynstr_Str(&accumulate), "w", WHERE(receive));
	    dynstr_Destroy(&accumulate);
	    {
		struct stat status;

		ASSERT(!fstat(fileno(sink), &status), strerror(errno), WHERE(receive));
		ASSERT(status.st_uid == getuid(), "suspicious ownership in waiting room", WHERE(receive));
		ASSERT(S_ISREG(status.st_mode), "suspicious file type in waiting room", WHERE(receive));
	    }
	    closer = fclose;

	} else {
	    sink = popen_delivery();
	    closer = pclose;
	    *waiting_room = 0;
	}

	{
	    struct mime_pair *header;
	    int counter;
	    
	    fputs(dynstr_Str(&from_), sink);
	    dynstr_Destroy(&from_);
	    putc('\n', sink);

	    glist_FOREACH(headers, struct mime_pair, header, counter) {
		fputs(dynstr_Str(&header->name), sink);
		putc(':', sink);
		fputs(dynstr_Str(&header->value), sink);
	    }
	}

	while (dpipe_Ready(&source)) {
	    char *unread;
	    int count = dpipe_Get(&source, &unread);
	    efwrite(unread, 1, count, sink, WHERE(receive));		
	    free(unread);
	}
	dpipe_Destroy(&source);
    }

    {
	char transfer[BUFSIZ];

	while (!feof(stdin)) {
	    const int count = efread(transfer, 1, sizeof(transfer), stdin, WHERE(receive));
	    efwrite(transfer, 1, count, sink, WHERE(receive));
	}
	
	closer(sink);
    }

    return total ? strtoul(total, 0, 10) : 0;
}
