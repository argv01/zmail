/* UI actions */

#include <zmail.h>
#include <general.h>
#include <uiact.h>
#include <uichoose.h>
#include <dynstr.h>
#include <catalog.h>

#include <except.h>

#ifndef lint
static char	uiact_rcsid[] =
    "$Id: uiact.c,v 1.17 2005/05/09 09:15:23 syd Exp $";
#endif

struct act_type {
    catalog_ref type_desc, arg_desc;
    char *script;
    uiact_ArgType arg_type;
};

struct act_type act_types[] = {
    {
	catref(CAT_UISUPP, 74, "Save by Author"),
	catref(CAT_UISUPP, 75, "Directory:"),
	"save -a %",
	uiact_Arg_Directory
    },
    {
	catref(CAT_UISUPP, 76, "Save by Subject"),
	catref(CAT_UISUPP, 75, "Directory:"),
	"save -s %",
	uiact_Arg_Directory
    },
    {
	catref(CAT_UISUPP, 33, "Save to File"),
	catref(CAT_UISUPP, 34, "Save to file:"),
	"save %",
	uiact_Arg_File
    },
    {
	catref(CAT_UISUPP, 35, "Copy to File"),
	catref(CAT_UISUPP, 36, "Copy to file:"),
	"copy %",
	uiact_Arg_File
    },
    {
	catref(CAT_UISUPP, 37, "Delete"),
	CATREF_NULL,
	"delete",
	uiact_Arg_None
    },
    {
	catref(CAT_UISUPP, 38, "Undelete"),
	CATREF_NULL,
	"undelete",
	uiact_Arg_None
    },
    {
	catref(CAT_UISUPP, 39, "Mark"),
	CATREF_NULL,
	"mark",
	uiact_Arg_None
    },
    {
	catref(CAT_UISUPP, 40, "Unmark"),
	CATREF_NULL,
	"unmark",
	uiact_Arg_None
    },
    {
	catref(CAT_UISUPP, 41, "Forward"),
	catref(CAT_UISUPP, 42, "Address:"),
	"mail -resend %",
	uiact_Arg_Address
    },
    {
	catref(CAT_UISUPP, 43, "Reply"),
	catref(CAT_UISUPP, 44, "Template file:"),
	"zmfilter_reply %",
	uiact_Arg_File,
    },
    {
	catref(CAT_UISUPP, 27, "Z-Script"),
	catref(CAT_UISUPP, 46, "Command:"),
	NULL,
	uiact_Arg_Script
    }
};

struct act_arg_type {
    catalog_ref prompt_str, missing_str, ask_desc;
    char *deflt;
};
struct act_arg_type act_arg_types[] = {
    { CATREF_NULL, CATREF_NULL, CATREF_NULL, NULL },
    {
	catref(CAT_UISUPP, 47, "Filename:"),
	catref(CAT_UISUPP, 48, "You must provide a filename."),
	catref(CAT_UISUPP, 49, "Enter filename:"),
	NULL
    },
    {
	catref(CAT_UISUPP, 42, "Address:"),
	catref(CAT_UISUPP, 51, "You must provide an address."),
	catref(CAT_UISUPP, 52, "Enter address:"),
	NULL
    },
    {
	catref(CAT_UISUPP, 53, "Directory name:"),
	catref(CAT_UISUPP, 54, "You must provide a directory name."),
	catref(CAT_UISUPP, 55, "Enter directory name:"),
	"+"
    },
    {
	catref(CAT_UISUPP, 56, "Z-Script command:"),
	catref(CAT_UISUPP, 57, "You must provide a z-script command."),
	catref(CAT_UISUPP, 58, "Enter Z-Script command:"),
	NULL
    },
    {
	catref(CAT_UISUPP, 59, "Command argument:"),
	catref(CAT_UISUPP, 60, "You must provide a command argument."),
	catref(CAT_UISUPP, 61, "Enter command argument:"),
	NULL
    }
};

