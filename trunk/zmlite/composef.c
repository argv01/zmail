/*
 * $RCSfile: composef.c,v $
 * $Revision: 2.96 $
 * $Date: 2005/05/09 09:15:25 $
 * $Author: syd $
 */

#include <spoor.h>
#include <composef.h>

#include <stdio.h>
#include <dynstr.h>

#include <zmlite.h>
#include <zmail.h>
#include <cmdtab.h>
#include <hooks.h>

#include <mainf.h>
#include <zmlutil.h>
#include <smalias.h>
#include <tsearch.h>
#include <dynhdrs.h>
#include <regexpr.h>
#include <spoor/wrapview.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/menu.h>
#include <spoor/cmdline.h>
#include <spoor/window.h>
#include <spoor/im.h>
#include <spoor/toggle.h>

#include "catalog.h"

#ifndef lint
static const char zmlcomposeframe_rcsid[] =
    "$Id: composef.c,v 2.96 2005/05/09 09:15:25 syd Exp $";
#endif /* lint */

struct spWclass *zmlcomposeframe_class = 0;

int m_zmlcomposeframe_readEdFile;
int m_zmlcomposeframe_writeEdFile;

static Compose *new_comp;
int autosave_ct;

extern void EnterScreencmd(), ExitScreencmd();

#define LXOR(a,b) ((!(a))!=(!(b)))

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define Split spSplitview_Create
#define Wrap spWrapview_Create

enum {
    DONE_B, NEW_B, SEND_B, CNCL_B, HELP_B
};

struct phone_tag_output_table {
    char *the_format;
    int  the_source_offset;
  };

struct phone_tag_output_table phone_tag_header[] = {
    " \n" , -1,
    "$EOM$\n", -1,
    "\n" , -1,
    "From: %s\n",  1,
    "Of: %s\n", 2,
    "Area Code: %s\n", 5,
    "No.: %s\n", 6,
    "Ext.: %s\n", 7,
    "Fax #: %s\n", 8,
    "Date: %s (M/d/yy)\n", 3,
    "Time: %s\n", 4,
    NULL, -1
  };

char phone_tag_strings[11][65];
 
struct phone_tag_output_table phone_tag_checkbox[] = {
    "Phoned: %s\n", 0,
    "Call back: %s\n", 1,
    "Returned Call: %s\n", 2,
    "Wants to see you: %s\n", 3,
    "Was in: %s\n", 4,
    "Will call again: %s\n", 5,
    "Urgent: %s\n", 6,
    NULL, -1
  };

Boolean phone_tag_toggles[7];
 
struct phone_tag_output_table phone_tag_trailer[] = {
    "Special attention: No\n", -1,
    "Signed: \"%s\" <%s>\n", -2,
    "To: %s\n", 0,
    "ID: %s 0\n", -3,
    "Memo type: 0\n", -1,
    NULL,-1
  };

/*
  This function returns the users return address.
*/
void get_user_return_address(signed_by)
char *signed_by;
{
int j , k;
char *host;
char *p;
 
  p = value_of(VarFromAddress);
  if (p == NULL)
    {
      strcpy(signed_by,value_of(VarUser));
      strcat(signed_by,"@");
      host = value_of(VarHostname);
      j = 0;
      k = strlen(signed_by);
      while (!((host[j] == '\0') || (host[j] == ' ')))
        {
          signed_by[k] = host[j];
          k++;
          signed_by[k] = '\0';
          j++;
        }
    }
  else
    strcpy(signed_by,p);
}

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
char tempstr[128];
char the_message_id[16];
char the_date[16];
char the_time[16];
char the_input_date[16];
char the_input_time[16];
char Zone[4];
char *tempmsg_name;
time_t seconds , curr_t;
struct tm T;
struct tm *curr_T;
FILE *tempmsg;
struct dynstr v;

  if (ison(compose->send_flags,READ_RECEIPT))
    {
      get_user_return_address(signed_by);
      sprintf(tempstr,"<%s>",signed_by);
      input_header("X-Chameleon-Return-To",tempstr,False);
    }

  if (ison(compose->send_flags,TAG_IT))
    {
/* Set the subject into the header */
      input_address(SUBJ_ADDR,"Tag-It!");
/* Set the special tag itheader into the header */
      input_header("X-Chameleon-TagIt","Chameleon Tag It Ver 4.10",False);
      return;
    }

  if (ison(compose->send_flags,PHONE_TAG))
    {
      dynstr_Init(&v);
/* Get the phone tag text strings */
      spSend(spView_observed(compose->compframe->headers.to), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[0],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.from), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[1],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.of), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[2],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.msg_date), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[3],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.msg_time), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[4],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.area_code), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[5],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.phone_number), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[6],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.extension), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[7],dynstr_Str(&v));
      dynstr_Set(&v, "");
      spSend(spView_observed(compose->compframe->headers.fax_number), m_spText_appendToDynstr, &v, 0, -1);
      strcpy(phone_tag_strings[8],dynstr_Str(&v));
      dynstr_Destroy(&v);
/* Get the message toggle box states */
    if (spToggle_state(compose->compframe->toggles.phoned))
      phone_tag_toggles[0] = True;
    else
      phone_tag_toggles[0] = False;
    if (spToggle_state(compose->compframe->toggles.call_back))
      phone_tag_toggles[1] = True;
    else
      phone_tag_toggles[1] = False;
    if (spToggle_state(compose->compframe->toggles.returned_call))
      phone_tag_toggles[2] = True;
    else
      phone_tag_toggles[2] = False;
    if (spToggle_state(compose->compframe->toggles.see_you))
      phone_tag_toggles[3] = True;
    else
      phone_tag_toggles[3] = False;
    if (spToggle_state(compose->compframe->toggles.was_in))
      phone_tag_toggles[4] = True;
    else
      phone_tag_toggles[4] = False;
    if (spToggle_state(compose->compframe->toggles.will_call))
      phone_tag_toggles[5] = True;
    else
      phone_tag_toggles[5] = False;
    if (spToggle_state(compose->compframe->toggles.urgent))
      phone_tag_toggles[6] = True;
    else
      phone_tag_toggles[6] = False;

/* Get the current date and time */
      curr_t = time((time_t *)0);
      curr_T = time_n_zone(Zone);
/* Set the message id date and time with the current date and time */
      phone_tag_date(the_message_id,the_date,the_time);
