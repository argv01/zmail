#include <X11/Xlib.h>
#include "client.h"
#include "remoteMail.h"


Status
ZmailSendFile(Screen *screen, const char *username,
	      const char *filename, const char *recipient,
	      const char *subject, const char *content)
{
    const char *argv[] = { "trusted_sendfile", filename,
			   recipient, subject, content };
    const int argc = sizeof(argv)/sizeof(*argv);

    // XXX casting away const, here, since (char **) and const'ing just don't work right
    return ZmailExecute(screen, username, argc, (char **) argv);
}
