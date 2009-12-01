#include <zmail.h>
#include <general.h>
#include <uivars.h>
#include <vars.h>

#ifndef lint
static char	uivars_rcsid[] =
    "$Id: uivars.c,v 1.8 1998/12/07 23:52:15 schaefer Exp $";
#endif

static u_long _uivar_category_values[] = {
    0,			/* All */
    ULBIT(0),		/* Compose */
    ULBIT(1),		/* Message */
    ULBIT(2),		/* User Interface */
    ULBIT(3),		/* Z-Script */
    ULBIT(4),		/* Localization */
    ULBIT(5),		/* LDAP */
    ULBIT(6),		/* POP */
    ULBIT(7),		/* IMAP */
    ULBIT(8),		/* Preferences */
};
u_long *uivar_category_values = _uivar_category_values;

static catalog_ref _uivar_category_names[] = {
    catref(CAT_UISUPP, 23, "All Variables"),
    catref(CAT_UISUPP, 24, "Compositions"),
    catref(CAT_UISUPP, 25, "Messages"),
    catref(CAT_UISUPP, 26, "User Interface"),
    catref(CAT_UISUPP, 27, "Z-Script"),
    catref(CAT_UISUPP, 28, "Localization"),
    catref(CAT_UISUPP, 29, "LDAP"),
    catref(CAT_UISUPP, 30, "POP"),
    catref(CAT_UISUPP, 31, "IMAP"),
    catref(CAT_UISUPP, 32, "User Preferences")
};
char **uivar_category_names;

unsigned int uivar_num_categories;

#define uivar_GetVarIndex(V) (V-variables)

zmBool
uivarlist_Init(uiv, vflag, category)
uivarlist_t *uiv;
unsigned long vflag;
unsigned long category;
{
    int n, pos = 0;
    int *var_list_pos, *var_pos_list;
    long vignoreflag;
    
    if (!var_list_pos)
	var_list_pos = uiv->list_pos = (int *) malloc(n_variables*sizeof(int));
    if (!var_pos_list)
	var_pos_list = uiv->pos_list = (int *) malloc(n_variables*sizeof(int));
    if (!uivar_category_names) {
	uivar_num_categories =
	    (sizeof(_uivar_category_names) / sizeof(*_uivar_category_names));
	uivar_category_names = catgetrefvec(_uivar_category_names,
					    uivar_num_categories);
    }

    /* extract only the appropriate variables */
    vignoreflag = (V_VUI|V_MAC|V_MSWINDOWS|V_CURSES|V_GUI) & ~vflag;
    for (n = 0; n < n_variables; n++) {
	var_list_pos[pos] = n;
	if (ison(variables[n].v_flags, vignoreflag)
		&& !ison(variables[n].v_flags, vflag)
		|| category && !(variables[n].v_category & category))
	    var_pos_list[n] = -1;
	else {
	    var_pos_list[n] = pos;
	    pos++;
	}
    }
    uiv->count = pos;
    return True;
}

uivar_t
uivarlist_GetVar(uiv, n)
uivarlist_t *uiv;
int n;
{
    if (n < 0 || n >= uiv->count) return NULL;
    return &variables[uiv->list_pos[n]];
}

void
uivarlist_Destroy(uiv)
uivarlist_t *uiv;
{
    xfree(uiv->list_pos);
    xfree(uiv->pos_list);
}

void
uivar_GetValue(var, val)
uivar_t var;
uivarvalue_t *val;
{
    unsigned int i;
    
    val->var = var;
    val->val = get_var_value(var->v_opt);
    val->subvals = 0;
    if (!val->val)
	return;
    if (ison(val->var->v_flags, V_SINGLEVAL|V_MULTIVAL)) {
	for (i = 0; i != val->var->v_num_vals; i++) {
	    if (!chk_two_lists(val->val, val->var->v_values[i].v_label, " ,"))
		continue;
	    turnon(val->subvals, 1<<i);
	    if (ison(val->var->v_flags, V_SINGLEVAL))
		break;
	}
    }
}

zmBool
uivarvalue_GetStr(val, p)
uivarvalue_t *val;
char **p;
{
    *p = val->val;
    return ison(val->var->v_flags, V_STRING);
}

zmBool
uivarvalue_GetLong(val, l)
uivarvalue_t *val;
long *l;
{
    *l = val->val ? atol(val->val) : 0;
    return ison(val->var->v_flags, V_NUMERIC);
}

zmBool
uivarvalue_Copy(dest, val)
uivarvalue_t *dest, *val;
{
    bcopy((VPTR) val, (VPTR) dest, sizeof *dest);
    dest->val = savestr(val->val);
    return True;
}

zmBool
uivar_SetValue(v, val)
uivar_t v;
uivarvalue_t *val;
{
    return set_var(v->v_opt, "=", val->val) == 0;
}

void
uivarvalue_Destroy(val)
uivarvalue_t *val;
{
    xfree(val->val);
}

zmBool
uivar_SetLong(var, l)
uivar_t var;
long l;
{
    char buf[40];
    sprintf(buf, "%ld", l);
    return set_var(var->v_opt, "=", buf) == 0;
}

zmBool
uivar_SetStr(var, s)
uivar_t var;
const char *s;
{
    return set_var(var->v_opt, "=", s) == 0;
}

zmBool
uivar_SetBool(var, b)
uivar_t var;
zmBool b;
{
    const char *argv[2];

    if (!b) {
	user_unset(&set_options, var->v_opt);
	return True;
    }
    argv[0] = var->v_opt;
    argv[1] = NULL;
    return user_set(&set_options, argv) == 0;
}

zmBool
uivar_SetSubVal(var, n, b)
uivar_t var;
int n;
zmBool b;
{
    return set_var(var->v_opt, b ? "+=" : "-=", var->v_values[n].v_label);
}

char *
uivar_GetLongDescription(var)
uivar_t var;
{
    return variable_stuff(uivar_GetVarIndex(var), NULL);
}
