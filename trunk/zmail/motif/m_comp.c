/* m_comp.c	Copyright 1990, 1991 Z-Code Software Corp. */

#ifndef lint
static char	m_comp_rcsid[] =
    "$Id: m_comp.c,v 2.133 2005/05/09 09:15:20 syd Exp $";
#endif

#define DETACH_RIGHT_EDGE
#define HIDE_BCC

#include "zmail.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "addressArea/addressArea.h"
#include "attach/area.h"
#include "buttons.h"
#include "catalog.h"
#include "dirserv.h"
#include "config/features.h"
#include "hooks.h"
#include "m_comp.h"
#include "statbar.h"
#include "gui/zeditres.h"
#include "zm_motif.h"

#include <Xm/DialogS.h>
#include <Xm/SelectioB.h>
#include <Xm/ScrolledW.h>
#include <Xm/PushB.h>
#include <Xm/Separator.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */
#include <Xm/PanedW.h>
#include <Xm/MainW.h>

#if (XmVersion * 1000 + XmUPDATE_LEVEL) >= 1002003
#define USE_XMNSOURCE
#endif /* Motif 1.2.3 or later */

#ifdef SCO
#undef XtNumber
#define XtNumber(arr)	(sizeof(arr)/sizeof(arr[0]))
#endif /* SCO */

static void set_comp_items P((struct ComposeInterface *, struct Compose *));
static void free_compose_data P((struct Compose *));
#ifdef IXI_DRAG_N_DROP
static void textw_string P((Widget, XtPointer, char *, int));
#endif /* IXI_DRAG_N_DROP */
static void mark_as_modified P((Widget, ZmFrame, XmTextVerifyCallbackStruct *));


#include "bitmaps/comp.xbm"
ZcIcon comp_icon = {
    "compose_icon", 0, comp_width, comp_height, comp_bits
};


static int
comp_refresh(frame, fldr, reason)
ZmFrame frame;
msg_folder *fldr;
u_long reason;
{
    Compose *comp = (Compose *) 0;

    if (ison(reason, PREPARE_TO_EXIT)) {
	abort_mail(FrameGetChild(frame), False);
	return 0;
    }
    comp = FrameComposeGetComp(frame);
	
    if (FrameGetFolder(frame) != current_folder ||
	    ison(folder_flags, CONTEXT_RESET)) {
	FrameSet(frame,
	    FrameFolder,    current_folder,
	    FrameMsgString, "",
	    FrameEndArgs);
#ifdef NOT_NOW
	if (FrameGetFreeClient(frame)) {
	    Compose *compose = FrameComposeGetComp(frame);
	    clear_msg_group(&(compose->replied_to));
	    /* XXX Insensitize replied-to selection in include menu */
	}
#endif /* NOT_NOW */
    } else if (fldr == current_folder)
	/* Make sure the active folder name and message list matches tool */
	FrameCopyContext(FrameGetData(tool), frame);
    if (comp) {
	AttachInfo *ai;
	ai = comp->interface->attach_info;
	if (ai && number_of_links(comp->attachments) != ai->count)
	    fill_attach_list_w(ai->list_w, comp->attachments, FrameCompose);
	draw_attach_area(comp->interface->attach_area, frame);
	ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
    }
    return 0;
}

void
gui_clean_compose(ignored)
int ignored;
{
    return;	/* comp_refresh() handles this at time of exit */
}

/* When opening compositions from the toolbox or the "dialog" command,
 * return a null ZmFrame.  This prevents the same frame from being
 * reused the next time the toolbox or "dialog" command is used.
 */
ZmFrame
DialogCreateCompose(w, item)
Widget w, item;
{
    ask_item = w;
    do_cmd_line(item, "mail");
    return (ZmFrame)0;
}

void
reset_opts_menu(comp_shell, comp_curr)
Widget comp_shell;
Compose *comp_curr;
{
    reset_comp_opts(comp_curr, False);
}

/*
 * If a user-defined function "compose_mode_hook" exists when a composition is
 * started, zmail will call that function (with no arguments, i.e. $# == 0).
 * This happens only once per composition, e.g. if a composition in CLI mode
 * is suspended with ~z, the compose_mode_hook is not called again when the
 * composition resumes.  If the composition is started with "mail -z", the
 * hook is called *before* the composition suspends.  Also in CLI mode, the
 * hook is called *after* prompting for To/Subject headers, *after* running
 * the external editor (when autoedit is set), but *before* "compose mode"
 * input of the message body is begun.
 * 
 * In GUI mode, the compose_mode_hook is called *in the context of the compose
 * window*, which means that commands like "dialog CompAliases" that can only
 * be executed from a compose window are legal within the hook.  This differs
 * from interposers on the "mail" operation or command, which are run in the
 * context from which the mail command originated.  The order in which the
 * external auto-editor and the hook are executed in GUI mode is explicitly
 * *not defined*, so compose_mode_hook should not depend on the state of the
 * external editor.  This is because GUI mode is permitted to run the editor
 * asynchronously, so even if the hook is called after the editor is started,
 * the editor may still be running; "compcmd" is permitted to fail when used
 * from the compose_mode_hook in any GUI mode (including Lite) when autoedit
 * is set.
 * 
 * The current Motif implementation calls the hook after starting the auto-
 * editor but before it returns control to the compose window.  Lite or any
 * other GUI is allowed to call the hook before starting the editor if that
 * implementation is better for some reason (though "compcmd edit-visual"
 * etc. should either fail or behave sanely when autoedit is on).
 */
static void
gui_compose_hook(comp_frame)
ZmFrame comp_frame;
{
    Widget w = ask_item;

    if (lookup_function(COMPOSE_HOOK)) {
	ForceExposes();
	ask_item = FrameGetChild(comp_frame);
	(void) gui_cmd_line(COMPOSE_HOOK, comp_frame);
	ask_item = w;
    }
}

void remove_callback_cb();

#ifdef NOT_NOW
static void
toolbar_changed(toolbar, data)
    Widget toolbar;
    ZmCallbackData data;
{
    if (BListButtons(GetButtonList(button_panels[COMP_WINDOW_TOOLBAR]))
	    && chk_option(VarComposePanes, "toolbar"))
	XtManageChild(toolbar);
    else
	XtUnmanageChild(toolbar);
}    
#endif /* NOT_NOW */

static void
comp_panes_cb(paned, data)
    Widget paned;
    ZmCallbackData data;
{
    struct PaneToggle
    {
	char *name;
	unsigned position;
    };
    
#ifndef __STDC__
    static
#endif /* !__STDC__ */
    struct PaneToggle composePanes[] = {
	{ "folder",	 0 },
#ifdef MEDIAMAIL
	{ "body",	 6 },
	{ "action_area", 8 }
#else /* !MEDIAMAIL */
	{ "body",	 5 },
	{ "action_area", 7 }
#endif /* !MEDIAMAIL */
    };
    
    Widget manage[XtNumber(composePanes)], unmanage[XtNumber(composePanes)];
    unsigned stepManage = 0, stepUnmanage = 0, sweep;
    WidgetList children;
    
    XtVaGetValues(paned, XmNchildren, &children, 0);

    for (sweep = XtNumber(composePanes); sweep--;)
	if (chk_two_lists(data->xdata, composePanes[sweep].name, " \t,"))
	    manage[stepManage++] = children[composePanes[sweep].position];
	else
	    unmanage[stepUnmanage++] = children[composePanes[sweep].position];

#ifdef NOT_NOW
    toolbar_changed(children[1], data);
#endif /* NOT_NOW */

    XtUnmanageChildren(unmanage, stepUnmanage);
    {
        ZmFrame frame = FrameGetData(paned);
        if (FrameGetFreeClient(frame))
            draw_attach_area(((Compose *) FrameGetClientData(frame))->interface->attach_area, frame);
    }
    XtManageChildren(manage, stepManage);
}    


#if XmVersion >= 1002 /* drag & drop */
#include "drag-drop.h"

static void
drop_sites_swap(active, inactive)
    Widget active, inactive;
{
    XmDropSiteStartUpdate(active);
    zmDropSiteDeactivate(inactive);
    zmDropSiteActivate(active);
    XmDropSiteEndUpdate(active);
}
#else /* no drag & drop */
#define drop_sites_swap(active, inactive)
#endif /* no drag & drop */

static Widget find_hdr_focus();

