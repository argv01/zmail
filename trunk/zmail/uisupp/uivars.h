#ifndef _UIVARS_H_
#define _UIVARS_H_

/*
 * $RCSfile: uivars.h,v $
 * $Revision: 1.5 $
 * $Date: 1995/10/05 05:28:04 $
 * $Author: liblit $
 */

#include "uisupp.h"
#include "zmflag.h"

#define uivar_FIELD_TEXT catgets(catalog, CAT_UISUPP, 3, "Press RETURN to set new value.")
#define uivar_SLIDER_TEXT catgets(catalog, CAT_UISUPP, 4, "Move slider to change value.")

#define uivar_IsReadonly(V)  	(ison((V)->v_flags, V_READONLY|V_ADMIN))
#define uivar_IsBoolean(V)   	(ison((V)->v_flags, V_BOOLEAN))
#define uivar_IsString(V)    	(ison((V)->v_flags, V_STRING))
#define uivar_IsSingleval(V)	(ison((V)->v_flags, V_SINGLEVAL))
#define uivar_IsMultival(V)	(ison((V)->v_flags, V_MULTIVAL))
#define uivar_HasSubVals(V)	(ison((V)->v_flags, V_SINGLEVAL|V_MULTIVAL))
#define uivar_IsNumeric(V)	(ison((V)->v_flags, V_NUMERIC))

#define uivar_GetName(V)	((V)->v_opt)
#define uivar_GetPrompt(V)	((V)->v_prompt.v_label)
#define uivar_GetNumMax(V)	((V)->v_gui_max)
#define uivar_GetCategory(V)	((V)->v_category)
#define uivar_GetSubValCount(V)	((V)->v_num_vals)
#define uivar_GetSubValLabel(V, N) ((V)->v_values[(N)].v_label)

#define uivarlist_Init_mac	V_MAC
#define uivarlist_Init_lite	V_VUI
#define uivarlist_Init_motif	V_GUI
#define uivarlist_Init_win	V_MSWINDOWS
#define uivarlist_Init_tty	V_TTY

typedef struct Variable *uivar_t;

struct _uivarvalue {
    uivar_t var;
    char *val;
    unsigned long subvals;
};
typedef struct _uivarvalue uivarvalue_t;

#define uivarvalue_GetSubValBool(V, N) (ison((V)->subvals, 1<<N))
#define uivarvalue_GetBool(V) (!!(V)->val)

struct _uivarlist {
    int *list_pos, *pos_list;
    int count;
};
typedef struct _uivarlist uivarlist_t;

#define uivarlist_GetCount(vl) ((vl)->count)
#define uivarlist_FOREACH(vl,v,i) \
    for (i = 0; \
	     (i < (vl)->count) && (v = &variables[(vl)->list_pos[i]]); i++)

extern char *uivar_GetLongDescription P ((uivar_t));
extern zmBool uivar_SetSubVal P ((uivar_t, int, zmBool));
extern zmBool uivar_SetBool P ((uivar_t, zmBool));
extern zmBool uivar_SetStr P ((uivar_t, const char *));
extern zmBool uivar_SetLong P ((uivar_t, long));
extern zmBool uivarvalue_GetLong P ((uivarvalue_t *, long *));
extern zmBool uivarvalue_GetStr P ((uivarvalue_t *, char **));
extern void uivar_GetValue P ((uivar_t, uivarvalue_t *));
extern void uivarlist_Destroy P ((uivarlist_t *));
extern uivar_t uivarlist_GetVar P ((uivarlist_t *, int));
extern zmBool uivarlist_Init P ((uivarlist_t *, unsigned long, unsigned long));
extern zmBool uivarvalue_Copy P ((uivarvalue_t *, uivarvalue_t *));
extern void uivarvalue_Destroy P ((uivarvalue_t *));
extern zmBool uivar_SetValue P ((uivar_t, uivarvalue_t *));

extern unsigned long *uivar_category_values;
extern char **uivar_category_names;
extern unsigned int uivar_num_categories;

#endif /* _UIVARS_H_ */
