#include <dpipe.h>
#include <dynstr.h>
#include <errno.h>
#include <excfns.h>
#include <glist.h>
#include <limits.h>
#include <mime-api.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "decode.h"
#include "fatal.h"
#include "files.h"
#include "headers.h"
#include "options.h"


void
dumper(struct dpipe *source, FILE *destination)
{
    char *transfer;
    const int available = dpipe_Get(source, &transfer);
    if (available) {
	efwrite(transfer, 1, available, destination, WHERE(dumper));
	free(transfer);
    }
}


static const char *
pick_filename(const struct glist *parameters)
{
    static char filename[PATH_MAX];
#define MAX_PREFIX_LEN (sizeof(filename) - MAX_SUFFIX_LEN - 1)
    static int tail = 0;
    const char * const suggestion = header_find(parameters, "name");

    if (!tail) {
	if (externDir) {
	    sprintf(filename, "%.*s/", MAX_PREFIX_LEN - 1,
		    *externDir ? externDir : ".");
	    tail = strlen(filename);
	} else {
	    const struct passwd * const passwd = getpwuid(getuid());
	    const char addition[] = "/Mail/detach.dir/";
	    ASSERT(passwd && passwd->pw_dir, strerror(ENOENT), WHERE(pick_filename));
	    sprintf(filename, "%.*s%s",
		    MAX_PREFIX_LEN - (sizeof(addition) - 1),
		    passwd->pw_dir, addition);
	    tail = strlen(filename);
	}
	ASSERT(!mkdirhier(filename, S_IRWXU), strerror(errno), WHERE(pick_filename));
    }

    if (suggestion && *suggestion) {
	sprintf(filename+tail, "%.*s", MAX_PREFIX_LEN - tail, suggestion);
	filename_disarm(filename+tail);
    } else
	sprintf(filename+tail, "%.*s", MAX_PREFIX_LEN - tail, "part");
    
    filename_uniqify(filename);

    return filename;
}	    


const char *
decoder_open(struct dpipe *pipe, const struct glist *headers)
{
    FILE *sink;
    int (*closer)(FILE *);
    char *type, *encoding;
    char method = '=';		/* assume unencoded */
    const char *filename = 0;
    const char *text = "";	/* assume no special text processing */
    struct glist *parameters = 0;

    /* XXX casting away const */
    mime_AnalyzeHeaders((struct glist *) headers, &parameters, &type, 0, 0, &encoding);

    if (encoding && *encoding) {
	if (!strcasecmp(encoding, "base64"))
	    method = 'b';
	else if (!strcasecmp(encoding, "quoted-printable"))
	    method = 'q';
	else if (!strcasecmp(encoding, "7bit")
		 || strcasecmp(encoding, "8bit")
		 || strcasecmp(encoding, "binary"))
	    method = '=';
	else
	    method = 0;
    }

    switch (method) {
    case '=':
	filename = pick_filename(parameters);
	sink = fopen(filename, "w");
	closer = fclose;
	break;
    case 'b':
	if (!type || !*type || !strcasecmp(type, "text"))
	    text = " -p";
	/* fall through */
    case 'q': {
	filename = pick_filename(parameters);
	{
	    struct dynstr command;
	    struct dynstr environment;
	    
	    dynstr_Init(&command);
	    
	    dynstr_Set(&command, mimencode);
	    dynstr_Append(&command, text);
	    dynstr_Append(&command, " -");
	    dynstr_AppendChar(&command, method);
	    dynstr_Append(&command, " -u -o \"$outfile\"");

	    dynstr_Init(&environment);
	    dynstr_Append(&environment, "outfile=");
	    dynstr_Append(&environment, filename);
	    eputenv(dynstr_Str(&environment), WHERE(decoder_open));

	    sink = popen(dynstr_Str(&command), "w");
	    dynstr_Destroy(&environment);
	    dynstr_Destroy(&command);
	}	    
	closer = pclose;
	break;
    }
    default:
	sink = 0;
    }
  
    if (sink) {
	dpipe_Init(pipe, (dpipe_Callback_t) dumper, sink, 0, closer, 1);
	return filename;
    } else
	return 0;
}
      

void decoder_close(struct dpipe *pipe)
{
    FILE *sink = (FILE *) dpipe_rddata(pipe);

    dpipe_Flush(pipe);
    dpipe_Close(pipe);
    if (sink) {
	fflush(sink);
	((int (*)(FILE *))(dpipe_wrdata(pipe)))(sink);
    }
    dpipe_Destroy(pipe);
}
