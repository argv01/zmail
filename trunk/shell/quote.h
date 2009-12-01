#ifndef INCLUDE_QUOTE_H
#define INCLUDE_QUOTE_H

#include <general.h>


/* quote a string against the Z-Script parser */
extern char *quotezs P((const char *, const int));

/* quote a string against the Bourne shell */
extern char *quotesh P((const char *, const int, const int));

/* quote a string against the old Z-Script parser */
extern char *quoteit P((const char *, const int, const int));

/* quote every string in an argv with quoteit */
extern char **quote_argv P((char **, int, int));

/* quote using backslashes, for shell and/or RFC822 */
extern char *backwhack P((const char *));


extern void strip_quotes P((char *, char *));

#endif /* INCLUDE_QUOTE_H */
