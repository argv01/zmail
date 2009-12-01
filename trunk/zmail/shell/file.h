#ifndef INCLUDE_SHELL_FILE_H
#define INCLUDE_SHELL_FILE_H


#include "osconfig.h"
#include <general.h>
#include <stdio.h>

struct stat;

/* expand path names that begin with "~" (home) */
char *homepath P((char *buf, char *p));
/* expand path names that begin with "%" (spool) */
char *spoolpath P((char *buf, char *p));
/* expand path names that begin with "+" (folder) */
char *pluspath P((char *buf, char *p));
/* expand "&" (for mbox) or return mbox value */
char *mboxpath P((char *buf, char *p));
int pathstat P((const char *p, char *buf, struct stat *s_buf));
int getstat P((const char *p, char *buf, struct stat *s_buf));
int dgetstat P((const char *dir, const char *file, char *buf, struct stat *s_buf));
char *getpath P((const char *p, int *isdir));
int dvarstat P((const char *dir, const char *file, char *buf, struct stat *s_buf));
int varstat P((const char *file, char *buf, struct stat *s_buf));
/* static char returning path (expanding variables) */
char *varpath P((const char *path, int *isdir));
char *pathclean P((char *, const char *));
/* convert a path to a full path without "." or ".." */
char *fullpath P((char *path, int hardpath));
char *expandpath P((const char *path, char *newpath, int hardpath));
char *getdir P((const char *path, int makeit));
int file_to_fp P((register char *p, register FILE *fp, char *mode, int add_newline));
long fp2fp P((FILE *infp, long start, long len, FILE *outfp));
long fp_to_fp P((FILE *infp, long start, long len, FILE *outfp));
long fioxlate P((FILE *in, long start, long len, FILE *out,
		 long (*xlate)(), VPTR state));
long xlcount P((char *input, long len, char **output, char *state));
int emptyfile P((register FILE **fp, register char *fname));
int nopenfiles P((int argc));
int closefileds_above P((int n));
/* open a file or program for write/append */
FILE *open_file P((register char *p, int program, int lockit));
int open_list P((char *names[], FILE *files[], int size));
int find_files P((register char *s, char *names[], int size, int force, int recorduser));
int StatAccess P((struct stat *buf, int mode));
#ifndef WIN16
int Access P((const char *file, int mode));
#endif /* !WIN16 */
/* open a file with umask 077 (permissions 600) */
FILE *mask_fopen P((const char *file, const char *mode));
/* remove or condense leading file name path */
char *trim_filename P((const char *name));
char *alloc_tempfname P((const char *seedname));
char *alloc_tempfname_dir P((const char *seedname, const char *dir_prefix));
/* open a temporary file with a unique name */
FILE *open_tempfile P((const char *seedname, char **tmpname));
void touch P((const char *path));
int zwipe P((const char *path));
#ifndef HAVE_RENAME
int rename P((const char *from, const char *to));
#endif /* !HAVE_RENAME */
#if !defined(HAVE_MKDIR) || defined(MKDIR_PROG)
int mkdir P((const char *path, int mode));
#endif /* !HAVE_MKDIR || MKDIR_PROG */
int exists_dir P((const char *path, int chklast));
int zmkdir P((const char *path, int mode));
int zmkpath P((const char *path, int mode, int mklast));
long file_size P((FILE *fp));

#ifndef HAVE_FTRUNCATE
extern int ftruncate P((int, off_t));
#endif

#endif /* !INCLUDE_SHELL_FILE_H */
