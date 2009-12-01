#include <zmail.h>
#include <general.h>
#include <uisort.h>
#include <dynstr.h>

#include <except.h>

#ifndef lint
static char	uisort_rcsid[] =
    "$Id: uisort.c,v 1.7 1995/05/19 01:07:49 tom Exp $";
#endif

char *sort_type_flags = "dsalpS";

catalog_ref sort_types[] = {
    catref(CAT_UISUPP, 13, "date sent"),
    catref(CAT_UISUPP, 14, "subject"),
    catref(CAT_UISUPP, 15, "author"),
    catref(CAT_UISUPP, 16, "message length"),
    catref(CAT_UISUPP, 17, "message priority"),
    catref(CAT_UISUPP, 18, "message status")
};

void
uisort_Init(uis)
uisort_t *uis;
{
    bzero((VPTR) uis, sizeof *uis);
}

void
uisort_Destroy(uis)
uisort_t *uis;
{
    return;
}

void
uisort_AddIndex(uis, ind)
uisort_t *uis;
uisort_index_t ind;
{
    if (ison(uis->inuse, uisort_IndBit(ind)))
	return;
    uis->indices[uis->count++] = ind;
    turnon(uis->inuse, uisort_IndBit(ind));
}

void
uisort_RemoveIndex(uis, ind)
uisort_t *uis;
uisort_index_t ind;
{
    int i;
    
    if (isoff(uis->inuse, uisort_IndBit(ind)))
	return;
    for (i = 0; i != uis->count; i++)
	if (uis->indices[i] == ind) break;
    for (; i != uis->count; i++)
	uis->indices[i] = uis->indices[i+1];
    uis->count--;
    turnoff(uis->inuse, uisort_IndBit(ind));
}

void
uisort_ReverseIndex(uis, ind, how)
uisort_t *uis;
uisort_index_t ind;
zmBool how;
{
    if (how)
	turnon(uis->reverse, uisort_IndBit(ind));
    else
	turnoff(uis->reverse, uisort_IndBit(ind));
}

char **
uisort_DescribeSort(uis)
uisort_t *uis;
{
    int i;
    char buf[100];
    char **vec = DUBL_NULL;

    for (i = 0; i != uis->count; i++) {
	const char *name;
	uisort_index_t ind = uis->indices[i];
	if (ind != uisort_IndexDate)
	    name = catgetref(sort_types[(int)ind]);
	else if (ison(uis->flags, uisort_FlagDateReceived))
	    name = catgets(catalog, CAT_UISUPP, 19, "date received");
	else
	    name = catgets(catalog, CAT_UISUPP, 13, "date sent");
	sprintf(buf, "%d: %s%s", i+1, name,
	    ison(uis->reverse, uisort_IndBit(ind))
	    ? catgets(catalog, CAT_UISUPP, 21, " (reverse)") : "");
	vcatstr(&vec, buf);
    }
    if (!vec)
	return unitp(NULL);
    return vec;
}

zmBool
uisort_DoSort(uis)
uisort_t *uis;
{
    struct dynstr cmd;
    int i;
    zmBool ret;
    zmFlags old_flags = glob_flags;
    
    if (uis->count == 0) {
	error(UserErrWarning, catgets(catalog, CAT_UISUPP, 22, "Please specify sorting order."));
	return False;
    }
    dynstr_Init(&cmd);
    TRY {
	dynstr_Set(&cmd, "sort ");
	if (ison(uis->flags, uisort_FlagIgnoreCase))
	    dynstr_Append(&cmd, " -i ");
	for (i = 0; i != uis->count; i++) {
	    uisort_index_t ind = uis->indices[i];
	    if (ison(uis->reverse, uisort_IndBit(ind)))
		dynstr_Append(&cmd, " -r ");
	    if (ind == uisort_IndexSubject &&
		    ison(uis->flags, uisort_FlagUseRe))
		dynstr_Append(&cmd, " -R ");
	    else {
		dynstr_Append(&cmd, " -");
		dynstr_AppendChar(&cmd, sort_type_flags[(int)ind]);
	    }
	}
	if (ison(uis->flags, uisort_FlagDateReceived))
	    turnon(glob_flags, DATE_RECV);
	else
	    turnoff(glob_flags, DATE_RECV);
	ret = uiscript_Exec(dynstr_Str(&cmd), 0) == 0;
    } EXCEPT(ANY) {
	/* nothing */
    } FINALLY {
	glob_flags = old_flags;
	dynstr_Destroy(&cmd);
    } ENDTRY;
    return ret;
}

void
uisort_UseOptions(uis, fl)
uisort_t *uis;
zmFlags fl;
{
    uis->flags = fl;
}

