/* m_msg.c     Copyright 1990 Z-Code Software Corp. */

#ifndef lint
static char	m_msg_rcsid[] =
    "$Id: m_msg.c,v 2.176 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"
#include "actionform.h"
#include "attach/area.h"
#include "buttons.h"
#include "catalog.h"
#include "dismiss.h"
#include "finder.h"
#include "zfolder.h"
#include "glob.h"
#include "addressArea/addressArea.h"
#include "m_comp.h"
#include "m_menus.h"
#include "m_msg.h"
#include "statbar.h"
#include "strcase.h"
#include "gui/zeditres.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "zm_motif.h"

/* #include <X11/cursorfont.h>	/* XXX Hack -- see do_hdr_form() */

#include <Xm/DialogS.h>
#include <Xm/Form.h>
#ifndef SANE_WINDOW
#include <Xm/PanedW.h>
#else /* SANE_WINDOW */
#include "xm/sanew.h"
#endif /* SANE_WINDOW */
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/MainW.h>
#include <Xm/Label.h>
#include <Xm/Text.h>
#include <Xm/ToggleBG.h>
#include <Xm/List.h>
#include <Xm/ScrolledW.h>

#define HAS_PAGERSW  ULBIT(0)
#define HAS_FACE     ULBIT(1)

#define NEW_BUTTON_NAME "New"
#define DISPLAY_BUTTON_NAME "Display"

#define SHORT_PINUP_TEXT_LEN 6

#ifdef SCO
#undef XtNumber
#define XtNumber(arr)	(sizeof(arr)/sizeof(arr[0]))
#endif /* SCO */

void display_attachments(), select_attachment();
extern void remove_callback_cb();

#ifdef STATUS_CHOICES
static char *status_choices[] = {
    "New", "Saved", "Replied-To", "Forwarded", "Printed", "Deleted"
};
static void reset_toggle();
#endif /* STATUS_CHOICES */

static int refresh_msg();
static void fill_ai(), do_attachment(),
       	    msg_hdr_fmt_cb(), examine_attachment(), ai_from_finder(),
	    ai_from_attach(), attach_menu_cb();
	    /* do_pin_up(), forward_w_cmnts(); */
#ifdef IXI_DRAG_N_DROP
static Boolean attdrag_convert();
#endif /* IXI_DRAG_N_DROP */
extern void do_hdr_form(), help_context_cb(), attach_filename();

static void msg_panes_cb P((Widget, ZmCallbackData));

extern AttachKey *code_keys, *type_keys;

/*
  Callback to prevent user from changing phone tag toddle buttons
*/
static void
phone_tag_toggle_callback(w, m_data, cbs)
Widget w;
msg_data *m_data;
XmToggleButtonCallbackStruct *cbs;
{
  XmToggleButtonSetState(w,!XmToggleButtonGetState(w),False);
}

/*
  These tables control the look of the phone tag headers.
*/
static char *phone_tag_toggle_list[] = {
    "Phoned\n ",
    "Call\nBack",
    "Returned\nCall",
    "Wants To\nSee You",
    "Was In\n ",
    "Will Call\nAgain",
    "Urgent\n ",
    NULL
  };
 
static char *phone_tag_label_list[] = {
    "To:",
    "From:",
    "Of:",
    "blank",
    "blank",
    "Date:",
    "Time:",
    "Area Code:",
    "No:",
    "Ext:",
    "Fax:",
    NULL
  };

static int phone_tag_label_length[] = {
    25,
    25,
    25,
    1,
    1,
    10,
    10,
    5,
    12,
    5,
    12,
    0
  };
static struct phone_tag_parse_struct {
    char *the_token;
    int  the_destination_offset;
  };
 
#define MAX_PHONE_TAG_TEXT_RETURNS 12
#define MAX_RETURN_LENGTH 64
static struct phone_tag_parse_struct phone_tag_parse_text[] = {
    "To:",        0,
    "From:",      1,
    "Of:",        2,
    "Date:",      5,
    "Time:",      6,
    "Area Code:", 7,
    "No.:",       8,
    "Ext.:",      9,
    "Fax #:",     10,
    "ID:",        11,
    NULL,         -1
  };

#define MAX_PHONE_TAG_TOGGLE_RETURNS 7
#define MAX_TOGGLE_RETURN_LENGTH 3
static struct phone_tag_parse_struct phone_tag_parse_checkbox[] = {
    "Phoned:",           0,
    "Call back:",        1,
    "Returned Call:",    2,
    "Wants to see you:", 3,
    "Was in:",           4,
    "Will call again:",  5,
    "Urgent:",           6,
    NULL,                -1
  };
 
/*
  These tables control the look of the tag it headers.
*/
static char *tag_it_label_list[] = {
    "Date: %s Time %s",
    "To:",
    "From: %s",
    NULL
  };
 
static int tag_it_label_type[] = {
    0,
    1,
    2,
    -1
  };
 
static int tag_it_label_length[] = {
    0,
    70,
    0,
    -1
  };

static void
phone_tag_parse_message_body(phone_tag_text_value, phone_tag_toggle_value)
char phone_tag_text_value[MAX_PHONE_TAG_TEXT_RETURNS][MAX_RETURN_LENGTH+1];
char phone_tag_toggle_value[MAX_PHONE_TAG_TOGGLE_RETURNS][MAX_TOGGLE_RETURN_LENGTH+1];
{
int i , j;
long pos;
char line[1028];
Boolean found_it;
char *loc;
  
/* Clear out the return fields */

  i = 0;
  while (phone_tag_parse_text[i].the_token != NULL)
    {
      strcpy(phone_tag_text_value[i],"");
      i++;
    }
  while (phone_tag_parse_checkbox[i].the_token != NULL)
    {
      strcpy(phone_tag_toggle_value[i],"No");
      i++;
    }

/* Go the beginning of the current message */

  fseek(tmpf, msg[current_msg]->m_offset, 0);

/* Search for the $EOM$ trailer marker */

    found_it = False;
    while ((pos = ftell(tmpf)) < msg[current_msg]->m_offset + msg[current_msg]->m_size &&
           fgets(line, sizeof (line), tmpf)) 
      {
        if (strstr(line,"$EOM$") != NULL)
          {
            found_it = True;
            break;
          }
      }

/* If found look for the other key words and their values */
    if (found_it)
      {
        while ((pos = ftell(tmpf)) < msg[current_msg]->m_offset + msg[current_msg]->m_size &&
               fgets(line, sizeof (line), tmpf)) 
          {
            i = 0;
            while (phone_tag_parse_text[i].the_token != NULL)
              {
                if ((loc = strstr(line,phone_tag_parse_text[i].the_token)) != NULL)
                  {
                    loc = loc + strlen(phone_tag_parse_text[i].the_token);
                    while (*loc == ' ')
                      loc ++;
                    j = 0;
                    while ((*loc != '\0') && (*loc != '\n'))
                      {
                        phone_tag_text_value[phone_tag_parse_text[i].the_destination_offset][j] = *loc; 
                        loc++;
                        if (j < MAX_RETURN_LENGTH)
                          j++;
                      }
                    phone_tag_text_value[phone_tag_parse_text[i].the_destination_offset][j] = '\0'; 
                    break;
                  }
                i++;
              }
            i = 0;
            while (phone_tag_parse_checkbox[i].the_token != NULL)
              {
                if ((loc = strstr(line,phone_tag_parse_checkbox[i].the_token)) != NULL)
                  {
                    loc = loc + strlen(phone_tag_parse_checkbox[i].the_token);
                    while (*loc == ' ')
                      loc ++;
                    j = 0;
                    while ((*loc != '\0') && (*loc != '\n'))
                      {
                        phone_tag_toggle_value[phone_tag_parse_checkbox[i].the_destination_offset][j] = *loc; 
                        loc++;
                        if (j < MAX_TOGGLE_RETURN_LENGTH)
                          j++;
                      }
                    phone_tag_toggle_value[phone_tag_parse_checkbox[i].the_destination_offset][j] = '\0';
                    break;
                  }
                i++;
              }
          }
      }

/* Go back to the beginning of the current message and then exit */

  fseek(tmpf, msg[current_msg]->m_offset, 0);
}
 
void
RebuildTagIt(frame)
ZmFrame frame;
{
    msg_data *m_data;
    int msg_no;
    int i , h;
    char the_string[128];
    char *toaddr, *frmaddr;
    struct tm *T;
    time_t senttime;
    char ampm[3];
    FrameTypeName type;
    short crt_win = SHORT_PINUP_TEXT_LEN;
 
    FrameGet(frame,
        FrameClientData, &m_data,
        FrameEndArgs);
    type = FrameGetType(frame);
    if (type == FramePinMsg)
      XtVaSetValues(m_data->text_w, XmNrows, crt_win, NULL);
    msg_no = m_data->this_msg_no;
    m_data->tag_it_managed = True;
    m_data->phone_tag_managed = False;
    m_data->header_managed = False;
    XtUnmanageChild(XtParent(XtParent(m_data->hdr_fmt_w)));
    XtUnmanageChild(m_data->phone_tag_text_widgets);
    XtUnmanageChild(m_data->phone_tag_toggle_widgets);
    XtManageChild(m_data->tag_it_text_widgets);

    i = 0;
    while (tag_it_label_list[i] != NULL)
      {
        if (tag_it_label_type[i] == 0)
          {
            strncpy(the_string,m_data->this_msg.m_date_sent,10);
            the_string[10] = '\0';
            sscanf(the_string,"%ld",&senttime);
            T = localtime(&senttime);
            h = T->tm_hour;
            if (h < 12)
              strcpy(ampm,"AM");
            else if (h == 12)
              strcpy(ampm,"PM");
            else
              {
                h = h - 12;
                strcpy(ampm,"PM");
              }
            sprintf(the_string,"Date: %d/%d/%02d Time: %d:%02d%s",T->tm_mon+1,T->tm_mday,T->tm_year%100,h,T->tm_min,ampm);
            XtVaSetValues(m_data->tag_it_text_widget[i],
            XmNlabelString, zmXmStr(the_string), NULL);
          }
        else if (tag_it_label_type[i] == 1)
          {
            toaddr = header_field(m_data->this_msg_no,"to");
            XmTextSetString(m_data->tag_it_text_widget[i],toaddr);
          }
        else if (tag_it_label_type[i] == 2)
          {
            frmaddr = header_field(m_data->this_msg_no,"from");
            sprintf(the_string,tag_it_label_list[i],frmaddr);
            XtVaSetValues(m_data->tag_it_text_widget[i],
            XmNlabelString, zmXmStr(the_string), NULL);
          }
        else
          ;
        i++;
      }
}
 
void
RebuildPhoneTag(frame)
ZmFrame frame;
{
    msg_data *m_data;
    int msg_no;
    char phone_tag_text_value[MAX_PHONE_TAG_TEXT_RETURNS][MAX_RETURN_LENGTH+1];
    char phone_tag_toggle_value[MAX_PHONE_TAG_TOGGLE_RETURNS][MAX_TOGGLE_RETURN_LENGTH+1];
    int i , h;
    char widget_name[32];
    Widget label_widget;
    Widget date_time_widget;
    struct tm *T;
    time_t senttime;
    char ampm[3];
    Boolean val;
    FrameTypeName type;
    short crt_win = SHORT_PINUP_TEXT_LEN;
 
    FrameGet(frame,
        FrameClientData, &m_data,
        FrameEndArgs);
    type = FrameGetType(frame);
    if (type == FramePinMsg)
      XtVaSetValues(m_data->text_w, XmNrows, crt_win, NULL);
    msg_no = m_data->this_msg_no;
    m_data->phone_tag_managed = True;
    m_data->tag_it_managed = False;
    m_data->header_managed = False;
    XtUnmanageChild(XtParent(XtParent(m_data->hdr_fmt_w)));
    XtUnmanageChild(m_data->tag_it_text_widgets);
    XtManageChild(m_data->phone_tag_text_widgets);
    XtManageChild(m_data->phone_tag_toggle_widgets);
    phone_tag_parse_message_body(phone_tag_text_value,phone_tag_toggle_value);
    if (strlen(phone_tag_text_value[11]) > 9)
      {
        sscanf(phone_tag_text_value[11],"%ld",&senttime);
        T = localtime(&senttime);
        h = T->tm_hour;
        if (h < 12)
          strcpy(ampm,"AM");
        else if (h == 12)
          strcpy(ampm,"PM");
        else
          {
            h = h - 12;
            strcpy(ampm,"PM");
          }
        sprintf(phone_tag_text_value[5],"%d/%d/%02d",T->tm_mon+1,T->tm_mday,T->tm_year%100);
        sprintf(phone_tag_text_value[6],"%d:%02d%s",h,T->tm_min,ampm);
      }
    i = 0;
    while (phone_tag_label_list[i] != NULL)
      {
        if (strcmp(phone_tag_label_list[i],"blank") == 0)
          {
            XtVaSetValues(m_data->phone_tag_text_widget[i],
            XmNlabelString, zmXmStr(" "), NULL);
          }
        else if (strcmp(phone_tag_label_list[i],"Date:") == 0)
          {
            XmTextSetString(m_data->phone_tag_text_widget[i],
                           phone_tag_text_value[i]);
            i++;
            XmTextSetString(m_data->phone_tag_text_widget[i],
                            phone_tag_text_value[i]);
          }
        else
          {
            XmTextSetString(m_data->phone_tag_text_widget[i],phone_tag_text_value[i]);
          }
        i++;
      }
    i = 0;
    while (phone_tag_toggle_list[i] != NULL)
      {
        if ((phone_tag_toggle_value[i][0] == 'Y') || 
            (phone_tag_toggle_value[i][0] == 'y'))
          val = True;
        else
          val = False;
        XtVaSetValues(m_data->phone_tag_toggle_widget[i],
        XmNset, val,
        NULL);
        i++;
      }
}
 
