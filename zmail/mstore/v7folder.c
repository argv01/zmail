/* 
 * $RCSfile: v7folder.c,v $
 * $Revision: 1.24 $
 * $Date: 1996/08/09 16:44:58 $
 * $Author: schaefer $
 */

#include <zmail.h>
#include <spoor.h>
#include <v7folder.h>
#include <v7msg.h>
#include <except.h>
#include <dputil.h>
#include "mime-api.h"

#ifndef lint
static const char v7folder_rcsid[] =
    "$Id: v7folder.c,v 1.24 1996/08/09 16:44:58 schaefer Exp $";
#endif /* lint */

static void v7folder_initialize P((struct v7folder *));

/* The class descriptor */
struct spClass *v7folder_class = 0;

static void
v7folder_initialize(self)
    struct v7folder *self;
{
    self->zfolder = 0;
}

extern char *get_name_n_addr();

/* This code roughly duplicates part of match_msg_sep() */
static char *
find_where_date_begins(str)
    char *str;
{
    int warn = ison(glob_flags, WARNINGS);
    char *p = str + 5;		/* skip "From " */
    char *result = 0;

    skipspaces(0);		/* amazingly, this depends on "p" */
    p = strpbrk(p, " \t");
    turnoff(glob_flags, WARNINGS);
    if ((p && parse_date(p + 1, (long) 0))
	|| (p = get_name_n_addr(str + 5, 0, 0)))
	result = p + 1;
    if (warn)
	turnon(glob_flags, WARNINGS);
    return (result);
}

static struct mmsg *
v7folder_Import(self, arg)
    struct v7folder *self;
    spArgList_t arg;
EXC_BEGIN
{
    struct mmsg *mesg = spArg(arg, struct mmsg *);
    struct v7message *new_mesg = 0;
    int pos = spArg(arg, int);
    int newest_msg;
    char *temp = 0;
    FILE *fp;

    /* There are two ways we could do this:
     * 1.  Cause mesg to append itself to file self->zfolder->mf_name.
     * 2.  Cause mesg to be written to a file, then merge that file.
     *     (If mesg already exists as a file, this is really easy.)
     *
     * Which one is best depends on the capabilities of the store that
     * contains mesg.  Furthermore, it may be necessary to convert the
     * message to the V7 format in the process.  This is why I expected
     * this type of operation to be handled by the meta-store.
     *
     * In this particular case (only POP to V7), we know that we can get
     * raw v7 data from message_Stream(mesg), so we could handle either
     * technique locally to this function.
     *
     * However, appending directly to zfolder->mf_name could cause a
     * conflict with new mail arrival, so we adopt the more expensive
     * but more correct second technique.
     *
     * [Fri Mar 31 09:19:37 1995] Now mmsg_Stream always gives an
     * RFC822 stream, and mmsg_FromLine gives the RFC976 "From " line.
     */

    msg_folder *save_folder = current_folder;
    struct dynstr ds;

    if (ison(self->zfolder->mf_flags, CORRUPTED)) {
	error(UserErrWarning, catgets(
	    catalog, CAT_CUSTOM, 250, "Import not performed: folder previously marked as corrupted."));
	/* XXX Throw exception?? */
	return 0;
    }
    if (isoff(self->zfolder->mf_flags, CONTEXT_IN_USE)) {
	error(UserErrWarning, catgets(
	    catalog, CAT_CUSTOM, 251, "Import not performed: folder not open."));
	/* XXX Throw exception?? */
	return 0;
    }

    turnon(glob_flags, IGN_SIGS);	/* I don't trust TRYSIG yet */

    WITH_FMODE_BINARY {
	fp = open_tempfile("v7import", &temp);
    } END_WITH_FMODE;

    dynstr_Init(&ds);
    TRY {
	long orig_offset;
	struct dpipe dp;
	char *date;

	mmsg_FromLine(mesg, &ds);
	if (date = find_where_date_begins(dynstr_Str(&ds))) {
            time_t t = time(0);

	    if (FolderDelimited == self->zfolder->mf_type)
		efprintf(fp, "%s", msg_separator);

	    /* Write the "From " line up to but not including the date */
	    fwrite(dynstr_Str(&ds), 1, date - dynstr_Str(&ds), fp);
	    /* Now insert a new date */
	    fwrite(ctime(&t), 1, 24, fp); /* 24 is the length of
					   * the ctime string, minus
					   * the \n\0
					   */
	    fputs(mime_LF, fp);
	} else {
	    efclose(fp, "v7folder_Import");
	    RAISE(mfldr_err_FromSyntax, "v7folder_Import");
	}

	mmsg_Stream(mesg, &dp);
	TRY {
	    while (!dpipe_Eof(&dp))
		dputil_DpipeToFILE(&dp, fp);
	    if (FolderDelimited == self->zfolder->mf_type)
		efprintf(fp, "%s", msg_separator);
	} FINALLY {
	    mmsg_DestroyStream(&dp);
	    efclose(fp, "v7folder_Import");
	} ENDTRY;

	/* THE FOLLOWING RIPPED DIRECTLY OUT OF merge_folders(),
	 * WITH NO ATTEMPT TO MODULARIZE TO self->zfolder!
	 */

	current_folder = self->zfolder;		/* Make globals work */
	orig_offset = msg[msg_cnt]->m_offset;
	/* If load_folder() returns less than 0, there was some kind of
	 * loading error.  Since we're merging into a known "clean" folder
	 * parse errors in the loaded messages aren't critical.
	 * Only failure to read or write the tempfile is a serious problem.
	 * If load_folder() returns 1, the error was a parse error.
	 */
	if (load_folder(temp, 2, 0, NULL_GRP) < 1) {
	    if (!tmpf) {
		/* Catastrophic failure -- we destroyed at least
		 * our handle to the folder we were importing into!
		 */
		flush_msgs();
		current_folder->mf_last_size = 0;
		last_msg_cnt = 0;
		current_folder = save_folder;
		/* XXX Throw exception? */
		turnoff(glob_flags, IGN_SIGS);
		EXC_RETURNVAL(struct mmsg *, 0);
	    }
	} else {
	    turnoff(folder_flags, CORRUPTED); /* Ignore parse error */
	}

	msg[msg_cnt]->m_offset = orig_offset;
	newest_msg = last_msg_cnt;
	Debug("newest_msg = %d\n", newest_msg);
	if (isoff(glob_flags, IS_FILTER))
	    last_msg_cnt = msg_cnt;  /* for check_new_mail */
	Debug("msg_cnt = %d\n", msg_cnt);
	if (current_msg < 0) {
	    /* Merging into an empty folder -- make sure
	     * the file exists for later update!
	     */
	    if (Access(mailfile, F_OK) < 0) {
		FILE *mf = mask_fopen(mailfile, FLDR_APPEND_MODE);
		if (mf) (void) fclose(mf);
	    }
	    current_msg = 0;
	}
    } FINALLY {
	(void) unlink(temp);
	free(temp);
	current_folder = save_folder;
	turnoff(glob_flags, IGN_SIGS);
	dynstr_Destroy(&ds);
    } ENDTRY;

    new_mesg = core_to_v7message(self->zfolder->mf_msgs[newest_msg]);
    mmsg_Owner(new_mesg) = (struct mfldr *) self;
    mmsg_Num(new_mesg) = glist_Length(&(((struct mfldr *) self)->mmsgs));
    glist_Add(&(((struct mfldr *) self)->mmsgs), &new_mesg);

    return ((struct mmsg *) new_mesg);
}
EXC_END

