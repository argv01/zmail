/* i18n.h	Copyright 1993 Z-Code Software Corp. */

#ifndef _I18N_H_
#define _I18N_H_

#include "osconfig.h"

#ifdef ZMAIL_INTL

#include "zmail.h"

/* Since this is all #ifdef'd, let's just define variable names here,
 * rather than cluttering up shell/vars.c.
 */

#define VarFromSpoolFilter	"i18n_read_mailbox"
#define VarToSpoolFilter	"i18n_write_mailbox"
#define VarToMtaFilter		"i18n_mail_transport"

#define I18N_READER		"cat"
#define I18N_WRITER		"cat"

extern int i18n_copy P ((char *, size_t, char *, char *));
extern int i18n_handle_error P ((int, char *, char *));
extern int i18n_initialize P ((void));
extern void i18n_mirror_spool P ((struct ftrack *, struct stat *));
extern void i18n_update_spool P ((struct ftrack **, struct stat *));
extern void i18n_mta_interface P ((char *));

#else /* not ZMAIL_INTL */

#define i18n_mta_interface(s) (*s=0)

#endif /* ZMAIL_INTL */
#endif /* _I18N_H_ */