void
RebuildNormal(frame)
ZmFrame frame;
{
    msg_data *m_data;
    int msg_no;
    char *message_panes;
 
    FrameGet(frame,
        FrameClientData, &m_data,
        FrameEndArgs);
    msg_no = m_data->this_msg_no;
    m_data->header_managed = True;
    m_data->phone_tag_managed = False;
    m_data->tag_it_managed = False;
    XtUnmanageChild(m_data->tag_it_text_widgets);
    XtUnmanageChild(m_data->phone_tag_text_widgets);
    XtUnmanageChild(m_data->phone_tag_toggle_widgets);
    message_panes = value_of(VarMessagePanes);
    if (strstr(message_panes,"headers") != NULL)
      XtManageChild(XtParent(XtParent(m_data->hdr_fmt_w)));
}
 
void
RebuildMsgDisplay(frame)
ZmFrame frame;
{
    msg_data *m_data;
    int msg_no;
    Widget focus_widget;

    FrameGet(frame,
        FrameClientData, &m_data,
        FrameEndArgs);
    if (m_data == NULL) return;
    msg_no = m_data->this_msg_no;
    focus_widget = XmGetFocusWidget(m_data->text_w);
    if (header_field(msg_no,"X-Chameleon-TagIt"))
      RebuildTagIt(frame);
    else if (header_field(msg_no,"X-Chameleon-PhoneTag"))
      RebuildPhoneTag(frame);
    else
      RebuildNormal(frame);
    if (focus_widget != NULL)
      XmProcessTraversal(focus_widget,XmTRAVERSE_CURRENT);
    else
      SetTextInput(m_data->text_w);
}

Widget
msg_return_textw(frame)
ZmFrame frame;
{
    msg_data *data = (msg_data *)FrameGetClientData(frame);
    return data->text_w;
}

#include "bitmaps/attach.xbm"
ZcIcon attach_icon = {
    "attach_icon", 0, attach_icon_width, attach_icon_height, attach_icon_bits
};
Pixmap attach_label_pixmap;

#include "bitmaps/pager.xbm"
#include "bitmaps/readmsg.xbm"
ZcIcon pager_icons[] = {
    { "message_icon", 0, pager_width, pager_height, pager_bits },
    { "pinup_icon", 0, readmsg_width, readmsg_height, readmsg_bits }
};

#define TAB_TRAVERSAL "\
	Shift ~Meta ~Alt <Key>Tab:	PrimitivePrevTabGroup()\n\
	~Meta ~Alt <Key>Tab:	PrimitiveNextTabGroup()"

ZmFrame
DialogCreatePageMsg(w)
Widget w;
{
  if (msg_cnt > 0) {
    ask_item = w;
    display_msg(current_msg, display_flags());
    add_msg_to_group(&current_folder->mf_group, current_msg);
  }

  return 0;
}

ZmFrame
DialogCreatePinMsg(w)
Widget w;
{
    Widget junk;
    
    CreateMsgFrame(&junk, FramePinMsg);
    return (ZmFrame) 0;
}


void
set_msg_icon_and_title(w)
Widget w;
{
  static char *msg_icon_label, *msg_title_bar;
  Widget shell = GetTopShell(w);

  if (!msg_icon_label)
  {
    XtVaGetValues(shell, XtNiconName, &msg_icon_label, NULL);
    if (!msg_icon_label || !*msg_icon_label)
      msg_icon_label = catgets(catalog, CAT_MOTIF, 922, "Msg %m");
    msg_icon_label = savestr(msg_icon_label);
  }
  if (!msg_title_bar) {
    XtVaGetValues(shell, XtNtitle, &msg_title_bar, NULL);
    if (!msg_title_bar || !*msg_title_bar)
      msg_title_bar = catgets(catalog, CAT_MOTIF, 923, "Message %m");
    msg_title_bar = savestr(msg_title_bar);
  }

  /* Two calls to XtVaSetValues because format_prompt() uses static data */
  XtVaSetValues(shell, XtNtitle,
		format_prompt(current_folder, msg_title_bar), NULL);
  XtVaSetValues(shell, XtNiconName,
		format_prompt(current_folder, msg_icon_label), NULL);
}

ZmFrame
CreateMsgFrame(sw, type)
Widget *sw;
FrameTypeName type;
{
    Arg args[16];
    int nargs;
    char *cw = value_of(VarCrtWin);
    short crt_win = cw? atoi(cw) : 0;
    Widget action_area, shell, main_w, menu_bar, pane, w;
    u_long frame_flags;
    ZmFrame newframe;
    ZmCallback cb;
    msg_data *m_data;
    char *wname;
    int del_response;
    char m[8], buf[32];
    ZcIcon *icon;

    if (msg_cnt <= 0) return 0;

    (void) sprintf(m, "%d", current_msg+1);
    (void) sprintf(buf, catgets( catalog, CAT_MOTIF, 290, "Message %s" ), m);
    frame_flags = FRAME_SHOW_ICON | FRAME_SHOW_FOLDER | FRAME_SUPPRESS_ICON;
    if (type == FramePageMsg) {
	wname = "message_window";
	del_response = XmUNMAP;
	icon = &pager_icons[0];
	turnon(frame_flags, FRAME_EDIT_LIST|FRAME_UNMAP_ON_DEL);
    } else {
	wname = "pinup_window";
	del_response = XmDESTROY;
	icon = &pager_icons[1];
	turnon(frame_flags, FRAME_SHOW_LIST|FRAME_DESTROY_ON_DEL);
    }
    shell = XtVaAppCreateShell(wname,
	ZM_APP_CLASS, applicationShellWidgetClass, display,
	XmNdeleteResponse,	del_response,
#ifdef OLD_WAY
	XmNtitle,		buf,
	XmNiconName,		m,
#endif /* OLD_WAY */
	NULL);
#ifndef OLD_WAY
    set_msg_icon_and_title(shell);
#endif /* OLD_WAY */
    EditResEnable(shell);

    /* Create MainWindow widget -- contains menubar and pane */
    main_w = XtVaCreateManagedWidget("message_window",
	xmMainWindowWidgetClass, shell,
        XmNshadowThickness,	 0,
	NULL);
    DialogHelpRegister(main_w, "Message Display Window");

#ifdef SANE_WINDOW
    pane = XtCreateWidget("pane", zmSaneWindowWidgetClass, main_w, (Arg*)0, 0);
#else /* SANE_WINDOW */
    pane = XtCreateWidget("pane", xmPanedWindowWidgetClass, main_w, NULL, 0);
#endif /* SANE_WINDOW */
    cb = ZmCallbackAdd(VarMessagePanes, ZCBTYPE_VAR, msg_panes_cb, pane);
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, cb);
    m_data = XtNew(msg_data);
    m_data->this_msg = *(msg[current_msg]);
    m_data->this_msg_no = current_msg;
    m_data->flags = 0L;
    if (type == FramePageMsg)
	ZmCallbackAdd(VarMsgWinHdrFmt, ZCBTYPE_VAR, msg_hdr_fmt_cb, m_data);

    newframe = FrameCreate("message_window", /* used only for help lookup */
	type, tool,
	FrameIcon,		icon,
	FrameRefreshProc,	refresh_msg,
	FrameFlags,		frame_flags,
	FrameMsgString,		m,
	FrameTitle,		buf,
	FrameIsPopup,		False,
	FrameClass,		NULL,
	FrameChildClass,	NULL,
	FrameChild,		&pane,
	FrameClientData,	m_data,
	FrameFreeClient,	XtFree,
	FrameEndArgs);
    XtVaSetValues(main_w, XmNuserData, newframe, NULL);

    DialogHelpRegister(GetNthChild(pane, 0), "Message Folder Panel");

    menu_bar = BuildMenuBar(main_w, MSG_WINDOW_MENU);
    XtManageChild(menu_bar);

    ToolBarCreate(main_w, MSG_WINDOW_TOOLBAR, True);
    {
	StatusBar *status = StatusBarCreate(main_w);
	FrameSet(newframe, FrameStatusBar, status, FrameEndArgs);
	statusBar_SetHelpKey(status, "Message Status Bar");
    }

#ifndef SANE_WINDOW
    XtVaSetValues(pane,
	/* XmNseparatorOn, False, */
	XmNsashWidth,  1,
	XmNsashHeight, 1,
	NULL);
#endif /* !SANE_WINDOW */

    if (type == FramePageMsg) {
	Widget msgs_item = FrameGetMsgsItem(newframe);
	turnon(m_data->flags, HAS_PAGERSW);
	XtAddCallback(msgs_item, XmNactivateCallback,
		      (XtCallbackProc) do_cmd_line, "type %s");
    }

#ifdef STATUS_CHOICES
    m_data->status = msg[current_msg]->m_flags;
    m_data->status_kids = CreateToggleBox(pane, False, True, False,
	reset_toggle, &m_data->status, catgets( catalog, CAT_MOTIF, 292, "Status:" ), status_choices,
	XtNumber(status_choices));
    XtManageChild(m_data->status_kids);
#endif /* STATUS_CHOICES */
    
    {
	Widget layout = XtVaCreateManagedWidget("upper", xmFormWidgetClass, pane,
						XmNfractionBase, 12,
#ifndef OZ_DATABASE
 						XmNresizePolicy, XmRESIZE_NONE,
#endif /* !OZ_DATABASE */
#ifndef SANE_WINDOW
						XmNskipAdjust,       True,
#endif /* !SANE_WINDOW */
						0);
	
#ifndef __STDC__ /* no automatic aggregate initialization */
	static
#endif /* no automatic aggregate initialization */
	Arg args[] = { { XmNeditable,			False },
		       { XmNcursorPositionVisible,	False },
		       { XmNtraversalOn,      		False },
		       { XmNautoShowCursorPosition,	False },
		       { XmNresizeHeight,		True },
		       { XmNeditMode,			XmMULTI_LINE_EDIT },
		       { XmNtopAttachment,		XmATTACH_FORM },
		       { XmNbottomAttachment,		XmATTACH_FORM },
		       { XmNleftAttachment,		XmATTACH_FORM },
		       { XmNrightAttachment,		XmATTACH_POSITION },
		       { XmNrightPosition,		12 },
		       { XmNscrollingPolicy,		XmAUTOMATIC } };

	m_data->hdr_fmt_w = XmCreateScrolledText(layout, "message_header", args, XtNumber(args));
	XtManageChild(m_data->hdr_fmt_w);
	XtManageChild(XtParent(m_data->hdr_fmt_w));
	DialogHelpRegister(XtParent(m_data->hdr_fmt_w), "Message Headers Panel");
	    
	m_data->attach_area = create_attach_area(layout, type);
	DialogHelpRegister(attach_area_widget(m_data->attach_area), "Message Attachment Panel");

	XtManageChild(layout);
    }
    
