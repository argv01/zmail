/* setopts.c     Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char     setopts_rcsid[] = "$Id: setopts.c,v 2.148 2005/05/31 07:36:42 syd Exp $";
#endif

#include "zmail.h"
#include "zmcomp.h"
#include "buttons.h"
#include "c_bind.h"
#include "catalog.h"
#include "dirserv.h"
#include "config/features.h"
#include "fetch.h"
#include "fsfix.h"
#include "hashtab.h"
#include "hooks.h"
#include "init.h"
#include "linklist.h"
#include "pager.h"
#include "setopts.h"
#include "strcase.h"
#include "vars.h"

#ifdef GUI
#include "zmframe.h"
#endif /* GUI */

static int need_to_save();
static void clean_slate();
char **link_to_argv();
static void user_set_var P ((char *, char *, char *));
static void save_priorities P ((FILE *, int));
static void save_list P ((char *, struct options **, char *, int, FILE *));
static void save_cmd P ((char *, struct cmd_map *, char *, int, FILE *));

typedef struct options *aliasMapping;	/* For now */

static struct thread_hashtab {
    /* fixedorder here should be replaced by (a) an ordering element
     * in the structure pointed to by the hash table, and (b) a pointer
     * to a comparison function that will be used by tht_thread().
     */
    char initialized, threaded, fixedorder;
    struct hashtab ht;
}
    variable_ht,
    alias_ht,
    ignore_ht,
    retain_ht,
    cmd_ht,
    filter_ht,
    /* fkey_ht, */	/* Not used */
    header_ht;

struct thread_hashtab *
optlist2tht(list)
struct options **list;
{
    if (list == &set_options)
	return &variable_ht;
    if (list == &aliases)
	return &alias_ht;
    if (list == &ignore_hdr)
	return &ignore_ht;
    if (list == &show_hdr)
	return &retain_ht;
    if (list == &cmdsubs)
	return &cmd_ht;
    if (list == &filters)
	return &filter_ht;
    if (list == &own_hdrs)
	return &header_ht;
    return NULL;
}

static unsigned int
aliasHash(elt)
aliasMapping *elt;
{
    return (hashtab_StringHash((*elt)->option));
}

static int
aliasCmp(elt1, elt2)
aliasMapping *elt1, *elt2;
{
    return (strcmp((*elt1)->option, (*elt2)->option));
}

static aliasMapping
tht_find(tht, str)
struct thread_hashtab *tht;
const char *str;
{
    struct options probe;
    aliasMapping elt = &probe, *found;

    if (!tht->initialized)
	return 0;

    /* XXX casting away const */
    probe.option = (char *) str;
    found = (aliasMapping *)hashtab_Find(&(tht->ht), &elt);
    if (found)
	return *found;
    else
	return 0;
}

struct options *
optlist_fetch(list, str)
struct options **list;
const char *str;
{
    return tht_find(optlist2tht(list), str);
}

void
tht_thread(tht, list)
struct thread_hashtab *tht;
struct options **list;	/* Double deref to manipulate global list */
{
    struct hashtab_iterator i;
    aliasMapping *elt, opt;

    if (tht->threaded || !tht->initialized || hashtab_EmptyP(&(tht->ht)))
	return;

    *list = 0;
    hashtab_InitIterator(&i);
    while (elt = hashtab_Iterate(&(tht->ht), &i)) {
	opt = *elt;
	elt = list;
	while (*elt && strcmp((*elt)->option, opt->option) < 1)
	    elt = &((*elt)->next);
	opt->next = *elt;
	if (*elt) {
	    opt->prev = (*elt)->prev;
	    (*elt)->prev = opt;
	} else if (*list) {
	    opt->prev = (*list)->prev;
	    (*list)->prev = opt;
	} else
	    opt->prev = opt;
	*elt = opt;
    }
    tht->threaded = 1;
}

void
optlist_sort(list)
struct options **list;
{
    tht_thread(optlist2tht(list), list);
}

static void
insert_option(list, opt, order)
struct options **list, *opt;
int order;    /* Insert in sorted order? */
{
    struct thread_hashtab *tht = optlist2tht(list);

    if (!tht->initialized) {
	tht->fixedorder = !order;
	hashtab_Init(&tht->ht,
		     (unsigned int (*) P((CVPTR))) aliasHash,
		     (int (*) P((CVPTR, CVPTR))) aliasCmp,
		     sizeof(aliasMapping), 127);
	tht->initialized = 1;
    }
    hashtab_Add(&tht->ht, &opt);
    if (tht->fixedorder) {
	tht->threaded = 1;
	if (*list) {
	    if ((*list)->prev) {
		(*list)->prev->next = opt;
		opt->prev = (*list)->prev;
	    } else {
		(*list)->next = opt;
		opt->prev = *list;
	    }
	    (*list)->prev = opt;
	    opt->next = 0;	/* Circular only in prev direction */
	} else {
	    *list = opt->prev = opt;
	    opt->next = 0;
	}
    } else {
	tht->threaded = 0;
	opt->next = opt->prev = 0;
	*list = opt;	/* The thread-root is never empty */
    }
}

/* Reset the global state based on the new value of a variable */
/*
 * this function is going away!  Don't add new cases to this function,
 * or to un_set; add callbacks instead.  You should add them in
 * add_var_callbacks if they are core variables; GUI-related callbacks
 * can be added in GUI code, when the item they relate to is created.
 */
int
reset_state(tmp, list)
struct options *tmp, **list;
{
    if (list == &set_options) {
	/* check for options which must have values or are used frequently */
#ifdef CURSES
	if (!strcmp(tmp->option, VarNoReverse))
	    turnoff(glob_flags, REV_VIDEO);
	else
#endif /* CURSES */
#ifdef GUI
	if (!strcmp(tmp->option, VarGuiHelp))
	    if (tmp->value && *(tmp->value))
		ZSTRDUP(tool_help, tmp->value);
	    else {
		int n = 0; /* don't ignore no such file or directory */
		char *p = getenv("GUI_HELP"); /* Might be in environ */
		p = getpath((p && *p)? p : def_tool_help, &n);
		if (n)
		    ZSTRDUP(tool_help, def_tool_help);
		else
		    ZSTRDUP(tool_help, p);
		ZSTRDUP(tmp->value, tool_help);
	    }
	else
#endif /* GUI */
	if (!strcmp(tmp->option, VarCmdHelp))
	    if (tmp->value && *(tmp->value))
		ZSTRDUP(cmd_help, tmp->value);
	    else {
		int n = 0 /* don't ignore no such file or directory */;
		char *p = getenv("CMD_HELP");
		p = getpath((p && *p)? p : def_cmd_help, &n);
		if (n)
		    ZSTRDUP(cmd_help, def_cmd_help);
		else
		    ZSTRDUP(cmd_help, p);
		ZSTRDUP(tmp->value, cmd_help);
	    }
	else if (!strcmp(tmp->option, VarCrt)) {
	    if (!istool)
		crt = (tmp->value)? max(atoi(tmp->value), 2): 18;
	}
	else if (!strcmp(tmp->option, VarScreen)) {
	    screen = (tmp->value)? max(atoi(tmp->value), 1): 18;
#ifdef CURSES
	    if (iscurses && screen > LINES-2)
		screen = LINES-2;
#endif /* CURSES */
	} else if (!strcmp(tmp->option, VarWrapcolumn)) {
	    char wval[16];
	    wrapcolumn =
		(tmp->value && *(tmp->value))? max(atoi(tmp->value), 0): 78;
#ifdef CURSES
	    /* Use COLS-2 because of silly terminals like vt100 */
	    if (iscurses && wrapcolumn > COLS - 2)
		wrapcolumn = COLS - 2;
#endif /* CURSES */
	    xfree(tmp->value);
	    sprintf(wval, "%d", wrapcolumn);
	    tmp->value = savestr(wval);
	}
#ifndef GUI_ONLY
	else if (!strcmp(tmp->option, VarHistory))
	    init_history((tmp->value && *(tmp->value))? atoi(tmp->value) : 1);
#endif /* GUI_ONLY */
	else if (!strcmp(tmp->option, "show_hdrs") && tmp->value) {
	    (void) cmd_line(
		    zmVaStr("builtin unretain * ; builtin retain %s",
			    tmp->value),
		    msg_list);
	} else if (!strcmp(tmp->option, VarKnownHosts)) {
	    free_vec(known_hosts);
	    known_hosts = strvec(tmp->value, ", ", TRUE);
	} else if (!strcmp(tmp->option, VarHostname)) {
	    free_vec(ourname);
	    ourname = strvec(tmp->value, ", ", TRUE);
	} else if (!strcmp(tmp->option, VarFolderType)) {
	    if (!ci_strcmp(tmp->value, "standard"))
		def_fldr_type = FolderStandard;
	    else if (!ci_strcmp(tmp->value, "delimited"))
		def_fldr_type = FolderDelimited;
	    else { /* XXX */
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 624, "Unknown folder type: %s" ), tmp->value);
		if (def_fldr_type == FolderStandard)
		    ZSTRDUP(tmp->value, "standard");
		else
		    ZSTRDUP(tmp->value, "delimited");
	    }
	} else if (!strcmp(tmp->option, VarComplete)) {
	    if (tmp->value && *(tmp->value)) {
		char value[8];
		value[0] = 0;
		strncat(value, tmp->value, 7);  /* nul-terminates */
		m_xlate(value); /* use the original, don't change tmp->value */
		complete = value[0];
		complist = value[1];
	    } else {
		tmp->value = savestr("\\E\\CD");
		complete = '\033';
		complist = '\004';
	    }
	} else if (!strcmp(tmp->option, VarAlternates)) {
	    char **altv = unitv("");	/* Special value, see alts() */
	    int altc = vcat(&altv, strvec(tmp->value, " \t,", TRUE));

	    if (altc > 1) {
		xfree(tmp->value);
		tmp->value = joinv(NULL, altv, " ");
		(void) alts(altc, altv);
	    }
	    free_vec(altv);
	}
    }
    return 0;
}

