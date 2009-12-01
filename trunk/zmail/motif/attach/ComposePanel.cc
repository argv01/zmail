extern "C" {
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#include <stdio.h>
#include "../addressArea/addressArea.h"
#include "../m_comp.h"
#include "../m_msg.h"
#include "attach.h"
#include "error.h"
#include "gui_def.h"
#include "msgs/autotype/oz.h"
#include "shell/file.h"
#include "sounds.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include "zmstring.h"
#include "zprint.h"

  void timeout_cursors(int);	// from zmail.h
}

#include <oz/AppEvent.h>
#include <oz/DropInfo.h>
#include <oz/FileCategory.h>
#include <oz/FileIconData.h>
#include <oz/SelectionList.h>
#include <oz/View.h>
#include <strstream.h>
#include "AttachPanel.h"
#include "ComposePanel.h"


ComposePanel::ComposePanel(Widget parent)
  : AttachPanel(parent, VarCompAttachLabel)
{
  setDragable(True);
  setDropable(True);
  addCmdCallback(AppEvent::dropName, (CmdCallback) dropHandler, this);

  addButton("Show",	(XtCallbackProc) displayHandler);
  addButton("Unattach",	(XtCallbackProc) unattachHandler);
  addButton("Print",	(XtCallbackProc) printHandler);
}


void
ComposePanel::attachIcons(DropInfo &drops)
{
  timeout_cursors(True);
  suppressRedraw = True;
  ask_item = getWidget();
  
  Boolean playedSound = False;
  for (register View *view = drops.selectionList.getFirstView(); view; view = drops.selectionList.getNextView())
    {
      FID * const fileData = view->getIconData()->getFID();
      if (fileData)
	{
	  struct Attach *part;
	  const char * const message = add_attachment(hostCompose, fileData->getMountedName(), NULL, NULL, NULL, 0, &part);
	  if (message)
	    error(UserErrWarning, message);
	  else
	    {
	      if (!playedSound)
		{
		  playSound(SS_DROP_REFERENCE);
		  playedSound = True;
		}
	      // preserve icon
	      AddContentParameter(&part->content_params, IrixTypeParam, fileData->getTypeName());
	    }
	}
    }
  suppressRedraw = False;
  redrawIcons();
  timeout_cursors(False);
}


void
ComposePanel::unattachSelected()
{
  Attach ** const chaff = new (Attach * [getSelectionList()->getLength()]);
  unsigned append = 0;
  
  for (register View *view = getSelectionList()->getFirstView(); view; view = getSelectionList()->getNextView(), append++)
    chaff[append] = attachment(*view);

  while (append--)
    {
      remove_link(&hostCompose->attachments, chaff[append]);
      free_attach(chaff[append], TRUE);
    }

  delete[] chaff;

  redrawIcons();
  if (hostCompose->interface->attach_info)
    fill_attach_list_w(hostCompose->interface->attach_info->list_w, attachments(), FrameCompose);
  ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
}


Boolean
ComposePanel::dropHandler(const void * const, ComposePanel * const panel, const AppEvent * const event)
{
  panel->attachIcons(*((class DropInfo *) event->callData));
  return True;
}


void
ComposePanel::unattachHandler(Widget, ComposePanel * const panel)
{
  panel->unattachSelected();
}


int
ComposePanel::printPart(const unsigned partNumber, const char * const printer)
{
  Attach &part = *attachment(partNumber);
  char * const name = part.a_name;

  return printFile(name, partNumber, printer);
}


int
ComposePanel::displayPart(const unsigned int partNumber)
{
  AttachProg *viewer;
  char *errors = 0;

  fclose(open_tempfile("err", &errors)); // safe even if open_tempfile() returns NULL

  popen_coder(viewer = coder_prog(FALSE, attachment(partNumber), NULL, NULL, "x", FALSE), NULL, errors, "x");
  int status = handle_coder_err(viewer->exitStatus, viewer->program, errors);
  unlink(errors);
  free(errors);

  return status;
}


int
ComposePanel::dragPart(const unsigned partNumber, char *proxyName)
{
  proxyName[MAXPATHLEN-1] = '\0';
  strncpy(proxyName, attachment(partNumber)->a_name, MAXPATHLEN);
  return proxyName[MAXPATHLEN-1] != '\0';
}


void
ComposePanel::drawIcons(ZmFrame frame)
{
  if (FrameGetFreeClient(frame))
    {
      hostCompose = FrameComposeGetComp(frame);
      
      SAVE_RESIZE(GetTopShell(getTopWidget()));
      SET_RESIZE(False);
  
      if (chk_option(VarComposePanes, "attachments"))
	{
	  redrawIcons();
	  AddressAreaSetWidth(hostCompose->interface->prompter, 7);
	  XtManageChild(getTopWidget());
	}
      else
	{
	  XtUnmanageChild(getTopWidget());
	  AddressAreaSetWidth(hostCompose->interface->prompter, 12);
	}

        
      RESTORE_RESIZE();
    }
}
