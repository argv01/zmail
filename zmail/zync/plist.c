/*
 * Copyright (c) Copyright (c) 1993-1998 NetManage, Inc. 
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
static char plist_rcsid[] = "$Id: plist.c,v 1.8 1998/12/07 23:57:24 schaefer Exp $";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif /* HAVE_MALLOC_H */

#include "plist.h"

/* Avoid <string.h>/<strings.h> controversy... */
extern char *strdup();

struct plist *plist_New(growsize)
     int growsize;
{
  struct plist *plist;

  plist = (struct plist *)malloc(sizeof(struct plist));
  glist_Init(&plist->proplist, sizeof(struct plist_prop), growsize);
  plist->sorted = -1;
  return plist;
}
  

void plist_Add(plist, name, value)
     struct plist *plist;
     char *name;
     char *value;
{     
  struct plist_prop prop;

  prop.name = strdup(name);
  prop.value = strdup(value);
  glist_Add(&plist->proplist, &prop);
  plist->sorted = 0;
}


int plist_prop_cmp(prop1, prop2)
     CVPTR prop1;
     CVPTR prop2;
{
  return strcmp(((struct plist_prop *)prop1)->name,
		((struct plist_prop *)prop2)->name);
}


void plist_Sort(plist)
     struct plist *plist;
{
  if (plist->sorted == 0) {
    glist_Sort(&plist->proplist, plist_prop_cmp);
    plist->sorted = -1;
  }
}      


char *
plist_Get(plist, name)
     struct plist *plist;
     char *name;
{
  struct plist_prop probe;
  int result;
  
  if (plist->sorted == 0)
    plist_Sort(plist);
  probe.name = name;
  result = glist_Bsearch(&plist->proplist, &probe, plist_prop_cmp);
  return ((result == -1) ? 0 :
	  ((struct plist_prop *)
	   glist_Nth(&plist->proplist, result))->value);
}
    

void plist_Free(plist)
     struct plist *plist;
{
  int i;
  struct plist_prop *prop;

  for(i=glist_Length(&plist->proplist)-1; i>=0; i--) {
    prop = glist_Nth(&plist->proplist, i);
    free(prop->name);
    free(prop->value);
    glist_Pop(&plist->proplist);
  }
}


