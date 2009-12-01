/*
 * $RCSfile: msgf.c,v $
 * $Revision: 2.59 $
 * $Date: 2005/05/09 09:15:25 $
 * $Author: syd $
 */

#include <spoor.h>
#include <msgf.h>
#include <facepop.h>
#include <tsearch.h>

#include <spoor/wrapview.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/cmdline.h>
#include <spoor/im.h>

#include <zmlite.h>

#include <zmail.h>
#include <zmlutil.h>

#include <dynstr.h>

#include "catalog.h"

#define Split spSplitview_Create
#define Wrap spWrapview_Create

#define LXOR(a,b) ((!(a))!=(!(b)))

#ifndef lint
static const char zmlmsgframe_rcsid[] =
    "$Id: msgf.c,v 2.59 2005/05/09 09:15:25 syd Exp $";
#endif /* lint */

struct spWclass *zmlmsgframe_class = 0;

int m_zmlmsgframe_clear;
int m_zmlmsgframe_append;
int m_zmlmsgframe_setmsg;
int m_zmlmsgframe_countAttachments;
int m_zmlmsgframe_msg;

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
    "Date:",      3,
    "Time:",      4,
    "Area Code:", 5,
    "No.:",       6,
    "Ext.:",      7,
    "Fax #:",     8,
    "ID:",        9,
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

static char *phone_tag_toggle_list[] = {
    "Phoned[%c] ",
    "Call Back[%c] ",
    "Returned Call[%c] ",
    "Wants To See You[%c] ",
    "Was In[%c] ",
    "Will Call Again[%c] ",
    "Urgent[%c]",
    NULL
  };
 
/*
  This function scans the message body looking for a line containing $EOM$
  in a phone tag message. If it finds such a line it then scans the lines
  following looking for lines containing key words that are unique to
  a phone tag message and collecting and storing the text that follow these
  key words.
*/
static void
phone_tag_parse_message_body(self,phone_tag_text_value, phone_tag_toggle_value)
struct zmlmsgframe *self;
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

  fseek(tmpf, msg[self->mymsgnum]->m_offset, 0);

