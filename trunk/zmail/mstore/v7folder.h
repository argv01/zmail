/* 
 * $RCSfile: v7folder.h,v $
 * $Revision: 1.5 $
 * $Date: 1995/07/08 01:22:03 $
 * $Author: spencer $
 */

#ifndef V7FOLDER_H
# define V7FOLDER_H

#include <spoor.h>
#include <mfolder.h>

#include <zfolder.h>		/* from zmail/include */

struct v7folder {
    SUPERCLASS(mfldr);
    msg_folder *zfolder;
};

#define v7folder_zfolder(x) \
    (((struct v7folder *) (x))->zfolder)

extern struct spClass *v7folder_class;
extern void v7folder_InitializeClass();

#define v7folder_NEW() \
    ((struct v7folder *) spoor_NewInstance(v7folder_class))

extern struct v7folder *core_to_mfldr P((msg_folder *));

#endif /* V7FOLDER_H */
