/* 
 * $RCSfile: zync_zsmy.c,v $
 * $Revision: 1.5 $
 * $Date: 1995/07/10 22:59:48 $
 * $Author: bobg $
 */

#include "popper.h"
#include <except.h>

static const char zync_zsmy_rcsid[] =
    "$Id: zync_zsmy.c,v 1.5 1995/07/10 22:59:48 bobg Exp $";

int
zync_zsmy(p)
    POP *p;
{
    int num = atoi(p->pop_parm[1]);

    do_drop(p);

    if (!(NTHMSG(p, num)->summary))
	compute_extras(p, 1, &num);
    pop_msg(p, POP_SUCCESS, "%s", NTHMSG(p, num)->summary);
    return (POP_SUCCESS);
}