/* Get the input date and time from the dialog */
      i = 0;
      while (phone_tag_header[i].the_format != NULL)
        {
          if (phone_tag_header[i].the_source_offset == 3)
            {
              strncpy(the_input_date,phone_tag_strings[phone_tag_header[i].the_source_offset],15);
              the_input_date[15] = '\0';
            }
          else if (phone_tag_header[i].the_source_offset == 4)
            {
              strncpy(the_input_time,phone_tag_strings[phone_tag_header[i].the_source_offset],15);
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
          if (hr == 12)
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
/* Set the subject into the header */
      sprintf(signed_by,"Phone Tag - %s",phone_tag_strings[1]);
      input_address(SUBJ_ADDR,signed_by);
/* Set the special phone tag header into the header */
      input_header("X-Chameleon-PhoneTag","Chameleon Phone Tag Version 6.0",False);
/* Write out the Phone-Tag lines at the end of the message */
      i = 0;
      while (phone_tag_header[i].the_format != NULL)
        {
          if (phone_tag_header[i].the_source_offset == -1)
            fprintf(compose->ed_fp,phone_tag_header[i].the_format);
          else if (phone_tag_header[i].the_source_offset == 3)
            fprintf(compose->ed_fp,phone_tag_header[i].the_format,the_date);
          else if (phone_tag_header[i].the_source_offset == 4)
            fprintf(compose->ed_fp,phone_tag_header[i].the_format,the_time);
          else
            {
              str = phone_tag_strings[phone_tag_header[i].the_source_offset];
              fprintf(compose->ed_fp,phone_tag_header[i].the_format,str);
            }
          i++;
        }
      i = 0;
      while (phone_tag_checkbox[i].the_format != NULL)
        {
          if (phone_tag_toggles[phone_tag_checkbox[i].the_source_offset])
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
              get_user_return_address(signed_by);
              fprintf(compose->ed_fp,phone_tag_trailer[i].the_format,value_of(VarRealname),signed_by);
            }
          else if (phone_tag_trailer[i].the_source_offset == -1)
            fprintf(compose->ed_fp,phone_tag_trailer[i].the_format);
          else
            {
              str = phone_tag_strings[phone_tag_trailer[i].the_source_offset];
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
/* Close and delete the temporary file */
      fclose(tempmsg);
      unlink(tempmsg_name); 
/* Close the message file and repopen it as update */
      fclose(compose->ed_fp);
      compose->ed_fp = fopen(compose->edfile,"r+");
      return;
    }
}

static void
msgsItem(self, str)
    struct spCmdline *self;
    char *str;
{
    char *argv[2];
    msg_group mgroup;

    init_msg_group(&mgroup, 1, 0);
    argv[0] = str;
    argv[1] = NULL;
    if (get_msg_list(argv, &mgroup))
	spSend(Dialog(&MainDialog), m_dialog_setmgroup, &mgroup);
    destroy_msg_group(&mgroup);
}

static void
fill_in_prompt(self, selector)
    struct zmlcomposeframe *self;
    int selector;
{
    struct spCmdline *field;

    switch (selector) {
    case TO_ADDR:
	field = self->headers.to;
	break;
    case SUBJ_ADDR:
        if (self->headers.subject)
	  field = self->headers.subject;
	break;
    case CC_ADDR:
        if (self->headers.cc)
	  field = self->headers.cc;
	break;
    case BCC_ADDR:
        if (self->headers.bcc)
	  field = self->headers.bcc;
	break;
    default:
	field = 0;
    }

    if (field) {
	char **vec = get_address(zmlcomposeframe_comp(self), selector);
	char *addr = joinv(NULL, vec, ", ");
	free_vec(vec);
	spSend(spView_observed(field), m_spText_clear);
	if (addr)
	    spSend(spView_observed(field), m_spText_insert,
		   0, strlen(addr), addr, spText_mAfter);
	xfree(addr);
    }
}

static void
header_change_cb(self, data)
    struct zmlcomposeframe *self;
    struct zmCallbackData *data;
{
    struct Compose * const composition = (struct Compose *) data->xdata;
    
    if (self->comp == composition && isoff(self->comp->flags, EDIT_HDRS))
	fill_in_prompt(self, data->event);
}

static void
header_change_listen(self)
    struct zmlcomposeframe *self;
{
    self->headers.recipients_cb = ZmCallbackAdd("recipients", ZCBTYPE_ADDRESS,
						header_change_cb, self);
    self->headers.subject_cb    = ZmCallbackAdd("subject",    ZCBTYPE_ADDRESS,
						header_change_cb, self);
}

static void
header_change_unlisten(self)
    struct zmlcomposeframe *self;
{
    if (self->headers.recipients_cb) {
	ZmCallbackRemove(self->headers.recipients_cb);
	self->headers.recipients_cb = 0;
    }
    if (self->headers.subject_cb) {
	ZmCallbackRemove(self->headers.subject_cb);
	self->headers.subject_cb = 0;
    }
}

static void
grab_addresses(self, force)
    struct zmlcomposeframe *self;
    int force;
{
    struct dynstr d;
    char **vec;

    if (!force
	&& ((!(self->comp))
	    || ison(self->comp->flags, EDIT_HDRS)))
	return;
    if (!(self->headers.to))
	return;

    dynstr_Init(&d);
    TRY {
	header_change_unlisten(self);

	spSend(spView_observed(self->headers.to), m_spText_appendToDynstr,
	       &d, 0, -1);
	set_address(zmlcomposeframe_comp(self), TO_ADDR,
		    vec = unitv(dynstr_Str(&d)));
	free_vec(vec);
	dynstr_Set(&d, "");
    
        if (self->headers.cc)
          {
            spSend(spView_observed(self->headers.cc), m_spText_appendToDynstr,
	       &d, 0, -1);
	    set_address(zmlcomposeframe_comp(self), CC_ADDR,
		    vec = unitv(dynstr_Str(&d)));
	    free_vec(vec);
	    dynstr_Set(&d, "");
          }
    
        if (self->headers.bcc)
          {
	    spSend(spView_observed(self->headers.bcc), m_spText_appendToDynstr,
	       &d, 0, -1);
	    set_address(zmlcomposeframe_comp(self), BCC_ADDR,
		    vec = unitv(dynstr_Str(&d)));
	    free_vec(vec);
	    dynstr_Set(&d, "");
          }
    
        if (self->headers.subject)
          {
	    spSend(spView_observed(self->headers.subject), m_spText_appendToDynstr,
	       &d, 0, -1);
	    set_address(zmlcomposeframe_comp(self), SUBJ_ADDR,
		    vec = unitv(dynstr_Str(&d)));
	    free_vec(vec);
          }
    } FINALLY {
	dynstr_Destroy(&d);
	header_change_listen(self);
    } ENDTRY;
}

static int
do_send(self)
    struct zmlcomposeframe *self;
{
    int retval = 0;

#ifdef ZMCOT
    if (!self) {
	spSend(ZmcotDialog->composeBody, m_spTextview_writeFile,
	       comp_current->edfile);
	resume_compose(comp_current);
	reload_edfile();
	return (finish_up_letter());
    }
#endif /* ZMCOT */

    LITE_BUSY {
	spSend(self, m_zmlcomposeframe_writeEdFile, NULL,
	       ison(self->comp->flags, AUTOFORMAT));
	grab_addresses(self, 0);
	resume_compose(self->comp);
	reload_edfile();
	if (!finish_up_letter()) {
	    self->comp = 0;
	    spSend(self, m_dialog_deactivate, dialog_Close);
	    retval = 1;
	}
	gui_refresh(current_folder, REDRAW_SUMMARIES);
    } LITE_ENDBUSY;
    if (!retval)
	error(UserErrWarning, catgets(catalog, CAT_LITE, 114, "Message not sent"));
    return (retval);
}

static void
do_abort(self, close_it)
    struct zmlcomposeframe *self;
    int close_it;
{
    AskAnswer answer;

    if (close_it && (self->comp->exec_pid != 0)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 115, "Please exit from external editor first"));
	return;
    }
    if (close_it && (ask(WarnNo, catgets(catalog, CAT_LITE, 116, "Abort Message?")) != AskYes))
	return;
    else
	answer = (boolean_val(VarNosave) ?
		  AskNo :
		  (bool_option(VarVerify, "dead") ?
		   AskYes :
		   (close_it ?
		    AskNo :
		    AskUnknown)));
    if ((spSend_i(spView_observed(self->body), m_spText_length) == 0)
	&& (ison(self->comp->flags, EDIT_HDRS)
	    || ((spSend_i(spView_observed(self->headers.to),
			  m_spText_length) == 0)
		&& (spSend_i(spView_observed(self->headers.subject),
			     m_spText_length) == 0)
		&& (spSend_i(spView_observed(self->headers.cc),
			     m_spText_length) == 0)
		&& (spSend_i(spView_observed(self->headers.bcc),
			     m_spText_length) == 0))))
	answer = AskNo;
    if (answer == AskYes)
	answer = ask((ison(self->comp->send_flags, SEND_KILLED) ?
		      AskOk :
		      AskYes),
		     (ison(self->comp->send_flags, SEND_KILLED) ?
		      catgets(catalog, CAT_LITE, 117, "Message killed on send, save as dead letter?") :
		      catgets(catalog, CAT_LITE, 118, "Save message as dead letter?")));
    if (answer == AskCancel) {
	turnoff(self->comp->send_flags, SEND_KILLED|SEND_CANCELLED);
	return;
    }
    if (answer != AskNo) {
	grab_addresses(self, 0);
	spSend(self, m_zmlcomposeframe_writeEdFile, self->comp->edfile,
	       ison(self->comp->flags, AUTOFORMAT));
    }
    resume_compose(self->comp);
    rm_edfile((answer == AskNo) ? 0 : -1);
    self->comp = 0;
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
do_insert_file(self, fname)
    struct zmlcomposeframe *self;
    char *fname;
{
    FILE *fp = NULL;
    char buf[1 + MAXPATHLEN];
    int bytes, pos;
    struct dynstr d;

    dynstr_Init(&d);
    TRY {
	if ((fname && *fname) ?
	    (dynstr_Set(&d, fname), 1) :
	    !dyn_choose_one(&d, catgets(catalog, CAT_LITE, 119, "File to read:"),
			    0, 0, 0, PB_FILE_BOX | PB_MUST_MATCH)) {
	    struct stat stbuf;

	    pos = spText_markPos((struct spText *)
				 spView_observed(self->body),
				 spTextview_textPosMark(self->body));
	    estat(dynstr_Str(&d), &stbuf, "do_insert_file");
	    if (S_ISDIR(stbuf.st_mode)) {
  	        error(UserErrWarning,
		      catgets(catalog, CAT_LITE, 875, "%s is a directory"),
		      dynstr_Str(&d));
	    }
	    else if (fp = fopen(dynstr_Str(&d), "r")) {
  	        LITE_BUSY {
		    while ((bytes = fread(buf, (sizeof (char)),
					  (sizeof (buf)), fp)) > 0) {
			spSend(spView_observed(self->body), m_spText_insert,
			       pos, bytes, buf, spText_mNeutral);
			pos += bytes;
		    }
		} LITE_ENDBUSY;
		fclose(fp);
		fp = NULL;
	    } else {
		error(SysErrWarning,
		      catgets(catalog, CAT_LITE, 120, "Cannot open %s"),
		      dynstr_Str(&d));
	    }
	}
    } FINALLY {
        if (fp)
	    fclose(fp);
        dynstr_Destroy(&d);
    } ENDTRY;
}

