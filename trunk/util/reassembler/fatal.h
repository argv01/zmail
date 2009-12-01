#ifndef INCLUDE_REASSEMBLER_FATAL_H
#define INCLUDE_REASSEMBLER_FATAL_H


#include <stdio.h>


#define _stringize(token) # token
#define _stringval(macro) _stringize(macro)
#define WHERE(function)  # function "(" _stringval(__LINE__) ")"


extern FILE *fatal_sink;
extern const char *fatal_basename;

void fatal_handler(void);
void usage(void);


#endif /* !INCLUDE_REASSEMBLER_FATAL_H */
