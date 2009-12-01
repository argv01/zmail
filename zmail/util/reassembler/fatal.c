#include <errno.h>
#include <except.h>
#include <stdio.h>
#include <stdlib.h>
#include "fatal.h"
#include "lock.h"

FILE *fatal_sink = stderr;
const char *fatal_basename = TARGET;


void
usage()
{
    /* XXX change when frontend comes in */
    fprintf(stderr, "\
Usage:\n\
   %s [-p dir] [-n] [-d command] [-s size] [-x dir] [-m command] [dir]\n\
\n\
Options:\n\
\n\
   -p   Place fragments in this directory while they are arriving.\n\
        The directory and all of its parents will be created if they do not\n\
        already exist.  Defaults to \"/usr/mail/<username>.partial\".\n\
\n\
   -n   Be nondestructive; do not remove fragments of completed messages.\n\
        By default, fragments are removed once the message is complete.\n\
\n\
   -d   Feed completed messages through this command for local delivery.\n\
        The command will be executed using the Borne shell (sh).\n\
        Defaults to \"/bin/mail -s -d <username>\".\n\
\n\
   -s   Externalize attachments larger than this size in bytes.\n\
        Attachments split across multiple fragments will also be externalized.\n\
        Defaults to 50 kilobytes; set this to zero to disable externalization.\n\
\n\
   -x   Place externalized attachments into this directory.\n\
        The directory and all of its parents will be created if they do not\n\
        already exist.  Defaults to \"~/Mail/detach.dir\".\n\
\n\
   -m   Full pathname of MIME decoder.\n\
        The decoder will be executed in the manner of metamail's mimencode.\n\
        Defaults to \"/usr/lib/Zmail/bin/mimencode\".\n\
", fatal_basename);
    exit(3);
}


void
fatal_handler()
{
    unlock();
    if (fatal_sink) {
	if (except_GetExceptionValue())
	    fprintf(fatal_sink, "%s: fatal error in %s: %s\n", fatal_basename, except_GetExceptionValue(), except_GetRaisedException());
	else
	    fprintf(fatal_sink, "%s: fatal error: %s\n", fatal_basename, except_GetRaisedException());
	fclose(fatal_sink);
    }
    
    exit(errno ? errno : -1);
}