static int
do_save(self, dflt)
    struct zmlcomposeframe *self;
    char *dflt;
{
    char savefile[1 + MAXPATHLEN], *file;
    int x = 1, e;

    if (choose_one(savefile, catgets(catalog, CAT_LITE, 121, "File to write:"), dflt, (char **) 0,
		   0, PB_FILE_BOX | PB_NOT_A_DIR))
	return (-1);
    if (!*savefile)
	return (-1);
    file = varpath(savefile, &x);
    if (x) {
	if (x == 1)
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 122, "%s: is a directory"), savefile);
	else if (x == -1)
	    error(UserErrWarning, "%s: %s", savefile, file);
	return (-1);
    }
    if ((Access(file, F_OK) == 0)
	&& (ask(WarnNo, catgets(catalog, CAT_LITE, 123, "Overwrite %s?"), file) != AskYes))
	return (-1);
    TRY {
	LITE_BUSY {
	    spSend(self, m_zmlcomposeframe_writeEdFile, file, 0);
	} LITE_ENDBUSY;
	e = 0;
    } EXCEPT(ANY) {
	e = -1;
    } ENDTRY;
    return (e);
}

static void
do_spell(self)
    struct zmlcomposeframe *self;
{
    struct tsearch *ts = tsearch_NEW();

    TRY {
	spSend(ts, m_tsearch_setText, spView_observed(self->body));
	spSend(ts, m_tsearch_setTextPos,
	       spText_markPos((struct spText *) spView_observed(self->body),
			      spTextview_textPosMark(self->body)));
	TRY {
	    spSend(ts, m_dialog_interactModally);
	} FINALLY {
	    spSend(spView_observed(self->body), m_spText_setMark,
		   spTextview_textPosMark(self->body),
		   spSend_i(ts, m_tsearch_textPos));
	    spSend(self->body, m_spView_wantUpdate, self->body,
		   1 << spTextview_cursorMotion);
	} ENDTRY;
    } FINALLY {
	spoor_DestroyInstance(ts);
    } ENDTRY;
}

static void
editor(self, editorstr)
    struct zmlcomposeframe *self;
    char *editorstr;
{
    spSend(self, m_zmlcomposeframe_writeEdFile, NULL, 0);
    resume_compose(self->comp);
    turnoff(self->comp->flags, ACTIVE);
    EnterScreencmd(1);
    spIm_LOCKSCREEN {
	invoke_editor(editorstr);
    } spIm_ENDLOCKSCREEN;
    ExitScreencmd(0);
    turnon(self->comp->flags, ACTIVE);
    suspend_compose(self->comp);
    spSend(self, m_zmlcomposeframe_readEdFile, 1);
    if (dialog_actionArea(self)
	&& spView_window(dialog_actionArea(self)))
	spSend(dialog_actionArea(self), m_spView_wantFocus,
	       dialog_actionArea(self));
}

static void
do_edit(self)
    struct zmlcomposeframe *self;
{
    editor(self, 0);
}

static void
selectWidget(obj)
    struct spView *obj;
{
    if (obj && spView_window(obj))
	spSend(obj, m_spView_wantFocus, obj);
    else
	spSend(ZmlIm, m_spIm_showmsg,
	       catgets(catalog, CAT_LITE, 124, "No field to receive focus"), 15, 0, 5);
}

static void
hdrFn(self, str)
    struct spCmdline *self;
    char *str;
{
    struct zmlcomposeframe *cf = (struct zmlcomposeframe *) spIm_view(ZmlIm);

    if ((self == cf->headers.to)
	|| (self == cf->headers.cc)
	|| (self == cf->headers.bcc)) {
	if (str && *str) {
	    char *p = str;

	    if (ison(zmlcomposeframe_comp(cf)->flags, DIRECTORY_CHECK)) {
		p = (char *) address_book(str, aliases_should_expand(), 0);
	    }
	    if (p && *p && ison(zmlcomposeframe_comp(cf)->flags,
				SORT_ADDRESSES)) {
		p = (char *) address_sort(p);
	    }
	    spSend(spView_observed(self), m_spText_clear);
	    if (p && *p)
		spSend(spView_observed(self), m_spText_insert, 0,
		       strlen(p), p, spText_mNeutral);
	}
    }

    spSend(self, m_spView_invokeInteraction, "text-beginning-of-line",
	   self, NULL, NULL);
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next",
	   self, NULL, NULL);
}

static void
recomputecomposefocus(self)
    struct zmlcomposeframe *self;
{
    spSend(self, m_dialog_clearFocusViews);
    if (dialog_messages(self)
	&& spView_window(dialog_messages(self)))
	spSend(self, m_dialog_addFocusView, dialog_messages(self));
    if (self->headers.tree) {
	if (spView_window(self->headers.to))
	    spSend(self, m_dialog_addFocusView, self->headers.to);
	if (spView_window(self->headers.subject))
	    spSend(self, m_dialog_addFocusView, self->headers.subject);
	if (spView_window(self->headers.cc))
	    spSend(self, m_dialog_addFocusView, self->headers.cc);
	if (spView_window(self->headers.bcc))
	    spSend(self, m_dialog_addFocusView, self->headers.bcc);
    if (self->the_state == PHONE_TAG_STATE)
      {
	if (spView_window(self->headers.from))
	    spSend(self, m_dialog_addFocusView, self->headers.from);
	if (spView_window(self->headers.of))
	    spSend(self, m_dialog_addFocusView, self->headers.of);
	if (spView_window(self->headers.msg_date))
	    spSend(self, m_dialog_addFocusView, self->headers.msg_date);
	if (spView_window(self->headers.msg_time))
	    spSend(self, m_dialog_addFocusView, self->headers.msg_time);
	if (spView_window(self->headers.area_code))
	    spSend(self, m_dialog_addFocusView, self->headers.area_code);
	if (spView_window(self->headers.phone_number))
	    spSend(self, m_dialog_addFocusView, self->headers.phone_number);
	if (spView_window(self->headers.extension))
	    spSend(self, m_dialog_addFocusView, self->headers.extension);
	if (spView_window(self->headers.fax_number))
	    spSend(self, m_dialog_addFocusView, self->headers.fax_number);
      }
    }
    if (spView_window(self->body))
	spSend(self, m_dialog_addFocusView, self->body);
    if (self->the_state == PHONE_TAG_STATE)
        spSend(self, m_dialog_addFocusView, self->phone_tag_options);

    if (self->comp && isoff(zmlcomposeframe_comp(self)->flags, EDIT_HDRS)) {
	if (spSend_i(spView_observed(self->headers.to), m_spText_length)) {
	    if (spSend_i(spView_observed(self->headers.subject),
		       m_spText_length)) {
		spSend(self->body, m_spView_wantFocus, self->body);
	    } else {
		spSend(self->headers.subject, m_spView_wantFocus,
		       self->headers.subject);
	    }
	} else {
	    spSend(self->headers.to, m_spView_wantFocus, self->headers.to);
	}
    } else {
	spSend(self->body, m_spView_wantFocus, self->body);
    }
    self->paneschanged = 0;
}

static void
opt_toggle_cb(t, self)
    struct spToggle *t;
    struct zmlcomposeframe *self;
{
}

static char field_template[] = "%*s ";
static void
MakeHeaderTree(self)
    struct zmlcomposeframe *self;
{
    char *str[9];
    int field_length;
    char the_date[16];
    char the_time[16];
    char the_id[16];
    char signed_by[128];
    char strout[128];

    struct spSplitview *spl, *spll, *splr, *sl, *sr, *sdt;
    struct spWrapview *wto, *wsubject, *wcc, *wbcc , *wdt , *wsb;
    struct spWrapview *wlab1, *wlab2;
    struct spWrapview *wfrom, *wof, *wdate, *wtime, *warea, *wphone, *wext, *wfax;

