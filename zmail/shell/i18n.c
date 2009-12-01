/* i18n.c	Copyright 1993 Z-Code Software Corp. */

#include "osconfig.h"

#include "zmail.h"
#include "i18n.h"
#include "catalog.h"
#include "child.h"
#include "fsfix.h"
#include "pager.h"
#include "zmstring.h"

#ifdef ZMAIL_INTL

/* Currently just test whether things will work */
int
i18n_initialize()
{
    if (!boolean_val(VarFromSpoolFilter))
	return -1;
    return 0;
}

/* This is almost a verbatim copy of handle_coder_err() from attach.c,
 * except that we unlink the error file after displaying it.
 */
int
i18n_handle_error(val, program, errfile)
int val;
char *program, *errfile;
{
    ZmPager pager;
    
    if (errfile && *errfile) {
	FILE *fp;
	struct stat s_buf;

	if (stat(errfile, &s_buf) == 0) {
	    if ((val || s_buf.st_size != 0) &&
		(fp = fopen(errfile, "r"))) {
		pager = ZmPagerStart(PgText);
		ZmPagerSetTitle(pager,
				catgets( catalog, CAT_SHELL, 810, "Error Processing File" ));
		if (program) {
		    zmVaStr(catgets(catalog, CAT_SHELL, 891, "The command\n\t%s\nexited with status %d.\n\n\
%s\n\n"), program, val, s_buf.st_size != 0? catgets(catalog, CAT_SHELL, 892, "Output:") : "");
		    ZmPagerWrite(pager, zmVaStr(NULL));
		}
		(void) fioxlate(fp, -1, -1, NULL_FILE, fiopager, pager);
		ZmPagerStop(pager);
		(void) fclose(fp);
	    } else if (val) {
		error(SysErrWarning, catgets(catalog, CAT_SHELL, 893, "Cannot open error file \"%s\""), errfile);
	    }
	    (void) unlink(errfile);
	}
    }
    return val;
}

int
i18n_copy(infile, start, i18n_translate, outfile)
char *infile, *i18n_translate, *outfile;
size_t start;
{
    char **shellcmd = vavec("(", "", ") >>", "", "2>", "", NULL);
    FILE *rfp, *wfp;
    size_t size;
    int status;

    ZSTRDUP(shellcmd[1], i18n_translate);
    ZSTRDUP(shellcmd[3], outfile);
    shellcmd[5] = NULL;
    if (!(wfp = open_tempfile("err", &shellcmd[5]))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 811, "Cannot open error tempfile" ));
	free_vec(shellcmd);
	return -1;
    } else
	(void) fclose(wfp);
    if (!(rfp = lock_fopen(infile, "r"))) {
	error(SysErrWarning,
	    catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), infile);
	free_vec(shellcmd);
	return -1;
    }
    i18n_translate = joinv(NULL, shellcmd, " ");
    Debug("I18N mirror command: %s\n",
	i18n_translate ? i18n_translate : "(null)");
    if (!i18n_translate || !(wfp = popen(i18n_translate, "w"))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 813, "Cannot filter \"%s\"" ), infile);
	xfree(i18n_translate);
	free_vec(shellcmd);
	return -1;
    }
    xfree(i18n_translate);
    size = fp_to_fp(rfp, start, -1, wfp);
    close_lock(infile, rfp);
    status = pclose(wfp);
    (void) i18n_handle_error(WEXITSTATUS(*(WAITSTATUS *)&status),
		shellcmd[1], shellcmd[5]);
    free_vec(shellcmd);
    return size;
}

/* Function to be called via Ftrack to monitor the real spool file.
 *
 * This is a hack and should be converted to progio or better.	XXX
 */
void
i18n_mirror_spool(realspool, spoolstatus)
struct ftrack *realspool;
struct stat *spoolstatus;
{
    char *i18n_read_spool = value_of(VarFromSpoolFilter);
    size_t oldsize, newsize;

