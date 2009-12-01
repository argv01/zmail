#ifndef CHARSETF_H
#define CHARSETF_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/listv.h>
#include <spoor/text.h>

struct charsets {
    SUPERCLASS(dialog);
    struct spListv *list;
    struct spTextview *instructions;
    int composep;
};

extern struct spWclass *charsets_class;

extern struct spWidgetInfo *spwc_Choosecharset;

extern void charsets_InitializeClass P((void));

#define charset_NEW() \
    ((struct charsets *) spoor_NewInstance(charsets_class))


#endif /* CHARSETF_H */
