#include "osconfig.h"
#include "../attach/area.h"
#include "addressArea.h"
#include "entry.h"
#include "geometry.h"
#include "listing.h"
#include "private.h"
#include "raw.h"
#include "walktag.h"
#include "subject.h"
#include "synchronize.h"
#include "traverse.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/List.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#ifdef SANE_WINDOW
#include "../xm/sanew.h"
#define PANED_WINDOW_CLASS zmSaneWindowWidgetClass
#else /* !SANE_WINDOW */
#include <Xm/PanedW.h>
#define PANED_WINDOW_CLASS xmPanedWindowWidgetClass
#endif /* !SANE_WINDOW */


static void
destroy(suicide, prompter)
    Widget suicide;
    struct AddressArea *prompter;
{
    AddressAreaDestroy(prompter);
}


#define LINEUP(widget, prompter, position) XtAddEventHandler(prompter->lineup[position] = widget, StructureNotifyMask, False, (XtEventHandler) align, prompter)

void AddressAreaUnmanage(prompter)
struct AddressArea *prompter;
{
  XtUnmanageChild(prompter->layout);
  XtUnmanageChild(prompter->subjectArea);
}

void AddressAreaManage(prompter)
struct AddressArea *prompter;
{
  XtManageChild(prompter->layout);
  XtManageChild(prompter->subjectArea);
}

void AddressAreaFocus(prompter,on_field)
struct AddressArea *prompter;
Boolean on_field;
{
  if (on_field)
    SetTextInput(prompter->field);
  else
    SetTextInput(prompter->raw);
}

