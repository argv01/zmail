#include "config.h"

#ifdef ZPOP_SYNC

#include "syncfldr.h"
#include "mfolder.h"

extern AskAnswer
gui_sync_ask()
{
    return ask(AskYes, "Preview the sync?");
}

extern int
gui_sync_preview(mf1, mf2, A, B, C)
struct mfldr *mf1, *mf2;
struct dlist *A, *B, *C;
{
    return (ask(AskOk, "Do the sync?") == AskYes);
}

#endif /* ZPOP_SYNC */