#ifndef SANE_WINDOW
    { char *p; int lines = 1; /* hackhackhack...*/
    for (p = index(value_of(VarMsgWinHdrFmt), '\n'); p; p = index(p+1, '\n'))
	lines++;
    SetPaneMinByFontHeight(m_data->hdr_fmt_w, lines);
    }
#endif /* !SANE_WINDOW */

    /* Tag-Tt area */
    {
      m_data->tag_it_text_widgets = XtVaCreateWidget("tag_it_text_rc", xmRowColumnWidgetClass,pane,
          XmNorientation, XmVERTICAL,
          XmNmarginHeight, 0,
          XmNnumColumns, 1,
          XmNpacking, XmPACK_COLUMN,
          NULL);
      {
        int i;
        char widget_name[32];
        Widget label_widget;

        i = 0;
        while (tag_it_label_list[i] != NULL)
          {
            if (tag_it_label_type[i] == 0)
              {
                sprintf(widget_name,"tag_it_text_%d",i);
                m_data->tag_it_text_widget[i] =
                  XtVaCreateManagedWidget(
                    widget_name, xmLabelGadgetClass,
                    m_data->tag_it_text_widgets, 
                    NULL);
              }
            else if (tag_it_label_type[i] == 1)
              {
                sprintf(widget_name,"tag_it_text_%d",i);
                m_data->tag_it_text_widget[i] =
                  CreateLabeledTextSetWidth(
                    widget_name,m_data->tag_it_text_widgets,
                    tag_it_label_list[i],
                    CLT_HORIZ|CLT_REPLACE_NL,35,tag_it_label_length[i],
                    &label_widget);
                XtVaSetValues(m_data->tag_it_text_widget[i], XmNeditable, False,
                NULL);
              }
            else if (tag_it_label_type[i] == 2)
              {
                sprintf(widget_name,"tag_it_text_%d",i);
                m_data->tag_it_text_widget[i] =
                  XtVaCreateManagedWidget(
                    widget_name, xmLabelGadgetClass,
                    m_data->tag_it_text_widgets, 
                    NULL);
              }
            else
              ;
            i++;
          }
       }
    }
 
    /* Phone-Tag text area */
    if (pane) {
        m_data->phone_tag_text_widgets = XtVaCreateWidget(
            "phone_tag_text_rc", xmRowColumnWidgetClass, pane,
            XmNorientation, XmVERTICAL,
            XmNmarginHeight, 0,
            XmNnumColumns, 2,
            XmNpacking, XmPACK_COLUMN,
            NULL);
        {
            int i;
            char widget_name[32];
            Widget col1, col2, parent_widget;
            Widget label_widget;
            Widget date_time_widget;
  
            col1 = XtVaCreateWidget("phone_tag_col1",
                xmRowColumnWidgetClass,
                m_data->phone_tag_text_widgets,
                XmNorientation, XmVERTICAL,
                XmNmarginHeight, 0,
                NULL);
            col2 = XtVaCreateWidget("phone_tag_col2",
                xmRowColumnWidgetClass,
                m_data->phone_tag_text_widgets,
                XmNorientation, XmVERTICAL,
                XmNmarginHeight, 0,
                NULL);
            i = 0;
            while (phone_tag_label_list[i] != NULL) {
                if (i <= 4)
                    parent_widget = col1;
                else
                    parent_widget = col2;
                if (strcmp(phone_tag_label_list[i],"blank") == 0) {
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    m_data->phone_tag_text_widget[i] =
                    XtVaCreateManagedWidget(
                        widget_name, xmLabelGadgetClass,
                        parent_widget, NULL);
                }
                else if (strcmp(phone_tag_label_list[i],"Date:") == 0) {
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    date_time_widget = XtVaCreateWidget("date_time_rc",
                        xmRowColumnWidgetClass,
                        parent_widget,
                        XmNorientation, XmHORIZONTAL,
                        XmNmarginHeight, 0,
                        NULL);
                    m_data->phone_tag_text_widget[i] =
                        CreateLabeledTextSetWidth(widget_name,
                            date_time_widget,
                            phone_tag_label_list[i],
                            CLT_HORIZ|CLT_REPLACE_NL, 40,
                            phone_tag_label_length[i],
                            &label_widget);
                    XtVaSetValues(m_data->phone_tag_text_widget[i], 
                        XmNeditable, False, NULL);
                    i++;
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    m_data->phone_tag_text_widget[i] =
                        CreateLabeledTextSetWidth("phone_tag_time",
                            date_time_widget, phone_tag_label_list[i],
                            CLT_HORIZ|CLT_REPLACE_NL, 40,
                            phone_tag_label_length[i],
                            &label_widget);
                    XtVaSetValues(m_data->phone_tag_text_widget[i], 
                        XmNeditable, False, NULL);
                    XtManageChild(date_time_widget);
                }
                else {
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    m_data->phone_tag_text_widget[i] =
                        CreateLabeledTextSetWidth(
                            widget_name,parent_widget,
                            phone_tag_label_list[i],
                            CLT_HORIZ|CLT_REPLACE_NL, 75,
                            phone_tag_label_length[i],
                            &label_widget);
                    XtVaSetValues(m_data->phone_tag_text_widget[i], 
                        XmNeditable, False, NULL);
                }
                i++;
            }
            XtManageChild(col1);
            XtManageChild(col2);
        }
    }

    XtOverrideTranslations(m_data->hdr_fmt_w,
	XtParseTranslationTable(TAB_TRAVERSAL));

    if (type == FramePinMsg && pager_textsw)
	XtVaGetValues(pager_textsw, XmNrows, &crt_win, NULL);

    /* pf Sun Jul 11 19:57:32 1993
     * we need this in order for SetPaneMaxAndMin to be able to get
     * the size of this pane, since the ScrolledWindow won't tell us
     * what its height is.
     */
    w = XtVaCreateManagedWidget(NULL, xmFormWidgetClass, pane, NULL);
    nargs = XtVaSetArgs(args, XtNumber(args),
		XmNscrollVertical,		True,
		/* XmNscrollHorizontal,		False, */
		XmNcursorPositionVisible,	False,
		XmNeditMode,			XmMULTI_LINE_EDIT,
		XmNeditable,			False,
		/* XmNwordWrap,			True, */
		XmNblinkRate,			0,
		XmNtopAttachment,   	        XmATTACH_FORM,
		XmNbottomAttachment,		XmATTACH_FORM,
		XmNleftAttachment,		XmATTACH_FORM,
		XmNrightAttachment,		XmATTACH_FORM,
		/* next arg only included if crt_win is nonzero */
		XmNrows,			crt_win,
		NULL);
    /* XtSetArg(args[7], XmNcolumns, 80); */
    *sw = m_data->text_w =
	XmCreateScrolledText(w, "message_text", args, nargs - !crt_win);
    (void) BuildPopupMenu(m_data->text_w, MESSAGE_TEXT_POPUP_MENU);
#ifdef SANE_WINDOW
    XtVaSetValues(w, ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */

    /* Phone-Tag toggle area */
    {
      m_data->phone_tag_toggle_widgets = XtVaCreateWidget("phone_tag_toggle_rc", xmRowColumnWidgetClass,pane,
          XmNorientation, XmHORIZONTAL,
          XmNspacing,    0,
          XmNmarginHeight, 0,
          XmNadjustLast, False,
          XmNpacking, XmPACK_COLUMN,
          NULL);
      {
        int i;
        char widget_name[32];
        Widget label_widget;
        Boolean val;
        i = 0;
        while (phone_tag_toggle_list[i] != NULL)
          {
            sprintf(widget_name,"phone_tag_toggle_%d",i);
            m_data->phone_tag_toggle_widget[i] =
              XtVaCreateManagedWidget(
              widget_name, xmToggleButtonWidgetClass,
              m_data->phone_tag_toggle_widgets, 
              NULL);
            XtVaSetValues(m_data->phone_tag_toggle_widget[i],
            XmNmarginHeight , 0 ,
            XmNlabelString, zmXmStr(phone_tag_toggle_list[i]), 
            NULL);
            XtAddCallback(m_data->phone_tag_toggle_widget[i],
              XmNvalueChangedCallback,
              (void_proc)phone_tag_toggle_callback, (msg_data *) m_data);
            i++;
          }
       }
    }

    DialogHelpRegister(w, "Message Body");
    XtOverrideTranslations(*sw, XtParseTranslationTable(TAB_TRAVERSAL));
    XtManageChild(*sw);

    action_area = CreateActionArea(pane, (ActionAreaItem *)NULL, 0, "Message Button Panel");

    m_data->attach_info = NULL;
    
    do_hdr_form(newframe); /* Keep the format spec in one place */

    if (type == FramePageMsg) pager_textsw = *sw;

#ifdef OZ_DATABASE
    /* able to avoid hairy geometry recalculations */
    XtSetMappedWhenManaged(pane, False);
#endif /* OZ_DATABASE */
    
    XtManageChild(pane);
    XtManageChild(main_w);

    gui_install_all_btns(MSG_WINDOW_BUTTONS, NULL, action_area);

    ZmCallbackCallAll(VarMessagePanes, ZCBTYPE_VAR, ZCB_VAR_SET,
		      value_of(VarMessagePanes));

    FramePopup(newframe);

    m_data->tag_it_managed = False;
    m_data->phone_tag_managed = False;
    m_data->header_managed = False;
    RebuildMsgDisplay(newframe);

    /* workaround various motif bugs by doing this now... */
    draw_attach_area(m_data->attach_area, newframe);
    
#ifdef OZ_DATABASE
    /* able to avoid hairy geometry recalculations */
    XtSetMappedWhenManaged(pane, True);
#endif /* OZ_DATABASE */

    return newframe;
}

#ifdef NOT_NOW
void
do_next(w, newframe, cbs)
Widget w;
ZmFrame newframe;
XmAnyCallbackStruct *cbs;
{
    char *p;
    msg_data *m_data;

    timeout_cursors(True);

    FrameGet(newframe,
	FrameFolder,    &current_folder,
	FrameClientData,&m_data,
	FrameEndArgs);
    current_msg = m_data->this_msg_no;
    ask_item = w;
    p = XtName(w);
    /* Pass current_folder->mf_group so it will be cleared */
    (void) cmd_line(zmVaStr("\\%s", *p == 'N'? "next" : "previous"),
	&current_folder->mf_group);
    FrameSet(FrameGetData(hdr_list_w),
	FrameMsgList, &current_folder->mf_group,
	FrameEndArgs);
    gui_refresh(current_folder, REDRAW_SUMMARIES);

    timeout_cursors(False);
}
#endif /* NOT_NOW */

void
do_msg_cmd(w, cmd, cbs)
Widget w;
char *cmd;
XmAnyCallbackStruct *cbs;
{
    msg_data *m_data;
    msg_folder *save_fldr = current_folder;

    timeout_cursors(True);

    FrameGet(FrameGetData(w),
	FrameFolder,    &current_folder,
	FrameClientData,&m_data,
	FrameEndArgs);
    current_msg = m_data->this_msg_no;
    ask_item = w;
    /* Pass current_folder->mf_group so it will be cleared */
    (void) cmd_line(zmVaStr("\\%s", cmd), &current_folder->mf_group);
    if (save_fldr == current_folder)
	FrameSet(FrameGetData(hdr_list_w),
	    FrameMsgList, &current_folder->mf_group,
	    FrameEndArgs);
    else
	current_folder = save_fldr;
    gui_refresh(current_folder, REDRAW_SUMMARIES);

    timeout_cursors(False);
}

#ifndef NO_X_FACE
#include "xface.h"

