#ifndef INCLUDE_ADDRESSAREA_ENTRY_H
#define INCLUDE_ADDRESSAREA_ENTRY_H


#include "uicomp.h"
#include <X11/Intrinsic.h>
#include <general.h>

struct AddressArea;


extern void flavor_menu_apply P((Widget, enum uicomp_flavor));
extern void field_activate    P((Widget, struct AddressArea *));
extern void field_dirty	      P((Widget, struct AddressArea *));
extern void field_merge	      P((Widget, struct AddressArea *));

extern void field_clear	    P((struct AddressArea *));
extern void flavor_menu_set P((struct AddressArea *, enum uicomp_flavor));


#endif /* !INCLUDE_ADDRESSAREA_ENTRY_H */
