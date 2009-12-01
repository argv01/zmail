#ifndef INCLUDE_REASSEMBLER_FEEDER_H
#define INCLUDE_REASSEMBLER_FEEDER_H


#include <stdio.h>

struct dpipe;


struct FeederState {
    const char *directory;
    unsigned sequence;
    FILE *file;
    int destructive;
};


void skip_headers(FILE *);
void feeder(struct dpipe *, struct FeederState *);


#endif /* !INCLUDE_REASSEMBLER_FEEDER_H */
