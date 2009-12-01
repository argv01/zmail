#ifndef INCLUDE_MOTIF_M_MENUS_H
#define INCLUDE_MOTIF_M_MENUS_H

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include <general.h>


void PostIt P((Widget, Widget, XButtonPressedEvent *));
Widget BuildSimplePulldownMenu P((Widget, char **, void(*)(), VPTR));
void DestroyOptionMenu P((Widget));

char *make_accelerator P((const char *));


#endif /* !INCLUDE_MOTIF_M_MENUS_H */
