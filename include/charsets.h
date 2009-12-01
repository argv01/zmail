#ifndef INCLUDE_CHARSETS_H
#define INCLUDE_CHARSETS_H

/*
 * Character set manipulation, presently for Motif only
 */

#include "config.h"

#ifdef MOTIF
#include <general.h>
#include <X11/Intrinsic.h>

extern void restrict_to_charset P(( Widget, const char * ));

#endif /* MOTIF */

#endif /* INCLUDE_CHARSETS_H */
