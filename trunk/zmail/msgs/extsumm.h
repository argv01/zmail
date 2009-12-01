#ifndef _EXTSUMM_H_
#define _EXTSUMM_H_

/*
 * $RCSfile: extsumm.h,v $
 * $Revision: 2.5 $
 * $Date: 1995/10/05 05:22:53 $
 * $Author: liblit $
 */

#include <general.h>
#include <glist.h>
#include "zmflag.h"

struct _esumm {
    struct glist gl;
    long total_width;
};
typedef struct _esumm esumm_t;

#define esumm_FOREACH(E,V,I) glist_FOREACH(&(E)->gl,esummseg_t,V,I)

typedef enum {
    esumm_Invalid,
    esumm_MsgFmt,
    esumm_FolderFmt,
    esumm_Status
} esumm_Segtype;

struct _esummseg {
    esumm_Segtype type;
    char *data;
    long width;
    int charwidth;
    unsigned long flags;
};
typedef struct _esummseg esummseg_t;

#define ESUMSF_RIGHT_JUST	ULBIT(0)
#define ESUMSF_CENTER_JUST	ULBIT(1)

extern int esumm_Parse P ((esumm_t *, const char *));
extern void esumm_Convert P ((esumm_t *, int));
extern void esumm_Update P ((esumm_t *));
extern void esumm_Init P ((esumm_t *));
extern void esumm_Destroy P ((esumm_t *));
extern int esumm_Split P ((esumm_t *, int));
extern void esumm_Remove P ((esumm_t *, int));
extern void esummseg_Destroy P ((esummseg_t *));
extern void esumm_ChangeAllWidths P((esumm_t *, long));

#define esumm_SetTotalWidth(ES, W) ((ES)->total_width = (W))
#define esumm_GetSegment(ES, N) ((esummseg_t *) glist_Nth(&(ES)->gl, (N)))
#define esummseg_GetWidth(ESG) ((ESG)->width)
#define esummseg_GetCharWidth(ESG) ((ESG)->charwidth)
#define esumm_GetTotalWidth(ES) ((ES)->total_width)
#define esummseg_SetWidth(ESG, W) ((ESG)->width = (W))
#define esummseg_GetFormat(ESG) ((ESG)->data)
#define esumm_GetSegCount(ES) (glist_Length(&(ES)->gl))
#define esummseg_SetFormat(ESG, FM) (str_replace(&(ESG)->data, (FM)))
#define esummseg_GetFlags(ESG, FL) (ison((ESG)->flags, (FL)))
#define esummseg_SetFlags(ESG, FL) (turnon((ESG)->flags, (FL)))
#define esummseg_ClearFlags(ESG, FL) (turnoff((ESG)->flags, (FL)))
#define esummseg_GetType(ESG) ((ESG)->type)

#endif /* _EXTSUMM_H_ */
