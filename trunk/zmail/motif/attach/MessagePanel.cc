extern "C" {
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include "getpath.h"
#include "file.h"
#include "error.h"
#include "fsfix.h"
#include "msgs/prune.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmflag.h"
#include "zmstring.h"
#include "zprint.h"
}

#include <oz/AppEvent.h>
#include <oz/FileCategory.h>
#include <oz/SelectionList.h>
#include <oz/View.h>
#include <strstream.h>
#include "AttachPanel.h"
#include "MessagePanel.h"


MessagePanel::MessagePanel(Widget parent)
  : AttachPanel(parent, VarMsgAttachLabel)
{
  setDropable(False);
  setDragable(True);

  addButton("Show",	(XtCallbackProc) displayHandler);
  addButton("Save",	(XtCallbackProc) saveHandler);
  addButton("Print",	(XtCallbackProc) printHandler);
  XtUnmanageChild(separator	 = addSeparator());
  XtUnmanageChild(deleteButton	 = addButton("Delete",   (XtCallbackProc) deleteHandler));
  XtUnmanageChild(undeleteButton = addButton("Undelete", (XtCallbackProc) undeleteHandler));

  pruneCallback = ZmCallbackAdd("", ZCBTYPE_PRUNE, (void (*)()) pruneHandler, this);
}


void
MessagePanel::saveHandler(Widget, MessagePanel * const panel)
{
  panel->saveSelected();
}


void
MessagePanel::saveSelected()
{
  const unsigned long flags = PB_MUST_MATCH | PB_FILE_BOX | PB_FILE_WRITE;

  ask_item = getWidget();
  char *response;
  
  if (getSelectionList()->getLength() == 1)
    {
      unsigned partNumber = (unsigned) getSelectionList()->getFirstView()->getUserData();
      Attach &part = *attachment(partNumber);
      char buffer[MAXPATHLEN];
      char *fullname;

      if (part.a_name)
	fullname = part.a_name;
      else
	{
	  fullname = buffer;
	  dgetstat(get_detach_dir(), part.content_name, fullname, NULL);
	}
      
      response = PromptBox(getWidget(),
			   catgets(catalog, CAT_MOTIF, 913, "\
Double click on a directory in the list below to open the one in\n\
which you want to save the attached file. Enter the name of the\n\
attached file in the \"File\" field, then click on [Save]."),
			   fullname, NULL, 0, flags | PB_NOT_A_DIR, NULL);

      if (response)
	{
	  current_folder = hostFolder;
	  detach_parts(hostMessage->this_msg_no, partNumber, NULL, response, NULL, NULL, DetachSave);
	}
    }
  else
    {
      response = PromptBox(getWidget(),
			   catgets(catalog, CAT_MOTIF, 899, "Save selected attachments into which directory?"),
			   get_detach_dir(), NULL, 0, flags | PB_MUST_EXIST, NULL);

      if (response)
	{
	  int isDir = ZmGP_IgnoreNoEnt;
	  char *directory = getpath(response, &isDir);
      
	  switch (isDir)
	    {
	    case ZmGP_File:
	      {
		char *trim = last_dsep(directory);
		if (trim)
		  *trim = '\0';
		else
		  break;
	      }
	    case ZmGP_Dir:
	      {
		char fullname[MAXPATHLEN];
		ask_item = getWidget();
		int status = 0;
		for (register View *view = getSelectionList()->getFirstView(); view && !status; view = getSelectionList()->getNextView())
		  {
		    flashChildView(view);
		    current_folder = hostFolder;
		    Attach &part = *attachment(*view);
		    dgetstat(directory, part.content_name, fullname, NULL);
		    status = detach_parts(hostMessage->this_msg_no, (unsigned) view->getUserData(), NULL, fullname, NULL, NULL, DetachSave);
		  }
	      }
	    }
	}
    }
  if (response) XtFree(response);
}


int
MessagePanel::displayPart(const unsigned partNumber)
{
  current_folder = hostFolder;
  return detach_parts(hostMessage->this_msg_no, partNumber, NULL, NULL, NULL, NULL, DetachDisplay);
}


int
MessagePanel::printPart(const unsigned partNumber, const char * const printer)
{
  current_folder = hostFolder;
  const Attach &part = *attachment(partNumber);
  char fullname[MAXPATHLEN];

  dgetstat(get_detach_dir(), part.content_name, fullname, NULL);
  int status = detach_parts(hostMessage->this_msg_no, partNumber, NULL, fullname, NULL, NULL, 0);
  
  return status ? status : printFile(fullname, partNumber, printer);
}