    if (self->the_state == TAG_IT_STATE)
      {
        get_user_return_address(signed_by); 
        phone_tag_date(the_id,the_date,the_time);

        self->headers.tree = spSplitview_NEW();

        spSend(wdt = spTextview_NEW(), m_spView_setObserved,
           self->headers.date_time = spText_NEW());
        sprintf(strout,"Date: %s Time: %s",the_date,the_time);
        spSend(self->headers.date_time, m_spText_insert, 0, -1,strout,
           spText_mAfter);

        spSend((self->headers.to = spCmdline_NEW()),
    	   m_spView_setObserved, spText_NEW());
        ZmlSetInstanceName(self->headers.to, "to-header-field", self);
        spCmdline_fn(self->headers.to) = hdrFn;
        wto = spWrapview_NEW();
        spSend(wto, m_spWrapview_setView, self->headers.to);
        str[0] = catgets(catalog, CAT_LITE, 125, "To:");
        field_length = strlen(str[0]);
        spSend(wto, m_spWrapview_setLabel,      zmVaStr(field_template, field_length, str[0]),
	   spWrapview_left);

        spSend(wsb = spTextview_NEW(), m_spView_setObserved,
           self->headers.signed_by = spText_NEW());
        sprintf(strout,"Signed: \"%s\" <%s>",value_of(VarRealname),signed_by);
        spSend(self->headers.signed_by, m_spText_insert, 0, -1,strout,
           spText_mAfter);

        spl = SplitAdd(self->headers.tree, wdt, 1, 0, 0,
		   spSplitview_topBottom, spSplitview_plain, 0); 
        spSend(spl, m_spSplitview_setup, wto , wsb, 1, 0, 0,
	   spSplitview_topBottom, spSplitview_plain, 0); 

        header_change_listen(self);
      }
    else if (self->the_state == PHONE_TAG_STATE)
      {
        get_user_return_address(signed_by); 
        phone_tag_date(the_id,the_date,the_time);

        self->headers.tree = spSplitview_NEW();
        spll = spSplitview_NEW();
        splr = spSplitview_NEW();
        sr = spSplitview_NEW();
        sl = spSplitview_NEW();
        sdt = spSplitview_NEW();

        spSend((self->headers.to = spCmdline_NEW()),
    	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.from = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.of = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.msg_date = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.msg_time = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.area_code = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.phone_number = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.extension = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.fax_number = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend(wlab1 = spTextview_NEW(), m_spView_setObserved, 
           self->headers.label1 = spText_NEW());
        spSend(self->headers.label1, m_spText_insert, 0, -1," ", spText_mAfter);
        spSend(wlab2 = spTextview_NEW(), m_spView_setObserved, 
           self->headers.label2 = spText_NEW());
        spSend(self->headers.label2, m_spText_insert, 0, -1," ", spText_mAfter);
        
        spSend(spView_observed(self->headers.msg_date), m_spText_insert, 0, -1,the_date, spText_mNeutral);
        spSend(spView_observed(self->headers.msg_time), m_spText_insert, 0, -1,the_time, spText_mNeutral);

        ZmlSetInstanceName(self->headers.to, "to-header-field", self);
        ZmlSetInstanceName(self->headers.from, "from-header-field", self);
        ZmlSetInstanceName(self->headers.of, "of-header-field", self);
        ZmlSetInstanceName(self->headers.msg_date, "date-header-field", self);
        ZmlSetInstanceName(self->headers.msg_time, "time-header-field", self);
        ZmlSetInstanceName(self->headers.area_code, "area-header-field", self);
        ZmlSetInstanceName(self->headers.phone_number, "phone-header-field", self);
        ZmlSetInstanceName(self->headers.extension, "ext-header-field", self);
        ZmlSetInstanceName(self->headers.fax_number, "fax-header-field", self);

        spCmdline_fn(self->headers.to) = hdrFn;
        spCmdline_fn(self->headers.from) = hdrFn;
        spCmdline_fn(self->headers.of) = hdrFn;
        spCmdline_fn(self->headers.msg_date) = hdrFn;
        spCmdline_fn(self->headers.msg_time) = hdrFn;
        spCmdline_fn(self->headers.area_code) = hdrFn;
        spCmdline_fn(self->headers.phone_number) = hdrFn;
        spCmdline_fn(self->headers.extension) = hdrFn;
        spCmdline_fn(self->headers.fax_number) = hdrFn;

        wto = spWrapview_NEW();
        wfrom = spWrapview_NEW();
        wof = spWrapview_NEW();
        wdate = spWrapview_NEW();
        wtime = spWrapview_NEW();
        warea = spWrapview_NEW();
        wphone = spWrapview_NEW();
        wext = spWrapview_NEW();
        wfax = spWrapview_NEW();

        spSend(wto, m_spWrapview_setView, self->headers.to);
        spSend(wfrom, m_spWrapview_setView, self->headers.from);
        spSend(wof, m_spWrapview_setView, self->headers.of);
        spSend(wdate, m_spWrapview_setView, self->headers.msg_date);
        spSend(wtime, m_spWrapview_setView, self->headers.msg_time);
        spSend(warea, m_spWrapview_setView, self->headers.area_code);
        spSend(wphone, m_spWrapview_setView, self->headers.phone_number);
        spSend(wext, m_spWrapview_setView, self->headers.extension);
        spSend(wfax, m_spWrapview_setView, self->headers.fax_number);

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
        spSend(wfrom, m_spWrapview_setLabel, zmVaStr(field_template, 6, str[1]),
	   spWrapview_left);
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

    self->phone_tag_options =
        spButtonv_Create(spButtonv_multirow,
                         (self->toggles.phoned =
                          spToggle_Create(catgets(catalog, CAT_LITE, 98, "Phoned"),
                                          opt_toggle_cb, self, 0)),
                         (self->toggles.call_back =
                          spToggle_Create(catgets(catalog, CAT_LITE, 99, "Call Back"),
                                          opt_toggle_cb, self, 0)),
                         (self->toggles.returned_call =
                          spToggle_Create(catgets(catalog, CAT_LITE, 100, "Returned Call"),
                                          opt_toggle_cb, self, 0)),
                         (self->toggles.see_you =
                          spToggle_Create(catgets(catalog, CAT_LITE, 101, "Wants To See You"),
                                          opt_toggle_cb, self, 0)),
                         (self->toggles.was_in =
                          spToggle_Create(catgets(catalog, CAT_LITE, 102, "Was In"),
                                          opt_toggle_cb, self, 0)),
                         (self->toggles.will_call =
                          spToggle_Create(catgets(catalog, CAT_LITE, 103, "Will Call"),
                                          opt_toggle_cb, self, 0)),
                         (self->toggles.urgent =
                          spToggle_Create(catgets(catalog, CAT_LITE, 104, "Urgent"),
                                          opt_toggle_cb, self, 0)),
                         0);
    spButtonv_toggleStyle(self->phone_tag_options) = spButtonv_checkbox;
    spButtonv_anticipatedWidth(self->phone_tag_options) = 60;
    ZmlSetInstanceName(self->phone_tag_options, "phone_tag-options-tg", self);
    spSend(self->phone_tag_options, m_spView_setWclass, spwc_Togglegroup);

        sl = SplitAdd(spll, wto, 1, 0, 0,
		   spSplitview_topBottom, spSplitview_plain, 0);
        sl = SplitAdd(sl, wfrom, 1, 0, 0,
		   spSplitview_topBottom, spSplitview_plain, 0);
        sl = SplitAdd(sl, wof, 1, 0, 0,
		   spSplitview_topBottom, spSplitview_plain, 0);
        spSend(sl, m_spSplitview_setup, wlab1, wlab2, 1, 0, 0,
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

        spSend(self->headers.tree, m_spSplitview_setup, spll, splr, 45, 0, 0,
	   spSplitview_leftRight, spSplitview_plain, 0);

        header_change_listen(self);
      }
    else
      {
        self->headers.tree = spSplitview_NEW();
        spSend((self->headers.to = spCmdline_NEW()),
    	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.subject = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.cc = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());
        spSend((self->headers.bcc = spCmdline_NEW()),
	   m_spView_setObserved, spText_NEW());

        ZmlSetInstanceName(self->headers.to, "to-header-field", self);
        ZmlSetInstanceName(self->headers.subject, "subject-header-field",
		       self);
        ZmlSetInstanceName(self->headers.cc, "cc-header-field", self);
        ZmlSetInstanceName(self->headers.bcc, "bcc-header-field", self);

        spCmdline_fn(self->headers.to) = hdrFn;
        spCmdline_fn(self->headers.subject) = hdrFn;
        spCmdline_fn(self->headers.cc) = hdrFn;
        spCmdline_fn(self->headers.bcc) = hdrFn;

        wto = spWrapview_NEW();
        wsubject = spWrapview_NEW();
        wcc = spWrapview_NEW();
        wbcc = spWrapview_NEW();

        spSend(wto, m_spWrapview_setView, self->headers.to);
        spSend(wsubject, m_spWrapview_setView, self->headers.subject);
        spSend(wcc, m_spWrapview_setView, self->headers.cc);
        spSend(wbcc, m_spWrapview_setView, self->headers.bcc);

        str[0] = catgets(catalog, CAT_LITE, 125, "To:");
        str[1] = catgets(catalog, CAT_LITE, 126, "Subject:");
        str[2] = catgets(catalog, CAT_LITE, 127, "Cc:");
        str[3] = catgets(catalog, CAT_LITE, 128, "Bcc:");
        field_length = max (strlen(str[0]), strlen(str[1]));
        field_length = max ( max ( strlen(str[2]), field_length), strlen(str[3]));

        spSend(wto, m_spWrapview_setLabel,      zmVaStr(field_template, field_length, str[0]),
	   spWrapview_left);
        spSend(wsubject, m_spWrapview_setLabel, zmVaStr(field_template, field_length, str[1]),
	   spWrapview_left);
        spSend(wcc, m_spWrapview_setLabel,      zmVaStr(field_template, field_length, str[2]),
	   spWrapview_left);
        spSend(wbcc, m_spWrapview_setLabel,     zmVaStr(field_template, field_length, str[3]),
	   spWrapview_left);

        spl = SplitAdd(self->headers.tree, wto, 1, 0, 0,
		   spSplitview_topBottom, spSplitview_plain, 0);
        spl = SplitAdd(spl, wsubject, 1, 0, 0,
		   spSplitview_topBottom, spSplitview_plain, 0);
        spSend(spl, m_spSplitview_setup, wcc, wbcc, 1, 0, 0,
	   spSplitview_topBottom, spSplitview_plain, 0);

        header_change_listen(self);
      }
}

static void
FreePhoneTag(self)
    struct zmlcomposeframe *self;
{
  if (self->headers.from) {
      spSend(self->headers.from, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.from);
  }
  if (self->headers.of) {
      spSend(self->headers.of, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.of);
  }
  if (self->headers.msg_date) {
      spSend(self->headers.msg_date, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.msg_date);
  }
  if (self->headers.msg_time) {
      spSend(self->headers.msg_time, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.msg_time);
  }
  if (self->headers.area_code) {
      spSend(self->headers.area_code, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.area_code);
  }
  if (self->headers.phone_number) {
      spSend(self->headers.phone_number, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.phone_number);
  }
  if (self->headers.extension) {
      spSend(self->headers.extension, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.extension);
  }
  if (self->headers.fax_number) {
      spSend(self->headers.fax_number, m_spView_destroyObserved);
      spoor_DestroyInstance(self->headers.fax_number);
  }
  self->headers.from =         (struct spCmdline *) 0;
  self->headers.of =           (struct spCmdline *) 0;
  self->headers.msg_date =     (struct spCmdline *) 0;
  self->headers.msg_time =     (struct spCmdline *) 0;
  self->headers.area_code =    (struct spCmdline *) 0;
  self->headers.phone_number = (struct spCmdline *) 0;
  self->headers.extension =    (struct spCmdline *) 0;
  self->headers.fax_number =   (struct spCmdline *) 0;
}

static void
RebuildComposeTree(self)
    struct zmlcomposeframe *self;
{
    int f = chk_option(VarComposePanes, "folder");
    Boolean pt = (ison(self->comp->send_flags,PHONE_TAG) || ison(self->comp->send_flags,TAG_IT));
    struct spView *v = dialog_view(self);

    dialog_MUNGE(self) {
	if (v) {
	    spSend(self, m_dialog_setView, 0);
            if (self->the_state != ABNORMAL_STATE)
	      KillAllButOneSplitview(v, self->headers.tree);
            else
              {
                if (ison(self->comp->send_flags,TAG_IT))
                  self->the_state = TAG_IT_STATE;
                else if (ison(self->comp->send_flags,PHONE_TAG))
                  self->the_state = PHONE_TAG_STATE;
                else
                  self->the_state = NORMAL_STATE;
	        KillSplitviewsAndWrapviews(v);
	        self->headers.tree = (struct spSplitview *) 0;
                FreePhoneTag(self);
              }
	}
	if (!zmlcomposeframe_comp(self)
	    || ison(zmlcomposeframe_comp(self)->flags, EDIT_HDRS)) {
	    if (f && !pt) {		/* eh, f */
		spSend(self, m_dialog_setopts,
		       dialog_ShowFolder | dialog_ShowMessages);
	    } else {		/* eh, !f */
		spSend(self, m_dialog_setopts, (unsigned long) 0);
	    }
	    spSend(self, m_dialog_setView, self->body);
	} else {
	    if (!(self->headers.tree))
		MakeHeaderTree(self);
	    if (f && !pt) {		/* !eh, f */
		spSend(self, m_dialog_setopts,
		       dialog_ShowFolder | dialog_ShowMessages);
	    } else {		/* !eh, !f */
		spSend(self, m_dialog_setopts, (unsigned long) 0);
	    }
            if (self->the_state == TAG_IT_STATE)
	      spSend(self, m_dialog_setView,
		   Split(self->headers.tree,
			 self->body,
			 3, 0, 0,
			 spSplitview_topBottom,
			 spSplitview_boxed,
			 spSplitview_SEPARATE));
            else if (self->the_state == PHONE_TAG_STATE)
	      spSend(self, m_dialog_setView,
		   Split(Split(self->headers.tree,
			        self->body,
			        5, 0, 0,
			        spSplitview_topBottom,
			        spSplitview_boxed,
			        spSplitview_SEPARATE),
                           self->phone_tag_options,
                           15, 0, 0,
                           spSplitview_topBottom,
                           spSplitview_boxed, spSplitview_SEPARATE));
            else
	      spSend(self, m_dialog_setView,
		   Split(self->headers.tree,
			 self->body,
			 4, 0, 0,
			 spSplitview_topBottom,
			 spSplitview_boxed,
			 spSplitview_SEPARATE));
	}
        if (pt)
 	  spSend(self, m_dialog_setActionArea, self->aa);
        else 
 	  spSend(self, m_dialog_setActionArea,
	       chk_option(VarComposePanes, "action_area") ? self->aa : 0);
    } dialog_ENDMUNGE;
    if (spView_window(self)) {
	recomputecomposefocus(self);
    } else {
	self->paneschanged = 1;
    }
}

static void
panes_cb(self, unused)
    struct zmlcomposeframe *self;
    ZmCallback unused;
{
    RebuildComposeTree(self);
}

static void
fill_in_prompts(self)
    struct zmlcomposeframe *self;
{
    fill_in_prompt(self,   TO_ADDR);
    fill_in_prompt(self, SUBJ_ADDR);
    fill_in_prompt(self,   CC_ADDR);
    fill_in_prompt(self,  BCC_ADDR);
}

static void
EditHdrs(self)
    struct zmlcomposeframe *self;
{
    int edit_hdrs = ison(self->comp->flags, EDIT_HDRS);

    if (!LXOR(edit_hdrs, self->edit_hdrs))
	return;

    self->edit_hdrs = edit_hdrs;

    spSend(self, m_zmlcomposeframe_writeEdFile, NULL, 0);

    /* That I need to do this is totally brain-damaged. */
    if (edit_hdrs) {
	turnoff(self->comp->flags, EDIT_HDRS);
    } else {
	turnon(self->comp->flags, EDIT_HDRS);
    }

    resume_compose(self->comp);
    if (reload_edfile() != 0) {
	spSend(self, m_dialog_deactivate, dialog_Close);
	wprint(catgets(catalog, CAT_LITE, 129, "Message not sent.\n"));
	return;
    }
    if (edit_hdrs) {
	grab_addresses(self, 1);
    } else {
	if (!(self->headers.tree)) {
	    MakeHeaderTree(self);
	}
	fill_in_prompts(self);
    }

    /* That I need to do this is totally brain-damaged. */
    if (edit_hdrs) {
	turnon(self->comp->flags, EDIT_HDRS);
    } else {
	turnoff(self->comp->flags, EDIT_HDRS);
    }

    (void) prepare_edfile();
    suspend_compose(self->comp);

    RebuildComposeTree(self);

    spSend(self, m_zmlcomposeframe_readEdFile, 0);
}

static void
state_callback(self, cb)
    struct zmlcomposeframe *self;
    ZmCallback cb;
{
    if (!(self->comp))
	return;

    /* Autoformat */
    spTextview_wrapmode(self->body) = (ison(self->comp->flags, AUTOFORMAT) ?
				       spTextview_wordwrap :
				       spTextview_nowrap);
    spSend(self->body, m_spView_wantUpdate, self->body,
	   1 << spView_fullUpdate);

    /* Edit headers */
    if (self->comp) {
	EditHdrs(self);
    }

    /* Tag-It */
    if (ison(self->comp->send_flags, TAG_IT))
      {
        if (self->the_state == NORMAL_STATE)
          {
            self->the_state = ABNORMAL_STATE;
            RebuildComposeTree(self);
            return;
          }
      }

    /* not Tag-Tt */
    if (isoff(self->comp->send_flags, TAG_IT))
      {
        if (self->the_state == TAG_IT_STATE)
          {
            self->the_state = ABNORMAL_STATE;
            RebuildComposeTree(self);
            return;
          }
      }

    /* Phone-Tag */
    if (ison(self->comp->send_flags, PHONE_TAG))
      {
        if (self->the_state == NORMAL_STATE)
          {
            self->the_state = ABNORMAL_STATE;
            RebuildComposeTree(self);
            return;
          }
      }

    /* not Phone-Tag */
    if (isoff(self->comp->send_flags, PHONE_TAG))
      {
        if (self->the_state == PHONE_TAG_STATE)
          {
            self->the_state = ABNORMAL_STATE;
            RebuildComposeTree(self);
            return;
          }
      }
}

static void
zmlcomposeframe_initialize(self)
    struct zmlcomposeframe *self;
{
    char *wrapcval = get_var_value(VarWrapcolumn);

    ComposeDialog = self;

    dialog_MUNGE(self) {
	TRY {
	    self->dh = 0;
	    self->aa = 0;
	    self->comp = new_comp;
	    self->comp->compframe = self;

	    self->paneschanged = 1;
	    self->new = 1;
            self->the_state = NORMAL_STATE;

	    spSend(self, m_dialog_setopts,
		   dialog_ShowFolder | dialog_ShowMessages);
	    spCmdline_fn(dialog_messages(self)) = msgsItem;
	    ZmlSetInstanceName(dialog_messages(self),
			       "compose-messages-field", self);

	    self->body = spTextview_NEW();
	    ZmlSetInstanceName(self->body, "compose-body", self);
	    spSend(self->body, m_spView_setObserved, spText_NEW());
	    spTextview_showpos(self->body) = 1;
	    if (wrapcval && *wrapcval)
		spTextview_wrapcolumn(self->body) = atoi(wrapcval);

	    spTextview_wrapmode(self->body) = (ison(self->comp->flags,
						    AUTOFORMAT) ?
					       spTextview_wordwrap :
					       spTextview_nowrap);

	    self->headers.tree =    (struct spSplitview *) 0;
	    self->headers.to =	    (struct spCmdline *) 0;
	    self->headers.subject = (struct spCmdline *) 0;
	    self->headers.cc =	    (struct spCmdline *) 0;
	    self->headers.bcc =	    (struct spCmdline *) 0;
            self->headers.from =         (struct spCmdline *) 0;
            self->headers.of =           (struct spCmdline *) 0;
            self->headers.msg_date =     (struct spCmdline *) 0;
            self->headers.msg_time =     (struct spCmdline *) 0;
            self->headers.area_code =    (struct spCmdline *) 0;
            self->headers.phone_number = (struct spCmdline *) 0;
            self->headers.extension =    (struct spCmdline *) 0;
            self->headers.fax_number =   (struct spCmdline *) 0;

	    self->headers.recipients_cb = 0;
	    self->headers.subject_cb    = 0;
		
	    self->state_cb = ZmCallbackAdd("compose_state", ZCBTYPE_VAR,
					   state_callback, self);

	    if (isoff(self->comp->flags, EDIT_HDRS)) {
		self->edit_hdrs = 0;
		if (!(self->headers.tree))
		    MakeHeaderTree(self);
		fill_in_prompts(self);
	    } else {
		self->edit_hdrs = 1;
	    }
	    spSend(self, m_zmlcomposeframe_readEdFile, 0);

	    gui_install_all_btns(COMP_WINDOW_BUTTONS, 0,
				 (struct dialog *) self);
	    gui_install_all_btns(COMP_WINDOW_MENU, 0,
				 (struct dialog *) self);

	    self->panes_cb = ZmCallbackAdd(VarComposePanes, ZCBTYPE_VAR,
					   panes_cb, self);

	    RebuildComposeTree(self);
	    ZmlSetInstanceName(self, "compose", self);

	    if (ison(self->comp->flags, EDIT_HDRS)) {
		regexp_t rxpbuf = (regexp_t) emalloc(sizeof (*rxpbuf),
						     "text search");
		char fastmap[256];
		int pos, end, found = -1;

		TRY {
		    rxpbuf->buffer = NULL;
		    rxpbuf->allocated = 0;
		    rxpbuf->translate = NULL;
		    rxpbuf->fastmap = fastmap;
		    if (!re_compile_pattern("^To: $", 6, rxpbuf)) {
			re_compile_fastmap(rxpbuf);
			if ((pos = spSend_i(spView_observed(self->body),
					    m_spText_rxpSearch,
					    rxpbuf, 0, &end)) >= 0) {
			    found = end;
			} else {
			    /*
			     * Re-use these fields from last compile
			     * to avoid extra allocations:
			    rxpbuf->buffer = NULL;
			    rxpbuf->allocated = 0;
			     */
			    rxpbuf->translate = NULL;
			    rxpbuf->fastmap = fastmap;
			    if (!re_compile_pattern("^Subject: $", 11,
						    rxpbuf)) {
				re_compile_fastmap(rxpbuf);
				if ((pos =
				     spSend_i(spView_observed(self->body),
					      m_spText_rxpSearch, rxpbuf,
					      0, &end)) >= 0) {
				    found = end;
				}
			    }
			}
		    }
		    if (found >= 0) {
			spSend(spView_observed(self->body), m_spText_setMark,
			       spTextview_textPosMark(self->body), found);
			spSend(self->body, m_spView_wantUpdate, self->body,
			       1 << spTextview_cursorMotion);
		    }
		} FINALLY {
		    if (rxpbuf->buffer)
			free(rxpbuf->buffer);
		    free(rxpbuf);
		} ENDTRY;
	    }
	} FINALLY {
	    ComposeDialog = 0;
	} ENDTRY;
    } dialog_ENDMUNGE;
}

static void
zmlcomposeframe_readEdFile(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    int endp;

    endp = spArg(arg, int);

    turnon(self->comp->flags, MODIFIED);
    spSend(spView_observed(self->body), m_spText_readFile,
	   self->comp->edfile);

    if (endp)
	spSend(spView_observed(self->body), m_spText_setMark,
	       spTextview_textPosMark(self->body),
	       spSend_i(spView_observed(self->body), m_spText_length));
}

static void
zmlcomposeframe_writeEdFile(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    char *filename = spArg(arg, char *);
    int wrap = spArg(arg, int);

    if (wrap) {
	if ((!(self->headers.to))
	    || !spView_window(self->headers.to)) {
	    int pos = 0, Max = spSend_i(spView_observed(self->body),
					m_spText_length);
	    int newline = 1;
	    char ch;
	    FILE *fp;

	    while (pos < Max) {
		ch = spTextview_getc(self->body, pos++);
		if (newline) {
		    if (ch == '\n')
			break;
		    else
			newline = 0;
		} else {
		    if (ch == '\n')
			newline = 1;
		    else
			newline = 0;
		}
	    }
	    if (fp = fopen(filename ? filename : self->comp->edfile, "w")) {
		TRY {
		    spSend(spView_observed(self->body), m_spText_writePartial,
			   fp, 0, pos);
		    spSend(self->body, m_spTextview_writePartial,
			   fp, pos, Max - pos, 1);
		} FINALLY {
		    fclose(fp);
		} ENDTRY;
	    } else {
		error(SysErrWarning, catgets(catalog, CAT_LITE, 120, "Cannot open %s"),
		      filename ? filename : self->comp->edfile);
	    }
	} else {
	    spSend(self->body, m_spTextview_writeFile,
		   filename ? filename : self->comp->edfile);
	}
    } else {
	spSend(spView_observed(self->body), m_spText_writeFile,
	       filename ? filename : self->comp->edfile);
    }
}

static void
zmlcomposeframe_install(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    struct spWindow *window;

    window = spArg(arg, struct spWindow *);
    spSuper(zmlcomposeframe_class, self, m_spView_install, window);
#if 0				/* is this really necessary? */
    RebuildComposeTree(self);
#endif
    if (self->paneschanged)
	recomputecomposefocus(self);
}

static void
zmlcomposeframe_enter(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    int bodyfocus = 0;

    resume_compose(self->comp);
    suspend_compose(self->comp); /* make this composition the default for
				  * compcmd */
    if (self->new) {
	if (ison(self->comp->flags, EDIT))
	    editor(self, NULL);
	if (ison(self->comp->flags, EDIT_HDRS))
	    bodyfocus = 1;
	self->new = 0;
    }
    spSuper(zmlcomposeframe_class, self, m_dialog_enter);
    if (bodyfocus)
	spSend(self->body, m_spView_wantFocus, self->body);
}

static void
gotoTo(self, requestor, data, keys)
    struct zmlcomposeframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    selectWidget(self->headers.to);
}

static void
gotoSubject(self, requestor, data, keys)
    struct zmlcomposeframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (self->headers.subject)
      selectWidget(self->headers.subject);
}