/*
  This function returns the UNIX epoch date the date and the time as strings.
*/
void phone_tag_date(the_id,the_date,the_time)
char *the_id;
char *the_date;
char *the_time;
{
time_t t;
struct tm *T;
char Zone[4];
 
  t = time((time_t *)0);
  sprintf(the_id,"%ld",t);
  T = time_n_zone(Zone);
  sprintf(the_date,"%d/%d/%02d",T->tm_mon+1,T->tm_mday,T->tm_year%100);
  if (T->tm_hour > 12)
    sprintf(the_time,"%d:%02dPM",(T->tm_hour)-12,T->tm_min);
  else if (T->tm_hour == 12)
    sprintf(the_time,"%d:%02dPM",T->tm_hour,T->tm_min);
  else
    sprintf(the_time,"%d:%02dAM",T->tm_hour,T->tm_min);
}

/*
  This function sets the address chosen in the address browser into the
  phone tag To: address.
*/
void 
set_phone_tag_address(w)
Widget w;
{
    Compose *compose;
    int is_set;
    ZmFrame frame;

    frame = FrameGetData(w);
    compose = FrameComposeGetComp(frame);
    if (!compose) return;
    if (compose->addresses[TO_ADDR])
      XmTextSetString(compose->interface->phone_tag_text_widget[0],compose->addresses[TO_ADDR]);
}

/*
  This function sets the address chosen in the address browser into the
  tag it To: address.
*/
void 
set_tag_it_address(w)
Widget w;
{
    Compose *compose;
    int is_set;
    ZmFrame frame;

    frame = FrameGetData(w);
    compose = FrameComposeGetComp(frame);
    if (!compose) return;
    if (compose->addresses[TO_ADDR])
      XmTextSetString(compose->interface->tag_it_text_widget[1],compose->addresses[TO_ADDR]);
}

static void
turn_tag_it_on(compose)
Compose *compose;
{
Widget focus_widget;

  compose->interface->tag_it_managed = True;
  XtManageChild(compose->interface->tag_it_text_widgets);

  focus_widget = compose->interface->tag_it_text_widget[1];
  XmProcessTraversal(focus_widget,XmTRAVERSE_CURRENT);
  XmTextSetCursorPosition(focus_widget, 0);
}

static void
turn_tag_it_off(compose)
Compose *compose;
{
  compose->interface->tag_it_managed = False;
  XtUnmanageChild(compose->interface->tag_it_text_widgets);
}

static void
turn_phone_tag_on(compose)
Compose *compose;
{
Widget focus_widget;

  compose->interface->phone_tag_managed = True;
  XtManageChild(compose->interface->phone_tag_text_widgets);
  XtManageChild(compose->interface->phone_tag_toggle_widgets);
  
  focus_widget = compose->interface->phone_tag_text_widget[0];
  XmProcessTraversal(focus_widget,XmTRAVERSE_CURRENT);
  XmTextSetCursorPosition(focus_widget, 0);
}

static void
turn_phone_tag_off(compose)
Compose *compose;
{
  compose->interface->phone_tag_managed = False;
  XtUnmanageChild(compose->interface->phone_tag_text_widgets);
  XtUnmanageChild(compose->interface->phone_tag_toggle_widgets);
}

static void
turn_regular_on(frame,compose)
ZmFrame frame;
Compose *compose;
{
char *st;
  XtManageChild(XtParent(XtParent(frame->folder_label)));
  AddressAreaManage(compose->interface->prompter);
  if (chk_option(VarComposePanes, "folder"))
    XtManageChild(XtParent(XtParent(frame->folder_label)));
  else
    XtUnmanageChild(XtParent(XtParent(frame->folder_label)));
  st = get_compose_state();
  if (strstr(st,"edit_headers") == NULL)
    AddressAreaFocus(compose->interface->prompter,True);
  else
    AddressAreaFocus(compose->interface->prompter,False);
}

static void
turn_regular_off(frame,compose)
ZmFrame frame;
Compose *compose;
{
  AddressAreaUnmanage(compose->interface->prompter);
  XtUnmanageChild(XtParent(XtParent(frame->folder_label)));
}

