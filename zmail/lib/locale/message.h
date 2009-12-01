#ifndef INCLUDE_MESSAGE_H
#define INCLUDE_MESSAGE_H

/*
 *  Catsup:  Catalog Synchronizer and Updater
 */

#include <general.h>
#include "phase.h"


typedef struct
{
  char *comments;
  char *text;
  enum phase phase;
} message;


int notice_message P(( message, int ));

enum { NoMessageNumber = -1 };


#endif /* !INCLUDE_MESSAGE_H */
