/*
 * $RCSfile: choose1.h,v $
 * $Revision: 2.14 $
 * $Date: 1998/12/07 23:56:32 $
 * $Author: schaefer $
 *
 * $Log: choose1.h,v $
 * Revision 2.14  1998/12/07 23:56:32  schaefer
 * Merge NetManage's changes from Z-Mail 5.0 on the Motif-4-1-branch:
 *
 * Add IMAP support.  No more editorial comments, you know what I think.
 * Add LDAP support.  (Bite tongue.)
 * Add phone-tag and tag-it support for ECCO/ZM-Pro integration.
 *
 * Revision 2.13.4.2  1997/05/29 05:39:16  syd
 * Put #ifdef IMAP around code.
 * Add GUI to open, rename, remove, and message saves. Make everything
 * dynamic e.g. IMAP4 toggle shows only if use_imap is set.
 *
 * Revision 2.13.4.1  1997/05/28 20:46:05  bentley
 * Added some toggles for imap.
 *
 * Revision 2.13  1995/09/20 06:43:55  liblit
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
 * Revision 2.12  1994/05/03 02:43:02  bobg
 * Create AddressAsk subclass of InputAsk widget class, for the
 * address-confirmation dialog.  Make dead-letter test look at header
 * widgets as well as compose body.  Make compcmds bcc, cc, subject, and
 * to work.  Don't expose hidden action area when adding button(s).  Get
 * rid of extra -> in dynamic headers option menu.  Fix core bug in
 * finalizing dynamic headers dialog.  Solve the
 * cursor-jumping-when-toggle-selection bug, I think.  Give a message
 * when selecting addresses from COT directory dialog.  Clear group field
 * when clearing directory dialog.  Add N/ prefixes to fkeylabels.  Don't
 * allow attachtype dialog opening in wrong contexts.
 *
 * Revision 2.11  1994/02/24  19:19:33  bobg
 * Switch over to the new way of doing widget classes.  Previously,
 * hashtable lookups due to widget-class searches accounted for more than
 * 33% of Lite's running time.  The new widget class scheme eliminates
 * nearly all those lookups, replacing them with simple pointer
 * traversals and structure dereferences.  Profiling this version of Lite
 * reveals that we're back to the state where the main thing slowing down
 * Lite is the core itself.  Yay!
 *
 * Can you believe all this worked perfectly on the very first try?
 *
 * Revision 2.10  1994/02/18  19:58:30  bobg
 * The changes from the avoid-timer-api branch, plus:  don't allow
 * wprint("\n") to blank the status line.
 *
 * Revision 2.9  1994/01/28  05:32:01  bobg
 * Get rid of some dead code.  Special-case the chooseone dialog for the
 * NewFolder, AddFolder, and RenameFolder cases (causing extra widgets to
 * appear, to wit, the read-onlyness toggle or the "New name" field or
 * the mkdir/make-folder toggle).  Fix a bug in the filefinder which
 * caused directory info to be lost when the user input a relative
 * pathname.  Add the following widget-class subhierarchy:
 *     Ask
 * 	FileAsk
 * 		AddfolderAsk
 * 		NewfolderAsk
 * 		RenamefolderAsk
 *     Ask/InputAsk
 * 	ListAsk
 * 	MsgAsk
 * Add the following named widgets:
 *     ask-choices (List)
 *     ask-mkdir-mkfolder-rg (Radiogroup)
 *     ask-newname-field (Inputfield)
 *     ask-read-onlyness-rg (Radiogroup)
 * Understand "dialog newfolder" and "dialog renamefolder".
 *
 * Revision 2.8  1994/01/26  19:02:47  bobg
 * Shorten CVS logs, delete dead code.  Mimic much more closely the Motif
 * version's behavior when canceling a composition.  Give names to the
 * Main screen's Folder and Messages fields (main-folder-field and
 * main-messages-field, to be precise).  Give the name
 * message-messages-field to the Message screen's Messages field.  Fix
 * core dump in class-based bindkeys that happen too early in the startup
 * sequence.
 *
 * Revision 2.7  1994/01/14  18:17:04  bobg
 * Return a distinct value from a chooseone dialog when the user accepts,
 * so as not to confuse acceptance with dismissal (via the dialog-close
 * interaction).  Correctly count messages to be hidden when redrawing
 * summaries.  Get rid of the multikey queue and just initialize
 * spoor/zmlapp if the first multikey command shows up earlier than we
 * expected.
 *
 * Revision 2.6  1993/12/01  00:09:49  bobg
 *     It compiles, it links,
 *     I need a drink.
 *
 *     If you want it to run
 *     I'll need another one.
 *
 * Permit Z-Scriptable buttons and menus.
 */

#ifndef CHOOSEONE_H
#define CHOOSEONE_H

#include <spoor.h>
#include <dialog.h>

#include <zmail.h>
#include <dynstr.h>
#include <spoor/textview.h>
#include <spoor/listv.h>
#include <spoor/cmdline.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <filelist.h>

struct chooseone {
    SUPERCLASS(dialog);
    unsigned long flags;
    struct spTextview *query;
    struct filelist *files;
    struct spListv *choices;
    struct spCmdline *input;	/* not used with filelist */
    struct dynstr chosen;
    struct spButtonv *dir_folder; /* create dir/create folder radiogroup */
#if defined( IMAP )
    struct spButtonv *imap_local; /* imap/local radiogroup */
#endif
    struct spButtonv *rwro;	/* open read-write/read-only toggle */
    struct spCmdline *newname;	/* for rename dialog */
#if defined( IMAP )
    struct spToggle *rw_toggle, *mkdir_toggle, *imap_toggle;
#else
    struct spToggle *rw_toggle, *mkdir_toggle;
#endif
};

extern struct spWclass *chooseone_class;

extern struct spWidgetInfo *spwc_Ask;
extern struct spWidgetInfo *spwc_MsgAsk;
extern struct spWidgetInfo *spwc_ListAsk;
extern struct spWidgetInfo *spwc_FileAsk;
extern struct spWidgetInfo *spwc_InputAsk;
extern struct spWidgetInfo *spwc_NewfolderAsk;
extern struct spWidgetInfo *spwc_AddfolderAsk;
extern struct spWidgetInfo *spwc_RenamefolderAsk;
extern struct spWidgetInfo *spwc_CommandAsk;
extern struct spWidgetInfo *spwc_AddressAsk;

#define chooseone_NEW() \
    ((struct chooseone *) spoor_NewInstance(chooseone_class))

extern int m_chooseone_result;
extern int m_chooseone_setDirectory;

extern void chooseone_InitializeClass P((void));

enum chooseone_DeactivateReason {
    chooseone_Accept = dialog_DEACTIVATEREASONS,
    chooseone_FileOptionSearch,
    chooseone_Retry,
    chooseone_Omit,
    chooseone_DEACTIVATEREASONS
};

enum chooseone_ExtraFlags {
    chooseone_CreateFolder = (1 << 0), /* Create Folder dialog */
    chooseone_OpenFolder = (1 << 1), /* Open Folder dialog */
    chooseone_RenameFolder = (1 << 2), /* Rename Folder dialog */
    chooseone_Command = (1 << 3) /* Prompt for Z-Script command */
};

extern struct chooseone *chooseone_Create P((char *, char *,
					     unsigned long,
					     char **, int,
					     unsigned long));
#endif /* CHOOSEONE_H */