/*
  This function toggles the headers on and off.
*/
static void 
toggle_headers(w)
Widget w;
{
    Compose *compose;
    int is_set;
    ZmFrame frame;
    Widget f;
    char *st;

    frame = FrameGetData(w);
    compose = FrameComposeGetComp(frame);
    if (!compose) return;
    
/* Detect a change in the state of the tag_it flag */
    if (ison(compose->send_flags, TAG_IT))
      {
        if (!compose->interface->tag_it_managed)
          {
            turn_regular_off(frame,compose);
            turn_tag_it_on(compose);
          }
      }
    else
      {
        if (compose->interface->tag_it_managed)
          {
             turn_tag_it_off(compose);
             turn_regular_on(frame,compose);
          }
      }

/* Detect a change in the state of the phone_tag flag */
    if (ison(compose->send_flags, PHONE_TAG))
      {
        if (!compose->interface->phone_tag_managed)
          {
            turn_regular_off(frame,compose);
            turn_phone_tag_on(compose);
          }
      }
    else
      {
        if (compose->interface->phone_tag_managed)
          {
            turn_phone_tag_off(compose);
            turn_regular_on(frame,compose);
          }
      }
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

static struct phone_tag_output_table {
    char *the_format;
    int  the_source_offset;
  };

static struct phone_tag_output_table phone_tag_header[] = {
    " \n" , -1,
    "$EOM$\n", -1,
    "\n" , -1,
    "From: %s\n",  1,
    "Of: %s\n", 2,
    "Area Code: %s\n", 7,
    "No.: %s\n", 8,
    "Ext.: %s\n", 9,
    "Fax #: %s\n", 10,
    "Date: %s (M/d/yy)\n", 5,
    "Time: %s\n", 6,
    NULL, -1
  };

static struct phone_tag_output_table phone_tag_checkbox[] = {
    "Phoned: %s\n", 0,
    "Call back: %s\n", 1,
    "Returned Call: %s\n", 2,
    "Wants to see you: %s\n", 3,
    "Was in: %s\n", 4,
    "Will call again: %s\n", 5,
    "Urgent: %s\n", 6,
    NULL, -1 
  };

static struct phone_tag_output_table phone_tag_trailer[] = {
    "Special attention: No\n", -1,
    "Signed: \"%s\" <%s>\n", -2,
    "To: %s\n", 0,
    "ID: %s 0\n", -3,
    "Memo type: 0\n", -1,
    NULL,-1 
  };

/*
  These tables control the look of the tag it headers.
*/
static char *tag_it_label_list[] = {
    "Date: %s Time: %s",
    "To:",
    "From: \"%s\" <%s>",
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

static void get_my_return_address(compose,buf)
Compose *compose;
char *buf;
{
       char *pF, *host = NULL;
 
        if (ison(compose->mta_flags, MTA_HIDE_HOST))
            host = NULL;
        else
            host = get_from_host(isoff(compose->mta_flags, MTA_ADDR_UUCP),
                                 False);
        if (host && *host) {
            if (ison(compose->mta_flags, MTA_ADDR_UUCP))
                (void) sprintf(buf, "%s!%s", host, zlogin);
            else
                (void) sprintf(buf, "%s@%s", zlogin, host);
            pF = buf + strlen(buf);
        } else
            pF = buf + Strcpy(buf, zlogin);
}

/*
  This function returns the users return address.
*/
void get_user_return_address(compose,signed_by)
Compose *compose;
char *signed_by;
{
int j , k;
char *host;
char *p;

  p = value_of(VarFromAddress);
  if (p == NULL)
    {
      strcpy(signed_by,"");
      get_my_return_address(compose,signed_by);
    }
  else
    strcpy(signed_by,p);
}

/* 
  This function resets the tag it and phone tag headers.
*/
void
reset_tagit_phone_tag(comp_curr)
Compose *comp_curr;
{
int i;
char the_string[128];
char signed_by[128];
char the_date[16];
char the_time[16];
char the_id[16];
char *p;

  phone_tag_date(the_id,the_date,the_time);

/* Reset the tag it widgets */

  i = 0;
  while (tag_it_label_list[i] != NULL)
    {
      if (tag_it_label_type[i] == 0)
        {
          sprintf(the_string,tag_it_label_list[i],the_date,the_time);
          XtVaSetValues(comp_curr->interface->tag_it_text_widget[i], XmNlabelString, zmXmStr(the_string), NULL);
        }
      else if (tag_it_label_type[i] == 1)
        {
          XmTextSetString(comp_curr->interface->tag_it_text_widget[i],""); 
        }
      else if (tag_it_label_type[i] == 2)
        {
          get_user_return_address(comp_curr,signed_by);
          if ((p = value_of(VarRealname)) || (p = value_of(VarName)))
            sprintf(the_string,tag_it_label_list[i],p,signed_by);
          else
            strcpy(the_string,signed_by);
          XtVaSetValues(comp_curr->interface->tag_it_text_widget[i], XmNlabelString, zmXmStr(the_string), NULL);
        }
      else
        ;
      i++;
    }

/* Reset the phone tag widgets */

  i = 0;
  while (phone_tag_label_list[i] != NULL)
    {
      if (strcmp(phone_tag_label_list[i],"blank") == 0)
        ;
      else if (strcmp(phone_tag_label_list[i],"Date:") == 0)
        {
          XmTextSetString(comp_curr->interface->phone_tag_text_widget[i],
                         the_date);
          i++;
          XmTextSetString(comp_curr->interface->phone_tag_text_widget[i],
                          the_time);
        }
      else
        {
          XmTextSetString(comp_curr->interface->phone_tag_text_widget[i],""); 
        }
      i++;
    }

/* Reset the phone tag toggle widgets */

  i = 0;
  while (phone_tag_toggle_list[i] != NULL)
    {
      XmToggleButtonSetState(comp_curr->interface->phone_tag_toggle_widget[i],False,False);
      i++;
    }
}
 
/*
 * Then sending a phone tag message the To: address and the Subject: are
 * replaced.  A special X header is placed into the message and additional
 * information is added at the end of the message. When sending a Tag-It
 * message a special X header is placed into the message. When sending a
 * message with a rerd receipt requested  a special X header is placed into 
 * the message. 
*/
void
gui_compose_headers(compose)
Compose *compose;
{
int i , j , k , d , m , y , hr , minit , am , ch;
char *str , *host;
char state[4];
char signed_by[128];
char the_message_id[16];
char the_date[16];
char the_time[16];
char the_input_date[16];
char the_input_time[16];
char Zone[4];
char *tempmsg_name = 0;
char *p;
time_t seconds , curr_t;
struct tm T;
struct tm *curr_T;
FILE *tempmsg;
  
  if (ison(compose->send_flags,READ_RECEIPT))
    {
      get_user_return_address(compose,signed_by);
      input_header("X-Chameleon-Return-To",signed_by,False);
    }

  if (ison(compose->send_flags,TAG_IT))
    {
/* Set the input address into the header */
      str = XmTextGetString(compose->interface->tag_it_text_widget[1]);
      input_address(TO_ADDR,str);
/* Set the subject into the header */
      input_address(SUBJ_ADDR,"Tag-It!");
/* Set the special tag it header into the header */
      input_header("X-Chameleon-TagIt","Chameleon Tag It Ver 4.10",False);
      return;
    }
 
  if (ison(compose->send_flags,PHONE_TAG))
    {
/* Get the current date and time */
      curr_t = time((time_t *)0);
      curr_T = time_n_zone(Zone);
/* Set the message id date and time with the current date and time */
      phone_tag_date(the_message_id,the_date,the_time);
/* Get the input date and time from the dialog */
      i = 0;
      while (phone_tag_header[i].the_format != NULL)
        {
          if (phone_tag_header[i].the_source_offset == 5)
            {
              strncpy(the_input_date,XmTextGetString(compose->interface->phone_tag_text_widget[phone_tag_header[i].the_source_offset]),15);
              the_input_date[15] = '\0';
            }
          else if (phone_tag_header[i].the_source_offset == 6)
            {
              strncpy(the_input_time,XmTextGetString(compose->interface->phone_tag_text_widget[phone_tag_header[i].the_source_offset]),15);
              the_input_time[15] = '\0';
            }
          i++;
        }
/* If the input date or input time has changed redo the message id */
      if (!((strcmp(the_date,the_input_date) == 0) && 
            (strcmp(the_time,the_input_time) == 0)))
        {
          sscanf(the_input_date,"%d/%d/%d",&m,&d,&y);
          sscanf(the_input_time,"%d:%d",&hr,&minit);
          if      (am=strstr(the_input_time,"A") != 0)
            am = 0;
          else if (am=strstr(the_input_time,"a") != 0)
            am = 0;
          else if (am=strstr(the_input_time,"P") != 0)
            am = 12;
          else if (am=strstr(the_input_time,"p") != 0)
            am = 12;
          else
            am = 0;
          if (y >= 0 && y < 69)
            y += 100;
          else if (y > 1900)
            y -= 1900;
          if (hr < 12)
            hr = hr + am;
/* If it passes the check then override the date time and message id */
          if ((y > 0) &&
              ((m>= 1) && (m<=12)) &&
              ((d>=1) && (d<=31)) &&
              ((hr>=1) && (hr<24)) &&
              ((minit>=0) && (minit<=59)))
            {
              T.tm_mon = m-1;
              T.tm_mday = d;
              T.tm_year = y;
              T.tm_hour = hr;
              T.tm_min = minit;
              T.tm_sec = 0;
              seconds = time2gmt(&T,Zone,1);
              strcpy(the_date,the_input_date);
              strcpy(the_time,the_input_time);
              sprintf(the_message_id,"%ld",seconds);
            }
        }
/* Set the input address into the header */
      str = XmTextGetString(compose->interface->phone_tag_text_widget[0]);
      input_address(TO_ADDR,str);
/* Set the subject into the header */
      str = XmTextGetString(compose->interface->phone_tag_text_widget[1]);
      sprintf(signed_by,"Phone Tag - %s",str);
      input_address(SUBJ_ADDR,signed_by);
/* Set the special phone tag header into the header */
      input_header("X-Chameleon-PhoneTag","Chameleon Phone Tag Version 6.0",False);
/* Write out the Phone-Tag lines at the end of the message */
      i = 0;
      while (phone_tag_header[i].the_format != NULL)
        {
          if (phone_tag_header[i].the_source_offset == -1)
            fprintf(compose->ed_fp,phone_tag_header[i].the_format);
          else if (phone_tag_header[i].the_source_offset == 5)
            fprintf(compose->ed_fp,phone_tag_header[i].the_format,the_date);
          else if (phone_tag_header[i].the_source_offset == 6)
            fprintf(compose->ed_fp,phone_tag_header[i].the_format,the_time);
          else
            {
              str = XmTextGetString(compose->interface->phone_tag_text_widget[phone_tag_header[i].the_source_offset]);
              fprintf(compose->ed_fp,phone_tag_header[i].the_format,str);
            }
          i++;
        }
      i = 0;
      while (phone_tag_checkbox[i].the_format != NULL)
        {
          if (XmToggleButtonGetState(compose->interface->phone_tag_toggle_widget[phone_tag_checkbox[i].the_source_offset]))
            strcpy(state,"Yes");
          else
            strcpy(state,"No");
          fprintf(compose->ed_fp,phone_tag_checkbox[i].the_format,state);
          i++;
        }
      i = 0;
      while (phone_tag_trailer[i].the_format != NULL)
        {
          if (phone_tag_trailer[i].the_source_offset == -3)
            fprintf(compose->ed_fp,phone_tag_trailer[i].the_format,the_message_id);
          else if (phone_tag_trailer[i].the_source_offset == -2)
            {
              get_user_return_address(compose,signed_by);
              if ((p = value_of(VarRealname)) || (p = value_of(VarName)))
                fprintf(compose->ed_fp,phone_tag_trailer[i].the_format,p,signed_by);
              else
                fprintf(compose->ed_fp,"Signed: %s\n",signed_by);
            }
          else if (phone_tag_trailer[i].the_source_offset == -1)
            fprintf(compose->ed_fp,phone_tag_trailer[i].the_format);
          else
            {
              str = XmTextGetString(compose->interface->phone_tag_text_widget[phone_tag_trailer[i].the_source_offset]);
              fprintf(compose->ed_fp,phone_tag_trailer[i].the_format,str);
            }
          i++;
        }
/* Close the message file and reopen it as an input file */
      fclose(compose->ed_fp);
      compose->ed_fp = fopen(compose->edfile,"r");
/* Open a temporary file as an output file */
      tempmsg = open_tempfile("comp",&tempmsg_name);
/* Write the Phone-Tag 'Message: ' tag into the output file */
      fprintf(tempmsg,"Message: ");
/* Copy the message file into the temporary file */
      while ((ch = fgetc(compose->ed_fp)) != EOF)
        fputc(ch,tempmsg);
/* Close both files */
      fclose(compose->ed_fp);
      fclose(tempmsg);
/* Open the temporary file as input and the message file as output */
      tempmsg = fopen(tempmsg_name,"r");
      compose->ed_fp = fopen(compose->edfile,"w");
/* Copy the temporary file into the message file */
      while ((ch = fgetc(tempmsg)) != EOF)
        fputc(ch,compose->ed_fp);
/* Close the temporary file and delete it */
      fclose(tempmsg);
      unlink(tempmsg_name);
/* Close the message file and repopen it as update */
      fclose(compose->ed_fp);
      compose->ed_fp = fopen(compose->edfile,"r+");
      return;
    }
}

int
gui_open_compose(w, comp_curr)
Widget w;
Compose *comp_curr;
{
    Widget comp_shell, /* xm_frame, */ form, pane;
    Widget comp_main_w, /* menu, rc, */ menu_bar, *textw;
    Arg args[10];
    ZmFrame comp_frame, start;
    char *mw = value_of(VarMsgWin);
    int found = 0, msg_win = mw? atoi(mw): 0, wrap = 0;
    FrameTypeName type;
    caddr_t free_client;
    u_long flags;
    ZmCallback zcb;

    if (!w)
	w = tool;

    start = comp_frame = FrameGetData(w);

    /* Find a compose frame that is free to edit a new letter.
     * Start with the frame of the "w" passed in to possibly
     * head off the search by using the current comp-frame.
     */
    do  {
	FrameGet(comp_frame,
	    FrameType,       &type,
	    FrameFreeClient, &free_client,
	    FrameFlags,      &flags,
	    FrameEndArgs);
	if (type == FrameCompose && free_client == 0 &&
		isoff(flags, FRAME_WAS_DESTROYED)) {
	    found = 1;
	    break;
	}
	comp_frame = nextFrame(comp_frame);
    } while (start != comp_frame);

    if (found) {
	Widget child;
	/* Normally, interface is stored in the Compose structure
	 * which is in turn pointed to by the FrameClientData.  However,
	 * when composition is not active, interface is stored in
	 * the FrameClientData, and the Compose structure is freed.
	 * See abort_mail() and do_send().
	 */
	FrameGet(comp_frame,
	    FrameChild,      &child,
	    FrameClientData, &comp_curr->interface,
	    FrameEndArgs);
	FrameSet(comp_frame,
	    FrameClientData, comp_curr,
	    FrameFreeClient, free_compose_data,
	    FrameEndArgs);
	FrameCopyContext(start, comp_frame);

	comp_shell = GetTopShell(child);

	/* pf Sun Sep 12 18:21:17 1993
	 * do this before set_comp_items, or else weird things happen
	 * when you bring up an old compose window with edit_headers
	 * if we're requesting no edit_headers.
	 */
#if 0
	reset_hdr_prompts(comp_shell, comp_curr);
#endif /* 0 */

	/* Must set_comp_items() before LoadComposition() to get
	 * start_textsw_edit() to work properly WRT setting
	 * the input to the text window.  Has somthing to do
	 * with sensitizing the parent of COMP_TEXT_W before
	 * sensitizing the text widget itself.  Tab groups?
	 */
	turnon(comp_curr->flags, ACTIVE); /* pf */
	set_comp_items(comp_curr->interface, comp_curr);
	/* Bart: Sat Sep  5 16:03:20 PDT 1992
	 * This is a no-op, I've forgotten why we're doing it ...
	grab_addresses(comp_curr, INIT_HEADERS);
	 */
	/* Must also set the COMP_AUTOFORMAT state to properly
	 * initialize COMP_TEXT_W before opening the file.
	 */
	reset_opts_menu(comp_shell, comp_curr);

        /* Reset the tagit and phone tag widgets */
        reset_tagit_phone_tag(comp_curr);
        turn_tag_it_off(comp_curr);
        turn_phone_tag_off(comp_curr);
        turn_regular_on(comp_frame,comp_curr);

#ifdef DETACH_RIGHT_EDGE
	if (mw = value_of(VarWrapcolumn))
	    msg_win = atoi(mw);
	else
	    msg_win = 0;
	if (msg_win) {
	    XtVaSetValues(comp_curr->interface->comp_items[COMP_TEXT_W],
		XmNcolumns, msg_win, NULL);
	    XtVaSetValues(comp_curr->interface->comp_items[COMP_SWAP_TEXT_W],
		XmNcolumns, msg_win, NULL);
	}
#endif /* DETACH_RIGHT_EDGE */

	LoadComposition(comp_curr);
	turnoff(comp_curr->flags, MODIFIED);

	if (isoff(comp_curr->flags, EDIT)) {
	    FrameSet(comp_frame,
		FrameFlagOn, FRAME_IS_OPEN, 
		FrameEndArgs);

	    AddressAreaUse(comp_curr->interface->prompter, comp_curr);
	    draw_attach_area(comp_curr->interface->attach_area, comp_frame);

	    FramePopup(comp_frame);
	} else
	    do_edit(comp_curr->interface->comp_items[COMP_TEXT_W], True, NULL);

	gui_compose_hook(comp_frame);

	return 0;
    }

    timeout_cursors(TRUE);

    /* "Should this name really be the same as the comp_main_w
     *  name? Does it hurt anything that they are the same??"
     */
    comp_shell = XtVaAppCreateShell("compose_window",
	ZM_APP_CLASS, applicationShellWidgetClass, display,
	NULL);
    SetDeleteWindowCallback(comp_shell, abort_mail, (char *)True);
    EditResEnable(comp_shell);

    comp_main_w = XtVaCreateWidget("compose_window",
	xmMainWindowWidgetClass, comp_shell,
        XmNshadowThickness,	0,
	XmNscrollingPolicy,	XmAPPLICATION_DEFINED,
	NULL);
    DialogHelpRegister(comp_main_w, "Compose Window");

    comp_curr->interface = (struct ComposeInterface *) XtCalloc(1, sizeof(struct ComposeInterface));

    /* compose_window_child here to match "real" FrameCreate */
    pane = XtVaCreateWidget("compose_window",
#ifdef SANE_WINDOW
	zmSaneWindowWidgetClass, comp_main_w,
#else /* !SANE_WINDOW */
	xmPanedWindowWidgetClass, comp_main_w,
	XmNsashWidth,	1,
	XmNsashHeight,	1,
#endif /* !SANE_WINDOW */
	XmNseparatorOn, False,
	NULL);
    zcb = ZmCallbackAdd(VarComposePanes, ZCBTYPE_VAR, comp_panes_cb, pane);
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, zcb);

    comp_frame = FrameCreate("compose_window", FrameCompose, tool,
	FrameIsPopup,    False,
	FrameClass,      NULL,
	FrameChildClass, NULL,
	FrameChild,      &pane,
	FrameRefreshProc,comp_refresh,
	FrameIcon,       &comp_icon,
	FrameFlags,	 FRAME_SHOW_ICON | FRAME_SHOW_FOLDER |
			 FRAME_EDIT_LIST | FRAME_SUPPRESS_ICON |
#ifdef DETACH_RIGHT_EDGE
			 FRAME_CANNOT_SHRINK_H |
#endif /* DETACH_RIGHT_EDGE */
			 FRAME_IGNORE_DEL, /* We've set a delete callback */
	FrameEndArgs);
    XtVaSetValues(comp_main_w, XmNuserData, comp_frame, NULL);
    DialogHelpRegister(GetNthChild(pane, 0), "Compose Folder Panel");

    menu_bar = BuildMenuBar(comp_main_w, COMP_WINDOW_MENU);
    XtManageChild(menu_bar);

    {
#ifdef MEDIAMAIL
	Widget toolbar = ToolBarCreate(pane, COMP_WINDOW_TOOLBAR, True);
#else /* !MEDIAMAIL */
	Widget toolbar = ToolBarCreate(comp_main_w, COMP_WINDOW_TOOLBAR, True);
#endif /* !MEDIAMAIL */
	DialogHelpRegister(toolbar, "Compose Tool Bar");
    }
    {
	StatusBar *status = StatusBarCreate(comp_main_w);
	FrameSet(comp_frame, FrameStatusBar, status, FrameEndArgs);
	statusBar_SetHelpKey(status, "Compose Status Bar");
    }
    
    /* Headers area: To:, Cc:, Bcc:, Subject: */
    comp_curr->interface->prompter = AddressAreaCreate(pane, &comp_curr->interface->attach_area, &comp_curr->interface->comp_items[COMP_TEXT_W]);

    /* Tag-Tt area */
    comp_curr->interface->tag_it_text_widgets = XtVaCreateWidget("tag_it_text_rc", xmRowColumnWidgetClass,pane,
        XmNorientation, XmVERTICAL,
        XmNmarginHeight, 0,
        XmNnumColumns, 1,
        XmNpacking, XmPACK_COLUMN,
        NULL);
    {
      int i;
      char widget_name[32];
      char the_string[128];
      char signed_by[128];
      Widget label_widget;
      char the_date[16];
      char the_time[16];
      char the_id[16];
      char *p;
      phone_tag_date(the_id,the_date,the_time);
      i = 0;
      while (tag_it_label_list[i] != NULL)
        {
          if (tag_it_label_type[i] == 0)
            {
              sprintf(widget_name,"tag_it_text_%d",i);
              sprintf(the_string,tag_it_label_list[i],the_date,the_time);
              comp_curr->interface->tag_it_text_widget[i] = 
                XtVaCreateManagedWidget(
                  widget_name, xmLabelGadgetClass, 
                  comp_curr->interface->tag_it_text_widgets, NULL);
                  XtVaSetValues(comp_curr->interface->tag_it_text_widget[i], 
                  XmNlabelString, zmXmStr(the_string), NULL);
            }
          else if (tag_it_label_type[i] == 1)
            {
              sprintf(widget_name,"tag_it_text_%d",i);
              comp_curr->interface->tag_it_text_widget[i] = 
                CreateLabeledTextSetWidth(
                  widget_name,comp_curr->interface->tag_it_text_widgets,
                  tag_it_label_list[i],
                  CLT_HORIZ|CLT_REPLACE_NL,35,tag_it_label_length[i],
                  &label_widget); 
              XmTextSetString(comp_curr->interface->tag_it_text_widget[i],""); 
            }
          else if (tag_it_label_type[i] == 2)
            {
              get_user_return_address(comp_curr,signed_by);
              sprintf(widget_name,"tag_it_text_%d",i);
              if ((p = value_of(VarRealname)) || (p = value_of(VarName)))
                sprintf(the_string,tag_it_label_list[i],p,signed_by);
              else
                strcpy(the_string,signed_by);
              comp_curr->interface->tag_it_text_widget[i] = 
                XtVaCreateManagedWidget(
                  widget_name, xmLabelGadgetClass, 
                  comp_curr->interface->tag_it_text_widgets, NULL);
                  XtVaSetValues(comp_curr->interface->tag_it_text_widget[i], 
                  XmNlabelString, zmXmStr(the_string), NULL);
            }
          else
            ;
          i++;
        }
    }

    /* Phone-Tag text area */

    if (pane) {
        comp_curr->interface->phone_tag_text_widgets = XtVaCreateWidget(
            "phone_tag_text_rc", xmRowColumnWidgetClass,pane,
            XmNorientation, XmVERTICAL,
            XmNmarginHeight, 0,
            XmNnumColumns, 2,
            XmNpacking, XmPACK_COLUMN,
            NULL);
        {
            int i;
            char widget_name[32];
            Widget col1 , col2, parent_widget;
            Widget label_widget;
            Widget date_time_widget;
            char the_id[16];
            char the_date[16];
            char the_time[16];
            phone_tag_date(the_id,the_date,the_time);
            col1 = XtVaCreateWidget("phone_tag_col1", 
              xmRowColumnWidgetClass,
              comp_curr->interface->phone_tag_text_widgets,
              XmNorientation, XmVERTICAL,
              XmNmarginHeight, 0,
              NULL);
            col2 = XtVaCreateWidget("phone_tag_col2", 
              xmRowColumnWidgetClass,
              comp_curr->interface->phone_tag_text_widgets,
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
                    comp_curr->interface->phone_tag_text_widget[i] = 
                    XtVaCreateManagedWidget(
                        widget_name, xmLabelGadgetClass, 
                        parent_widget, NULL);
                    XtVaSetValues(comp_curr->interface->phone_tag_text_widget[i], 
                        XmNlabelString, zmXmStr(" "), NULL);
                }
                else if (strcmp(phone_tag_label_list[i],"Date:") == 0) {
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    date_time_widget = XtVaCreateWidget("date_time_rc", 
                        xmRowColumnWidgetClass,
                        parent_widget,
                        XmNorientation, XmHORIZONTAL,
                        XmNmarginHeight, 0,
                        NULL);
                    comp_curr->interface->phone_tag_text_widget[i] = 
                        CreateLabeledTextSetWidth(widget_name,date_time_widget,
                            phone_tag_label_list[i],
                            CLT_HORIZ|CLT_REPLACE_NL,40,
                            phone_tag_label_length[i],
                            &label_widget); 
                    XmTextSetString(
                        comp_curr->interface->phone_tag_text_widget[i],
                        the_date);
                    i++;
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    comp_curr->interface->phone_tag_text_widget[i] = 
                    CreateLabeledTextSetWidth("phone_tag_time",
                        date_time_widget, phone_tag_label_list[i],
                        CLT_HORIZ|CLT_REPLACE_NL,40,phone_tag_label_length[i],
                        &label_widget); 
                    XmTextSetString(
                        comp_curr->interface->phone_tag_text_widget[i],
                        the_time);
                    XtManageChild(date_time_widget);
                }
                else {
                    sprintf(widget_name,"phone_tag_text_%d",i);
                    comp_curr->interface->phone_tag_text_widget[i] = 
                        CreateLabeledTextSetWidth(
                            widget_name,parent_widget,
                            phone_tag_label_list[i],
                            CLT_HORIZ|CLT_REPLACE_NL,75,
                            phone_tag_label_length[i],
                            &label_widget); 
                    XmTextSetString(
                        comp_curr->interface->phone_tag_text_widget[i],""); 
                }
                i++;
            }
            XtManageChild(col1);
            XtManageChild(col2);
        }
    }

    /* create two text widgets -- one for autoformat, the other not */
    textw = (WidgetList)XtCalloc(2, sizeof(Widget));
    form = XtVaCreateWidget("body", xmFormWidgetClass, pane,
#ifdef SANE_WINDOW
	ZmNextResizable, True,
#endif /* SANE_WINDOW */
	NULL);
    DialogHelpRegister(form, "Compose Body");

    (void) BuildPopupMenu(form, COMPOSE_TEXT_POPUP_MENU);

    XtVaSetArgs(args, XtNumber(args),
	XmNeditMode,		XmMULTI_LINE_EDIT,
	XmNverifyBell,		False,
	XmNwordWrap,		False,
	XmNscrollHorizontal,	True,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
#ifdef DETACH_RIGHT_EDGE
	XmNrightAttachment,     boolean_val("stretch_compose") ?
				    XmATTACH_FORM : XmATTACH_NONE,
#else
	XmNrightAttachment,     XmATTACH_FORM,
#endif /* DETACH_RIGHT_EDGE */
	XmNrows,		msg_win,
	NULL);
    textw[0] = XmCreateScrolledText(form, "compose_text", args, 8 + !!msg_win);
    XtAddCallback(textw[0], XmNmodifyVerifyCallback, (XtCallbackProc) mark_as_modified, comp_frame);
    XtVaSetValues(textw[0], XmNuserData, NULL, NULL);
#ifdef IXI_DRAG_N_DROP
    DropRegister(textw[0], NULL, NULL, textw_string, NULL);
#endif /* IXI_DRAG_N_DROP */
    /* (void) BuildPopupMenu(textw[0], COMPOSE_TEXT_POPUP_MENU); */

    XtSetArg(args[2], XmNwordWrap, True);
    XtSetArg(args[3], XmNscrollHorizontal, False);
    textw[1] = XmCreateScrolledText(form, "compose_text", args, 8 + !!msg_win);
    XtAddCallback(textw[1], XmNmodifyVerifyCallback, (XtCallbackProc) mark_as_modified, comp_frame);
    XtVaSetValues(textw[1], XmNuserData, NULL, NULL);
#ifdef IXI_DRAG_N_DROP
    DropRegister(textw[1], NULL, NULL, textw_string, NULL);
#endif /* IXI_DRAG_N_DROP */
    /* (void) BuildPopupMenu(textw[1], COMPOSE_TEXT_POPUP_MENU); */

#ifdef USE_XMNSOURCE
    XmTextSetSource(textw[1], XmTextGetSource(textw[0]), 0, 0);
#endif /* USE_XMNSOURCE */

#ifdef DETACH_RIGHT_EDGE
    if (mw = value_of(VarWrapcolumn))
	msg_win = atoi(mw);
    else
	msg_win = 0;
    if (msg_win) {
	XtVaSetValues(textw[0], XmNcolumns, msg_win, NULL);
	XtVaSetValues(textw[1], XmNcolumns, msg_win, NULL);
    }
    /* Else we're using the resource from the app-defaults file */
#endif /* DETACH_RIGHT_EDGE */

    wrap = bool_option(VarAutoformat, "compose");
    comp_curr->interface->comp_items[COMP_TEXT_W] = textw[wrap];

    comp_curr->interface->comp_items[COMP_SWAP_TEXT_W] = textw[!wrap];
#if 0
    zcb = ZmCallbackAdd("compose_state", ZCBTYPE_VAR,
			toggle_edit, textw[wrap]);
#endif /* 0 */
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, zcb);
    zcb = ZmCallbackAdd("compose_state", ZCBTYPE_VAR,
		  toggle_autoformat, textw[wrap]);
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, zcb);
    zcb = ZmCallbackAdd("recipients", ZCBTYPE_ADDRESS,
                  set_phone_tag_address , textw[wrap]);
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, zcb);
    zcb = ZmCallbackAdd("compose_state", ZCBTYPE_VAR,
		  toggle_headers, textw[wrap]);
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, zcb);
    zcb = ZmCallbackAdd("recipients", ZCBTYPE_ADDRESS,
                  set_tag_it_address , textw[wrap]);
    XtAddCallback(pane, XmNdestroyCallback, remove_callback_cb, zcb);

    /* Phone-Tag toggle area */
    comp_curr->interface->phone_tag_toggle_widgets = XtVaCreateWidget("phone_tag_toggle_rc", xmRowColumnWidgetClass,pane,
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
      i = 0;
      while (phone_tag_toggle_list[i] != NULL)
        {
          sprintf(widget_name,"phone_tag_toggle_%d",i);
          comp_curr->interface->phone_tag_toggle_widget[i] = 
            XtVaCreateManagedWidget(
            widget_name, xmToggleButtonWidgetClass, 
            comp_curr->interface->phone_tag_toggle_widgets, NULL);
          XtVaSetValues(comp_curr->interface->phone_tag_toggle_widget[i], 
          XmNmarginHeight , 0 ,
          XmNlabelString, zmXmStr(phone_tag_toggle_list[i]), NULL);
          i++;
        }
    }
    
    /* XmCreateScrolledText() manages the parent scrolled window, but not
     * the underlying text widget.  To make the correct widget visible in
     * the form, unmanage the parent of the other one and manage both of
     * the children.  toggle_autoformat() then (un)manages the parents.
     */
    XtManageChild(comp_curr->interface->phone_tag_text_widgets);
    XtManageChild(textw[!wrap]);
    XtUnmanageChild(XtParent(textw[!wrap]));
    XtManageChild(textw[wrap]);
    XtManageChild(comp_curr->interface->phone_tag_toggle_widgets);
    XtManageChild(form);
    drop_sites_swap(textw[wrap], textw[!wrap]);
    XtFree((char *) textw);

    comp_curr->interface->comp_items[COMP_ACTION_AREA] =
	CreateActionArea(pane, (ActionAreaItem *)NULL, 0, "Compose Button Panel");

    XtManageChild(pane);
    XtManageChild(comp_main_w);

    FrameCopyContext(start, comp_frame);
    FrameSet(comp_frame,
	FrameClientData, comp_curr, 
	FrameFreeClient, free_compose_data,
	FrameEndArgs);

    gui_install_all_btns(COMP_WINDOW_BUTTONS, NULL,
			 comp_curr->interface->comp_items[COMP_ACTION_AREA]);

    ZmCallbackCallAll(VarComposePanes, ZCBTYPE_VAR, ZCB_VAR_SET,
		      value_of(VarComposePanes));

    turnon(comp_curr->flags, ACTIVE);
    set_comp_items(comp_curr->interface, comp_curr);
    LoadComposition(comp_curr);
    turnoff(comp_curr->flags, MODIFIED);

    turn_tag_it_off(comp_curr);
    turn_phone_tag_off(comp_curr);
    turn_regular_on(comp_frame,comp_curr);

    if (isoff(comp_curr->flags, EDIT)) {
	FramePopup(comp_frame);
	AddressAreaUse(comp_curr->interface->prompter, comp_curr);
	draw_attach_area(comp_curr->interface->attach_area, comp_frame);
    } else
	do_edit(comp_main_w, False, NULL);

    gui_compose_hook(comp_frame);

    timeout_cursors(FALSE);
    return 0;
}

