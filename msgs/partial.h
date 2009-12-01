#ifndef INCLUDE_MSGS_PARTIAL_H
#define INCLUDE_MSGS_PARTIAL_H


#include "config/features.h"
#include "general.h"
#include "zm_ask.h"


struct Compose;

#ifdef PARTIAL_SEND
enum AskAnswer partial_confirm P((struct Compose *));
#ifdef PARTIAL_SEND_DIALOG
enum AskAnswer gui_partial_confirm P((unsigned long, unsigned long, unsigned long *));
#endif /* PARTIAL_SEND_DIALOG */
#endif /* PARTIAL_SEND */

unsigned long compose_guess_size P((const struct Compose *));


#endif /* !INCLUDE_MSGS_PARTIAL_H */