Pixmap
set_face(display, widget, msg_no)
Display *display;
Widget widget;
int msg_no;
{
    char *mt_info;
    Pixel fg, bg;
    XImage *image;
    XGCValues gcv;
    GC gc;
    Pixmap pix01, pix;
    Screen *screen = XtScreen(widget);
    Window root = RootWindowOfScreen(screen);
    Pixel maxpix = (Pixel) (1 << DefaultDepthOfScreen(screen));
    char *fbuf;

    if (!(mt_info = header_field(msg_no, "x-face")))
	return (Pixmap)0;

    if (!(fbuf = (char *) malloc(512)))
	return (Pixmap)0;

    if (uncompface(strcpy(fbuf, mt_info)) < 0) {
	xfree(fbuf);
	return (Pixmap)0;
    }

    /* Create an XImage from the bits decyphered from X-Face header... */
    image = XCreateImage(display, DefaultVisualOfScreen(screen), 1,
	XYPixmap, 0, fbuf, XF_WIDTH, XF_HEIGHT, 8, 0);
    /* The data is big-endian. */
    image->byte_order = MSBFirst;
    image->bitmap_bit_order = MSBFirst;
    image->bitmap_unit = 8;
    /* Create a depth-1 pixmap for it. */
    pix01 = XCreatePixmap(display, root, XF_WIDTH, XF_HEIGHT, 1);
    gcv.foreground = 1;
    gcv.background = 0;
    gc = XCreateGC(display, pix01, GCForeground | GCBackground, &gcv);

    /* Transfer the image from the client side to the server (rop to pixmap). */
    XPutImage(display, pix01, gc, image, 0, 0, 0, 0, XF_WIDTH, XF_HEIGHT);
    XFreeGC(display, gc);
    XDestroyImage(image);	/* FREES fbuf TOO!! */

    /* We must return a pixmap of the same depth as screen, and using fg and
     * bg instead of 1 and 0.  So create a pixmap of the right depth and
     * copy the pixmap into it.
     */
    fg = 1234567890L;
    XtVaGetValues(widget, XmNforeground, &fg, XmNbackground, &bg, NULL);
    if (fg >= maxpix) {
	/* Not all widgets have a foreground resource, unfortunately.
	 * Try the parent.
	 */
	XtVaGetValues(XtParent(widget), XmNforeground, &fg, NULL);
	if (fg >= maxpix) {
	    /* Nope that didn't work either.  Fall back on black. */
	    fg = BlackPixelOfScreen(screen);
	}
    }
    pix = XCreatePixmap(display, root, XF_WIDTH, XF_HEIGHT,
	    DefaultDepthOfScreen(screen));
    gcv.foreground = fg;
    gcv.background = bg;
    gc = XCreateGC(display, pix, GCForeground | GCBackground, &gcv);
    XCopyPlane(display, pix01, pix, gc, 0,0,
	XF_WIDTH, XF_HEIGHT, 0,0, 1L);
    XFreePixmap(display, pix01);
    XFreeGC(display, gc);

    return pix;
}
#endif /* NO_X_FACE */

#ifdef IXI_DRAG_N_DROP
static Boolean
attdrag_convert(widget, clientdata, nameP)
Widget widget;
XtPointer clientdata;
char **nameP;
{
    ZmFrame frame;
    msg_data *md;
    int att_num = -1;
    Attach *a;

    /* Get the frame. */
    frame = FrameGetData(widget);

    /* Get the message data. */
    md = (msg_data *) FrameGetClientData(frame);

    /* Get the attachment number. */
    XtVaGetValues(XtParent(widget), R_USER_DATA, &att_num, NULL);

    /* Get the Attach struct. */
    a = md->this_msg.m_attach;
    a = (Attach *)retrieve_nth_link(a, att_num+1);

    /* Detach. */
    if (cmd_line(zmVaStr(
	    "builtin detach -part %d %d", att_num, md->this_msg_no+1),
	    NULL_GRP) == -1)
	return False;
    a->a_flags |= AT_TEMPORARY;

    /* And return the filename. */
    *nameP = a->a_name;
    return True;
}
#endif /* IXI_DRAG_N_DROP */

void
hdr_form_set_width(hdr_form, position)
Widget hdr_form;
unsigned position;
{
    short cols;

    /* Have to set position of ScrolledWindow, not the text */
    XtVaSetValues(XtParent(hdr_form),
	XmNrightAttachment, XmATTACH_POSITION,
	XmNrightPosition, position,
	NULL);

    /* Hack to try to prevent horizontal resizing */
    XtVaGetValues(hdr_form, XmNcolumns, &cols, NULL);
    XtVaSetValues(hdr_form, XmNcolumns, cols, NULL);
}

void
do_hdr_form(frame)
ZmFrame frame;
{
    msg_folder *save = current_folder;
    msg_data *m_data;
    AttachInfo *ai;
    Pixmap pix;
    Widget child;
    char buf[32];
    int msg_no;

    FrameGet(frame,
	FrameFolder,     &current_folder,
	FrameClientData, &m_data,
#ifdef MSG_ATTACH_BUTTON
	FrameToggleItem, &toggle_item,
#endif /* MSG_ATTACH_BUTTON */
	FrameChild,      &child,
	FrameEndArgs);
    msg_no = m_data->this_msg_no;

#ifdef OZ_DATABASE
    SAVE_RESIZE(GetTopShell(m_data->hdr_fmt_w));
    SET_RESIZE(False);
#endif /* OZ_DATABASE */

    zmXmTextSetString(m_data->hdr_fmt_w,
	format_hdr(msg_no,
	    value_of(VarMsgWinHdrFmt),
	    False));

#ifdef OLD_WAY
    (void) sprintf(buf, catgets( catalog, CAT_MOTIF, 293, "Message %d" ), msg_no+1);
    XtVaSetValues(GetTopShell(child), XmNtitle, buf, NULL);
    (void) sprintf(buf, catgets( catalog, CAT_MOTIF, 294, "Msg %d" ), msg_no+1);
    XtVaSetValues(GetTopShell(child), XmNiconName, buf, NULL);
#else /* OLD_WAY */
    set_msg_icon_and_title(child);
#endif /* OLD_WAY */

    /* XtSetSensitive(toggle_item, !!msg[msg_no]->m_attach); */
#ifdef MSG_ATTACH_BUTTON
    XtSetSensitive(toggle_item, !!msg[msg_no]->m_attach);
#endif /* MSG_ATTACH_BUTTON */

    draw_attach_area(m_data->attach_area, frame);

#ifdef OZ_DATABASE
    RESTORE_RESIZE();
#endif /* OZ_DATABASE */

#ifndef NO_X_FACE
    if (pix = set_face(display, FrameGetIconItem(frame), msg_no)) {
	turnon(m_data->flags, HAS_FACE);
	FrameSet(frame, FrameIconPix, pix, FrameEndArgs);
    } else if (ison(m_data->flags, HAS_FACE)) {
	turnoff(m_data->flags, HAS_FACE);
	FrameSet(frame, FrameIconFile, NULL, FrameEndArgs);
    }
#endif /* NO_X_FACE */

    ai = m_data->attach_info;
    if (ai) {
	ai->count = 0;	/* force a redraw on next message */
	if (XtIsManaged(ai->shell)) {
#ifdef NOT_NOW
	/* Bart: Sun Jul 26 18:10:32 PDT 1992
	 * Added a popupCallback in create_frame() to reset the cursor of
	 * any ZmFrame when it is popped up.  So this *should* be obsolete.
	 */
	    /* XXX HACK!  Make sure the timeout_cursors() cursor is gone ... */
	    XSetWindowAttributes attrs;
	    attrs.cursor = None;
	    XChangeWindowAttributes(display,
				XtWindow(GetTopShell(attach_area_widget(m_data->attach_area))),
				CWCursor, &attrs);
#endif /* NOT_NOW */
	    XtUnmanageChild(ai->shell); /* Motif-ism */
	}
	XtPopdown(ai->shell);
    }

    current_folder = save;
}

/*
 * refresh this frame.  If the folder associated with this frame is
 * is not the current one, just ignore it.  Otherwise, the folder
 * may have been updated, in which case we need to destroy ourselves
 * and reset pager_textsw.  If not, check the msg structure and see
 * if the message has switched positions (sort/move).  Else, a status
 * change may have taken place in which case the appropriate buttons
 * may need to be updated.
 */
static int
refresh_msg(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    char str[32], *msgs_str;
    int msg_no, done_editing = FALSE;
    msg_data *m_data;
    u_long flags;
    msg_folder *this_folder, *save_folder = current_folder;
    Widget child;

    if (ison(reason, PREPARE_TO_EXIT))
	return 0;
    FrameGet(frame,
	FrameClientData, &m_data,
	FrameFlags,      &flags,
	FrameFolder,     &this_folder,
	FrameMsgString,  &msgs_str,
	FrameChild,      &child,
	FrameEndArgs);

    if (ison(flags, FRAME_WAS_DESTROYED) || isoff(flags, FRAME_IS_OPEN) ||
	    reason == PROPAGATE_SELECTION)
	return 0;

    /* Bart: Fri Jul 10 11:07:57 PDT 1992
     * Stupid, but we have to change current_folder
     * to use chk_msg() and to reference current_msg
     */
    fldr = current_folder = this_folder;

    if (fldr->mf_count == 0 ||
	    ison(fldr->mf_flags, CONTEXT_RESET) && !chk_msg(msgs_str)) {
	if (ison(m_data->flags, HAS_PAGERSW))
	    FrameClose(frame, False);
	else
	    FrameDestroy(frame, False);
	current_folder = save_folder;
	return 0;
    }
    if (msg_no = chk_msg(msgs_str))
	--msg_no;
    else {
	Debug("Bogus message number %d\n" , msg_no);
	if (isoff(flags, FRAME_IS_LOCKED))
	    msg_no = m_data->this_msg_no;
	else
	    msg_no = current_msg; /* We must be here via gui_pager() */
    }
    ask_item = child;
    current_folder = save_folder;

    /* if the offsets are different, either a new message is being
     * printed, or the folder was resorted, or the message was edited.
     * A new message can only be sent to -this- frame if the flag
     * HAS_PAGERSW is set.
     */
    if (m_data->this_msg.m_offset != fldr->mf_msgs[msg_no]->m_offset ||
	    m_data->this_msg.m_size != fldr->mf_msgs[msg_no]->m_size) {
	if (m_data->this_msg_no != msg_no) {
	    if (isoff(m_data->flags, HAS_PAGERSW)) {
		error(ZmErrWarning,
		    catgets( catalog, CAT_MOTIF, 296, "Inconceivable!  My message number changed?" ));
	    }
	} else if (isoff(flags, FRAME_IS_LOCKED)) {
	    int i;
	    for (i = 0; i < fldr->mf_count; i++)
		if (m_data->this_msg.m_offset == fldr->mf_msgs[i]->m_offset)
		    break;
	    if (i < fldr->mf_count &&
		    m_data->this_msg.m_size != fldr->mf_msgs[i]->m_size)
		i = fldr->mf_count;
	    if (i == fldr->mf_count &&
		  isoff(fldr->mf_msgs[msg_no]->m_flags, EDITING) &&
		  ison(m_data->this_msg.m_flags, EDITING)) {
		if (isoff(m_data->flags, HAS_PAGERSW)) {
		    FrameClose(frame, False);
		    return -1;
		}
		i = msg_no;
		done_editing = TRUE;
	    }
	    if (i == fldr->mf_count) {
		if (ison(m_data->flags, HAS_PAGERSW))
		    FrameClose(frame, False);
		else
		    FrameDestroy(frame, False);
		error(ZmErrWarning,
		    catgets( catalog, CAT_MOTIF, 297, "HELP!  I cannot find myself in this folder!" ));
		return -1;
	    }
	    msg_no = i;
	}
	if (isoff(flags, FRAME_IS_LOCKED)) {
	    sprintf(str, "%d", msg_no+1);
	    FrameSet(frame,
		FrameMsgString, str,
		FrameIconTitle, str,
		FrameTitle,     zmVaStr(catgets( catalog, CAT_MOTIF, 293, "Message %d" ), msg_no+1),
		FrameEndArgs);
	}
	m_data->this_msg = *(fldr->mf_msgs[msg_no]);
	m_data->this_msg_no = msg_no;
#ifdef STATUS_CHOICES
	ToggleBoxSetValue(m_data->status_kids, fldr->mf_msgs[msg_no].m_flags);
#endif /* STATUS_CHOICES */
	do_hdr_form(frame);
        RebuildMsgDisplay(frame);
    } else if (ison(flags, FRAME_IS_LOCKED)) {
	m_data->this_msg = *(fldr->mf_msgs[msg_no]);
	m_data->this_msg_no = msg_no;
	do_hdr_form(frame); /* LOCKED means a redraw was forced */
        RebuildMsgDisplay(frame);
    }
    if (m_data->this_msg.m_flags != fldr->mf_msgs[msg_no]->m_flags) {
	int do_iconify = bool_option(VarAutoiconify, "message");
	if (ison(fldr->mf_msgs[msg_no]->m_flags, DELETE)) {
	    int do_close = isoff(m_data->this_msg.m_flags, DELETE) &&
			    (do_iconify || !boolean_val(VarShowDeleted) ||
			    bool_option(VarAutodismiss, "message"));
	    if (isoff(m_data->flags, HAS_PAGERSW)) {
		if (do_close)
		    FrameDestroy(frame, False);
		return 0;
	    } else if (isoff(m_data->this_msg.m_flags, DELETE)) {
		if (fldr == current_folder &&	/* just to be sure */
			XtIsManaged(child) && boolean_val(VarAutoprint)) {
		    int num = f_next_msg(fldr, msg_no, 1);
		    if (num != msg_no) {
			sprintf(str, "%d", (fldr->mf_current = num)+1);
			FrameSet(frame, FrameMsgString,
				 str, FrameEndArgs);
			/* SetCurrentMsg() causes a recursive call */
			SetCurrentMsg(hdr_list_w, msg_no = num, True);
#if defined( IMAP )
                        zmail_mail_status(0);
#else
                        mail_status(0);
#endif
			/* do_close = FALSE; */
			return 0;
		    } else
			gui_print_status(catgets(catalog, CAT_MOTIF, 873, "No more messages.\n"));
		}
	    }
	    if (do_close)
		FrameClose(frame, do_iconify);
	}
	m_data->this_msg = *(fldr->mf_msgs[msg_no]);
	m_data->this_msg_no = msg_no;
    }

    ZmCallbackCallAll("message_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
    if (done_editing)
	display_msg(current_msg = m_data->this_msg_no, display_flags());
    return 0;
}

#ifdef STATUS_CHOICES
static void
reset_toggle(w, position, reason)
Widget w;
int position;
XmToggleButtonCallbackStruct *reason;
{
    int *global;
    XtVaGetValues(XtParent(w), XmNuserData, &global, NULL);
    if (reason->set)
	*global |= (1<<position);
    else
	*global &= ~(1<<position);
    XmToggleButtonSetState(w, !reason->set, False);
    ask_item = w;
    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 299, "You cannot set these values." ));
}
#endif /* STATUS_CHOICES */

