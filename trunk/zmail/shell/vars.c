/* variables.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "linklist.h"
#include "vars.h"
#include "catalog.h"
#ifdef VUI
#include "dialog.h" /* to circumvent "unreasonable include nesting" */
#endif /* VUI */
#include "dirserv.h"
#include "strcase.h"
#include <general.h>

Variable *variables;
int n_variables;

static void process_var_help();

void
free_vars(vars, nvars)
Variable vars[];
int nvars;
{
    while (nvars-- > 0) {
	xfree(vars[nvars].v_opt);
	xfree(vars[nvars].v_default);
	while (vars[nvars].v_num_vals-- > 0) {
	    xfree(vars[nvars].v_values[vars[nvars].v_num_vals].v_label);
#ifdef OLD_BEHAVIOR
	    xfree(vars[nvars].v_values[vars[nvars].v_num_vals].v_description);
#endif /* OLD_BEHAVIOR */
	    /* need to free GuiItems somehow */
	}
	xfree((char*)(vars[nvars].v_values));
	xfree(vars[nvars].v_prompt.v_label);
#ifdef OLD_BEHAVIOR
	xfree(vars[nvars].v_prompt.v_description);
#endif /* OLD_BEHAVIOR */
	/* need to free GuiItems somehow */
    }
    free((char *)vars);
}

#ifdef MAC_OS
# include "zminit.seg"
#endif /* MAC_OS */
static int
load_flags(fp, file, line_no)
FILE *fp;
char *file;
int *line_no;
{
    register char *p, *p2;
    char 	  line[MAXPATHLEN];

    if (!(p = next_line(line, sizeof line, fp, line_no))) {
	error(ZmErrFatal,
	    catgets( catalog, CAT_SHELL, 764, "%s: line %d, expected flag specs" ), file, *line_no);
	return -1;
    }
    /* tty, fullscreen, gui, readonly, boolean, string, multivalue */
    variables[n_variables].v_flags = 0L;
    variables[n_variables].v_gui_max = 100;
    variables[n_variables].v_category = 0;
    while ((p2 = any(p, " \t,")) || p && *p) {
	if (p2)
	    *p2++ = 0;
	else
	    p2 = p+strlen(p);
	if (!ci_strcmp(p, "tty"))
	    turnon(variables[n_variables].v_flags, V_TTY);
	else if (!ci_strcmp(p, "fullscreen"))
	    turnon(variables[n_variables].v_flags, V_CURSES);
	else if (!ci_strcmp(p, "gui"))
	    turnon(variables[n_variables].v_flags, V_GUI);
	else if (!ci_strcmp(p, "lite"))
	    turnon(variables[n_variables].v_flags, V_VUI);
	else if (!ci_strcmp(p, "mac"))
	    turnon(variables[n_variables].v_flags, V_MAC);
	else if (!ci_strcmp(p, "mswindows"))
	    turnon(variables[n_variables].v_flags, V_MSWINDOWS);
	else if (!ci_strcmp(p, "readonly"))
	    turnon(variables[n_variables].v_flags, V_READONLY);
	else if (!ci_strcmp(p, "boolean"))
	    turnon(variables[n_variables].v_flags, V_BOOLEAN);
	else if (!ci_strcmp(p, "string"))
	    turnon(variables[n_variables].v_flags, V_STRING);
	else if (!ci_strcmp(p, "multivalue"))
	    turnon(variables[n_variables].v_flags, V_MULTIVAL);
	else if (!ci_strcmp(p, "singlevalue"))
	    turnon(variables[n_variables].v_flags, V_SINGLEVAL);
	else if (!ci_strcmp(p, "numeric"))
	    turnon(variables[n_variables].v_flags, V_NUMERIC);
	else if (!ci_strcmp(p, "permanent"))
	    turnon(variables[n_variables].v_flags, V_PERMANENT);
	else if (!ci_strcmp(p, "admin"))
	    turnon(variables[n_variables].v_flags, V_ADMIN);
	else if (!ci_strncmp(p, "gui_max=", 8))
	    variables[n_variables].v_gui_max = atoi(p+8);
	else if (!ci_strncmp(p, "category=", 9))
	    variables[n_variables].v_category = atoi(p+9);
	else
	    error(ZmErrFatal,
		catgets( catalog, CAT_SHELL, 765, "%s (line %d): %s is not a legal variable flag" ),
		file, *line_no, p);
	for (p = p2; *p && (*p == ',' || isspace(*p)); p++)
	    ;
    }
    return 0;
}

static char *
parse_ternary(s)
char *s;
{
    char *v, *t, *f = NULL;

    /* To permit '?' to occur in the default strings, require that the
     * '?' of the ternary precedes any spaces or colons in the string.
     */
    if ((t = any(s, "? \t:")) && *t == '?') {
	*t++ = 0;
	if (f = index(t, ':'))
	    *f++ = 0;
    }
    if (v = check_internal(s))
	if (*v != '0')
	    return t? t : "";
	else
	    return f;
    else
	return s;
}

/* Reduce a default value containing conditionals into the canonical form
 * expected by load_default().  Conditionals may be either a simple keyword
 * such as "is_gui" or "redirect", which is tested for truth; or a ternary
 * expression "keyword?true-value:false-value".  The ":false-value" may be
 * omitted to specify "" (empty).  Multivalues may have a comma-separated
 * list of conditionals.
 */
static char *
parse_default(val, flags)
char *val;
u_long flags;
{
    int c;
    char *p = val, *v = val, *next, line[MAXPATHLEN], *l = line;

    /* Copy a leading "*", then parse the rest */
    if (*p == '*') {
	skipspaces(1);
	c = *p, *p = 0;
	l += Strcpy(l, v);
	*p = c, v = p;
    }
    /* Booleans must have a truth-value as the first word on the line,
     * even if they are also string or multivalued.
     */
    if (ison(flags, V_BOOLEAN) && (p = any(v, " \t"))) {
	c = *p, *p = 0;
	skipspaces(1);
	next = p;
    } else
	next = NULL, c = 0;
    if (ci_strncmp(v, "none", 4) != 0) {
	p = parse_ternary(v);
	if ((!p || !*p) && ison(flags, V_BOOLEAN))
	    l += Strcpy(l, p? "true" : "false");
	else if (ison(flags, V_BOOLEAN) && (v = check_internal(p)))
	    l += Strcpy(l, *v != '0'? "true" : "false");
	else
	    l += Strcpy(l, p);
    } else
	l += Strcpy(l, v);
    if (c)
	*l++ = c;
    while (v = next) {
	if (ison(flags, V_MULTIVAL) && (p = any(v, " \t,"))) {
	    c = *p, *p = 0;
	    skipspaces(1);
	    next = p;
	} else
	    next = NULL, c = 0;
	p = parse_ternary(v);
	if (p && *p) {
	    l += Strcpy(l, p);
	    if (c)
		*l++ = c;
	}
    }
    *l = 0;
    return strcpy(val, line);
}

