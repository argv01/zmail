#undef _NO_PROTO

extern "C" {
#include "../m_menus.h"
#include "../m_msg.h"
#include "area.h"
#include "msgs/autotype/oz.h"
#include "pager.h"
#include "sounds.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include "zmalloc.h"
#include "zmstring.h"
#include "zprint.h"
#include <X11/Intrinsic.h>
#include <Xm/Form.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/SeparatoG.h>
#include <Xm/Xm.h>
#include <spool.h>

  void timeout_cursors(int);	// from zmail.h
}
#include <oz/AppEvent.h>
#include <oz/IconLayout.h>
#include <oz/IconTypeCategory.h>
#include <oz/SelectionList.h>
#include <oz/FileCategory.h>
#include <strstream.h>
#include "AttachPanel.h"
#include "SorterPart.h"


AttachPanel::AttachPanel(Widget parent, const char * const labelVar)
  : AdvPanel(parent, NULL, 0, 90),
    suppressRedraw(False)
{
  callbacks.attach = ZmCallbackAdd("", ZCBTYPE_ATTACH, (void (*)()) attach_rehash_cb, this);
  callbacks.variable = ZmCallbackAdd(labelVar, ZCBTYPE_VAR, (void (*)()) attach_rehash_cb, this);

  setRenameable(False);
  displayFrame(True);

  registerCategory(IconTypeCategoryName);
  // IconTypeCategory::setCtrFilename(OZ_DATABASE);	// XXX buggy
  registerCategory(FileCategoryName);			// XXX workaround

  {
    IconLayout *layout = viewAsIcons();
    
    layout->setSorter(&partSorter);
    layout->setAutoRelayout(True);
    layout->setStaggeredIndent(0);
  }
    
  addCmdCallback(AppEvent::openName, (CmdCallback) displayHandler, this);
  addCmdCallback(AppEvent::initiateDropName, (CmdCallback) dragHandler, this);
  
  instantiate();

  if (getPanelFrame())
      label = XtVaCreateManagedWidget("attachment_area_label",
				      xmLabelGadgetClass,	getPanelFrame(),
				      XmNchildType,		XmFRAME_TITLE_CHILD,
				      NULL);
  
  Arg args[1];
  XtSetArg(args[0], XmNuserData, this);
  popup = XmCreatePopupMenu(getWidget(), ICON_AREA_NAME, args, XtNumber(args));

  XtSetSensitive(popup, False);
  XtAddEventHandler(getWidget(), ButtonPressMask, False, (XtEventHandler) PostIt, popup);
  addPostCmdCallback(AppEvent::selectName, (CmdCallback) selectHandler, this);
  addPostCmdCallback(AppEvent::adjustName, (CmdCallback) selectHandler, this);

  XtAddCallback(getTopWidget(), XmNdestroyCallback, (XtCallbackProc) deallocate, this);

  XtVaSetValues(getTopWidget(),
		XmNtopAttachment,    XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment,   XmATTACH_POSITION,
		XmNleftPosition,     7,
		XmNrightAttachment,  XmATTACH_FORM,
		0);
}


AttachPanel::~AttachPanel()
{
  ZmCallbackRemove(callbacks.variable);
  ZmCallbackRemove(callbacks.attach);
}


void
AttachPanel::deallocate(Widget, AttachPanel * const chaff)
{
  delete chaff;
}


void
AttachPanel::drawIcons(unsigned offset)
{
  Attach *parts = attachments();
  if (suppressRedraw) return;
  if (!XtIsRealized(getWidget())) return;
  
  suppressRedraw = True;

  unsigned m = is_multipart(parts);
  if (label) XtVaSetValues(label, XmNlabelString,
			   zmXmStr(zmVaStr(catgets(catalog, CAT_MOTIF, 915, "Attachments: %d"),
					   number_of_links(parts)-m)), NULL);
  
  clearIcons();
  
  if (parts)
    {      
      Attach *part;
      for (register unsigned number = 1; part = (Attach *)retrieve_nth_link((struct link *)parts, m+1); number++, m++)
	{
	  // The thing with zmVaStr is to force all icons to have unique names.
	  // Only the part after the slash is actually used to choose the screen image.
	  char *name = zmVaStr("%d/%s", number, guess_desk_type(*part));
	  View *view = addIcon(name, IconTypeCategoryName);
	  // XXX casting away const
	  view->setDisplayName((char *)get_attach_label(part, offset));
	  view->setUserData((void *)number); // to be used later in a call to "detach"
	}
    }

  relayoutAndRender();
  setSensitivities();

  suppressRedraw = False;
}


