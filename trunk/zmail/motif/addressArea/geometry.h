#ifndef INCLUDE_ADDRESSAREA_GEOMETRY_H
#define INCLUDE_ADDRESSAREA_GEOMETRY_H


#include <X11/Intrinsic.h>
#include <general.h>

struct AddressArea;
struct zmCallbackData;


extern void align  P((Widget, struct AddressArea *, XEvent *, Boolean *));
extern void center P((Widget, Widget,		     XEvent *, Boolean *));


#endif /* !INCLUDE_ADDRESSAREA_GEOMETRY_H */
