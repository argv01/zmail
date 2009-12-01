#include "zmail.h"

int
_XrmInternalStringToQuark(string)
char *string;
{
    return XrmStringToQuark(string);
}

void
do_nothing()
{
    return;
}

int
_aixXLoadIM()
{
    static char called;

    if (!called) {
	XtAppSetWarningHandler(app, do_nothing);
	XtAppSetWarningMsgHandler(app, do_nothing);
	called = 1;
    }
    return 0;
}
