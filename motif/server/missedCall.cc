#include <X11/Xlib.h>
#include "client.h"
#include "remoteMail.h"


Status
ZmailMissedCall(Screen *screen, const char *username,
		const char *recipient, const char *subject,
		const char *content, const char *inpersonReplyName)
{
    const char *argv[] = { "trusted_mailto", recipient, subject,
			   content, inpersonReplyName };
    const int argc = sizeof(argv)/sizeof(*argv) -
	(!inpersonReplyName || !*inpersonReplyName);

    // XXX casting away const, here, since (char **) and const'ing just don't work right
    return ZmailExecute(screen, username, argc, (char **) argv);
}