struct AddressArea *
AddressAreaCreate(parent, attachments, progressLast)
    Widget parent;
    AttachArea *attachments;
    Widget *progressLast;
{
    struct AddressArea *prompter = (struct AddressArea *) malloc(sizeof(*prompter));

    prompter->layout = XtVaCreateWidget("prompter", xmFormWidgetClass, parent,
					XmNfractionBase, 12,
#ifdef SANE_WINDOW
					ZmNextResizable, True,
#endif /* SANE_WINDOW */
					0);
    
    {
	Widget label;
	
	prompter->subjectArea = XtVaCreateWidget("subject", xmFormWidgetClass, parent,
						 XmNskipAdjust, True,
						 /* XmNmarginWidth, 0, */
						 XmNmarginHeight, 0,
#ifdef SANE_WINDOW
						 ZmNextResizable, False,
						 ZmNhasSash, False,
#endif /* SANE_WINDOW */
						 0);
	DialogHelpRegister(prompter->subjectArea, "Compose Headers");

	label = XtVaCreateManagedWidget("Subject:", xmLabelWidgetClass, prompter->subjectArea,
					XmNtopAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNleftAttachment, XmATTACH_FORM,
					0);
	
	LINEUP(label, prompter, Subject);
	
	prompter->subject = XtVaCreateManagedWidget("subject_text", xmTextWidgetClass, prompter->subjectArea,
						    XmNrows, 1,
						    XmNeditMode, XmSINGLE_LINE_EDIT,
						    XmNtopAttachment, XmATTACH_FORM,
						    XmNbottomAttachment, XmATTACH_FORM,
						    XmNleftAttachment, XmATTACH_WIDGET,
						    XmNleftWidget, label,
						    XmNrightAttachment, XmATTACH_FORM,
						    0);

	XtAddCallback(prompter->subject, XmNactivateCallback, (XtCallbackProc) subject_activate, prompter);
	XtAddCallback(prompter->subject, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, (XtPointer)True);
	XtManageChild(prompter->subjectArea);
    }
    
    if (attachments) {
	*attachments = create_attach_area(prompter->layout, FrameCompose);
	DialogHelpRegister(attach_area_widget(*attachments), "Compose Attachment Panel");
    }

    {
	Arg args[9];
	int argcount =
	XtVaSetArgs(args, XtNumber(args),
		    XmNtopAttachment,    XmATTACH_FORM,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNleftAttachment,   XmATTACH_FORM,
		    XmNrightAttachment,  XmATTACH_POSITION,
		    XmNrightPosition,    attachments ? 7 : 12,
		    XmNeditable,         True,
		    XmNeditMode,         XmMULTI_LINE_EDIT,
		    XmNscrollingPolicy,  XmAUTOMATIC,
		    XmNverifyBell,       False,
		    NULL);

	if (attachments) {
	    prompter->raw = XmCreateScrolledText(prompter->layout, "raw", args, XtNumber(args));
	    XtAddCallback(prompter->raw,
		XmNmodifyVerifyCallback, (XtCallbackProc) progress_callback,
		prompter);
	    XtAddCallback(prompter->raw,
		XmNmotionVerifyCallback, (XtCallbackProc) progress_callback,
		prompter);
	    XtManageChild(prompter->raw);
	    prompter->rawArea = XtParent(prompter->raw);
	    XtUnmanageChild(prompter->rawArea);
	} else {
	    prompter->raw = 0;
	    prompter->rawArea = 0;
	}
	prompter->pos_flags = 0;

	/* Replace scrollingPolicy with separatorOn */
	XtSetArg(args[argcount-2], XmNseparatorOn, False);
	/* Replace verifyBell with spacing */
	XtSetArg(args[argcount-1], XmNspacing, 4);

	prompter->cookedArea = XtCreateWidget("cooked", PANED_WINDOW_CLASS, prompter->layout, args, argcount);
	DialogHelpRegister(prompter->cookedArea, "Address Editing");

	{
	    Widget entryArea = XtVaCreateWidget("entry", xmFormWidgetClass, prompter->cookedArea,
						XmNskipAdjust, True,
						XmNmarginWidth, 0,
						XmNmarginHeight, 0,
#ifdef SANE_WINDOW
						ZmNextResizable, False,
						ZmNhasSash, False,
#endif /* SANE_WINDOW */
						0);

	    DialogHelpRegister(entryArea, "Address Entry");

	    {
		Widget submenu;
		prompter->menu = BuildSimpleMenu(entryArea, "", uicomp_flavor_names, XmMENU_OPTION, prompter, flavor_menu_apply);

#if XmVersion >= 1002
		XtUnmanageChild(XmOptionLabelGadget(prompter->menu));
#else /* Motif 1.1 */
		XtVaSetValues(XmOptionLabelGadget(prompter->menu),
			      XmNwidth, 0,
			      XmNheight, 0,
			      XmNmarginWidth, 0,
			      XmNmarginHeight, 0,
			      0);
#endif /* Motif 1.1 */
		XtVaSetValues(prompter->menu,
			      XmNtopAttachment, XmATTACH_FORM,
			      XmNleftAttachment, XmATTACH_FORM,
#if XmVersion < 1002
			      XmNspacing, 0,
#endif /* Motif 1.1 */
			      0);
		XtVaGetValues(prompter->menu, XmNsubMenuId, &submenu, 0);
		XtVaSetValues(submenu, XmNuserData, prompter, 0);
		XtManageChild(prompter->menu);
	    }

	    {
		prompter->field = XtVaCreateManagedWidget("field", xmTextWidgetClass, entryArea,
							  XmNtopAttachment, XmATTACH_FORM,
							  XmNleftAttachment, XmATTACH_WIDGET,
							  XmNleftWidget, prompter->menu,
							  XmNrightAttachment, XmATTACH_FORM,
							  0);
		XtAddCallback(prompter->field, XmNactivateCallback,	(XtCallbackProc) field_activate, prompter);
		XtAddCallback(prompter->field, XmNmodifyVerifyCallback,	(XtCallbackProc) field_dirty,	 prompter);
		XtAddCallback(prompter->field, XmNmodifyVerifyCallback, (XtCallbackProc) newln_cb, (XtPointer)True);
	    }

	    XtAddEventHandler(prompter->field, StructureNotifyMask, False, (XtEventHandler) center, prompter->menu);
	    XtAddEventHandler(prompter->menu,  StructureNotifyMask, False, (XtEventHandler) center, prompter->field);

	    LINEUP(prompter->menu, prompter, Entry);

	    XtManageChild(entryArea);
	}

	{
	    Widget listArea = XtVaCreateWidget("listing", xmFormWidgetClass, prompter->cookedArea,
					       XmNmarginWidth, 1,
					       XmNmarginHeight, 0,
					       XmNskipAdjust, False,
#ifdef SANE_WINDOW
					       ZmNextResizable, True,
					       ZmNhasSash, False,
#endif /* SANE_WINDOW */
					       0);
	    
	    {
		prompter->push = XtVaCreateWidget("push", xmFormWidgetClass, listArea, 0);
		{
		    Widget edit   = XtCreateManagedWidget("Edit",   xmPushButtonWidgetClass, prompter->push, NULL, 0);
		    Widget expand = XtCreateManagedWidget("Expand", xmPushButtonWidgetClass, prompter->push, NULL, 0);
		    Widget remove = XtCreateManagedWidget("Remove", xmPushButtonWidgetClass, prompter->push, NULL, 0);
		    XtAddCallback(edit,   XmNactivateCallback, (XtCallbackProc) list_edit,   prompter);
		    XtAddCallback(expand, XmNactivateCallback, (XtCallbackProc) list_expand, prompter);
		    XtAddCallback(remove, XmNactivateCallback, (XtCallbackProc) list_delete, prompter);
		}
		XtManageChild(prompter->push);
	    }
	    {
		Arg args[12];
		int argcount =
		XtVaSetArgs(args, XtNumber(args),
			    XmNselectionPolicy, XmEXTENDED_SELECT,
			    /* Scrollbar gets weird placement with this */
			    /* XmNlistSizePolicy,  XmRESIZE_IF_POSSIBLE, */
			    XmNresizable,	False,
			    XmNtopAttachment, XmATTACH_FORM,
			    XmNbottomAttachment, XmATTACH_FORM,
#if 0
			    XmNtopAttachment,	XmATTACH_OPPOSITE_WIDGET,
			    XmNtopWidget,	prompter->push,
			    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
			    XmNbottomWidget,	prompter->push,
#endif /* 0 */
			    XmNleftAttachment,	XmATTACH_WIDGET,
			    XmNleftWidget,	prompter->push,
			    XmNleftOffset,	1,
			    XmNrightAttachment, XmATTACH_FORM,
			    NULL);

		prompter->list = XmCreateScrolledList(listArea, "list", args, argcount);
		
		XtAddCallback(prompter->list, XmNextendedSelectionCallback, (XtCallbackProc) (address_edit ? list_select : list_select_noedit), prompter);
		XtAddCallback(prompter->list, XmNdefaultActionCallback,     (XtCallbackProc) list_edit,   prompter);
		ListInstallNavigator(prompter->list);

		XtManageChild(prompter->list);
	    }
	    LINEUP(prompter->push, prompter, Listing);
	    XtManageChild(listArea);
	}
	XtManageChild(prompter->cookedArea);
    }

    prompter->progressLast = progressLast;

    prompter->compose = 0;
    prompter->addressSync = prompter->subjectSync = prompter->editSync = 0;

    XtAddCallback(prompter->layout, XmNdestroyCallback, (XtCallbackProc) destroy, prompter);

    XtManageChild(prompter->layout);
    
    return prompter;
}


