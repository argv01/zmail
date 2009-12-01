#include <dpipe.h>
#include <dputil.h>
#include <glist.h>
#include <mime-api.h>
#include <stdlib.h>
#include <string.h>

#include "decode.h"
#include "edge.h"
#include "feeder.h"
#include "headers.h"
#include "sweepers.h"


static void
emit_elision(const struct glist *headers, const char *filename, struct dpipe *destination)
{
    const char elided[] = "Content-Type: message/external-body; access-type=local-file; name=";
    int counter;
    const struct mime_pair *header;
    /* XXX casting away const */
    glist_FOREACH((struct glist *) headers, const struct mime_pair, header, counter)
	if (!strcasecmp("content-description", dynstr_Str(&header->name))) {
	    header_emit(header, destination);
	    break;
	}
    
    dpipe_Write(destination, elided, sizeof(elided) - 1);
    dpipe_Write(destination, filename, strlen(filename));
    dpipe_Write(destination, "\n\n", 2);
}


/*
 * This function comes as close to being
 * monolithic as I hope anything ever will.
 */

void
merge(struct dpipe *source, struct dpipe *destination, const struct FeederState *state)
{
    struct glist headers;

    /* opening From_ line */
    {
	struct dynstr from_;
	dynstr_Init(&from_);
	mime_Readline(source, &from_);

	dputil_PutDynstr(destination, &from_);
	dpipe_Putchar(destination, '\n');
    }

    {
	struct glist outer;
	
	headers_read(&outer, source);
	if (dpipe_Peekchar(source) == '\n') dpipe_Getchar(source);
	headers_read(&headers, source);
	
	headers_merge(&outer, &headers); /* destroys outer */
    }
	
    if (boundary) {
	/* top-level multipart */
	
	struct glist stack;
	glist_Init(&stack, sizeof(struct mime_stackelt), 1);
	mime_MultipartStart(&stack, boundary, 0, 0);

	headers_emit(&headers, destination, 0);
	mime_NextBoundary(source, destination, &stack, 0);
	
	{
	    unsigned partNumber = 0;

	    while (!glist_EmptyP(&stack)) {
		struct dpipe decoder;
		struct glist partHeaders;
		const char *filename = 0;
		
		headers_read(&partHeaders, source);
		boundary_emit(destination, 0);
	
		if (edge_large(partNumber++)) {
		    filename = decoder_open(&decoder, &partHeaders);
		    emit_elision(&partHeaders, filename, destination);
		}
	    
		headers_emit(&partHeaders, destination, filename ? header_decoded : 0);
		glist_CleanDestroy(&partHeaders, (void(*)(void *)) mime_pair_destroy);
		if (filename) dpipe_Putchar(destination, '\n');
	    
		mime_NextBoundary(source, filename ? &decoder : destination, &stack, 0);

		if (filename) decoder_close(&decoder);
	    }
	}
	boundary_emit(destination, 1);
	free(boundary);
	mime_Unwind(&stack, glist_Length(&stack));
	glist_Destroy(&stack);

	/* grab any random trailing crud; also, delete the last fragment */
	mime_NextBoundary(source, destination, 0, 0);
	
    } else {
	/* top-level leaf */
	
	const char mime_version[] = "Mime-Version: 1.0\n";
	struct dpipe decoder;
	const char *filename = decoder_open(&decoder, &headers);
      
	if (dpipe_Peekchar(source) == '\n') dpipe_Getchar(source);

	dpipe_Write(destination, mime_version, sizeof(mime_version) - 1);
	emit_elision(&headers, filename, destination);

	headers_emit(&headers, destination, filename ? header_decoded : 0);
	mime_NextBoundary(source, filename ? &decoder : destination, 0, 0);

	if (filename) decoder_close(&decoder);
    }
    glist_CleanDestroy(&headers, (void(*)(void *)) mime_pair_destroy);
}