#ifdef NOT_NOW
static void
what_now(w, what)
Widget w;
{
    ask_item = w;
    error(HelpMessage, what);
}
#endif /* NOT_NOW */

/* update the GUI to reflect a new type.  This can be called in either
 * the compose or message windows.
 */
static void
update_gui_for_type(atype, ai, overrideAttachSpecificFlag)
    char	*atype;
    AttachInfo	*ai;
    int		overrideAttachSpecificFlag;
{
    AttachKey *ak;
    Widget w;
    
    if (ak = get_attach_keys(FALSE, (Attach *)0, atype)) {
	if (ai->is_compose && overrideAttachSpecificFlag) {
	    if (ak->use_code)
		SetOptionMenuChoice(ai->encoding_w, ak->use_code, False);
	    if (ak->description)
		SetTextString(ai->comment_w, ak->description);
	    else
		SetTextString(ai->comment_w, NULL);
	}
#ifdef CREATE_ATTACH
	if (w = XtNameToWidget(ai->action_area, NEW_BUTTON_NAME))
	    XtSetSensitive(w, ak && ak->encode.program &&
		ci_strcmp(ak->encode.program, "none"));
#endif /* CREATE_ATTACH */
	/* XXX This is incorrect if the encoding is not the default */
	if (w = XtNameToWidget(ai->action_area, DISPLAY_BUTTON_NAME))
	    XtSetSensitive(w, ak && ak->decode.program &&
		ci_strcmp(ak->decode.program, "none"));
    }
}

static void
describe_type(button)
    Widget button;
{
    AttachInfo *ai;
    char *atype = XtName(button);
    
    XtVaGetValues(button, XmNuserData, &ai, NULL);
    if (!ai)
	return;
    
    update_gui_for_type(atype, ai, 1);
}

static void
mark_modified_ai(text_w, ptr, cbs)
Widget text_w;
int *ptr;
XmTextVerifyCallbackStruct *cbs;
{
    /* if setting it with zmXmTextSetString(), clear modified flag */
    if (!cbs || !cbs->text) return;
    if (!cbs->event) {
	*ptr = False;
	return;
    }
    if (cbs->startPos == cbs->endPos && cbs->text->length == 0) return;
    *ptr = True;
}

static char *
CreateDataTypeMenu(parent, ai, type)
Widget parent;
AttachInfo *ai;
FrameTypeName type;
{
    char **choices, **labels = 0, *default_type = 0;
    int n;

    if (type == FrameCompose)
	(void) get_compose_type_keys(&choices);
    else
	(void) get_display_type_keys(&choices);
    ai->datatype_w = BuildSimpleMenu(parent, "file_type",
		     choices, XmMENU_OPTION, ai, describe_type);
    if (chk_option(VarAttachLabel, "comment")) {
	if (n = vlen(choices)) {
	    AttachKey *ak;

	    labels = (char **)calloc(n+1, sizeof(char *));
	    while (n--) {
		ak = get_attach_keys(0, 0, choices[n]);
		if (ak && ak->description && ak->description[0])
		    labels[n] = savestr(zmVaStr("%s", ak->description));
		else
		    labels[n] = savestr(choices[n]);
	    }
	    SetOptionMenuLabels(ai->datatype_w, choices, labels);
	    free_vec(labels);
	}	
    }
    XtManageChild(ai->datatype_w);
    if (choices && choices[0])
	default_type = savestr(choices[0]);
    free_vec(choices);

    return default_type;
}

static void
CreateEncodingMenu(parent, ai, type)
Widget parent;
AttachInfo *ai;
FrameTypeName type;
{
    char **choices;

    if (type == FrameCompose)
	get_compose_code_keys(&choices);
    else
	get_display_code_keys(&choices);
    ai->encoding_w = 
	BuildSimpleMenu(parent, "encoding",
			choices, XmMENU_OPTION, NULL, (void_proc)0);
    XtManageChild(ai->encoding_w);
    free_vec(choices);
}


#ifndef OZ_DATABASE
static void
autotyper_cb(toggle, cdata)
    Widget toggle;
    ZmCallbackData cdata;
{
    const Boolean enable = cdata->event == ZCB_VAR_SET;
    XtSetSensitive(toggle, enable);
    if (!enable) XmToggleButtonSetState(toggle, False, False);
}
#endif /* !OZ_DATABASE */


static void
sensitize_menus(toggle, menus, details)
    Widget toggle;
    Widget menus;
    XmToggleButtonCallbackStruct *details;
{
    if (details && details->reason == XmCR_VALUE_CHANGED)
	XtSetSensitive(menus, !details->set);
    else
	XtSetSensitive(menus, !XmToggleButtonGetState(toggle));
}


#if !(defined(MSDOS) || defined(MAC_OS))
AttachInfo *
open_attach(frame, required)
ZmFrame frame;
Boolean required;
{
    char *p;
    Arg args[3];
    Widget pane, form, attach_area, w;
    Attach *attachments;
    AttachInfo **ai;
    FrameTypeName type;
    FileFinderStruct *ffs;
    union {
	Compose *compose;
	msg_data *message;
    } cl_data;
    char	*default_type = NULL;

    FrameGet(frame,
	FrameClientData, &cl_data,
	FrameType,       &type,
#ifdef MSG_ATTACH_BUTTON
	FrameToggleItem, &toggle_item,
#endif /* MSG_ATTACH_BUTTON */
	FrameEndArgs);

    if (type == FrameCompose) {
	attachments = cl_data.compose->attachments;
	attach_area = attach_area_widget(cl_data.compose->interface->attach_area);
	ai	    = &cl_data.compose->interface->attach_info;
    } else {
	attachments = cl_data.message->this_msg.m_attach;
	attach_area = attach_area_widget(cl_data.message->attach_area);
	ai	    = &cl_data.message->attach_info;
    }

    if (!*ai) {
	ZmFrame newframe;
	ZmCallback zc;
	ActionAreaItem *attach_btns;
	unsigned buttonCount;
	static ActionAreaItem compose_attach_btns[] = {
	    { "Attach",		   do_attachment,          NULL },
	    { "Unattach",	   detach_attachment,      NULL },
#ifdef CREATE_ATTACH
	    { NEW_BUTTON_NAME,	   examine_attachment,     NULL },
#endif /* CREATE_ATTACH */
	    { DISPLAY_BUTTON_NAME, detach_attachment,      NULL },
	    { "Edit",		   examine_attachment,     NULL },
	    { DONE_STR,		   DoParent, (caddr_t)XtPopdown },
	    { "Help",		   DialogHelp, "Attachments Dialog" },
	};
	static ActionAreaItem message_attach_btns[] = {
	    { "Save",		   detach_attachment,      NULL },
	    { NULL,		   0,			   NULL },
	    { DISPLAY_BUTTON_NAME, detach_attachment,      NULL },
	    { NULL,		   0,			   NULL },
	    { DONE_STR,		   DoParent, (caddr_t)XtPopdown },
	    { NULL,		   0,			   NULL },
	    { "Help",		   DialogHelp, "Attachments Dialog" },
	};

	if (!required)
	    return *ai;

	if (type == FrameCompose) {
	    attach_btns = compose_attach_btns;
	    buttonCount = XtNumber(compose_attach_btns);
	    attach_btns[0].data = attach_btns[1].data =
		attach_btns[2].data = attach_btns[3].data =
#ifdef CREATE_ATTACH
		attach_btns[4].data =
#endif /* CREATE_ATTACH */
		frame;
	} else {
	    attach_btns = message_attach_btns;
	    buttonCount = XtNumber(message_attach_btns);
	    attach_btns[0].data = attach_btns[2].data = frame;
	}
	
	*ai = XtNew(AttachInfo);
	(*ai)->is_compose = (type == FrameCompose);
	(*ai)->count = 0;

	newframe = FrameCreate("attachments_dialog",
	    FrameAttachments,	GetTopShell(attach_area),
	    FrameIcon,		&attach_icon,
#ifdef NOT_NOW
	    FrameTitle,		"Attachments",
#endif /* NOT_NOW */
	    FrameChild,		&pane,
	    FrameFlags,		FRAME_SHOW_ICON | FRAME_SHOW_FOLDER |
	    			FRAME_CANNOT_SHRINK_H |
			((type == FrameCompose)? NO_FLAGS : FRAME_SHOW_LIST),
	    FrameRefreshProc,   generic_frame_refresh,
	    FrameEndArgs);
	(*ai)->shell = XtParent(pane);

	/* Add callbacks so that when the parent is destroyed, this
	 * frame is destroyed along with it.
	 */
	XtAddCallback(attach_area, XmNdestroyCallback,
		      (XtCallbackProc) free_user_data, *ai);
	XtAddCallback(attach_area, XmNdestroyCallback,
		      (XtCallbackProc) DestroyFrameCallback, newframe);

	if (type == FrameCompose || !(p = get_detach_dir()))
	    p = value_of(VarCwd);
	form = CreateFileFinder(pane, p, (void_proc)0, ai_from_finder, (caddr_t) *ai);
	
	XtVaGetValues(form, XmNuserData, &ffs, NULL);
#if defined( IMAP )
	XtUnmanageChild( ffs->imap_w );
#endif
	if (type == FrameCompose) {
	    (*ai)->autotype_w = XtVaCreateManagedWidget("auto_type",
		xmToggleButtonGadgetClass, XtParent(ffs->text_w), NULL);
	    if (XtClass(XtParent(ffs->text_w)) == xmFormWidgetClass) {
		XtVaSetValues(ffs->text_w,
		    XmNrightAttachment, XmATTACH_WIDGET,
		    XmNrightWidget, (*ai)->autotype_w,
		    NULL);
		XtVaSetValues((*ai)->autotype_w,
		    XmNtopAttachment, XmATTACH_FORM,
		    XmNbottomAttachment, XmATTACH_FORM,
		    XmNrightAttachment, XmATTACH_FORM,
		    NULL);
	    }
#ifndef OZ_DATABASE
	    {
		Boolean enabled = boolean_val(VarAutotyper);
		XtSetSensitive((*ai)->autotype_w, enabled);
		if (!enabled)
		    XmToggleButtonSetState((*ai)->autotype_w, False, False);
	    }
	    XtAddCallback((*ai)->autotype_w, XmNdestroyCallback, remove_callback_cb,
		  ZmCallbackAdd(VarAutotyper, ZCBTYPE_VAR, autotyper_cb, (*ai)->autotype_w));
#endif /* !OZ_DATABASE */
	} else
	    (*ai)->autotype_w = 0;
	
	XtManageChild(form);
	(*ai)->file_w = ffs->text_w;

	form = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, pane,
	    XmNorientation,	XmHORIZONTAL,
	    /* XmNpacking,	XmPACK_COLUMN, */
	    /* XmNnumColumns,	3, */
	    XmNspacing,		24,
	    XmNskipAdjust,	True,
	    NULL);

	default_type = CreateDataTypeMenu(form, *ai, type);

	CreateEncodingMenu(form, *ai, type);
	
	(*ai)->comment_w = CreateLabeledText("comment", pane, NULL,
	    CLT_HORIZ|CLT_REPLACE_NL);
	XtAddCallback((*ai)->comment_w, XmNmodifyVerifyCallback,
		      (XtCallbackProc) mark_modified_ai,
		      &(*ai)->comment_modified);
	XtVaSetValues(XtParent((*ai)->comment_w),
#ifdef NOT_NOW
	    XmNleftAttachment,   XmATTACH_FORM,
	    XmNtopAttachment,    XmATTACH_WIDGET,
	    XmNtopWidget,        (*ai)->encoding_w,
	    XmNrightAttachment,  XmATTACH_FORM,
#else /* !NOT_NOW */
	    XmNskipAdjust, True,
#endif /* !NOT_NOW */
	    NULL);

	if ((*ai)->autotype_w) {
	    XtAddCallback((*ai)->autotype_w, XmNvalueChangedCallback,
			  (XtCallbackProc) sensitize_menus, form);
	    sensitize_menus((*ai)->autotype_w, form, 0);
	}
	
	XtManageChild(form);

	XtVaCreateManagedWidget("attached_docs",
	    xmLabelGadgetClass, pane, NULL);
	XtSetArg(args[0], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE);
	XtSetArg(args[1], XmNscrollingPolicy, XmAUTOMATIC);
	XtSetArg(args[2], XmNselectionPolicy, XmBROWSE_SELECT);
	(*ai)->list_w = XmCreateScrolledList(pane, "attachment_list", args, 3);
	ListInstallNavigator((*ai)->list_w);
	XtManageChild((*ai)->list_w);
	XtAddCallback((*ai)->list_w, XmNbrowseSelectionCallback, fill_ai, frame);
	XtAddCallback((*ai)->list_w, XmNdefaultActionCallback,
		      (XtCallbackProc) detach_attachment, frame);
#ifdef NOT_NOW
	XtAddCallback((*ai)->list_w, XmNdefaultActionCallback,
	    what_now, (*ai)->is_compose?
		catgets( catalog, CAT_MOTIF, 310, "Please choose Unattach, Display, or Edit" ):
		catgets( catalog, CAT_MOTIF, 311, "Please choose Detach or Display" ));
#endif /* NOT_NOW */

	(*ai)->action_area = CreateActionArea(pane, attach_btns,
	    buttonCount, "Attachments");
	FrameSet(newframe, FrameDismissButton, XtNameToWidget((*ai)->action_area, DONE_STR), FrameEndArgs);
#ifdef CREATE_ATTACH
	if (w = XtNameToWidget((*ai)->action_area, NEW_BUTTON_NAME))
	    XtSetSensitive(w, False);
#endif /* CREATE_ATTACH */
	if (w = XtNameToWidget((*ai)->action_area, DISPLAY_BUTTON_NAME))
	    XtSetSensitive(w, False);

#ifdef IXI_DRAG_N_DROP
	/* For compose attachments dialogs, register to accept drops. */
	if (type == FrameCompose)
	    DropRegister(pane, NULL, attach_filename, NULL, frame);
#endif /* IXI_DRAG_N_DROP */
	zc = ZmCallbackAdd("", ZCBTYPE_ATTACH, attach_menu_cb, *ai);
	XtAddCallback(form, XmNdestroyCallback, remove_callback_cb, zc);

	/* Bart: Thu Apr  7 18:33:14 PDT 1994
	 * This function isn't supposed to map this dialog, but this
	 * call maps it anyway.  When I commented this next line out,
	 * a window-manager frame with no dialog in it would appear
	 * anyway [in spite of setting mappedWhenManaged of the shell
	 * to False in create_frame()].  I give up; let it be mapped.
	 *
	 * Strangely enough, this works for the message window, but not
	 * for the compose window.  What's different??
	 */
	XtManageChild(pane);

	FrameCopyContext(frame, newframe);
    } else
	FrameCopyContext(frame, FrameGetData((*ai)->shell));

    /* List widget needs userData before call to fill_attach_list_w() */
    XtVaSetValues((*ai)->list_w, XmNuserData, *ai, NULL);
    /* fill_attach_list_w resets the selected item, so only call it
     * if necessary.  Otherwise, selecting attachment icons in the
     * compose window displays the wrong icon.
     */
    if ((*ai)->count != number_of_links(attachments))
	fill_attach_list_w((*ai)->list_w, (Attach *)attachments, type);

    if (!attachments && default_type) {
	/* Get the initial value from the File Type menu and set comment 
	 * Don't call describe_type directly as XtName for the type button
	 * isn't initialized yet 
	 */
	update_gui_for_type(default_type, *ai, 1);
	xfree(default_type);
	default_type = NULL;
    }

    return *ai;
}
#endif /* MSDOS || MAC_OS */

