#ifndef INCLUDE_MSGS_NEWMAIL_H
#define INCLUDE_MSGS_NEWMAIL_H


#include <general.h>

struct stat;

/* newmail.c 21/12/94 16.20.56 */
int get_new_mail P((int));
void process_new_mail P((int, int));
int show_new_mail P((void));
void zm_license_failed P((void));
int check_new_mail P((void));
void folder_new_mail P((msg_folder *, struct stat *));
void check_other_folders P((void));


#endif /* !INCLUDE_MSGS_NEWMAIL_H */