static int
load_default(fp, file, line_no)
FILE *fp;
char *file;
int *line_no;
{
    register char *p;
    char 	  line[MAXPATHLEN];

    variables[n_variables].v_default = NULL;
    if (isoff(variables[n_variables].v_flags, V_READONLY)) {
	const char *argv[4];

	if (!next_line(line, sizeof line, fp, line_no)) {
	    error(ZmErrFatal, 
		catgets( catalog, CAT_SHELL, 766, "%s: line %d, expected default value" ), file, *line_no);
	    return -1;
	} else
	    p = parse_default(line, variables[n_variables].v_flags);
	if (ci_strncmp(p, "none", 4) != 0) {
	    int default_boolean = TRUE;
	    char *env_val = NULL;

	    if (*p == '*') {
		skipspaces(1);
		env_val = value_of(variables[n_variables].v_opt);
		default_boolean = !env_val;
	    }
	    if (ison(variables[n_variables].v_flags, V_BOOLEAN)) {
		if (!ci_strncmp(p, "true", 4))
		    p += 4;
		else if (!ci_strncmp(p, "false", 5)) {
		    p += 5;
		    default_boolean = FALSE;
		} else {
		    error(ZmErrFatal, 
			"%s: line %d, bad boolean: %s", file, *line_no, p);
		    return -1;
		}
		skipspaces(0);
	    }
	    if (ison(variables[n_variables].v_flags,
			V_NUMERIC|V_STRING|V_SINGLEVAL|V_MULTIVAL) ||
		    (all_p(variables[n_variables].v_flags,
			    V_PERMANENT|V_BOOLEAN) && default_boolean)) {
		if (*p && *p != '*')
		    variables[n_variables].v_default = savestr(p);
		else
		    variables[n_variables].v_default =
			savestr(value_of(variables[n_variables].v_opt));
	    }
	    argv[0] = variables[n_variables].v_opt;
	    if (default_boolean) {
		if (variables[n_variables].v_default) {
		    argv[1] = "=";
		    argv[2] = variables[n_variables].v_default;
		    argv[3] = NULL;
		} else
		    argv[1] = NULL;
		(void) add_option(&set_options, argv);
	    } else if (env_val && !optlist_fetch(&set_options, argv[0])) {
		/* Force an import from the environment, to call callbacks */
		if (*env_val) {
		    argv[1] = "=";
		    argv[2] = env_val;
		    argv[3] = NULL;
		} else
		    argv[1] = NULL;
		(void) add_option(&set_options, argv);
	    }
	}
    }
    return 0;
}

static char *
load_description(fp, file, line_no, vi)
FILE *fp;
char *file;
int *line_no;
VarItem *vi;
{
    char     *p, **p2 = DUBL_NULL;
    int	      count = 0;
    unsigned int size = 0;
    char      line[MAXPATHLEN], **unitv();
#ifdef OLD_BEHAVIOR
    static char *buf;
#else /* !OLD_BEHAVIOR */
    char *buf = NULL;
    int store = !fp;

    if (store) {
	if (vi->v_description)
	    return vi->v_description;
	if (!(fp = fopen(variables_file, "r")) ||
		fseek(fp, vi->v_desc_pos, 0) < 0) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 767, "Cannot find variable description" ));
	    if (fp)
		(void) fclose(fp);
	    return NULL;
	}
    } else {
	vi->v_description = NULL;
	vi->v_desc_pos = ftell(fp);
    }
#endif /* OLD_BEHAVIOR */

    while (p = next_line(line, sizeof line, fp, line_no)) {
	if (!strcmp(p, "%%"))
	    break;
#ifndef OLD_BEHAVIOR
	if (!store)
	    continue;
#endif /* OLD_BEHAVIOR */
	size += strlen(p) + 1;
	if ((count = catv(count, &p2, 1, unitv(p))) < 0)
	    break;
    }
    if (!p2)
	return NULL;
#ifdef OLD_BEHAVIOR
    if (buf)
	buf = realloc(buf, size + 1);
    else
#endif /* OLD_BEHAVIOR */
	buf = (char *) malloc(size + 1);
    if (buf) {
	int use_newline;
	p = buf;
	for (count = 0; p2[count]; count++, *p++ = use_newline? '\n' : ' ') {
	    p += Strcpy(p, p2[count]);
	    use_newline =
		(!istool || !p2[count][0] || isspace(p2[count][0]) ||
		p2[count+1] && (!p2[count+1][0] || isspace(p2[count+1][0])));
	}
	*p = 0;
	free_vec(p2);
    } else
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 768, "Cannot load variable description" ));

#ifndef OLD_BEHAVIOR
    if (store) {
	vi->v_description = buf;
	buf = NULL;
	fclose(fp);
	return vi->v_description;
    }
#endif /* OLD_BEHAVIOR */

    return buf;
}

static int
load_varitem(fp, file, line_no, vi)
FILE *fp;
char *file;
int *line_no;
VarItem *vi;
{
    char *p, line[MAXPATHLEN];

    if (!(p = next_line(line, sizeof line, fp, line_no))) {
	error(ZmErrFatal, 
	    catgets( catalog, CAT_SHELL, 769, "%s: line %d, expected label or NULL" ), file, *line_no);
	return -1;
    }
    if (!strcmp(p, "NULL"))
	vi->v_label = NULL;
    else
	vi->v_label = savestr(p);
    if (!load_description(fp, file, line_no, vi))
	return 0;
    return -1;
}