static Widget
find_hdr_focus(compose)
Compose *compose;
{
#if 0
    if (!compose->addresses[TO_ADDR] || !*compose->addresses[TO_ADDR])
	return compose->interface->comp_items[COMP_TO_TEXT];
    else if (!compose->addresses[SUBJ_ADDR] || !*compose->addresses[SUBJ_ADDR])
	    /* && (boolean_val(VarAsk) || boolean_val(VarAsksub)) ?? */
	return compose->interface->comp_items[COMP_SUBJ_TEXT];
#ifdef ASK_CC
    else if ((!compose->addresses[CC_ADDR] || !*compose->addresses[CC_ADDR]) &&
	    boolean_val(VarAskcc))
	return compose->interface->comp_items[COMP_CC_TEXT];
#endif /* ASK_CC */
    else /* No header wants the focus */
#endif /* 0 */
	return (Widget)0;
}

int
gui_set_hdrs(parent, compose)
Widget parent;
Compose *compose;
{
    char *answer =
	PromptBox(parent,
	    ison(compose->flags, FORWARD)? catgets( catalog, CAT_MOTIF, 69, "Resend To:" )
		                         : catgets( catalog, CAT_MOTIF, 70, "Send to:" ),
	    NULL, NULL, 0, NO_FLAGS, 0);

    if (answer) {
	ZSTRDUP(compose->addresses[TO_ADDR], answer);
	return 0;
    }
    return -1;
}