int
MessagePanel::dragPart(const unsigned partNumber, char *proxyName)
{
  ask_item = getWidget();
  current_folder = hostFolder;
  int status = 0;

  status = dgetstat(get_detach_dir(), attachment(partNumber)->content_name, proxyName, NULL);
  if (!status)
    status = detach_parts(hostMessage->this_msg_no, partNumber, NULL,
			  proxyName, NULL, NULL, DetachOverwrite | DetachSave);

  return status;
}


void
MessagePanel::drawIcons(ZmFrame frame)
{
  FrameGet(frame,
	   FrameClientData, &hostMessage,
	   FrameFolder, &hostFolder,
	   FrameEndArgs);
  
  SAVE_RESIZE(GetTopShell(getTopWidget()));
  SET_RESIZE(False);
  
  if (!can_show_inline(attachments()) && chk_option(VarMessagePanes, "attachments"))
    {
      AttachPanel::drawIcons();
      hdr_form_set_width(hostMessage->hdr_fmt_w, 7);
      XtManageChild(getTopWidget());
    }
  else
    {
      XtUnmanageChild(getTopWidget());
      hdr_form_set_width(hostMessage->hdr_fmt_w, 12);
    }
  
  RESTORE_RESIZE();
}


void
MessagePanel::setSensitivities()
{
  Boolean deletables = 0;
  Boolean undeleteables = 0;

  for (register View *view = getSelectionList()->getFirstView(); view && !(deletables && undeleteables); view = getSelectionList()->getNextView())
    {
      Attach *part = attachment(*view);

      if (!pruned(part))
	(ison(part->a_flags, AT_DELETE) ? undeleteables : deletables) = 1;
    }

  {
    void (*remanage[2])(Widget) = { XtUnmanageChild, XtManageChild };
  
    (remanage[deletables || undeleteables])(separator);
    (remanage[deletables])(deleteButton);
    (remanage[undeleteables])(undeleteButton);
  }

  const Boolean prunable = isoff(hostFolder->mf_flags, READ_ONLY) && MsgIsMime(&hostMessage->this_msg);
  XtSetSensitive(  deleteButton, prunable);
  XtSetSensitive(undeleteButton, prunable);
  
  AttachPanel::setSensitivities();
}


void
MessagePanel::deleteHandler(Widget, MessagePanel * const panel)
{
  panel->deleteSelected();
}


 
void
MessagePanel::deleteSelected()
{
  ZmCallbackRemove(pruneCallback);

  for (register View *view = getSelectionList()->getFirstView(); view; view = getSelectionList()->getNextView())
    {
      struct Attach * const part = attachment(*view);
      prune_part_delete(hostFolder, &hostMessage->this_msg, part);
    }

  pruneCallback = ZmCallbackAdd("", ZCBTYPE_PRUNE, (void (*)()) pruneHandler, this);
  redrawIcons();
}


void
MessagePanel::undeleteHandler(Widget, MessagePanel * const panel)
{
  panel->undeleteSelected();
}


void
MessagePanel::undeleteSelected()
{
  ZmCallbackRemove(pruneCallback);

  for (register View *view = getSelectionList()->getFirstView(); view; view = getSelectionList()->getNextView())
    {
      struct Attach * const part = attachment(*view);
      prune_part_undelete(hostFolder, &hostMessage->this_msg, part);
    }

  pruneCallback = ZmCallbackAdd("", ZCBTYPE_PRUNE, (void (*)()) pruneHandler, this);
  redrawIcons();
}


void
MessagePanel::pruneHandler(MessagePanel * const panel, struct zmCallbackData *)
{
  panel->redrawIcons();
}


void
MessagePanel::redrawIcons()
{
  unsigned *selections = getSelectionList()->getLength() ? new unsigned[getSelectionList()->getLength()] : 0;
  unsigned append = 0;

  if (selections)
    for (register View *view = getSelectionList()->getFirstView(); view; view = getSelectionList()->getNextView())
      selections[append++] = (unsigned) view->getUserData() - 1;

  AttachPanel::drawIcons();
  
  if (selections)
    {
      while (append--)
	addSelection(getView(selections[append]));

      delete[] selections;
      render();
    }
}
