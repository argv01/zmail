#include "zmail.h"
#include "cparse.h"

#include <ctype.h>

static cpOpt lastoptlist = (cpOpt) 0;

int
cpParseOpts(cpdata, argv, descs, fixflags, flags)
cpData cpdata;
char **argv;
cpDesc descs;
char *(*fixflags)[2];
u_long flags;
{
    char *arg, *cmd, *arg2;
    int opt;
    cpDesc desc;
    static cpDescRec cpRegArgDesc = { ' ', CP_STR_ARG, 0 };

    xfree(lastoptlist);
    bzero((char *) cpdata, sizeof *cpdata);
    cpdata->argv = argv;
    cpdata->descs = descs;
    cpdata->optslots = 4;
    cpdata->optlist = lastoptlist =
	(cpOpt) malloc(cpdata->optslots * sizeof(*cpdata->optlist));
    cmd = *argv++;
    while (*argv) {
	if (**argv != '-') {
	    if (isoff(flags, CP_ALL_ARGS)) break;
	    cpAddOpt(cpdata, &cpRegArgDesc, *argv++);
	    continue;
	}
	fix_word_flag(argv, fixflags);
	arg = *argv++; opt = arg[1];
	desc = cpGetDesc(cpdata, opt);
	if (!desc) {
	    print(catgets(catalog, CAT_SHELL, 861, "%s: unrecognized option: %s\n"), cmd, arg);
	    return FALSE;
	}
	if (cpdata->opts[desc->opt] && isoff(desc->flags, CP_MULTIPLE)) {
	    print(catgets(catalog, CAT_SHELL, 862, "%s: multiple %s options not allowed\n"), cmd, arg);
	    return FALSE;
	}
	if (isoff(desc->flags, CP_STR_ARG|CP_INT_ARG)) {
	    int type = arg[2] == '!' ? cpOff : cpOn;
	    if (ison(desc->flags, CP_POSITIONAL))
		cpAddOpt(cpdata, desc, (VPTR) type);
	    else
		cpAddOptBool(cpdata, desc, type);
	} else {
	    arg2 = NULL;
	    if (*argv && arg[2] != '!')
		arg2 = *argv++;
	    else if (isoff(desc->flags, CP_OPT_ARG)) {
		print(catgets(catalog, CAT_SHELL, 863, "%s: argument expected to %s\n"), cmd, arg);
		return FALSE;
	    }
	    if (ison(desc->flags, CP_INT_ARG)) {
		if (!isdigit(*arg2)) {
		    print(catgets(catalog, CAT_SHELL, 864, "%s: argument to %s must be a number\n"),
			cmd, arg);
		}
		cpAddOpt(cpdata, desc, (VPTR) atoi(arg2));
	    } else
	      cpAddOpt(cpdata, desc, arg2);
	}
    }
    /* XXX casting away const */
    cpdata->argv = (char **) argv;
    return TRUE;
}

VPTR
cpGetOpt(cpdata, opt)
cpData cpdata;
int opt;
{
    int i;
    cpOpt copt;

    if (cpdata->opts[opt] != cpOn) return (VPTR) 0;
    for (i = 0, copt = cpdata->optlist; i != cpdata->optct; i++, copt++) {
	if (copt->desc->opt == opt)
	    return copt->value;
    }
    return (VPTR) 0;
}

void
cpAddOpt(cpdata, desc, arg)
cpData cpdata;
cpDesc desc;
VPTR arg;
{
    cpOpt opt;
    
    if (cpdata->optslots == cpdata->optct) {
	cpdata->optslots += 10;
	cpdata->optlist = (cpOpt) realloc(cpdata->optlist,
	    			  sizeof(*cpdata->optlist)*cpdata->optslots);
	lastoptlist = cpdata->optlist;
    }
    opt = &cpdata->optlist[cpdata->optct++];
    bzero((char *) opt, sizeof *opt);
    opt->desc = desc;
    opt->value = arg;
    if (ison(desc->flags, CP_STR_ARG|CP_INT_ARG))
	cpdata->opts[desc->opt] = (arg) ? cpOn : cpOff;
    else
	cpAddOptBool(cpdata, desc, (int) arg);
}

void
cpAddOptBool(cpdata, desc, arg)
cpData cpdata;
cpDesc desc;
int arg;
{
    cpdata->opts[desc->opt] = arg;
    if (arg == cpOn)
	turnon(cpdata->onbits, desc->bit);
    else
	turnoff(cpdata->offbits, desc->bit);
}

cpDesc
cpGetDesc(cpdata, opt)
cpData cpdata;
int opt;
{
    cpDesc desc = cpdata->descs;
    
    for (; desc->opt; desc++)
	if (desc->opt == opt) return desc;
    return (cpDesc) 0;
}

int
cpOtherOpts(cpdata, opts)
cpData cpdata;
char *opts;
{
    cpDesc desc;

    desc = cpdata->descs;
    for (; desc->opt; desc++)
	if (cpdata->opts[desc->opt] && !index(opts, desc->opt))
	    return desc->opt;
    return 0;
}

int
cpHasOpts(cpdata, opts)
cpData cpdata;
char *opts;
{
    for (; *opts; opts++) if (cpdata->opts[*opts] == cpOn) return *opts;
    return 0;
}

int
cpNextOpt(cpdata, opt, strarg, intarg)
cpData cpdata;
int *opt;
char **strarg;
int *intarg;
{
    cpOpt copt;
    
    if (cpdata->optct == cpdata->optptr) return FALSE;
    copt = &cpdata->optlist[cpdata->optptr++];
    *opt = copt->desc->opt;
    if (ison(copt->desc->flags, CP_STR_ARG))
	*strarg = (char *) copt->value;
    else
	*intarg = (int) copt->value;
    return TRUE;
}