/* Search for the $EOM$ trailer marker */

    found_it = False;
    while ((pos = ftell(tmpf)) < msg[self->mymsgnum]->m_offset + msg[self->mymsgnum]->m_size &&
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
        while ((pos = ftell(tmpf)) < msg[self->mymsgnum]->m_offset + msg[self->mymsgnum]->m_size &&
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

  fseek(tmpf, msg[self->mymsgnum]->m_offset, 0);
}

static void
msgsItem(self, str)
    struct spCmdline *self;
    char *str;
{
    char buf[64];

    sprintf(buf, "read %s", str);
    ZCommand(buf, zcmd_use);
}

static void
recomputemsgfocus(self)
    struct zmlmsgframe *self;
{
    spSend(self, m_dialog_clearFocusViews);

    if (dialog_messages(self)
	&& spView_window(dialog_messages(self)))
	spSend(self, m_dialog_addFocusView, dialog_messages(self));
    if (spView_window(self->body))
	spSend(self, m_dialog_addFocusView, self->body);
    if (dialog_actionArea(self)
	&& spView_window(dialog_actionArea(self)))
	spSend(dialog_actionArea(self), m_spView_wantFocus,
	       dialog_actionArea(self));
    else
	spSend(self->body, m_spView_wantFocus, self->body);

    self->paneschanged = 0;
}

static void
hdrFn(self, str)
    struct spCmdline *self;
    char *str;
{
}

static char field_template[] = "%*s ";

/*
  This function kills the tree that defines the current message display
  format and rebuilds the tree in a form suitable for displaying a
  tag it message. 
*/
static void
RebuildTagitTree(self)
    struct zmlmsgframe *self;
{
    int f = chk_option(VarMessagePanes, "folder");
    char *str[9];
    char strout[128];
    int field_length , hr;
    struct tm *T;
    time_t senttime;
    char ampm[3];
    char *toaddr , *frmaddr;
  
    struct spSplitview *spl;
    struct spWrapview *wto, *wdt , *wsb;

    strncpy(strout,self->mymsg->m_date_sent,10);
    strout[10] = '\0';
    sscanf(strout,"%ld",&senttime);
    T = localtime(&senttime);
    hr = T->tm_hour;
    if (hr < 12)
      strcpy(ampm,"AM");
    else if (hr == 12)
      strcpy(ampm,"PM");
    else
      {
        hr = hr - 12;
        strcpy(ampm,"PM");
      }
    sprintf(strout,"Date: %d/%d/%02d Time: %d:%02d%s",T->tm_mon+1,T->tm_mday,T->tm_year%100,hr,T->tm_min,ampm);

    dialog_MUNGE(self) {
        if (self->tree != NULL)
	  KillSplitviews(self->tree);
        self->tree = spSplitview_NEW();

        spSend(wdt = spTextview_NEW(), m_spView_setObserved,
           self->extra_headers.date_time);
        spSend(self->extra_headers.date_time, m_spText_clear);
        spSend(self->extra_headers.date_time, m_spText_insert, 0, -1,strout,
           spText_mAfter);
 
        wto = spWrapview_NEW();
        spSend(wto, m_spWrapview_setView, self->extra_headers.to);
        str[0] = catgets(catalog, CAT_LITE, 125, "To:");
        field_length = strlen(str[0]);
        spSend(wto, m_spWrapview_setLabel,      zmVaStr(field_template, field_length, str[0]),
           spWrapview_left);
 
        spSend(wsb = spTextview_NEW(), m_spView_setObserved,
           self->extra_headers.signed_by);
        frmaddr = header_field(self->mymsgnum,"from");
        sprintf(strout,"From: %s",frmaddr);
        spSend(self->extra_headers.signed_by, m_spText_clear);
        spSend(self->extra_headers.signed_by, m_spText_insert, 0, -1,strout,
           spText_mAfter);

        toaddr = header_field(self->mymsgnum,"to");
        spSend(spView_observed(self->extra_headers.to), m_spText_clear);
        spSend(spView_observed(self->extra_headers.to), m_spText_insert, 0, -1,toaddr,spText_mNeutral);

        spl = SplitAdd(self->tree, wdt, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        spSend(spl, m_spSplitview_setup, wto , wsb, 1, 0, 0,
           spSplitview_topBottom, spSplitview_plain, 0); 

        spSend(self, m_dialog_setView,
             Split(self->tree,
                   self->body,
                   3, 0, 0,
                   spSplitview_topBottom,
                   spSplitview_boxed,
                   spSplitview_SEPARATE));

	if (f) {
	    spSend(self, m_dialog_setopts,
		       dialog_ShowFolder | dialog_ShowMessages);
	    ZmlSetInstanceName(dialog_messages(self), "message-messages-field",
				   self);
	} else {
	    spSend(self, m_dialog_setopts, (unsigned long) 0);
	}

	spSend(self, m_dialog_setActionArea,
	       chk_option(VarMessagePanes, "action_area") ? self->aa : 0);
	if (dialog_messages(self))
	    spCmdline_fn(dialog_messages(self)) = msgsItem;
    } dialog_ENDMUNGE;
}

/*
  This function kills the tree that defines the current message display
  format and rebuilds the tree in a form suitable for displaying a
  phone tag message. 
*/
static void
RebuildPhonetagTree(self)
    struct zmlmsgframe *self;
{
    int f = chk_option(VarMessagePanes, "folder");

    char phone_tag_text_value[MAX_PHONE_TAG_TEXT_RETURNS][MAX_RETURN_LENGTH+1];
    char phone_tag_toggle_value[MAX_PHONE_TAG_TOGGLE_RETURNS][MAX_TOGGLE_RETURN_LENGTH+1];
    char *str[9];
    char strout[128];
    char toggles1[128];
    char toggles2[128];
    int field_length , body_length, hr;
    struct tm *T;
    time_t senttime;
    char ampm[3];
  
    struct spSplitview *spl, *spll, *splr, *sl, *sr, *sdt, *spt;
    struct spWrapview *wto, *wsubject, *wcc, *wbcc , *wdt , *wsb;
    struct spWrapview *wlab1, *wlab2, *wtog1, *wtog2;
    struct spWrapview *wfrom, *wof, *wdate, *wtime, *warea, *wphone, *wext, *wfax;
    strcpy(toggles1,"");
    strcpy(toggles2,"");
    phone_tag_parse_message_body(self,phone_tag_text_value,phone_tag_toggle_value);
    if (strlen(phone_tag_text_value[9]) > 9)
      {
        sscanf(phone_tag_text_value[9],"%ld",&senttime);
        T = localtime(&senttime);
        hr = T->tm_hour;
        if (hr < 12)
          strcpy(ampm,"AM");
        else if (hr == 12)
          strcpy(ampm,"PM");
        else
          {
            hr = hr - 12;
            strcpy(ampm,"PM");
          }
        sprintf(phone_tag_text_value[3],"%d/%d/%02d",T->tm_mon+1,T->tm_mday,T->tm_year%100);
        sprintf(phone_tag_text_value[4],"%d:%02d%s",hr,T->tm_min,ampm);
      }
      {
        int i;
        char ch;
        for (i=0;i<5;i++)
          {
            if ((phone_tag_toggle_value[i][0] == 'Y') ||
                (phone_tag_toggle_value[i][0] == 'y'))
              ch = 'X';
            else
              ch = ' ';
            sprintf(strout,phone_tag_toggle_list[i],ch);
            strcat(toggles1,strout);
          }
        for (i=5;i<7;i++)
          {
            if ((phone_tag_toggle_value[i][0] == 'Y') ||
                (phone_tag_toggle_value[i][0] == 'y'))
              ch = 'X';
            else
              ch = ' ';
            sprintf(strout,phone_tag_toggle_list[i],ch);
            strcat(toggles2,strout);
          }
      }
 
    dialog_MUNGE(self) {
        if (self->tree != NULL)
	  KillSplitviews(self->tree);
        self->tree = spSplitview_NEW();

        spll = spSplitview_NEW();
        splr = spSplitview_NEW();
        sr = spSplitview_NEW();
        sl = spSplitview_NEW();
        sdt = spSplitview_NEW();
        spt = spSplitview_NEW();
 
        spSend(wlab1 = spTextview_NEW(), m_spView_setObserved,
           self->extra_headers.label1);
        spSend(self->extra_headers.label1, m_spText_clear);
        spSend(self->extra_headers.label1, m_spText_insert, 0, -1," ", spText_mAfter);
        spSend(wlab2 = spTextview_NEW(), m_spView_setObserved,
           self->extra_headers.label2);
        spSend(self->extra_headers.label2, m_spText_clear);
        spSend(self->extra_headers.label2, m_spText_insert, 0, -1," ", spText_mAfter);
        spSend(wtog1 = spTextview_NEW(), m_spView_setObserved,
           self->extra_headers.toggles1);
        spSend(self->extra_headers.toggles1, m_spText_clear);
        spSend(self->extra_headers.toggles1, m_spText_insert, 0, -1,toggles1, spText_mAfter);
        spSend(wtog2 = spTextview_NEW(), m_spView_setObserved,
           self->extra_headers.toggles2);
        spSend(self->extra_headers.toggles2, m_spText_clear);
        spSend(self->extra_headers.toggles2, m_spText_insert, 0, -1,toggles2, spText_mAfter);
 
        spSend(spView_observed(self->extra_headers.to), m_spText_clear);
        spSend(spView_observed(self->extra_headers.to), m_spText_insert, 0, -1,phone_tag_text_value[0], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.from), m_spText_clear);
        spSend(spView_observed(self->extra_headers.from), m_spText_insert, 0, -1,phone_tag_text_value[1], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.of), m_spText_clear);
        spSend(spView_observed(self->extra_headers.of), m_spText_insert, 0, -1,phone_tag_text_value[2], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.msg_date), m_spText_clear);
        spSend(spView_observed(self->extra_headers.msg_date), m_spText_insert, 0, -1,phone_tag_text_value[3], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.msg_time), m_spText_clear);
        spSend(spView_observed(self->extra_headers.msg_time), m_spText_insert, 0, -1,phone_tag_text_value[4], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.area_code), m_spText_clear);
        spSend(spView_observed(self->extra_headers.area_code), m_spText_insert, 0, -1,phone_tag_text_value[5], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.phone_number), m_spText_clear);
        spSend(spView_observed(self->extra_headers.phone_number), m_spText_insert, 0, -1,phone_tag_text_value[6], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.extension), m_spText_clear);
        spSend(spView_observed(self->extra_headers.extension), m_spText_insert, 0, -1,phone_tag_text_value[7], spText_mNeutral);
        spSend(spView_observed(self->extra_headers.fax_number), m_spText_clear);
        spSend(spView_observed(self->extra_headers.fax_number), m_spText_insert, 0, -1,phone_tag_text_value[8], spText_mNeutral);
 
        wto = spWrapview_NEW();
        wfrom = spWrapview_NEW();
        wof = spWrapview_NEW();
        wdate = spWrapview_NEW();
        wtime = spWrapview_NEW();
        warea = spWrapview_NEW();
        wphone = spWrapview_NEW();
        wext = spWrapview_NEW();
        wfax = spWrapview_NEW();
 
        spSend(wto, m_spWrapview_setView, self->extra_headers.to);
        spSend(wfrom, m_spWrapview_setView, self->extra_headers.from);
        spSend(wof, m_spWrapview_setView, self->extra_headers.of);
        spSend(wdate, m_spWrapview_setView, self->extra_headers.msg_date);
        spSend(wtime, m_spWrapview_setView, self->extra_headers.msg_time);
        spSend(warea, m_spWrapview_setView, self->extra_headers.area_code);
        spSend(wphone, m_spWrapview_setView, self->extra_headers.phone_number);
        spSend(wext, m_spWrapview_setView, self->extra_headers.extension);
        spSend(wfax, m_spWrapview_setView, self->extra_headers.fax_number);
 
        str[0] = catgets(catalog, CAT_LITE, 125, "To:");
        str[1] = catgets(catalog, CAT_LITE, 136, "From:");
        str[2] = catgets(catalog, CAT_LITE, 137, "Of:");
        str[3] = catgets(catalog, CAT_LITE, 138, "Date:");
        str[4] = catgets(catalog, CAT_LITE, 139, "Time:");
        str[5] = catgets(catalog, CAT_LITE, 140, "Area Code:");
        str[6] = catgets(catalog, CAT_LITE, 141, "No:");
        str[7] = catgets(catalog, CAT_LITE, 132, "Ext:");
        str[8] = catgets(catalog, CAT_LITE, 133, "Fax:");
        field_length = 10;
 
        spSend(wto, m_spWrapview_setLabel,      zmVaStr(field_template, 6, str[0]),
           spWrapview_left);
        spSend(wfrom, m_spWrapview_setLabel, zmVaStr(field_template, 6, str[1]),spWrapview_left);
        spSend(wof, m_spWrapview_setLabel,      zmVaStr(field_template, 6, str[2]),
           spWrapview_left);
        spSend(wdate, m_spWrapview_setLabel,     zmVaStr(field_template, 6, str[3]),
           spWrapview_left);
        spSend(wtime, m_spWrapview_setLabel,     zmVaStr(field_template, 6, str[4]),
           spWrapview_left);
        spSend(warea, m_spWrapview_setLabel,     zmVaStr(field_template, field_length, str[5]),
           spWrapview_left);
        spSend(wphone, m_spWrapview_setLabel,     zmVaStr(field_template, field_length, str[6]),
           spWrapview_left);
        spSend(wext, m_spWrapview_setLabel,     zmVaStr(field_template, field_length, str[7]),
           spWrapview_left);
        spSend(wfax, m_spWrapview_setLabel,     zmVaStr(field_template, field_length, str[8]),
           spWrapview_left);
        sl = SplitAdd(spll, wto, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        sl = SplitAdd(sl, wfrom, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        sl = SplitAdd(sl, wof, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        spSend(sl, m_spSplitview_setup, wlab1, wlab2, 1, 0, 0,
           spSplitview_topBottom, spSplitview_plain, 0);
        spSend(spt, m_spSplitview_setup, wtog1, wtog2, 1, 0, 0,
           spSplitview_topBottom, spSplitview_plain, 0);
 
        spSend(sdt, m_spSplitview_setup, wdate, wtime, 18, 0, 0,
           spSplitview_leftRight, spSplitview_plain, 0);
 
        sr = SplitAdd(splr, sdt, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        sr = SplitAdd(sr, warea, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        sr = SplitAdd(sr, wphone, 1, 0, 0,
                   spSplitview_topBottom, spSplitview_plain, 0);
        spSend(sr, m_spSplitview_setup, wext, wfax, 1, 0, 0,
           spSplitview_topBottom, spSplitview_plain, 0);
 
        spSend(self->tree, m_spSplitview_setup, spll, splr, 45, 0, 0,
           spSplitview_leftRight, spSplitview_plain, 0);

	if (f) {
              body_length = 14;
	} else {
              body_length = 17;
        }
        if (chk_option(VarMessagePanes, "action_area"))
          body_length = body_length - 2;

        spSend(self, m_dialog_setView,
             Split(Split(self->tree,
                          self->body,
                          5, 0, 0,
                          spSplitview_topBottom,
                          spSplitview_boxed,
                          spSplitview_SEPARATE),
                     spt,
                     body_length, 0, 0,
                     spSplitview_topBottom,
                     spSplitview_boxed, spSplitview_SEPARATE));

	if (f) {
	    spSend(self, m_dialog_setopts,
		       dialog_ShowFolder | dialog_ShowMessages);
	    ZmlSetInstanceName(dialog_messages(self), "message-messages-field",
				   self);
	} else {
	    spSend(self, m_dialog_setopts, (unsigned long) 0);
	}

	spSend(self, m_dialog_setActionArea,
	       chk_option(VarMessagePanes, "action_area") ? self->aa : 0);
	if (dialog_messages(self))
	    spCmdline_fn(dialog_messages(self)) = msgsItem;
    } dialog_ENDMUNGE;
}

/*
  This function kills the tree that defines the current message display
  format and rebuilds the tree in a form suitable for displaying a
  regular message. 
*/
static void
RebuildNormalTree(self)
    struct zmlmsgframe *self;
{
    int f = chk_option(VarMessagePanes, "folder");
    int h = chk_option(VarMessagePanes, "headers");
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

    dialog_MUNGE(self) { 
        if (self->tree != NULL)
	  KillSplitviews(self->tree);
        self->tree = spSplitview_NEW();

	spSend(self->headers, m_spView_desiredSize,
	       &minh, &minw, &maxh, &maxw, &besth, &bestw);

	if (f) {
	    spSend(self, m_dialog_setopts,
		       dialog_ShowFolder | dialog_ShowMessages);
	    ZmlSetInstanceName(dialog_messages(self), "message-messages-field",
				   self);
	    if (h) {		/* f, h */
		spSend(self, m_dialog_setView,
		       Split(self->headers,
			     self->body,
			     besth, 0, 0, spSplitview_topBottom,
			     spSplitview_boxed,
			     spSplitview_SEPARATE));
	    } else {		/* f, !h */
		spSend(self, m_dialog_setView, self->body);
	    }
	} else {
	    spSend(self, m_dialog_setopts, (unsigned long) 0);
	    if (h) {		/* !f, h */
		spSend(self, m_dialog_setView,
		       Split(self->headers,
			     self->body,
			     besth, 0, 0, spSplitview_topBottom,
			     spSplitview_boxed,
			     spSplitview_SEPARATE));
	    } else {		/* !f, !h */
		spSend(self, m_dialog_setView, self->body);
	    }
	}

	spSend(self, m_dialog_setActionArea,
	       chk_option(VarMessagePanes, "action_area") ? self->aa : 0);
	if (dialog_messages(self))
	    spCmdline_fn(dialog_messages(self)) = msgsItem;
    } dialog_ENDMUNGE;
}

/*
  This function determines the type of message to be displayed and then
  calls the function to rebuild the message display for that message type.
*/
static void
RebuildMsgTree(self)
    struct zmlmsgframe *self;
{
    if (header_field(self->mymsgnum,"X-Chameleon-TagIt"))
      {
        self->mymsgstate = TAG_IT_STATE;
        RebuildTagitTree(self);
      }
    else if (header_field(self->mymsgnum,"X-Chameleon-PhoneTag"))
      {
        self->mymsgstate = PHONE_TAG_STATE;
        RebuildPhonetagTree(self);
      }
    else
      {
        self->mymsgstate = NORMAL_STATE;
        RebuildNormalTree(self);
      }

    if (spView_window(self)) {
	recomputemsgfocus(self);
    } else {
	self->paneschanged = 1; 
    }
}

static void
panes_cb(self, unused)
    struct zmlmsgframe *self;
    ZmCallback unused;
{
    RebuildMsgTree(self);
}

static void
msgwinhdrfmt_cb(self, cb)
    struct zmlmsgframe *self;
    ZmCallback cb;
{
    if (dialog_activeIndex(self) < 0)
	return;
    spSend(spView_observed(self->headers), m_spText_clear);
    spSend(spView_observed(self->headers), m_spText_insert,
      0, -1, format_hdr(self->mymsgnum, value_of(VarMsgWinHdrFmt), 0),
      spText_mAfter);
    RebuildMsgTree(self);
}

static void
autoformat_cb(self, unused)
    struct zmlmsgframe *self;
    ZmCallback unused;
{
    if (chk_option(VarAutoformat, "message"))
	spTextview_wrapmode(self->body) = spTextview_wordwrap;
    else
	spTextview_wrapmode(self->body) = spTextview_nowrap;	
    spSend(self->body, m_spView_wantUpdate, self->body,
	   1 << spView_fullUpdate);
}

static void
zmlmsgframe_initialize(self)
    struct zmlmsgframe *self;
{
    struct spText *t;

    self->uniquifier = 0;

    self->fldr = 0;

    self->aa = 0;
    self->paneschanged = 1;

    self->mymsgstate = ABNORMAL_STATE;

    self->tree = NULL;

    spSend(self, m_dialog_setopts,
	   dialog_ShowFolder | dialog_ShowMessages);
    ZmlSetInstanceName(dialog_messages(self), "message-messages-field", self);

    spSend(self->headers = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spTextview_wrapmode(self->headers) = spTextview_nowrap;

    spSend((self->extra_headers.to = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.to, "to-header-field", self);
    spCmdline_fn(self->extra_headers.to) = hdrFn;
    spSend((self->extra_headers.from = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.from, "from-header-field", self);
    spCmdline_fn(self->extra_headers.from) = hdrFn;
    spSend((self->extra_headers.of = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.of, "of-header-field", self);
    spCmdline_fn(self->extra_headers.of) = hdrFn;
    spSend((self->extra_headers.msg_date = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.msg_date, "date-header-field", self);
    spCmdline_fn(self->extra_headers.msg_date) = hdrFn;
    spSend((self->extra_headers.msg_time = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.msg_time, "time-header-field", self);
    spCmdline_fn(self->extra_headers.msg_time) = hdrFn;
    spSend((self->extra_headers.area_code = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.area_code, "area-header-field", self);
    spCmdline_fn(self->extra_headers.area_code) = hdrFn;
    spSend((self->extra_headers.phone_number = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.phone_number, "phone-header-field", self);
    spCmdline_fn(self->extra_headers.phone_number) = hdrFn;
    spSend((self->extra_headers.extension = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.extension, "ext-header-field", self);
    spCmdline_fn(self->extra_headers.extension) = hdrFn;
    spSend((self->extra_headers.fax_number = spCmdline_NEW()),
           m_spView_setObserved, spText_NEW());
    ZmlSetInstanceName(self->extra_headers.fax_number, "fax-header-field", self);
    spCmdline_fn(self->extra_headers.fax_number) = hdrFn;
           
    self->extra_headers.date_time = spText_NEW();
    self->extra_headers.signed_by = spText_NEW();
    self->extra_headers.label1 = spText_NEW();
    self->extra_headers.label2 = spText_NEW();
    self->extra_headers.toggles1 = spText_NEW();
    self->extra_headers.toggles2 = spText_NEW();

    self->body = spTextview_NEW();
    spSend(self->body, m_spView_setObserved, t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spTextview_showpos(self->body) = 1;

    if (chk_option(VarAutoformat, "message"))
	spTextview_wrapmode(self->body) = spTextview_wordwrap;
    else
	spTextview_wrapmode(self->body) = spTextview_nowrap;

    gui_install_all_btns(MSG_WINDOW_BUTTONS, 0, (struct dialog *) self);
    gui_install_all_btns(MSG_WINDOW_MENU, 0, (struct dialog *) self);

    self->panes_cb = ZmCallbackAdd(VarMessagePanes, ZCBTYPE_VAR,
				   panes_cb, self);
    self->msgwinhdrfmt_cb = ZmCallbackAdd(VarMsgWinHdrFmt, ZCBTYPE_VAR,
					  msgwinhdrfmt_cb, self);
    self->autoformat_cb = ZmCallbackAdd(VarAutoformat, ZCBTYPE_VAR,
					autoformat_cb, self);

    ZmlSetInstanceName(self->body, "message-body", self);
    /* RebuildMsgTree(self); */
    ZmlSetInstanceName(self, "message", self);
}

static void
Refresh(self)
    struct zmlmsgframe *self;
{
    msg_folder *fldr, *save_folder = current_folder;
    char *mgroupstr = (char *) spSend_p(self, m_dialog_mgroupstr);
    int msg_no, done_editing = 0;
    static msg_group mg;
    static int initialized = 0;

    if (RefreshReason == PROPAGATE_SELECTION)
	return;

    if (!initialized) {
	init_msg_group(&mg, 1, 0);
	initialized = 1;
    }

    fldr = (msg_folder *) spSend_p(self, m_dialog_folder);
    if (fldr->mf_count == 0
	|| ison(fldr->mf_flags, CONTEXT_RESET) && !chk_msg(mgroupstr)) {
	spSend(self, m_dialog_deactivate, dialog_Close);
	current_folder = save_folder;
	return;
    }
    if (msg_no = chk_msg(mgroupstr))
	--msg_no;
    else
	msg_no = current_msg;
    current_folder = save_folder;
    if ((msg_no < 0)
	|| (msg_no >= fldr->mf_count)
	|| (self->mymsg != fldr->mf_msgs[msg_no])) {
	int i;

	for (i = 0; i < fldr->mf_count; ++i)
	    if (self->mymsg == fldr->mf_msgs[i])
		break;
	if ((i == fldr->mf_count)
	    && (msg_no >= 0)
	    && (msg_no < fldr->mf_count)
	    && isoff(fldr->mf_msgs[msg_no]->m_flags, EDITING)
	    && ison(self->mymsgflags, EDITING)) {
	    i = msg_no;
	    done_editing = 1;
	}
	if (i == fldr->mf_count) {
	    spSend(self, m_dialog_deactivate, dialog_Close);
	    return;
	}
	msg_no = i;
	self->mymsgnum = i;
	clear_msg_group(&mg);
	add_msg_to_group(&mg, i);
	spSend(self, m_dialog_setmgroup, &mg);
    }
    if (self->mymsgflags != fldr->mf_msgs[msg_no]->m_flags) {
	int do_iconify = bool_option(VarAutoiconify, "message");

	if (ison(fldr->mf_msgs[msg_no]->m_flags, DELETE)) {
	    int was_deleted = ison(self->mymsgflags, DELETE);
	    int do_close = (!was_deleted
			    && (do_iconify
				|| !boolean_val(VarShowDeleted)
				|| bool_option(VarAutodismiss, "message")));

	    if (!was_deleted && spView_window(self)) {
		if ((fldr == current_folder)
		    && spView_window(self)
		    && boolean_val(VarAutoprint)) {
		    int num = f_next_msg(fldr, msg_no, 1);

		    if (num != msg_no) {
			ZCommand(zmVaStr("\\read %d", num + 1), zcmd_use);
			return;
		    }
		}
		if (do_close) {
		    spSend(self, m_dialog_deactivate, dialog_Close);
		    return;
		} else if (do_iconify) {
		    spSend(self, m_dialog_bury);
		    return;
		}
	    }
	}
    }
    ZmCallbackCallAll("message_state", ZCBTYPE_VAR, ZCB_VAR_SET, 0);
    if (done_editing && spView_window(self))
	ZCommand(zmVaStr("\\read %d", self->mymsgnum + 1), zcmd_use);
}

static void
zmlmsgframe_finalize(self)
    struct zmlmsgframe *self;
{
    SPOOR_PROTECT {
	ZmCallbackRemove(self->panes_cb);
	ZmCallbackRemove(self->msgwinhdrfmt_cb);
	ZmCallbackRemove(self->autoformat_cb);
	spSend(self, m_dialog_uninstallZbuttonList, MSG_WINDOW_BUTTONS, 0);
	spSend(self, m_dialog_uninstallZbuttonList, MSG_WINDOW_MENU, 0);
	KillSplitviewsAndWrapviews((struct spView *) spSend_p(self, m_dialog_setView, 0));
	spSend(self->body, m_spView_destroyObserved);
	spoor_DestroyInstance(self->body);
	spSend(self->headers, m_spView_destroyObserved);
	spoor_DestroyInstance(self->headers);
    } SPOOR_ENDPROTECT;
}

static void
zmlmsgframe_clear(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    spSend(spView_observed(self->body), m_spText_clear);
}

static void
zmlmsgframe_append(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    char *buf;
    buf = spArg(arg, char *);
    spSend(spView_observed(self->body), m_spText_insert, -1,
	   strlen(buf), buf, spText_mAfter);
}

static void
zmlmsgframe_receiveNotification(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(zmlmsgframe_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (o == (struct spObservable *) ZmlIm) {
	if (event == dialog_refresh) {
	    Refresh(self);
	}
    }
}

static void
zmlmsgframe_install(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    struct spWindow *window;

    window = spArg(arg, struct spWindow *);
    spSuper(zmlmsgframe_class, self, m_spView_install, window);
    RebuildMsgTree(self);
    if (self->paneschanged)
	recomputemsgfocus(self);
}

static int
countAttachments(self)
    struct zmlmsgframe *self;
{
    int num = 0;
    Attach *a;

    if (a = self->mymsg->m_attach)
	for (num = 0;
	     a && ((a = (Attach *) a->a_link.l_next) !=
		   self->mymsg->m_attach);
	     ++num)
	    ;
    return (num);
}

static void
zmlmsgframe_setmsg(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    msg_group mg;
    msg_folder *f;
    self->mymsgnum = spArg(arg, int);
    if (!(f = spArg(arg, msg_folder *)))
	f = current_folder;

    self->mymsg = f->mf_msgs[self->mymsgnum];
    self->mymsgflags = f->mf_msgs[self->mymsgnum]->m_flags;
    init_msg_group(&mg, 1, 0);
    add_msg_to_group(&mg, self->mymsgnum);
    spSend(self, m_zmlmsgframe_clear);
    spSend(self, m_dialog_setmgroup, &mg); /* Refreshes MessagesItem */
    spSend(self, m_dialog_setfolder, f);
    destroy_msg_group(&mg);
    RebuildMsgTree(self);
    spSend(spView_observed(self->headers), m_spText_clear);
    spSend(spView_observed(self->headers), m_spText_insert,
        0, -1, format_hdr(self->mymsgnum, value_of(VarMsgWinHdrFmt), 0),
	     spText_mAfter);
    ++(self->uniquifier);
    spSend(ZmlIm, m_spIm_showmsg, "", 15, 0, 0);
}

#ifndef NO_X_FACE

/* Bit-map for 2x4 ASCII graphics (hex):
**     1  2
**     4  8
**     10 20
**     40 80
** The idea here is first to preserve geometry, then to show density.
*/
#define SQQ '\''
#define BSQ '\\'
#define D08 'M'
#define D07 'H'
#define D06 '&'
#define D05 '$'
#define D04 '?'
static char GraphTab2x4[256] = {
/*0  1    2   3    4   5    6   7    8   9    A   B    C   D    E   F  */
' ',SQQ, '`','"', '-',SQQ, SQQ,SQQ, '-','`', '`','`', '-','^', '^','"',/*00-0F*/
'.',':', ':',':', '|','|', '/',D04, '/','>', '/','>', '~','+', '/','*',/*10-1F*/
'.',':', ':',':', BSQ,BSQ, '<','<', '|',BSQ, '|',D04, '~',BSQ, '+','*',/*20-2F*/
'-',':', ':',':', '~',D04, '<','<', '~','>', D04,'>', '=','b', 'd','#',/*30-3F*/
'.',':', ':',':', ':','!', '/',D04, ':',':', '/',D04, ':',D04, D04,'P',/*40-4F*/
',','i', '/',D04, '|','|', '|','T', '/',D04, '/','7', 'r','}', '/','P',/*50-5F*/
',',':', ';',D04, '>',D04, 'S','S', '/',')', '|','7', '>',D05, D05,D06,/*60-6F*/
'v',D04, D04,D05, '+','}', D05,'F', '/',D05, '/',D06, 'p','D', D06,D07,/*70-7F*/
'.',':', ':',':', ':',BSQ, ':',D04, ':',BSQ, '!',D04, ':',D04, D04,D05,/*80-8F*/
BSQ,BSQ, ':',D04, BSQ,'|', '(',D05, '<','%', D04,'Z', '<',D05, D05,D06,/*90-9F*/
',',BSQ, 'i',D04, BSQ,BSQ, D04,BSQ, '|','|', '|','T', D04,BSQ, '4','9',/*A0-AF*/
'v',D04, D04,D05, BSQ,BSQ, D05,D06, '+',D05, '{',D06, 'q',D06, D06,D07,/*B0-BF*/
'_',':', ':',D04, ':',D04, D04,D05, ':',D04, D04,D05, ':',D05, D05,D06,/*C0-CF*/
BSQ,D04, D04,D05, D04,'L', D05,'[', '<','Z', '/','Z', 'c','k', D06,'R',/*D0-DF*/
',',D04, D04,D05, '>',BSQ, 'S','S', D04,D05, 'J',']', '>',D06, '1','9',/*E0-EF*/
'o','b', 'd',D06, 'b','b', D06,'6', 'd',D06, 'd',D07, '#',D07, D07,D08 /*F0-FF*/
};

#define FACE_WIDTH  48 /* XXX From Xt/xface.h! Fix that file! */
#define FACE_HEIGHT 48 /* Remove these macros/constants! */
char *
set_face(msg_no)
int msg_no;
{
    char *mt_info;
    unsigned char fbuf[512], *fp, mask;
    int row, col, arow, acol, sum;
    char bits[FACE_HEIGHT][FACE_WIDTH];
    static char ascii[FACE_HEIGHT/4 * (FACE_WIDTH/2 + 1) + 1];
    char *ap;

    if (!(mt_info = header_field(msg_no, "x-face")))
	return (char *)0;

    if (uncompface(strcpy((char *) fbuf, mt_info)) < 0)
	return (char *)0;

    /* Unpack fbuf into individual bits. */
    fp = fbuf;
    mask = 0x80;
    for (row=0; row < FACE_HEIGHT; ++row) {
	for (col=0; col < FACE_WIDTH; ++col) {
	    bits[row][col] = !!(*fp & mask);
	    mask >>= 1;
	    if (mask == 0) {
		++fp;
		mask = 0x80;
	    }
	}
    }

    /* Turn bits into ascii. */
    ap = ascii;
    for (arow=0; arow < FACE_HEIGHT; arow += 4) {
	for (acol=0; acol < FACE_WIDTH; acol += 2) {
	    sum = 0;
	    mask = 1;
	    for (row=arow; row < arow + 4; ++row) {
		for (col=acol; col < acol + 2; ++col) {
		    if (bits[row][col])
			sum += mask;
		    mask <<= 1;
		}
	    }
	    *ap++ = GraphTab2x4[sum];
	}
	*ap++ = '\n';
    }
    *ap = '\0';

    /* Done. */
    return ascii;
}
#endif /* NO_X_FACE */

static void
zmlmsgframe_showface(self, requestor, data, keys)
    struct zmlmsgframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct facepopup *fp;
    char *ascii = set_face(self->mymsgnum);

    if (ascii) {
	fp = facepopup_Create(ascii);
	TRY {
	    spSend(fp, m_dialog_interactModally);
	} FINALLY {
	    spoor_DestroyInstance(fp);
	} ENDTRY;
    }
}

static void
zmlmsgframe_deactivate(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    int val = spArg(arg, int);

    spSuper(zmlmsgframe_class, self, m_dialog_deactivate, val);
}

static int
zmlmsgframe_countAttachments(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    return (countAttachments(self));
}

static struct spButtonv *
zmlmsgframe_setActionArea(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    struct spButtonv *new = spArg(arg, struct spButtonv *);
    struct spButtonv *old;

    old = spSuper_p(zmlmsgframe_class, self,
		    m_dialog_setActionArea, new);
    if (old) {
	spSend(old, m_spoor_setInstanceName, 0);
    }
    if (new) {
	ZmlSetInstanceName(new, "message-aa", self);
	self->aa = new;
    }
    if (LXOR(old, new)) {
	recomputemsgfocus(self);
    }
    return (old);
}

static void
zmlmsgframe_uninstallZbutton(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLMessageActions);

    spSuper(zmlmsgframe_class, self, m_dialog_uninstallZbutton, zb, blist,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlmsgframe_updateZbutton(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    ZmButton button = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    ZmCallbackData is_cb = spArg(arg, ZmCallbackData);
    ZmButton oldb = spArg(arg, ZmButton);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLMessageActions);

    spSuper(zmlmsgframe_class, self, m_dialog_updateZbutton,
	    button, blist, is_cb, oldb,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlmsgframe_installZbutton(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLMessageActions);

    spSuper(zmlmsgframe_class, self, m_dialog_installZbutton, zb, blist,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlmsgframe_setfolder(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    msg_folder *new = spArg(arg, msg_folder *);

    self->fldr = new;
    spSuper(zmlmsgframe_class, self, m_dialog_setfolder, new);
}

static msg_folder *
zmlmsgframe_folder(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    return (self->fldr);
}

static msg_group *
zmlmsgframe_mgroup(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    static msg_group mg;
    static int initialized = 0;

    if (!initialized) {
	init_msg_group(&mg, 1, 0);
	initialized = 1;
    }
    clear_msg_group(&mg);
    add_msg_to_group(&mg, self->mymsgnum);
    return (&mg);
}

static void
zmlmsgframe_uninstallZbuttonList(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    int slot = spArg(arg, int);
    ZmButtonList blist = spArg(arg, ZmButtonList);

    if ((slot == MSG_WINDOW_BUTTONS)
	&& !dialog_actionArea(self)
	&& self->aa)
	spSend(self, m_dialog_setActionArea, self->aa);
    spSuper(zmlmsgframe_class, self, m_dialog_uninstallZbuttonList,
	    slot, blist);
    self->aa = 0;
}

static void
zmlmsgframe_nextpageormsg(self, requestor, data, ks)
    struct zmlmsgframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *ks;
{
    static int lastinteraction = -2;
    int h, w;
    int p, l;

    if (!spView_window(self->body))
	return;

    if (spInteractionNumber == (lastinteraction + 1)) {
	ZCommand("\\next", zcmd_use);
	spSend(ZmlIm, m_spIm_showmsg, "", 15, 0, 0);
	return;
    }

    /* First figure out the height of the body window */
    spSend(spView_window(self->body), m_spWindow_size, &h, &w);
    /* Now get the first visible position of the text */
    p = spText_markPos((struct spText *) spView_observed(self->body),
		       spTextview_firstVisibleLineMark(self->body));
    /* Now, can we go forward h lines without hitting the end? */
    l = h;
    while ((l-- > 0) && (p >= 0))
	p = spSend_i(self->body, m_spTextview_nextSoftBol, p, w);
    if (p >= 0) {
	/* We should page the text */
	spSend(self->body, m_spView_invokeInteraction,
	       "text-next-page", requestor, data, ks);
    } else {
	/* We should set up for another invocation to do "next" */
	struct dynstr d;
	int i;

	lastinteraction = spInteractionNumber;
	dynstr_Init(&d);
	TRY {
	    dynstr_Set(&d, "");
	    if (ks && (spKeysequence_Length(ks) > 0)) {
		for (i = 0; i < spKeysequence_Length(ks); ++i) {
		    if (i > 0)
			dynstr_AppendChar(&d, ' ');
		    dynstr_Append(&d, spKeyname(spKeysequence_Nth(ks, i),
						1));
		}
	    }
	    spSend(ZmlIm, m_spIm_showmsg, zmVaStr(
               catgets(catalog, CAT_LITE, 298, "Press `%s' again for next message"),
               dynstr_Str(&d)), 15, 0, 0);
	} FINALLY {
	    dynstr_Destroy(&d);
	} ENDTRY;
    }
}

static Msg *
zmlmsgframe_msg(self, arg)
    struct zmlmsgframe *self;
    spArgList_t arg;
{
    return (self->mymsg);
}

struct spWidgetInfo *spwc_Message = 0;

void
zmlmsgframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlmsgframe_class)
	return;
    zmlmsgframe_class =
	spWclass_Create("zmlmsgframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlmsgframe)),
			zmlmsgframe_initialize,
			zmlmsgframe_finalize,
			spwc_Message = spWidget_Create("Message",
						       spwc_MenuScreen));

    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_uninstallZbuttonList, NULL,
		      zmlmsgframe_uninstallZbuttonList);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_mgroup, NULL,
		      zmlmsgframe_mgroup);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_setfolder, NULL,
		      zmlmsgframe_setfolder);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_folder, NULL,
		      zmlmsgframe_folder);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_installZbutton, NULL,
		      zmlmsgframe_installZbutton);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_updateZbutton, NULL,
		      zmlmsgframe_updateZbutton);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_uninstallZbutton, NULL,
		      zmlmsgframe_uninstallZbutton);
    spoor_AddOverride(zmlmsgframe_class,
		      m_dialog_setActionArea, NULL,
		      zmlmsgframe_setActionArea);
    spoor_AddOverride(zmlmsgframe_class, m_spObservable_receiveNotification,
		      NULL, zmlmsgframe_receiveNotification);
    spoor_AddOverride(zmlmsgframe_class, m_spView_install, NULL,
		      zmlmsgframe_install);
    spoor_AddOverride(zmlmsgframe_class, m_dialog_deactivate, NULL,
		      zmlmsgframe_deactivate);

    m_zmlmsgframe_msg =
	spoor_AddMethod(zmlmsgframe_class, "msg",
			0, zmlmsgframe_msg);
    m_zmlmsgframe_clear =
	spoor_AddMethod(zmlmsgframe_class, "clear",
			NULL,
			zmlmsgframe_clear);
    m_zmlmsgframe_append =
	spoor_AddMethod(zmlmsgframe_class, "append",
			NULL,
			zmlmsgframe_append);
    m_zmlmsgframe_setmsg =
	spoor_AddMethod(zmlmsgframe_class, "setmsg",
			NULL,
			zmlmsgframe_setmsg);
    m_zmlmsgframe_countAttachments =
	spoor_AddMethod(zmlmsgframe_class, "countAttachments",
			NULL, zmlmsgframe_countAttachments);

    spWidget_AddInteraction(spwc_Message, "message-next-page-or-message",
			    zmlmsgframe_nextpageormsg,
			    catgets(catalog, CAT_LITE, 299, "Next page or next message"));
    spWidget_AddInteraction(spwc_Message, "message-showface",
			    zmlmsgframe_showface,
			    catgets(catalog, CAT_LITE, 300, "Show X-Face"));

    spWidget_bindKey(spwc_Message, spKeysequence_Parse(0, " ", 0),
		     "message-next-page-or-message", 0, 0, 0, 0);

    spWidget_bindKey(spwc_Message, spKeysequence_Parse(0, "^Xf", 1),
		     "message-showface", 0, 0, 0, 0);

    spWrapview_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spCmdline_InitializeClass();
    spIm_InitializeClass();
    facepopup_InitializeClass();
    tsearch_InitializeClass();
}