static int
load_multivalues(fp, file, line_no)
FILE *fp;
char *file;
int *line_no;
{
    register char *p;
    register unsigned i;
    char 	  line[MAXPATHLEN];

    if (isoff(variables[n_variables].v_flags, V_SINGLEVAL|V_MULTIVAL)) {
	variables[n_variables].v_num_vals = 0;
	return 0;
    }

    if (!(p = next_line(line, sizeof line, fp, line_no)) ||
	    !(variables[n_variables].v_num_vals = atoi(p))) {
	error(ZmErrFatal, 
	    catgets( catalog, CAT_SHELL, 770, "%s: line %d, expected number of values > 0" ), file, *line_no);
	return -1;
    }
    variables[n_variables].v_values =
	(VarItem *)calloc(variables[n_variables].v_num_vals, sizeof(VarItem));
    if (!variables[n_variables].v_values) {
	error(SysErrFatal, 
	    catgets( catalog, CAT_SHELL, 771, "Out of memory for values of variable %d" ), n_variables);
	return -1;
    }
    for (i = 0; i < variables[n_variables].v_num_vals; i++)
	load_varitem(fp, file, line_no, &variables[n_variables].v_values[i]);
    return 0;
}

#define load_prompt(f,n,l) \
	load_varitem(f, n, l, &variables[n_variables].v_prompt)

int
load_variables()
{
    register char *p;
    FILE 	  *fp;
    char 	  file[MAXPATHLEN], line[MAXPATHLEN], *found_version = NULL;
    int		   n, line_no = 0, wrong_version = TRUE;
    Variable	  *tmp;

    add_var_callbacks();
    if (variables_file) {
	n = 1;
	p = getpath(variables_file, &n);
	if (n == -1) {
	    if (isoff(glob_flags, REDIRECT))
		error(SysErrFatal, "%s: %s", variables_file, p);
	    return -1;
	} else if (n) {
	    if (isoff(glob_flags, REDIRECT))
		error(ZmErrFatal, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), variables_file);
	    return -2;
	}
	if (!(fp = fopen(p, "r"))) {
	    if (isoff(glob_flags, REDIRECT))
		error(SysErrFatal, catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), p);
	    return -1;
	}
#if defined(MAC_OS) && defined(USE_SETVBUF)
	if (fp)
	    (void) setvbuf(fp, NULL, _IOFBF, BUFSIZ * 8);
#endif /* MAC_OS && USE_SETVBUF */
#ifdef OLD_BEHAVIOR
	xfree(variables_file), variables_file = NULL;
#endif /* OLD_BEHAVIOR */
    } else
	return -3;
    (void) strcpy(file, p);

    /* read new vars from variables file till EOF. */
    while (p = next_line(line, sizeof line, fp, &line_no)) {
	if (wrong_version && ci_strncmp(line, "version", 7) == 0) {
	    char *vers = zmVersion(1);
	    int mx, mn;

	    p += 8;
	    skipspaces(0);
	    /*
	     * If the file has a patchlevel, check it, else check
	     * only the major and minor version; but in no case check
	     * beyond the length of the full version and patchlevel.
	     */
	    mn = min(strlen(vers), strlen(p));
	    mx = max(mn, strlen(zmVersion(0)));
	    wrong_version = strncmp(vers, p, mx);
	    /* Bart: Mon Jan 25 12:43:10 PST 1993
	     * Make the error message a bit more informative.
	     */
	    if (wrong_version) {
		if (found_version)
		    (void) strapp(&found_version, ", ");
		(void) strapp(&found_version, p);
	    }
	    continue;
	}
	if (strcmp(p, "--"))
	    continue;
	if (!variables)
	    tmp = (Variable *)malloc(sizeof(Variable));
	else
	    tmp = (Variable *)realloc(variables,
		sizeof(Variable)*(n_variables+1));
	if (!tmp) {
	    error(SysErrFatal,
		catgets( catalog, CAT_SHELL, 775, "out of memory after %d variables" ), n_variables);
	    line_no = -1;
	    break;
	}
	variables = tmp;
	if (!(p = next_line(line, sizeof line, fp, &line_no))) {
	    error(ZmErrFatal,
		catgets( catalog, CAT_SHELL, 776, "%s: line %d, expected variable name" ), file, line_no);
	    break;
	}
	variables[n_variables].v_opt = savestr(p);
	if (load_flags(fp, file, &line_no) < 0 ||
	    load_default(fp, file, &line_no) < 0 ||
	    load_multivalues(fp, file, &line_no) < 0 ||
	    load_prompt(fp, file, &line_no) < 0)
	    break;
	n_variables++;
    }
    for (n = 0, tmp = 0; n < n_variables; n++)
	insert_link(&tmp, &variables[n]);
    if (wrong_version && found_version)
	error(ZmErrWarning,
	    catgets( catalog, CAT_SHELL, 777, "Variables file does not match version:\n File: %s\n Z-Mail: %s" ),
	    found_version, zmVersion(1));
    else if (wrong_version)
	error(ZmErrWarning,
	    catgets( catalog, CAT_SHELL, 778, "Variables file does not contain version number." ));
    (void) fclose(fp);
    (void) xfree(found_version);
    return (wrong_version || line_no < 0) ? -1 : 0;
}

#ifdef MAC_OS
# include "sh4seg.seg"
#endif /* MAC_OS */
char *
next_line(line, size, fp, line_no)
char line[];
register int size;
register FILE *fp;
int *line_no;
{
    register char *p, *p2;
    register int cont_line = 0;

    *line = 0;
    while (p = fgets(&line[cont_line], size - cont_line, fp)) {
	(*line_no)++;
	if (*(p2 = no_newln(p)) == '\\') {
	    *p2++ = ' ';
	    cont_line = p2 - line;
	} else if (*p != '#')	/* not quite right: consider "\\\n#..." */
	    break;
    }
    return p || *line? line : (char *) NULL;
}

/*
 * return a string describing a variable.
 * parameters: count, str, buf.
 * If str != NULL, check str against ALL variables
 * in variables array.  The one that matches, set count to it and 
 * print up all the stuff from the variables[count] into the buffer
 * space in "buf" and return it.
 */
