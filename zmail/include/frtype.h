#ifndef _FRTYPE_H_
#define _FRTYPE_H_

/*
 * $RCSfile: frtype.h,v $
 * $Revision: 2.13 $
 * $Date: 1998/12/07 22:49:52 $
 * $Author: schaefer $
 */

#ifdef NOT_NOW
/* must correspond to frame_map array in zm_frame.c */
#endif /* NOT_NOW */

#include "osconfig.h"

typedef enum {			/* Button in the toolbox? */
    FrameUnknown = 0,
    FrameAlias,				/* Yes */
    FrameAttachments,
    FrameBrowseAddrs,			/* Yes?? */
    FrameCompAliases,
    FrameCompHdrs,
    FrameCompOpts,
    FrameCompose,			/* Yes */
    FrameConfirmAddrs,
    FrameCustomHdrs,			/* Yes */
    FrameFileFinder,
    FrameFolders,			/* Yes */
    FrameFontSetter,
    FrameHeaders,			/* Yes */
    FrameHelpIndex,			/* Yes */
    FrameMain,
    FrameOpenFolders,			/* Yes */
    FrameOptions,			/* Yes */
    FramePageMsg,
    FramePageText,
    FramePainter,			/* Yes */
    FramePickDate,			/* Yes */
    FramePickPat,			/* Yes */
    FramePinMsg,
    FramePrinter,			/* Yes */
    FrameSaveMsg, /* Left as a place holder, see frame_map[] in zm_frame.c */
    FrameScript,			/* Yes */
    FrameSearchReplace,
    FrameSort,				/* Yes */
    FrameTaskMeter,
    FrameTemplates,			/* Yes */
    FrameToolbox,
    FrameLicense,
    FrameAddFolder,
    FrameRemoveFolder,
    FrameCompOptions,
    FrameDynamicHdrs,
    FrameReopenFolders,
    FrameNewFolder,
    FramePageEditText,
    FrameRenameFolder,
    FrameFilters,
    FrameFunctions,
    FrameMenus,
    FrameButtons,
    FrameSubmenus,
    FrameFunctionsHelp,
    FrameMenuToggle,
    FrameAskFF,
    FrameAskMultiple,
    FrameAskInput,
    FrameAskRetry,
#ifdef HAVE_HELP_BROKER
    FrameHelpContents,
#endif /* HAVE_HELP_BROKER */
    FrameAskMessageList,
    FrameLogin,
    FrameAbout,
    FrameZCal,
#ifdef ZPOP_SYNC
    FrameSyncSpools,
    FrameSyncEdit,
#endif /* ZPOP_SYNC */
#ifdef WIN16
    FrameSignature,
    FrameMDIMain,
#endif
    FrameTclTk,
    FrameTotalDialogs, /* must be last */
#ifndef HAVE_HELP_BROKER
    FrameHelpContents = FrameHelpIndex,
#endif /* !HAVE_HELP_BROKER */
    FrameTrailingGarbage /* to avoid having comma on last item */
} FrameTypeName;

#endif /* _FRTYPE_H_ */
