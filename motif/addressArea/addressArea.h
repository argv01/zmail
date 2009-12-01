#ifndef INCLUDE_ADDRESSAREA_H
#define INCLUDE_ADDRESSAREA_H


#include <X11/Intrinsic.h>
#include <general.h>
#include "../attach/area.h"
#include "uicomp.h"


struct Compose;
struct AddressArea;


struct AddressArea *AddressAreaCreate P((Widget, AttachArea *, Widget *));
void AddressAreaDestroy P((struct AddressArea *));

void AddressAreaUse P((struct AddressArea *, struct Compose *));
void AddressAreaRefresh P((struct AddressArea *));
void AddressAreaSetWidth P((struct AddressArea *, const unsigned));

void AddressAreaGotoSubject P((struct AddressArea *));
void AddressAreaGotoAddress P((struct AddressArea *, enum uicomp_flavor));

Widget AddressAreaGetRaw P((struct AddressArea *));
unsigned long AddressAreaWalkRaw P((struct AddressArea *, unsigned long));

#endif /* !INCLUDE_ADDRESSAREA_H */
