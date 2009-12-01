extern "C" {
#undef _NO_PROTO		// we need 'em
#include <X11/Intrinsic.h>
#include <Sgm/PrintBox.h>
#include <X11/Shell.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Text.h>
#include <spool.h>
#include <stdlib.h>
#include <string.h>
#include "dismiss.h"
#include "error.h"
#include "print.h"
#include "quote.h"
#include "setopts.h"
#include "uiprint.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmframe.h"
#include "zmopt.h"
#include "zmstring.h"
#include "zprint.h"
}
#include <dynstr.h>


struct PrintOptions {
  unsigned long headerType;
  Widget command;

  PrintOptions(const Widget);
};


PrintOptions::PrintOptions(const Widget command)
  : command(command)
{
  switch (uiprint_GetDefaultHdrType())
    {
    case uiprint_HdrStd:
      headerType = 0;
      break;
    case uiprint_HdrAll:
      headerType = 1;
      break;
    case uiprint_HdrNone:
      headerType = 2;
    }
}

static void
printer_error(Widget printBox, const char * const message)
{
  ask_item = printBox;
  error(SysErrWarning, message, SLErrorString(SLerrno));
}


static void
submit(Widget printBox, const PrintOptions * const options)
{
  ask_item = printBox;
  uiprint_t uprint;
  uiprint_Init(&uprint);

  switch (options->headerType)
    {
    case 0:
      uiprint_SetHdrType(&uprint, uiprint_HdrStd);
      break;
    case 1:
      uiprint_SetHdrType(&uprint, uiprint_HdrAll);
      break;
    case 2:
      uiprint_SetHdrType(&uprint, uiprint_HdrNone);
    }
  
  const char *printerName, *title, *special;
  Boolean copy, mail, message;
  int numCopies;
  XtVaGetValues(printBox,
		PuiNprinter, &printerName,
		PuiNcopy, &copy,
		PuiNjobTitle, &title,
		PuiNmail, &mail,
		PuiNmessage, &message,
		PuiNnumCopies, &numCopies,
		PuiNprinterOptions, &special,
		NULL);

  uiprint_SetPrinterName(&uprint, printerName);

  {
    char * const commandString = XmTextGetString(options->command);
    uiprint_SetPrintCmd(&uprint, commandString);
    XtFree(commandString);
  }

  {
    char *saved_printer_opt = value_of(VarPrinterOpt);
    if (saved_printer_opt) saved_printer_opt = savestr(saved_printer_opt);
  
    {
      ZDynstr switches;

      if (copy) switches += "-c ";
      if (title && *title) switches += zmVaStr("-t%s ", quotesh(title, 0, False));
      if (mail) switches += "-m ";
      if (message) switches += "-w ";
      if (numCopies > 1) switches += zmVaStr("-n%d ", numCopies);
      if (special && *special) switches += zmVaStr("-o%s ", quotesh(special, 0, False));
      switches += "-P";

      set_var(VarPrinterOpt, "=", switches);
    }

    zmBool successful = uiprint_Print(&uprint);
    uiprint_Destroy(&uprint);

    if (saved_printer_opt)
      {
	set_var(VarPrinterOpt, "=", saved_printer_opt);
	free(saved_printer_opt);
      }
    else
      un_set(&set_options, VarPrinterOpt);
  
    /* maybe print attachments? */
  
    if (successful)
      {
	Autodismiss(printBox, "print");
	DismissSetWidget(printBox, DismissClose);
      }
  }
}


static void
command_changed(Widget command)
{
  XmTextSetString(command, uiprint_GetPrintCmdInfo(NULL));
  XmTextSetCursorPosition(command, XmTextGetLastPosition(command));
}  


static void
listen(Widget, Widget command)
{
  command_changed(command);
  XtVaSetValues(command, XmNuserData, ZmCallbackAdd(VarPrintCmd, ZCBTYPE_VAR, (void (*)()) command_changed, command), 0);
}


static void
unlisten(Widget, Widget command)
{
  ZmCallback callback;
  XtVaGetValues(command, XmNuserData, &callback, 0);
  ZmCallbackRemove(callback);
}