/* Modify the value of a variable in place.  WARNING: Must not be called
 * with how != 0 before load_variables() in vars.c has completed its work!
 */
static int
alter_value(list, name, value, how)
struct options **list;
const char *name, *value;
int how;	/* 1 == insert, -1 == delete */
{
    struct options *opt;
    Variable *var = (how && list == &set_options)?
	(Variable *)retrieve_link(variables, name, strcmp) : (Variable *)0;
    int multival = var? ison(var->v_flags, V_MULTIVAL) : FALSE;
    int singleval = var? ison(var->v_flags, V_SINGLEVAL) : FALSE;
    char **allvals;
    int nvals;

    if (singleval && how != 0) {
	if (how > 0) how = 0;
	multival = 1;
    }
    if (how < 0 && !multival && !singleval) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 627, "%s: not a multivalued variable" ), name);
	return -1;
    }

    if (how == 0) {
	if (!(opt = optlist_fetch(list, name))) {
	    if (!(opt = (struct options *)
		    calloc((unsigned)1,sizeof(struct options)))) {
		error(SysErrWarning, "calloc");
		return -1;
	    }
	    opt->option = savestr(name);
	    opt->value = savestr(value);
	    insert_option(list, opt, (list != &own_hdrs));
	} else {
	    xfree(opt->value);
	    opt->value = savestr(value);
	}
    } else {
#ifdef OLD_BEHAVIOR
	for (opt = *list; opt; opt = opt->next)
	    if (strcmp(name, opt->option) == 0)
		break;
	if (!opt) /* Should decr of an unset var be an error? */
	    return how > 0? alter_value(list, name, value, 0) : 0;
#endif /* OLD_BEHAVIOR */
	struct thread_hashtab *tht = optlist2tht(list);
	opt = tht_find(tht, name);
	if (!opt)
	    return how > 0? alter_value(list, name, value, 0) : 0;
	/*
	 * Search for opt before testing for value in case this
	 * is += of a variable that is not currently set.
	 */
	if (!value || !*value)
	    return 0;	/* Nothing to add/delete */
	if (multival) {
	    char **eachval, **nextval;

	    if ((allvals = strvec(opt->value, ", \t", TRUE)) == 0 && how < 0)
		return -1;

	    if (allvals)
		for (nvals = 0, eachval = allvals; *eachval; nvals++, eachval++)
		     if (ci_strcmp(value, *eachval) == 0)
			break;

	    if (how > 0) {
		if (!allvals || !*eachval)
		    nvals = vcat(&allvals, unitv(value));
		else {
		    free_vec(allvals);
		    return 0;	/* Nothing changed */
		}
	    } else {
		if (*(nextval = eachval)) {
		    xfree(*eachval);
		    while (*eachval++ = *++nextval)
			nvals++;
		} else {
		    free_vec(allvals);
		    return 0;	/* Nothing changed */
		}
	    }
	    /* "nvals" now holds number of values or -1 on error */
	    if (nvals > 0) {
		xfree(opt->value);
		opt->value = joinv(NULL, allvals, ",");
		free_vec(allvals);
	    } else {
		free_vec(allvals);
		if (nvals == 0)		/* We decremented to nothing */
		    (void) user_unset(list, (char *) name);
		return nvals;
	    }
	} else /* appending to a string value */
	    opt->value = strapp(&opt->value, value);
    }

    if (reset_state(opt, list) < 0) {
	(void) un_set(list, name);
	return -1;
    }
    if (list == &set_options)
	ZmCallbackCallAll(name, ZCBTYPE_VAR, ZCB_VAR_SET, opt->value);

    return 0;
}

/* add an option indicated by "set option[=value]" or by "alias name alias"
 * function is recursive, so multilists get appended accordingly
 */
int
add_option(list, argv)
struct options **list;
const char * const *argv;
{
    char *option;
    const char *value = NULL;
    int incr = FALSE, decr = FALSE;
    static char *optbuf = NULL;
    static int optbuf_len = 0;

    if (!argv[0])
	return 1;
    /* check for one of five forms:
     * opt=val / opt= val / opt = val / opt += val / opt -= val
     *
     * Note that += and -= must be surrounded by whitespace.  They currently
     * mean STRING APPEND or LIST DELETE, _not_ NUMERIC ARITHMETIC.
     *
     * Why not  option =value  ?  (Too hard to parse right now.)
     */
    if (value = index(*argv, '=')) {
	int len;
	if (value == *argv) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 628, "%s: No variable specified." ), value);
	    return 0;
	}
	/* "option=value" strip into option="option" value="value" */
	len = value-*argv;
	if (!optbuf || optbuf_len < len+1) {
	    xfree(optbuf);
	    optbuf_len = (len+1)*2;
	    optbuf = malloc(optbuf_len);
	}
	option = optbuf;
	strncpy(option, *argv, len);
	option[len] = '\0';

	if (*++value || (value = *++argv)) { /* "option= value" */
	    ++argv;
	}
    } else {
	/* XXX casting away const */
	option = (char *) *argv;
	if (*++argv &&
	    (!strcmp(*argv, "=") ||
	     (incr = !strcmp(*argv, "+=")) ||
	     (decr = !strcmp(*argv, "-=")))) {
	    if (value = *++argv) /* "option = value" */
		++argv;
	}
    }

    /* Backwards and sideways compatibility stuff */
    if (list == &set_options) {
	if (!strcmp(option, VAR_MAP(VT_ToolHelp)) ||
		!strcmp(option, "motif_help") ||
		!strcmp(option, "olit_help") ||
		!strcmp(option, "lite_help"))
	    option = VAR_REMAP(VT_ToolHelp);
#ifdef LOOKUP_STUFF
	else if (!strcmp(option, VAR_MAP(VT_AddressBook)))
	    option = VAR_REMAP(VT_AddressBook);
#endif /* LOOKUP_STUFF */
	else if (!strcmp(option, VAR_MAP(VT_Filec)))
	    option = VAR_REMAP(VT_Filec);
	else if (!strcmp(option, VAR_MAP(VT_HdrFormat)))
	    option = VAR_REMAP(VT_HdrFormat);
	else if (!strcmp(option, VAR_MAP(VT_Title)))
	    option = VAR_REMAP(VT_Title);
	else if (!strcmp(option, VAR_MAP(VT_CrtWin)))
	    option = VAR_REMAP(VT_CrtWin);
	else if (!strcmp(option, VAR_MAP(VT_MsgWin)))
	    option = VAR_REMAP(VT_MsgWin);
	else if (!strcmp(option, VAR_MAP(VT_ScreenWin)))
	    option = VAR_REMAP(VT_ScreenWin);
    }

    /* check for internal vars that can't be set this way */
    if (list == &set_options) {
	if (!strcmp(option, "compose_state")) {
	    set_compose_state(value, incr-decr);
	    return 0;
	} else if (check_internal(option)) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 629, "You cannot change %s with \"set\"." ), option);
	    return 0;
	}
    }

#ifdef NOT_NOW
    /* Bart: Thu Mar 25 13:44:25 PST 1993
     * This shouldn't be necessary any more.  The edit_hdrs return of -1
     * has been changed, and alter_value now always changes the value in
     * place if it already exists, doing a hash lookup to find the old
     * value.  The only possible tricky part is the button lists ....
     */
    if (!incr && !decr) {
	/* check to see if option is already set by attempting to unset it.
	 * This probably isn't necessary any longer -- the only possible -1
	 * return is if edit_hdrs changes during a compose -- but there is
	 * some global state clean-up done here that reset_state() may need.
	 * Also, alter_value() currently assumes that if it is neither incr
	 * nor decr, then it should insert a new variable into the list.
	 */
	if (un_set(list, option) == -1)
	    return 0;
    }
#endif /* NOT_NOW */
    if (alter_value(list, option, value, incr? 1 : 0 - decr) < 0)
	return -1;

    if (*argv)
	return add_option(list, argv);

    return incr;
}

int
set_var(var, eq, val)
const char *var, *eq, *val;
{
    const char *tmpv[4];

    if (!var)
	return -1;
    if (!val)
	eq = NULL;

    tmpv[0] = var;
    tmpv[1] = eq;	/* One of:	"="	"+="	"-="	NULL	*/
    tmpv[2] = val;
    tmpv[3] = 0;

    return add_option(&set_options, tmpv);
}

