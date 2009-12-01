#include <zmail.h>
#include <general.h>
#include <uivsrch.h>
#include <vars.h>
#include <strcase.h>

#ifndef lint
static char	uivars_rcsid[] =
    "$Id: uivsrch.c,v 1.4 1994/06/28 02:04:43 pf Exp $";
#endif

static zmBool searchvar P ((uivarsearch_t *, int));

void
uivarsearch_Init(uivs, vl)
uivarsearch_t *uivs;
uivarlist_t *vl;
{
    bzero((VPTR) uivs, sizeof *uivs);
    uivs->vl = vl;
}

void
uivarsearch_Destroy(uivs)
uivarsearch_t *uivs;
{
    xfree(uivs->search_str);
}

char *
lcase_strstr(buf, str)
const char *buf, *str;
{
    int len = strlen(str);
    
    for (; *buf; buf++)
	/* XXX casting away const */
	if (!ci_istrncmp(buf, str, len)) return (char *) buf;
    return NULL;
}

static zmBool
searchvar(uivs, vno)
uivarsearch_t *uivs;
int vno;
{
    char *text, *s;
    uivar_t v = uivarlist_GetVar(uivs->vl, vno);
    if (!v) return False;
    uivs->desc = text = uivar_GetLongDescription(v);
    if (!text) return False;
    s = lcase_strstr(text+uivs->offset+1, uivs->search_str);
    if (!s) return False;
    uivs->offset = s-text;
    uivs->end_offset = uivs->offset + strlen(uivs->search_str);
    return True;
}

#ifdef MSDOS
int
uivarsearch_AdjustOffset(uivs, off)
uivarsearch_t *uivs;
int off;
{
    char *s = uivs->desc;
    int ct = off;

    while (ct--)
	if (*s++ == '\n')
	    off++;
    return off;
}
#endif /* MSDOS */

zmBool
uivarsearch_Search(uivs, str, off, varno)
uivarsearch_t *uivs;
const char *str;
int off, varno;
{
    if (!uivs->search_str || strcmp(str, uivs->search_str))
	uivarsearch_EndSearch(uivs);
    if (!uivs->search_str) {
	uivs->first_var = varno;
	uivs->varno = -1;
	uivs->offset = off;
	str_replace(&uivs->search_str, str);
	turnoff(uivs->flags, uivarsearch_XREF);
    }
    for (; uivs->varno < uivarlist_GetCount(uivs->vl); uivs->varno++) {
	if (uivs->varno == uivs->first_var) continue;
	if (searchvar(uivs, (uivs->varno == -1) ?
	    	uivs->first_var : uivs->varno))
	    return True;
	uivs->offset = -1;
    }
    return False;
}

void
uivarsearch_ReportNoMatch(uivs)
uivarsearch_t *uivs;
{
    error(UserErrWarning, uivarsearch_NO_MATCHES, uivs->search_str);
    uivarsearch_EndSearch(uivs);
}

void
uivarsearch_EndSearch(uivs)
uivarsearch_t *uivs;
{
    xfree(uivs->search_str);
    uivs->search_str = NULL;
    uivs->offset = uivs->varno = uivs->first_var -1;
}

void
uivarsearch_SetXref(uivs)
uivarsearch_t *uivs;
{
    uivarsearch_EndSearch(uivs);
    turnon(uivs->flags, uivarsearch_XREF);
}