void
select_attachment(w, frame)
Widget w;
ZmFrame frame;
{
    AttachInfo *ai;
    XtPointer p;

    ai = open_attach(frame, True);	/* Create if needed, but don't map */
    if (!ai)
	return;
    XtVaGetValues(XtParent(w), XmNuserData, &p, NULL);
    XmListSelectPos(ai->list_w, (int) p, True);
    /* Bart: Thu Apr  7 23:38:25 PDT 1994
     * I don't think this is necessary:
    FramePopup(FrameGetData(ai->shell));
     */
}

void
display_attachments(parent, frame)
Widget parent;
ZmFrame frame;
{
#if defined(MSDOS) || defined(MAC_OS)
    error(ZmErrWarning, catgets( catalog, CAT_MSGS, 53, "Attachments not supported for DOS (yet)." ));
    return;
#else /* !(MSDOS || MAC_OS) */
    AttachInfo *ai;

    if (!frame)
	frame = FrameGetData(parent);

    ai = open_attach(frame, True);

    FramePopup(FrameGetData(ai->shell));
#endif /* MSDOS || MAC_OS */
}

/* bring up the attachments dialog. */
ZmFrame
DialogCreateAttachments(t, w)
Widget t, w;
{
    ZmFrame frame;
    FrameTypeName type;

    frame = FrameGetData(w);
    type = FrameGetType(frame);
    if (type != FrameCompose && type != FramePageMsg && type != FramePinMsg)
	return (ZmFrame) 0;
    /* probably a good idea to check this... */
    if (type == FrameCompose && !FrameGetFreeClient(frame)) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MSGS, 904, "No active composition in this window"));
	return (ZmFrame) 0;
    }
    display_attachments(w, frame);
    return (ZmFrame) 0;
}

void
fill_attach_list_w(list_w, a, type)
Widget list_w;
Attach *a;
FrameTypeName type;
{
    XmStringTable items;
    int x = 0;
    char name[256], buf[256];
    Attach *attachments = a;
    AttachInfo *ai;

    if (!a) {
	XtVaSetValues(list_w,
	    XmNitems,      NULL,
	    XmNitemCount,  0,
	    NULL);
	fill_ai(list_w, NULL, NULL);
	XtVaGetValues(list_w, XmNuserData, &ai, NULL);
	if (ai) ai->count = 0;
	return;
    }

    items = (XmStringTable)XtMalloc(sizeof(XmString));

    if (type != FrameCompose)
	a = (Attach *)a->a_link.l_next;

    do  {
	x++;
	if (a->content_name)
	    (void) strcpy(name, basename(a->content_name));
	else if (a->a_name)
	    (void) strcpy(name, basename(a->a_name));
	else {
	    (void) sprintf(name, catgets( catalog, CAT_MOTIF, 313, "Attach.%d" ), x);
	    a->content_name = savestr(name);	/* For lookups */
	    a->a_name = savestr(name);
	    turnon(a->a_flags, AT_TEMPORARY);
	}
	(void) sprintf(buf, "%2d %-24.24s %-24.24s", x, name,
		attach_data_type(a) ? attach_data_type(a) :
		a->content_type ? a->content_type : catgets(catalog, CAT_MSGS, 827, "unknown"));
	if (a->content_length)
	    (void) sprintf(&buf[strlen(buf)], catgets( catalog, CAT_MOTIF, 316, " (%d bytes)" ), a->content_length);
	else
	    (void) strcat(buf, catgets( catalog, CAT_MOTIF, 317, " (pending)" ));
	items = (XmStringTable)XtRealloc((char *)items, (x+1) * sizeof(XmString));
	items[x-1] = XmStr(buf);
    } while ((a = (Attach *)a->a_link.l_next) != attachments);
    items[x] = 0;

    XtVaSetValues(list_w,
	XmNitems,      items,
	XmNitemCount,  x,
	NULL);
    XmStringFreeTable(items);
    if (x)
	XmListSelectPos(list_w, 1, True);
    XtVaGetValues(list_w, XmNuserData, &ai, NULL);
    ai->count = number_of_links(a);
}

/* Blecch -- to avoid having to change the callbacks and button labels for
 * the buttons that appear in both the reading and composing attachments
 * dialogs, we have examine_attachment() and detach_attachment(), each of
 * which calls the other in certain circumstances.
 */

static void
examine_attachment(w, frame)
Widget w;
ZmFrame frame;
{
    FileFinderStruct *ffs;
    Compose *compose;
    AttachInfo *ai;
    AttachProg *ap;
    Attach *tmp_att;
    FILE *errfp;
    char *errnm = NULL, *type, *part, name[MAXPATHLEN];
    int displaying = !strcmp(XtName(w), DISPLAY_BUTTON_NAME) ||
	    !strcmp(XtName(w), "attachment_list") /* GROSS HACK XXX */,
	from_icon = !strcmp(XtName(w), ICON_AREA_NAME);
    Widget widget;
    char *err;
    int isdir;

    ask_item = w;
    if (FrameGetType(frame) != FrameCompose) {
	detach_attachment(w, frame);
	return;
    } else {
	compose = FrameComposeGetComp(frame);
	if (from_icon) displaying = True;
    }

    if (from_icon)
	select_attachment(w, frame);	/* Creates attachments dialog */
    ai = compose->interface->attach_info;

    XtVaGetValues(XtParent(XtParent(ai->file_w)),
	XmNuserData, &ffs,
	NULL);

#ifdef CREATE_ATTACH
    if (!strcmp(XtName(w), NEW_BUTTON_NAME)) {
	(void) new_attach_filename(compose, name);
	FileFinderSetFullPath(ai->file_w, name);
    } else
#endif /* CREATE_ATTACH */
    if (!(part = FileFinderGetFullPath(ai->file_w)) || !*part) {
	xfree(part);
	ask_item = ai->file_w;
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 320, "Specify file to examine" ));
	return;
    } else {
	type = FileFinderGetPath(ffs, part, (int *)0);
	XtFree(part);
	if (!type)
	    return;
	(void) strcpy(name, type);
    }
    if (!fullpath(name, FALSE)) {
	error(SysErrWarning, name);
	return;
    }
    isdir = ZmGP_IgnoreNoEnt;
    err = getpath(name, &isdir);
    if (isdir == ZmGP_Dir) {
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 853, "Can't attach directories."));
	return;
    }
    if (isdir != ZmGP_File) {
	error(SysErrWarning, err);
	return;
    }

    tmp_att = (Attach *)retrieve_link((struct link **) compose->attachments,
				  name, strcmp);

    XtVaGetValues(ai->datatype_w, XmNmenuHistory, &widget, NULL);
    type = XtName(widget);
    ap = coder_prog(!displaying, tmp_att, type, name, "x", FALSE);
    if (Access(name, F_OK) == 0) {
	if (!displaying && ap->checked == 0 && ap->givename < 1) {
	    if (ask(ap->givename == 0? WarnOk : WarnCancel,
		    catgets( catalog, CAT_MOTIF, 321, "The editor program %s overwrite %s%s.\nContinue anyway?" ),
		    ap->givename == 0? catgets( catalog, CAT_MOTIF, 322, "may" ) : "will", name,
		    ap->givename == 0? catgets( catalog, CAT_MOTIF, 323, " before editing" ) : "") != AskYes)
		return;
	}
    } else if (displaying) {
	error(UserErrWarning,
		catgets( catalog, CAT_MOTIF, 324, "Cannot display \"%s\":\nFile does not exist." ), name);
	return;
    }
    if (errfp = open_tempfile("err", &errnm))
	(void) fclose(errfp);
    timeout_cursors(TRUE);
    (void) popen_coder(ap, name, errnm, "x");
    (void) handle_coder_err(ap->exitStatus, ap->program, errnm);
    (void) unlink(errnm);
    xfree(errnm);
    timeout_cursors(FALSE);
    
    /* XXX
     * Now what?  The coder program may have done anything, may
     * still be running in the background, or almost anything.
     * Just attach the (possibly non-existant) file ....
     */
    if (!displaying && !tmp_att)
	do_attachment(w, frame);
    FileFinderDefaultSearch(FileFindDir, ffs, NULL, name);
    DismissSetWidget(w, DismissClose);
}

