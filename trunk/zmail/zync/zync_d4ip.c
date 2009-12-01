/* 
 * $RCSfile: zync_d4ip.c,v $
 * $Revision: 1.2 $
 * $Date: 1995/06/05 14:54:19 $
 * $Author: bobg $
 */

#include "popper.h"

#ifndef lint
static const char zync_d4ip_rcsid[] =
    "$Id: zync_d4ip.c,v 1.2 1995/06/05 14:54:19 bobg Exp $";
#endif /* lint */

/* D4IP
 * Test whether this is a Z-POP server (for Delphi, aka d4i).
 */

#ifdef D4I
int
zync_d4ip(p)
    POP *p;
{
    pop_msg(p, POP_SUCCESS, "T");
    return (POP_SUCCESS);
}
#endif /* D4I */
