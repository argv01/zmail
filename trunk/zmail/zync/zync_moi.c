/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <ctype.h>

int
zync_moi(p)
    POP *p;
{
    char line[MAXLINELEN];
    char *valSplit, *valStart, *valEnd;
    int i;
    char *errstr;
    struct plist_prop *prop;

    /* Clean out or create the client prop plist */
    if (p->client_props == NULL)
	p->client_props = plist_New(CLIENT_PROPS_GROWSIZE);
    else 
	plist_Free(p->client_props);

    /* Get keywords and values from client */
    pop_msg(p, POP_SUCCESS, "Tell me about vous.");
    while (-1) {
	if (zync_fgets(line, MAXLINELEN, p) == NULL)
	    return POP_ABORT;
	if (strcmp(line, ".\r\n") == 0) {
	    if (p->debug & DEBUG_COMMANDS)
		pop_log(p, POP_DEBUG, "Received: \".\"");
	    break;
	}

	line[strlen(line)-2] = '\0';
	if (p->debug & DEBUG_COMMANDS)
	    pop_log(p, POP_DEBUG, "Received: \"%s\"", line);
	if ((valStart = index(line, ' ')) == NULL) {
	    errstr = strdup(line);
	    glist_Add(p->errGlist, &errstr);
	    continue;
	}
	*(valSplit=valStart++) = '\0';
	while(*valStart!='\0' && index(" \t", *valStart))
	    valStart++;
	valEnd = valStart+strlen(valStart)-1;
	while(valEnd>=valStart && index(" \t", *valEnd))
	    valEnd--;
	if (valEnd < valStart) {
	    *valSplit=' ';
	    errstr = strdup(line);
	    glist_Add(p->errGlist, &errstr);
	    continue;
	}
	for(valEnd=line; *valEnd!='\0'; valEnd++)
	    *valEnd=toupper(*valEnd);
	plist_Add(p->client_props, line, valStart);
    }

    /* Handle error messages, if any. */
    if (glist_EmptyP(p->errGlist)) {
	pop_msg(p, POP_SUCCESS, "");
	return POP_SUCCESS;
    } else {
	char **errstrptr;
	size_t sent = 0;

	pop_msg(p, POP_FAILURE, "Could not parse the following ZMOI lines:");
	glist_FOREACH(p->errGlist, char *, errstrptr, i) {
	    fprintf(p->output, "  %s\r\n", *errstrptr);
	    sent += strlen(*errstrptr);
	}
	fprintf(p->output, ".\r\n");
	fflush(p->output);
	allow(p, sent + 60);	/* roughly */
	freeErrGlist(p->errGlist);
	return POP_FAILURE;
    }
}