static int 
detach_images_and_html_if_necessary(i,frame,w)
int i;
ZmFrame frame;
Widget w;
{
    int x = 0;
    char buf[256], name[256];
    Attach *attachments, *a;
    Attach *hold_a;
    AttachInfo *ai;
    char *the_type;
    Boolean we_have_html , we_have_images_or_html;
    char *path;
    FILE *xreffile;

    ZmFrame attachFrame = FrameGetData(w);
    msg_data *message = (msg_data *)FrameGetClientData(frame);

    attachments = message->this_msg.m_attach;
    a = attachments;
    strcpy(name,"");
/*
  If we have no attachments then return.
*/ 
    if (!a) {
        return(0);
    }
/*
  If a compose frame then adjust attachment pointer.
*/ 
    if (FrameGetType(frame) != FrameCompose)
        a = (Attach *)a->a_link.l_next;
    hold_a = a;
/*
  The first time through we look to see if we are viewing html and if there
  are images or html included as attachments.
*/
    we_have_html = False;
    we_have_images_or_html = False;
    do  {
        x++;
        the_type = attach_data_type(a) ? attach_data_type(a) :
                a->content_type ? a->content_type : "unknown";
/* 
  If the type is html and this is what we are going to display then we 
  are displaying html.
*/
        if ((i == x) && (ci_strcmp(the_type,"text/x-html") == 0))
          {
            we_have_html = True;
            if (a->content_name)
                (void) strcpy(name, basename(a->content_name));
            else if (a->a_name)
                (void) strcpy(name, basename(a->a_name));
            else 
                (void) sprintf(name, catgets( catalog, CAT_MOTIF, 313, 
                               "Attach.%d" ), x);
          }
/*
  Else if the type is image or html then we found an attachment that may be 
  linked the the html we found may have found.
*/
        else
          {
            if (ci_strncmp(the_type,"image/",strlen("image/")) == 0)
              we_have_images_or_html = True;
            if (ci_strncmp(the_type,"text/x-html",strlen("text/x-html")) == 0)
              we_have_images_or_html = True;
          }
    } while ((a = (Attach *)a->a_link.l_next) != attachments);
/*
  If we are not viewing html or if we are and the other attachments are not
  images or html then return.
*/
  if (!we_have_html)
    return(x);
  if (!we_have_images_or_html)
    return(x);
/*
  There are images or html along with the html so detach them and write
  out a table of the names to be used later to replace the locations of
  the in the html.
*/
    path = get_detach_dir();
    sprintf(buf,"%s/%s.xrf",path,name);
    xreffile = fopen(buf,"w");
    if (xreffile == NULL) {
      error(ZmErrWarning, "Cannot open html xref file");
      return(x);
    }
    a = hold_a;
    x = 0;
    do  {
        x++;
        if (x != i)
          {
            the_type = attach_data_type(a) ? attach_data_type(a) :
                    a->content_type ? a->content_type : "unknown";
            if ((ci_strncmp(the_type,"image/",strlen("image/")) == 0) ||
                (ci_strncmp(the_type,"text/x-html",strlen("text/x-html")) == 0))
              {
                if (a->content_name)
                    (void) strcpy(name, basename(a->content_name));
                else if (a->a_name)
                    (void) strcpy(name, basename(a->a_name));
                else 
                    (void) sprintf(name, catgets( catalog, CAT_MOTIF, 313, 
                                   "Attach.%d" ), x);
                if (xreffile != NULL)
                  fprintf(xreffile,"%s %s %s/%s\n",the_type,name,path,name);
                sprintf(buf, "builtin detach -part %d -T -O ", x);
	        gui_cmd_line(buf, attachFrame);
              }
          }
    } while ((a = (Attach *)a->a_link.l_next) != attachments);
    if (xreffile != NULL)
      fclose(xreffile);
  return(x);
}

/* For compose, this function removes an attachment from a composition's list.
 * Otherwise, it detaches and possibly displays an attachment from a message.
 */
void
detach_attachment(w, frame)
Widget w;
ZmFrame frame;
{
    FileFinderStruct *ffs;
    Compose *compose;
    extern void free_attach();
    Attach *a;
    AttachInfo *ai = 0;
    char *file, *decoding = NULL, *p, *program = NULL;
    char buf[max(BUFSIZ,2*MAXPATHLEN)];
    int i, *ip;
    int displaying = (!strcmp(XtName(w), DISPLAY_BUTTON_NAME) ||
	    !strcmp(XtName(w), "attachment_list") /* GROSS HACK XXX */),
	from_icon = !strcmp(XtName(w), ICON_AREA_NAME);
    Widget widget;
    ZmFrame attachFrame = FrameGetData(w);

    ask_item = w;
    if (from_icon) {
	ai = open_attach(frame, False);
	displaying = True;
    } else {
	if (FrameGetType(frame) == FrameCompose)
	    ai = (AttachInfo *) FrameComposeGetComp(frame)->interface->attach_info;
	else
	    ai = ((msg_data *)FrameGetClientData(frame))->attach_info;
	if (!ai)
	    return;
    }

    if (from_icon) {
	XtVaGetValues(XtParent(w), XmNuserData, &ip, NULL);
	i = (int)ip;
	if (ai)
	    XmListSelectPos(ai->list_w, i, True);
    } else if (XmListGetSelectedPos(ai->list_w, &ip, &i)) {
	i = ip[0];
	XtFree((char *)ip);
    } else {
	ask_item = w;
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 327, "Select an attachment." ));
	return;
    }

    if (ai && !from_icon)
	ask_item = ai->file_w;
    else
	ask_item = w;

    sprintf(buf, "builtin detach -part %d ", i);
    p = buf + strlen(buf);

    if (!ai || from_icon && displaying) {
	if (FrameGetType(frame) == FrameCompose)
	    examine_attachment(w, frame);
	else {
            detach_images_and_html_if_necessary(i,frame,w);
	    (void) strcpy(p, "-display");
	    if (!gui_cmd_line(buf, attachFrame))
		DismissSetFrame(attachFrame, DismissClose);
	    if (ai)
		XmListSelectPos(ai->list_w, i, True);
	}
	return;
    }

    XtVaGetValues(ai->file_w, XmNuserData, &a, NULL);
    if (!a)
	return;

    if (ai->is_compose)
	compose = FrameComposeGetComp(frame);

    if (ai->is_compose) {
	if (displaying) {
	    /* Bart: Tue Jun 30 13:41:20 PDT 1992
	     * We need to reset the types and name according to the
	     * selected position, in case the user has been fooling
	     * with them, so we don't display the wrong thing.
	     */
	    if (!from_icon)
		XmListSelectPos(ai->list_w, i, True);
	    examine_attachment(w, frame);
	} else {
	    remove_link(&compose->attachments, a);
	    free_attach(a, TRUE);
	    fill_attach_list_w(ai->list_w, compose->attachments, FrameCompose);
	    draw_attach_area(compose->interface->attach_area, frame);
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR,
			       ZCB_VAR_SET, NULL);
	    DismissSetFrame(attachFrame, DismissClose);
	}
	return;
    }
    if ((!(file = FileFinderGetFullPath(ai->file_w)) || !*file) && !displaying) {
	if (!file || !*file)
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 328, "Specify the file to detach." ));
	else
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 329, "File not attached." ));
	XtFree(file);
	return;
    }

    if (displaying) {
	XtVaGetValues(ai->datatype_w, XmNmenuHistory, &widget, NULL);
	program = XtName(widget);
	if (!ci_strcmp(program, "unknown")) {
	    ask_item = ai->datatype_w;
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 331, "Unknown file type -- cannot display." ));
	    XtFree(file);
	    return;
	} else if (ci_strcmp(program, "none") != 0) {
	    sprintf(p, "-use %s ", program);
	    p += strlen(p);
	}
    }

    XtVaGetValues(ai->encoding_w, XmNmenuHistory, &widget, NULL);
    decoding = XtName(widget);

    sprintf(p, "-encode \"%s\" ", decoding);
    p += strlen(p);

    if (file)
	(void) sprintf(p, " %s", quotezs(file, 0));

    {
	int status;

	FrameCopyContext(frame, attachFrame);
	status = gui_cmd_line(buf, attachFrame);

	if (a->a_name) {
	    XtVaGetValues(XtParent(XtParent(ai->file_w)),
			  XmNuserData, &ffs, NULL);
	    FileFinderDefaultSearch(FileFindFile, ffs, NULL, a->a_name);
	}
	if (!status) DismissSetFrame(attachFrame, DismissClose);
    }
    XtFree(file);
}

/*
 * isn't this a great name?
 */
static void
do_attachment(w, frame)
Widget w;
ZmFrame frame;
{
    Compose *compose;
    AttachInfo *ai;
    char *file, *type = "Text", *encoding, *comment;
    const char *err;
    u_long a_flags = NO_FLAGS;
    Widget widget;
    int n = 0;

    FrameGet(frame,
	FrameClientData, &compose,
#ifdef MSG_ATTACH_BUTTON
	FrameToggleItem, &toggle_item,
#endif /* MSG_ATTACH_BUTTON */
	FrameEndArgs);
    ai = compose->interface->attach_info;

    ask_item = ai->file_w;
    if (!(file = FileFinderGetFullPathEx(ai->file_w, &n)) || !*file) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 333, "You must provide a file to attach." ));
	xfree(file);
	return;
    }
    if (ai->is_compose && n == 1) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MOTIF, 854, "You cannot attach directories."));
	xfree(file);
	return;
    }
    if (n != 0 && n != 1) {
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 855, "Can't attach file: %s"), file);
	xfree(file);
	return;
    }
    if (Access(file, R_OK) != 0) {
	error(SysErrWarning, file);
	xfree(file);
	return;
    }
    
    if (
#ifdef CREATE_ATTACH
	strcmp(XtName(w), NEW_BUTTON_NAME) == 0 ||
#endif /* CREATE_ATTACH */
	(!XtIsSensitive(ai->autotype_w) || XmToggleButtonGetState(ai->autotype_w) == 0)) {
	XtVaGetValues(ai->encoding_w, XmNmenuHistory, &widget, NULL);
	encoding = XtName(widget);
	if (ci_strcmp(encoding, "none") == 0)
	    encoding = "";
	XtVaGetValues(ai->datatype_w, XmNmenuHistory, &widget, NULL);
	type = XtName(widget);
	comment = XmTextGetString(ai->comment_w);
    } else {
	type = NULL;
	encoding = NULL;
	comment = (ai->comment_modified) ?
	    XmTextGetString(ai->comment_w) : SNGL_NULL;
    }

#ifdef CREATE_ATTACH
    if (!strcmp(XtName(w), NEW_BUTTON_NAME))
	turnon(a_flags, AT_TEMPORARY);
