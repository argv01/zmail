#ifndef INCLUDE_FONTS_GC_H
#define INCLUDE_FONTS_GC_H

/* fonts_gc.h 	Copyright 1993 Z-Code Software Corp.
 *
 * Simple garbage collection for font lists
 */

#include <general.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

void reference_init P((void));			/* Begin reference counting */

void reference_add    P(( XmFontList ));	/* Add a reference */
void reference_remove P(( XmFontList ));	/* Remove a reference; possibly deallocate */


#endif /* !INCLUDE_FONTS_GC_H */
