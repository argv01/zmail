/* 
 * $RCSfile: zync_zsiz.c,v $
 * $Revision: 1.5 $
 * $Date: 1995/07/10 22:59:47 $
 * $Author: bobg $
 */

#include "popper.h"

static const char zync_zsiz_rcsid[] =
    "$Id: zync_zsiz.c,v 1.5 1995/07/10 22:59:47 bobg Exp $";

int
zync_zsiz(p)
    POP *p;
{
    int num = atoi(p->pop_parm[1]);
    struct msg_info *info;

    do_drop(p);

    info = NTHMSG(p, num);
    pop_msg(p, POP_SUCCESS, "%ld",
	    info->header_length + info->body_length);
    return (POP_SUCCESS);
}
