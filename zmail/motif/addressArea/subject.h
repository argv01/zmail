#ifndef INCLUDE_ADDRESSAREA_SUBJECT_H
#define INCLUDE_ADDRESSAREA_SUBJECT_H


#include <X11/Intrinsic.h>
#include <general.h>

struct AddressArea;
struct zmCallbackData;


extern void subject_activate	P((Widget, struct AddressArea *));
extern void subject_set_compose P((Widget, struct AddressArea *));

extern void subject_get_compose P((struct AddressArea *));
extern void subject_changed	P((struct AddressArea *, struct zmCallbackData *));

#endif /* !INCLUDE_ADDRESSAREA_SUBJECT_H */
