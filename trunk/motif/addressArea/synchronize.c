#include "gui_def.h"
#include "private.h"
#include "synchronize.h"
#include <X11/Intrinsic.h>


static Boolean
synchronize(prompter)
    struct AddressArea *prompter;
{
    list_get_compose(prompter->list, prompter, False);
    prompter->refresh = 0;
    return True;
}


void
address_changed(prompter, data)
    struct AddressArea *prompter;
    struct zmCallbackData *data;
{
    if (prompter->compose == (struct Compose *) data->xdata && !prompter->refresh)
	prompter->refresh = XtAppAddWorkProc(app, (XtWorkProc) synchronize, prompter);
}


void
AddressAreaFlush(prompter)
    struct AddressArea *prompter;
{
    if (prompter && prompter->compose) {
	field_merge(prompter->field,   prompter);
	subject_set_compose(prompter->subject, prompter);
    }
}
