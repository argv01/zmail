#ifndef INCLUDE_MSG_PRUNE_H
#define INCLUDE_MSG_PRUNE_H


#include "osconfig.h"
#include <general.h>

struct Attach;
struct Msg;
struct mfolder;


extern unsigned long attach_prune_size;

int prune_omit_set    P((struct Attach * const, const unsigned long));
void prune_omit_clear P((struct Attach * const));

int prune_part_delete   P((struct mfolder * const, struct Msg * const, struct Attach * const));
int prune_part_undelete P((struct mfolder * const, struct Msg * const, struct Attach * const));

extern const char * const prune_externalize;
int pruned P((const struct Attach * const));


#endif /* !INCLUDE_MSG_PRUNE_H */
