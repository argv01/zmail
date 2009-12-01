#ifndef _ZMINDEX_H_
#define _ZMINDEX_H_

/*
 * $RCSfile: zmindex.h,v $
 * $Revision: 2.3 $
 * $Date: 1995/07/14 04:25:06 $
 * $Author: schaefer $
 */

#include <zmsource.h>
#include <dynstr.h>

extern int ix_parse P ((Msg *, struct dynstr *, Source *));
extern int ix_init P((Source *, int, long));
extern int ix_verify P ((Source *, int, int));
extern int ix_load_msg P ((Source *, int));
extern long ix_reset P((Source *, int));
extern char *ix_locate P((char *, char *));
extern int ix_folder P ((FILE *, FILE *, u_long, int));
extern void ix_footer P ((FILE *, long));
extern void ix_header P ((FILE *, msg_index *));
extern void ix_write P ((int, msg_index *, u_long, FILE *));
extern int ix_folder P ((FILE *, FILE *, u_long, int));
extern int ix_switch P ((int));

#endif /* _ZMINDEX_H_ */