Boolean
AttachPanel::displayHandler(const void * const, AttachPanel * const panel)
{
  timeout_cursors(True);
  panel->displaySelected();
  timeout_cursors(False);
  return True;
}


void
AttachPanel::displaySelected()
{
  if (getSelectionList()->getLength())
    {
      ask_item = getWidget();
      int status = 0;
      suppressRedraw = True;

      Boolean playedSound = False;
      for (register View *view = getSelectionList()->getFirstView(); view && !status; view = getSelectionList()->getNextView())
	{
	  if (!playedSound && fiGetBooleanVariableValue(view->getIconData()->getType(), "noLaunchSound") != True)
	    {
	      playSound(SS_LAUNCH);
	      playedSound = True;
	    }

	  flashChildView(view);
	  status = displayPart((unsigned) view->getUserData());
	}

      suppressRedraw = False;
    }
}


void
AttachPanel::selectHandler(const void * const, AttachPanel * const panel)
{
  panel->setSensitivities();
}


void
AttachPanel::printHandler(const void * const, AttachPanel * const panel)
{
  timeout_cursors(True);
  panel->printSelected();
  timeout_cursors(False); 
}


void
AttachPanel::printSelected()
{
  ask_item = getWidget();
  char * const printer = printer_choose_one(0);

  if (printer)
    {
      int status = 0;
      suppressRedraw = True;

      for (register View *view = getSelectionList()->getFirstView();
	   view && !status; view = getSelectionList()->getNextView())
	{
	  flashChildView(view);
	  status = printPart((unsigned) view->getUserData(), printer);
	}
      
      suppressRedraw = False;
      free(printer);
    }
}


int
AttachPanel::printFile(const char * const filename, const unsigned partNumber, const char * const printer)
{
  SLPrintJob *job = SLSubmitJob(filename, printer, 1, 0, 0, 0, 0);

  if (job)
    wprint(catgets(catalog, CAT_MOTIF, 896, "Printing attachment %u as job %s\n"), partNumber, job->job_id);
  else
    {
      ZmPager pager = ZmPagerStart(PgText);
      ZmPagerSetTitle(pager, catgets(catalog, CAT_MOTIF, 897, "Printing Errors"));
      ZmPagerWrite(pager, zmVaStr(catgets(catalog, CAT_MOTIF, 898, "Could not print attachment %u: %s\n\n"), partNumber, SLErrorString(SLerrno)));

      int outputCount;
      char **output;
      
      if (SLGetSpoolerError(&output, &outputCount))
	for (unsigned scan = 0; scan < outputCount; scan++)
	  {
	    ZmPagerWrite(pager, output[scan]);
	    ZmPagerWrite(pager, "\n");
	  }
      
      ZmPagerStop(pager);
    }

  return !job;
}


Boolean
AttachPanel::dragHandler(const void * const, AttachPanel * const panel, AppEvent * const event)
{
  panel->dragSelected(event);
  return True;
}


void
AttachPanel::dragSelected(AppEvent * const event)
{
  ostrstream target;
  FileCategory * const files = (FileCategory * const) getCategory(FileCategoryName);
  
  for (register View *view = getSelectionList()->getFirstView();
       view; view = getSelectionList()->getNextView())
    {
      char proxyName[MAXPATHLEN];
      if (dragPart((unsigned) view->getUserData(), proxyName))
	break;
      
      IconData *proxy = files->createNamedIconData(proxyName);
      if (!proxy) break;
      
      char single[2 * MAXPATHLEN];
      proxy->getTargetData(event->targetName, single);
      target << single << ends;
      
      delete proxy;
    }
  
  target << ends;
  event->targetLength = target.pcount();
  event->targetBuf    = target.str();
}


Widget
AttachPanel::addButton(const char * const name, const XtCallbackProc handler)
{
  Widget button = XtCreateManagedWidget(name, xmPushButtonWidgetClass, popup, NULL, 0);
  XtAddCallback(button, XmNactivateCallback, (XtCallbackProc) handler, this);

  return button;
}


Widget
AttachPanel::addSeparator()
{
  Widget button = XtCreateManagedWidget(0, xmSeparatorGadgetClass, popup, NULL, 0);

  return button;
}