int
set_int_var(var, eq, val)
char *var, *eq;
long val;
{
    static char buf[10];

    sprintf(buf, "%ld", val);
    return set_var(var, eq, buf);
}

int
user_settable(list, varname, varptr)
struct options **list;
const char *varname;
struct Variable **varptr;
{
    Variable *var;
    char *eq;
    
    if (list != &set_options) return TRUE;
    eq = index(varname, '=');
    if (eq) *eq = 0;
    var = (Variable *) retrieve_link(variables, varname, strcmp);
    if (eq) *eq = '=';
    if (!var) return TRUE;
    if (varptr) *varptr = var; /* optimization */
    if (isoff(var->v_flags, V_READONLY) &&
        (isoff(var->v_flags, V_ADMIN) || ison(glob_flags, ADMIN_MODE)))
	return TRUE;
    if (boolean_val(VarWarning)) {
	if (eq) *eq = 0;
	error(UserErrWarning, catgets(catalog, CAT_SHELL, 858, "%s is a readonly variable"), varname);
	if (eq) *eq = '=';
    }
    return FALSE;
}

int
user_set(optlist, argv)
struct options **optlist;
const char * const *argv;
{
    if (user_settable(optlist, argv[0], (Variable **)0))
	return add_option(optlist, argv);
    else
	return -1;
}

static void
user_set_var(var, eq, val)
char *var, *eq, *val;
{
    if (user_settable(&set_options, var, (Variable **)0))
	set_var(var, eq, val);
}

int
user_unset(list, varname)
struct options **list;
char *varname;
{
    Variable *var = (Variable *) 0;
    
    if (!user_settable(list, varname, &var)) return 0;
    if (var && ison(var->v_flags, V_PERMANENT) && var->v_default) {
	set_var(varname, "=", var->v_default);
	return 1;
    }
    return un_set(list, varname);
}

int
set_env(var, val)
char *var, *val;
{
    char *tmpv[4];

    if (!var)
	return -1;

    tmpv[0] = "Setenv";
    tmpv[1] = var;
    tmpv[2] = val? val : "";
    tmpv[3] = 0;

    return Setenv(3, tmpv);
}

int
unset_env(var)
char *var;
{
    char *tmpv[3];

    if (!var)
	return -1;

    tmpv[0] = "Unsetenv";
    tmpv[1] = var;
    tmpv[2] = 0;

    return Unsetenv(2, tmpv);
}

/*
 * If str is NULL, just print options and their values. Note that numerical
 * values are not converted to int upon return.  If str is not NULL
 * return the string that matched, else return NULL;
 */
char *
zm_set(list, str)
register struct options **list;
register const char *str;
{
    struct options *opts;
    struct thread_hashtab *tht = optlist2tht(list);
    ZmPager pager;

    if (!str) {
	if (!*list)
	    return NULL;
	tht_thread(tht, list);
	pager = ZmPagerStart(PgText);
	for (opts = *list; opts; opts = opts->next) {
	    ZmPagerWrite(pager, opts->option);
	    if (opts->value && *opts->value) {
		(void) ZmPagerWrite(pager, "     \t");
		(void) ZmPagerWrite(pager, opts->value);
	    }
	    ZmPagerWrite(pager, "\n");
	    if (ZmPagerIsDone(pager)) break;
	}
	ZmPagerStop(pager);
	return NULL;
    } else {
	opts = tht_find(tht, str);
	if (opts) {
	    if (opts->value)
		return opts->value;
	    else
		return "";
	}
    }

    /* If we still haven't matched, check for environment variables.
     * Never check for temporaries (leading "__") in the environment.
     */
    if (str && list == &set_options &&
		(str[0] != '_' || str[1] == '\0' || str[1] != '_')) {
	register int N, n;
	for (N = 0; environ[N]; N++) {
	    char *p = index(environ[N], '=');
	    if (p)
		*p = 0;
	    n = ci_strcmp(str, environ[N]);
	    if (p)
		*p = '=';
	    if (!n)
		return p+1;
	}
    }
    return NULL;
}

static int
remove_option(list, p)
struct options **list;
const char *p;
{
    struct options *opts, **tmp;
    struct thread_hashtab *tht = optlist2tht(list);
    struct hashtab_iterator i;
    char sigsignd = ison(glob_flags, IGN_SIGS);	/* YUCK! */

    if (!list || !*list || !p || !*p)
	return 0;

    opts = tht_find(tht, p);
    if (opts) {
	turnon(glob_flags, IGN_SIGS);
	hashtab_Remove(&(tht->ht), NULL);
	/*
	 * API backwards compatibility stuff -- *list has to point to
	 * something valid as long as hashtab_EmptyP() is not true.
	 * See notes above about fixedorder.
	 */
	if (tht->threaded) {
	    if (*list == opts)
		*list = opts->next;
	    else
		opts->prev->next = opts->next;
	    if (opts->next)
		opts->next->prev = opts->prev;
	    else if (*list)
		(*list)->prev = opts->prev;
	} else {
	    hashtab_InitIterator(&i);
	    tmp = hashtab_Iterate(&(tht->ht), &i);
	    if (tmp)
		*list = *tmp;	/* We don't care where list points */
	     else
		*list = 0;
	}
	xfree (opts->option);
	if (opts->value)
	    xfree(opts->value);
	xfree((char *)opts);
	if (!sigsignd)
	    turnoff(glob_flags, IGN_SIGS);
	return 1;
    }
    return 0;
}

/*
 * unset the variable described by p in the list "list".
 * if the variable isn't set, then return 0, else return 1.
 */
int
un_set(list, p)
    struct options **list;
    const char *p;
{
    int ret;
    if (!list || !*list || !p || !*p)
	return 0;
    if (list == &set_options) {
#ifdef CURSES
	if (!strcmp(p, VarNoReverse))
	    turnon(glob_flags, REV_VIDEO);
	else
#endif /* CURSES */
	if (!strcmp(p, VarCrt))
	    crt = 18;
	else if (!strcmp(p, VarScreen)) {
	    screen = 18;
#ifdef CURSES
	    if (iscurses && screen > LINES-2)
		screen = LINES-2;
#endif /* CURSES */
	} else
#ifdef GUI
	if (!strcmp(p, VarGuiHelp)) {
	    int n = 0;
	    char *p2 = getenv("GUI_HELP");
	    p2 = getpath((p2 && *p2)? p2 : def_tool_help, &n);
	    if (n)
		ZSTRDUP(tool_help, TOOL_HELP);
	    else
		ZSTRDUP(tool_help, p2);
	} else
#endif /* GUI */
	if (!strcmp(p, VarCmdHelp)) {
	    int n = 0; /* don't ignore no such file or directory */
	    char *p2 = getenv("CMD_HELP");
	    p2 = getpath((p2 && *p2)? p2 : def_cmd_help, &n);
	    if (n)
		ZSTRDUP(cmd_help, COMMAND_HELP);
	    else
		ZSTRDUP(cmd_help, p2);
	} else if (!strcmp(p, VarWrapcolumn))
	    wrapcolumn = 0;
#ifndef GUI_ONLY
	else if (!strcmp(p, VarHistory))
	    init_history(1);
#endif /* !GUI_ONLY */
	else if (!strcmp(p, "show_hdrs"))
	    (void) cmd_line(zmVaStr("builtin unretain *"), msg_list);
	else if (!strcmp(p, VarKnownHosts)) {
	    free_vec(known_hosts);
	    known_hosts = DUBL_NULL;
	} else if (!strcmp(p, VarHostname)) {
	    free_vec(ourname);
	    ourname = DUBL_NULL;
	} else if (ison(glob_flags, IS_GETTING) && !strcmp(p, VarEditHdrs)) {
	    wprint(catgets( catalog, CAT_SHELL, 631, "(current composition not affected by change in %s.)\n" ),
		VarEditHdrs);
	} else if (!strcmp(p, VarFolderType)) {
	    def_fldr_type = FolderStandard;	/* XXX */
	} else if (!strcmp(p, VarComplete)) {
	    complete = complist = 0;
	} else if (!strcmp(p, VarAlternates)) {
	    free_vec(alternates);
	    alternates = DUBL_NULL;
	}
#ifdef _WINDOWS
	else if (!strcmp(p, VarConnected) && boolean_val(VarConnected)) {
	    if (chk_option(VarVerify, "disconnect") &&
		  ask(AskOk, catgets(catalog, CAT_SHELL, 913, "Are you sure you want to disconnect from the network?")) == AskCancel)
		return 0;
	}
#endif /* _WINDOWS */
    }

    ret = remove_option(list, p);
    if (list == &set_options)
	ZmCallbackCallAll(p, ZCBTYPE_VAR, ZCB_VAR_UNSET, NULL);
    return ret;
}

#if defined(GUI) && !defined(VUI)
extern void show_var();
#endif /* GUI && !VUI */

/* The functions below return 0 since they don't affect
 * messages.
 */
