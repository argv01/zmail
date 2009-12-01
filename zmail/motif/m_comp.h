#ifndef INCLUDE_MOTIF_M_COMP_H
#define INCLUDE_MOTIF_M_COMP_H


#include <X11/Intrinsic.h>
#include "attach/area.h"
#include "config/features.h"

struct AttachInfo;
struct FrameDataRec;
struct AddressArea;


struct ComposeInterface {
    AttachArea attach_area;
    struct AttachInfo *attach_info;
    struct AddressArea *prompter;
    Widget alias_list;
#ifdef DYNAMIC_HEADERS
    struct FrameDataRec *dynamics;
#endif /* DYNAMIC_HEADERS */
    struct FrameDataRec *browser;
    struct FrameDataRec *options;
#define COMP_TEXT_W          0  /* Indices into the comp_items array.  */
#define COMP_SWAP_TEXT_W     1  /* text widget to use for autoformat */
#define COMP_ACTION_AREA     2
#define COMP_LOGGING	     3
#define COMP_LOGGING_TXT     4
#define COMP_RECORDING2	     5
#define COMP_RECORDING_TXT   6
#define COMP_AUTOSIGN2	     7
#define COMP_RECEIPT2	     8
#define COMP_AUTOFORMAT2     9
#define COMP_EDIT_HDRS2	     10
#define COMP_AUTOCLEAR	     11
#define COMP_VERBOSE	     12
#define COMP_RECORDUSER	     13
#define COMP_SYNCHRONOUS     14
#define COMP_ADDRESS_LABEL   15
#define COMP_ADDRESS_ACTIONS 16
#define COMP_DIRECTORY2      17
#define COMP_SORTER2         18
#define COMP_SENDCHECK2      19
#define COMP_VERIFY          20
#define NUM_COMP_ITEMS       21 /* Must one more than last item defined */
    Widget comp_items[NUM_COMP_ITEMS];
#define COMP_NUM_PHONE_TAG_TEXT 12
#define COMP_NUM_PHONE_TAG_TOGGLE 8
    Widget phone_tag_text_widgets;
    Widget phone_tag_text_widget[COMP_NUM_PHONE_TAG_TEXT];
    Widget phone_tag_toggle_widgets;
    Widget phone_tag_toggle_widget[COMP_NUM_PHONE_TAG_TOGGLE];
    Boolean    phone_tag_managed;
#define COMP_NUM_TAG_IT_TEXT 4
    Widget tag_it_text_widgets;
    Widget tag_it_text_widget[COMP_NUM_TAG_IT_TEXT];
    Boolean    tag_it_managed;
};


#endif /* ZComposeFrame */