#endif /* CREATE_ATTACH */
    ask_item = w;
    err = add_attachment(compose, file, type, comment, encoding, a_flags, 
			 NULL);
    xfree(file);
    XtFree(comment);
    if (err && strcmp(err, catgets( catalog, CAT_SHELL, 160, "Cancelled" ))) {
	/* Figure out what to flash based on error message */
	if (zglob(err, "*type*"))
	    ask_item = ai->datatype_w;
	else if (zglob(err, "*encod*"))
	    ask_item = ai->encoding_w;
	else if (strcmp(err, strerror(ENOENT)))
	    ask_item = ai->file_w;
	else
	    ask_item = w;
	error(zglob(err, catgets( catalog, CAT_GUI, 66, "Cannot*" ))? SysErrWarning : UserErrWarning, err);
    } else {
	fill_attach_list_w(ai->list_w, compose->attachments, FrameCompose);
	draw_attach_area(compose->interface->attach_area, frame);
	ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
	if (!err) DismissSetWidget(w, DismissClose);
    }
}

static void
ai_from_finder(how, ffs, file)
FileFinderType how;
FileFinderStruct *ffs;
char *file;
{
    ZmFrame frame;
    
    if (how == FileFindDir)
	FileFinderDefaultSearch(how, ffs, file, NULL);
    else {
	/* attach the file, if this is a compose window. */
	FileFinderSelectItem(ffs, file, True);
	frame = FrameGetFrameOfParent(FrameGetData(ffs->list_w));
	if (FrameGetType(frame) == FrameCompose)
	    do_attachment(ffs->text_w, frame);
    }
}

static void
ai_from_attach(ai, a)
AttachInfo *ai;
Attach *a;
{
    FileFinderStruct *ffs;
    AttachKey *ak = (AttachKey *) 0;
    char *name;
    int n = !ai->is_compose;

    XtVaGetValues(XtParent(XtParent(ai->file_w)), XmNuserData, &ffs, NULL);
    /* Use fullpath even if the content_name is already a full path
     * to remove multiple slashes, etc.
     */
    name = FileFinderGetPath(ffs, a->a_name? a->a_name : a->content_name, &n);
    if (n == 0 || !ai->is_compose && n == 1) {
	FileFinderSetFullPath(ai->file_w, name);
	FileFinderSelectItem(ffs, name, False);  /* Select it if it's there */
    }
#ifdef NOT_NOW
    SetTextString(ai->datatype_w, attach_data_type(a));
    SetTextString(ai->encoding_w, a->encoding_algorithm);
#endif /* NOT_NOW */
    if (attach_data_type(a)) {
	if (!SetOptionMenuChoice(ai->datatype_w, attach_data_type(a), False))
	    (void) SetOptionMenuChoice(ai->datatype_w, catgets(catalog, CAT_MSGS, 826, "Unknown"), False);
    } else
	(void) SetOptionMenuChoice(ai->datatype_w, catgets(catalog, CAT_MSGS, 826, "Unknown"), False);
    if (a->encoding_algorithm) {
	if (!SetOptionMenuChoice(ai->encoding_w, a->encoding_algorithm, False))
	    (void) SetOptionMenuChoice(ai->encoding_w, catgets(catalog, CAT_MSGS, 826, "Unknown"), False);
    } else
	(void) SetOptionMenuChoice(ai->encoding_w, "None", False);
    if (a->content_abstract)
	SetTextString(ai->comment_w, a->content_abstract);
    else if (ak = get_attach_keys(FALSE, a, NULL))
	SetTextString(ai->comment_w, ak->description);
    update_gui_for_type(attach_data_type(a), ai, 0);
}

static void
fill_ai(list_w, frame, cbs)
Widget list_w;
ZmFrame frame;
XmListCallbackStruct *cbs;
{
    int offset;
    AttachInfo *ai;
    Attach *attachments, *a = 0;

    XtVaGetValues(list_w, XmNuserData, &ai, NULL);

    if (!ai)
	return;
    if (!frame || !cbs) {
	zmXmTextSetString(ai->file_w, NULL);
#ifdef NOT_NOW
	zmXmTextSetString(ai->datatype_w, NULL);
	zmXmTextSetString(ai->encoding_w, NULL);
#endif /* NOT_NOW */
	zmXmTextSetString(ai->comment_w, NULL);
	XtVaSetValues(ai->file_w, XmNuserData, NULL, NULL);
	return;
    }
    if (ai->is_compose)
	a = attachments = FrameComposeGetComp(frame)->attachments;
    else {
	if (attachments =
		((msg_data *)FrameGetClientData(frame))->this_msg.m_attach)
	    a = (Attach *)attachments->a_link.l_next;
    }
    if (!a)
	return;

    offset = cbs->item_position;
    a = (Attach *)retrieve_nth_link((struct link *) a, offset);
    XtVaSetValues(ai->file_w, XmNuserData, a, NULL);

    ai_from_attach(ai, a);
}

void
ack_new_mail(w, frame)
Widget w;
ZmFrame frame;
{
    int i, new, last_cnt;
    static int prev_cnt = -1;
    AskAnswer answer = AskYes;

    ask_item = w;
    FrameGet(frame, FrameFolder, &current_folder, FrameEndArgs);
    if (isoff(current_folder->mf_flags, CONTEXT_IN_USE))
	return;
    last_cnt = msg_cnt;
    turnoff(folder_flags, REINITIALIZED);	/* User has noticed */
    new = ison(folder_flags, NEW_MAIL); /* Remember the icon state */
    turnon(folder_flags, NEW_MAIL);	/* mail_status() turns it off */
#if defined( IMAP )
    zmail_mail_status(0);                       /* Reset NEW_MAIL correctly *
/
#else
    mail_status(0);                     /* Reset NEW_MAIL correctly */
#endif
    await(0, DUBL_NULL, &current_folder->mf_group);
    if (msg_cnt <= last_cnt) {
	char buf[MAXPATHLEN];
	if (ison(folder_flags, NEW_MAIL) && prev_cnt == last_cnt)
	    answer = ask(AskOk,
catgets( catalog, CAT_MOTIF, 346, "No new mail in %s since the last check.\n\
Clear New Arrival status of existing messages?" ),
			    folder_shortname(current_folder, buf));
	else {
	    if (!new)
		wprint(catgets( catalog, CAT_MOTIF, 347, "No new mail in %s since the last check.\n" ),
			folder_shortname(current_folder, buf));
	    answer = AskNo;
	}
    }
    if (msg_cnt && answer == AskYes) {
	clear_msg_group(&current_folder->mf_group);
	for (i = 0; i < last_cnt; i++)
	    if (ison(msg[i]->m_flags, NEW)) {
		turnoff(msg[i]->m_flags, NEW);
		add_msg_to_group(&current_folder->mf_group, i);
	    }
	gui_refresh(current_folder, REDRAW_SUMMARIES|ADD_NEW_MESSAGES);
    } else {
	turnoff(folder_flags, NEW_MAIL);
	gui_refresh(current_folder, REDRAW_SUMMARIES);
    }
    prev_cnt = msg_cnt;
}

int
gui_get_state(item)
int item;
{
    msg_folder *fldr;
    msg_data *data;
    FrameTypeName type;
    ZmFrame frame;
    int msgno;
    char *msg_str;

    if (!ask_item) return -1;
    frame = FrameGetData(GetTopShell(ask_item));
    switch (item) {
    case GSTATE_PINUP:
	FrameGet(frame, FrameType, &type, FrameEndArgs);
	return (type == FramePinMsg);
    case GSTATE_ATTACHMENTS:
	FrameGet(frame,
		 FrameType, &type,
		 FrameClientData, &data,
		 FrameEndArgs);
	if (type != FramePinMsg && type != FramePageMsg) return 0;
        if (data == NULL)
          return(0);
	return (data->this_msg.m_attach != 0);
    case GSTATE_IS_NEXT: case GSTATE_IS_PREV:
	FrameGet(frame, FrameFolder, &fldr, FrameEndArgs);
	if (current_folder != fldr) return False;
	msg_str = FrameGetMsgsStr(frame);
	msgno = (msg_str) ? atoi(msg_str)-1 : current_msg;
	return (next_msg(msgno, (item == GSTATE_IS_NEXT) ? 1 : -1) != msgno);
    case GSTATE_ACTIVE_COMP:
	if (FrameGetType(frame) != FrameCompose) return False;
	return (FrameComposeGetComp(frame)) ? True : False;
    case GSTATE_PHONETAG:
	FrameGet(frame,
		 FrameType, &type,
		 FrameClientData, &data,
		 FrameEndArgs);
	if (type == FramePinMsg)
          {
            if (data == NULL)
              return(0);
	    if (data->phone_tag_managed)
              return(1);
            else
              return(0);
          }
        else
          return(0);
    case GSTATE_TAGIT:
	FrameGet(frame,
		 FrameType, &type,
		 FrameClientData, &data,
		 FrameEndArgs);
	if (type == FramePinMsg)
          {
            if (data == NULL)
              return(0);
	    if (data->tag_it_managed)
              return(1);
            else
              return(0);
          }
        else
          return(0);
     default:
/* Unimplemented */
        return(0);
    }
/* Not reached */
  return(0);
}

static void
msg_panes_cb(paned, data)
    Widget paned;
    ZmCallbackData data;
{
    struct PaneToggle
    {
	char *name;
	unsigned position;
    };
    
msg_data *m_data;
ZmFrame current_frame;
    
    current_frame = FrameGetData(paned);
    m_data = (msg_data *)FrameGetClientData(current_frame);
    {
#ifndef __STDC__
        static
#endif /* !__STDC__ */
    struct PaneToggle messagePanes[] = {
        	{ "folder",	 0 },
        	{ "headers",	 1 },
             /* { "attachments", 1 }, */
        	{ "body",	 4 },
        	{ "action_area", 6 }
        };
        Widget manage[XtNumber(messagePanes)], unmanage[XtNumber(messagePanes)];
        unsigned stepManage = 0, stepUnmanage = 0, sweep;
        WidgetList children;
        XtVaGetValues(paned, XmNchildren, &children, 0);

        for (sweep = XtNumber(messagePanes); sweep--;)
        	if (chk_two_lists(data->xdata, messagePanes[sweep].name, " \t,"))
                    {
                      if (messagePanes[sweep].position == 1)
                        {
                          if (m_data->header_managed)
        	            manage[stepManage++] = children[messagePanes[sweep].position];
                        }
                      else
        	        manage[stepManage++] = children[messagePanes[sweep].position];
                    }
        	else
        	    unmanage[stepUnmanage++] = children[messagePanes[sweep].position];
    
        XtUnmanageChildren(unmanage, stepUnmanage);
        {
        	ZmFrame frame = FrameGetData(paned);
    
        	if (ison(FrameGetFlags(frame), FRAME_IS_OPEN))
        	    draw_attach_area(((msg_data *) FrameGetClientData(frame))->attach_area, frame);
        }
        XtManageChildren(manage, stepManage);
    }
}

static void
msg_hdr_fmt_cb(m_data, cdata)
msg_data *m_data;
ZmCallbackData cdata;
{
    ZmFrame frame = FrameGetData(m_data->hdr_fmt_w);
    /* It's not clear that both of these lines are necessary, but
     * let's be safe...
     */
    if (isoff(FrameGetFlags(frame), FRAME_IS_OPEN)) return;
    if (ison(FrameGetFlags(frame), FRAME_WAS_DESTROYED)) return;

    SAVE_RESIZE(GetTopShell(m_data->hdr_fmt_w));
    SET_RESIZE(True);

    zmXmTextSetString(m_data->hdr_fmt_w,
	format_hdr(m_data->this_msg_no, value_of(VarMsgWinHdrFmt), False));

    RESTORE_RESIZE();
}

static void
attach_menu_cb(ai, cdata)
AttachInfo *ai;
ZmCallbackData cdata;
{
    char *default_type;
    ZmFrame frame = FrameGetData(ai->shell);
    FrameTypeName type;

    if (isoff(FrameGetFlags(frame), FRAME_IS_OPEN)) return;
    if (ison(FrameGetFlags(frame), FRAME_WAS_DESTROYED)) return;

    type = FrameGetType(FrameGetFrameOfParent(frame));

    DestroyOptionMenu(ai->datatype_w);
    default_type = CreateDataTypeMenu(XtParent(ai->datatype_w), ai, type);

    DestroyOptionMenu(ai->encoding_w);
    CreateEncodingMenu(XtParent(ai->encoding_w), ai, type);

    update_gui_for_type(default_type, ai, 1);
    xfree(default_type);
}
