#ifndef INCLUDE_MSGS_FOLOAD_H
#define INCLUDE_MSGS_FOLOAD_H


#include <general.h>
#include "zfolder.h"

/* foload.c 21/12/94 16.20.50 */

struct Attach;
struct Source;
struct mgroup;

char *match_msg_sep P((char *buf, FolderType fotype));
void parse_from P((struct Source *ss, char path[]));
int explode_multipart P((int, struct Attach *, struct Source *, int *, int));
int load_attachments P((int, struct Source *, int *, int));
void parse_header P((char *buf,
	 Msg *mesg,
	 struct Attach **attach,
	 int want_status,
	 long today,
	 int cnt));
int load_headers P((char *line, int cnt, struct Source *ss, int *lines));
int recover_folder P((void));
int load_folder P((const char *file, int append,
		   int last, struct mgroup *list));
long from_stuff P((char *line, long len, char **output, char *state));
u_long parse_priorities P((const char *str));


#endif /* !INCLUDE_MSGS_FOLOAD_H */
