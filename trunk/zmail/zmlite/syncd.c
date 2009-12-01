/* $RCSfile: syncd.c,v $
 * $Revision: 2.1 $
 * $Date: 1995/07/20 20:28:58 $
 * $Author: bobg $
 */

#ifdef ZPOP_SYNC

#include <spoor.h>
#include <zmail.h>
#include <zmlite.h>
#include "syncfldr.h"
#include <mstore/mfolder.h>

static const char syncd_rcsid[] =
    "$Id: syncd.c,v 2.1 1995/07/20 20:28:58 bobg Exp $";

extern AskAnswer
gui_sync_ask()
{
    return (ask(AskYes, "Preview the sync?"));
}

extern int
gui_sync_preview(mf1, mf2, A, B, C)
    struct mfldr *mf1, *mf2;
    struct dlist *A, *B, *C;
{
    return (ask(AskOk,
		"Preview not yet implemented.  Do the sync?") == AskYes);
}

#endif /* ZPOP_SYNC */
