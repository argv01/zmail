#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "callback.h"
#include "private.h"
#include "subject.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include <X11/Intrinsic.h>
#include <Xm/Text.h>
#include <Xm/Xm.h>


void
subject_get_compose(prompter)
    struct AddressArea *prompter;
{
    char **subject = get_address(prompter->compose, SUBJ_ADDR);
    if (subject) {
	zmXmTextSetString(prompter->subject, subject[0]);
	free_vec(subject);
    } else
	zmXmTextSetString(prompter->subject, NULL);
}


void
subject_changed(prompter, data)
    struct AddressArea *prompter;
    struct zmCallbackData *data;
{
    if (prompter->compose == (struct Compose *) data->xdata)
	subject_get_compose(prompter);
}


void
subject_set_compose(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    char *content = XmTextGetString(prompter->subject);

    ZmCallbackRemove(prompter->subjectSync);
    set_address(prompter->compose, SUBJ_ADDR, &content);
    prompter->subjectSync = ZmCallbackAdd("subject", ZCBTYPE_ADDRESS, subject_changed, prompter);

    XtFree(content);
}


void
subject_activate(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    subject_set_compose(caller, prompter);
    if (prompter->progressLast) SetTextInput(*(prompter->progressLast));
}
