/*
 * $RCSfile: msgf.h,v $
 * $Revision: 2.19 $
 * $Date: 1998/12/07 23:56:33 $
 * $Author: schaefer $
 */

#ifndef ZMLMSGFRAME_H
#define ZMLMSGFRAME_H

#include <spoor.h>
#include <dialog.h>

#include <spoor/textview.h>
#include <spoor/buttonv.h>

#include <callback.h>

#define ABNORMAL_STATE  -1
#define NORMAL_STATE    0
#define TAG_IT_STATE    1
#define PHONE_TAG_STATE 2

struct zmlmsgframe {
    SUPERCLASS(dialog);
    struct spTextview *headers;
    struct spTextview *body;
    struct spButtonv *aa;
    Msg *mymsg;
    int mymsgnum, paneschanged;
    u_long mymsgflags;
    ZmCallback panes_cb, msgwinhdrfmt_cb, autoformat_cb;
    msg_folder *fldr;
    int uniquifier;		/* allows callers to know whether
				 * contents have changed */
    struct spView *tree;
    struct {
        struct spCmdline *to, *subject, *cc, *bcc , *date_time, *signed_by;
        struct spCmdline *from, *of, *msg_date, *msg_time;
        struct spCmdline *area_code, *phone_number;
        struct spCmdline *extension, *fax_number;
        struct spCmdline *label1, *label2;
        struct spCmdline *toggles1, *toggles2;
    } extra_headers;
    int mymsgstate;
};

#define zmlmsgframe_mymsgnum(m) (((struct zmlmsgframe *) (m))->mymsgnum)
#define zmlmsgframe_body(m) (((struct zmlmsgframe *) (m))->body)
#define zmlmsgframe_uniquifier(m) (((struct zmlmsgframe *) (m))->uniquifier)

extern struct spWclass *zmlmsgframe_class;

extern struct spWidgetInfo *spwc_Message;

#define zmlmsgframe_NEW() \
    ((struct zmlmsgframe *) spoor_NewInstance(zmlmsgframe_class))

extern int m_zmlmsgframe_clear;
extern int m_zmlmsgframe_append;
extern int m_zmlmsgframe_setmsg;
extern int m_zmlmsgframe_countAttachments;
extern int m_zmlmsgframe_msg;

extern void zmlmsgframe_InitializeClass P((void));

#endif /* ZMLMSGFRAME_H */
