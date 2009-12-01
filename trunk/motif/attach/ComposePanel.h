// -*- c++ -*-
#pragma once


extern "C" {
#include <X11/Intrinsic.h>
#include "zmcomp.h"
}
#include "AttachPanel.h"


class ComposePanel : public AttachPanel
{
public:
  // Ashes to ashes, dust to dust
  ComposePanel(Widget);

  // Attach a set of dropped icons to a composition
  void attachIcons(class DropInfo &);
  
  // Show icons for a given frame
  virtual void drawIcons(ZmFrame);
  
protected:
  // Refresh without losing selection set
  inline void redrawIcons();

  // Convenient shorthand
  inline virtual Attach *attachments();

  // Common manipulations
  virtual int displayPart(const unsigned);
  virtual int   printPart(const unsigned, const char * const);
  virtual int    dragPart(const unsigned, char *);

  // Attach & unattach things
  static Boolean dropHandler(const void * const, ComposePanel * const, const AppEvent * const);
  static void unattachHandler(Widget, ComposePanel * const);
  void unattachSelected();

  // Affiliations
  Compose *hostCompose;
};


void
ComposePanel::redrawIcons()
{
  AttachPanel::drawIcons(1);
}


Attach *
ComposePanel::attachments()
{
  return hostCompose->attachments;
}
