#include "osconfig.h"
#include <X11/Intrinsic.h>
#include <Xm/List.h>
#include <Xm/Xm.h>
#include "catalog.h"
#include "listing.h"
#include "private.h"
#include "traverse.h"
#include "uicomp.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include "error.h"



void
#ifdef HAVE_PROTOTYPES
list_get_compose(Widget list, struct AddressArea *prompter, Boolean fresh)
#else /* !HAVE_PROTOTYPES */
list_get_compose(list, prompter, fresh)
    Widget list;
    struct AddressArea *prompter;
    Boolean fresh;
#endif /* !HAVE_PROTOTYPES */
{
    unsigned count;
    int selCount;
    char **mixed = uicomp_compose_to_vector(prompter->compose, &count);
    XmStringTable items = ArgvToXmStringTable(count, mixed);
    int *selected;
#if XmVersion >= 1002
    int keyboardPosition = XmListGetKbdItemPos(list);
#endif /* Motif 1.2 or later */

    /* zero out list items first to prevent disappearing vertical scrollbar */
    XtVaSetValues(list,
		  XmNitems,	0,
		  XmNitemCount, 0,
		  0);
    XtVaSetValues(list,
		  XmNitems,	items,
		  XmNitemCount, count,
		  0);

    if (fresh)
	XtVaSetValues(list, XmNselectedItemCount, selCount = 0, 0);
    else {
#if XmVersion >= 1002
	XmListUpdateSelectedList(list);
#endif /* Motif 1.2 or later */
        XtVaGetValues(list, XmNselectedItemCount, &selCount, 0);
    }
    
    XtSetSensitive(prompter->push, selCount);
    if (selCount && XmListGetSelectedPos(list, &selected, &selCount)) {
	LIST_VIEW_POS(list, selected[0]);
	XtFree((char *) selected);
    }

#if XmVersion >= 1002
    XmListSetKbdItemPos(list, keyboardPosition);
#endif /* Motif 1.2 or later */

    XmStringFreeTable(items);
    free_vec(mixed);
}


static void
list_set_compose(list, compose)
    Widget list;
    struct Compose *compose;
{
    XmStringTable items;
    unsigned count;
    char **mixed;
    
    XtVaGetValues(list,
		  XmNitems,	&items,
		  XmNitemCount, &count,
		  0);

    mixed = XmStringTableToArgv(items, count);
    uicomp_vector_to_compose(mixed, compose);

    free_vec(mixed);
}


void
list_remove(prompter)
    struct AddressArea *prompter;
{
    int *positions;
    int count;
    
    if (XmListGetSelectedPos(prompter->list, &positions, &count)) {
#if XmVersion < 1002
	while (count--)
	    XmListDeletePos(prompter->list, positions[count]);
#else /* Motif 1.2 or later */
	XmListDeletePositions(prompter->list, positions, count);
#endif /* Motif 1.2 or later */
	XtFree((char *)positions);
    }

    XtSetSensitive(prompter->push, False);

    list_set_compose(prompter->list, prompter->compose);
}


void
list_mark_triple(prompter, triple)
    struct AddressArea *prompter;
    char **triple[3];
{
    unsigned selCount;
    char **vector = uicomp_triple_to_vector(triple, &selCount);
    XmStringTable selected = ArgvToXmStringTable(selCount, vector);
    const Widget list = prompter->list;

#if !defined(SELECT_POS_LIST) && XmVersion < 1002
    /* Workaround for buggy Motif list widget */
    XtVaSetValues(list,
	XmNselectedItems, NULL,
	XmNselectedItemCount, 0,
	0);
#endif /* SELECT_POS_LIST */
    XtVaSetValues(list,
	XmNselectedItems, selected,
	XmNselectedItemCount, selCount,
	0);

#if XmVersion >= 1002
    if (selCount) {
	list_get_compose(prompter->list, prompter, False);
	XmListSetKbdItemPos(prompter->list, XmListItemPos(prompter->list, selected[0]));
	if (prompter->refresh) {
	    XtRemoveWorkProc(prompter->refresh);
	    prompter->refresh = 0;
	}
    }
#endif /* Motif 1.2 or later */

    XmStringFreeTable(selected);
    free_vec(vector);
}


void
list_select(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    XmStringTable items;
    unsigned count;

    XtVaGetValues(prompter->list,
		  XmNselectedItems, &items,
		  XmNselectedItemCount, &count,
		  0);

    if (count) {
	char **vector = XmStringTableToArgv(items, count);
	char *string;
	enum uicomp_flavor dominant;
	char **triple[3];
	
	uicomp_vector_to_triple(uicomp_Unknown, vector, NULL, triple);
	dominant = uicomp_predominant_flavor(triple);
	uicomp_free_triple(triple);

	string = uicomp_vector_to_string(vector);
	free_vec(vector);

	field_merge(caller, prompter);

	zmXmTextSetString(prompter->field, string);
	free(string);

	flavor_menu_set(prompter, dominant);
	prompter->dirty = False;
    }

    XtSetSensitive(prompter->push, count);
}


void
list_select_noedit(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    unsigned count;

    XtVaGetValues(prompter->list, XmNselectedItemCount, &count, 0);
    XtSetSensitive(prompter->push, count);
}


void
list_expand(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    char **before[3];
    char **after[3];
    int selCount;
    {
	XmStringTable selected;

	XtVaGetValues(prompter->list,
		      XmNselectedItems, &selected,
		      XmNselectedItemCount, &selCount, 0);
	
	{
	    char **vector = XmStringTableToArgv(selected, selCount);
	    uicomp_vector_to_triple(prompter->dominant, vector, NULL, before);
	    free_vec(vector);
	}
    }

    if (uicomp_expand_triple(before, after, True, ison(prompter->compose->flags, DIRECTORY_CHECK))) {
	list_remove(prompter);
	uicomp_merge_triple(after, prompter->compose);
	list_mark_triple(prompter, after);
    } else
	alias_already_expanded(caller, selCount);

    uicomp_free_triple(after);
    uicomp_free_triple(before);

    (address_edit ? list_select : list_select_noedit)(caller, prompter);
    SetInput(prompter->list);
}


void
list_edit(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    list_select(prompter->list, prompter);
    list_get_compose(prompter->list, prompter, False);
    list_remove(prompter);
    prompter->dirty = True;
    SetTextInput(prompter->field);
    XmTextSetSelection(prompter->field, 0, XmTextGetLastPosition(prompter->field), CurrentTime);
}


void
list_delete(caller, prompter)
    Widget caller;
    struct AddressArea *prompter;
{
    list_remove(prompter);
    field_clear(prompter);
    
    if (ADDRESSED(prompter->compose, TO_ADDR))
	SetInput(prompter->list);
    else
	AddressAreaGotoAddress(prompter, uicomp_To);
}
