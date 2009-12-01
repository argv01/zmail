#ifndef _UIFLDR_H_
#define _UIFLDR_H_

/*
 * $RCSfile: uifldr.h,v $
 * $Revision: 1.5 $
 * $Date: 1995/10/05 05:28:00 $
 * $Author: liblit $
 */

#include <uisupp.h>
#include <glist.h>
#include <gptrlist.h>
#include "zmflag.h"

#define uifolderlist_NO_FOLDER_STR catgets(catalog, CAT_UISUPP, 2, "[No folder]")

struct _uifolder {
    int fno;
    zmFlags flags;
    msg_folder *fldr;
    char *short_name, *long_name, *basename, *filename;
};
typedef struct _uifolder uifolder_t;

#define uifolder_GetShortName(UIF) ((UIF)->short_name)
#define uifolder_GetLongName(UIF) ((UIF)->long_name)
#define uifolder_GetFolder(UIF) ((UIF)->fldr)
#define uifolder_GetFolderNo(UIF) ((UIF)->fno)
#define uifolder_IsSpool(UIF) (ison((UIF)->flags, uifolder_Spool))

extern zmBool uifolder_ChangeTo P ((uifolder_t *));
extern char *uifolder_GetFolderFilename P ((uifolder_t *));

struct _uifolderlist {
    struct glist flist;
    struct gintlist map;
    zmBool changed;
    int seqno;
};
typedef struct _uifolderlist uifolderlist_t;

#define uifolder_Spool ULBIT(0)
#define uifolder_Disambiguated ULBIT(1)

extern int uifolderlist_Sort P ((uifolderlist_t *));
extern void uifolderlist_Init P ((uifolderlist_t *));
extern uifolder_t *uifolderlist_Get P ((uifolderlist_t *, msg_folder *));
extern uifolder_t *uifolderlist_GetAt P ((uifolderlist_t *, int));
extern void uifolderlist_Remove P ((uifolderlist_t *, msg_folder *));
extern void uifolderlist_Add P ((uifolderlist_t *, msg_folder *));
extern int uifolderlist_GetIndex P ((uifolderlist_t *, msg_folder *));

#define uifolderlist_FOREACH(fl, f, i) glist_FOREACH(&(fl)->flist, uifolder_t, f, i)

#endif /* _UIFLDR_H_ */
