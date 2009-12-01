/*
 * $RCSfile: filelist.h,v $
 * $Revision: 2.11 $
 * $Date: 1998/12/07 23:56:32 $
 * $Author: schaefer $
 *
 * $Log: filelist.h,v $
 * Revision 2.11  1998/12/07 23:56:32  schaefer
 * Merge NetManage's changes from Z-Mail 5.0 on the Motif-4-1-branch:
 *
 * Add IMAP support.  No more editorial comments, you know what I think.
 * Add LDAP support.  (Bite tongue.)
 * Add phone-tag and tag-it support for ECCO/ZM-Pro integration.
 *
 * Revision 2.10.4.1  1997/05/31 13:18:10  syd
 * Flesh out most of the lite IMAP folder GUI.
 *
 * Revision 2.10  1995/09/20 06:44:10  liblit
 * Get rather carried away and prototype a large number of zero-argument
 * functions.  Unlike C++, ANSI C has two extremely different meanings
 * for "()" and "(void)" in function declarations.
 *
 * Also prototype some parameter-taking functions, but not too many,
 * because there are only so many compiler warnings I can take.  :-)
 *
 * In printdialog_activate(), found in printd.c, change the order of some
 * operations so that a "namep" field gets initialized before it is first
 * referenced.  The UMR that this fixes corresponds to PR #6441.
 *
 * Revision 2.9  1994/04/26 05:23:58  bobg
 * Change the way compositions are sent in the Zmcot screen; use compcmds
 * instead of a non-interactive mail command, which was causing the
 * address-confirmation dialog to not allow itself to be canceled.  Make
 * filelist_class a wclass, not a class.
 *
 * Revision 2.8  1994/04/19  03:06:19  bobg
 * Fix wording in attachment-dialog prompts.  In filefinders, focus in
 * the Inputfield, not in the Filelist, and allow up, down, ^N, and ^P to
 * traverse the list.  Create a new widget class for this purpose named
 * Filebox (inheriting from Widget) with interactions filebox-up and
 * filebox-down.  Improve wording in various flavors of ask dialog.
 *
 * Revision 2.7  1994/01/26  19:02:54  bobg
 * Shorten CVS logs, delete dead code.  Mimic much more closely the Motif
 * version's behavior when canceling a composition.  Give names to the
 * Main screen's Folder and Messages fields (main-folder-field and
 * main-messages-field, to be precise).  Give the name
 * message-messages-field to the Message screen's Messages field.  Fix
 * core dump in class-based bindkeys that happen too early in the startup
 * sequence.
 */

#ifndef FILELIST_H
#define FILELIST_H

#include <spoor.h>
#include <spoor/splitv.h>
#include <spoor/cmdline.h>
#include <spoor/listv.h>
#include <zmail.h>

struct filelist {
    SUPERCLASS(spSplitview);
    char	       dir[1 + MAXPATHLEN];
#if defined( IMAP )
    char		imapdir[1 + MAXPATHLEN];
    int			useIMAP;
    void		*pFolder;
#endif	
    struct spListv  *list;
    struct spCmdline *choice;
    struct glist       names;
    void (*fn) NP((struct filelist *, char *));
    struct spoor      *obj;
    unsigned long      flags;
};

#define filelist_list(f)   (((struct filelist *) (f))->list)
#define filelist_choice(f) (((struct filelist *) (f))->choice)
#define filelist_fn(f)     (((struct filelist *) (f))->fn)
#define filelist_obj(f)    (((struct filelist *) (f))->obj)
#define filelist_flags(f)  (((struct filelist *) (f))->flags)

extern struct spWclass *filelist_class;

#define filelist_NEW() \
    ((struct filelist *) spoor_NewInstance(filelist_class))

extern int m_filelist_setDirectory;
extern int m_filelist_setPrompt;
extern int m_filelist_setChoice;
extern int m_filelist_fullpath;
extern int m_filelist_setFile;
extern int m_filelist_setDefault;

extern void filelist_InitializeClass P((void));

#endif /* FILELIST_H */