void
AddressAreaDestroy(prompter)
    struct AddressArea *prompter;
{
    if (prompter) {
	AddressAreaUse(prompter, NULL);
	free(prompter);
    }
}


#define RECALL(prompter, callback, name, type, function)  do { if (!((prompter)->callback)) ((prompter)->callback) = ZmCallbackAdd((name), (type), (function), (prompter)); } while (0)
#define UNCALL(callback)  do { if (callback) ZmCallbackRemove(callback); (callback) = 0; } while (0)

void
AddressAreaUse(prompter, compose)
    struct AddressArea *prompter;
    struct Compose *compose;
{
    if (prompter)
	if (compose) {
	    const Boolean fresh = !prompter->compose;
	    
	    prompter->compose = compose;
	    prompter->dirty = False;
	    prompter->dominant = uicomp_Unknown;
	    prompter->progress = True;
	    prompter->refresh = 0;

	    if (prompter->rawArea)
		XtSetSensitive(prompter->rawArea, True);
	    XtSetSensitive(prompter->cookedArea, True);
	    XtSetSensitive(prompter->subjectArea, True);
	    XtSetSensitive(prompter->layout, True);

	    RECALL(prompter, addressSync, "recipients",	ZCBTYPE_ADDRESS, address_changed);
	    RECALL(prompter, subjectSync, "subject",	ZCBTYPE_ADDRESS, subject_changed);
	    RECALL(prompter, editSync, "compose_state", ZCBTYPE_VAR,  edit_changed);
	    list_get_compose(prompter->list, prompter, fresh);
	    subject_get_compose(prompter);
	    
 	    if (edit_switch_modes(prompter, fresh))
 		start_textsw_edit(prompter->raw, &prompter->pos_flags);
 	    else
 		progress(prompter);
	    
	} else {
	    if (prompter->rawArea)
		XtSetSensitive(prompter->rawArea, False);
	    XtSetSensitive(prompter->cookedArea, False);
	    XtSetSensitive(prompter->subjectArea, False);
	    XtSetSensitive(prompter->layout, False);
	    UNCALL(prompter->addressSync);
	    UNCALL(prompter->subjectSync);
	    UNCALL(prompter->editSync);
	    if (prompter->refresh) {
		XtRemoveWorkProc(prompter->refresh);
		prompter->refresh = 0;
	    }
	    field_clear(prompter);
	    zmXmTextSetString(prompter->subject, NULL);
	    prompter->compose = 0;
	}
}

Widget
AddressAreaGetRaw(prompter)
    struct AddressArea *prompter;
{
    return prompter->raw;
}
