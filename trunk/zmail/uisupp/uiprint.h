#ifndef _UIPRINT_H_
#define _UIPRINT_H_

/*
 * $RCSfile: uiprint.h,v $
 * $Revision: 1.4 $
 * $Date: 1995/10/05 05:28:01 $
 * $Author: liblit $
 */

#include "uisupp.h"
#include "zmflag.h"

typedef enum {
    uiprint_HdrAll,
    uiprint_HdrStd,
    uiprint_HdrNone
} uiprint_HdrType_t;

struct _uiprint {
    char *name, *cmd;
    zmFlags flags;
    uiprint_HdrType_t type;
};
typedef struct _uiprint uiprint_t;

#define uiprint_SetPrinterName(X, Y) (str_replace(&(X)->name, (Y)))
#define uiprint_SetPrintCmd(X, Y)  (str_replace(&(X)->cmd, (Y)))
#define uiprint_SetHdrType(X, Y) ((X)->type = (Y))
#define uiprint_GetPrinterName(X) ((X)->name)
#define uiprint_GetPrintCmd(X) ((X)->cmd)
#define uiprint_GetHdrType(X) ((X)->type)
#define uiprint_SetFlags(X, Y) (turnon((X)->flags, (Y)))
#define uiprint_GetFlags(X, Y) (ison((X)->flags, (Y)))

#define uiprint_SingleProcess   ULBIT(0)

extern zmBool uiprint_Print P ((uiprint_t *));
extern void uiprint_Destroy P ((uiprint_t *));
extern void uiprint_Init P ((uiprint_t *));
extern int uiprint_ListPrinters P ((char ***, int *));
extern char *uiprint_GetPrintCmdInfo P ((char **));
extern uiprint_HdrType_t uiprint_GetDefaultHdrType P ((void));

#endif /* _UIPRINT_H_ */