    /* Ignore changes in modification time only */
    if (ftrack_Size(realspool) == spoolstatus->st_size)
	return;
    if (isoff(spool_folder.mf_flags, CONTEXT_IN_USE)) {
	print(catgets( catalog, CAT_SHELL, 814, "You have new mail in your system mailbox.\n" ));
	return;
    }
    if (pathcmp(spoolfile, ftrack_Name(realspool)) == 0) /* Sanity */
	return;
    if (!i18n_read_spool || !*i18n_read_spool)
	i18n_read_spool = I18N_READER;
    if (spoolstatus->st_size < ftrack_Size(realspool)) {
	FILE *fp = mask_fopen(spoolfile, "w");
	if (fp)
	    (void) fclose(fp);
	oldsize = 0;
    } else
	oldsize = ftrack_Size(realspool);
    newsize = i18n_copy(ftrack_Name(realspool),
			oldsize, i18n_read_spool, spoolfile);
#ifdef NOT_NOW
    if (newsize < spoolstatus->st_size - oldsize) {
	error(ZmErrWarning, catgets( catalog, CAT_SHELL, 815, "mailbox filtering incomplete" ));
    }
#endif /* NOT_NOW */
    spoolstatus->st_size = oldsize + newsize;
}

void
i18n_update_spool(spools, sbuf)
struct ftrack **spools;
struct stat *sbuf;
{
#define FAKESPOOL spools[0]
#define REALSPOOL spools[1]
    char *i18n_translate, **shellcmd, buf[BUFSIZ];
    FILE *efp, *rfp, *wfp = NULL_FILE;
    int i, status;

    /* Attempt to make sure we've read all new mail */
    while ((status = ftrack_Do(REALSPOOL, TRUE)) > 0)
	ftrack_Stat(FAKESPOOL);

    /* This is dangerous without dot-locking ...	XXX */
    if (status == 0)
	wfp = lock_fopen(ftrack_Name(REALSPOOL), "w+");
    if (!wfp) {
	error(SysErrWarning, "unable to update %s", ftrack_Name(REALSPOOL));
	return;
    }

    shellcmd = vavec("(", "", ") <", "", "2>", "", NULL);

    i18n_translate = value_of(VarToSpoolFilter);
    if (!i18n_translate || !*i18n_translate)
	i18n_translate = I18N_WRITER;

    ZSTRDUP(shellcmd[1], i18n_translate);
    ZSTRDUP(shellcmd[3], spoolfile);
    shellcmd[5] = NULL;
    if (!(efp = open_tempfile("err", &shellcmd[5]))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 816, "cannot open error tempfile" ));
	free_vec(shellcmd);
	close_lock(ftrack_Name(REALSPOOL), wfp);
	return;
    } else
	(void) fclose(efp);
    i18n_translate = joinv(NULL, shellcmd, " ");
    Debug("I18N update command: %s\n",
	i18n_translate ? i18n_translate : "(null)");
    if (!i18n_translate || !(rfp = popen(i18n_translate, "r"))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 817, "cannot filter %s" ), spoolfile);
	xfree(i18n_translate);
	free_vec(shellcmd);
	close_lock(ftrack_Name(REALSPOOL), wfp);
	return;
    }
    xfree(i18n_translate);
    do {	/* Reading from pipe -- don't fail on SIGCHLD */
	while (i = fread(buf, sizeof(char), sizeof buf, rfp)) {
	    if (i != fwrite(buf, sizeof(char), i, wfp) || fflush(wfp) != 0) {
		error(SysErrWarning, catgets(catalog, CAT_SHELL, 818,
					    "mailbox update incomplete"));
		break;
	    }
	}
    } while (errno == EINTR && !feof(rfp));
    status = pclose(rfp);
    (void) i18n_handle_error(WEXITSTATUS(*(WAITSTATUS *)&status),
		shellcmd[1], shellcmd[5]);
    free_vec(shellcmd);
    close_lock(ftrack_Name(REALSPOOL), wfp);

    set_mbox_time(ftrack_Name(REALSPOOL));
    ftrack_Stat(REALSPOOL);
}

void
i18n_mta_interface(str)
char *str;
{
    char *i18n_translate = value_of(VarToMtaFilter);

    if (i18n_translate && *i18n_translate)
	(void) sprintf(str, "( %s ) | ", i18n_translate);
    else
	*str = 0;
}

#else /* ZMAIL_INTL */

#undef i18n_mta_interface

void
i18n_mta_interface(str)
char *str;
{
    *str = 0;
}

#endif /* ZMAIL_INTL */

char **
catgetrefvec(references, count)
catalog_ref *references;
int count;
{
    char **strings, **insert;

    insert = strings = (char **) malloc((count+1) * sizeof *strings);

    while (count--) {
	*insert++ = savestr(catgetref(*references));
	references++;
    }
    
    *insert = NULL;
    return strings;
}
