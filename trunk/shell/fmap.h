#ifndef _FMAP_H_
#define _FMAP_H_

#include "zcunix.h"

#define FMAP_MAX	32

extern char *fmap P((FILE *));
extern void funmap P((char *));
extern size_t fmap_size P((char *));

#endif /* !_FMAP_H_ */