static void
gotoCc(self, requestor, data, keys)
    struct zmlcomposeframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (self->headers.cc)
      selectWidget(self->headers.cc);
}

static void
gotoBcc(self, requestor, data, keys)
    struct zmlcomposeframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (self->headers.bcc)
      selectWidget(self->headers.bcc);
}

static void
gotoBody(self, requestor, data, keys)
    struct zmlcomposeframe *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    selectWidget(self->body);
}

static void
zmlcomposeframe_receiveNotification(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(zmlcomposeframe_class, self, m_spObservable_receiveNotification,
	    o, event, data);
    if (o == (struct spObservable *) ZmlIm) {
	switch (event) {
	  case dialog_refresh:
	    if (current_folder)
		spSend(self, m_dialog_setfolder, current_folder);
	    spSend(self, m_dialog_setmgroup, 
		   (msg_group *) spSend_p(MainDialog, m_dialog_mgroup));
	    break;
	  case zmlmainframe_mainselection:
	    spSend(self, m_dialog_setmgroup,
		   (msg_group *) spSend_p(MainDialog, m_dialog_mgroup));
	    break;
	}
    }
}

static void
zmlcomposeframe_finalize(self)
    struct zmlcomposeframe *self;
{
    struct spView *v = dialog_view(self);
    SPOOR_PROTECT {
	if (self->comp) {
	    resume_compose(self->comp);
	    rm_edfile(0);
	}
	self->comp = 0;

	spSend(self, m_dialog_uninstallZbuttonList, COMP_WINDOW_BUTTONS, 0);
	spSend(self, m_dialog_uninstallZbuttonList, COMP_WINDOW_MENU, 0);

	ZmCallbackRemove(self->state_cb);
	ZmCallbackRemove(self->panes_cb);

	spSend(self, m_dialog_setView, 0);
	KillSplitviewsAndWrapviews(v);
	if (self->dh)
	    spoor_DestroyInstance(self->dh);
	spSend(self->body, m_spView_destroyObserved);
	spoor_DestroyInstance(self->body);

	if (self->headers.to) {
	    spSend(self->headers.to, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->headers.to);
	}
	if (self->headers.subject) {
	    spSend(self->headers.subject, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->headers.subject);
	}
	if (self->headers.cc) {
	    spSend(self->headers.cc, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->headers.cc);
	}
	if (self->headers.bcc) {
	    spSend(self->headers.bcc, m_spView_destroyObserved);
	    spoor_DestroyInstance(self->headers.bcc);
	}
        FreePhoneTag(self); 
	header_change_unlisten(self);
    } SPOOR_ENDPROTECT;
}

