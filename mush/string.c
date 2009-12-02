
#include "mush.h"

char *
savestr(s)
register char *s;
{
    register char *p;

    if (!s)
	s = "";
    if (!(p = malloc((unsigned) (strlen(s) + 1)))) {
	error("out of memory saving %s", s);
	return NULL;
    }
    return strcpy(p, s);
}

