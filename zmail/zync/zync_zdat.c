/* 
 * $RCSfile: zync_zdat.c,v $
 * $Revision: 1.7 $
 * $Date: 1995/07/10 22:59:44 $
 * $Author: bobg $
 */

#include "popper.h"
#include <except.h>
#include <intset.h>
#include <mstore/mfolder.h>

static const char zync_zdat_rcsid[] =
    "$Id: zync_zdat.c,v 1.7 1995/07/10 22:59:44 bobg Exp $";

/* date
 * Return the date for a given message
 * Response is "+OK 27836482764"
 * where the number is the mktime() version of the date
 */

int
zync_zdat(p)
    POP *p;
{
    int num = atoi(p->pop_parm[1]);

    do_drop(p);

    if (!(NTHMSG(p, num)->have_date))
	compute_extras(p, 1, &num);
    pop_msg(p, POP_SUCCESS, "%lu", NTHMSG(p, num)->date);
    return (POP_SUCCESS);
}