extern "C" ZmFrame
DialogCreatePrinter(Widget parent, Widget item)
{
  Widget dialog = XtCreateWidget("printer_dialog", topLevelShellWidgetClass, parent, NULL, 0);
  Arg args[] = { { PuiNprintingPolicy, PuiAPPLICATION_PRINTING },
		 { PuiNjobType,	 PuiPRINTJOB_DESCRIPTOR } };
  Widget printBox = PuiCreatePrintBox(dialog, "printBox", args, XtNumber(args));

  ZmFrame frame = FrameCreate("printer_dialog", FramePrinter, parent,
			      FrameClass,	NULL,
			      FrameIcon,	&lpr_icon,
			      FrameChildClass,	NULL,
			      FrameChild,	&printBox,
			      FrameEndArgs);
    
  Widget options = XtVaCreateManagedWidget("options", xmFormWidgetClass, printBox, XmNorientation, XmHORIZONTAL, 0);

  Widget separator = XtVaCreateManagedWidget(NULL, xmSeparatorGadgetClass, options,
					     XmNbottomAttachment, XmATTACH_FORM,
					     XmNleftAttachment,	  XmATTACH_FORM,
					     XmNrightAttachment,  XmATTACH_FORM, NULL);

  Pixmap pixmap;
  FrameGet(frame, FrameIconPix, &pixmap, FrameEndArgs);
  Widget icon = XtVaCreateManagedWidget(lpr_icon.var, xmLabelWidgetClass, options,
					XmNlabelType,        XmPIXMAP,
					XmNlabelPixmap,      pixmap,
					XmNuserData,         &lpr_icon,
					XmNtopAttachment,    XmATTACH_FORM,
					XmNrightAttachment,  XmATTACH_FORM,
					XmNalignment,        XmALIGNMENT_END, NULL);
  FrameSet(frame,
	   FrameFlagOn,   FRAME_SHOW_ICON,
	   FrameIconItem, icon,
	   FrameEndArgs);

  Widget fields  = XtVaCreateManagedWidget("fields", xmRowColumnWidgetClass, options,
					   XmNtopAttachment,	XmATTACH_FORM,
					   XmNbottomAttachment,	XmATTACH_WIDGET,
					   XmNbottomWidget,	separator,
					   XmNleftAttachment,	XmATTACH_FORM, NULL);
  
  FrameCopyContext(FrameGetData(item), frame);
  FrameFolderLabelAdd( frame, fields, FRAME_SHOW_FOLDER);
  FrameMessageLabelAdd(frame, fields, FRAME_EDIT_LIST);
  
  Widget command = CreateLabeledText("command", fields, "Use command:", (int) CLT_HORIZ);
  XtAddCallback(dialog, XmNpopupCallback,   (XtCallbackProc)   listen, command);
  XtAddCallback(dialog, XmNpopdownCallback, (XtCallbackProc) unlisten, command);

  Widget dismissButton;
  XtVaGetValues(printBox, XmNcancelButton, &dismissButton, 0);
  FrameSet(frame, FrameDismissButton, dismissButton, FrameEndArgs);

  PrintOptions *state = new PrintOptions(command);
  Widget toggles = CreateToggleBox(options, False, False, True, NULL, &state->headerType, "print_message", print_header_choices, print_header_count);
  XtVaSetValues(toggles,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_WIDGET,
		XmNbottomWidget,	separator,
		XmNrightAttachment,	XmATTACH_WIDGET,
		XmNrightWidget,		icon, NULL);

  XtManageChild(command);
  XtManageChild(toggles);
  XtManageChild(printBox);
  
  XtAddCallback(printBox, PuiNcancelCallback, (XtCallbackProc) PopdownFrameCallback, NULL);
  XtAddCallback(printBox, PuiNerrorCallback, (XtCallbackProc) printer_error, catgets(catalog, CAT_MOTIF, 892, "Unable to submit print job: %s"));
  XtAddCallback(printBox, PuiNoptionErrorCallback, (XtCallbackProc) printer_error, catgets(catalog, CAT_MOTIF, 893, "Unable to use printer option panel: %s"));
  XtAddCallback(printBox, PuiNprintCallback, (XtCallbackProc) submit, state);
  XtAddCallback(printBox, XmNdestroyCallback, (XtCallbackProc) free_user_data, state);
  
  return frame;
}