int
set(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    char firstchar = **argv;
    register const char *cmd = *argv;
    register char *str;
    register struct options **optlist;
    int success;

    if (*cmd == 'u')
	cmd += 2;

    if (*++argv && !strcmp(cmd, "set")) {
	if (**argv == '?') {
	    int incurses = iscurses;
	    if (incurses)
		clr_bot_line(), iscurses = FALSE;
	    if (!strcmp(*argv, "?all")) {
#ifdef GUI
		if (istool) {
		    gui_dialog("variables");
		    return -1;
		}
#endif /* GUI */
		ZmPagerStart(PgHelp);
		for (argc = 0; str = variable_stuff(argc, NULL); argc++)
		    ZmPagerWrite(cur_pager, str);
		ZmPagerStop(cur_pager);
	    } else {
		ZmPagerStart(PgHelp);
		ZmPagerWrite(cur_pager, variable_stuff(0, (*argv)+1));
		ZmPagerStop(cur_pager);
	    }
	    iscurses = incurses;
	    return 0 - in_pipe();
	} else if (strcmp(*argv, "-") == 0 || strcmp(*argv, "--") == 0) {
	    ZSTRDUP(*argv, zmfunc_args? zmfunc_args->o_name : prog_name);
	    pop_args();
	    push_args(--argc, vdup(argv), list);
	    return 0;
	}
    }

    optlist = !strcmp(cmd, "ignore")? &ignore_hdr :
	      !strcmp(cmd, "retain")? &show_hdr : &set_options;
    if (firstchar == 'u') {
	if (!*argv) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 632, "un%s what?" ), cmd);
	    return -1;
	} else {
	    do  {
		if (!strcmp(*argv, "*")) {
		    while (*optlist)
			(void) user_unset(optlist, (*optlist)->option);
#if defined(GUI) && !defined(VUI) && !defined(MOTIF)
		    if (istool > 1 && optlist == &set_options)
			show_var((GuiItem)0, NULL, NULL);
#endif /* GUI && !VUI && !MOTIF */
		} else {
		    success = user_unset(optlist, *argv);
#ifdef STUPID_UNSET_WARNING
		    if (success && boolean_val(VarWarning)) {
			error(UserErrWarning,
			    catgets( catalog, CAT_SHELL, 633, "un%s: %s not set." ), cmd, *argv);
		    }
#endif /* STUPID_UNSET_WARNING */
#if defined(GUI) && !defined(VUI) && !defined(MOTIF)
		    if (istool > 1 && success && optlist == &set_options)
			show_var((GuiItem)0, *argv, NULL);
#endif /* GUI && !VUI && !MOTIF */
		}
	    } while (*++argv);
#ifdef GUI
	    if (optlist != &set_options && istool > 1)
		gui_update_list(optlist);
#endif /* GUI */
	}
	return 0;
    }

    if (!*argv) {
	(void) zm_set(optlist, NULL);
	return 0 - in_pipe();
    }

    /*
     * Check for input redirection.  If so, set the variable to the ascii
     * value of the current msg_list.
     */
    if (ison(glob_flags, IS_PIPE)) {
	char *buf;

	if (optlist != &set_options) {
	    error(UserErrWarning,
		catgets( catalog, CAT_SHELL, 634, "You cannot pipe to the \"%s\" command." ), cmd);
	    return -1;
	}
	if (buf = index(argv[0], '='))
	    *buf = 0;
	if (!(buf = list_to_str(list)))
	    return -1;
	if (!buf[0] && !boolean_val(argv[0])) {
	    xfree(buf);
	    return 0;
	}
	(void) user_set_var(argv[0], "=", buf);
	xfree(buf);
	return 0;
    }

    /*
     * finally, just set the variable the user requested.
     */
    user_set(optlist, (const char **) argv);
#ifdef GUI
    if (istool > 1)
#ifdef VUI
	gui_update_list(optlist);
#else /* !VUI */
	if (optlist != &set_options)
	    gui_update_list(optlist);
#ifndef MOTIF
	else
	    while (*argv)
		show_var((GuiItem)0, *argv++, NULL);
	    /*
	     * NOTE: The above is a bit of a hack -- show_var() will
	     * ignore all the elements of argv unless one matches the
	     * name of the variable it is presently displaying.  Most
	     * of the argv will be values, not variable names, but
	     * because add_option() adds multiple settings recursively,
	     * the only way to guarantee that we've updated the right
	     * variables is to walk the argv.  Note that add_option()
	     * will have cleaned up any '=' signs, so those elements of
	     * argv that ARE variable names will be correct.
	     */
#endif /* !MOTIF */
#endif /* !VUI */
#endif /* GUI */
    return 0;
}

int
zm_readonly(argc, argv, list)
int argc;
char **argv;
msg_group *list;
{
    char *cmd = *argv;
    
    while (*++argv) {
	if (var_mark_readonly(*argv) < 0) {
	    print(catgets(catalog, CAT_SHELL, 859, "%s: must be a built-in variable: %s\n"), cmd, *argv);
	    return -1;
	}
    }
    return 0;
}

int
var_mark_readonly(name)
const char *name;
{
    Variable *var;
    
    var = (Variable *) retrieve_link(variables, name, strcmp);
    if (!var)
	return -1;
    turnon(var->v_flags, V_READONLY);
    return 0;
}

/*
 *   The alts list is a list of hostnames or pathnames where the user
 * has an account.  If he doesn't specify "metoo", then when replying
 * to mail, if his address is listed, it will be removed.  The syntax
 * is compatible with ucb Mail in that just hostnames can be used.
 * However, there is an added feature that zmail provides by which another
 * login name or path to another login can be specified by preceding the
 * path or login with a !
 *
 * Bart: Wed May 26 11:36:33 PDT 1993 -- the following is obsolete but
 * can still be forced by use of "saveopts -o alts" so we still do it.
 * "argv" may be a file pointer to write the data into by use of save_opts()
 */
int
alts(argc, arg)
    int argc;
    VPTR arg;
{
    char buf[BUFSIZ], *p;

    if (argc <= 1) {
	FILE *file = (FILE *)arg;
	int n;

	if (!alternates)
	    return 0 - in_pipe();

	if (argc == 0)
	    (void) fprintf(file, catgets( catalog, CAT_SHELL, 635, "#\n# alternate names\n#\nalts " ));

	for (n = 0; alternates[n]; n++) {
	    p = 0;
	    buf[0] = 0;
	    (void) strcpy(&buf[1], alternates[n]);
	    if (buf[1] != '*')
		(void) reverse(&buf[1]);
	    if ((p = rindex(&buf[1], '!')) && !ci_strcmp(p+1, zlogin))
		*p = 0;
	    else if (buf[1] != '*')
		buf[0] = '!';
	    if (argc == 0)
		(void) fprintf(file, "%s ", *buf? buf : &buf[1]);
	    else
		wprint("%s ", *buf? buf : &buf[1]);
	    if (p)
		*p = '!';
	}
	if (argc == 0)
	    (void) fputc('\n', file);
	else
	    wprint("\n");
	return 0 - in_pipe();
    }
    free_vec(alternates);
    if (alternates = (char **)calloc((unsigned)argc+1, sizeof(char *))) {
	char **argv = (char **) arg;
	/* An empty argv[0] means don't reset the variable */
	if (--argc && **argv++) {
	    struct options *opt;

	    if (!(opt = optlist_fetch(&set_options, VarAlternates))) {
		if (!(opt = (struct options *)
			calloc((unsigned)1,sizeof(struct options)))) {
		    error(SysErrWarning, "calloc");
		    return -1;
		}
		opt->option = savestr(VarAlternates);
		opt->value = joinv(NULL, argv, " ");
		insert_option(&set_options, opt, TRUE);
	    } else {
		xfree(opt->value);
		opt->value = joinv(NULL, argv, " ");
	    }
#if defined(GUI) && !defined(VUI)
	    if (istool > 1)
		show_var((GuiItem)0, VarAlternates, NULL);
#endif /* GUI && !VUI */
	}
	while (argc-- > 0) {
	    if (argv[argc][0] == '!')
		alternates[argc] = savestr(reverse(&argv[argc][1]));
	    else if (argv[argc][0] == '*') {
		alternates[argc] = savestr(argv[argc]);
	    } else {
		if (index(argv[argc], '@'))
		    bang_form(buf, argv[argc]);
		else {
		    p = buf + Strcpy(buf, argv[argc]);
		    *p++ = '!', p += Strcpy(p, zlogin);
		}
		alternates[argc] = savestr(reverse(buf));
	    }
	}
    }
    return 0 - in_pipe();
}

static int save_all;	/* Blecch */
extern int zmailrc_read_incomplete; /* in shell/init.c */

/*
 * NOTE: For GUI, ask_item should be set before calling this function!
 */
