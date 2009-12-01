#ifndef _UIACT_H_
#define _UIACT_H_

/*
 * $RCSfile: uiact.h,v $
 * $Revision: 1.8 $
 * $Date: 1994/04/18 04:16:41 $
 * $Author: pf $
 */

/* UI actions */

#include "uisupp.h"

typedef enum {
    uiact_first = 0,
    uiact_SaveByAuthor = 0,
    uiact_SaveBySubject,
    uiact_Save,
    uiact_Copy,
    uiact_Delete,
    uiact_Undelete,
    uiact_Mark,
    uiact_Unmark,
    uiact_Forward,
    uiact_Reply,
    uiact_Script,
    uiact_count
} uiact_Type;

typedef enum {
    uiact_Arg_None = 0,
    uiact_Arg_File,
    uiact_Arg_Address,
    uiact_Arg_Directory,
    uiact_Arg_Script,
    uiact_Arg_Other,
    uiact_Arg_count
} uiact_ArgType;

struct _uiact {
    uiact_Type type;
    char *arg;
};
typedef struct _uiact uiact_t;

struct _uiacttypelist {
    int count;
    uiact_Type *list;
};
typedef struct _uiacttypelist uiacttypelist_t;

#define uiact_GetType(X) ((X)->type)
#define uiact_SetType(X, Y) ((X)->type = (Y))
#define uiact_GetArg(X) ((X)->arg)
#define uiact_SetArg(X, Y) (str_replace(&(X)->arg, (Y)))
#define uiact_NeedsArg(X) (uiact_GetArgType(X) != uiact_Arg_None)

#define uiacttypelist_GetType(X, Y) ((X)->list[(Y)])

extern uiact_ArgType uiacttype_GetArgType P ((uiact_Type));
extern uiact_ArgType uiact_GetArgType P ((uiact_t *));
extern zmBool uiact_SupplyArg P ((uiact_t *));
extern zmBool uiact_GetScript P ((struct dynstr *, uiact_t *));
extern zmBool uiact_Perform P ((uiact_t *));
extern void uiact_Init P ((uiact_t *));
extern void uiact_Destroy P ((uiact_t *));
extern const char *uiact_GetTypeDesc P ((uiact_t *));
extern void uiact_InitFrom P ((uiact_t *, char *));
extern zmBool uiacttype_IsScript P ((uiact_ArgType));
extern char *uiacttype_GetDefaultArg P ((uiact_Type));
extern const char *uiacttype_GetPromptStr P ((uiact_Type));
extern const char *uiacttype_GetMissingStr P ((uiact_Type));

extern char **uiacttypelist_GetDescList P ((uiacttypelist_t *));
extern int uiacttypelist_GetIndex P ((uiacttypelist_t *, uiact_Type));

extern uiacttypelist_t uiacttype_FilterActList;

#endif /* _UIACT_H_ */
