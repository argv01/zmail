#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "announce.h"
#include "files.h"
#include "headers.h"
#include "options.h"


void
print_externDir(FILE *delivery, const char *home)
{
    if (externDir)
	fputs(externDir, delivery);
    else
	fprintf(delivery, "%s/Mail/detach.dir", home);
}


void
announce(const char *waiting_room, const char *total, const struct glist *headers)
{
    const struct passwd * const passwd = getpwuid(getuid());

    if (passwd && passwd->pw_name && passwd->pw_dir) {
	FILE *delivery = popen_delivery();
	const time_t now = time(0);
	const char *self = passwd->pw_name;

	fprintf(delivery, "From %s %s", self, ctime(&now));

	{
	    const struct tm * const tm = localtime(&now);
	    char date[160];
	    const size_t success = strftime(date, sizeof(date), "%e %h %y %R", tm);

	    if (success) {
		const int hours   = timezone / 3600;
		const int minutes = (timezone - (hours * 3600)) % 60;
		fprintf(delivery, "Date: %s %+03d%02d\n", date, -hours, minutes);
	    }
	}


	fprintf(delivery, "\
From: %s\n\
To: %s\n\
Subject: Large Message Arriving\n\
Content-Type: multipart/mixed; boundary=+\n\
\n\
\n\
--+\n\n",
		self, self);

	{
	    const char * const from = header_find(headers, "from");
	    
	    if (from)
		fprintf(delivery, "\
A very large message from%s\
is being received.\n\n", from);
	    else
		fputs("A very large message is being received.\n\n", delivery);
	}
	
	fputs("This message has been split into ", delivery);
	if (total)
	    fprintf(delivery, "%s", total);
	else
	    fputs("several", delivery);

	fputs(" parts and will be\n\
reassembled when all parts have been received. Then, the\n\
message will arrive in your mailbox.\n\n", delivery);
	
	if (externSize) {
	    fputs("Large attachments will be located in\n", delivery);
	    print_externDir(delivery, passwd->pw_dir);
	    fputs(".\n\
Large attachments continue to occupy space on your hard disk\n\
until you delete them. If you delete the message, these\n\
attachments are not deleted. To delete the attachments, open\n\
a directory view of ",
		  delivery);
	    print_externDir(delivery, passwd->pw_dir);
	    fputs("\n\
and drag the large attachments to the dumpster.\n\n", delivery);
	}

	fprintf(delivery, "\
To check the status of the incoming message, or for\n\
more information about the message, double click on the\n\
document icon in the attachment area above. This status\n\
document is updated each time a new part is received. To\n\
see the latest status, close and reopen the status document.\n\
\n\
Once the reassembled message has arrived in your mailbox,\n\
the status document will not be available.\n\
\n\
If one or more pieces fail to arrive you will not receive the\n\
message. Ask the sender to resend pieces that may have\n\
\"bounced\" back. If the message still does not arrive,\n\
remove the pieces that have arrived and ask the sender to\n\
resend the message.\n\
\n\
Pieces are located in\n\
%s. These pieces\n\
will continue to occupy space on your hard disk until you delete\n\
them, or until all of the remaining parts have arrived.\n\
\n\
--+\n\
Content-Description: Status Document\n\
Content-Type: message/external-body; access-type=local-file; name=%s/status\n\
\n\
\n\
--+--\n",
		waiting_room, waiting_room);

	pclose(delivery);
    }
}
