/*
 * Copyright (c) 1993-1998 NetManage, Inc.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include "popper.h"
#include <stdio.h>
#include <ctype.h>

#define ATTR_DELIMITER "-%-"
#define ERR_GLIST_GROWSIZE 5


int parseAttrLine(line, attrPlist, errGlist)
     struct plist *attrPlist;
     struct glist *errGlist;
     char *line;
{
  char *start, *end;
  char *idStart, *idEnd;
  char *valStart, *valEnd;
  char *errstr;
  char strbuf[MAXLINELEN];

  if (((start = strstr(line, ATTR_DELIMITER)) == NULL)
      || ((end = strstr(start+=3, ATTR_DELIMITER)) == NULL)) {
    if (errGlist != NULL) {
      errstr = "No attribute line.";
      glist_Add(errGlist, &errstr);
    }
  } else {
    *end = '\0';
    for(idStart=strtok(start, ";"); idStart!=NULL; idStart=strtok(NULL, ";")) {

      /* Trim whitespace from start of identifier */
      idStart+=strspn(idStart, " \t");
      if (*idStart=='\0') continue;

      /* Split attribute into identifier and value */
      if ((idEnd = strpbrk(idStart, ":")) == NULL) {
	idEnd = idStart+strlen(idStart);
	valStart = idEnd;
      } else {
	*idEnd='\0';
	valStart = idEnd+1;
      }

      /* Trim whitespace from end of value */
      for(valEnd=valStart+strlen(valStart)-1; 
	  (valEnd>valStart) && index(" \t", *valEnd);
	  *valEnd-- = '\0');
	
      /* Trim whitespace from end of identifier */
      while((idEnd>idStart) && index(" \t", *--idEnd)) *idEnd='\0';
      if (idEnd == idStart) {
	if (errGlist != NULL) {
	  sprintf(strbuf, "Malformed attribute (null identifier): :%s", 
		  valStart);
	  errstr = strdup(strbuf);
	  glist_Add(errGlist, &errstr);
	}
	continue;
      }

      /* Trim whitespace from start of value */
      valStart+=strspn(valStart, " \t");
      
      /* Whitespace not allowed within identifiers */
      if (strpbrk(idStart, " \t") != NULL) {
	if (errGlist != NULL) {
	  sprintf(strbuf, "Malformed attribute (illegal identifier): %s",
		  idStart);
	  errstr = strdup(strbuf);
	  glist_Add(errGlist, &errstr);
	}
	continue;
      }

      /* Upcase identifier */
      for (idEnd=idStart; *idEnd!='\0'; idEnd++)
	*idEnd=toupper(*idEnd);

      plist_Add(attrPlist, idStart, valStart);
    }
  }

  if (!plist_EmptyP(attrPlist))
    plist_Sort(attrPlist);
  return ((errGlist == NULL) || glist_EmptyP(errGlist)) ? 0 : -1;
}


struct glist *newErrGlist()
{
  struct glist *errGlist = (struct glist *)malloc(sizeof(struct glist));
  glist_Init(errGlist, sizeof(char *), ERR_GLIST_GROWSIZE);
  return errGlist;
}


void freeErrGlist(errGlist)
     struct glist *errGlist;
{
  int i;
  char **errstrptr;

  for(i=glist_Length(errGlist)-1; i>=0; i--) {
    errstrptr = glist_Nth(errGlist, i);
    free(*errstrptr);
    glist_Pop(errGlist);
  }
}


int cmpVersion(alpha, beta)
     char *alpha;
     char *beta;
{
  return -1;
}


/*
 * main()
 * {
 *   struct glist *errGlist = newErrGlist();
 *   struct plist *attrPlist = plist_New(ATTR_PLIST_GROWSIZE);
 *   char line[256];
 *   int i;
 *   char **errstrptr;
 *   struct plist_prop *propptr;
 * 
 *   while (gets(line) != NULL) {
 *     if (parseAttrLine(line, attrPlist, errGlist)) {
 *       puts("Errors:");
 *       glist_FOREACH(errGlist, char *, errstrptr, i)
 * 	printf("  %s\n", *errstrptr);
 *       freeErrGlist(errGlist);
 *     }
 *     puts("Attributes:");
 *     plist_FOREACH(attrPlist, propptr, i)
 *       printf("  %s: '%s'\n", propptr->name, propptr->value);
 *     plist_Free(attrPlist);
 *   }
 * }
 */
