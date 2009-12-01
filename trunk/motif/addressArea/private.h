#ifndef INCLUDE_ADDRESSAREA_PRIVATE_H
#define INCLUDE_ADDRESSAREA_PRIVATE_H

#ifdef SPTX21
#define _XOS_H_
#endif /* SPTX21 */

#include "osconfig.h"
#include "callback.h"
#include "uicomp.h"
#include <X11/Intrinsic.h>



#ifdef OZ_DATABASE
#define SHOW_EMPTY_ATTACH
#endif /* OZ_DATABASE */



enum LineupMember { Entry, Listing, Subject, LineupCount };


struct AddressArea {
    struct Compose *compose;
    Widget field;
    Widget layout;
    Widget list;
    Widget menu;
    Widget push;
    Widget subject;
    Widget lineup[LineupCount];
    Widget raw;
    ZmCallback subjectSync;
    ZmCallback addressSync;
    ZmCallback editSync;
    enum uicomp_flavor dominant;
    Boolean dirty;
    XtWorkProcId refresh;
    Boolean progress;
    Widget *progressLast;
    Widget subjectArea, rawArea, cookedArea;
    unsigned long pos_flags;
};


extern Bool address_edit;


#endif /* !INCLUDE_ADDRESSAREA_PRIVATE_H */
