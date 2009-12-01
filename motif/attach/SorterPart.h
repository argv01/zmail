#pragma once

#include <oz/Sorter.h>

class View;


class SorterPart : public Sorter
{
public:
  virtual char *getClassName();
  virtual int compare(View *, View *);

private:
  static char *className;
};

extern SorterPart partSorter;