static void
v7folder_DeleteMsg(self, arg)
    struct v7folder *self;
    spArgList_t arg;
{
    int n = spArg(arg, int);

    if (ison(self->zfolder->mf_msgs[n]->m_flags, EDITING)) {
	error(UserErrWarning,
	      catgets(catalog, CAT_CUSTOM, 252, "delete: message %d is being edited."), n);
	/* XXX Throw exception?? */
    } else {
	turnon(self->zfolder->mf_msgs[n]->m_flags, DELETE|DO_UPDATE);
	turnoff(self->zfolder->mf_msgs[n]->m_flags, NEW);
	turnon(self->zfolder->mf_flags, DO_UPDATE);
    }
}

static void
v7folder_Update(self, arg)	/* This is Z-Mail old-core specific code! */
    struct v7folder *self;
    spArgList_t arg;
{
    /* msg_folder save_folder = current_folder; */	/* See below */
    char command[7];	/* Need writable space */

    current_folder = self->zfolder;		/* Make globals work */
    TRY {
	strcpy(command, "update");
	if (cmd_line(command, NULL_GRP) != 0)
	    /* RAISE(??, ??) */;
    } FINALLY {
	/* Ideally what we'd do here is restore the old current folder,
	 * so that you can sync folders that are "in the background".
	 * However, the "update" command is going to call assorted
	 * refresh routines and in GUI mode repaint the main window 
	 * showing the folder that was just updated, so it could cause
	 * confusion to change the current folder back again now.
	 */
	/* current_folder = save_folder; */
    } ENDTRY;
}

void
v7folder_InitializeClass()
{
    if (!mfldr_class)
	mfldr_InitializeClass();
    if (v7folder_class)
	return;
    v7folder_class =
	spoor_CreateClass("v7folder",
			  "V7 subclass of mfldr",
			  mfldr_class,
			  (sizeof (struct v7folder)),
			  (void (*) NP((VPTR))) v7folder_initialize, 
			  (void (*) NP((VPTR))) 0);

    spoor_AddOverride(v7folder_class, m_mfldr_Import,
		      (GENERIC_POINTER_TYPE *) 0, v7folder_Import);
    spoor_AddOverride(v7folder_class, m_mfldr_DeleteMsg,
		      (GENERIC_POINTER_TYPE *) 0, v7folder_DeleteMsg);
    spoor_AddOverride(v7folder_class, m_mfldr_Update,
		      (GENERIC_POINTER_TYPE *) 0, v7folder_Update);

    v7message_InitializeClass();
}

struct v7folder *
core_to_mfldr(fldr)
    msg_folder *fldr;
{
    struct v7folder *result = v7folder_NEW();
    struct v7message *m;
    int i;

    result->zfolder = fldr;
    for (i = 0; i < fldr->mf_count; ++i) {
	m = core_to_v7message(fldr->mf_msgs[i]);
	mmsg_Owner(m) = (struct mfldr *) result;
	mmsg_Num(m) = i;
	glist_Add(&(((struct mfldr *) result)->mmsgs), &m);
    }
    return (result);
}
