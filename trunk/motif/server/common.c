#include "osconfig.h"
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "xlib.h"

static Atom advertiseAtom = 0;
static const char basis[] = "ZMAIL_EXEC:";


Atom
get_advertisement(display, identity, nocreate)
    Display *display;
    const char *identity;
    Bool nocreate;
{
    if (!advertiseAtom)
	if (display && identity) {
	    char * advertiseName = (char *) malloc(sizeof(basis) + strlen(identity));
	    
	    if (advertiseName) {
		strcpy(advertiseName, basis);
		strcpy(advertiseName + sizeof(basis) - 1, identity);
		
		advertiseAtom = XInternAtom(display, advertiseName, nocreate);
		free(advertiseName);
	    }
	}
    
    return advertiseAtom;
}
