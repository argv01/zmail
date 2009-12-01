/* zmerror.h	Copyright 1991 Z-Code Software Corp. */

#ifndef _ZMERROR_H_
#define _ZMERROR_H_

#ifdef NOT_NOW

#include "linklist.h"

typedef struct {
    struct link e_link;
#define e_file	e_link.l_name
    int e_line;
    char *e_what;
} ErrorStack;

extern void
    zmError VP((PromptReason, char *, ...)),
    zmErrFlush P((void)),
    zmErrStack P((char *, int));

#endif /* NOT_NOW */

/* Error conditions -- general */

ZM_ENOMEM		/* Out of memory */
ZM_EGETPATH		/* Couldn't getpath() */
ZM_EISDIR		/* Wanted file, got directory */
ZM_EFOPEN		/* Couldn't open file  XXX */
ZM_EFSEEK		/* Couldn't reposition file  XXX */
ZM_EFSTAT		/* Couldn't stat() a file */
ZM_EFWRITE		/* Write failed or was short */

ZM_ENOMESSAGE		/* There aren't any messages */
ZM_EINTR		/* Operation was interrupted somehow */

/* Error conditions --  child management */

ZM_ENOHANDLER		/* Can't install signal handler */
ZM_EWAITLOST		/* Can't find waited-for child */
ZM_EWAITFAIL		/* Caught sigchild for no good reason */
ZM_EBADCATCH		/* Caught unexpected signal */
ZM_EODDCHILD		/* Reaped child that shouldn't exist */
ZM_ENOTACHILD		/* Process not a child of Z-Mail */
ZM_EUNSAFEBLOCK		/* Unsafe child blocking operation  XXX ??? */

ZM_ETOOMANYFDS		/* Ran out of file descriptors */
ZM_ENOTPOPENED		/* Tried to pclose non-popened stream */
ZM_ENOGETCWDBUF		/* Passed NULL buffer to getcwd() */

/* -- gui/gui_api.c */

ZM_EBADPIXMAP		/* Attempt to load rotten pixmap */
ZM_ECREATEPIX		/* Failed to create pixmap */

/* -- gui/zm_frame.c */

ZM_ENOFRAMEDATA		/* Frame has no data */
ZM_EBADFRAMEDATA	/* Passed invalid frame data */
ZM_EBADFRAMETYPE	/* Passed unknown frame type */
ZM_ENOPARENT		/* No parent widget available */
ZM_EFRAMESET		/* Tried to set unknown or read-only attribute */
ZM_EFRAMEGET		/* Tried to get unknown or write-only attribute */
ZM_ENOPIXMAP		/* Operation on pixmap when frame has none */

/* -- license/access.c */

ZM_EBADDEMO		/* Demo license invalid or expired */
ZM_ERDLICENSE		/* Couldn't read license entry */
ZM_EALTERED		/* License data changed */
ZM_ENOLICENSE		/* License simply doesn't exist */
ZM_EWRLICENSE		/* Couldn't write license entry */
ZM_EUSERLIMIT		/* Too many users in license entry */
ZM_ENEEDHOST		/* Only one of hostname or hostid given */
ZM_EBADLICENSE		/* License is invalid */

/* -- motif/m_alias.c */

ZM_EBADALIAS		/* Malformed alias name */
ZM_ENOALIAS		/* Missing alias name */
ZM_ENOADDRESS		/* Missing alias address */
ZM_EALIASFAILED		/* Couldn't install an alias */

/* -- motif/m_comp.c */

ZM_ENOFILENAME		/* Needed a file name but none given */
ZM_ENOSAVE		/* Can't save to a file */
ZM_ENOMSGLIST		/* Needed a message list but none given */
ZM_EUNKNOWN		/* Hmm ... */
ZM_EAPPEND		/* Can't append to file */
ZM_ELOSTMENU		/* Can't find a menu that should exist */

/* -- motif/m_comp2.c */

ZM_ESELECTION		/* No text selected when selection required */

/* -- motif/m_edit.c */

ZM_ESETPGRP		/* Can't change process group */
ZM_EFORK		/* Fork failed */

/* -- motif/m_fldrs.c */

ZM_EBADACTION		/* Strange ActionData */
ZM_EBADFOLDER		/* Invalid folder specified */
ZM_EISCURRENT		/* Not really an error ... */
ZM_EGETSTRING		/* Can't get string from text widget */
ZM_EFOLDERFMT		/* File not in folder format */
ZM_EBADGLOB		/* File globbing failed */

/* -- motif/m_fonts.c */

ZM_ENOFONTS		/* Can't load fonts */
ZM_ECURSOR		/* Can't create cursor */
ZM_EWIDGETNAME		/* Nameless or badly named widget */

/* -- motif/m_hdrs.c */

ZM_EBADHEADER		/* Malformed message header name */
ZM_ENOHEADER		/* Missing header name */
ZM_ENOBODY		/* Missing header body (not an error??) */
ZM_EHEADERFAILED	/* Couldn't install a header */

/* -- motif/m_help.c */

ZM_ENODIALOG		/* Dialog does not exist */

/* -- motif/m_lpr.c */

ZM_EPRINTCMD		/* Print command is empty */

/* -- motif/m_msg.c */

ZM_EMSGNUMBER		/* Message number changed */
ZM_EMSGEXIST		/* Message does not exist */
ZM_ENOTOGGLE		/* Tried to change a read-only toggle */
ZM_EDETACH		/* Tried to detach something not attached */
ZM_EFILETYPE		/* Tried to display unknown file type */
ZM_EATTACH		/* Something wrong with attachment */

/* -- motif/m_paint.c */

ZM_ENOTCOLOR		/* Can't use painter on a B&W display */
ZM_EBADCOLOR		/* Can't parse desired color */
ZM_ECOLORMAP		/* Can't allocate desired color */

/* -- motif/m_pkdate.c */

ZM_EMULTISRCH		/* Must perform function when searching all */
ZM_EBADDATE		/* Invalid date specified */
ZM_ESEARCH		/* Pick operation failed */

/* -- motif/m_pkpat.c */

ZM_ENOPATTERN		/* No pattern for a search */

/* -- motif/m_prompt.c */

ZM_ENOCHOICE		/* You must make a choice */

/* -- motif/m_script.c */

ZM_ENOBUTTON		/* Can't find button on panel */

/* -- motif/m_sort.c */

ZM_ESORTING		/* No (or bad) sort order provided */

/* -- motif/m_tbox.c */

ZM_EBADDIALOG		/* Invalid dialog number or name */
ZM_ENOBITMAP		/* No bitmap file for load operation */
ZM_ENOTGUI		/* GUI operation in non-gui mode */

/* -- motif/m_tool.c */

ZM_ERESOURCE		/* Something wrong with X Resources */
ZM_EONEMESSAGE		/* Operation limited to one message */
ZM_ESOMEMESSAGE		/* Operation needs at least one message */
ZM_ELASTMESSAGE		/* Ran out of messages to process */
ZM_ECMDFORMAT		/* Bad format string for command (internal) */

/* -- motif/m_vars.c */

ZM_EVARDATA		/* No variables data available (how??) */

/* -- msgs/addrs.c */

ZM_EDOMAIN		/* Illegal domain route */

/* -- msgs/attach.c */

ZM_ENOENCODE		/* Attachment requires encoding but none given */
ZM_EENCODING		/* Unknown attachment encoding */


/* MORE TO COME */

extern int zm_errno;

#define error	if (zmErrStack(__FILE__, __LINE__)) {;} else zmError

#endif /* _ZMERROR_H_ */