int
save_opts(cnt, argv)
int cnt;
char **argv;
{
    /* These must appear in the same order as the calls
     * to write out the corresponding values, below.
     */
    static char *optkeys[] = {
	"set",		/*  0 */
	"my_hdr",	/*  1 */
	"alias",	/*  2 */
	"alts",		/*  3 */
	"retain",	/*  4 */
	"ignore",	/*  5 */
	"cmd",		/*  6 */
	"filter",	/*  7 */
	"function",	/*  8 */
	"bind",		/*  9 */
	"map",		/* 10 */
	"map!",		/* 11 */
	"button",	/* 12 */
	"menu",		/* 13 */
	"priority",	/* 14 */
	"interpose",	/* 15 */
	NULL
    };
    char file[MAXPATHLEN], *tmp, *p;
    char *mode = "w", **ok = DUBL_NULL, *mycmd = "saveopts";
    FILE *fp;
    int n, force = FALSE, shut_up = FALSE, save_gui = FALSE;

    save_all = FALSE;
    for (++argv, n = 1; *argv && argv[0][0] == '-' && argv[0][n]; n++) {
	switch (argv[0][n]) {
	    case 'a': mode = "a";
	    when 'A': save_all = TRUE;
	    when 'f': force = TRUE;
	    when 'g': save_gui = TRUE;
	    when 'o':
		if (!argv[1]) {
		    error(UserErrWarning, catgets( catalog, CAT_SHELL, 636, "%s: no keyword" ), mycmd);
		    return -1;
		} else if (!vindex(optkeys, argv[1])) {
		    if (strcmp(argv[1], "gui")) {
			error(UserErrWarning,
			      catgets( catalog, CAT_SHELL, 637, "%s: unknown keyword: %s" ), mycmd, argv[1]);
			return -1;
		    } else
			save_gui = TRUE;
		} else if (!vindex(ok, argv[1]) &&
			vcat(&ok, unitv(argv[1])) < 1) {
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 638, "Cannot save configuration" ));
		    return -1;
		}
		n = 0;
		argv += 2;
		continue;
	    when 'q': shut_up = TRUE;
	    otherwise :
		error(UserErrWarning,
		    catgets( catalog, CAT_SHELL, 639, "%s: %c: unknown option" ), mycmd, argv[0][1]);
		return -1;
	}
	if (!argv[0][n+1]) {
	    n = 0;
	    argv++;
	}
    }

    if (!force && ison(glob_flags, NO_INTERACT))
	return -1;
    
    if (cnt && *argv)
	(void) strcpy(file, *argv);
    else if (tmp = zmRcFileName(0, 1, 1))
	(void) strcpy(file, tmp);
    else /* Should be impossible */
	return -1;

    if (!ok)
	ok = optkeys;

    cnt = 1;
    tmp = getpath(file, &cnt);
    if (cnt) {
	if (cnt == -1) {
	    error(UserErrWarning, "%s: %s", file, tmp);
	    return -1;
	} else {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), tmp);
	    return -2;
	}
    }

    /*
     * the upshot of the stuff below is this:
     *
     * #ifdef MEDIAMAIL
     * if (zmailrc_read_incomplete) {
     *   ask with INC_CONFIRMATION
     * } else if (!force) {
     *   ask with CONFIRMATION
     * }
     * #else
     * if (!force) {
     *   if (zmailrc_read_incomplete) {
     *     ask with INC_CONFIRMATION
     *   } else {
     *     ask with CONFIRMATION
     *   }
     * }
     * #endif
     *
     * except I ifdeffed the two !force lines twice instead of duplicating
     * both blocks.  [spencer Thu Dec 14 16:59:00 1995]
     */
#ifndef MEDIAMAIL
    if (!force) {
#endif /* !MEDIAMAIL */
	if (zmailrc_read_incomplete) {
#define INC_CONFIRMATION \
	    \
catgets(catalog, CAT_SHELL, 927, "Your preferences file was not completely\n\
processed at start-up.  If you overwrite\n\
this file, you may lose some or all of the\n\
original contents of that file.\n\n\
This action is necessary only if you want\n\
to save these settings permanently.  The\n\
current settings are already in effect for\n\
this session.\n\n" )
		if (ison(glob_flags, NO_INTERACT|REDIRECT))
		    return -1;
	    if (ask(WarnNo, catgets( catalog, CAT_SHELL, 642, "%sOverwrite \"%s\"?" ),
		    INC_CONFIRMATION, trim_filename(tmp)) != AskYes) {
		if (istool < 2)
		    error(HelpMessage, catgets( catalog, CAT_SHELL, 643, "\"%s\" unchanged." ), tmp);
		return -3;
	    }
#ifdef MEDIAMAIL
	} else if (!force) {
#else /* MEDIAMAIL */
	} else {
#endif /* MEDIAMAIL */
#define CONFIRMATION \
	    \
		     catgets( catalog, CAT_SHELL, 641, "This action is necessary only if you\n\
want to save these settings permanently.\n\
The current settings are already in effect\n\
for this session.\n\n" )

		/* See if the file exists and confirm overwrite */
		if (*mode == 'w' && !Access(tmp, F_OK)) {
		    if (ison(glob_flags, NO_INTERACT))
			return -1;
		    if (ask(WarnNo, catgets( catalog, CAT_SHELL, 642, "%sOverwrite \"%s\"?" ),
			    (ok == optkeys || save_all)? CONFIRMATION : "", 
			    trim_filename(tmp)) != AskYes) {
			if (istool < 2)
			    error(HelpMessage, catgets( catalog, CAT_SHELL, 643, "\"%s\" unchanged." ), tmp);
			return -3;
		    }
		}
	}
#ifndef MEDIAMAIL
    }
#endif /* !MEDIAMAIL */
    if (!(fp = fopen(tmp, mode))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), file);
	return -1;
    }

    if (*mode == 'w') {
	(void) fprintf(fp, catgets(catalog, CAT_SHELL, 916, "#\n# Z-Mail Initialization\n#\n" ));
	(void) fprintf(fp, "set saved_by_version=\"%s\"\n#\n", check_internal("version"));
#ifdef MAC_OS
	gui_set_filetype(PreferenceFile, tmp, NULL);
#endif
    }
    (void) fprintf(fp, "unset %s\n", VarWarning);

    if (vindex(ok, p = optkeys[0])) {
	save_list(catgets( catalog, CAT_SHELL, 646, "basic variable settings" ), &set_options, p, '=', fp);
	if (vlen(ok) > 1)
	    (void) fprintf(fp, "#\nif hdrs_only\n    exit\nendif\n");
    } else if (ok == optkeys)
	(void) fprintf(fp, "if hdrs_only\n    exit\nendif\n");

    if (vindex(ok, p = optkeys[1]))
	save_list(catgets( catalog, CAT_SHELL, 647, "mail headers for outgoing mail" ), &own_hdrs, p, 0, fp);

    if (vindex(ok, p = optkeys[2]))
	save_list(catgets( catalog, CAT_SHELL, 648, "aliases" ), &aliases, p, 0, fp);

    /* Bart: Wed May 26 12:06:14 PDT 1993
     * Special case "alts" as they are now saved as a variable.
     */
    if (ok != optkeys && vindex(ok, p = optkeys[3]))
	(void) alts(0, (char **)fp);

    if (vindex(ok, p = optkeys[4]))
	save_list(catgets( catalog, CAT_SHELL, 649, "headers to show" ), &show_hdr, p, ' ', fp);
    if (vindex(ok, p = optkeys[5]))
	save_list(catgets( catalog, CAT_SHELL, 650, "headers to ignore" ), &ignore_hdr, p, ' ', fp);

    if (vindex(ok, p = optkeys[6]))
	save_list(catgets( catalog, CAT_SHELL, 651, "command abbreviations" ), &cmdsubs, p, ' ', fp);

    if (vindex(ok, p = optkeys[7])) {
	save_filter(catgets( catalog, CAT_SHELL, 652, "new-mail filters" ), &new_filters, fp);
	save_filter(catgets( catalog, CAT_SHELL, 653, "folder filters" ), &folder_filters, fp);
    }

    if (vindex(ok, p = optkeys[8]))
	save_funct(catgets( catalog, CAT_SHELL, 654, "user-defined functions" ), &function_list, fp);

#ifdef CURSES
    if (vindex(ok, p = optkeys[9]))
	save_cmd("fullscreen mode key bindings", cmd_map, p, 1, fp);
#endif /* CURSES */

    if (vindex(ok, p = optkeys[10]))
	save_cmd(catgets( catalog, CAT_SHELL, 656, "line mode mappings" ), line_map, p, 0, fp);

    if (vindex(ok, p = optkeys[11]))
	save_cmd(catgets( catalog, CAT_SHELL, 657, "composition mode mappings" ), bang_map, p, 0, fp);

    if (vindex(ok, p = optkeys[12])) {
	fprintf(fp, catgets( catalog, CAT_SHELL, 658, "#\n# User-Defined Buttons\n#\n" ));
	print_all_button_info(fp, TRUE, save_all);
    }

    if (vindex(ok, p = optkeys[13])) {
	fprintf(fp, catgets( catalog, CAT_SHELL, 659, "#\n# User-Defined Menus\n#\n" ));
	print_all_button_info(fp, FALSE, save_all);
    }	

    if (vindex(ok, p = optkeys[14]))
	save_priorities(fp, save_all);

    if (vindex(ok, p = optkeys[15]))
	save_interposer_tables(fp, save_all);

    if (boolean_val(VarWarning))
	(void) fprintf(fp, "#\nset %s\n", VarWarning);

    (void) fclose(fp);
    if (!shut_up)
	print(catgets( catalog, CAT_SHELL, 660, "Saved options to %s.\n" ), trim_filename(tmp));
    zmailrc_read_incomplete = 0; /* we just wrote it out, so... */

    if (ok != optkeys)
	free_vec(ok);
    save_all = FALSE;
