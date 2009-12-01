#ifndef INCLUDE_REASSEMBLER_EDGE_H
#define INCLUDE_REASSEMBLER_EDGE_H


#include <glist.h>

struct dpipe;
struct FeederState;

struct Location {
    unsigned sequence;
    unsigned long offset;
};


void edge_init(void);
void edge_add(const struct dpipe *, const struct FeederState *);
int edge_large(unsigned);


#endif /* !INCLUDE_REASSEMBLER_EDGE_H */
