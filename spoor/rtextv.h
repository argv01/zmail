/*
 * $RCSfile: rtextv.h,v $
 * $Revision: 2.4 $
 * $Date: 1994/03/23 01:44:19 $
 * $Author: bobg $
 *
 * $Log: rtextv.h,v $
 * Revision 2.4  1994/03/23 01:44:19  bobg
 * Banish the spUnpack() function to portability hell.  Don't
 * RAISE(strerror(EINVAL), ...) just because the caller tried to install
 * a menu item in an impossible slot.  Trim CVS logs.
 *
 * Revision 2.3  1993/05/29  00:48:47  bobg
 * Change spoorClass_t to struct spClass, the SPOOR metaclass.  Make
 * bitfields in buttonv.h unsigned to shut up some compilers.  Get rid of
 * SPOORCLASS_NULL constant.  Get rid of spoorfn_t.  Turn several SPOOR
 * functions into macros that invoke spClass methods.  Make sane some
 * function names.  Make classes remember their children to speed the
 * numberClasses operation.  Declare spUnpack in spoor.h (we weren't
 * before??).
 */

#ifndef SPOOR_RTEXTV_H
#define SPOOR_RTEXTV_H

#include <spoor.h>
#include <textview.h>

enum spRtextv_wrapMode {
    spRtextv_regionwrap = spTextview_WRAPMODES,
    spRtextv_WRAPMODES
};

enum spRtextv_attribute {
    spRtextv_highlighted,
    spRtextv_nowrap,
    spRtextv_ATTRIBUTES
};

struct spRtextv {
    SUPERCLASS(spTextview);
};

extern struct spClass *spRtextv_class;

#define spRtextv_NEW() \
    ((struct spRtextv *) spoor_NewInstance(spRtextv_class))

extern void spRtextv_InitializeClass();

#endif /* SPOOR_RTEXTV_H */
