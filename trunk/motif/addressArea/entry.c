#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "entry.h"
#include "listing.h"
#include "private.h"
#include "traverse.h"
#include "vars.h"
#include "zmcomp.h"
#include "zm_motif.h"
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>


extern Bool address_replace;


void
flavor_menu_apply(menuItem, flavor)
    Widget menuItem;
    enum uicomp_flavor flavor;
{
    struct AddressArea *prompter;
    XtVaGetValues(XtParent(menuItem), XmNuserData, &prompter, 0);

    if (flavor != prompter->dominant)
	prompter->dirty = True;
    prompter->dominant = flavor;

    if (prompter->progress)
	field_merge(prompter->menu, prompter);
    else {
	char *edibles = XmTextGetString(prompter->field);
	char *bland = uicomp_make_bland(edibles);
	
	zmXmTextSetString(prompter->field, bland);
	
	XtFree(edibles);
	XtFree(bland);
    }

    SetTextInput(prompter->field);
}


void
flavor_menu_set(prompter, flavor)
    struct AddressArea *prompter;
    enum uicomp_flavor flavor;
{
    Widget cascade = XmOptionButtonGadget(prompter->menu);
    
    if (flavor == uicomp_Unknown)
	XtVaSetValues(cascade, XmNlabelString, zmXmStr(""), 0);
    else {
	XtVaSetValues(cascade, XmNlabelString, zmXmStr(uicomp_flavor_names[flavor]), 0);
	SetNthOptionMenuChoice(prompter->menu, flavor);
    }

    prompter->dominant = flavor;
}


void
field_clear(prompter)
    struct AddressArea *prompter;
{
    if (prompter->dominant == uicomp_Unknown)
	flavor_menu_set(prompter, uicomp_To);
    zmXmTextSetString(prompter->field, NULL);
    prompter->dirty = False;
}


void
field_merge(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    if (prompter->dirty) {
	char *content = XmTextGetString(prompter->field);
	char **before[3];
	char **after[3];
	enum uicomp_flavor newDominant;

	ask_item = caller;	/* string-to-triple may run address book */

	if (address_replace) list_remove(prompter);

	uicomp_string_to_triple(prompter->dominant, content, &newDominant, before);
	uicomp_expand_triple(before, after, aliases_should_expand(), ison(prompter->compose->flags, DIRECTORY_CHECK));
	uicomp_merge_triple(after, prompter->compose);

	if (caller != prompter->menu)
	    flavor_menu_set(prompter, newDominant);

	if (address_replace) {
	    char **newbies = uicomp_triple_to_vector(after, 0);

	    if (newbies && *newbies) {
		XmString first = XmStr(*newbies);

		list_get_compose(prompter->list, prompter, False);
		LIST_VIEW_POS(prompter->list, XmListItemPos(prompter->list, first));
		
		XmStringFree(first);
		free_vec(newbies);
	    }
	} else if (caller != prompter->list)
	    list_mark_triple(prompter, after);

	uicomp_free_triple(before);
	uicomp_free_triple(after);
	
	XtFree(content);
    }
    field_clear(prompter);
}


void
field_activate(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    field_merge(caller, prompter);
    progress(prompter);
}


void
field_dirty(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    prompter->dirty = True;
}
