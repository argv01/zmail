/* lpr.c     Copyright 1993 Z-Code Software Corp. */

#ifndef lint
static char	lpr_rcsid[] =
    "$Id: lpr.c,v 2.35 1998/12/07 23:50:09 schaefer Exp $";
#endif

#include "osconfig.h"
#include "lpr.h"
#include "mime.h"
#include "pager.h"
#include "zmail.h"

#ifdef HAVE_IMPRESSARIO
#include <spool.h>
#endif /* HAVE_IMPRESSARIO */

#define MSG_NO_PRINTER catgets( catalog, CAT_SHELL, 124, "Specify printer")
#define MSG_NO_PRINTCMD catgets(catalog, CAT_SHELL, 883, "Specify print command")
#define MSG_NO_MSGS    catgets( catalog, CAT_SHELL, 127, "No messages input to lpr.\n")
#define MSG_PRT_MSG    catgets(catalog, CAT_SHELL, 869, "Printing %d message ...")
#define MSG_PRT_MSGS   catgets(catalog, CAT_SHELL, 870, "Printing %d messages ...")
#define MSG_PRT_NO     catgets( catalog, CAT_SHELL, 130, "printing message %d ... " )
#define MSG_PRTD_MSG   catgets(catalog, CAT_SHELL, 871, "%d message printed")
#define MSG_PRTD_MSGS  catgets(catalog, CAT_SHELL, 872, "%d messages printed")

static void next_page P ((ZmPager *, int, struct printdata *));
static int lpr_files P ((struct printdata *, char **));
static int lpr_messages P ((struct printdata *, msg_group *, long));

int
zm_lpr(argc, argv, list)
int argc;
char **argv;
struct mgroup *list;
{
    long copy_flags = 0;
    const char *printer = NULL, *printcmd = NULL;
    int	msg_args, result;
    int single_process = False;
    struct printdata pdata;

    bzero((VPTR) &pdata, sizeof pdata);
    if (!chk_option(VarAlwaysignore, "printer"))
	turnon(copy_flags, NO_IGNORE);
    turnon(copy_flags, NO_SEPARATOR);
    turnon(copy_flags, FOLD_ATTACH);

    while (argv && *++argv && **argv == '-') {
	char c;
	int n;
	
	n = 1;
	while (c = argv[0][n++])
	    switch(c) {
		case 'n': turnon(copy_flags, NO_HEADER);
		when 'h': turnoff(copy_flags, NO_IGNORE);
		when 's': single_process = 1;
		when 'a': turnon(copy_flags, NO_IGNORE);
		when 'P': case 'd':
		    if (!argv[0][n]) {
		        error(HelpMessage, MSG_NO_PRINTER);
		        return -1;
		    }
		    if (argv[0][n]) {
			printer = argv[0] + n;
			n += strlen(printer);
		    }
		when 'c':
		    if (!argv[0][n]) {
		        error(HelpMessage, MSG_NO_PRINTCMD);
		        return -1;
		    }
		    if (argv[0][n]) {
			printcmd = argv[0] + n;
			n += strlen(printcmd);
		    }
		when '?' : return help(0, "lpr", cmd_help);
		otherwise:
		    help(0, "lpr", cmd_help);
		    return -1;
	    }
    }
    msg_args = get_msg_list(argv, list);
    if (msg_args == -1) return -1;

    pdata.printer  = (char *) printer;	/* XXX casting away const */
    pdata.printcmd = (char *) printcmd;	/* XXX casting away const */
    pdata.single_process = single_process;

    SetCurrentCharSet(printerCharSet);

    if (msg_args == 0 && *argv)
	result = lpr_files(&pdata, argv);
    else
	result = lpr_messages(&pdata, list, copy_flags);

    SetCurrentCharSet(displayCharSet);		/* Just in case */

    return result;
}

