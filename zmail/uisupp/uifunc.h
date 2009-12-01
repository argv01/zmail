#ifndef _UIFUNC_H_
#define _UIFUNC_H_

/*
 * $RCSfile: uifunc.h,v $
 * $Revision: 1.3 $
 * $Date: 1994/10/25 01:04:58 $
 * $Author: liblit $
 */


#include "uisupp.h"

struct dynstr;

extern zmBool uifunctions_GetText P ((const char *, const char *, struct dynstr *));
extern zmBool uifunctions_Delete P ((const char *));
extern zmBool uifunctions_Add P ((const char *, const char *, GuiItem, GuiItem));
extern zmBool uifunctions_List P ((char ***, zmBool));
extern void uifunctions_HelpScript P ((const char *));

#endif /* _UIFUNC_H_ */
