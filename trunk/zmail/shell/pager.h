/* pager.h	Copyright 1993, Z-Code Software Corp. */

#ifndef INCLUDE_PAGER_H
#define INCLUDE_PAGER_H

/*
 * $RCSfile: pager.h,v $
 * $Revision: 2.32 $
 * $Date: 1998/12/07 23:50:09 $
 * $Author: schaefer $
 */

#include "config.h"		/* for UNIX macro */
#include "general.h"
#include "gui_def.h"
#include "mimetype.h"
#include "zmflag.h"
#include <stdio.h>

struct FrameDataRec;
struct mgroup;


typedef struct zmPager zmPager, *ZmPager;
typedef struct zmPagerDevice zmPagerDevice, *ZmPagerDevice;
typedef enum {
    PgHelp,
    PgHelpIndex,
    PgText,
    PgMessage,
    PgNormal,
    PgInternal,
    PgPrint,
    PgOutput,
    PgTotalTypes
} ZmPagerType;

struct zmPager {
    ZmPagerDevice device;
    int (*write_func) NP((ZmPager, char *));
    void (*end_func) NP((ZmPager));
    char *title, *file, *prog, *printer;
    mimeCharSet charset;
#ifdef UNIX
    pid_t pid;
# ifdef GUI
    char *outfile, *errfile;
# endif /* GUI */
#endif /* UNIX */
    FILE *fp;
    u_long flags;
    int line_chct, line_ct, cur_msg;
    struct mgroup *msg_list;
# ifdef GUI
#  ifdef _WINDOWS
#   ifdef __cplusplus
    class ZPagerFrame *frame;
#   else /* !__cplusplus */
    struct ZPagerFrame *frame;
#   endif /* !__cplusplus */
    struct zmPrintData *print_data;
#  endif /* _WINDOWS */
#  ifdef VUI
    struct spView *pager;
#  endif /* VUI */
#  ifdef MAC_OS
#   ifdef __cplusplus
    class TFrameView *frame;
#   else /* !__cplusplus */
    struct TFrameView *frame;
#   endif /* !__cplusplus */
    GuiItem text_w;
#  endif /* MAC_OS */
    int tot_len;
#  ifdef MOTIF
    int last;
    char *text;
    struct FrameDataRec *frame;
    GuiItem text_w;
#  endif /* MOTIF */
# endif /* GUI */
    GENERIC_POINTER_TYPE *pagerdata;
};

struct zmPagerDevice {
    ZmPagerType type;
    void (*init_func) P((ZmPager));
    int (*write_func) P((ZmPager, char *));
    void (*end_func) P((ZmPager));
    u_long flags;
};

/* ZmPager->flags */
#define PG_RESTORE_ECHO     ULBIT(0)
#define PG_DONE	  	    ULBIT(1)
#define PG_ALREADY_ASKED    ULBIT(2) /* GUI: already asked user about length */
#define PG_EDITABLE	    ULBIT(3)
#define PG_FUNCTIONS	    ULBIT(4)
#define PG_PINUP  	    ULBIT(5)
#define PG_NO_PAGING        ULBIT(6)
#define PG_NO_GUI	    ULBIT(7)
#define PG_CANCELLED	    ULBIT(8) /* GUI: canceled before window came up */
#define PG_DEFAULT	    ULBIT(9) /* default [printer] was specified */
#define PG_ERROR	    ULBIT(10) /* an error occurred while paging */
#define PG_PARSE_INLINE	    ULBIT(11) /* GUI:  enable in-line detection */
#define PG_FLAGGED_INLINE   ULBIT(12) /* GUI:  block has been marked with a [..] summary */
#define PG_IN_UUENCODE	    ULBIT(13) /* GUI:  eliding a uuencode block */
#define PG_IN_BINHEX	    ULBIT(14) /* GUI:  eliding a binhex block */
#define PG_EMPTY	    ULBIT(15) /* true until something's written */

#define PGD_IGNORE_VARPAGER  ULBIT(0)

#define ZmPagerSetCharset(PG, X) ((PG)->charset = (X))
#define ZmPagerSetTitle(PG, X)	 ZSTRDUP((PG)->title, (X))
#define ZmPagerSetFile(PG, X)	 ZSTRDUP((PG)->file, (X))
#define ZmPagerSetProgram(PG, X) ZSTRDUP((PG)->prog, (X))
#define ZmPagerSetFlag(PG, X)	 (turnon((PG)->flags, (X)))
#define ZmPagerSetPrinter(PG, X) ZSTRDUP((PG)->printer, (X))
#define ZmPagerSetCurMsg(PG, X)	 ((PG)->cur_msg = (X))
#define ZmPagerSetMsgList(PG, X) ((PG)->msg_list = (X))
#define ZmPagerGetCurMsg(PG)	 ((PG)->cur_msg)
#define ZmPagerGetMsgList(PG)	 ((PG)->msg_list)
#define ZmPagerGetProgram(PG) 	 ((PG)->prog)
#define ZmPagerSetError(PG) 	 (turnon((PG)->flags, PG_ERROR))
#define ZmPagerClearError(PG) 	 (turnoff((PG)->flags, PG_ERROR))
#define ZmPagerCheckError(PG) 	 (ison((PG)->flags, PG_ERROR))
#define ZmPagerIsDone(PG) (ison((PG)->flags, PG_DONE))
#define ZmPagerCheckCancel(PG)	 (ison((PG)->flags, PG_CANCELLED))
#define ZmPagerSetData(PG, X)	 ((PG)->pagerdata = (X))
#define ZmPagerGetData(PG)	 ((PG)->pagerdata)
ZmPager ZmPagerStart P((ZmPagerType));
void ZmPagerSetType P((ZmPager, ZmPagerType));
void ZmPagerStop P((ZmPager));
void ZmPagerWrite P((ZmPager, char *));
void ZmPagerInitialize P((ZmPager));

extern ZmPager cur_pager;

char *printer_choose_one P((const char *));
long fiopager P((char *, long, char **, char *));
int c_more P((register const char *));
int pg_check_interrupt P((ZmPager, size_t));
int pg_check_max_text_length P((ZmPager));
#endif /* INCLUDE_PAGER_H */