static void
zmlcomposeframe_updateZbutton(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    ZmButton button = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    ZmCallbackData is_cb = spArg(arg, ZmCallbackData);
    ZmButton oldb = spArg(arg, ZmButton);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    Compose *c = comp_current;
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLComposeActions);

    if (!(self->comp))
	return;

    comp_current = zmlcomposeframe_comp(self);
    TRY {
	spSuper(zmlcomposeframe_class, self, m_dialog_updateZbutton,
		button, blist, is_cb, oldb,
		aa ? aa :
		((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
    } FINALLY {
	comp_current = c;
    } ENDTRY;
}

static struct spButtonv *
zmlcomposeframe_setActionArea(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    struct spButtonv *new = spArg(arg, struct spButtonv *);
    struct spButtonv *old = spSuper_p(zmlcomposeframe_class, self,
				      m_dialog_setActionArea, new);

    if (old) {
	spSend(old, m_spoor_setInstanceName, 0);
    }
    if (new) {
	ZmlSetInstanceName(new, "compose-aa", self);
	self->aa = new;
    }
    if (LXOR(old, new)) {
	recomputecomposefocus(self);
    }
    return (old);
}

static void
zmlcomposeframe_uninstallZbutton(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLComposeActions);

    spSuper(zmlcomposeframe_class, self, m_dialog_uninstallZbutton, zb, blist,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlcomposeframe_installZbutton(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    ZmButton zb = spArg(arg, ZmButton);
    ZmButtonList blist = spArg(arg, ZmButtonList);
    struct spButtonv *aa = spArg(arg, struct spButtonv *);
    int hiddenaa = (!dialog_actionArea(self) && self->aa);
    int isaa = !strcmp(blist->name, BLComposeActions);

    spSuper(zmlcomposeframe_class, self, m_dialog_installZbutton, zb, blist,
	    aa ? aa :
	    ((isaa && hiddenaa) ? self->aa : dialog_actionArea(self)));
}

static void
zmlcomposeframe_uninstallZbuttonList(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    int slot = spArg(arg, int);
    ZmButtonList blist = spArg(arg, ZmButtonList);

    if ((slot == COMP_WINDOW_BUTTONS)
	&& !dialog_actionArea(self)
	&& self->aa)
	spSend(self, m_dialog_setActionArea, self->aa);
    spSuper(zmlcomposeframe_class, self, m_dialog_uninstallZbuttonList,
	    slot, blist);
    self->aa = 0;
}

static void
zmlcomposeframe_deactivate(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    int retval = spArg(arg, int);

    spSuper(zmlcomposeframe_class, self, m_dialog_deactivate, retval);
    spoor_DestroyInstance(self);
}

static void
zmlcomposeframe_activate(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    spSuper(zmlcomposeframe_class, self, m_dialog_activate);
    if (lookup_function(COMPOSE_HOOK)) {
	ZCommand(COMPOSE_HOOK, zcmd_ignore);
    }
}

static char *autosave_str = 0;
static int autosave;

static void
autosave_cb(unused1, unused2)
    GENERIC_POINTER_TYPE *unused1;
    ZmCallback unused2;
{
    if (autosave_str = get_var_value(VarAutosave))
	autosave = atoi(autosave_str);
}

static void
zmlcomposeframe_wantUpdate(self, arg)
    struct zmlcomposeframe *self;
    spArgList_t arg;
{
    struct spView *requestor = spArg(arg, struct spView *);
    unsigned long flags = spArg(arg, unsigned long);

    spSuper(zmlcomposeframe_class, self, m_spView_wantUpdate,
	    requestor, flags);
    if ((requestor == (struct spView *) self->body)
	&& autosave_str
	&& (autosave > 0)
	&& (!(spTextview_changecount(self->body) % autosave))) {
	turnon(self->comp->flags, MODIFIED);
	spSend(self, m_zmlcomposeframe_writeEdFile, 0, 0);
    }
}

struct spWidgetInfo *spwc_Compose = 0;

void
zmlcomposeframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlcomposeframe_class)
	return;
    zmlcomposeframe_class =
	spWclass_Create("zmlcomposeframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlcomposeframe)),
			zmlcomposeframe_initialize,
			zmlcomposeframe_finalize,
			spwc_Compose = spWidget_Create("Compose",
						       spwc_MenuScreen));

    spoor_AddOverride(zmlcomposeframe_class,
		      m_spView_wantUpdate, NULL,
		      zmlcomposeframe_wantUpdate);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_activate, NULL,
		      zmlcomposeframe_activate);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_deactivate, NULL,
		      zmlcomposeframe_deactivate);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_uninstallZbuttonList, NULL,
		      zmlcomposeframe_uninstallZbuttonList);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_installZbutton, NULL,
		      zmlcomposeframe_installZbutton);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_uninstallZbutton, NULL,
		      zmlcomposeframe_uninstallZbutton);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_setActionArea, NULL,
		      zmlcomposeframe_setActionArea);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_dialog_updateZbutton, NULL,
		      zmlcomposeframe_updateZbutton);
    spoor_AddOverride(zmlcomposeframe_class, m_spView_install, NULL,
		      zmlcomposeframe_install);
    spoor_AddOverride(zmlcomposeframe_class, m_dialog_enter, NULL,
		      zmlcomposeframe_enter);
    spoor_AddOverride(zmlcomposeframe_class,
		      m_spObservable_receiveNotification, NULL,
		      zmlcomposeframe_receiveNotification);
    m_zmlcomposeframe_readEdFile =
	spoor_AddMethod(zmlcomposeframe_class, "readEdFile",
			NULL,
			zmlcomposeframe_readEdFile);
    m_zmlcomposeframe_writeEdFile =
	spoor_AddMethod(zmlcomposeframe_class, "writeEdFile",
			NULL,
			zmlcomposeframe_writeEdFile);

    spWidget_AddInteraction(spwc_Compose, "goto-to-header", gotoTo,
			    catgets(catalog, CAT_LITE, 131, "Move to To: header"));
    spWidget_AddInteraction(spwc_Compose, "goto-subject-header", gotoSubject,
			    catgets(catalog, CAT_LITE, 132, "Move to Subject: header"));
    spWidget_AddInteraction(spwc_Compose, "goto-cc-header", gotoCc,
			    catgets(catalog, CAT_LITE, 133, "Move to Cc: header"));
    spWidget_AddInteraction(spwc_Compose, "goto-bcc-header", gotoBcc,
			    catgets(catalog, CAT_LITE, 134, "Move to Bcc: header"));
    spWidget_AddInteraction(spwc_Compose, "goto-body", gotoBody,
			    catgets(catalog, CAT_LITE, 135, "Move to message body"));

    ZmCallbackAdd(VarAutosave, ZCBTYPE_VAR, autosave_cb, 0);
    if (autosave_str = get_var_value(VarAutosave))
	autosave = atoi(autosave_str);

    dynhdrs_InitializeClass();
    spToggle_InitializeClass();
    spWrapview_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spButtonv_InitializeClass();
    spToggle_InitializeClass();
    spMenu_InitializeClass();
    spCmdline_InitializeClass();
    spWindow_InitializeClass();
    spIm_InitializeClass();
    smallaliases_InitializeClass();
}

