/* refresh.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _REFRESH_H_
#define _REFRESH_H_

#include "zctype.h"
#include "config/features.h"
#include "linklist.h"
#include "zmopt.h"
#include "callback.h"
#include <general.h>

extern char using_pop;
extern char pop_initialized;

#ifdef POP3_SUPPORT
extern void using_pop_cb P((char *, ZmCallbackData));
#endif /* POP3_SUPPORT */

#if defined( IMAP )
extern char using_imap;
extern char imap_initialized;
extern void using_imap_cb P((char *, ZmCallbackData));
#endif


/*
 * One thing we seem to do a lot of is keep track of file modification
 * times and check those files for change in mod time.  Lets set up a
 * little structure and some functions to handle this for us.
 */
typedef struct ftrack {
    struct link ft_link;
#define ft_name ft_link.l_name
    long ft_mtime;              /* Should be time_t */
    long ft_size;		/* Should be off_t */
    void_proc ft_call;          /* Function to call if file changed */
    VPTR ft_data;              /* Data passed to ft_call */
} Ftrack;

extern Ftrack *global_ftlist_head, **global_ftlist;

extern void ftrack_Init(/* ft, sbuf, callback, calldata */);
extern Ftrack *ftrack_CreateStat(/* name, sbuf, callback, calldata */);
extern Ftrack *ftrack_Create(/* name, callback, calldata */);
extern int ftrack_Do(/* ft, call */);
extern int ftrack_Stat(/* ft */);
extern int ftrack_Add(/* ftlist, ft */);
extern Ftrack *ftrack_Get(/* ftlist, name */);
extern void ftrack_Destroy(/* ft */);
extern int ftrack_Del(/* ftlist, ft */);
extern int ftrack_CheckFile(/* ftlist, name, call */);
extern void ftrack_CheckAll(/* ftlist, call */);

#define ftrack_Data(ft)		(ft)->ft_data
#define ftrack_Name(ft)		(ft)->ft_name
#define ftrack_Size(ft)		(ft)->ft_size
#define ftrack_Mtime(ft)	(ft)->ft_mtime

/*
 * Another thing we seem to need is a way to defer an action until a stable
 * point in the event loop or other ongoing operations.
 */
typedef enum {
    DeferredBuiltin,
    DeferredCommand,
    DeferredFileCheck,
    DeferredFileOp,
    DeferredSigFunc,
    DeferredSigTrap,
    DeferredSpecial
} DA_type;

typedef struct deferred_action {
    union {
	/* Note:  It is important that each member of this union
	 * begins with a "struct link" for linked-list insertion.
	 */
	struct link link;
	Option option;
	Ftrack ftrack;
    } da_u;
#define da_link da_u.link
#define da_option da_u.option
#define da_ftrack da_u.ftrack
    DA_type da_type;
    int da_finished;
} DeferredAction;


extern void init_periodic P((void));
#ifndef TIMER_API
extern time_t zm_current_time P((int));
extern int fetch_periodic P((int));
#endif /* !TIMER_API */
extern void shell_refresh P((void));
extern void trigger_actions P((void));

struct mgroup;

int exec_deferred P((int argc, char **argv, struct mgroup *list));
int cmd_deferred P((char *buf, struct mgroup *list));
int func_deferred P((void_proc, VPTR));
int add_deferred P((void_proc, VPTR));

#ifdef POP3_SUPPORT
void pop_init();
#endif /* POP3_SUPPORT */

#endif /* _REFRESH_H_ */
