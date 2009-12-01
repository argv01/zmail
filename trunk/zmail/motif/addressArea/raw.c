#include "private.h"
#include "shell/file.h"
#include "raw.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include <X11/Intrinsic.h>
#include <stdio.h>


#if XmVersion >= 1002 /* drag & drop */
#include "../drag-drop.h"

static void
#ifdef HAVE_PROTOTYPES
set_drop_layout(struct AddressArea *prompter, Boolean useRaw)
#else /* !HAVE_PROTOTYPES */
set_drop_layout(prompter, useRaw)
    struct AddressArea *prompter;
    Boolean useRaw;
#endif /* !HAVE_PROTOTYPES */
{
    XmDropSiteStartUpdate(prompter->layout);

    if (useRaw) {
	zmDropSiteDeactivate(prompter->subject);
	zmDropSiteDeactivate(prompter->field);
	zmDropSiteActivate(  prompter->raw);
    } else {
	zmDropSiteDeactivate(prompter->raw);
	zmDropSiteActivate( prompter->subject);
	zmDropSiteActivate( prompter->field);
    }

    XmDropSiteEndUpdate(prompter->layout);
}
#else /* no drag & drop */
#define set_drop_layout(prompter, useRaw)
#endif /* no drag & drop */

static void
#ifdef HAVE_PROTOTYPES
set_edit_layout(struct AddressArea *prompter, Boolean useRaw)
#else /* !HAVE_PROTOTYPES */
set_edit_layout(prompter, useRaw)
    struct AddressArea *prompter;
    Boolean useRaw;
#endif /* !HAVE_PROTOTYPES */
{
    if (prompter && prompter->compose && prompter->rawArea) {

	SAVE_RESIZE(GetTopShell(prompter->layout));

	XtSetSensitive(prompter->cookedArea, !useRaw);
	XtSetSensitive(prompter->subjectArea, !useRaw);
	XtSetSensitive(prompter->rawArea, useRaw);

	if (useRaw) {
	    SET_RESIZE(True);
	    XtUnmanageChild(prompter->subjectArea);
	    SET_RESIZE(False);
	    XtUnmanageChild(prompter->cookedArea);
	    XtManageChild(prompter->rawArea);
	    
	    start_textsw_edit(prompter->raw, &prompter->pos_flags);
	} else {
	    SET_RESIZE(False);
	    XtUnmanageChild(prompter->rawArea);
	    XtManageChild(prompter->cookedArea);
	    SET_RESIZE(True);
	    XtManageChild(prompter->subjectArea);
	}

	RESTORE_RESIZE();
    }
}

/*
 * Switch between "cooked" and "raw" edit-headers modes.
 *
 * The effects of this function should be equivalent to the following
 * sequence of operations:
 *
 *    \* With the original state of EDIT_HDRS: *\
 *    SaveComposition(prompter->compose, False);
 *    resume_compose(prompter->compose);
 *    reload_edfile();	
 *    \* Change state of EDIT_HDRS and then: *\
 *    prepare_edfile();
 *    suspend_compose(prompter->compose);
 *    LoadComposition(prompter->compose);
 *
 * To avoid doing all that work, we grovel around with the Compose API
 * a bit so that all we have to do is write out or read in the contents
 * of the prompter->raw text widget.
 */
Boolean
#ifdef HAVE_PROTOTYPES
edit_switch_modes(struct AddressArea *prompter, Boolean fresh)
#else /* HAVE_PROTOTYPES */
edit_switch_modes(prompter, fresh)
    struct AddressArea *prompter;
    Boolean fresh;
#endif /* !HAVE_PROTOTYPES */
{
    Compose *compose = prompter->compose;
    const Boolean desired = ison(compose->flags, EDIT_HDRS);

    if (prompter->rawArea) {
	const Boolean relayout = desired != XtIsManaged(prompter->rawArea);
	
	if (relayout) {
	    char *transferName = NULL;
	    FILE *transfer = open_tempfile("hdr", &transferName);

	    timeout_cursors(TRUE);

	    do {	/* Destitute man's exception handling TRY */
		if (desired) {
		    /* XXX check more return values here */

		    output_headers(&prompter->compose->headers, transfer, True);
		    fclose(transfer);

		    if (OpenFile(prompter->raw, transferName, -1) == 0) {
			turnoff(prompter->compose->flags, EDIT_HDRS);
			break;
		    }
		} else if (!fresh) {
		    HeaderField *hf = compose->headers;

		    fclose(transfer);
		    if (SaveFile(prompter->raw, transferName, "w", False) == 0) {
			turnon(compose->flags, EDIT_HDRS);
			break;
		    }
		    if (!(transfer = fopen(transferName, "r"))) {
			turnon(compose->flags, EDIT_HDRS);
			break;
		    }
		    compose->headers = 0;
		    if (store_headers(&compose->headers, transfer) > 0) {
			free_headers(&hf);
			mta_headers(compose);
		    } else {
			compose->headers = hf;
			turnon(compose->flags, EDIT_HDRS);
			break;
		    }
		}
		/* Destitute man's FINALLY */
		set_edit_layout(prompter, desired);
	    } while (0);	/* Destitute man's ENDTRY */

	    unlink(transferName);
	    free(transferName);

	    timeout_cursors(FALSE);
	}

	if (relayout || fresh) set_drop_layout(prompter, desired);
    }
    
    return desired;
}


void
edit_changed(prompter, data)
    struct AddressArea *prompter;
    struct zmCallbackData *data;
{
    /*
     * We depend here on the fact that $compose_state initialization
     * for new Compositions will *not* trigger callbacks.
     */
    edit_switch_modes(prompter, False);
}