static void
AddressToList(list_w, idx)
Widget list_w;
int idx;
{
    ZmFrame frame = FrameGetData(list_w);
    Compose *compose = FrameComposeGetComp(frame);
    char **addrs;
    XmStringTable strs;
    int items;

#ifdef DSERV
    /* Bart: Wed Aug 11 02:36:54 PDT 1993
     * I hate having to ifdef this, but it doesn't take long enough to
     * warrant doing a timeout if we aren't doing directory lookups.
     * Probably we should move these calls lower into the directory
     * service code -- in spite of having to ifdef GUI -- just so they
     * never happen except when they're really needed.
     */
    timeout_cursors(TRUE);
#endif /* DSERV */
    addrs = get_address(compose, idx);
    strs = ArgvToXmStringTable(items = vlen(addrs), addrs);
    free_vec(addrs);
    if (items) {
	XmListAddItems(list_w, strs, items, 0);
	XmListSelectItem(list_w, strs[0], False);
    }
    XmStringFreeTable(strs);
#ifdef DSERV
    timeout_cursors(FALSE);
#endif /* DSERV */
}

int autosave_ct;

/*
 * mark the composition as modified, and do autosaving if necessary.
 */
static void
mark_as_modified(text_w, frame, cbs)
Widget text_w;
ZmFrame frame;
XmTextVerifyCallbackStruct *cbs;
{
    ZmFrame compose_frame = FrameGetData(text_w);
    Compose *compose = FrameComposeGetComp(compose_frame);

    if (!compose) return;
    if (!cbs || !cbs->text) return;
    if (cbs->startPos == cbs->endPos && cbs->text->length == 0) return;
    turnon(compose->flags, MODIFIED);
    if (text_w == compose->interface->comp_items[COMP_TEXT_W] &&
	    autosave_ct && ++compose->autosave_ct >= autosave_ct) {
	SaveComposition(compose, False);
	compose->autosave_ct = 0;
    }
}

