#include <dpipe.h>
#include <dynstr.h>
#include <glist.h>
#include <mime-api.h>
#include <string.h>
#include "headers.h"


struct glist outerHeaders;
struct glist innerHeaders;

char *boundary;


void
headers_read(struct glist *headers, struct dpipe *source)
{
    glist_Init(headers, sizeof(struct mime_pair), 4);
    mime_Headers(source, headers, 0);
}


const char *
header_find(const struct glist *headers, const char *name)
{
    if (name) {
	int counter;
	const struct mime_pair *header;
	/* XXX casting away const */
	glist_FOREACH((struct glist *) headers, const struct mime_pair, header, counter)
	    if (!strcasecmp(name, dynstr_Str(&header->name)))
		return dynstr_Str(&header->value);
    }
    
    return 0;
}


void
headers_merge(struct glist *outer, struct glist *inner)
{
    /*
     * This is a fairly inefficient implementation of preferential
     * merge -- O(m*n).  But the actual numbers involved are quite
     * small, so I'm willing to live with it.  Larger data sets might
     * warrant reimplementation using a skip list or hash table.
     */

    int counter;
    struct mime_pair *candidate;

    glist_FOREACH(outer, struct mime_pair, candidate, counter) {
	const char *name = dynstr_Str(&candidate->name);
	
	if (header_outer(name) && !header_find(inner, name))
	    glist_Add(inner, candidate);
	else
	    mime_pair_destroy(candidate);
    }

    glist_Destroy(outer);
}
    

void
header_emit(const struct mime_pair *header, struct dpipe *destination)
{
    dpipe_Write(destination, dynstr_Str(&header->name),  dynstr_Length(&header->name));
    dpipe_Putchar(destination, ':');
    dpipe_Write(destination, dynstr_Str(&header->value), dynstr_Length(&header->value));
}


void
headers_emit(const struct glist *headers, struct dpipe *destination, int (*predicate)(const char *))
{
    int counter;
    const struct mime_pair *header;
    /* XXX casting away const */
    glist_FOREACH((struct glist *) headers, const struct mime_pair, header, counter)
	if (!predicate || predicate(dynstr_Str(&header->name)))
	    header_emit(header, destination);
}


static const char content_[] = "content-";
static const char mime_version[] = "mime-version";
static const char message_id[] = "message-id";
static const char encrypted[] = "encrypted";
static const char encoding[] = "content-transfer-encoding";
static const char subject[] = "subject";


int
header_outer(const char *name)
{
    /* Technically, DeMorgan's would let us merge this with
     * header_inner(), below.  However, the transformed
     * expression would not short-circuit as efficiently. */
    return strncasecmp(name, content_, sizeof(content_) - 1)
	&& strcasecmp(name, mime_version)
	&& strcasecmp(name, message_id)
	&& strcasecmp(name, encrypted)
	&& strcasecmp(name, subject); /* technically wrong, but
					 more correct in practice */
}


int
header_decoded(const char *name)
{
    return strcasecmp(name, encoding);
}


void
boundary_emit(struct dpipe *destination, int final)
{
    dpipe_Write(destination, "--", 2);
    dpipe_Write(destination, boundary, strlen(boundary));
  
    if (final)
	dpipe_Write(destination, "--", 2);

    dpipe_Putchar(destination, '\n');
}