#ifdef GUI
    if (save_gui)
#ifdef VUI
	gui_save_state();
#else /* VUI */
	gui_save_state(force ? PainterSaveForced : PainterSave);
#endif /* VUI */
#endif /* GUI */
    return 0;
}

static void
save_priorities(fp, save_all)
FILE *fp;
int save_all;
{
    int i;

    if (!save_all && !pri_user_mask) return;
    fprintf(fp, catgets(catalog, CAT_SHELL, 860, "#\n# User-Defined Priorities\n#\n"));
    for (i = 1; i != PRI_NAME_COUNT; i++)
	if (pri_names[i] && (save_all || ison(pri_user_mask, ULBIT(i))))
	    fprintf(fp, "priority '%s=%d'\n", pri_names[i], i);
}


#define SKIPOPTS \
	"connected,cmd_help,cwd,filec,gui_help,hostname,home,motif_help,\
	olit_help,show_hdrs,saved_by_version,tool_help,user,warning"

static void
#ifdef __STDC__
save_list(char *title, struct options **list, char *command,
	  int equals, FILE *fp)
#else
save_list(title, list, command, equals, fp)
    char *title;
    struct options **list;
    char *command;
    int equals;
    FILE *fp;
#endif
{
    register struct options *opts;
    register char *p;

    if (!list || !*list)
	return;
    if (title)
	(void) fprintf(fp, "#\n# %s\n#\n", title);

    clean_slate(list, command, fp);

    optlist_sort(list);
    for (opts = *list; opts; opts = opts->next) {
	char *quote = "\"";
	if (list == &set_options && chk_two_lists(opts->option, SKIPOPTS, ","))
	    continue; /* don't set or unset SKIPOPTS */
	/* don't save vars starting with __ */
	if ((list == &set_options || list == &aliases || list == &cmdsubs) &&
		!strncmp(opts->option, "__", 2))
	    continue;
	if (!need_to_save(list, opts))
	    continue;
	quote = (!!any(opts->option, " \t;|#") ? "'" : "");
	(void) fprintf(fp, "%s %s%s%s", command,
		    quote, quotezs(opts->option, *quote), quote);
	if (opts->value && *opts->value) {
	    quote = "\"";
	    if (!strstr(opts->value, "\\!"))	/* No history characters */
		if (p = any(opts->value, "\"'"))
		    if (*p == '\'')
			quote = "\"";
		    else
			quote = "'";
		else
		    if (!any(opts->value, " \t;|#~"))
			quote = "";
		    else
			quote = "'";
	    (void) fputc(equals? equals: ' ', fp);
	    (void) fprintf(fp, "%s%s%s",
				quote,
				quotezs(opts->value, *quote),
				quote);
	}
	(void) fputc('\n', fp);
    }
}

extern struct cmd_map map_func_names[];

static void
save_cmd(title, list, command, equals, fp)
struct cmd_map *list;
register char *command, *title;
register int equals;
register FILE *fp;
{
    register struct cmd_map *opts;
    register char *p;
    char buf[MAX_MACRO_LEN * 2];

    if (!list)
	return;
    (void) fprintf(fp, "#\n# %s\n#\n", title);

    for (opts = list; opts; opts = opts->m_next) {
	register char *quote;
	if ((p = any(opts->m_str, "\"'")) && *p == '\'')
	    quote = "\"";
	else
	    quote = "'";
	(void) fprintf(fp, "%s %s%s%s", command,
		    quote,
		    quotezs(ctrl_strcpy(buf, opts->m_str, TRUE), *quote),
		    quote);
	if (equals && map_func_names[opts->m_cmd].m_str)
	    (void) fprintf(fp, " %s", map_func_names[opts->m_cmd].m_str);
	if (opts->x_str && *opts->x_str) {
	    if ((p = any(opts->x_str, "\"'")) && *p == '\'')
		quote = "\"";
	    else
		quote = "'";
	    (void) fprintf(fp, " %s%s%s", quote,
			ctrl_strcpy(buf, opts->x_str, TRUE), quote);
	}
	(void) fputc('\n', fp);
    }
}

struct options **
name2optlist(argvp)
char ***argvp;
{
    struct options **list;
    char *cmd = *(*argvp)++;
    int firstchar = 0;

    if (cmd[firstchar] == 'u')
	firstchar = 2;

    switch (cmd[firstchar]) {
	case 'a': case 'e': case 'g':
	    list = &aliases;
	when 'c':
	    list = &cmdsubs;
	when 'f':
	    list = &fkeys;
	otherwise:
	    list = &own_hdrs;
    }

    return list;
}

/*
 * zm_alias handles aliases, header settings, functions, and fkeys.
 * since they're all handled in the same manner, the same routine is
 * used. argv[0] determines which to use.
 * alias is given here as an example
 *
 * alias           identify all aliases
 * alias name      identify alias
 * alias name arg1 arg2 arg3... -> name="arg1 arg2 arg3"; call add_option
 * unalias arg1 [arg2 arg3 ... ]        unalias args
 *
 * same is true for dealing with your own headers.
 * (also the expand command)
 */
int
zm_alias(argc, argv)
    int argc;
    char **argv;
{
    register char *cmd = *argv;
    register char *p;
    struct options **list;
    char firstchar, *buf;

    if (argc == 0)
	return 0 - in_pipe();

    if (cmd[0] == 'u')
	return zm_unalias(argc, argv);

    list = name2optlist(&argv);
    if (cmd[0] == 'u') {
	firstchar = cmd[2];
	Lower(cmd[2]);		/* UGH */
    } else
	firstchar = cmd[0];

    if (!*argv && *cmd != 'e') {
	/* just type out all the aliases or own_hdrs or ... */
	(void) zm_set(list, NULL);
	return 0;
    }

    if (*cmd == 'e') {   /* command was "expand" (aliases only) */
	if (!*argv) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 661, "expand which alias?\n" ));
	    return -1;
	} else
	    do  {
		print("%s: ", *argv);
		if (p = alias_to_address(*argv))
		    print("%s\n", p);
	    } while (*++argv);
	return 0;
    }

    /* check to see if a variable is already set... */
    if (!strcmp(*argv, "-q")) {
	while (*++argv)
	    if (optlist_fetch(list, *argv)) return 1;
	return 0;
    }
    
    /* at this point, *argv now points to a variable name ...
     * check for hdr -- if so, *argv better end with a ':' (check *p)
     */
    if (list == &own_hdrs && !(p = index(*argv, ':'))) {
	error(UserErrWarning,
	    catgets( catalog, CAT_SHELL, 662, "header labels must end with a ':' (%s)\n" ), *argv);
	return -1;
    }
    /* Bart: Tue Jan 19 19:21:24 PST 1993
     * Don't allow names of any sort beginning with a space.
     * This also covers the case of names consisting only of spaces,
     * which is even more ridiculous.
     */
    if (isspace(**argv)) {
	error(UserErrWarning,
	    catgets( catalog, CAT_SHELL, 663, "%s: names must not begin with spaces or tabs" ), cmd);
	return -1;
    }
    /* This is checking for '=' to allow the syntax "alias foo=bar"
     * or "cmd pow=zowie", which probably ought to be a syntax error.
     * Why was it ever supported at all?
     */
    if (!argv[1] && !index(*argv, '='))
	if (p = zm_set(list, *argv))
	    print("%s\n", p);
	else {
	    print(catgets( catalog, CAT_SHELL, 664, "%s is not set\n" ), *argv);
	    return 1;
	}	
    else {
	const char *tmpargv[2];
	if (list == &cmdsubs &&
		argv[0][0] == '\\' || !strcmp(argv[0], "builtin")) {
	    error(UserErrWarning, catgets( catalog, CAT_SHELL, 665, "%s:  Illegal cmd name.\n" ), argv[0]);
	    return -1;
	} else {

	    buf = argv_to_string(NULL, argv);

	    /* FIX THIS when fix_up_addr() has been repaired!	XXX */
	    buf = (char *) realloc(buf, max(HDRSIZ, 2*strlen(buf)));

	    if ((p = any(buf, " \t=")) && *p != '=')
		*p = '=';
	    /* if we're setting an alias, enforce the insertion of commas
	     * between each well-formed address.
	     */
	    if (list == &aliases) {
		fix_up_addr(p+1, 0);
#ifdef CRAY_CUSTOM
		/* XXX DIRECTORY CHECK HERE?? */
#endif /* CRAY_CUSTOM */
	    }
	}
	tmpargv[0] = buf;
	tmpargv[1] = NULL;
	(void) add_option(list, tmpargv);
	xfree(buf);
#ifdef GUI
	if (istool > 1)
	    gui_update_list(list);
#endif /* GUI */
    }
    return 0;
}

