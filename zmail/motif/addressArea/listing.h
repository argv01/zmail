#ifndef INCLUDE_ADDRESSAREA_LISTING_H
#define INCLUDE_ADDRESSAREA_LISTING_H


#include <X11/Intrinsic.h>
#include <general.h>

struct AddressArea;


extern void list_mark_triple P((struct AddressArea *, char **[3]));

extern void list_get_compose P((Widget, struct AddressArea *, Boolean));

extern void list_expand	P((Widget, struct AddressArea *));
extern void list_edit	P((Widget, struct AddressArea *));
extern void list_delete	P((Widget, struct AddressArea *));
extern void list_select	P((Widget, struct AddressArea *));

extern void list_remove P((struct AddressArea *));
extern void list_select_noedit P((Widget, struct AddressArea *));


#endif /* !INCLUDE_ADDRESSAREA_LISTING_H */
