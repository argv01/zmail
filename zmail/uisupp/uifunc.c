#include <zmail.h>
#include <general.h>
#include <uifunc.h>
#include <dynstr.h>
#include <glob.h>
#include <zmsource.h>
#include <funct.h>

#include <except.h>

#ifndef lint
static char	uifunc_rcsid[] =
    "$Id: uifunc.c,v 1.13 1995/05/19 01:07:46 tom Exp $";
#endif

zmBool
uifunctions_List(retptr, do_sort)
char ***retptr;
zmBool do_sort;
{
    zmFunction *tmp = function_list;
    int count;
    char **ptr;

    *retptr = DUBL_NULL;
    if (!tmp) return 0;
    count = number_of_links(tmp);
    *retptr = ptr = (char **) calloc(count+1, sizeof *ptr);
    do {
	*ptr++ = tmp->f_link.l_name;
	tmp = (zmFunction *)(tmp->f_link.l_next);
    } while (tmp != function_list);
    *ptr = NULL;
    if (do_sort)
	qsort((char *)*retptr, count, sizeof(char *),
	      (int (*)NP((CVPTR, CVPTR))) strptrcmp);
    return count;
}

zmBool
uifunctions_Add(fname, text, fname_item, script_item)
const char *fname, *text;
GuiItem fname_item, script_item;
{
    Source *ss;
    int n = 0, was_warning;

    if (!fname || !*fname) {
	if (fname_item) ask_item = fname_item;
	error(UserErrWarning,
	    catgets( catalog, CAT_UISUPP, 63, "You must provide a function name." ));
	return False;
    }
    if (index(fname, ' ')) {
	if (fname_item) ask_item = fname_item;
	error(UserErrWarning,
	    catgets( catalog, CAT_UISUPP, 64, "Function names may not contains spaces." ));
	return False;
    }
    if (!text || !*text) {
	if (script_item) ask_item = script_item;
	error(HelpMessage,
	    catgets( catalog, CAT_UISUPP, 65, "Please provide function text." ));
	return False;
    }
    was_warning = ison(glob_flags, WARNINGS);
    turnon(glob_flags, WARNINGS);
    /* XXX casting away const */
    ss = Sinit(SourceString, (VPTR) text);
    n = load_function(fname, NULL,
	catgets(catalog, CAT_UISUPP, 66, "Functions"), ss, &n);
    Sclose(ss);
    if (!was_warning)
	turnoff(glob_flags, WARNINGS);
    return n == 0;
}

zmBool
uifunctions_Delete(name)
const char *name;
{
    int was_warning = ison(glob_flags, WARNINGS);
    int ret;

    if (!name || !*name) {
	error(UserErrWarning,
	    catgets( catalog, CAT_UISUPP, 63, "You must provide a function name." ));
	return False;
    }
    turnon(glob_flags, WARNINGS);
    ret = uiscript_Exec(zmVaStr("\\function -d %s", quotezs(name, 0)), 0);
    if (!was_warning)
	turnoff(glob_flags, WARNINGS);
    return ret == 0;
}

zmBool
uifunctions_GetText(name, sep, dstr)
const char *name, *sep;
struct dynstr *dstr;
{
    zmFunction *tmp = lookup_function(name);
    char **line;
    int ret = False;
    
    dynstr_Init(dstr);
    if (!tmp) return False;
    TRY {
	for (line = tmp->f_cmdv; line && *line; line++) {
	    dynstr_Append(dstr, *line);
	    dynstr_Append(dstr, sep);
	}
	ret = True;
    } EXCEPT(ANY) {
	dynstr_Destroy(dstr);
    } ENDTRY;
    return ret;
}

void
uifunctions_HelpScript(str)
const char *str;
{
    char *s, *cmd = savestr(str);
    zmFunction *tmp;

    for (s = cmd; *s && !isspace(*s); s++);
    if (*s) *s = 0;
    if (tmp = lookup_function(cmd))
	help(0, tmp->help_text, 0);
    else
	help(0, cmd, cmd_help);
    xfree(cmd);
}
