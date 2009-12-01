/*
 * ZHAV & ZGET utilities.
 *
 * Copyright 1994, Z-Code Software Corporation.
 */

#ifndef lint
static char	download_rcsid[] = "$Id: download.c,v 2.10 1995/07/14 03:59:29 schaefer Exp $";
#endif

#include "zmail.h"

#define _POSIX_SOURCE
#ifdef __sgi
#define _BSD_TYPES	/* for uint typedef */
#endif

#define ZYNC_PRODUCT "zmail"

#ifdef UNIX
  /* could be more specific...  :-) */
# define ZYNC_PLATFORM "unix"
#else /* !UNIX */
# ifdef MAC_OS
#  define ZYNC_PLATFORM "mac"
# else /* !MAC_OS */
#  define ZYNC_PLATFORM "windows"
# endif /* !MAC_OS */
#endif /* !UNIX */

#include "config.h"
#ifndef MSDOS
#include <dirent.h>
#endif /* !MSDOS */
#include <limits.h>
#ifdef HAVE_MALLOC_H
# include <malloc.h>
#endif /* HAVE_MALLOC_H */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "epop.h"
#include "fsfix.h"
#include "pop.h"
#include "zcunix.h"
#include "zync.h"
#include "glob.h"

#ifndef CHAR_BIT
#define CHAR_BIT 8	/* a reasonable assumption */
#endif /* !CHAR_BIT */

/* A loose (but safe) upper bound on the maximum number decimal digits
 * needed to represent an integral type. */
#define MAXDIGITS(type)  (CHAR_BIT * sizeof(type))


static const char terminus[] = ".";
#define TERMINUS_CHAR '.'


/* Get the first line of the named file */

static int
get_first_line(filename, buf, size)
const char *filename;
char *buf;
unsigned int size;
{
    int ret = 0;
    
    FILE *file = fopen(filename, "r");
    if (!file)
	return 0;
    if (fgets(buf, size, file)) {
	ret = 1;
	if (*buf)
	    buf[strlen(buf)-1] = '\0';
    }
    fclose(file);
    return ret;
}


/*
 * List available versions of all files in the given directory.
 *
 * This function should be used to generate the data portion of a ZHAV
 * command.  Before executing zync_describe_files(), the caller must have
 * opened a server connection, issued a ZHAV command, and verified
 * that the server is ready to receive version data.
 */

void
zync_describe_files(listener, dirname)
     PopServer listener;
     const char *dirname;
{
  DIR *directory = opendir(dirname);

  if (directory)
    {
      const int dirlen = strlen(dirname);
      const struct dirent *entry;
      
      while (entry = readdir(directory))
	{
	  char fullname[MAXPATHLEN];
	  char firstline[250];
	  
	  sprintf(fullname, "%s%c%s", dirname, SLASH, entry->d_name);
	  if (get_first_line(fullname, firstline, sizeof firstline))
	      sendline(listener, firstline);
	}
    }
  sendline(listener, terminus);
}


void
zync_update_files(provider, dirname, backup)
     PopServer provider;
     const char *dirname;
     void (*backup)P((const char *));
{
  char *line;
  
  while ((line = egetline(provider)) && strcmp(line, terminus))
    {
      char filename[MAXPATHLEN];
      char fullname[MAXPATHLEN];
      struct stat status;
      FILE *file;
      int l;
      
      sscanf(line, "%s %*u", filename);
      sprintf(fullname, "%s%c%s", dirname, SLASH, filename);

      if (backup && !stat(fullname, &status))
	backup(fullname);

      file = fopen(fullname, "w");
      if (!file) {
	  error(SysErrWarning,
	      "Z-POP:\nCan't open %s for updating", fullname);
	  return;
      }
      while ((line = egetline(provider)) && strcmp(line, terminus)) {
        if (*line == TERMINUS_CHAR) line++;
	l = strlen(line);
	line[l++] = '\n';
	line[l] = 0;
	fputs(line, file);
	}
      
      fclose(file);
    }
}

int
zync_moi(server)
PopServer server;
{
    char buf[80];
    
    if (sendline(server, "ZMOI") || getok(server))
	return -1;
    sprintf(buf, "product %s", ZYNC_PRODUCT);
    sendline(server, buf);
    sprintf(buf, "version %s", zmVersion(1));
    sendline(server, buf);
    sprintf(buf, "platform %s", ZYNC_PLATFORM);
    sendline(server, buf);
    sendline(server, ".");
    return getok(server);
}
