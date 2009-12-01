/* 
 * $RCSfile: dsearch.h,v $
 * $Revision: 2.5 $
 * $Date: 1995/09/20 06:44:04 $
 * $Author: liblit $
 */

#ifndef DSEARCH_H
# define DSEARCH_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/cmdline.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/listv.h>
#include <spoor/textview.h>

#include <zfolder.h>

struct dsearch {
    SUPERCLASS(dialog);
    struct {
	struct spCmdline *y, *m, *d;
    } d1, d2;
    struct {
	struct spText *y, *m, *d;
    } d2text;
    struct {
	struct spToggle *constrain, *received, *nonmatches;
	struct spToggle *allopen, *function, *ondate;
	struct spToggle *beforedate, *afterdate, *between;
	struct spToggle *select, *viewonly;
    } toggles;
    struct spButtonv *options, *search, *result;
    struct spListv *functions;
    struct spTextview *instructions;
    struct spWrapview *optionswrap;
    msg_group mg;
};

/* Add field accessors */

/* Declare method selectors */

extern struct spWclass *dsearch_class;

extern struct spWidgetInfo *spwc_Datesearch;

extern void dsearch_InitializeClass P((void));

#define dsearch_NEW() \
    ((struct dsearch *) spoor_NewInstance(dsearch_class))

#endif /* DSEARCH_H */
