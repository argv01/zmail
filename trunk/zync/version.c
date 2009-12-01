/*
 * Copyright (c) 1993-1998 NetManage, Inc. 
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1993-1998 NetManage, Inc.\n";
#endif /* not lint */

#include <stdio.h>
#include <general/general.h>
#include "version.h"

/* Avoid <string.h>/<strings.h> controversy... */
extern char *strdup();

int parseVersion(string, version)
     char *string;
     struct version *version;
{
  char *start, *end;
  int majr = 0;
  int minr = 0;
  char levstr[4];
  int patch = 0;
  int i;

  levstr[0]='\0';

  majr = (int)strtol(string, &end, 10);    /* try for majr */
  if (end == string) return -1;            /* bail if we didn't get one */
  if (*end == '.') ++end;                  /* consume optional '.' */
  if (*end == '\0') goto ENDCHECK;         /* skip ahead if done */

  start = end;                             /* moving on, */
  minr = (int)strtol(end, &end, 10);       /* try for minr */
  if (end == start) return -1;             /* bail if we didn't get one */
  if (*end == '\0') goto ENDCHECK;         /* skip ahead if done */

  for(i=0; i<3; i++) {                     /* try for level */
    if (index(".\0", *end)) break;
    levstr[i] = *end++;
  }
  levstr[i] = '\0';
  
  if (*end == '.') ++end;                  /* consume optional '.' */
  if (*end == '\0') goto ENDCHECK;         /* skip ahead if done */

  start = end;                             /* moving on, */
  patch = (int)strtol(start, &end, 10);    /* try for patch */
  if (end == start) return -1;             /* bail if we didn't get one */
  if (*end != '\0') return -1;             /* bail if leftover garbage */
  
 ENDCHECK:
  if ((majr < 0) || (minr < 0) || (patch < 0))
    return -1;
  if (strcmp(levstr, "dev") == 0)
    version->level = dev;
  else if (strcmp(levstr, "a") == 0)
    version->level = alpha;
  else if (strcmp(levstr, "b") == 0)
    version->level = beta;
  else if (strcmp(levstr, "B") == 0)
    version->level = Beta;  /* groan */
  else if (strcmp(levstr, "") == 0)
    version->level = released;
  else
    return -1;
  version->majr = majr;
  version->minr = minr;
  version->patch = patch;
  return 0;
}


int versionCmp(version1, version2)
     struct version *version1;
     struct version *version2;
{
  int retval;

  ((retval = (int)version1->majr - (int)version2->majr)
   || (retval = (int)version1->minr - (int)version2->minr)
   || (retval = (int)version1->level - (int)version2->level)
   || (retval = (int)version1->patch - (int)version2->patch));
  return retval;
}


char *unparseVersion(version)
     struct version *version;
{
  char buffer[64];
  sprintf(buffer, "%d.%d%s.%d", version->majr, version->minr, 
	  (version->level == dev) ? "dev" :
	  (version->level == alpha) ? "a" :
	  (version->level == beta) ? "b" :
	  (version->level == Beta) ? "B" :
	  (version->level == released) ? "" : "unknown",
	  version->patch);
  return strdup(buffer);
}


