#ifndef INCLUDE_ACTIONFORM_H
#define INCLUDE_ACTIONFORM_H


#include <X11/Intrinsic.h>
#include <general.h>


typedef struct ActionAreaGeometry {
    Dimension max_width;
    Dimension max_height, nice_height;
    int flags;
    Widget widget;
} ActionAreaGeometry;

#define AA_ONE_ROW ULBIT(0)
#define AA_RESIZED ULBIT(1)

extern WidgetClass GetActionFormClass P((void));
extern WidgetClass GetCenterFormClass P((void));


#endif /* !INCLUDE_ACTIONFORM_H */
