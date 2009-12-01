#ifndef INCLUDE_MSGS_EDMAIL_H
#define INCLUDE_MSGS_EDMAIL_H


#include <config.h>
#include <general.h>
#include <stdio.h>

struct compose;
struct dynstr;
struct mgroup;

/* edmail.c 21/12/94 16.20.36 */
int add_to_letter P((char line[], int noisy));
struct edmail *fetch_comp_command P((char *name));
int zm_edmail P((int argc, char **argv, struct mgroup *list));
int interpose_on_send P((struct Compose *compose));
char *job_numbers P((void));
int zm_jobs P((int argc, char **argv, struct mgroup *unused));
char *dyn_gets P((struct dynstr *dsp, FILE *fp));
char *get_message_state P((void));
char *get_main_state P((void));
char *get_compose_state P((void));
void set_compose_state P((const char *item, int how));

#ifdef UNIX
int strs_from_program P((const char *command, char ***strs));
#endif /* UNIX */


#endif /* !INCLUDE_MSGS_EDMAIL_H */