static void
list_all_addrs(compose, list_w)
Compose *compose;
Widget list_w;
{
    XmStringTable strs;
    char **vec = DUBL_NULL, **v1, **v2, *p;
    int i, j = 0;

#ifdef DSERV
    timeout_cursors(TRUE);
#endif /* DSERV */
    for (i = TO_ADDR; i < NUM_ADDR_FIELDS; i++) {
	v1 = get_address(compose, i);
	for (v2 = v1; v2 && *v2; v2++) {
	    p = zmVaStr("%s: %s", address_headers[i], *v2);
	    ZSTRDUP(*v2, p);
	}
	j = vcat(&vec, v1);
    }
    strs = ArgvToXmStringTable(j, vec);
    free_vec(vec);
    XmListAddItems(list_w, strs, j, 0);
    XmStringFreeTable(strs);
#ifdef DSERV
    timeout_cursors(FALSE);
#endif /* DSERV */
}

/* panel selection button pressed to send a letter.
 * we've attached the sign panel item to this item to (1) avoid
 * using a global and (2) make it general enough so that multiple
 * compose windows can have multiple send_items and we can
 * identify which sign/fortune items are associated with this
 * particular letter.  The fortune item is attached to the sign item.
 */
/*ARGSUSED*/
int
do_send(item, do_close)
Widget item;
int do_close;
{
    Widget     textsw;
    Compose   *compose;
    ZmFrame    frame = FrameGetData(item);
    int        autoformat;
    int	       ret = -1;
    struct ComposeInterface *interface;
    
    ask_item = item;

    FrameGet(frame, FrameClientData, &compose, FrameEndArgs);

    interface = compose->interface;
    textsw = interface->comp_items[COMP_TEXT_W];
    autoformat = ison(compose->flags, AUTOFORMAT);

    /* Get text values from Compose Options dialog */
    update_comp_struct(compose);

    if (!SaveComposition(compose, autoformat)) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 81, "Cannot save to \"%s\"" ), compose->edfile);
	return -1;
    }

    timeout_cursors(TRUE);

    grab_addresses(compose, SEND_NOW);
    resume_compose(compose);

    if (reload_edfile() == 0) {
	/* Set receipt and autosign correctly */
	request_receipt(compose,
			ison(compose->send_flags, RETURN_RECEIPT), NULL);
	/* Toggle fortune with signature if the button and $autosign differ */
	if (boolean_val(VarAutosign) && isoff(compose->send_flags, SIGN))
	    turnoff(compose->send_flags, DO_FORTUNE);

	if (finish_up_letter() == 0) {
	    FrameSet(frame,
		FrameClientData, interface,
		FrameFreeClient, (void_proc)0,
		FrameEndArgs);

	    AddressAreaUse(interface->prompter, 0);
	    set_comp_items(interface, NULL);

	    abort_mail(item, do_close);
	    /* XXX  This message may be wrong if send_it() screwed up */
	    wprint(catgets( catalog, CAT_MOTIF, 82, "Message sent.\n" ));
	    ret = 0;
	} else if (ison(compose->send_flags, SEND_KILLED|SEND_CANCELLED)) {
	    abort_mail(item, ison(compose->send_flags, SEND_KILLED));
	    wprint(catgets( catalog, CAT_MOTIF, 83, "Message not sent.\n" ));
	} else {
	    if (compose->ed_fp)
		prepare_edfile();
	    suspend_compose(compose);

	    /* Bart: Fri May 14 18:30:02 PDT 1993
	     * Addresses can now be modified by the address book at
	     * time of send.  Stuff the new ones back into the prompts.
	     */
	    if (ison(compose->flags, EDIT_HDRS))
		LoadComposition(compose);
	}
    } else {
	FrameSet(frame,
	    FrameClientData, interface,
	    FrameFreeClient, (void_proc)0,
	    FrameEndArgs);
	abort_mail(item, do_close);
	wprint(catgets( catalog, CAT_MOTIF, 83, "Message not sent.\n" ));
    }

    timeout_cursors(FALSE);
    return ret;
}

