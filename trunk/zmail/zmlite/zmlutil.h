#ifndef ZMUTIL_H
#define ZMUTIL_H

#include <spoor.h>
#include <spoor/button.h>
#include <spoor/splitv.h>
#include <spoor/menu.h>

extern struct spSplitview *SplitAdd();

extern void doSaveState P((void));
extern void KillSplitviews P((struct spView *));
extern void KillAllButOneSplitview P((struct spView *, struct spSplitview *));
extern void KillSplitviewsAndWrapviews P((struct spView *));

extern void zmlhelp P((const char *));

extern struct spButtonv *ActionArea(VA_PROTO(GENERIC_POINTER_TYPE *));

#endif /* ZMUTIL_H */
