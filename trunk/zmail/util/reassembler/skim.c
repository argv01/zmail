#include <dpipe.h>
#include <errno.h>
#include <except.h>
#include <glist.h>
#include <mime-api.h>
#include <string.h>

#include "edge.h"
#include "fatal.h"
#include "feeder.h"
#include "headers.h"
#include "options.h"
#include "sweepers.h"


void
skim(struct dpipe *source, const struct FeederState *state)
{
    char *type, *subtype;
    struct glist headers;

    /* opening From_ line */
    mime_Readline(source, 0);

    /* outer headers */
    headers_read(&headers, source);
    mime_AnalyzeHeaders(&headers, 0, &type, &subtype, 0, 0);
    ASSERT(type && subtype && !strcasecmp(type, "message") && !strcasecmp(subtype, "partial"), "bad MIME type", WHERE(skim));

    if (dpipe_Peekchar(source) == '\n') dpipe_Getchar(source);
    glist_CleanDestroy(&headers, (void(*)(void *)) mime_pair_destroy);

    /* inner headers */
    headers_read(&headers, source);
    mime_AnalyzeHeaders(&headers, 0, 0, 0, &boundary, 0);
    
    if (boundary && externSize) {
	struct glist stack;
	
	glist_Init(&stack, sizeof(struct mime_stackelt), 1);
	mime_MultipartStart(&stack, boundary, 0, 0);
	
	if (!mime_NextBoundary(source, 0, &stack, 0))
	    do
		edge_add(source, state);
	    while (!mime_NextBoundary(source, 0, &stack, 0));
	
	edge_add(source, state);
	mime_Unwind(&stack, glist_Length(&stack));
	glist_Destroy(&stack);
	boundary = strdup(boundary);
	ASSERT(boundary, strerror(ENOMEM), WHERE(skim));
    }
    glist_CleanDestroy(&headers, (void(*)(void *)) mime_pair_destroy);
}
