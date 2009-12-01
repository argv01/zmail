/*
 * Copyright (c) 1993-1998 Z-Code Software Corp.
 */

#ifndef PLIST_H
#define PLIST_H

#include <glist.h>

#define plist_EmptyP(pl) glist_EmptyP(&(pl)->proplist)

#define plist_FOREACH(p,v,i) \
  glist_FOREACH(&(p)->proplist,struct plist_prop,v,i)

struct plist {
  struct glist proplist;  /* glist of properties */
  int sorted;             /* whether or not the glist is sorted */
};


struct plist_prop {
  char *name;          /* property name */
  char *value;         /* property value */
};


extern struct plist *plist_New();
extern void plist_Add();
extern char *plist_Get();
extern void plist_Free();

#endif /* PLIST_H */

