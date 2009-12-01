#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "missedCall.h"


int
main(argc, argv)
    int argc;
    char *argv[];
{
    const char *name = getenv("DISPLAY");
    Display *display = XOpenDisplay(name);
    if (display)
	return ZmailMissedCall(DefaultScreenOfDisplay(display), getlogin(), argv[1], argv[2], argv[3], argv[4]);
    else {
	fprintf(stderr, "Could not open %s.\n", name ? name : "default display");
	return 1;
    }
}
