#include <zmail.h>
#include <uichoose.h>

#ifndef lint
static char	uichoose_rcsid[] =
    "$Id: uichoose.c,v 1.3 1994/03/24 22:10:35 pf Exp $";
#endif

void
uichoose_Init(ch)
uichoose_t *ch;
{
    bzero((VPTR) ch, sizeof *ch);
    dynstr_Init(&(ch)->result);
}

void
uichoose_Destroy(ch)
uichoose_t *ch;
{
    dynstr_Destroy(&(ch)->result);
}

zmBool
uichoose_Ask(ch)
uichoose_t *ch;
{
    return dyn_choose_one(&(ch)->result,
	uichoose_GetQuery(ch), uichoose_GetDefault(ch),
	NULL, 0, 0) == 0;
}