#define FILTER_TYPE_COUNT 11
uiact_Type filter_types[FILTER_TYPE_COUNT] = {
    uiact_Save, uiact_SaveByAuthor, uiact_SaveBySubject, uiact_Copy,
    uiact_Mark, uiact_Unmark, uiact_Delete, uiact_Undelete,
    uiact_Forward, uiact_Reply, uiact_Script
};
uiacttypelist_t uiacttype_FilterActList = {
    FILTER_TYPE_COUNT, filter_types
};

static struct act_type *uiacttype_GetActTypeStruct P ((uiact_Type));
static struct act_type *uiact_GetActTypeStruct P ((uiact_t *));
static struct act_arg_type *uiact_GetArgTypeStruct P ((uiact_ArgType));
static struct act_arg_type *uiacttype_GetArgTypeStruct P ((uiact_ArgType));
static struct act_arg_type *uiargtype_GetArgTypeStruct P ((uiact_ArgType));

static zmBool has_magics P ((const char *, int));

uiact_ArgType
uiacttype_GetArgType(type)
uiact_Type type;
{
    return uiacttype_GetActTypeStruct(type)->arg_type;
}

static struct act_arg_type *
uiargtype_GetArgTypeStruct(type)
uiact_ArgType type;
{
    return &act_arg_types[(int) type];
}

static struct act_arg_type *
uiacttype_GetArgTypeStruct(type)
uiact_ArgType type;
{
    return uiargtype_GetArgTypeStruct(uiacttype_GetArgType(type));
}

static struct act_arg_type *
uiact_GetArgTypeStruct(type)
uiact_ArgType type;
{
    return uiacttype_GetArgTypeStruct(uiacttype_GetArgType(type));
}

uiact_ArgType
uiact_GetArgType(act)
uiact_t *act;
{
    return uiacttype_GetArgType(uiact_GetType(act));
}

static struct act_type *
uiacttype_GetActTypeStruct(type)
uiact_Type type;
{
    return &act_types[(int) type];
}

static struct act_type *
uiact_GetActTypeStruct(act)
uiact_t *act;
{
    return uiacttype_GetActTypeStruct(uiact_GetType(act));
}

zmBool
uiact_SupplyArg(act)
uiact_t *act;
{
    struct uichoose ch;
    zmBool ret = False;
    uiact_ArgType type = uiacttype_GetArgType(uiact_GetType(act));
    
    if (uiact_GetArg(act)) return True;
    if (type == uiact_Arg_None) return True;
    uichoose_Init(&ch);
    {
	const char *query =
	    catgetref(uiargtype_GetArgTypeStruct(type)->ask_desc);
	uichoose_SetQuery(&ch, query);
	if (uichoose_Ask(&ch)) {
	    uiact_SetArg(act, uichoose_GetResult(&ch));
	    ret = True;
	}
    }
    uichoose_Destroy(&ch);
    return ret;
}

zmBool
uiacttype_IsScript(type)
uiact_ArgType type;
{
    /* there used to be a uiact_Arg_Function type... */
    return type == uiact_Arg_Script;
}

zmBool
uiact_GetScript(dstr, act)
struct dynstr *dstr;
uiact_t *act;
{
    struct act_type *at = uiact_GetActTypeStruct(act);
    char *scr, *p;
    int prelen;
    zmBool retval = False;

    TRY {
	do {
	    if (uiacttype_IsScript(at->arg_type)) {
		if (!uiact_GetArg(act))
		    break;
		dynstr_Set(dstr, uiact_GetArg(act));
		retval = True;
		break;
	    }
	    scr = at->script;
	    if (!(p = strchr(scr, '%'))) {
		dynstr_Set(dstr, scr);
		retval = True;
		break;
	    }
	    if (!uiact_GetArg(act))
		break;
	    prelen = p-scr;
	    dynstr_AppendN(dstr, scr, prelen);
	    dynstr_Append(dstr, quotezs(uiact_GetArg(act), 0));
	    dynstr_Append(dstr, scr+prelen+1);
	    retval = True;
	} while(0);
    } EXCEPT(ANY) {
	/* nothing */;
    } ENDTRY;
    return retval;
}

