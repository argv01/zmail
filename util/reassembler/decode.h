#ifndef INCLUDE_REASSEMBLER_DECODE_H
#define INCLUDE_REASSEMBLER_DECODE_H


#include <stdio.h>

struct dpipe;
struct glist;


const char *decoder_open(struct dpipe *, const struct glist *);
void decoder_close(struct dpipe *);

void dumper(struct dpipe *, FILE *);


#endif /* !INCLUDE_REASSEMBLER_DECODE_H */