/* Perform a Z-Script mail-editing commmand on the indicated
 * composition.
 * Return EDMAIL_ABORT if the operation fails or could not be performed,
 * EDMAIL_COMPLETED if the operation was performed successfully by this
 * routine, EDMAIL_UNCHANGED if the compose state was not modified by this
 * routine, and EDMAIL_STATESAVED if calling this routine caused state
 * to be written into the file and into the compose structure.  In either
 * of the latter cases, the caller should do the operation directly on
 * the compose structure, but in the STATESAVED case the caller is expected
 * to synchronize the compose window via gui_end_edit().  In the UNCHANGED
 * case it is not safe to call gui_end_edit(); if the caller needs to modify
 * UI-visible parts of the compose structure or the message text, an error
 * should be generated.
 *
 * Bart: Thu Apr 21 21:46:48 PDT 1994
 * The above is pretty bogus -- it means this function has to know what the
 * caller is going to do in each of the cases that this routine does not
 * handle, and the caller has to know what parts of the compose structure
 * are `visible' to the UI.  This whole thing should be replaced with a real
 * API.  But at * least it's better than before, when the caller couldn't
 * tell whether it was safe to muck with the message text or to call
 * gui_end_edit().
 */

int
gui_edmail(code, negate, param, compose)
    int code, negate;
    char *param;
    Compose *compose;
