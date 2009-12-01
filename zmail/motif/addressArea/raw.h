#ifndef INCLUDE_ADDRESSAREA_RAW_H
#define INCLUDE_ADDRESSAREA_RAW_H


#include <X11/Intrinsic.h>
#include <general.h>

struct AddressArea;
struct zmCallbackData;


extern Boolean edit_switch_modes P((struct AddressArea *, Boolean));
extern void edit_changed P((struct AddressArea *, struct zmCallbackData *));

#endif /* !INCLUDE_ADDRESSAREA_RAW_H */