static int
lpr_messages(pdata, list, copy_flags)
struct printdata *pdata;
msg_group *list;
long copy_flags;
{
    int total, Total, n, cancelflag = FALSE, retval = 0;
    ZmPager pager = 0;
    char *print_cmd = "", *msgstr;
    long lines;	

    Total = count_msg_list(list);
    if (Total == 0) {
#ifdef GUI
	if (istool) print(MSG_NO_MSGS);
#endif /* GUI */
	return 0;
    }
    init_intr_mnr(zmVaStr(Total == 1 ? MSG_PRT_MSG:MSG_PRT_MSGS, Total), 0);
    
    if (Total > 1)
	print("%s\n", zmVaStr(MSG_PRT_MSGS, Total));

    for (n = total = 0; n < msg_cnt; n++) {
	if (!msg_is_in_group(list, n)) continue;
	next_page(&pager, total++, pdata);
	if (!pager) {
	    if (total == 1)
		cancelflag = TRUE;
	    break;
	}
	ZmPagerSetCurMsg(pager, n);
	ZmPagerSetMsgList(pager, list);
	if (check_intr_mnr(zmVaStr(MSG_PRT_NO, n+1),
	    (long)(total*100)/Total))
	    break;
	print("%s", zmVaStr(MSG_PRT_NO, n+1));
	lines = copy_msg(n, NULL_FILE, (u_long) copy_flags, NULL, 0);
	if (lines < 0 || ZmPagerCheckError(pager)) {
	    print_more(catgets( catalog, CAT_SHELL, 131, "failed.\n" ));
	    retval = -1;
	    total--;
	    break;
	}
	if (ZmPagerCheckCancel(pager)) {
	    print_more(catgets(catalog, CAT_SHELL, 894, "cancelled.\n"));
	    break;
	}
	print_more(catgets( catalog, CAT_SHELL, 132, "(%d lines)\n" ), lines);
	turnon(msg[n]->m_flags, PRINTED|DO_UPDATE);
	turnoff(msg[n]->m_flags, NEW);
	turnon(folder_flags, DO_UPDATE);
    }
    if (pager) {
	const int errflag = ZmPagerCheckError(pager);
	print_cmd = savestr(ZmPagerGetProgram(pager));
	cancelflag = ZmPagerCheckCancel(pager);
	ZmPagerStop(pager);
	if (errflag) {
	    /* Should there be an error message here? */
	    retval = -1;
	}
    } else
	total = 0;

    if (total > 0) {
	msgstr = zmVaStr(total == 1 ? MSG_PRTD_MSG : MSG_PRTD_MSGS, total);
	print_more(msgstr);
#ifdef UNIX
	print_more(catgets( catalog, CAT_SHELL, 135, " through \"%s\".\n" ), print_cmd);
#else /* !UNIX */
	/* windows doesn't care about print_cmd, and neither does mac,
	 * one would assume.  pf Fri Jan 14 13:30:45 1994
	 */
	print_more(".\n");
#endif /* !UNIX */
    } else
	msgstr = catgets( catalog, CAT_SHELL, 136, "Failed." );
    xfree(print_cmd);

    end_intr_mnr(msgstr, (long)(total*100)/Total);

#ifdef GUI
    if (istool == 2 && total < Total && !cancelflag)
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 137, "Printed %d of %d messages" ), total, Total);
#endif /* GUI */

    return retval;
}

static int
lpr_files(pdata, files)
struct printdata *pdata;
char **files;
{
    int total = 0, Total = vlen(files), ret = 0;
    int success = 0;
    FILE *in;
    ZmPager pager = (ZmPager) 0;
    
    init_intr_mnr(zmVaStr(catgets(catalog,
	CAT_SHELL, 840, "Printing %d files..."), Total),
	INTR_VAL(Total));

    for (; *files; files++) {
	next_page(&pager, total++, pdata);
	if (!pager) {
	    print(catgets(catalog, CAT_SHELL, 880, "Cannot initialize printer."));
	    break;
	}
	if (!(in = fopen(*files, "r"))) {
	    ret = -1;
	    error(SysErrWarning, catgets(catalog, CAT_SHELL, 398, "Cannot open \"%s\""), *files);
	    continue;
	}
	if (check_intr_mnr(zmVaStr(catgets(catalog, CAT_SHELL, 842, "printing %s ... "), *files),
	      (long)(total*100)/Total))
	    break;
	fioxlate(in, 0, -1, NULL_FILE, fiopager, pager);
	fclose(in);
	if (ZmPagerCheckError(pager)) {
	    print(catgets(catalog, CAT_SHELL, 856, "Cannot print %s.\n"), *files);
	    break;
	}
	success = 0;
	print(catgets(catalog, CAT_SHELL, 843, "Printed %s through \"%s\".\n"), *files, ZmPagerGetProgram(pager));
    }
    if (pager) ZmPagerStop(pager);

    end_intr_mnr(catgets( catalog, CAT_SHELL, 119, "Done." ), (long)(total*100)/Total);
    return ret;
}

