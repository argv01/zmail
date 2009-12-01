/* 
 * $RCSfile: addrbook.h,v $
 * $Revision: 2.5 $
 * $Date: 1998/12/07 23:56:31 $
 * $Author: schaefer $
 *
 * $Log: addrbook.h,v $
 * Revision 2.5  1998/12/07 23:56:31  schaefer
 * Merge NetManage's changes from Z-Mail 5.0 on the Motif-4-1-branch:
 *
 * Add IMAP support.  No more editorial comments, you know what I think.
 * Add LDAP support.  (Bite tongue.)
 * Add phone-tag and tag-it support for ECCO/ZM-Pro integration.
 *
 * Revision 2.4.4.2  1997/03/13 20:57:03  bentley
 * Added widget to allow selection of ldap service from dialog.
 *
 * Revision 2.4.4.1  1997/03/12 20:00:43  bentley
 * Made changes to lite browser dialog to allow ldap searches.
 *
 * Revision 2.4  1995/09/20 06:43:47  liblit
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
 * Revision 2.3  1994/07/05 23:06:48  bobg
 * Brr!  It was starting to get pretty chilly out there on the
 * lite-fcs-freeze branch.  I sure am glad I'm merging back onto the
 * trunk now.
 *
 * If this causes any problems, see Carlyn, who has asked to deal
 * directly with difficulties arising from this merge.
 *
 * Revision 2.2.2.1  1994/05/21  21:06:14  bobg
 * Create new widget class, ComposeAddrbrowse (subclass of Addrbrowse),
 * for the Addrbrowse that comes up in a Compose screen (and has a
 * different action area).  Add get- and set-cursor-position subcommands
 * of textedit, fix a few other subcommands to behave according to spec.
 *
 * Revision 2.2  1994/02/24  19:19:24  bobg
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
 * Revision 2.1  1994/02/01  00:07:56  bobg
 * Create address browser dialog (addrbook.[ch]).  Find out the hard way
 * that the calling sequence for help() has changed.  Put a label on the
 * Compose Options dialog.  Do a resume_compose/suspend_compose each time
 * a compose dialog is entered, to make it the default for compcmd's.
 * Don't assume that canceling a menu only happens in a full-screen
 * dialog.  Correct a keybinding from cancel-menu to menu-cancel.  Make
 * the menu help string work correctly in popups with menus, too.  Create
 * the MenuPopup widget class, with the same interactions and keybindings
 * as MenuDialog.  Wrap a failure-prone region in a TRY...ENDTRY for
 * cleanup.  Don't duplicate a list of all widget classes; we can now ask
 * the spView class for them directly.
 *
 */

#ifndef ADDRBOOK_H
# define ADDRBOOK_H

#define MAX_LDAP_LINES          5
#define MAX_LDAP_HOSTS          16
#define MAX_LDAP_NAME           64
#define MAX_LDAP_SEARCH_PATTERN 512
#define LDAP_RESOURCES          "ldap.zmailrc"

#include <spoor.h>
#include <dialog.h>

#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/listv.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <spoor/list.h>

struct addrbook {
    SUPERCLASS(dialog);
    struct spTextview *instructions;
    struct spCmdline *recalls;
    struct spCmdline *pattern_array[MAX_LDAP_LINES];
    struct spWrapview *pattern_array_wrap[MAX_LDAP_LINES];
    struct spWrapview *service_wrap;
    struct spText *host_label;
    struct spListv *matches;
    struct spList *addrlist, *descrlist;
    struct spButtonv *display;
    struct spToggle *display_toggle;
    struct spMenu *service;
    struct {
        char *service;
    } selected;
};

/* Add field accessors */

/* Declare method selectors */

extern struct spWclass *addrbook_class;

extern struct spWidgetInfo *spwc_Addrbrowse;
extern struct spWidgetInfo *spwc_ComposeAddrbrowse;

extern void addrbook_InitializeClass P((void));

#define addrbook_NEW() \
    ((struct addrbook *) spoor_NewInstance(addrbook_class))

#endif /* ADDRBOOK_H */
