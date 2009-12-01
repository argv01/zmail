/* 
 * $RCSfile: ccfolder.h,v $
 * $Revision: 1.1 $
 * $Date: 1995/07/27 18:19:36 $
 * $Author: schaefer $
 */

#ifndef CCFLDR_H
# define CCFLDR_H

#include <spoor.h>
#include <mfolder.h>
#include <pop.h>
#include <cc_mail.h>

#include <zfolder.h>		/* from zmail/include */
#include <intset.h>		/* from zmail/general */

struct ccfolder {
    SUPERCLASS(mfldr);
    MAILSTREAM *server;
};

/* Add field accessors */
#define ccfolder_server(x) \
    (((struct ccfolder *) (x))->server)

/* Declare method selectors */

extern struct spClass *ccfolder_class;
extern void ccfolder_InitializeClass();

#define ccfolder_NEW() \
    ((struct ccfolder *) spoor_NewInstance(ccfolder_class))

DECLARE_EXCEPTION(ccfolder_err_cc);

extern struct ccfolder *cc_to_mfldr P((MAILSTREAM *));

#endif /* CCFLDR_H */