EXC_BEGIN
{
    struct zmlcomposeframe *self = compose->compframe;

    if (compose->exec_pid)
	return EDMAIL_ABORT;

#ifndef ZMCOT
    if (!self)
	return EDMAIL_ABORT;
#endif /* ZMCOT */

    TRY {
	switch (code) {
	  case '\n':		/* append newline */
	    spSend(spView_observed(self->body), m_spText_insert,
		   -1, -1, "\n", spText_mNeutral);
	    /* Fall through */
	  case ' ':			/* append text */
	    if (self)
		spSend(spView_observed(self->body), m_spText_insert,
		       -1, -1, param, spText_mNeutral);
	    EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	  case 1:			/* attach-info */
	  case 2:			/* interpose */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 3:			/* spell */
	    do_spell(self);
	    EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	  case '.':			/* send */
	    EXC_RETURNVAL(int, (do_send(self) ? -1 : 0));
	  case 'A':			/* attach */
	    spSend(ZmlIm, m_spObservable_notifyObservers,
		   zmlcomposeframe_changeAttachments, 0);
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'b':			/* bcc */
	    spSend(self, m_spView_invokeInteraction, "goto-bcc-header",
		   0, 0, 0);
	    if (!param)
		EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	    break;
	  case 'c':			/* cc */
	    spSend(self, m_spView_invokeInteraction, "goto-cc-header",
		   0, 0, 0);
	    if (!param)
		EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	    break;
	  case 'e':			/* edit */
	  case 'v':			/* edit (visual) */
	    do_edit(self);
	    if (ison(compose->flags, DOING_INTERPOSE))
		interposer_thwart = 1;
	    EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	  case 'F':			/* fortune */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'L':			/* log */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'l':			/* record */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'k':			/* charset */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED);
	  case 'P':			/* preview */
	    spSend(ZmlIm, m_spObservable_notifyObservers,
		   zmlcomposeframe_changeAttachments, 0);
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'p':			/* pager */
	    EXC_RETURNVAL(int, EDMAIL_ABORT);	/* like the Motif version */
	  case 'q':			/* cancel */
	  case 'x':			/* kill */
	    if (ison(compose->flags, DOING_INTERPOSE))
		EXC_RETURNVAL(int, EDMAIL_UNCHANGED);
	    do_abort(self, code == 'x');
	    EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	  case 'R':			/* receipt */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'r':			/* insert file */
	    do_insert_file(self, param);
	    EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	  case 's':			/* subject */
	    spSend(self, m_spView_invokeInteraction, "goto-subject-header",
		   0, 0, 0);
	    if (!param)
		EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	    break;
	  case 'S':			/* signature */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 't':			/* to */
	    spSend(self, m_spView_invokeInteraction, "goto-to-header",
		   0, 0, 0);
	    if (!param)
		EXC_RETURNVAL(int, EDMAIL_COMPLETED);
	    break;
	  case 'T':			/* transport-via */
	    EXC_RETURNVAL(int, EDMAIL_UNCHANGED); /* like the Motif version */
	  case 'w':			/* write */
	    EXC_RETURNVAL(int,
		(do_save(self, param) ? EDMAIL_ABORT : EDMAIL_COMPLETED));
	}
    } EXCEPT(spView_FailedInteraction) {
	EXC_RETURNVAL(int, EDMAIL_ABORT);
    } EXCEPT(strerror(EPERM)) {
	EXC_RETURNVAL(int, EDMAIL_COMPLETED);
    } EXCEPT(strerror(EACCES)) {
	EXC_RETURNVAL(int, EDMAIL_COMPLETED);
#ifdef ELOOP
    } EXCEPT(strerror(ELOOP)) {
	EXC_RETURNVAL(int, EDMAIL_COMPLETED);
#endif
    } EXCEPT(strerror(ENOTDIR)) {
	EXC_RETURNVAL(int, EDMAIL_COMPLETED);
    } EXCEPT(strerror(ENOENT)) {
	EXC_RETURNVAL(int, EDMAIL_COMPLETED);
    } ENDTRY;

    /* We have to sync the file with the compose structure */
    spSend(self, m_zmlcomposeframe_writeEdFile, 0, 0);
    resume_compose(compose);
    if (reload_edfile() != 0) {
	do_abort(self, 0);
	wprint(catgets(catalog, CAT_LITE, 129, "Message not sent.\n"));
	return EDMAIL_ABORT;
    }
    suspend_compose(compose);

    /* m_edit.c contains a comment about how the following is
     * sometimes wrong.  Don't ask me what it means.  I just
     * work here.
     */
    grab_addresses(self, 0);

    switch (code) {
      case 'a':			/* save */
      case 'b':			/* bcc */
      case 'c':			/* cc */
      case 's':			/* subject */
      case 't':			/* to */
      case 'C':			/* address check */
	return EDMAIL_STATESAVED; /* like the Motif version */
      case 'D':			/* directory */
      case 'd':			/* save as draft */
	return EDMAIL_STATESAVED;
      case 'E':			/* erase */
      case 'f':			/* forward */
      case 'H':			/* insert-header */
      case 'I':			/* insert message */
      case 'i':			/* include message */
      case 'M':			/* message attached */
      case 'm':			/* insert message */
	return EDMAIL_STATESAVED;	/* like the Motif version */
      case 'O':			/* sort addresses */
	return EDMAIL_STATESAVED;
      case 'z':
	return EDMAIL_ABORT;	/* like the Motif version */
      case '|':			/* pipe */
	return EDMAIL_STATESAVED;	/* like the Motif version */
      case '\0':		/* get header */
	return EDMAIL_STATESAVED;	/* like the Motif version */
    }

    return EDMAIL_ABORT;		/* if all else fails, fail */
} EXC_END

void
gui_end_edit(compose)
    Compose *compose;
{
    int autosendit = 0;
    struct zmlcomposeframe *self = compose->compframe;

    /* We've overloaded SEND_NOW to accomplish autosend.  Must make
     * sure that it's off before making calls into compose.c.
     */
    if (ison(compose->send_flags, SEND_NOW)) {
	turnoff(compose->send_flags, SEND_NOW);
	autosendit = 1;
    }

    /* Bart: Thu Sep  3 15:42:23 PDT 1992
     * Sync the compose window with the file as we come out of the editor.
     */
    if (open_edfile(compose) < 0 || parse_edfile(compose) < 0) {
	do_abort(self, 0);
	return;
    }
    (void) close_edfile(compose);

    spSend(self, m_zmlcomposeframe_readEdFile, 0);

    if (autosendit) {
	do_send(self);
	return;
    }
    if (spView_window(self->body)
	&& (CurrentDialog == (struct dialog *) self))
	spSend(self->body, m_spView_wantFocus, self->body);
}

int
gui_open_compose(item, curr)
    struct dialog *item;
    Compose *curr;
{
    new_comp = curr;
    turnon(new_comp->flags, ACTIVE);
#ifdef ZMCOT
    if (CurrentDialog == Dialog(&ZmcotDialog)) {
	curr->compframe = 0;
	turnoff(curr->flags, EDIT_HDRS);
	return (0);
    }
#endif /* ZMCOT */
    LITE_BUSY {
	spSend(zmlcomposeframe_NEW(), m_dialog_enter);
    } LITE_ENDBUSY;
    return (0);
}
