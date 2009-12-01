#ifndef INCLUDE_MSGS_HDRS_H
#define INCLUDE_MSGS_HDRS_H


struct Msg;
struct mgroup;

long msg_seek P((struct Msg *mesg, int whence));
/* find start of message and return From_ line */
char *msg_get P((int n, char *from, int count));
/* the line in msg described by arg (message header) */
char *header_field P((int n, const char *str));
/* the From: header_field(), saved after lookup */
char *from_field P((int n));
/* the To: header_field(), saved after lookup */
char *to_field P((int n));
/* the Subj: header_field(), saved after lookup */
char *subj_field P((int n));
/* the Message-Id: header_field(), saved after lookup */
char *id_field P((int n));
/* who do we reply to when responding */
char *reply_to P((int n, int all, char buf[]));
void fix_my_addr P((char *buf));
/* skip "Re:" and "(Fwd)" in subject */
char *clean_subject P((char *subj, int fix_re));
/* when responding, return str which is the subject */
char *subject_to P((int n, char *buf, int add_re));
/* when responding, return str which is the cc-list */
char *cc_to P((int n, char *buf));
/* concatenate values of multiple hdrs into 1 string */
char *concat_hdrs P((int n, char *hdrs, int tartare, int *olen));
/* returns a formatted line describing passed msg # */
char *format_hdr P((int cnt, const char *hdr_fmt, int show_to));
/* passes hdr_format to format_hdr() for displays */
char *compose_hdr P((int cnt));
char *priority_string P((int i));
int zm_hdrs P((int argc, char **argv, struct mgroup *list));


#endif /* !INCLUDE_MSGS_HDRS_H */
