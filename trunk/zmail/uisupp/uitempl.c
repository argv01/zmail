#include <zmail.h>
#include <general.h>
#include <uitempl.h>
#include <uichoose.h>
#include <dynstr.h>
#include <glob.h>

#include <except.h>

#ifndef lint
static char	uitempl_rcsid[] =
    "$Id: uitempl.c,v 1.10 1995/10/05 05:28:03 liblit Exp $";
#endif

static char *copy_template P ((const char *, const char *));
static char *get_writable_templ_dir();

zmBool
uitemplate_Mail(name)
const char *name;
{
    struct dynstr cmd;
    int ret = -1;

    dynstr_Init(&cmd);
    TRY {
	dynstr_Set(&cmd, "builtin mail -p ");
	dynstr_Append(&cmd, quotezs(name, 0));
	ret = uiscript_Exec(dynstr_Str(&cmd), 0);
    } FINALLY {
	dynstr_Destroy(&cmd);
    } ENDTRY;
    return ret == 0;
}

zmBool
uitemplate_Edit(name)
const char *name;
{
    char *path = NULL, *newpath;
    struct dynstr cmd;
    int ret = -1;

    dynstr_Init(&cmd);
    TRY {
	path = get_template_path(name, False);
	if (path) {
	    if (Access(path, W_OK) != 0) {
		newpath = copy_template(path, name);
		xfree(path);
		path = newpath;
	    }
	    dynstr_Set(&cmd, "builtin page -e ");
	    dynstr_Append(&cmd, quotezs(path, 0));
	    ret = uiscript_Exec(dynstr_Str(&cmd), 0);
	}
    } EXCEPT(ANY) {
	/* empty */
    } FINALLY {
	dynstr_Destroy(&cmd);
	xfree(path);
    } ENDTRY;
    return ret == 0;
}

zmBool
uitemplate_New()
{
    char *dir = NULL;
    uichoose_t choice;
    struct dynstr cmd;
    int ret = False;

    dynstr_Init(&cmd);
    uichoose_Init(&choice);
    TRY {
	uichoose_SetQuery(&choice,
	    catgets(catalog, CAT_UISUPP, 72, "Enter name of new template:"));
	if (uichoose_Ask(&choice)) {
	    dynstr_Set(&cmd, "page -E ");
	    dir = get_writable_templ_dir();
	    if (dir) {
		dynstr_Append(&cmd, dir);
		dynstr_AppendChar(&cmd, '/');
		dynstr_Append(&cmd, quotezs(uichoose_GetResult(&choice), 0));
		ret = uiscript_Exec(dynstr_Str(&cmd), 0) == 0;
	    }
	}
    } EXCEPT(ANY) {
	/* empty */
    } FINALLY {
	xfree(dir);
	dynstr_Destroy(&cmd);
	uichoose_Destroy(&choice);
    } ENDTRY;
    return ret;
}

zmBool
uitemplate_List(retptr)
char ***retptr;
{
    char **names;
    int i, j, x, y;

    *retptr = DUBL_NULL;
    if ((x = list_templates(&names, NULL, 0)) <= 0)
	return x;

    /* Reduce list to unique basenames, and remove blanks */
    for (i = j = 0; i < x; i++) {
	char *p = names[i];
	names[j] = savestr(basename(p));
	xfree(p);
	if (names[j][0]) j++;
    }
    
    y = qsort_and_crunch((char *)names, j, sizeof(char *), strptrcmp);

    names[y] = NULL;
    *retptr = names;

    return y;
}

static char *
copy_template(path, name)
const char *path, *name;
{
    char *dir;
    char newpath[MAXPATHLEN];
    FILE *in, *out;

    if (!(in = fopen(path, "r"))) {
	error(SysErrWarning, "couldn't open %s for reading", path);
	return NULL;
    }
    dir = get_writable_templ_dir();
    if (!dir) return NULL;
    sprintf(newpath, "%s/%s", dir, name);
    if (!(out = fopen(newpath, "w"))) {
	error(SysErrWarning, "couldn't open %s for writing", path);
	xfree(dir); fclose(in);
	return NULL;
    }
    if (fioxlate(in, 0, -1, out, (long (*)())0, NULL) < 0) {
	error(SysErrWarning, "couldn't copy template");
	fclose(in); fclose(out); xfree(dir);
	return NULL;
    }
    fclose(in);
    fclose(out);
    xfree(dir);
    return savestr(newpath);
}

static char *
get_writable_templ_dir()
{
    char **templ = DUBL_NULL, **pptr, *p, *dir = NULL;

    if (p = value_of(VarTemplates)) {
	templ = strvec(p, " \t", TRUE);
	for (pptr = templ; *pptr; pptr++)
	{
	  char *fp;
	  int isdir = ZmGP_DontIgnoreNoEnt;

	  fp = getpath(*pptr, &isdir);
	  if (ZmGP_Dir == isdir && 0 == Access(fp, W_OK))
	    break;
	}
	if (*pptr)
	    dir = savestr(*pptr);
	free_vec(templ);
    }
    if (!dir)
	error(UserErrWarning, catgets(catalog, CAT_UISUPP, 73, "There are no writable directories in your template path.\nEdit the templates variable to correct this."));
    return dir;
}