int
zm_unalias(argc, argv)
int argc;
char **argv;
{
    register char *cmd = *argv;
    struct options **list;
    char firstchar = *cmd;

    if (firstchar == 'u') {
	firstchar = cmd[2];
    }
    list = name2optlist(&argv);

    if (!*argv) {
	error(HelpMessage, catgets( catalog, CAT_SHELL, 30, "%s what?" ), cmd);
	return -1;
    /* unset a list separated by spaces or ',' */
    } else while (*argv) {
	if (!strcmp(*argv, "*")) /* unset everything */
	    while (*list) {
		(void) un_set(list, (*list)->option);
	    }
	else if (!un_set(list, *argv) && boolean_val(VarWarning))
	    error(HelpMessage, catgets( catalog, CAT_SHELL, 667, "%s: \"%s\" is not set" ), cmd, *argv);
	argv++;
    }
#ifdef GUI
    if (istool > 1)
	gui_update_list(list);
#endif /* GUI */
    return 0;
}

/* Check the boolean/multivalue of a variable and field.
 * If the variable is unset, return false.
 * If the variable is '', return true.
 * See also definition of chk_option() in zmail.h.
 * (chk_option returns false for unset or ''.)
 */
int
bool_option(value, field)
const char *value, *field;
{
    if ((value = value_of(value)) && !*value)
	return TRUE;
    return value ? chk_two_lists(value, field, ", ") : FALSE;
}

typedef struct zm_state {
    struct state_pair {
	char **state;
	union {
	    char **strs;
	    zmFunction *funs;
	} vals;
    }
	variable_state,
	myhdr_state,
	alias_state,
	shown_state,
	ignored_state,
	cmds_state,
	/* button_state, */
	/* bind_state, */
	/* map_state, */
	bang_state,
	nfilter_state,
	ffilter_state,
	/* interposer_state, */
	funct_state;
    /* char **alts_state; */
} zmState;

zmState system_state;

void
stow_state()
{
    /* Using option_to_menu() here as a fast way to duplicate ... */

    option_to_menu(&set_options,
	&system_state.variable_state.state,
	&system_state.variable_state.vals.strs);
    option_to_menu(&own_hdrs,
	&system_state.myhdr_state.state, &system_state.myhdr_state.vals.strs);
    option_to_menu(&aliases,
	&system_state.alias_state.state, &system_state.alias_state.vals.strs);
    /* Doesn't make sense to save alternates, does it? */
    /* vdup(&system_state.alts_state, alternates); */

    option_to_menu(&show_hdr,
	&system_state.shown_state.state, &system_state.shown_state.vals.strs);
    option_to_menu(&ignore_hdr,
	&system_state.ignored_state.state,
	&system_state.ignored_state.vals.strs);
    option_to_menu(&cmdsubs,
	&system_state.cmds_state.state, &system_state.cmds_state.vals.strs);

    cache_funct(&new_filters, (zmFunction *)0);		/* Flush the cache */
    system_state.nfilter_state.state = link_to_argv(new_filters);
    cache_funct(&folder_filters, (zmFunction *)0);	/* Flush the cache */
    system_state.ffilter_state.state = link_to_argv(folder_filters);
    cache_funct(&function_list, (zmFunction *)0);	/* Flush the cache */
    system_state.funct_state.state = link_to_argv(function_list);

    stow_interposer_tables();
    stow_system_buttons();

    /* No convenient way to convert cmd_map at this point */
}

static
struct state_pair *
list_to_pair(list, state)
struct options **list;
zmState *state;
{
    struct state_pair *pair = 0;

    if (list == &set_options)
	pair = &state->variable_state;
    else if (list == &own_hdrs)
	pair = &state->myhdr_state;
    else if (list == &aliases)
	pair = &state->alias_state;
    else if (list == &show_hdr)
	pair = &state->shown_state;
    else if (list == &ignore_hdr)
	pair = &state->ignored_state;
    else if (list == &cmdsubs)
	pair = &state->cmds_state;

    return pair;
}

static int
need_to_save(list, opt)
struct options **list, *opt;
{
    int i;
    char **v;
    struct state_pair *pair = list_to_pair(list, &system_state);

    if (!pair || save_all)
	return 1;

    if (v = vindex(pair->state, opt->option)) {
	i = v - pair->state;
	if (strcmp(pair->vals.strs[i], opt->value) == 0)
	    return 0;
    }

    return 1;
}

static void
clean_slate(list, command, fp)
struct options **list;
char *command;
FILE *fp;
{
    char **v;
    struct state_pair *pair = list_to_pair(list, &system_state);

    if (!pair)
	return;

    for (v = pair->state; v && *v; v++) {
	/* Damn special cases ... */
	if (list == &set_options && chk_two_lists(*v, SKIPOPTS, ","))
	    continue; /* don't set or unset SKIPOPTS */
	if (!zm_set(list, *v))
	    (void) fprintf(fp, "un%s '%s'\n", command, quotezs(*v, '\''));
    }
}

static
struct state_pair *
funlist_to_pair(funlist, state)
zmFunction **funlist;
zmState *state;
{
    struct state_pair *pair = 0;

    if (funlist == &function_list)
	pair = &state->funct_state;
    else if (funlist == &folder_filters)
	pair = &state->ffilter_state;
    else if (funlist == &new_filters)
	pair = &state->nfilter_state;

    return pair;
}

/*
 * How function caching works:
 *
 * 1. Function bodies are cached if and only if they are redefined.
 * 2. A function body already in the cache is not replaced if the
 *    function is redefined again.
 * 3. Function names are cached in a state array by stow_state().
 *    The function body cache is flushed at this time, so redefining
 *    any function after stow_state() will cache the pre-stowed body.
 * 4. A function has been "modified" if either:
 *    a) Its name is not present in the function name state array.
 *    b) Its body differs from the cached body for the same function.
 *
 * 4(a) covers functions added since stow_state(), even if they have
 * not been modified since their creation.  4(b) covers functions
 * that existed before stow_state(), eliminating functions that have
 * been redefined back to the same value they had before stowing.
 */

void
cache_funct(list, fun)
struct zmFunction **list, *fun;
{
    struct state_pair *pair = funlist_to_pair(list, &system_state);

    if (!pair)
	return;

    if (!fun) {
	while (fun = pair->vals.funs) {
	    remove_link(&pair->vals.funs, fun);
	    free_funct(fun);
	}
	return;
    }
    if (retrieve_link(pair->vals.funs,
	    fun->f_link.l_name, strcmp))
	free_funct(fun);	/* Already cached, ignore */
    else
	insert_link(&pair->vals.funs, fun);
}

int
funct_modified(fun, list)
struct zmFunction *fun, **list;
{
    zmFunction *tmp;
    struct state_pair *pair = funlist_to_pair(list, &system_state);

    if (!fun)
	return 0;
    if (!pair || save_all)
	return 1;
    if (!vindex(pair->state, fun->f_link.l_name))
	return 1;
    if (!(tmp = (zmFunction *)
	    retrieve_link(pair->vals.funs, fun->f_link.l_name, strcmp)))
	return 0;
    if (vcmp(fun->f_cmdv, tmp->f_cmdv) != 0)
	return 1;
    return 0;
}

void
clean_funct(funlist, command, fp)
struct zmFunction **funlist;
char *command;
FILE *fp;
{
    char **v;
    struct state_pair *pair = funlist_to_pair(funlist, &system_state);

    if (!pair)
	return;

    for (v = pair->state; v && *v; v++)
	if (!retrieve_link(*funlist, *v, strcmp))
	    (void) fprintf(fp, "un%s %s\n", command, *v);
}

static void
toggle_glob_flag(flag, zcd)
long flag;
ZmCallbackData zcd;
{
    if (zcd->event == ZCB_VAR_SET)
	turnon(glob_flags, flag);
    else
	turnoff(glob_flags, flag);
}

extern void clear_hidden();

static void
hdr_format_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    hdr_format = cdata->xdata ? (char *) cdata->xdata : DEF_HDR_FMT;
}

/* This function should eventually be generalized to receive
 * "FUNCTION_HELP", function_help, and def_function_help in a
 * structure passed through the "data" parameter.  This would allow
 * the same callback to be used for other pathname-like variables as
 * well, such as cmd_help.	- BRL
 */
static void
function_help_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    if (cdata->xdata && *((char *)cdata->xdata))
	ZSTRDUP(function_help, (char *)cdata->xdata);
    else {
	int status = 0;		/* don't ignore no such file or directory */
	char *fallback_path = getenv("FUNCTION_HELP");
	fallback_path = getpath((fallback_path && *fallback_path) ? fallback_path : def_function_help, &status);
	if (def_function_help && *def_function_help)
	    set_var(VarFunctionHelp, "=", def_function_help);
    }
}

typedef struct int_var_info {
    long *ptr;
    int size, name, scale;
    long minval, maxval, unset_val, blank_val;
} int_var_info;

#ifdef GUI
extern int autosave_ct, max_text_length;
#ifdef MOTIF
extern unsigned long attach_prune_size;
#endif /* MOTIF */
#endif /* GUI */
extern unsigned int use_content_length;

