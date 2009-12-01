#ifndef _MAIL_H_
#define _MAIL_H_

void compose_mode P((int on));
int zm_mail P((register int n, register char **argv, struct mgroup *list));
char *get_template_path P((const char *str, int quiet));
int compose_letter P((void));
int finish_up_letter P((void));
int extract_addresses P((char *addr_list, char **names, int next_file, int size));
RETSIGTYPE rm_edfile P((int sig));
int write_draft P((char *file, char *outbox_addresses));
void dead_letter P((int sig));
int prepare_edfile P((void));
int truncate_edfile P((struct Compose *compose, long new_size));
int open_edfile P((struct Compose *compose));
int close_edfile P((struct Compose *compose));
int parse_edfile P((struct Compose *compose));
int reload_edfile P((void));
void invoke_editor P((const char *edit));
int run_editor P((const char *edit));
int init_attachment P((const char *attachment));
FILE *do_start_sendmail P((struct Compose *compose, char *addr_list));
int do_sendmail P((struct Compose *compose));

#endif
