#include "osconfig.h"
#include "config/features.h"
#ifdef USE_FAM


#include <fam.h>
#include "catalog.h"
#include "zfolder.h"
#include "refresh.h"
#include "zm_fam.h"
#ifdef MOTIF
#include "zm_motif.h"
#endif /* MOTIF */
#include "zmail.h"
#include "zmflag.h"


FAMConnection *fam;
FAMRequest     i18n_fam_request;


void
FAMError(FAMConnection **fam)
{
    if (*fam) FAMClose(*fam);
    *fam = 0;
    
#ifdef MOTIF
    if (fam_input) {
	XtRemoveInput(fam_input);
	fam_input = 0;
    }
#endif /* MOTIF */

    error(ZmErrWarning, catgets(catalog, CAT_SHELL, 902, "The File Alteration Monitor is not responding.\nNew mail checking is disabled."));
}


void
FAMDispatch(FAMConnection **fam)
{
    if (fam && *fam) {
	int pending;
	while ((pending = FAMPending(*fam)) > 0) {
	    FAMEvent event;
	    if (FAMNextEvent(*fam, &event) >= 0) {
		FAMClosure * const closure = (FAMClosure *) event.userdata;
		if (closure && closure->callback)
		  (closure->callback)(&event, closure->data);
	    } else
		FAMError(fam);
	}
	if (pending < 0)
	    FAMError(fam);
    }
}


#ifdef ZMAIL_INTL
static void
i18n_fam_callback(FAMEvent *event, Ftrack *track)
{
    if (ison(spool_folder.mf_flags, CONTEXT_IN_USE))
	ftrack_Do(track, True);
}
#endif /* ZMAIL_INTL */


void
FAMMonitorFolder(FAMConnection *fam, msg_folder *folder)
{
    extern Ftrack *real_spoolfile;

    if (fam)
	switch (folder->mf_type) {
	case FolderDirectory:
	case FolderEmptyDir:
	    FAMMonitorDirectory(fam, folder->mf_name, &folder->fam.request, &folder->fam.closure);
	    break;
	default:
#ifdef ZMAIL_INTL
	    if (real_spoolfile && folder == &spool_folder) {
		/* track the "real" spool too */
		static FAMClosure i18n_closure = { (FAMCallbackProc) i18n_fam_callback, NULL };
		i18n_closure.data = real_spoolfile;
		FAMMonitorFile(fam, real_spoolfile->ft_name, &i18n_fam_request, &i18n_closure);
	    }
#endif /* ZMAIL_INTL */
	    FAMMonitorFile(fam, folder->mf_name, &folder->fam.request, &folder->fam.closure);
	}
}


#endif /* USE_FAM */