void
abort_mail(w, close_it)
Widget w;
int close_it;
{
    ZmFrame  frame = FrameGetData(w);
    struct ComposeInterface *interface;

    /* if free_client is not null, a compose is in progress */
    if (FrameGetFreeClient(frame)) {
	Compose *compose;
	AskAnswer answer;

	FrameGet(frame, FrameClientData, &compose, FrameEndArgs);

	ask_item = w;
	if (close_it && compose->exec_pid != 0) {
	    error(UserErrWarning,
		catgets(catalog, CAT_MOTIF, 764, "Please exit from external editor first.\n"));
	    return;
	}
	if (close_it && gui_ask(WarnNo, catgets( catalog, CAT_MOTIF, 85, "Abort Message?" )) != AskYes)
	    return;
	else
	    answer = boolean_val(VarNosave)? AskNo :
		(bool_option(VarVerify, "dead")? AskYes :
		    (close_it? AskNo : AskUnknown));

	if (isoff(compose->flags, MODIFIED))
	  answer = AskNo;

	if (answer == AskYes)
	    answer =
		gui_ask(ison(compose->send_flags, SEND_KILLED)
			|| ison(FrameGetFlags(frame), FRAME_PREPARE_TO_EXIT)
			? AskOk
			: AskYes,
		    ison(compose->send_flags, SEND_KILLED)?
		    catgets(catalog, CAT_MOTIF, 86,
			"Message killed on send, save as dead letter?") :
		    catgets(catalog, CAT_MOTIF, 87,
			"Save message as dead letter?"));
	if (answer == AskCancel) {
	    /* We shouldn't have to do this here --
	     * I hate this cancel-of-a-cancel crap.
	     */
	    turnoff(compose->send_flags, SEND_KILLED|SEND_CANCELLED);
	    return;
	}
	if (answer != AskNo) {	/* Bart: Tue Aug 25 11:55:26 PDT 1992 */
	    grab_addresses(compose, SEND_KILLED);
	    /* Sync editor file with COMP_TEXT_W for dead_letter() */
	    (void) SaveComposition(compose, ison(compose->flags, AUTOFORMAT));
	}

	FrameSet(frame,
	    FrameClientData, compose->interface,
	    FrameFreeClient, (void_proc)0,
	    FrameEndArgs);

	resume_compose(compose);
	interface = compose->interface;
	AddressAreaUse(interface->prompter, 0);
#ifdef DYNAMIC_HEADERS
	FrameClose(interface->dynamics, False);
#endif /* DYNAMIC_HEADERS */
	rm_edfile(answer == AskNo? 0 : -1);
    } else {
	FrameGet(frame, FrameClientData, &interface, FrameEndArgs);
	AddressAreaUse(interface->prompter, 0);
#ifdef DYNAMIC_HEADERS
	FrameClose(interface->dynamics, False);
#endif /* DYNAMIC_HEADERS */
    }

    
    /* Bart: Tue Jun 30 18:13:14 PDT 1992
     * Plug leak of the filename attached to the text widget
     */
    CloseFile(interface->comp_items[COMP_TEXT_W]);

    /* don't destroy compose windows, as it causes problems under
     * some versions of Motif...   pf Wed Aug 25 16:43:49 1993
     */
    set_comp_items(interface, NULL);
    /* undo the effects of a edit_comp_external() */
    XtSetSensitive(GetTopShell(interface->comp_items[COMP_TEXT_W]), True);
    if (!close_it && bool_option(VarAutoiconify, "compose"))
	FrameClose(frame, True);
    else if (close_it || bool_option(VarAutodismiss, "compose")) {
	/* don't destroy compose windows, as it causes problems under
	 * some versions of Motif...   pf Wed Aug 25 16:43:49 1993
	 */
	FrameClose(frame, False);
    }
}

/* set the compose panel items */
static void
set_comp_items(interface, compose)
struct ComposeInterface *interface;
struct Compose *compose;
{
    AttachInfo *ai;

    if (!compose && interface->options)	/* For compose_state callbacks */
	FrameSet(interface->options, FrameClientData, NULL, FrameEndArgs);

    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, NULL);
    
    XtSetSensitive(interface->comp_items[COMP_TEXT_W], !!compose);
    XtSetSensitive(interface->comp_items[COMP_SWAP_TEXT_W], !!compose);

    ai = interface->attach_info;
    if (ai) {
	XtUnmanageChild(ai->shell); /* Motif-ism */
	XtPopdown(ai->shell);
    }
