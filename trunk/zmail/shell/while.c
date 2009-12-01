#include "catalog.h"
#include "dyncond.h"
#include "error.h"
#include "except.h"
#include "excfns.h"
#include "while.h"
#include "zcalloc.h"
#include "zcstr.h"
#include "zmail.h"


/*
 * Simple looping for Z-Script.
 *
 *   while <condition> <body>
 *
 * <condition> is a boolean expression in the manner of button sensitivities
 * <body> is a bit of Z-Script to evaluate in the manner of foreach
 *
 * Note that <condition> must generally be quoted, lest it be expanded
 * once instead of reexpanded (reevaluated) on each iteration.  This
 * is likely to be a common mistake.
 *
 * As with the foreach command, if <body> ever exits with nonzero
 * status, the entire while loop will be terminated.
 *
 */

int
zm_while(argc, argv, messageList)
    int argc;
    char **argv;
    struct mgroup *messageList;
{
    if (argc == 3) {
	size_t length = strlen(argv[2]) + 1;
	char *body = (char *) emalloc(length, "zm_while");
	int status = 0;
	int parsed;

	TRY {
	    while (!status && eval_bexp(argv[1], &parsed) && parsed) {
		bcopy(argv[2], body, length);
		status = check_intr() || cmd_line(body, messageList);
	    }
	} FINALLY {
	    xfree(body);
	} ENDTRY;
	
	return parsed ? status : -1;

    } else {
	if (argc < 3)
	    error(UserErrWarning, argc < 3 ?
		  catgets( catalog, CAT_SHELL, 75, "%s: not enough arguments" )
		  : catgets( catalog, CAT_SHELL, 181, "%s: too many arguments" ),
		  argv[0]);
	return -1;
    }
}
