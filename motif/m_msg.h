#ifndef INCLUDE_MOTIF_M_MSG_H
#define INCLUDE_MOTIF_M_MSG_H



#include <X11/Intrinsic.h>
#include "attach/area.h"
#include "zfolder.h"
#include "zmframe.h"


struct AttachInfo;
struct Msg;

typedef struct _msg_data {
    u_long flags;
#ifdef STATUS_CHOICES
    u_long status;
    Widget status_kids;
#endif /* STATUS_CHOICES */
    struct Msg this_msg;
    int    this_msg_no;
    Widget hdr_fmt_w, text_w;
    AttachArea attach_area;
    struct AttachInfo *attach_info;
    Widget phone_tag_text_widgets;
#define MSG_NUM_PHONE_TAG_TEXT 12
#define MSG_NUM_PHONE_TAG_TOGGLE 8
#define MSG_LEN_PHONE_TAG_ID 16
    Widget phone_tag_text_widget[MSG_NUM_PHONE_TAG_TEXT];
    Widget phone_tag_toggle_widgets;
    Widget phone_tag_toggle_widget[MSG_NUM_PHONE_TAG_TOGGLE];
    Boolean    phone_tag_managed;
    char   phone_tag_id[MSG_LEN_PHONE_TAG_ID];
    Widget tag_it_text_widgets;
#define MSG_NUM_TAG_IT_TEXT 4
    Widget tag_it_text_widget[MSG_NUM_TAG_IT_TEXT];
    Boolean    tag_it_managed;
    Boolean    header_managed;
} msg_data;


extern Pixmap attach_label_pixmap;
extern ZcIcon attach_icon;

void  detach_attachment  P((Widget, ZmFrame));
void display_attachments P((Widget, ZmFrame));

void hdr_form_set_width P((Widget, unsigned));



#endif /* !INCLUDE_MOTIF_M_MSG_H */
