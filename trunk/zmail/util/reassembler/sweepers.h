#ifndef INCLUDE_REASSEMBLER_SWEEPERS_H
#define INCLUDE_REASSEMBLER_SWEEPERS_H


#include <stdio.h>

struct dpipe;
struct FeederState;


void skim(struct dpipe *, const struct FeederState *);
void merge(struct dpipe *, struct dpipe *, const struct FeederState *);


#endif /* !INCLUDE_REASSEMBLER_SWEEPERS_H */
