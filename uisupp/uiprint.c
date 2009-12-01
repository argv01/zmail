#include <zmail.h>
#include <general.h>
#include <uiprint.h>
#include <dynstr.h>
#include <glob.h>

#include <except.h>

#ifndef lint
static char	uiprint_rcsid[] =
    "$Id: uiprint.c,v 1.13 1995/05/19 01:07:48 tom Exp $";
#endif

char *
uiprint_GetPrintCmdInfo(s)
char **s;
{
    char *cmd = value_of(VarPrintCmd);
    if (s) {
	*s = (cmd) ?
	    catgets( catalog, CAT_UISUPP, 70, "Print Command:" ) :
	    catgets( catalog, CAT_UISUPP, 71, "Printer Name: " );
    }
    return cmd;
}

int
uiprint_ListPrinters(vec, dflt)
char ***vec;
int *dflt;
{
    char **printers;
    char *dflt_name, *pr;
    int count, i;
    
    if (pr = value_of(VarPrinter)) {
	if (!*pr)
	    return 0;
	printers = strvec(pr, ", \t", TRUE);
    } else
	printers = unitv(DEF_PRINTER);
    if (!printers)
	return -1;
    *dflt = 0;
    *vec = printers;
    count = vlen(printers);
#ifdef PRINTER_NAMES_SORTED
    dflt_name = printers[0];
    qsort((char *) printers, count, sizeof(char *),
	  (int (*)NP((CVPTR, CVPTR))) strptrcmp);
    for (i = 0; i < count; i++)
	if (dflt_name == printers[i]) {
	    *dflt = i;
	    break;
	}
#endif
    return count;
}

void
uiprint_Init(up)
uiprint_t *up;
{
    bzero((VPTR) up, sizeof *up);
    uiprint_SetHdrType(up, uiprint_HdrStd);
}

void
uiprint_Destroy(up)
uiprint_t *up;
{
    xfree(up->name);
    xfree(up->cmd);
}

zmBool
uiprint_Print(up)
uiprint_t *up;
{
    struct dynstr cmd;
    zmBool ret = False;
    char *prname, *prcmd;

    dynstr_Init(&cmd);
    TRY {
	dynstr_Set(&cmd, "lpr ");
	switch (uiprint_GetHdrType(up)) {
	case uiprint_HdrStd:
	    dynstr_Append(&cmd, " -h "); break;
	when uiprint_HdrNone:
	    dynstr_Append(&cmd, " -n "); break;
	when uiprint_HdrAll:
	    dynstr_Append(&cmd, " -a "); break;
	}
	if (uiprint_GetFlags(up, uiprint_SingleProcess))
	    dynstr_Append(&cmd, " -s ");
	prname = uiprint_GetPrinterName(up);
	if (prname) {
	    dynstr_Append(&cmd, " -P");
	    dynstr_Append(&cmd, quotezs(prname, 0));
	}
	prcmd = uiprint_GetPrintCmd(up);
	if (prcmd) {
	    dynstr_Append(&cmd, " -c");
	    dynstr_Append(&cmd, quotezs(prcmd, 0));
	}
	ret = (uiscript_Exec(dynstr_Str(&cmd), 0) == 0);
    } EXCEPT(ANY) {
	/* nothing */;
    } FINALLY {
	dynstr_Destroy(&cmd);
    } ENDTRY;
    return ret;
}

uiprint_HdrType_t
uiprint_GetDefaultHdrType()
{
    return (bool_option(VarAlwaysignore, "print,printer")) ?
	uiprint_HdrStd : uiprint_HdrAll;
}
