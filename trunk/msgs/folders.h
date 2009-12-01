#ifndef INCLUDE_MSGS_FOLDERS_H
#define INCLUDE_MSGS_FOLDERS_H


#include <general.h>
#include "zfolder.h"

struct Msg;
struct mgroup;
struct mfolder;
struct stat;


/* folders.c 21/12/94 16.20.50 */
void clearMsg P((struct Msg *m, int f));
void flush_msgs P((void));
int vrfy_update P((struct mfolder *new, unsigned long *flags, int updating));
int folder P((int argc, char **argv, struct mgroup *list));
int change_folder P((char *buf,
	 struct mfolder *newfolder,
	 struct mgroup *list,
	 unsigned long flgs,
	 long prev_size,
	 int updating));
char *ident_folder P((char *name, char *buf));
int bringup_folder P((struct mfolder *fldr, struct mgroup *list, unsigned long flgs));
struct mfolder *new_folder P((int n, FolderType fotype));
struct mfolder *lookup_folder P((char *name, int n, unsigned long flgs));
void backup_folder P((void));
void unhook_backup P((struct mfolder *fldr));
int close_backups P((int updating, char *query));
/* return numbers of all open folders as a string */
char *fldr_numbers P((void));
/* descriptive text about a specified folder */
char *folder_info_text P((int n, struct mfolder *fldr));
char *folder_shortname P((struct mfolder *fldr, char *buf));
int shutdown_folder P((struct mfolder *fldr, unsigned long flgs, char *query));
int update_folders P((unsigned long flgs, char *query, int vrfy_each));
int folders P((int argc, char **argv));
int is_rfc822_header P((char *line, int prev_is_rfc822));
FolderType stat_folder P((const char *name, struct stat *s_buf));
FolderType test_folder P((const char *name, const char *query));
int rm_folder P((char *file, unsigned long prompt_flags));
int merge_folders P((int n, register const char **argv, struct mgroup *list));
int zm_undigest P((int n, char *argv[], struct mgroup *list));
int rename_folder P((int argc, char **argv, struct mgroup *list));
void sort_new_mail P((void));
int zm_uudecode P((int argc, char *argv[], struct mgroup *list));


#endif /* !INCLUDE_MSGS_FOLDERS_H */
