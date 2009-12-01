#ifndef INCLUDE_REASSEMBLER_HEADERS_H
#define INCLUDE_REASSEMBLER_HEADERS_H


#include <dpipe.h>
#include <glist.h>

struct mime_pair;


extern struct glist outerHeaders;
extern struct glist innerHeaders;

extern char *boundary;


void headers_read(struct glist *, struct dpipe *);
const char *header_find(const struct glist *, const char *);
void headers_merge(struct glist *, struct glist *);

void headers_emit(const struct glist *, struct dpipe *, int (*)(const char *));
int header_outer(const char *);
int header_decoded(const char *);

void header_emit(const struct mime_pair *, struct dpipe *);
void boundary_emit(struct dpipe *, int);


#endif /* !INCLUDE_REASSEMBLER_HEADERS_H */