#ifdef SEARCH_NOT_DONE
    if (interface->comp_items[COMP_SEARCH_W] && !compose) {
	XtUnmanageChild(GetTopShell(interface->comp_items[COMP_SEARCH_W])); /* Motif-ism */
	XtPopdown(GetTopShell(interface->comp_items[COMP_SEARCH_W]));
    }
#endif /* NOT_NOW */

    if (!compose) {
	if (boolean_val("autoclear"))
	    zmXmTextSetString(interface->comp_items[COMP_TEXT_W], NULL);
	FrameClose(interface->options, False);
	FrameClose(interface->browser, False);
	/* pf Mon Aug 30 22:36:50 1993
	 * pop down the alias list; if we iconify/close the compose window, we
	 * want the alias list to be gone as well, don't we?
	 */
	if (interface->alias_list)
	    FrameClose(FrameGetData(interface->alias_list), False);
#ifdef DYNAMIC_HEADERS
	FrameDestroy(interface->dynamics, False);
	interface->dynamics = 0;
#endif /* DYNAMIC_HEADERS */
    }
}

static void
free_compose_data(compose_data)
struct Compose *compose_data;
{
    resume_compose(compose_data);
    rm_edfile(-1);
}

void
toggle_autoformat(w)
Widget w;
{
    Widget text_w, swap_w;
    u_long flags;
    Compose *compose;
    int is_set;
    Boolean was_set;
    ZmFrame frame;

#ifdef NOT_NOW
    /* If this worked, we wouldn't need user data
     * or the unmanage/manage foolishness below.
     */
    XtVaSetValues(text_w,
	XmNwordWrap,           is_set,
	XmNscrollHorizontal,   !is_set,
	NULL);
#endif /* NOT_NOW */

    frame = FrameGetData(w);
    compose = FrameComposeGetComp(frame);
    if (!compose) return;
    swap_w = compose->interface->comp_items[COMP_SWAP_TEXT_W];
    text_w = compose->interface->comp_items[COMP_TEXT_W];
    XtVaGetValues(text_w, XmNwordWrap, &was_set, NULL);
    is_set = ison(compose->flags, AUTOFORMAT);
    if (was_set == is_set) return;

    drop_sites_swap(swap_w, text_w);

    XtUnmanageChild(XtParent(text_w));
    {
	XmTextPosition position;
	XtPointer userData;
	/*
	 * If the text widgets are sharing one source,
	 * the selection should be shared too.  However,
	 * that doesn't seem to work on the first swap.
	 * So set it explicitly even when sharing.
	 */
	XmTextPosition begin, end;
	const Boolean ownSelection = XmTextGetSelectionPosition(text_w, &begin, &end);
#ifndef USE_XMNSOURCE
	String value;
#endif /* !USE_XMNSOURCE */
    
	XtVaGetValues(text_w,
		      XmNuserData, &userData,
		      XmNcursorPosition, &position,
#ifndef USE_XMNSOURCE
		      XmNvalue, &value,
#endif /* !USE_XMNSOURCE */
		      NULL);

	XtVaSetValues(swap_w,
		      XmNuserData, userData,
		      XmNcursorPosition, position,
#ifndef USE_XMNSOURCE
		      XmNvalue, value,
#endif /* !USE_XMNSOURCE */
		      NULL);

#ifndef USE_XMNSOURCE
	XtFree(value);
#endif /* !USE_XMNSOURCE */
	if (ownSelection)
	    XmTextSetSelection(swap_w, begin, end, CurrentTime);
	
	XtVaSetValues(swap_w,
		      XmNuserData, NULL,
#ifndef USE_XMNSOURCE
		      XmNvalue, NULL,
#endif /* !USE_XMNSOURCE */
		      NULL);
    }
    XtManageChild(XtParent(swap_w));

    XmProcessTraversal(swap_w, XmTRAVERSE_CURRENT);

    /* Finally, reset the comp_item */
    compose->interface->comp_items[COMP_SWAP_TEXT_W] = text_w;
    compose->interface->comp_items[COMP_TEXT_W] = swap_w;
}

void
grab_addresses(compose, state)
Compose *compose;
u_long state;
{
    if (state != INIT_HEADERS && isoff(compose->flags, EDIT_HDRS))
	AddressAreaFlush(compose->interface->prompter);
}

char *
format_get_str(w, lastc)
Widget w;
char *lastc;
{
    char *str;
    int len;

    if (!(str = XmTextGetSelection(w)) || !*str) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 90, "No text is selected." ));
	return NULL;
    }
    if (lastc) {
	len = strlen(str);
	*lastc = (len) ? str[len-1] : 0;
    }
    return str;
}

int
get_wrap_column(w)
Widget w;
{
    short wrap = 0;
    char *p;
    
    if (boolean_val("stretch_compose") && (p = value_of(VarWrapcolumn)))
	wrap = *p ? atoi(p) : 80;
    if (wrap <= 0)
	XtVaGetValues(w, XmNcolumns, &wrap, NULL);
    /* motif breaks lines slightly differently then fmt_string(),
     * so correct the line length
     */
    wrap--;
    return wrap;
}

void
format_put_str(text_w, str, lastc)
Widget text_w;
char **str;
int lastc;
{
    XmTextPosition pos, end;

    if (lastc == '\n') strapp(str, "\n");
    if (!XmTextGetSelectionPosition(text_w, &pos, &end))
	return;			/* "can't happen" */
    zmXmTextReplace(text_w, pos, end, *str);
    XmTextClearSelection(text_w, CurrentTime);
}

#ifdef IXI_DRAG_N_DROP
/* Text-window drop handling. */

static catalog_ref binary_message = catref( CAT_MOTIF, 91, "\
That's a binary file, so you can't insert it directly into the\n\
message.  Try making it an attachment by dropping it onto\n\
the Attachments drop-target instead." );

static catalog_ref length_message = catref( CAT_MOTIF, 92, "\
That file is fairly long - are you sure you want to insert it\n\
as text instead of making it an attachment?" );
#define INSERT_WARN_THRESH 5000

static void
textw_string(widget, clientdata, contents, length)
Widget widget;
XtPointer clientdata;
char *contents;
int length;
{
    ZmFrame compose_frame = FrameGetData(widget);
    Compose *compose = FrameComposeGetComp(compose_frame);
    int i;

    ask_item = widget;

    if (!compose) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 93, "This Compose window is not active." ));
	return;
    }

    if (compose->exec_pid != 0) {
	error(UserErrWarning,
	    catgets( catalog, CAT_MOTIF, 94, "Please exit from external editor before dropping." ));
	return;
    }

    timeout_cursors(TRUE);

    /* Check if it's a binary string. */
    for (i=0; i < length; ++i) {
	if (!isascii(contents[i]) ||
		(!isprint(contents[i]) && !isspace(contents[i]))) {
	    error(UserErrWarning, catgetref(binary_message));
	    timeout_cursors(FALSE);
	    return;
	}
    }
    
    /* Check if it's too long. */
    if (length > INSERT_WARN_THRESH)
	if (gui_ask(AskOk, catgetref(length_message)) != AskYes) {
	    timeout_cursors(FALSE);
	    return;
	}
	
    /* Suck it in. */
    i = TEXT_GET_LAST_POS(widget);
    TEXT_REPLACE(widget, i, i, contents);
    timeout_cursors(FALSE);
}
#endif /* IXI_DRAG_N_DROP */

Widget *
FrameComposeGetItems(frame)
ZmFrame frame;
{
    Compose *compose;
    
    if (FrameGetType(frame) == FrameCompHdrs)
	return ((Compose *) FrameGetClientData(frame))->interface->comp_items;
    if (FrameGetType(frame) != FrameCompose) return (Widget *) 0;
    if (!FrameGetFreeClient(frame))
	return ((struct ComposeInterface *)
		FrameGetClientData(frame))->comp_items;
    compose = (Compose *) FrameGetClientData(frame);
    return compose->interface->comp_items;
}

Compose *
FrameComposeGetComp(frame)
ZmFrame frame;
{
    if (FrameGetType(frame) == FrameCompHdrs ||
	    FrameGetType(frame) == FrameCompOpts)
	return (Compose *) FrameGetClientData(frame);
    if (FrameGetType(frame) != FrameCompose) return (Compose *) 0;
    if (!FrameGetFreeClient(frame)) return (Compose *) 0;
    return (Compose *) FrameGetClientData(frame);
}
