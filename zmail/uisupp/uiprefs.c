#include <zmail.h>
#include <general.h>
#include "uiprefs.h"
#include <except.h>
#include <dynstr.h>
#include <fsfix.h>
#include <init.h>

#ifndef MAC_OS
/* this file should really be in include */
#include "custom/zync.h"
#endif /* !MAC_OS */

#ifndef lint
static char	uiprefs_rcsid[] =
    "$Id: uiprefs.c,v 1.10 1995/05/19 01:07:47 tom Exp $";
#endif

static zmBool do_save_load P ((zmBool));

zmBool
uiprefs_Save()
{
    return do_save_load(True);
}

zmBool
uiprefs_Load()
{
    return do_save_load(False);
}

static zmBool
do_save_load(save)
zmBool save;
{
    int ret = -1;
    char dflt[MAXPATHLEN];
    char *pmpt, *home;
    u_long flags = PB_FILE_OPTION;
    struct dynstr file;
#ifdef MAC_OS
    char *argv[3];
#endif /* MAC_OS */

#ifdef ZYNC_CLIENT
#if defined(_WINDOWS) || defined(MAC_OS)
    if (bool_option(VarZyncOptions, "prefs") && 
		boolean_val(VarConnected) && (!(ZmUsingUUCP()))) {
#else
    if (bool_option(VarZyncOptions, "prefs")) {
#endif
	AskAnswer answer;
	answer = ask(AskYes, save ?
		     catgets(catalog, CAT_UISUPP, 11, "Save preferences on server?") :
		     catgets(catalog, CAT_UISUPP, 12, "Load preferences from server?"));
	if (answer == AskCancel) return False;
	if (answer == AskYes) {
	    timeout_cursors(True);
	    if (save)
		zync_set_prefs();
	    else
		zync_get_prefs(NULL);
	    timeout_cursors(False);
	    return True;
	}
    }
#endif /* ZYNC_CLIENT */
    (void) strcpy(dflt, zmRcFileName(0, 1, 1));
    if (save)
	pmpt = catgets( catalog, CAT_UISUPP, 68, "Save application state to:" );
    else {
	pmpt =
	    catgets( catalog, CAT_UISUPP, 69, "Load application state from:" );
	turnon(flags, PB_MUST_EXIST|PB_NOT_A_DIR);
#ifdef MAC_OS
	turnon(flags, PB_FILE_BOX|PB_FILE_READ);
	argv[0] = argv[2] = nil;
	argv[1] = "__file_type_filter";
	un_set(&set_options, argv[1]);
	setenv(argv[1], "Zkfg");
#endif /* MAC_OS */
    }
    TRY {
	int result;
	dynstr_Init(&file);
	result = dyn_choose_one(&file, pmpt,
#ifdef UNIX
	    trim_filename(dflt),
#else /* !UNIX */
	    dflt,
#endif /* !UNIX */
	    DUBL_NULL, 0, flags);
	if (result < 0) RAISE("", NULL);
	ret = uiscript_Exec(zmVaStr("\\%s %s",
	    (save) ? "saveopts" : "source",
	    quotezs(dynstr_Str(&file), 0)), 0);
    } EXCEPT(ANY) {
	/* empty */
    } ENDTRY
#ifdef MAC_OS
    if (!ret)
	setenv("ZMAILRC", dynstr_Str(&file));
    if (!save) Unsetenv(2, argv);
#endif    	
    dynstr_Destroy(&file);
    return ret == 0;
}
