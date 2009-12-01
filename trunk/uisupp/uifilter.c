#include <zmail.h>
#include <general.h>
#include <uifilter.h>
#include <uichoose.h>
#include <dynstr.h>

#include <except.h>

#ifndef lint
static char	uifilter_rcsid[] =
    "$Id: uifilter.c,v 1.8 1995/07/14 05:30:01 schaefer Exp $";
#endif

uifilter_t *
uifilter_Create()
{
    uifilter_t *filt;

    filt = (uifilter_t *) calloc(sizeof *filt, 1);
    uiact_Init(&filt->action);
    uipick_Init(&filt->pick);
    return filt;
}

void
uifilter_Delete(uif)
uifilter_t *uif;
{
    uiact_Destroy(&uif->action);
    xfree(uif->name);
    uipick_Destroy(&uif->pick);
    xfree(uif);
}

char **
uifilter_List()
{
    char **vec = NULL;
    struct options *tmp;

    if (!filters)
	return DUBL_NULL;
    tht_thread(optlist2tht(&filters), &filters);
    tmp = filters;
    do {
	vcatstr(&vec, tmp->option);
    } while ((tmp = tmp->next) != 0);
    return vec;
}

zmBool
uifilter_Remove(no)
int no;
{
    char buf[256];
    struct options *opt;

    opt = filters;
    while (opt && no--) opt = opt->next;
    if (!opt) return False;
    sprintf(buf, "unfilter '%s'", opt->option);
    return uiscript_Exec(buf, 0) == 0;
}

uifilter_t *
uifilter_Get(no)
int no;
{
    zmFunction *func;
    zmBool new = False;
    struct options *opt = filters;
    char *name;
    uifilter_t *f;

    for (; opt && no--; opt = opt->next);
    if (!opt) return NULL;
    name = opt->option;
    func = (zmFunction *) retrieve_link((struct link *) folder_filters,
	name, strcmp);
    if (!func) {
	new = True;
	func = (zmFunction *) retrieve_link((struct link *) new_filters,
	    name, strcmp);
	if (!func)
	    return NULL;
    }
    f = uifilter_Create();
    if (!f) return NULL;
    uifilter_SetName(f, func->f_link.l_name);
    uipick_Parse(&f->pick, func->f_cmdv+1);
    uiact_InitFrom(&f->action, func->f_cmdv[0]);
    if (new)
	uifilter_SetFlags(f, uifilter_NewMail);
    return f;
}

zmBool
uifilter_Install(uif)
uifilter_t *uif;
{
    struct dynstr cmd, acttext;
    zmBool ret = False;
    char **argv, *s = NULL;

    dynstr_Init(&cmd);
    dynstr_Init(&acttext);
    TRY {
	do {
	    dynstr_Set(&cmd, "filter ");
	    if (uifilter_GetFlags(uif, uifilter_NewMail))
		dynstr_Append(&cmd, " -n ");
	    dynstr_Append(&cmd, quotezs(uifilter_GetName(uif), 0));
	    dynstr_AppendChar(&cmd, ' ');
	    if (!uiact_GetScript(&acttext, &uif->action))
		break;
	    dynstr_Append(&cmd, quotezs(dynstr_Str(&acttext), 0));
	    dynstr_AppendChar(&cmd, ' ');
	    if (!(argv = uipick_MakeCmd(&uif->pick, True)))
		break;
	    s = argv_to_string(NULL, argv+1);
	    free_vec(argv);
	    dynstr_Append(&cmd, s);
	    ret = (uiscript_Exec(dynstr_Str(&cmd), 0) == 0);
	} while (0);
    } EXCEPT(ANY) {
	/* nothing */;
    } FINALLY {
	dynstr_Destroy(&cmd);
	dynstr_Destroy(&acttext);
	xfree(s);
    } ENDTRY;
    return ret;
}

static void
uifilter_GenName(uif, buf)
uifilter_t *uif;
char *buf;
{
    uiact_t *act = uifilter_GetAction(uif);
    uipickpat_t *pat;
    int i;
    char *p, *q;
    
    strcpy(buf, uiact_GetTypeDesc(act));
    if (uiact_NeedsArg(act))
	p = uiact_GetArg(act);
    else
	uipick_FOREACH(uifilter_GetPick(uif), pat, i) {
	    if ((p = uipickpat_GetPattern(pat)) != NULL)
		break;
	}
    if (p) {
	strcat(buf, "-");
	strcat(buf, p);
    }
    p = q = buf;
    while (*q) {
	if (isalpha(*q)) {
	    *p++ = tolower(*q);
	    ++q;
	} else {
	    *p++ = '-';
	    while (*q && !isalpha(*q)) q++;
	}
    }
    *p = 0;
}

zmBool
uifilter_SupplyName(uif)
uifilter_t *uif;
{
    char buf[200];
    uichoose_t ch;
    zmBool ret = False;

    if (uifilter_GetName(uif))
	return True;
    uifilter_GenName(uif, buf);
    uichoose_Init(&ch);
    uichoose_SetQuery(&ch,
	catgets(catalog, CAT_UISUPP, 62, "Enter a name for this filter."));
    uichoose_SetDefault(&ch, buf);
    if (uichoose_Ask(&ch)) {
	uifilter_SetName(uif, uichoose_GetResult(&ch));
	ret = True;
    }
    uichoose_Destroy(&ch);
    return ret;
}