char *
variable_stuff(count, str)
register int count;
register const char *str;
{
    unsigned i;
    static char *buf;
#ifndef OLD_BEHAVIOR
    int no = 0;
#endif /* !OLD_BEHAVIOR */
    
    if (!buf && !(buf = (char *) malloc(BUFSIZ*2)))
	error(SysErrFatal, catgets( catalog, CAT_SHELL, 779, "Cannot continue" ));

    if (str)
	for (count = 0; count < n_variables; count++)
	    if (!strcmp(str, variables[count].v_opt))
		break;

    if (count >= n_variables)
	if (str) {
	    sprintf(buf, catgets( catalog, CAT_SHELL, 780, "%s: Not a default %s variable.\n" ),
		    str? str : itoa(count), prog_name);
	    return (buf);
	} else {
	    return NULL;
	}

#ifdef OLD_BEHAVIOR
    (void) sprintf(buf, catgets( catalog, CAT_SHELL, 781, "%s%s:\n%s" ), istool? "" : "\n",
	variables[count].v_opt, variables[count].v_prompt.v_description);
#else /* !OLD_BEHAVIOR */
    (void) sprintf(buf, catgets( catalog, CAT_SHELL, 782, "%s%s:\n" ), istool? "" : "\n", variables[count].v_opt);
    strapp(&buf, load_description(NULL_FILE, variables[count].v_opt, &no,
					    &variables[count].v_prompt));
#endif /* OLD_BEHAVIOR */
    if (ison(variables[count].v_flags, V_SINGLEVAL|V_MULTIVAL)) {
	str = &buf[strlen(buf)];
#ifdef OLD_BEHAVIOR
	sprintf(str, catgets( catalog, CAT_SHELL, 783, "\n%s may be set to one or more of the following:\n" ),
	    variables[count].v_opt);
	str += strlen(str);
#else /* !OLD_BEHAVIOR */
	strapp(&buf, "\n");
	if (ison(variables[count].v_flags, V_SINGLEVAL))
	    strapp(&buf, zmVaStr(catgets(catalog, CAT_SHELL, 896, "%s may be set to one of the following:\n\n"), variables[count].v_opt));
	else
	    strapp(&buf, zmVaStr(catgets(catalog, CAT_SHELL, 897, "%s may be set to one or more of the following:\n\n"), variables[count].v_opt));
#endif /* OLD_BEHAVIOR */
	for (i = 0; i < variables[count].v_num_vals; i++) {
#ifdef OLD_BEHAVIOR
	    (void) sprintf(str, "  %s: %s\n",
		variables[count].v_values[i].v_label,
		variables[count].v_values[i].v_description);
	    str += strlen(str);
#else /* !OLD_BEHAVIOR */
	    strapp(&buf, "  ");
	    strapp(&buf, variables[count].v_values[i].v_label);
	    strapp(&buf, ":\n    ");
	    strapp(&buf, load_description(NULL_FILE, variables[count].v_opt,
					&no, &variables[count].v_values[i]));
	    strapp(&buf, "\n");
#endif /* OLD_BEHAVIOR */
	}
    }
    process_var_help(buf);
    return buf;
}

#define VARS_HIGHLIGHT_SIZE 50
int *vars_highlight, vars_highlight_ct, *vars_highlight_locs;
void process_help_xref();

/*
 * process the "~#...#~" cross-reference text.
 * We make a list of the positions of all these areas, so we can highlight
 * them later, and so the user can double-click on them.
 */
static void
process_var_help(str)
char *str;
{
    char *in = str, *out = str, *xref = str;
    char esc1 = '~', esc2 = '#', swap;
    int total = 0, loc;

    vars_highlight_ct = 0;
    if (!vars_highlight) {
	vars_highlight = (int *) calloc(VARS_HIGHLIGHT_SIZE+1,
				sizeof *vars_highlight);
	vars_highlight_locs = (int *) calloc(VARS_HIGHLIGHT_SIZE/2,
				     sizeof *vars_highlight_locs);
    }
    for (;;) {
	for (; *in && !(in[0] == esc1 && in[1] == esc2);
	     *out++ = *in++, total++);
	if (!*in) { *out = 0; break; }
	loc = 0;
	if (esc1 == '#') {
	    int len;
	    *out = 0;
	    process_help_xref(xref, &loc, &len);
	    out -= len; total -= len;
	    vars_highlight_locs[vars_highlight_ct/2] = loc;
	}
	in += 2;
	xref = out;
	swap = esc1; esc1 = esc2; esc2 = swap;
	vars_highlight[vars_highlight_ct++] = total;
	/* don't add a hypertext item if there is an overflow,
	 * or if an invalid location was given
	 */
	if (vars_highlight_ct == VARS_HIGHLIGHT_SIZE || loc == -1)
	    vars_highlight_ct -= 2;
    }
    if (vars_highlight_ct & 1)
	vars_highlight[vars_highlight_ct++] = total;
    vars_highlight[vars_highlight_ct] = total;
}

static char *locs[4] = {
    "var", "ui", "cmd", NULL
};

void
process_help_xref(str, loc, len)
char *str;
int *loc, *len;
{
    int i;
    
    *loc = HHLOC_NONE;
    if (len) *len = 0;
    while (*str && *str != '~') str++;
    if (!*str) return;
    str++;
    for (i = 0; locs[i]; i++)
	if (!strcmp(str, locs[i])) {
	    *loc = i+HHLOC_VARS;
	    if (len) *len = strlen(str)+1;
	    str[-1] = 0;
#ifndef FUNCTIONS_HELP
	    if (*loc == HHLOC_CMDS)
		*loc = -1;
#endif /* FUNCTIONS_HELP */
	    return;
	}
}

#ifdef NOT_NOW

/* Variable management */

vset_numeric()
{
}

vset_multival()
{
}

vset_string()
{
}

vset_boolean()
{
}

#endif /* NOT_NOW */

/* 
 * See definitions of VT_Rec and VIX in vars.h
 */

/* Table of variable names and attributes. */

/* This table provides a mapping from symbolic names defined in vars.h
 * to the strings that name the corresponding Z-Script variables.  The
 * ideal usage is to avoid having to change the symbolic names when we
 * rename variables in Z-Script, but so far we haven't been using it
 * that way.  Also, attributes of most variables are augmented by the
 * $ZMLIB/variables file, so this table mostly provides attributes for
 * any variables that may exist but not be listed in that file.
 *
 * When a variable's name is changed, add a new new entry to this table
 * for the new variable and point the vt_synonym field of the OLD variable
 * at the new entry.  If the old variable is to be supported for backwards
 * compatibility, also edit add_option() in setopts.c and add a test for
 * the old variable to the list of VAR_REMAP()s there.
 *
 * Remember that we currently have to update the #define list of symbolic
 * names, the VT_map enum, and the table below when ever entries are added
 * or deleted.
 */