int_var_info int_var_infos[] = {
#ifdef IMAP
    { (long*)&imap_timeout, sizeof imap_timeout, VT_ImapTimeout,
          60, MIN_POP_TIMEOUT, 0, 0, POP_TIMEOUT },
#endif /* IMAP */

#ifdef POP3_SUPPORT
    { (long*)&pop_timeout, sizeof pop_timeout, VT_PopTimeout,
	  60, MIN_POP_TIMEOUT, 0, 0, POP_TIMEOUT },
#endif /* POP3_SUPPORT */
#if defined(GUI) || defined(TIMER_API)
    { (long*)&passive_timeout, sizeof passive_timeout, VT_Timeout,
	  1, MIN_PASSIVE_TIMEOUT, 0, PASSIVE_TIMEOUT, PASSIVE_TIMEOUT },
#endif /* GUI || TIMER_API */
    { (long*)&hook_timeout, sizeof hook_timeout, VT_FetchTimeout,
	  1, MIN_HOOK_TIMEOUT, 0, HOOK_TIMEOUT, HOOK_TIMEOUT },
    { (long*)&intr_level, sizeof intr_level, VT_IntrLevel,
	  1, 2, 0, -1, 10 },
    { (long*)&index_size, sizeof index_size, VT_IndexSize,
	  1, 0, 0, -1, 0 },
#ifdef DSERV
    { (long*)&address_cache_timeout, sizeof address_cache_timeout,
	  VT_ExpireCache, 1, 0, 0, 0, 0 },
#endif /* DSERV */
#ifdef GUI
    { (long*)&autosave_ct, sizeof autosave_ct,
	  VT_AutosaveCount, 1, 5, 0, 100, 100 },
    { (long*)&max_text_length, sizeof max_text_length,
          VT_MaxTextLength, 1000, 0, 0, -1, 50 },
#ifdef MOTIF
    { (long*)&attach_prune_size, sizeof attach_prune_size,
      VT_AttachPrune, 1, 0, 0, 1 << 20, 1 << 20 },
#endif /* MOTIF */
#endif /* GUI */
    { (long *)&use_content_length, sizeof use_content_length,
      VT_UseContentLength, 1, 0, 1, 0, 1 },
    { (long *) 0, }
};

typedef struct str_var_info {
    char **ptr;
    int name;
    char *def;
} str_var_info;

str_var_info str_var_infos[] = {
    { &prompt_var, VT_Prompt, DEF_PROMPT },
    { &escape, VT_Escape, DEF_ESCAPE },
    { &msg_separator, VT_MsgSeparator, MSG_SEPARATOR },
    { (char **) 0, },
};

static void
set_int_var_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    int_var_info *iv = (int_var_info *) data;
    long val = iv->blank_val;

    if (cdata->event == ZCB_VAR_UNSET)
	val = iv->unset_val;
    else if (cdata->xdata && *(char *)cdata->xdata) {
	val = iv->scale * atoi((char *)cdata->xdata);
	if (iv->size == sizeof(int)) val = (int) val;
	if (val < iv->minval) val = iv->minval;
	if (iv->maxval && val > iv->maxval) val = iv->maxval;
    }
    if (iv->size == sizeof(int))
	*(int *) iv->ptr = val;
    else
	*iv->ptr = val;
}

static void
set_str_var_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    str_var_info *sv = (str_var_info *) data;

    if (cdata->event == ZCB_VAR_UNSET || !cdata->xdata)
	*sv->ptr = sv->def;
    else
	*sv->ptr = (char *) cdata->xdata;
}

#ifndef MAC_OS
static void
export_var_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    if (cdata->event == ZCB_VAR_UNSET || !cdata->xdata)
	unset_env(data);
    else
	set_env(data, cdata->xdata);
}
#endif /* MAC_OS */

static void
perftweaks_var_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    extern int fmappingok, mcachingok, trustixok, noresortix, fastwriteok;

    if (cdata->event == ZCB_VAR_UNSET || !cdata->xdata)
	fmappingok = mcachingok = trustixok = noresortix = fastwriteok = 0;
    else if (cdata->xdata && !*(char *)(cdata->xdata))
	fmappingok = mcachingok = trustixok = noresortix = fastwriteok = 1;
    else {
	fmappingok = chk_two_lists(cdata->xdata, "mmap_reads", " \t,");
	mcachingok = chk_two_lists(cdata->xdata, "header_cache", " \t,");
	trustixok = chk_two_lists(cdata->xdata, "trust_index", " \t,");
	fastwriteok = chk_two_lists(cdata->xdata, "fast_write", " \t,");
	noresortix = chk_two_lists(cdata->xdata, "no_sort_index", " \t,");
    }
}

void
add_var_callbacks()
{
    static char var_callbacks_added = 0;
    int_var_info *iv;
    str_var_info *sv;

    if (var_callbacks_added)
	return;
    
    ZmCallbackAdd(VarSummaryFmt, ZCBTYPE_VAR, hdr_format_cb, NULL);
    ZmCallbackAdd(VarWarning, ZCBTYPE_VAR, toggle_glob_flag, (VPTR) WARNINGS);
    ZmCallbackAdd(VarMilTime, ZCBTYPE_VAR, toggle_glob_flag, (VPTR) MIL_TIME);
    ZmCallbackAdd(VarDateReceived, ZCBTYPE_VAR, toggle_glob_flag, (VPTR) DATE_RECV);
    ZmCallbackAdd(VarDotLock, ZCBTYPE_VAR, toggle_glob_flag, (VPTR) DOT_LOCK);
    ZmCallbackAdd(VarHidden, ZCBTYPE_VAR, clear_hidden, 0);
    ZmCallbackAdd(VarShowDeleted, ZCBTYPE_VAR, clear_hidden, 0);
    ZmCallbackAdd(VarFunctionHelp, ZCBTYPE_VAR, function_help_cb, NULL);
    ZmCallbackAdd(VarOutgoingCharset, ZCBTYPE_VAR, (void (*)()) out_charset_callback, NULL);
    ZmCallbackAdd(VarIncomingCharset, ZCBTYPE_VAR, (void (*)()) in_charset_callback, NULL);
    ZmCallbackAdd(VarDisplayCharset, ZCBTYPE_VAR, (void (*)()) display_charset_callback, NULL);
    ZmCallbackAdd(VarPrinterCharset, ZCBTYPE_VAR, (void (*)()) printer_charset_callback, NULL);
    ZmCallbackAdd(VarFileCharset, ZCBTYPE_VAR, (void (*)()) file_charset_callback, NULL);
    ZmCallbackAdd(VarTextpartCharset, ZCBTYPE_VAR, (void (*)()) charset_callback, NULL);
    for (iv = int_var_infos; iv->ptr; iv++)
	ZmCallbackAdd(VAR_MAP(iv->name), ZCBTYPE_VAR, set_int_var_cb, iv);
    for (sv = str_var_infos; sv->ptr; sv++)
	ZmCallbackAdd(VAR_MAP(sv->name), ZCBTYPE_VAR, set_str_var_cb, sv);
#ifdef TIMER_API
#ifdef DSERV
    ZmCallbackAdd(VarExpireCache,  ZCBTYPE_VAR,   cache_timeout_reset, NULL);
#endif /* DSERV */
#ifdef USE_FAM
    if (!fam)
#endif /* USE_FAM */
	ZmCallbackAdd(VarTimeout,  ZCBTYPE_VAR, passive_timeout_reset, NULL);
#ifdef POP3_SUPPORT
    ZmCallbackAdd(VarPopTimeout,   ZCBTYPE_VAR,     pop_timeout_reset, NULL);
#endif /* POP3_SUPPORT */
#if defined( IMAP )
    ZmCallbackAdd(VarImapTimeout,  ZCBTYPE_VAR,     imap_timeout_reset, NULL)
;
#endif /* IMAP */
    ZmCallbackAdd(VarFetchTimeout, ZCBTYPE_VAR,    hook_timeout_reset, NULL);
    ZmCallbackAdd(FETCH_MAIL_HOOK, ZCBTYPE_FUNC,  hook_function_reset, NULL);
#endif /* TIMER_API */
#ifndef MAC_OS
    ZmCallbackAdd(VarRealname,     ZCBTYPE_VAR,     export_var_cb, "NAME");
# if defined( POP3_SUPPORT ) || defined( IMAP )
#if defined( IMAP )
    ZmCallbackAdd(VarUseImap,      ZCBTYPE_VAR,     using_imap_cb, "USE_IMAP");
#endif
#if defined( POP3_SUPPORT )
    ZmCallbackAdd(VarUsePop,       ZCBTYPE_VAR,     using_pop_cb, "USE_POP");
#endif
    ZmCallbackAdd(VarMailhost,     ZCBTYPE_VAR,     export_var_cb, "MAILHOST");
#  ifndef UNIX
    ZmCallbackAdd(VarSmtphost,     ZCBTYPE_VAR,     export_var_cb, "SMTPHOST");
#  endif /* UNIX */
# endif /* POP3_SUPPORT || IMAP */ 
# ifdef ZYNC_CLIENT
    ZmCallbackAdd(VarZynchost,     ZCBTYPE_VAR,     export_var_cb, "ZYNCHOST");
# endif /* !ZYNC_CLIENT */
#endif /* MAC_OS */
    ZmCallbackAdd(VarDetachDir, ZCBTYPE_VAR, check_detach_dir, NULL);
    ZmCallbackAdd(VarTmpdir,    ZCBTYPE_VAR, check_detach_dir, NULL);
    ZmCallbackAdd("perftweaks",    ZCBTYPE_VAR,     perftweaks_var_cb, NULL);

    var_callbacks_added = 1;
}
