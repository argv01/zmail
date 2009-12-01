// -*- c++ -*-
#pragma once


extern "C" {
#include <X11/Intrinsic.h>
#include "attach.h"
#include "../m_msg.h"
}
#include "AttachPanel.h"


struct Attach;
struct mfolder;
struct _msg_data;


class MessagePanel : public AttachPanel
{
public:
  // Ashes to ashes, dust to dust
  MessagePanel(Widget);
  
  // Show icons for a given frame
  virtual void drawIcons(ZmFrame);

protected:
  // Child popup menu management
  virtual void setSensitivities();
  
  // Refresh without losing selection set
  void redrawIcons();

  // Convenient shorthand
  inline virtual Attach *attachments();

  // Common manipulations
  virtual int displayPart(const unsigned);
  virtual int   printPart(const unsigned, const char * const);
  virtual int    dragPart(const unsigned, char *);

  // Delete and undelete things
  static void   deleteHandler(Widget, MessagePanel * const);
  static void undeleteHandler(Widget, MessagePanel * const);
  void   deleteSelected();
  void undeleteSelected();

  // Save things
  static void saveHandler(Widget, MessagePanel * const);
  void saveSelected();

  // Affiliations
  struct mfolder *hostFolder;
  struct _msg_data *hostMessage;

  // Keep current on deleted parts
  ZmCallback pruneCallback;
  static void pruneHandler(MessagePanel * const, struct zmCallbackData *);

private:
  Widget separator;
  Widget deleteButton;
  Widget undeleteButton;
};


Attach *
MessagePanel::attachments()
{
  return hostMessage->this_msg.m_attach;
}
