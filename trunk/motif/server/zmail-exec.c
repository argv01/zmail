#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client.h"


int
main(argc, argv)
    int argc;
    char *argv[];
{
    if (argc >= 2) {
	char *name = getenv("DISPLAY");
	Display *display = XOpenDisplay(name);
	if (display)
	    return ZmailExecute(DefaultScreenOfDisplay(display), getlogin(), argc - 1, &argv[1]);
	else {
	    fprintf(stderr, "Could not open %s.\n", name ? name : "default display");
	    return 1;
	}
    } else {
	fprintf(stderr, "Usage: %s  { <z-script> ... }\n", argv[0]);
	return 2;
    }
}
