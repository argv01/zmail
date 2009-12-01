#ifndef INCLUDE_MSGS_MSGS_H
#define INCLUDE_MSGS_MSGS_H


#include <general.h>
#include <stdio.h>

#define DEF_SYSTEM_FOLDER_NAME catgets( catalog, CAT_MSGS, 712, "Mailbox" )

int init_msg_group P((struct mgroup *grp, int n, int is_local));
int resize_msg_group P((struct mgroup *grp, int n));
void destroy_msg_group P((struct mgroup *grp));
int next_msg_in_group P((int current, struct mgroup *group));
void display_msg P((register int n, unsigned long flg));
void set_mbox_time P((char *name));
int check_folder_space P((const char *primary,
	 const char *secondary,
	 int isspool));
int copy_all P((FILE *mail_fp,
	 FILE *mbox,
	 unsigned long flg,
	 int isspool,
	 int *held,
	 int *saved));
int copyback P((char *query, int final));
int mail_size P((const char *curfile, int checkspool));
#if defined( IMAP )
void zmail_mail_status P((int as_prompt));
#else
void mail_status P((int as_prompt));
#endif
char *get_spool_name P((char *buf));
char *abbrev_foldername P((const char *name));
/* function to format prompts from the prompt string */
char *format_prompt P((struct mfolder *fldr, const char *fmt));
int chk_msg P((const char *s));
int next_msg P((int current, int direction));
int f_next_msg P((struct mfolder *fldr, int curr, int direction));
int count_msg_list P((struct mgroup *list));
int get_msg_list P((char **argv, struct mgroup *list));
int letter_to_flags P((int letter, unsigned long *onbits, unsigned long *offbits));
/* return letter codes for a message's status bits */
char *flags_to_letters P((unsigned long flags, int loud));
int msg_flags P((int c, char **v, struct mgroup *list));
int set_hidden P((struct mfolder *fldr, int *hidden_ct));
void clear_hidden P((void));


#endif /* !INCLUDE_MSGS_MSGS_H */
