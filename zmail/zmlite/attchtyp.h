/* 
 * $RCSfile: attchtyp.h,v $
 * $Revision: 2.5 $
 * $Date: 1995/09/20 06:43:52 $
 * $Author: liblit $
 *
 * $Log: attchtyp.h,v $
 * Revision 2.5  1995/09/20 06:43:52  liblit
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
 * Revision 2.4  1994/05/10 15:41:15  bobg
 * Add Comment field to attachtype dialog.  Use
 * get_{create,compose}_{type,code}_keys as appropriate.  Erase the
 * status line when displaying a new message.  Fix textedit to work when
 * there's no selection in the get-selection and get-selection-position
 * cases; also make the two-argument form of get-selection-position work.
 *
 * Revision 2.3  1994/04/30  20:13:04  bobg
 * Use autotype API in Attachtype dialog.  Add doc strings for
 * interactions.  Don't doubly-define RefreshReason.  Show doc strings in
 * the `esc k' dialog.  Make "bindkey -d" work.
 *
 * Revision 2.2  1994/04/28  09:16:46  bobg
 * Add attachlist dialog.  Include needed files throughout.  Add "msg"
 * method to the message screen.  Add three new dialog names (for use in
 * the "dialog" Z-Script command): "attach", "attachfile", and
 * "attachnew".
 *
 * Revision 2.1  1994/04/28  01:26:22  bobg
 * Add new "attachtype" dialog; it's not yet complete.  Depend solely on
 * ../Files for naming LIBSOURCES.  Fix a type bug that the compiler
 * never reported in multikey.c.  Stop relying on the old attachdialog
 * code.
 */

#ifndef ATTCHTYP_H
# define ATTCHTYPE_H

#include <spoor.h>
#include <dialog.h>
#include <spoor/menu.h>
#include <spoor/cmdline.h>

struct attachtype {
    SUPERCLASS(dialog);
    struct spMenu *type, *encoding;
    struct spCmdline *comment;
    int commentedited;
    struct {
	char *type, *encoding;
    } selected;
};

#define attachtype_type(x) \
    (((struct attachtype *) (x))->selected.type)
#define attachtype_encoding(x) \
    (((struct attachtype *) (x))->selected.encoding)
#define attachtype_comment(x) \
    (((struct attachtype *) (x))->comment)

extern struct spWclass *attachtype_class;

extern int m_attachtype_select;

#define attachtype_NEW() \
    ((struct attachtype *) spoor_NewInstance(attachtype_class))

extern void attachtype_InitializeClass P((void));

extern struct attachtype *attachtype_Create P((int));

#endif /* ATTCHTYPE_H */