static void
next_page(pager, pageno, pdata)
ZmPager *pager;
int pageno;
struct printdata *pdata;
{
    if (pageno) {
	if (pdata->single_process) {
	    ZmPagerWrite(*pager, "\f");
	    return;
	}
	ZmPagerStop(*pager);
    }
    *pager = printer_setup(pdata, pageno);
}


char *
printer_choose_one(allPrinters)
    const char *allPrinters;
{
    char *selection;

    if (!allPrinters)
	allPrinters = value_of(VarPrinter);
    
    if (allPrinters) {
	char **choices = strvec(allPrinters, ", \t", TRUE);

	if (choices) {
	    int n_choices = vlen(choices);
	    if (n_choices > 1) {
		char buffer[BUFSIZ];
#ifdef HAVE_IMPRESSARIO
		char *dflt;
		SLGetDefPrinterName(&dflt);
#else /* !HAVE_IMPRESSARIO */
		const char *dflt = choices[0];
#endif /* !HAVE_IMPRESSARIO */
		buffer[0] = 0;
		if (choose_one(buffer, catgets( catalog, CAT_SHELL, 122, "Select Printer:" ),
			       dflt, choices, n_choices, 0L) < 0)
		    selection = 0;
		else
		    selection = savestr(buffer);
	    } else if (choices)
		selection = savestr(choices[0]);
	    free_vec(choices);
	} else
	    selection = savestr("");
    } else
	selection = savestr(DEF_PRINTER);
    
    return selection;
}


struct zmPager *
printer_setup(pdata, use_same_printer)
const struct printdata *pdata;
int use_same_printer;
{
    char *p2, *lprcmd, tmp[256];
    const char *printer_opt, *p;
    int append = TRUE, cmd = 0;
    char print_cmd[BUFSIZ];
    ZmPager pager;
    int use_default = False;
    char *printerList = pdata->printer;
    static char *printer = NULL;

#ifdef PRINTER_OPT
    char *opt = PRINTER_OPT;
#else
    char opt[3];
    opt[0] = '-';
#ifdef SYSV
    opt[1] = 'd';
#else
    opt[1] = 'P';
#endif /* SYSV */
    opt[2] = 0;
#endif /* PRINTER_OPT */

    if (!printer || !use_same_printer){
	if(printer)free(printer);
	printer = printer_choose_one(printerList);
    }
    if (!printer)
	return 0;
    
    if (!*printer || *printer == '-')
	printer_opt = printer;
    else if (!(printer_opt = value_of(VarPrinterOpt)))
	printer_opt = opt;
    for (p = printer_opt, p2 = tmp; *p; p++)
	if (*p == '%' && p[1] == 'P') {
	    append = FALSE;
	    p2 += Strcpy(p2, printer);
	    p++;
	} else
	    *p2++ = *p;
    if (append && printer_opt != printer)
	(void) strcpy(p2, printer);
    else
	*p2 = 0;
    printer_opt = tmp;

    lprcmd = pdata->printcmd;
    if (!lprcmd && (!(lprcmd = value_of(VarPrintCmd)) || !*lprcmd)) {
	lprcmd = LPR;
	append = TRUE;
	use_default = (printer) ? False : True;
    } else {
	cmd = 1;
	/* Never append printer_opt to a user's print_cmd;
	 * if they want printer_opt included, they use %P.
	 */
	append = FALSE;
    }
    for (p = lprcmd, p2 = print_cmd; *p; p++)
	if (*p == '%') {
	    switch (p[1]) {
		case 'P' :
		    append = FALSE;
		    p2 += Strcpy(p2, printer);
		    p++;
		when 'O' :
		    append = FALSE;
		    p2 += Strcpy(p2, printer_opt);
		    p++;
		otherwise: *p2++ = *p;
	    }
	} else
	    *p2++ = *p;
    if (append) {
	*p2++ = ' ';
	(void) strcpy(p2, printer_opt);
    } else
	*p2 = 0;

    Debug("print command: %s\n", print_cmd);

    pager = ZmPagerStart(PgPrint);
    ZmPagerSetProgram(pager, print_cmd);
    ZmPagerSetPrinter(pager, printer);

    ZmPagerSetCharset(pager, printerCharSet);

    if (use_default)
	ZmPagerSetFlag(pager, PG_DEFAULT);
    
    return pager;
}
