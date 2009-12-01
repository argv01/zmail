#ifndef _UISUPP_H_
#define _UISUPP_H_

/*
 * $RCSfile: uisupp.h,v $
 * $Revision: 1.3 $
 * $Date: 1994/04/18 04:16:50 $
 * $Author: pf $
 */

#include <general.h>

typedef unsigned long zmFlags;
extern int uiscript_Exec P ((const char *, zmFlags));
typedef int zmBool;

#define uiscript_ExecNoUpdate	ULBIT(0)

#endif /* _UISUPP_H_ */
