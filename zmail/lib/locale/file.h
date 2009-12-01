#ifndef INCLUDE_FILE_H
#define INCLUDE_FILE_H

/*
 *  Catsup:  Catalog Synchronizer and Updater
 */

#include "osconfig.h"
#include <dlist.h>
#include <general.h>


extern FILE  *yyin, *yyout;
int   yywrap();


extern struct dlist files;
extern int rewriteRequired;

char *backup P((const char *));


#endif /* !INCLUDE_FILE_H */
