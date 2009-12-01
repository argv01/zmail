#include <stdio.h>
#include <X11/Intrinsic.h>
#include <X11/Xlib.h>
#include "ipc_1.1.h"

extern int strncmp();
extern char *getcd();

#define max(a,b) ((a) > (b) ? (a) : (b))

static char *argv0;

static void
GetTempFile(tempname)
char tempname[];
{
    sprintf(tempname, "/tmp/ipcXXXXXX");
    mktemp(tempname);
}

static Time
GetCurTime()
{
    return CurrentTime;
}

static void
IpcEditApplyFunc(name, tname)
char *name;
char *tname;
{
    fprintf(stderr, "%s: received edited file '%s' (temp file '%s')\n",
	    argv0, name, tname);
    exit(0);
}

static void
IpcEditStatusFunc(status, host)
int status, host;
{
    fprintf(stderr,
	"%s: received edit status '%d' from host '%d'\n", argv0, status, host);
}

void
main(argc, argv)
int argc;
char *argv[];
{
    int argn;
    int ios_type;
    char *app, apppath[1024];
    Boolean wait;
    char *file, filepath[500];
    Display *dpy;
    Window xwindow;
    IOS_ATOM Atoms[1];
    char *iargv[6];
    char *usage = "usage:  %s [-wait] -paint|-draw|-eqn|-table|-chart <file>\n";

    /* Parse args. */
    argv0 = argv[0];
    argn = 1;
    app = NULL;
    wait = False;
    while (argn < argc && argv[argn][0] == '-') {
	if (strncmp(argv[argn], "-wait",
		max(strlen(argv[argn]), 2)) == 0) {
	    wait = True;
	} else if (strncmp(argv[argn], "-paint",
		max(strlen(argv[argn]), 2)) == 0) {
	    ios_type = IOS_PAINT_TYPE;
	    app = "IslandPaint";
	} else if (strncmp(argv[argn], "-draw",
		max(strlen(argv[argn]), 3)) == 0) {
	    ios_type = IOS_DRAW_TYPE;
	    app = "IslandDraw";
	} else if (strncmp(argv[argn], "-eqn",
		max(strlen(argv[argn]), 2)) == 0) {
	    ios_type = IOS_EQN_TYPE;
	    app = "UNKNOWN";
	} else if (strncmp(argv[argn], "-table",
		max(strlen(argv[argn]), 2)) == 0) {
	    ios_type = IOS_TABLE_TYPE;
	    app = "UNKNOWN";
	} else if (strncmp(argv[argn], "-chart",
		max(strlen(argv[argn]), 2)) == 0) {
	    ios_type = IOS_CHART_TYPE;
	    app = "UNKNOWN";
	} else if (strncmp(argv[argn], "-test",
		max(strlen(argv[argn]), 3)) == 0) {
	    ios_type = IOS_TEST_TYPE;
	    app = "UNKNOWN";
	} else {
	    fprintf(stderr, usage, argv0);
	    exit(1);
	}
	++argn;
    }
    if (app == NULL) {
	fprintf(stderr, usage, argv0);
	exit(1);
    }

    sprintf (apppath,
	    "/usr/java3/projects/ioffice_motif/bin/bin.sun4/bin.motif/%s", app);

    if (argn >= argc) {
	fprintf(stderr, usage, argv0);
	exit(1);
    }
    file = argv[argn++];
    if (file[0] == '/' || file[0] == '~')
	strcpy(filepath, file);
    else {
	(void) getwd(filepath);
	(void) strcat(filepath, "/");
	(void) strcat(filepath, file);
    }

    if (argn < argc) {
	fprintf(stderr, usage, argv0);
	exit(1);
    }

    dpy = (Display *) XOpenDisplay(0);
    xwindow = DefaultRootWindow(dpy);	/* could be any random window */
    XSelectInput( dpy, xwindow, PropertyChangeMask );
    Atoms[0].type = IOS_UNDEFINED;
    Atoms[0].atom = (Atom)-1;
    Atoms[0].name = NULL;
    Atoms[0].paste_func = NULL;
    IOSInitIPC(
	Atoms, dpy, xwindow, GetTempFile, GetCurTime, 0,
	IOS_TEST_REQUESTOR, NULL, IpcEditApplyFunc, IpcEditStatusFunc, 1);

    iargv[5] = (char *)0;
    IOSRequestEditGraphic(ios_type, filepath, app, apppath, iargv);

    if (!wait) {
	XSync(dpy, False);
	exit(0);
    }

    for (;;) {
	XEvent ev;

	XNextEvent(dpy, &ev);
	switch (ev.type) {
	    case PropertyNotify:
	    IOSPropertyEvent(&ev);
	    break;
	}

    }
}
