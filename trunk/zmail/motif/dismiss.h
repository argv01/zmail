#ifndef INCLUDE_MOTIF_DISMISS_H
#define INCLUDE_MOTIF_DISMISS_H

#include <general.h>
#include <X11/Intrinsic.h>

struct FrameDataRec;


enum DismissState { DismissCancel, DismissClose };

void DismissSetLabel P((Widget, enum DismissState));

void DismissSetFrame P((struct FrameDataRec *, enum DismissState));
void DismissSetWidget P((Widget, enum DismissState));


#endif /* INCLUDE_MOTIF_DISMISS_H */
