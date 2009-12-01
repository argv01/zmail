// -*- c++ -*-
#pragma once


extern "C" {
#include "attach.h"
#include "callback.h"
#include "frtype.h"
#include "zmframe.h"
#include <X11/Intrinsic.h>
}
#include <oz/AdvPanel.h>
#include <oz/SelectionList.h>
#include <oz/View.h>

class AppEvent;
struct Attach;
class ostream;


class AttachPanel : public AdvPanel
{
public:
  // Ashes to ashes, dust to dust
  AttachPanel(Widget, const char * const);
  virtual ~AttachPanel();
  
  // Show icons for a given set of attachments
  virtual void drawIcons(ZmFrame) = 0;

protected:
  // Show icons for a given set of attachments
  void drawIcons(unsigned = 0);

  // Menu creation
  Widget addButton(const char * const, const XtCallbackProc);
  Widget addSeparator();

  // Visibility management
  Boolean suppressRedraw;

  // Display things
  static Boolean displayHandler(const void * const, AttachPanel * const);
  void displaySelected();
  virtual int displayPart(const unsigned) = 0;

  // Print things
  static void printHandler(const void * const, AttachPanel * const);
  void printSelected();
  virtual int printPart(const unsigned, const char * const) = 0;
  static int printFile(const char * const, const unsigned, const char * const);
  
  // Drag things
  static Boolean dragHandler(const void * const, AttachPanel * const, AppEvent * const);
  void dragSelected(AppEvent * const);
  virtual int dragPart(const unsigned, char *) = 0;

  // Notice changes in attachment variables or types database
  struct {
    ZmCallback variable;
    ZmCallback attach;
  } callbacks;

  // Sibling label widget
  Widget label;

  // Child popup menu
  Widget popup;
  inline virtual void setSensitivities();

  // Convenient shorthand
  virtual Attach *attachments() = 0;
  inline Attach *attachment(const unsigned);
  inline Attach *attachment(View &);

private:
  static void selectHandler(const void * const, AttachPanel * const);
  static void deallocate(Widget, AttachPanel * const);
};


void
AttachPanel::setSensitivities()
{
  XtSetSensitive(popup, !!getSelectionList()->getLength());
}


Attach *
AttachPanel::attachment(const unsigned number)
{
  return lookup_part(attachments(), number, 0);
}

Attach *
AttachPanel::attachment(View &view)
{
  return attachment((unsigned) view.getUserData());
}
