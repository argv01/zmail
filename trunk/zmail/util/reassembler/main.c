#include <except.h>
#include <excfns.h>
#include <getopt.h>
#include <glist.h>
#include <mime-api.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "assemble.h"
#include "fatal.h"
#include "lock.h"
#include "options.h"
#include "receive.h"
#include "status.h"


unsigned long externSize = 50*1024;
const char *externDir = 0;
const char *partsDir = 0;
int destructive = 1;
const char *deliveryCmd = 0;
const char *mimencode = "/usr/lib/Zmail/bin/mimencode";


int
process_room(unsigned total, char *room)
{
    if (partials_status(total, room)) {
	unlock();
	assemble(room);
	if (destructive) {
	    remove_status_report(room);
	    rmdir(room);
	}
	return 1;
    } else
	return 0;
}


int
main(int argc, char *argv[])
{
    init_groups();
    umask(S_IRWXO|S_IRWXG);
    fatal_basename = argv[0];
    except_SetUncaughtExceptionHandler(fatal_handler);

    {
	int option;

	while ((option = getopt(argc, argv, "nd:x:p:s:m:?")) != EOF) {
	    switch (option) {

	    case 'n':
		destructive = 0;
		break;

	    case 'd':
		deliveryCmd = optarg;
		break;

	    case 'x':
		externDir = optarg;
		break;

	    case 'p':
		partsDir = optarg;
		break;

	    case 's':
		externSize = strtoul(optarg, 0, 0);
		break;

	    case 'm':
		mimencode = optarg;
		break;

	    default:
		usage();
	    }
	}
    }


    if (optind == argc - 1)
	process_room(0, argv[optind]);

    else if (optind == argc) {
	TRYSIG((SIGPIPE, 0)) {
	    unsigned num_stored;
	    char *room;
	    struct glist headers;
	    const unsigned total = receive(&room, &headers);

	    if (room) {
		if (!process_room(total, room)) {
		    unlock();
		    update_status_report(room, &headers);
		}
		free(room);
	    }
	    glist_CleanDestroy(&headers, (void(*)(void *)) mime_pair_destroy);
	    
	} EXCEPTSIG(SIGPIPE) {
	    errno = EPIPE;
	    fatal_handler();
	} ENDTRYSIG;
    } else
	usage();

    
    fclose(fatal_sink);
    unlock();
    return 0;
}