VT_Rec var_table[] = {
    { { { NULL }, 0, }, VT_Unknown, VIX(VT_Unknown)   },	/* Filler */
#ifdef LOOKUP_STUFF
    { { { "address_book"    }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_LookupService,  VIX(VT_AddressBook)    },
#else /* !LOOKUP_STUFF */
    { { { "address_book"    }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_AddressBook)    },
#endif /* !LOOKUP_STUFF */
    { { { "address_cache"   }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
        VT_Unknown,        VIX(VT_AddressCache)   },
    { { { "address_check"   }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
        VT_Unknown,        VIX(VT_AddressCheck)   },
    { { { "address_filter"  }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
        VT_Unknown,        VIX(VT_AddressFilter)  },
    { { { "address_sort"    }, V_TTY|V_CURSES|V_GUI|V_STRING|V_BOOLEAN,    },
	VT_Unknown,        VIX(VT_AddressSort)    },
#ifdef ZPOP_SYNC
    { { { "afterlife"       }, V_TTY|V_CURSES|V_GUI|V_NUMERIC,             },
	VT_Unknown,        VIX(VT_Afterlife)      },
#endif /* ZPOP_SYNC */
    { { { "alternates"      }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Alternates)     },
    { { { "alwaysexpand"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Alwaysexpand)   },
    { { { "alwaysignore"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
	VT_Unknown,        VIX(VT_Alwaysignore)   },
    { { { "always_send_spooled"}, V_MSWINDOWS|V_BOOLEAN,                   },
	VT_Unknown,        VIX(VT_AlwaysSendSpooled)},
    { { { "always_show_taskm"}, V_MSWINDOWS|V_BOOLEAN,                     },
	VT_Unknown,        VIX(VT_AlwaysShowTaskm)},
    { { { "always_sort"     }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_AlwaysSort)     },
    { { { "ask"             }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Ask)            },
    { { { "askbcc"          }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Askbcc)         },
    { { { "askcc"           }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Askcc)          },
    { { { "asksub"          }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Ask,            VIX(VT_Asksub)         },
    { { { "attach_label"    }, V_GUI|V_SINGLEVAL,                          },
	VT_Unknown,        VIX(VT_AttachLabel)    },
    { { { "attach_prune_size" }, V_GUI|V_NUMERIC,			   },
	VT_Unknown,        VIX(VT_AttachPrune)    },
    { { { "attach_types"    }, V_TTY|V_CURSES|V_GUI|V_PATHLIST,            },
	VT_Unknown,        VIX(VT_AttachTypes)    },
    { { { "auto_route"      }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING,    },
	VT_Unknown,        VIX(VT_AutoRoute)      },
    { { { "autoclear"       }, V_GUI|V_BOOLEAN,                            },
	VT_Unknown,        VIX(VT_Autoclear)      },
    { { { "autodismiss"     }, V_GUI|V_MULTIVAL,                           },
	VT_Unknown,        VIX(VT_Autodismiss)    },
    { { { "autodisplay"     }, V_TTY|V_CURSES|V_GUI|V_WORDLIST,            },
	VT_Unknown,        VIX(VT_Autodisplay)    },
    { { { "autoedit"        }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
	VT_Unknown,        VIX(VT_Autoedit)       },
    { { { "autoformat"      }, V_GUI|V_MULTIVAL,                           },
	VT_Unknown,        VIX(VT_Autoformat)     },
    { { { "autoiconify"     }, V_GUI|V_MULTIVAL,                           },
	VT_Unknown,        VIX(VT_Autoiconify)    },
    { { { "autoinclude"     }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Autoinclude)    },
    { { { "autopinup" 	    }, V_GUI|V_MULTIVAL,                           },
	VT_Unknown,        VIX(VT_Autopinup)      },
    { { { "autoprint"       }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Autoprint)      },
    { { { "autosave_count"  }, V_GUI|V_NUMERIC				   },
	VT_Unknown,        VIX(VT_AutosaveCount)  },
    { { { "autosend"     }, V_GUI|V_BOOLEAN    ,                           },
	VT_Unknown,        VIX(VT_Autosend)       },
    { { { "autosign"        }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING,    },
	VT_Unknown,        VIX(VT_Autosign)       },
    { { { "autosign2"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Autosign2)      },
    { { { "autotyper"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Autotype)       },
    { { { "baud"            }, V_GUI|V_NUMERIC                             },
	VT_Unknown,        VIX(VT_Baud)           },
    { { { "blink_time"      }, V_GUI|V_NUMERIC,                            },
	VT_Unknown,        VIX(VT_BlinkTime)      },
    { { { "cdpath"          }, V_TTY|V_CURSES|V_GUI|V_PATHLIST,            },
	VT_Unknown,        VIX(VT_Cdpath)         },
    { { { "child"           }, V_TTY|V_CURSES|V_GUI|V_NUMERIC,             },
	VT_Unknown,	   VIX(VT_Child)          },
    { { { "cmd_help"        }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_CmdHelp)        },
    { { { "colors_db"       }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_ColorsDB)       },
    { { { "comp_attach_label"}, V_TTY|V_GUI|V_SINGLEVAL,	 	   },
	VT_Unknown,        VIX(VT_CompAttachLabel)},
    { { { "comp_status_bar_fmt"}, V_GUI|V_STRING,                          },
	VT_Unknown,        VIX(VT_CompStatusBarFmt)},
    { { { "complete"        }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING,    },
	VT_Unknown,        VIX(VT_Complete)       },
    { { { "compose_lines"   }, V_GUI|V_NUMERIC|V_PERMANENT,                },
	VT_Unknown,        VIX(VT_ComposeLines)   },
    { { { "compose_panes"   }, V_GUI|V_MULTIVAL,			   },
	VT_Unknown,        VIX(VT_ComposePanes)   },
    { { { "connected"       }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Connection)     },
    { { { "connect_type"    }, V_TTY|V_CURSES|V_GUI|V_SINGLEVAL,           },
	VT_Unknown,        VIX(VT_ConnectType)    },
    { { { "crt"             }, V_TTY|V_CURSES|V_GUI|V_NUMERIC|V_PERMANENT, },
	VT_Unknown,        VIX(VT_Crt)            },
    { { { "crt_win"         }, V_GUI|V_NUMERIC|V_PERMANENT,                },
	VT_MessageLines,   VIX(VT_CrtWin)         },
    { { { "cwd"             }, V_TTY|V_CURSES|V_GUI|V_READONLY|V_STRING,   },
	VT_Unknown,        VIX(VT_Cwd)            },
    { { { "date_received"   }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_DateReceived)   },
    { { { "dead"            }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Dead)           },
    { { { "deletesave"      }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Deletesave)     },
    { { { "detach_dir"      }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_DetachDir)      },
    { { { "display_charset" }, V_TTY|V_GUI|V_STRING,                       },
	VT_Unknown,        VIX(VT_DisplayCharset) },
    { { { "display_headers" }, V_GUI|V_SINGLEVAL,                          },
	VT_Unknown,        VIX(VT_DisplayHeaders) },
    { { { "domain_route"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING,    },
	VT_Unknown,        VIX(VT_DomainRoute)    },
    { { { "dot"             }, V_TTY|V_CURSES|V_BOOLEAN,                   },
	VT_Unknown,        VIX(VT_Dot)            },
    { { { "dot_lock"        }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_DotLock)        },
    { { { "edit_hdrs"       }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_EditHdrs)       },
    { { { "editor"          }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Editor)         },
    { { { "escape"          }, V_TTY|V_CURSES|V_STRING,                    },
	VT_Unknown,        VIX(VT_Escape)         },
    { { { "exit_saveopts"   }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING     },
	VT_Unknown,        VIX(VT_ExitSaveopts)         },
    { { { "expire_cache"    }, V_TTY|V_CURSES|V_STRING,                    },
	VT_Unknown,        VIX(VT_ExpireCache)    },
    { { { "ext_summary_fmt" }, V_MSWINDOWS|V_STRING|V_PERMANENT,           },
	VT_Unknown,        VIX(VT_ExtSummaryFmt)  },
    { { { "fetch_timeout"   }, V_TTY|V_CURSES|V_GUI|V_NUMERIC              },
	VT_Unknown,	   VIX(VT_FetchTimeout)   },
    { { { "fignore"         }, V_TTY|V_CURSES|V_GUI|V_WORDLIST,            },
	VT_Unknown,        VIX(VT_Fignore)        },
    { { { "filec"           }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Complete,       VIX(VT_Filec)          },
    { { { "file_charset"    }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_FileCharset)    },
    { { { "filelist_fmt"    }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_FilelistFmt)    },
    { { { "file_test"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_FileTest)       },
    { { { "first_part_primary",}, V_TTY|V_GUI|V_BOOLEAN,                   },
        VT_Unknown,        VIX(VT_FirstPartPrimary) },
    { { { "fkeylabels",     }, V_GUI|V_BOOLEAN,                            },
        VT_Unknown,        VIX(VT_FkeyLabels)     },
    { { { "folder"          }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_Folder)         },
    { { { "folder_title"    }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_FolderTitle)    },
    { { { "folder_type"     }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_FolderType)     },
    { { { "fonts_db"        }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_FontsDB)        },
    { { { "fortunates"      }, V_TTY|V_CURSES|V_GUI|V_WORDLIST,            },
	VT_Unknown,        VIX(VT_Fortunates)     },
    { { { "fortune"         }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Fortune)        },
    { { { "from_address"    }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_FromAddress)    },
    { { { "fullscreen_help" }, V_TTY|V_CURSES|V_BOOLEAN|V_WORDLIST,        },
	VT_Unknown,        VIX(VT_FullscreenHelp) },
    { { { "function_help"   }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_FunctionHelp)   },
    { { { "gui_help"        }, V_GUI|V_STRING|V_PERMANENT,                 },
	VT_Unknown,        VIX(VT_GuiHelp)        },
    { { { "hangup"          }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Hangup)         },
    { { { "header_encoding" }, V_TTY|V_CURSES|V_GUI|V_SINGLEVAL,           },
        VT_Unknown,        VIX(VT_HeaderEncoding) },
    { { { "hdr_format"      }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_SummaryFmt,     VIX(VT_HdrFormat)      },
    { { { "hidden"          }, V_GUI|V_MULTIVAL,                           },
	VT_Unknown,        VIX(VT_Hidden)         },
    { { { "history"         }, V_TTY|V_CURSES|V_NUMERIC,                   },
	VT_Unknown,        VIX(VT_History)        },
    { { { "hold"            }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Hold)           },
    { { { "home"            }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_Home)           },
    { { { "hostname"        }, V_TTY|V_CURSES|V_GUI|V_WORDLIST|V_PERMANENT,},
	VT_Unknown,        VIX(VT_Hostname)       },
    { { { "ignore_bang"     }, V_TTY|V_CURSES|V_BOOLEAN,                   },
	VT_Unknown,        VIX(VT_IgnoreBang)     },
    { { { "ignoreeof"       }, V_TTY|V_CURSES|V_BOOLEAN|V_STRING,          },
	VT_Unknown,        VIX(VT_Ignoreeof)      },
#if defined( IMAP )
    { { { "imap_cache"  }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
        VT_Unknown,        VIX(VT_ImapCache) },
    { { { "imap_shared"  }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             }
,
        VT_Unknown,        VIX(VT_ImapShared) },
    { { { "imap_synchronize"  }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             }
,
        VT_Unknown,        VIX(VT_ImapSynchronize) },
    { { { "imap_timeout"     }, V_TTY|V_CURSES|V_GUI|V_NUMERIC              }
,
        VT_Unknown,        VIX(VT_ImapTimeout)     },
    { { { "imap_user"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              }
,
        VT_Unknown,        VIX(VT_ImapUser)        },
    { { { "imap_noprompt"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,              }
,
        VT_Unknown,        VIX(VT_ImapNoPrompt)        },
#endif
    { { { "in_reply_to"     }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING,    },
	VT_Unknown,        VIX(VT_InReplyTo)      },
    { { { "incoming_charset"}, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_IncomingCharset)},
    { { { "indent_str"      }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_IndentStr)      },
    { { { "index_dir"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_IndexDir)       },
    { { { "index_size"      }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_NUMERIC,   },
	VT_Unknown,        VIX(VT_IndexSize)      },
    { { { "intr_level"      }, V_TTY|V_CURSES|V_GUI|V_NUMERIC,             },
	VT_Unknown,        VIX(VT_IntrLevel)      },
    { { { "ispeller"        }, V_TTY|V_CURSES|V_GUI|V_STRING|V_VUI,        },
	VT_Unknown,        VIX(VT_Ispeller)       },
    { { { "keepsave"        }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Keepsave)       },
    { { { "known_hosts"     }, V_TTY|V_CURSES|V_GUI|V_WORDLIST,            },
	VT_Unknown,        VIX(VT_KnownHosts)     },
    { { { "layout_db"       }, V_GUI|V_STRING,				   },
	VT_Unknown,        VIX(VT_LayoutDB)       },
    { { { "ldap_service"    }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_LdapService)    },
    { { { "logfile"         }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Logfile)        },
    { { { "lookup_charset"  }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_LookupCharset)  },
    { { { "lookup_file"     }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_LookupFile)     },
    { { { "lookup_host_ip"  }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_LookupHostIP)   },
    { { { "lookup_max"      }, V_TTY|V_CURSES|V_GUI|V_NUMERIC,             },
	VT_Unknown,        VIX(VT_LookupMax)      },
    { { { "lookup_mode"     }, V_TTY|V_CURSES|V_GUI|V_MULTIVAL,            },
	VT_Unknown,        VIX(VT_LookupMode)     },
    { { { "lookup_sep"      }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_LookupSep)      },
    { { { "lookup_service"  }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_LookupService)  },
    { { { "mailhost"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Mailhost)       },
    { { { "mail_icon"       }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_MailIcon)       },
    { { { "mail_queue"      }, V_GUI|V_CURSES|V_TTY|V_STRING,              },
	VT_Unknown,        VIX(VT_MailQueue)      },
    { { { "mailbox_name"    }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_MailboxName)    },
    { { { "main_folder_title" },
			       V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_MainFolderTitle)},
    { { { "main_panes"      }, V_GUI|V_MULTIVAL,			   },
	VT_Unknown,        VIX(VT_MainPanes)      },
    { { { "main_status_bar_fmt"}, V_GUI|V_STRING,                          },
	VT_Unknown,        VIX(VT_MainStatusBarFmt)  },
    { { { "max_text_length" }, V_GUI|V_NUMERIC|V_PERMANENT,	           },
	VT_Unknown,        VIX(VT_MaxTextLength)    },
    { { { "mbox"            }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_Mbox)           },
    { { { "message_field"   }, V_GUI|V_MULTIVAL,			   },
	VT_Unknown,        VIX(VT_MessageField)   },
    { { { "message_lines"   }, V_GUI|V_NUMERIC|V_PERMANENT,                },
	VT_Unknown,        VIX(VT_MessageLines)   },
    { { { "message_panes"   }, V_GUI|V_MULTIVAL,			   },
	VT_Unknown,        VIX(VT_MessagePanes)   },
    { { { "metoo"           }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Metoo)          },
    { { { "mil_time"        }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_MilTime)        },
    { { { "msg_attach_label" },
			       V_TTY|V_GUI|V_SINGLEVAL,	    	 	   },
	VT_Unknown,        VIX(VT_MsgAttachLabel) },
    { { { "msg_separator"   },V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,   },
	VT_Unknown,        VIX(VT_MsgSeparator)   },
    { { { "msg_status_bar_fmt"}, V_GUI|V_STRING,                           },
	VT_Unknown,        VIX(VT_MsgStatusBarFmt)  },
    { { { "msg_win"         }, V_GUI|V_NUMERIC|V_PERMANENT,                },
	VT_ComposeLines,   VIX(VT_MsgWin)         },
    { { { "msg_win_hdr_fmt" }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_MsgWinHdrFmt)   },
    { { { "name"            }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Realname,       VIX(VT_Name)           },
    { { { "newline"         }, V_TTY|V_STRING,                             },
	VT_Unknown,        VIX(VT_Newline)        },
    { { { "newmail_icon"    }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_NewmailIcon)    },
    { { { "newmail_scroll"  }, V_GUI|V_BOOLEAN,				               },
	VT_Unknown,	   VIX(VT_NewmailScroll)  },
    { { { "no_expand"       }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_NoExpand)       },
    { { { "no_hdrs"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_NoHdrs)         },
    { { { "no_reverse"      }, V_TTY|V_CURSES|V_VUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_NoReverse)      },
    { { { "nonobang"        }, V_TTY|V_CURSES|V_BOOLEAN,                   },
	VT_Unknown,        VIX(VT_Nonobang)       },
    { { { "nosave"          }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Nosave)         },
    { { { "organization"    }, V_MSWINDOWS|V_READONLY,                     },
	VT_Unknown,        VIX(VT_Organization)},
    { { { "outgoing_charset"}, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_OutgoingCharset)},
    { { { "output"          }, V_TTY|V_CURSES|V_GUI|V_READONLY|V_STRING,   },
	VT_Unknown,        VIX(VT_Output)         },
    { { { "pager"           }, V_TTY|V_CURSES|V_STRING,                    },
	VT_Unknown,        VIX(VT_Pager)          },
    { { { "picky_mta"       }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
	VT_Unknown,        VIX(VT_PickyMta)       },
    { { { "pop_options"     }, V_TTY|V_CURSES|V_GUI|V_MULTIVAL             },
	VT_Unknown,	   VIX(VT_PopOptions)     },
    { { { "pop_timeout"     }, V_TTY|V_CURSES|V_GUI|V_NUMERIC              },
	VT_Unknown,	   VIX(VT_PopTimeout)     },
    { { { "pop_user"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_PopUser)        },
    { { { "post_indent_str" }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_PostIndentStr)  },
    { { { "pre_indent_str"  }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_PreIndentStr)   },
    { { { "presign"         }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Presign)        },
    { { { "print_cmd"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_PrintCmd)       },
    { { { "printer"         }, V_TTY|V_CURSES|V_GUI|V_WORDLIST,            },
	VT_Unknown,        VIX(VT_Printer)        },
    { { { "printer_charset" }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_PrinterCharset)     },
    { { { "printer_opt"     }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_PrinterOpt)     },
    { { { "prompt"          }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_Prompt)         },
    { { { "quiet"           }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
	VT_Unknown,        VIX(VT_Quiet)          },
    { { { "realname"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Realname)       },
    { { { "record"          }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Record)         },
    { { { "record_control"  }, V_TTY|V_CURSES|V_GUI|V_MULTIVAL,            },
	VT_Unknown,        VIX(VT_RecordControl)  },
    { { { "record_max"      }, V_TTY|V_CURSES|V_GUI|V_NUMERIC,             },
	VT_Unknown,        VIX(VT_RecordMax)      },
    { { { "record_users"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_RecordUsers)    },
    { { { "recursive"       }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Recursive)      },
    { { { "reply_to_hdr"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_WORDLIST,  },
	VT_Unknown,        VIX(VT_ReplyToHdr)     },
    { { { "save_empty"      }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_SaveEmpty)      },
    { { { "screen"          }, V_TTY|V_CURSES|V_GUI|V_NUMERIC|V_PERMANENT, },
	VT_Unknown,        VIX(VT_Screen)         },
    { { { "screen_win"      }, V_GUI|V_NUMERIC,                            },
	VT_SummaryLines,   VIX(VT_ScreenWin)      },
    { { { "scrollpct"       }, V_VUI|V_BOOLEAN,                            },
	VT_Unknown,        VIX(VT_Scrollpct)      },
    { { { "sendmail"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Sendmail)       },
    { { { "shell"           }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Shell)          },
    { { { "show_deleted"    }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_ShowDeleted)    },
    { { { "smtphost"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Smtphost)       },
    { { { "sort"            }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_STRING,    },
	VT_Unknown,        VIX(VT_Sort)           },
    { { { "speller"         }, V_GUI|V_STRING,			           },
	VT_Unknown,	   VIX(VT_Speller)        },
#ifdef PARTIAL_SEND
    { { { "split_limit"     }, V_NUMERIC,				   },
	VT_Unknown,	   VIX(VT_SplitLimit)	  },
    { { { "split_sendmail"  }, V_STRING,				   },
	VT_Unknown,	   VIX(VT_SplitSendmail)  },
    { { { "split_size"      }, V_NUMERIC,				   },
	VT_Unknown,	   VIX(VT_SplitSize)	  },
#endif /* PARTIAL_SEND */
    { { { "squeeze"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Squeeze)        },
    { { { "status"          }, V_TTY|V_CURSES|V_GUI|V_READONLY|V_NUMERIC,  },
	VT_Unknown,        VIX(VT_Status)         },
    { { { "status_bar_fmt"}, V_GUI|V_STRING,                               },
	VT_Unknown,        VIX(VT_StatusBarFmt)   },
    { { { "summary_fmt"     }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_SummaryFmt)     },
    { { { "summary_lines"   }, V_GUI|V_NUMERIC,                            },
	VT_Unknown,        VIX(VT_SummaryLines)   },
    { { { "templates"       }, V_TTY|V_CURSES|V_GUI|V_PATHLIST,            },
	VT_Unknown,        VIX(VT_Templates)      },
    { { { "textpart_charset" }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT, },
	VT_OutgoingCharset, VIX(VT_TextpartCharset) },
    { { { "thisfolder"      }, V_TTY|V_CURSES|V_GUI|V_READONLY|V_STRING,   },
	VT_Unknown,        VIX(VT_Thisfolder)     },
    { { { "timeout"         }, V_GUI|V_NUMERIC,                            },
	VT_Unknown,        VIX(VT_Timeout)        },
    { { { "title"           }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_FolderTitle,    VIX(VT_Title)          },
    { { { "tmpdir"          }, V_TTY|V_CURSES|V_GUI|V_STRING|V_PERMANENT,  },
	VT_Unknown,        VIX(VT_Tmpdir)         },
#ifdef ZPOP_SYNC
    { { { "tombfile"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Tombfile)       },
#endif /* ZPOP_SYNC */
    { { { "tool_help"       }, V_GUI|V_STRING,                             },
	VT_GuiHelp,        VIX(VT_ToolHelp)       },
    { { { "toplines"        }, V_TTY|V_CURSES|V_GUI|V_NUMERIC|V_PERMANENT, },
	VT_Unknown,        VIX(VT_Toplines)       },
    { { { "trusted_functions" }, V_GUI|V_WORDLIST, },
	VT_Unknown,        VIX(VT_TrustedFunctions) },
    { { { "unix"            }, V_TTY|V_CURSES|V_BOOLEAN,                   },
	VT_Unknown,        VIX(VT_Unix)           },
    { { { "use_content_length" }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,          },
	VT_Unknown,        VIX(VT_UseContentLength) },
#if defined( IMAP )
    { { { "use_imap"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
        VT_Unknown,        VIX(VT_UseImap)         },
#endif
    { { { "use_ldap"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_UseLdap)         },
    { { { "use_pop"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_UsePop)         },
    { { { "user"            }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_User)           },
    { { { "uucp_root"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_UucpRoot)       },
    { { { "verbose"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Verbose)        },
    { { { "verify"          }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_MULTIVAL,  },
	VT_Unknown,        VIX(VT_Verify)         },
    { { { "visual"          }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Editor,         VIX(VT_Visual)         },
    { { { "warning"         }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Warning)        },
    { { { "window_shell"    }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_WindowShell)    },
    { { { "wineditor"       }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_Wineditor)      },
    { { { "winterm"         }, V_GUI|V_STRING,                             },
	VT_Unknown,        VIX(VT_Winterm)        },
    { { { "wrap"            }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN,             },
	VT_Unknown,        VIX(VT_Wrap)           },
    { { { "wrapcolumn"      }, V_TTY|V_CURSES|V_GUI|V_BOOLEAN|V_NUMERIC,   },
	VT_Unknown,        VIX(VT_Wrapcolumn)     },
    { { { "zynchost"        }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_Zynchost)       },
    { { { "zync_options"    }, V_TTY|V_CURSES|V_GUI|V_MULTIVAL,            },
	VT_Unknown,        VIX(VT_ZyncOptions)    },
    { { { "zync_user"       }, V_TTY|V_CURSES|V_GUI|V_STRING,              },
	VT_Unknown,        VIX(VT_ZyncUser)       },
};