zmBool
uiact_Perform(act)
uiact_t *act;
{
    struct dynstr dstr;
    zmBool ret;

    if (!uiact_SupplyArg(act))
	return False;
    dynstr_Init(&dstr);
    if (uiact_GetScript(&dstr, act))
	ret = uiscript_Exec(dynstr_Str(&dstr), 0) == 0;
    dynstr_Destroy(&dstr);
    return ret;
}

void
uiact_Init(act)
uiact_t *act;
{
    act->type = uiact_Script;
    act->arg = NULL;
}

void
uiact_Destroy(act)
uiact_t *act;
{
    xfree(act->arg);
}

const char *
uiact_GetTypeDesc(act)
uiact_t *act;
{
    return catgetref(uiact_GetActTypeStruct(act)->type_desc);
}

const char *
uiacttype_GetPromptStr(type)
uiact_Type type;
{
    return catgetref(uiacttype_GetArgTypeStruct(type)->prompt_str);
}

const char *
uiacttype_GetMissingStr(type)
uiact_Type type;
{
    return catgetref(uiacttype_GetArgTypeStruct(type)->missing_str);
}

void
uiact_InitFrom(act, scr)
uiact_t *act;
char *scr;
{
    int i;
    struct act_type *at;
    char *p;

    for (i = (int)uiact_first; i < (int)uiact_Script; i++) {
	at = &act_types[i];
	if (at->arg_type == uiact_Arg_None) {
	    if (!strcmp(at->script, scr))
		break;
	} else {
	    char *tail, *templ = at->script;
	    int plen;
	    int alen = strlen(scr), tlen, arglen;
	    p = strchr(templ, '%');
	    plen = p - templ;

	    /* match beginning of string. */
	    if (strncmp(templ, scr, plen)) continue;
	    tail = templ+plen+1;
	    tlen = strlen(tail);
	    /* match end of string. */
	    if (tlen > alen-plen || strcmp(tail, scr+alen-tlen)) continue;
	    /* make sure there are no unquoted magic chars in portion
	     * matched by %.
	     */
	    arglen = alen-tlen-plen;
	    if (has_magics(scr+plen, arglen))
		continue;
	    act->arg = malloc(arglen+1);
	    strncpy(act->arg, scr+plen, arglen);
	    (act->arg)[arglen] = 0;
	    strip_quotes(act->arg, act->arg);
	    break;
	}
    }
    if (i == (int) uiact_Script)
	act->arg = savestr(scr);
    act->type = (uiact_Type) i;
}

static zmBool
has_magics(s, len)
const char *s;
int len;
{
    const char *end = s+len;
    const char *magics = "|$; \t";
    
    for (; s < end; s++) {
	if (*s == '\\' && s[1])
	    s++;
	else if (*s == '\'' || *s == '"') {
	    char quote = *s++;
	    while (*s && *s != quote)
		s++;
	} else if (index(magics, *s))
	    return True;
    }
    return False;
}

char **
uiacttypelist_GetDescList(tl)
uiacttypelist_t *tl;
{
    const char **vec = (const char **) malloc((tl->count+1)*sizeof *vec);
    const char **ptr = vec;
    uiact_Type *aptr = tl->list;
    int ct = tl->count;

    while (ct--) {
	struct act_type *at = uiacttype_GetActTypeStruct(*aptr++);
	*ptr++ = catgetref(at->type_desc);
    }
    *ptr = NULL;
    return vec;
}

char *
uiacttype_GetDefaultArg(atype)
uiact_Type atype;
{
    return (uiacttype_GetArgTypeStruct(atype))->deflt;
}

int
uiacttypelist_GetIndex(ul, type)
uiacttypelist_t *ul;
uiact_Type type;
{
    int i;

    for (i = 0; i != ul->count; i++)
	if (type == ul->list[i])
	    return i;
    return -1;
}
