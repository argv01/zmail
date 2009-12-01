#ifndef COPY_MSG_H
#define COPY_MSG_H

#include "zmail.h"
#include "glob.h"
#include "catalog.h"
#include "pager.h"
#include "linklist.h"
#include "prune.h"
#include "strcase.h"

#include <general.h>
#include <dynstr.h>
#include <except.h>

#include "zmindex.h"
#ifdef WIN16
#include <signal.h>
#endif

#include "zfolder.h"

struct cp_pos;
struct cp_pat;
struct cp_state;

/* copy_msg.c 21/12/94 16.20.46 */
extern struct cp_state *init_cps VP((struct cp_state *, ...));
extern void clean_cps P((struct cp_state *));
extern int can_show_inline P((const Attach *));
extern int summarize_attachment P((struct cp_state *, char **));
extern int scan_for_pat P((char *, struct cp_pat *));
extern void parse_pattern P((char *, struct cp_pat *));
extern int fix_position P((char *, long, struct cp_state *));
extern int ignore_this_line P((char *, struct cp_state *));
extern long copy_each_line P((char *, size_t, char **, struct cp_state *));
extern long copy_msg P((int, FILE *, u_long, const char *, unsigned long));
extern int status_line P((Msg *, u_long, char **));
extern msg_index *ix_gen P((int, u_long, msg_index *));
extern void ix_footer P((FILE *, long));
extern void ix_header P((FILE *, msg_index *));
extern char *ix_locate P((char *, char *));
extern int ix_folder P((FILE *, FILE *, u_long, int));
extern void ix_write P((int, msg_index *, u_long, FILE *));
extern int ix_parse P((Msg *, struct dynstr *, Source *));
extern void ix_destroy P((msg_index **, int));
extern int ix_match P((msg_index *, Msg *));
extern void ix_shrink P((msg_index *));
extern int ix_switch P((int));
extern int ix_init P((Source *, int, long));
extern void ix_confirm P((int));
extern int ix_load_msg P((Source *, int));
extern int ix_verify P((Source *, int, int));
extern long ix_reset P((Source *, int));
extern void ix_complete P((int));
extern int dumpAttach P((Attach *, FILE *, long));

#endif /* COPY_MSG_H */
