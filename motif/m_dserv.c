/* m_dserv.c	Copyright 1993 Z-Code Software Corp. */

/*
 * Directory services dialogs.
 */

#ifndef lint
static char	m_dserv_rcsid[] =
    "$Id: m_dserv.c,v 2.76 2005/05/09 09:15:20 syd Exp $";
#endif

#include "zmail.h"

#ifdef DSERV

/* Note that LDAP_LABEL_WIDTH is pels and LDAP_PATTERN_WIDTH is in characters */
#define LDAP_LABEL_WIDTH        140
#define LDAP_PATTERN_WIDTH      35
#define MATCHES_LIST_WIDTH      55
#define MAX_LDAP_LINES          5
#define MAX_LDAP_HOSTS          16 
#define MAX_LDAP_NAME           64
#define MAX_LDAP_SEARCH_PATTERN 512
#define LDAP_RESOURCES          "ldap.zmailrc"

#include "zmframe.h"
#include "zmcomp.h"
#include "catalog.h"
#include "cursors.h"
#include "dirserv.h"
#include "dismiss.h"
#include "m_comp.h"
#include "zm_motif.h"
#include "mime.h"

#include <c3/dyn_c3.h>

#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/PanedW.h>
#include <Xm/MainW.h>
#include <Xm/PushB.h>
#include <Xm/BulletinB.h>
#include <Xm/Separator.h>
#include <Xm/ToggleB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

extern Widget CreateRJustScrolledText(), CreateLJustScrolledText();

#endif /* DSERV */

#include "bitmaps/rolo.xbm"
ZcIcon addrbook_icon = {
    "addrbook_icon", 0, rolo_width, rolo_height, rolo_bits
};

/* A structure to hold ldap information */
typedef struct LDAPDef {
/* These widgets are only used as part of the AddressBrowser structure */
    Widget size_button;
    Widget option_menu;
    Widget option_submenu;
    Widget ldaptext[MAX_LDAP_LINES];
    Widget ldaplabel[MAX_LDAP_LINES];
/* These variables hold information from the ldap resources file */
    Boolean use_ldap;
    int  n_ldap_patterns;
    int  n_ldap_returns;
    int  n_ldap_visible;
    int  n_ldap_current_host;
    char ldap_host_address[MAX_LDAP_NAME+1];
    char ldap_host_name[MAX_LDAP_NAME+1];
    char ldap_search_base[MAX_LDAP_NAME+1];
    char ldap_options[MAX_LDAP_NAME+1];
    char ldap_name_index[MAX_LDAP_NAME+1];
    char ldap_password[MAX_LDAP_NAME+1];
    char ldap_attribute_name[MAX_LDAP_LINES][MAX_LDAP_NAME+1];
    char ldap_symbolic_name[MAX_LDAP_LINES][MAX_LDAP_NAME+1];
    char ldap_returns_symbolic_name[MAX_LDAP_LINES][MAX_LDAP_NAME+1];
    char ldap_compare_type[MAX_LDAP_LINES][16];
} LDAPDef;

/* A temporary place to read ldap resources into */
static LDAPDef ldap_def_read; 

/* An array of ldap host names */
static char *ldap_host_names[MAX_LDAP_HOSTS+1];

/* An array of pointers to ldap host names */
static char ldap_host_array[MAX_LDAP_HOSTS+1][MAX_LDAP_NAME+1];

/* the name of the current ldap host */
static char the_current_ldap_host[MAX_LDAP_NAME+1] = "";

#ifdef DSERV

static void
ok_callback(w, client_data)
Widget w;
AskAnswer *client_data;
{
    *client_data = AskYes;
}

static void
edit_callback(w, client_data)
Widget w;
AskAnswer *client_data;
{
    *client_data = AskNo;
}

static void
cancel_callback(w, client_data)
Widget w;
AskAnswer *client_data;
{
    *client_data = AskCancel;
}

ActionAreaItem addr_confirm_actions[] = {
    { "Ok",	ok_callback,	 NULL },
    { NULL,	(void_proc)0,	 NULL },
    { "Edit",	edit_callback,   NULL },
    { NULL,	(void_proc)0,	 NULL },
    { "Cancel", cancel_callback, NULL },
};

Widget
CreateRJustScrolledText(name, parent, label)
Widget parent;
char *name, *label;
{
    Widget form, item, label_w;
    char widget_name[256];	/* Hopefully big enough */
    Arg args[10];
    int argcount;

    if (!name)
	name = "";

    form = XtVaCreateWidget(name, xmFormWidgetClass, parent, NULL);

    (void) sprintf(widget_name, "%s_text", name);
    argcount = XtVaSetArgs(args, XtNumber(args),
	XmNrightAttachment,  XmATTACH_FORM,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNresizeWidth,      False,
	XmNeditMode,         XmMULTI_LINE_EDIT,
	XmNscrollHorizontal, False,	/* Get rid of horizontal scrollbar */
	NULL);
    item = XmCreateScrolledText(form, widget_name, args, argcount);

    /* Set default RJust attachment of label (see LJust below) */
    (void) sprintf(widget_name, "%s_label", name);
    label_w = XtVaCreateManagedWidget(widget_name, xmLabelGadgetClass, form,
	XmNalignment,        XmALIGNMENT_END,
	XmNtopAttachment,    XmATTACH_FORM,
	XmNrightAttachment,  XmATTACH_WIDGET,
	XmNrightWidget,      item,
	label? XmNlabelString : SNGL_NULL, label? zmXmStr(label) : NULL_XmStr,
	NULL);

    XtManageChild(item);
    XtManageChild(form);
    return item;
}

Widget
CreateLJustScrolledText(name, parent, label)
Widget parent;
char *name, *label;
{
    Widget form, item, label_w;
    char widget_name[256];	/* Hopefully big enough */

    /* Create an RJust text and change the attachments */

    item = CreateRJustScrolledText(name, parent, label);
    (void) sprintf(widget_name, "*%s", name);
    form = XtNameToWidget(parent, widget_name);
    (void) sprintf(widget_name, "*%s_label", name);
    label_w = XtNameToWidget(form, widget_name);

    XtVaSetValues(XtParent(item),
	XmNleftAttachment, XmATTACH_WIDGET,
	XmNleftWidget,     label_w,
	/* XmNresizeWidth,    True, */
	NULL);
    XtVaSetValues(label_w,
	XmNleftAttachment,  XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_NONE,
	XmNalignment,       XmALIGNMENT_BEGINNING,
	NULL);
    return item;
}

AskAnswer
gui_confirm_addresses(compose)
Compose *compose;
{
    static ZmFrame frame;
    Widget bboard = 0, pane, w;
    int i;

    /* This stuff should all be in a struct or something */
    static AskAnswer answer;
    static Widget fields[NUM_ADDR_FIELDS+1], *kids;
    static Widget parents[NUM_ADDR_FIELDS+1];
#define FROM_ADDR NUM_ADDR_FIELDS

    /* ask_item should not need to change in this function! */

    timeout_cursors(TRUE);
    assign_cursor(frame_list, do_not_enter_cursor);

    if (!frame) {
	frame = FrameCreate("confirm_address_dialog",
	    FrameConfirmAddrs,  hidden_shell,
	    FrameFlags,         FRAME_CANNOT_SHRINK, 
	    FrameFlags,         0, 
	    /* FrameChildClass, xmBulletinBoardWidgetClass, */
	    FrameChildClass,    xmFormWidgetClass,
	    FrameChild,         &bboard,
	    FrameEndArgs);
#ifdef NOT_NOW
	/* Special case to prevent stupid placement */
	XtRemoveCallback(GetTopShell(bboard),
	    XmNpopupCallback, place_dialog, hidden_shell);
#endif /* NOT_NOW */
	XtVaSetValues(bboard,
	    XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
	    NULL);
	pane = XtVaCreateWidget(NULL,
#ifdef SANE_WINDOW
	    zmSaneWindowWidgetClass, bboard,
#else /* !SANE_WINDOW */
	    xmPanedWindowWidgetClass, bboard,
#endif /* !SANE_WINDOW */
	    XmNtopAttachment,	XmATTACH_FORM,
	    XmNbottomAttachment,XmATTACH_FORM,
	    XmNleftAttachment,	XmATTACH_FORM,
	    XmNrightAttachment,	XmATTACH_FORM,
	    XmNseparatorOn,	False,
	    XmNsashWidth,	1,
	    XmNsashHeight,	1,
	    NULL);

	w = fields[FROM_ADDR] =
	    CreateRJustLabeledText("from", pane, NULL);
	XtVaSetValues(w,
	    XmNeditable,              False,
	    XmNcursorPositionVisible, False,
	    XmNblinkRate,             0,
	    NULL);
	parents[FROM_ADDR] = XtParent(w);

	w = fields[SUBJ_ADDR] =
	    CreateRJustLabeledText("subject", pane, NULL);
	XtVaSetValues(w,
	    XmNeditable,              False,
	    XmNcursorPositionVisible, False,
	    XmNblinkRate,             0,
	    NULL);
	parents[SUBJ_ADDR] = XtParent(w);

	w = fields[TO_ADDR] =
	    CreateRJustScrolledText("to", pane, NULL);
	XtVaSetValues(w,
	    XmNeditable,              False,
	    XmNcursorPositionVisible, False,
	    XmNblinkRate,             0,
	    NULL);
	parents[TO_ADDR] = XtParent(XtParent(w));

	w = fields[CC_ADDR] =
	    CreateRJustScrolledText("cc", pane, NULL);
	XtVaSetValues(w,
	    XmNeditable,              False,
	    XmNcursorPositionVisible, False,
	    XmNblinkRate,             0,
	    NULL);
	parents[CC_ADDR] = XtParent(XtParent(w));

	w = fields[BCC_ADDR] =
	    CreateRJustScrolledText("bcc", pane, NULL);
	XtVaSetValues(w,
	    XmNeditable,              False,
	    XmNcursorPositionVisible, False,
	    XmNblinkRate,             0,
	    NULL);
	parents[BCC_ADDR] = XtParent(XtParent(w));

	SetDeleteWindowCallback(GetTopShell(pane), cancel_callback, &answer);
	addr_confirm_actions[0].data =
	addr_confirm_actions[2].data =
	addr_confirm_actions[4].data = (caddr_t)(&answer);
	w = CreateActionArea(pane,
	    addr_confirm_actions, XtNumber(addr_confirm_actions), "Address Confirmation");
	SetPaneMaxAndMin(w); 
	XtManageChild(pane);
	XtVaGetValues(w, XmNchildren, &kids, NULL);
    }

    if (ison(compose->send_flags, SEND_NOW))
	XtSetSensitive(kids[2], False);
    else
	XtSetSensitive(kids[2], True);

    zmXmTextSetString(fields[FROM_ADDR], compose->rrto);
    for (i = 0; i < NUM_ADDR_FIELDS; i++) {
	if (fields[i]) {
	    if (compose->addresses[i] && *(compose->addresses[i])) {
		if (i == SUBJ_ADDR) {
		    zmXmTextSetString(fields[i], compose->addresses[i]);
		} else {
		    short cols;
		    char *tmp = savestr(compose->addresses[i]);

		    XtVaGetValues(fields[i], XmNcolumns, &cols, NULL);
		    zmXmTextSetString(fields[i], wrap_addrs(tmp, cols, FALSE));
		    xfree(tmp);
		}
		XtManageChild(parents[i]);
	    } else {
		zmXmTextSetString(fields[i], NULL);
		XtUnmanageChild(parents[i]);
	    }
	}
    }

    FramePopup(frame);

    for (answer = AskUnknown; answer == AskUnknown; )
	XtAppProcessEvent(app, XtIMAll);

    FramePopdown(frame);

    assign_cursor(frame_list, please_wait_cursor);
    timeout_cursors(FALSE);

    return answer;
}

typedef struct AddressBrowser {
    Widget intext;
    Widget outtext;
    Widget memtext;
    char *textvalue[3];
    int state;
    LDAPDef ldap_def;
} AddressBrowser;

#define SHOW_NONE	0
#define SHOW_DESCR	1
#define SHOW_ADDRS	2
#define SHOW_DESCR_NAME	"show_description"
#define SHOW_ADDRS_NAME	"show_addresses"

/*
  This function generates the command line arguments for the stand alone
  program lookup.ldap. lookup.ldap generates a ldap search script, executes
  the script, and formats the search results prior to sending then back
  here to be displayed.
*/
static 
void generate_ldap_search_pattern(ldap_search_pattern,ab,n_patterns,ldap_pattern)
AddressBrowser *ab;
char *ldap_search_pattern;
int n_patterns;
char *ldap_pattern[MAX_LDAP_LINES];
{
int i , j;

  strcpy(ldap_search_pattern,"\"");
  if ((strcmp(ab->ldap_def.ldap_host_address,"none") != 0) && (strlen(ab->ldap_def.ldap_host_address) != 0))
    {
      strcat(ldap_search_pattern,"-h ");
      strcat(ldap_search_pattern,ab->ldap_def.ldap_host_address);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ab->ldap_def.ldap_search_base,"none") != 0) && (strlen(ab->ldap_def.ldap_search_base) != 0))
    {
      strcat(ldap_search_pattern,"-b ");
      strcat(ldap_search_pattern,ab->ldap_def.ldap_search_base);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ab->ldap_def.ldap_options,"none") != 0) && (strlen(ab->ldap_def.ldap_options) != 0))
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ab->ldap_def.ldap_options);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ab->ldap_def.ldap_password,"none") != 0) && (strlen(ab->ldap_def.ldap_password) != 0))
    {
      strcat(ldap_search_pattern,"-w ");
      strcat(ldap_search_pattern,ab->ldap_def.ldap_password);
      strcat(ldap_search_pattern," ");
    }
  strcat(ldap_search_pattern,"'(");
  if (n_patterns == 1)
    {
      for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
        if (ldap_pattern[i] != 0)
          if (strlen(ldap_pattern[i]) != 0)
            {
              strcat(ldap_search_pattern,ab->ldap_def.ldap_symbolic_name[i]);
              strcat(ldap_search_pattern,ab->ldap_def.ldap_compare_type[i]);
              strcat(ldap_search_pattern,ldap_pattern[i]);
              if (strcmp(ab->ldap_def.ldap_compare_type[i],"=*") == 0)
                strcat(ldap_search_pattern,"*");
            }
    }
  else
    {
      for (i=0;i<n_patterns-2;i++)
        strcat(ldap_search_pattern,"&(");
      strcat(ldap_search_pattern,"&");
      j = 0;
      for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
        if (ldap_pattern[i] != 0)
          if (strlen(ldap_pattern[i]) != 0)
          {
            strcat(ldap_search_pattern,"(");
            strcat(ldap_search_pattern,ab->ldap_def.ldap_symbolic_name[i]);
            strcat(ldap_search_pattern,ab->ldap_def.ldap_compare_type[i]);
            strcat(ldap_search_pattern,ldap_pattern[i]);
            if (strcmp(ab->ldap_def.ldap_compare_type[i],"=*") == 0)
                strcat(ldap_search_pattern,"*");
            strcat(ldap_search_pattern,")");
            if ((j > 0) && (j < (n_patterns-1)))
              strcat(ldap_search_pattern,")");
            j++;
          }
    }
  strcat(ldap_search_pattern,")'");
  for (i=0;i<(ab->ldap_def.n_ldap_returns);i++)
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ab->ldap_def.ldap_returns_symbolic_name[i]);
    }
  strcat(ldap_search_pattern,"\"");
}

/*
  This function returns the ldap name index as an integer. This
  function call should be preceeded by a call to load_ldap_resources.
*/
int get_ldap_name_index()
{
  return(atoi(ldap_def_read.ldap_name_index));
}

/*
  This function generates an search pattern to be used by ldapsearch for
  address verification. The variable the_name_index contains what is assumed 
  to be the index of the persons name within the list of patterns. This 
  function call should be preceeded by a call to load_ldap_resources.
*/
void generate_ldap_verification_pattern(ldap_search_pattern,name_to_verify,the_name_index)
char *ldap_search_pattern;
char *name_to_verify;
int  the_name_index;
{
int i , j;

  strcpy(ldap_search_pattern,"\"");
  if ((strcmp(ldap_def_read.ldap_host_address,"none") != 0) && (strlen(ldap_def_read.ldap_host_address) != 0))
    {
      strcat(ldap_search_pattern,"-h ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_host_address);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_search_base,"none") != 0) && (strlen(ldap_def_read.ldap_search_base) != 0))
    {
      strcat(ldap_search_pattern,"-b ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_search_base);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_options,"none") != 0) && (strlen(ldap_def_read.ldap_options) != 0))
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_options);
      strcat(ldap_search_pattern," ");
    }
  if ((strcmp(ldap_def_read.ldap_password,"none") != 0) && (strlen(ldap_def_read.ldap_password) != 0))
    {
      strcat(ldap_search_pattern,"-w ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_password);
      strcat(ldap_search_pattern," ");
    }
  strcat(ldap_search_pattern,"'(");
  strcat(ldap_search_pattern,ldap_def_read.ldap_symbolic_name[the_name_index]);
  strcat(ldap_search_pattern,ldap_def_read.ldap_compare_type[the_name_index]);
  strcat(ldap_search_pattern,name_to_verify);
  if (strcmp(ldap_def_read.ldap_compare_type[the_name_index],"=*") == 0)
    strcat(ldap_search_pattern,"*");
  strcat(ldap_search_pattern,")'");
  for (i=0;i<(ldap_def_read.n_ldap_returns);i++)
    {
      strcat(ldap_search_pattern," ");
      strcat(ldap_search_pattern,ldap_def_read.ldap_returns_symbolic_name[i]);
    }
  strcat(ldap_search_pattern,"\"");
}

/*
  This function clears the ldap resources structure.
*/
static
void clear_ldap_resources()
{
int i;
  ldap_def_read.n_ldap_patterns=0;
  ldap_def_read.n_ldap_returns=0;
  ldap_def_read.n_ldap_visible = 0;
  strcpy(ldap_def_read.ldap_host_address,"");
  strcpy(ldap_def_read.ldap_host_name,"");
  strcpy(ldap_def_read.ldap_search_base,"");
  strcpy(ldap_def_read.ldap_options,"");
  strcpy(ldap_def_read.ldap_name_index,"0");
  strcpy(ldap_def_read.ldap_password,"");
  for (i=0;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ldap_def_read.ldap_attribute_name[i],"");
      strcpy(ldap_def_read.ldap_symbolic_name[i],"");
      strcpy(ldap_def_read.ldap_compare_type[i],"");
      strcpy(ldap_def_read.ldap_returns_symbolic_name[i],"");
    }
}

/*
  This is a utility function to trim spaces off of both ends of a string.
*/
static
void trim_both_ends(str)
char *str;
{
  while (str[0] == ' ')
    strcpy(str,str+1);
  while (strlen(str)> 0)
    if ((str[strlen(str)-1] == ' ') || (str[strlen(str)-1] == '\n'))
      str[strlen(str)-1] = '\0';
    else
      break;
}

/*
  This function copies the ldap resources from the structure into which they
  have been read into the AddressBrowser structure.
*/
static
void copy_ldap_resources(ab)
AddressBrowser *ab;
{
int i;

  ab->ldap_def.use_ldap = ldap_def_read.use_ldap;
  ab->ldap_def.n_ldap_patterns = ldap_def_read.n_ldap_patterns;
  ab->ldap_def.n_ldap_returns = ldap_def_read.n_ldap_returns;
  ab->ldap_def.n_ldap_visible = ldap_def_read.n_ldap_visible;
  ab->ldap_def.n_ldap_current_host = ldap_def_read.n_ldap_current_host;
  strcpy(ab->ldap_def.ldap_host_address,ldap_def_read.ldap_host_address);
  strcpy(ab->ldap_def.ldap_host_name,ldap_def_read.ldap_host_name);
  strcpy(ab->ldap_def.ldap_search_base,ldap_def_read.ldap_search_base);
  strcpy(ab->ldap_def.ldap_options,ldap_def_read.ldap_options);
  strcpy(ab->ldap_def.ldap_name_index,ldap_def_read.ldap_name_index);
  strcpy(ab->ldap_def.ldap_password,ldap_def_read.ldap_password);
  for (i=0;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ab->ldap_def.ldap_attribute_name[i],ldap_def_read.ldap_attribute_name[i]);
      strcpy(ab->ldap_def.ldap_symbolic_name[i],ldap_def_read.ldap_symbolic_name[i]);
      strcpy(ab->ldap_def.ldap_compare_type[i],ldap_def_read.ldap_compare_type[i]);
      strcpy(ab->ldap_def.ldap_returns_symbolic_name[i],ldap_def_read.ldap_returns_symbolic_name[i]);
    }
}

/*
  This function sets up a dummy ldap resources structure.
*/
static
void dummy_up_ldap_resources()
{
int i;
  ldap_def_read.n_ldap_patterns=1;
  ldap_def_read.n_ldap_returns=2;
  ldap_def_read.n_ldap_visible = 1;
  strcpy(ldap_def_read.ldap_host_address,"");
  strcpy(ldap_def_read.ldap_host_name,"");
  strcpy(ldap_def_read.ldap_search_base,"");
  strcpy(ldap_def_read.ldap_options,"");
  strcpy(ldap_def_read.ldap_name_index,"0");
  strcpy(ldap_def_read.ldap_password,"");
  strcpy(ldap_def_read.ldap_attribute_name[0],"Pattern:");
  strcpy(ldap_def_read.ldap_symbolic_name[0],"cn");
  strcpy(ldap_def_read.ldap_compare_type[0],"=*");
  strcpy(ldap_def_read.ldap_returns_symbolic_name[0],"cn");
  strcpy(ldap_def_read.ldap_returns_symbolic_name[1],"mail");
  for (i=1;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ldap_def_read.ldap_attribute_name[i],"");
      strcpy(ldap_def_read.ldap_symbolic_name[i],"");
      strcpy(ldap_def_read.ldap_compare_type[i],"");
    }
  for (i=2;i<MAX_LDAP_LINES;i++)
    {
      strcpy(ldap_def_read.ldap_returns_symbolic_name[i],"");
    }
  strcpy(ldap_host_array[0],"host");
  ldap_host_names[0] = ldap_host_array[0];
  ldap_host_names[1] = NULL;
  ldap_def_read.n_ldap_current_host = 0;
}

/* 
  This function reads the selected resources into the ldap resources structure.
  It also builds a list of the names of the ldap host defined in the ldap
  resources file.
*/
Boolean load_ldap_resources(ldap_service_name,initial)
char *ldap_service_name;
int initial;
{
FILE *fp;
int i;
Boolean end_of_file , found_it;
char the_type[MAX_LDAP_NAME+1];
char trash[MAX_LDAP_NAME+1];
char zmlib_path[256];
char test_path[256];
struct stat buf;

  if (getenv("ZMLIB") != NULL)
    {
      strcpy(zmlib_path,getenv("ZMLIB"));
      strcat(zmlib_path,"/");
      strcat(zmlib_path,LDAP_RESOURCES);
    }
  else
    {
      strcpy(zmlib_path,"./lib/");
      strcat(zmlib_path,LDAP_RESOURCES);
    }
  if (getenv("HOME") != NULL)
    {
      strcpy(test_path,getenv("HOME"));
      strcat(test_path,"/.");
      strcat(test_path,LDAP_RESOURCES);
      if (stat(test_path,&buf) == 0)
        strcpy(zmlib_path,test_path);
    }

/*
  Read it just to get the LDAP host names.
*/
  if (!(fp = fopen(zmlib_path, "r"))) {
    error(SysErrWarning, catgets( catalog, CAT_SHELL, 1004, "Cannot find ldap resources" ));
    if (fp)
      (void) fclose(fp);
    return False;
  }
  i = 0;
  while (fscanf(fp,"%s",ldap_def_read.ldap_host_address) != EOF)
    {
      if (strcmp(ldap_def_read.ldap_host_address,"hostname:") != 0)
        continue;
      if (fscanf(fp,"%s",ldap_def_read.ldap_host_address) == EOF)
          break;
      if (fgets(ldap_def_read.ldap_host_name,MAX_LDAP_NAME,fp) == NULL)
          break;
      trim_both_ends(ldap_def_read.ldap_host_name);
      strcpy(ldap_host_array[i],ldap_def_read.ldap_host_name);
      ldap_host_names[i] = ldap_host_array[i];
      if (i < MAX_LDAP_HOSTS)
        i++;
    }
  ldap_host_names[i] = NULL;
  (void) fclose(fp);

/*
  Read it again to get the specific LDAP resources.
*/
  found_it = False;
  end_of_file = False;
  ldap_def_read.n_ldap_current_host = -1;
  clear_ldap_resources();
/* Open the file */
  if (!(fp = fopen(zmlib_path, "r"))) {
    error(SysErrWarning, catgets( catalog, CAT_SHELL, 1000, "Cannot find ldap resources" ));
    if (fp)
      (void) fclose(fp);
    return False;
  }
/*
  Loop through the file reading the resources and storing them. 
*/
  while (!end_of_file)
    {
/* Read a key word */
      if (fscanf(fp,"%s",the_type) == EOF)
        break;
/* If the key word is a hostname ... */
      if (strcmp(the_type,"hostname:") == 0)
        {
/* If the host name previously read is a match I am finished */
          if (strcmp(ldap_service_name,ldap_def_read.ldap_host_name) == 0)
            break;
          clear_ldap_resources();
          if (fscanf(fp,"%s",ldap_def_read.ldap_host_address) == EOF)
            break;
          if (fgets(ldap_def_read.ldap_host_name,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_host_name);
          ldap_def_read.n_ldap_current_host++;
        }
/* If the key word is a searchbase ... */
      else if (strcmp(the_type,"searchbase:") == 0)
        {
          if (fgets(ldap_def_read.ldap_search_base,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_search_base);
        }
/* If the key word is a options ... */
      else if (strcmp(the_type,"options:") == 0)
        {
          if (fgets(ldap_def_read.ldap_options,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_options);
        }
/* If the key word is a nameindex ... */
      else if (strcmp(the_type,"nameindex:") == 0)
        {
          if (fgets(ldap_def_read.ldap_name_index,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_name_index);
        }
/* If the key word is a password ... */
      else if (strcmp(the_type,"password:") == 0)
        {
          if (fgets(ldap_def_read.ldap_password,MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_password);
        }
/* If the key word is a pattern ... */
      else if (strcmp(the_type,"pattern:") == 0)
        {
          if (fscanf(fp," %s ",ldap_def_read.ldap_symbolic_name[ldap_def_read.n_ldap_patterns]) == EOF)
            break;
          if (fscanf(fp," %s ",ldap_def_read.ldap_compare_type[ldap_def_read.n_ldap_patterns]) == EOF)
            break;
          if (fgets(ldap_def_read.ldap_attribute_name[ldap_def_read.n_ldap_patterns],MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_attribute_name[ldap_def_read.n_ldap_patterns]);
          if (ldap_def_read.n_ldap_patterns < MAX_LDAP_LINES)
            ldap_def_read.n_ldap_patterns++;
        }
/* If the kay word is a return ... */
      else if (strcmp(the_type,"returns:") == 0)
        {
          if (fgets(ldap_def_read.ldap_returns_symbolic_name[ldap_def_read.n_ldap_returns],MAX_LDAP_NAME,fp) == NULL)
            break;
          trim_both_ends(ldap_def_read.ldap_returns_symbolic_name[ldap_def_read.n_ldap_returns]);
          if (ldap_def_read.n_ldap_returns < MAX_LDAP_LINES)
            ldap_def_read.n_ldap_returns++;
        }
/* If the line is a comment ... */
      else if (the_type[0] == '#')
        {
          if (fgets(trash,MAX_LDAP_NAME,fp) == NULL)
            break;
        }
/* Else trash it */
      else
        {
          if (fgets(trash,MAX_LDAP_NAME,fp) == NULL)
            break;
        }
    }

  fclose(fp);
  if (strcmp(ldap_service_name,ldap_def_read.ldap_host_name) == 0)
    found_it = True;
  if (ldap_def_read.n_ldap_patterns < 1) {
            error(SysErrWarning, catgets( catalog, CAT_SHELL, 1001, "No patterns specified in resources" ));
    return False;
  }
  if (ldap_def_read.n_ldap_returns < 1) {
            error(SysErrWarning, catgets( catalog, CAT_SHELL, 1002, "No returns specified in resources" ));
    return False;
  }
  if (!found_it) {
            error(SysErrWarning, catgets( catalog, CAT_SHELL, 1003, "No matching LDAP service" ));
    return False;
  }
  ldap_def_read.n_ldap_visible = ldap_def_read.n_ldap_patterns;
  return True;
}

/*
  This function puts the labels defined in the ldap resources into the labels
  in the browser dialog.
*/
static
void set_ldap_labels(ab)
AddressBrowser *ab;
{
int j;

    for (j=0;j<ab->ldap_def.n_ldap_patterns;j++)
      {
        XtVaSetValues(ab->ldap_def.ldaplabel[j],
        XmNlabelString, zmXmStr(ab->ldap_def.ldap_attribute_name[j]),
        NULL);
      }
}
 
/* 
  This function removes unused patterns from the from the dialog.
*/
static
void remove_unused_ldap_patterns(ab)
AddressBrowser *ab;
{
int i;
 
  if (ab->ldap_def.use_ldap)
    {
      for (i=(ab->ldap_def.n_ldap_visible);i<MAX_LDAP_LINES;i++)
        XtUnmanageChild(XtParent(ab->ldap_def.ldaptext[i])); 
    }
}
 
/*
  This function customizes the dialog to be either the old address lookup
  dialog or the new ldap pattern match dialog.
*/
static
void customize_ldap_display(ab)
AddressBrowser *ab;
{
int i;
 
  if (ab->ldap_def.use_ldap)
    {
      XtUnmanageChild(XtParent(ab->intext));
      for (i=0;i<(ab->ldap_def.n_ldap_visible);i++)
        XtManageChild(XtParent(ab->ldap_def.ldaptext[i]));
    }
  else
    {
      XtUnmanageChild(XtParent(ab->ldap_def.option_menu));
      XtManageChild(XtParent(ab->intext));
      for (i=0;i<MAX_LDAP_LINES;i++)
        XtUnmanageChild(XtParent(ab->ldap_def.ldaptext[i]));
    }
}

char *
AddressBrowserGetOutTextValue(ab)
AddressBrowser *ab;
{
    return ab->textvalue[ab->state];
}

void
AddressBrowserSetInTextValue(ab, s)
AddressBrowser *ab;
char *s;
{
    if (ab->intext) zmXmTextSetString(ab->intext, s);
}
 
void
AddressBrowserSetLDAPTextValue(ab, i, s)
AddressBrowser *ab;
int i;
char *s;
{
    if (ab->ldap_def.ldaptext[i]) zmXmTextSetString(ab->ldap_def.ldaptext[i], s);
}

void
AddressBrowserSetMemTextValue(ab, s)
AddressBrowser *ab;
char *s;
{
    if (ab->memtext) zmXmTextSetString(ab->memtext, s);
}

#ifndef OUT_TEXT
static void
set_list_text(list_w, str)
Widget list_w;
char *str;
{
    char **vec;
    XmStringTable strtab;
    int i = 0, *pos, npos;
    Boolean sel;

    vec = strvec(str, "\n", False);
    strtab = (XmStringTable) XtCalloc(vlen(vec)+1,
				      sizeof *strtab);
    if (vec)
	for (i = 0; vec[i]; i++)
	    strtab[i] = XmStr(vec[i]);
    strtab[i] = (XmString) 0;
    sel = XmListGetSelectedPos(list_w, &pos, &npos);
    XtUnmanageChild(XtParent(list_w));
    XtVaSetValues(list_w,
	XmNitems,     strtab,
	XmNitemCount, i,
	NULL);
    XmStringFreeTable(strtab);
    if (vec) free_vec(vec);
    XtManageChild(XtParent(list_w));
    if (sel) {
	if (npos) {
	    LIST_VIEW_POS(list_w, pos[0]);
	    XmListSelectPos(list_w, pos[0], True);
	}
	XtFree((char *)pos);
    }
}
#endif /* !OUT_TEXT */

void
AddressBrowserSetOutTextValues(ab, addr, desc)
AddressBrowser *ab;
char *addr, *desc;
{
    ab->textvalue[SHOW_ADDRS] = addr;
    ab->textvalue[SHOW_DESCR] = desc;
    if (ab->outtext)
#ifdef OUT_TEXT
	zmXmTextSetString(ab->outtext, AddressBrowserGetOutTextValue(ab));
#else /* !OUT_TEXT */
        set_list_text(ab->outtext, AddressBrowserGetOutTextValue(ab));
#endif /* !OUT_TEXT */
}

#define AddressBrowserGetInText(ab)		((ab)->intext)
#define AddressBrowserGetLDAPNumber(ab)		((ab)->ldap_def.n_ldap_patterns)
#define AddressBrowserGetLDAPVisible(ab)	((ab)->ldap_def.n_ldap_visible)
#define AddressBrowserGetLDAPReturns(ab)	((ab)->ldap_def.n_ldap_returns)
#define AddressBrowserGetLDAPText(ab,i)		((ab)->ldap_def.ldaptext[i])
#define AddressBrowserGetLDAPLabel(ab,i)	((ab)->ldap_def.ldaplabel[i])
#define AddressBrowserGetOutText(ab)		((ab)->outtext)
#define AddressBrowserGetMemText(ab)		((ab)->memtext)
#define AddressBrowserGetState(ab)		((ab)->state)

#define AddressBrowserNothingShown(ab)		((ab)->state == SHOW_NONE)
#define AddressBrowserDescriptionsShown(ab)	((ab)->state == SHOW_DESCR)
#define AddressBrowserAddressesShown(ab)	((ab)->state == SHOW_ADDRS)

#define AddressBrowserGetAddresses(ab)		((ab)->textvalue[SHOW_ADDRS])
#define AddressBrowserGetDescriptions(ab)	((ab)->textvalue[SHOW_DESCR])

#define AddressBrowserSetInText(ab, t)		((ab)->intext = (t))
#define AddressBrowserSetLDAPNumber(ab,i)	((ab)->ldap_def.n_ldap_patterns = i)
#define AddressBrowserSetLDAPVisible(ab,i)	((ab)->ldap_def.n_ldap_visible = i)
#define AddressBrowserSetLDAPReturns(ab,i)	((ab)->ldap_def.n_ldap_returns = i)
#define AddressBrowserSetLDAPText(ab,i, t)	((ab)->ldap_def.ldaptext[i] = (t))
#define AddressBrowserSetLDAPLabel(ab,i, t)	((ab)->ldap_def.ldaplabel[i] = (t))
#define AddressBrowserSetOutText(ab, t)		((ab)->outtext = (t))
#define AddressBrowserSetMemText(ab, t)		((ab)->memtext = (t))
#define AddressBrowserSetState(ab, s)		((ab)->state = (s))

#ifdef NOT_NOW
static MenuItem file_items[] = {
    { "fm_close", &xmPushButtonWidgetClass,
        DeleteFrameCallback, 0, 1, (MenuItem *)NULL },
    NULL,
};
#endif /* NOT_NOW */

static void
browser_cache(w, ab)
Widget w;
AddressBrowser *ab;
{
int i , k;
    char *pattern;
    char *address = XmTextGetString(AddressBrowserGetMemText(ab));
    char *oldaddr;

    if (ab->ldap_def.use_ldap)
      {
        k = -1;
        for (i=0;i<(ab->ldap_def.n_ldap_visible);i++)
          {
            pattern = XmTextGetString(AddressBrowserGetLDAPText(ab,i));
            if (!pattern || !*pattern) 
              {
                XtFree(pattern);
                continue;
              }
            else
              {
                k = i;
                ask_item = AddressBrowserGetLDAPText(ab,i);
                break;
              }
          }
        if (k < 0)
          {
            ask_item = AddressBrowserGetLDAPText(ab,0);
            pattern = XmTextGetString(AddressBrowserGetLDAPText(ab,0));
          }
      }
    else
      {
        pattern = XmTextGetString(AddressBrowserGetInText(ab));
        ask_item = AddressBrowserGetInText(ab);
      }
    if (!pattern || !*pattern) {
	XtFree(pattern);
	XtFree(address);
	error(UserErrWarning, catgets(
	catalog, CAT_MOTIF, 862, "Please supply both a search pattern\nand an address to remember."));
	return;
    }
    ask_item = AddressBrowserGetMemText(ab);
    if (!address || !*address) {
	XtFree(pattern);
	XtFree(address);
	error(UserErrWarning, catgets(
	catalog, CAT_MOTIF, 863, "Please supply both a search pattern\nand an address to remember."));
	return;
    }

    oldaddr = uncache_address(pattern);
    xfree(oldaddr);

    cache_address(pattern, address, True);

    XtFree(pattern);
    XtFree(address);
    DismissSetWidget(w, DismissClose);
}

static void
browser_uncache(w, ab)
Widget w;
AddressBrowser *ab;
{
int i , k;
    char *pattern;
    char *address = XmTextGetString(AddressBrowserGetMemText(ab));
    char *oldpat, *oldaddr;

    if (ab->ldap_def.use_ldap)
      {
        k = -1;
        for (i=0;i<(ab->ldap_def.n_ldap_visible);i++)
          {
            pattern = XmTextGetString(AddressBrowserGetLDAPText(ab,i));
            if (!pattern || !*pattern) 
              {
                XtFree(pattern);
                continue;
              }
            else
              {
                k = i;
                ask_item = AddressBrowserGetLDAPText(ab,i);
                break;
              }
          }
        if (k < 0)
          {
            ask_item = AddressBrowserGetLDAPText(ab,0);
            pattern = XmTextGetString(AddressBrowserGetLDAPText(ab,0));
          }
      }
    else
      {
        pattern = XmTextGetString(AddressBrowserGetInText(ab));
        ask_item = AddressBrowserGetMemText(ab);
      }
    if (!pattern || !*pattern) {
      error(UserErrWarning, catgets(
		    catalog, CAT_MOTIF, 1100, "Please supply pattern to forget."));
	    return;
    }
    if (!pattern || !*pattern) {
	if (!address || !*address) {
	    error(UserErrWarning, catgets(
		    catalog, CAT_MOTIF, 1101, "Please supply address to forget."));
	    return;
	}
	oldpat = revert_address(oldaddr = address);
    } else {
	oldaddr = uncache_address(oldpat = pattern);
    }

    error(Message, catgets( catalog, CAT_MOTIF, 113, "Forgotten:\n%s -->\n%s" ), oldpat,
	!oldaddr ? catgets(catalog, CAT_MOTIF, 840, "Already forgotten or never remembered.") :
	  strlen(oldaddr) > 50? zmVaStr("%47s...", oldaddr): oldaddr);

    if (oldpat != pattern)
	xfree(oldpat);
    if (oldaddr != address)
	xfree(oldaddr);
    XtFree(pattern);
    XtFree(address);
    DismissSetWidget(w, DismissClose);
}

static void
browser_flush(w, ab)
Widget w;
AddressBrowser *ab;
{
int i;

    address_cache_erase();

    AddressBrowserSetInTextValue(ab, NULL);
    AddressBrowserSetMemTextValue(ab, NULL);
    AddressBrowserSetOutTextValues(ab, NULL, NULL);
    for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
      AddressBrowserSetLDAPTextValue(ab, i, NULL);

    DismissSetWidget(w, DismissClose);
}

static MenuItem edit_items[] = {
    { "em_remember", &xmPushButtonWidgetClass,
	browser_cache, 0, 1, (MenuItem *)NULL },
    { "em_forget", &xmPushButtonWidgetClass,
	browser_uncache, 0, 1, (MenuItem *)NULL },
    { "_sep1", &xmSeparatorWidgetClass,
	0, 0, 1, (MenuItem *)NULL },
    { "em_forget_all", &xmPushButtonWidgetClass,
	browser_flush, 0, 1, (MenuItem *)NULL },
    NULL,
};

#ifdef BROWSER_VIEW_MENU

static void
toggle_shown(menuitem, idx)
Widget menuitem;
int idx;
{
    Widget parent = XtParent(menuitem);
    Widget menuother = XtNameToWidget(parent,
	idx == SHOW_ADDRS? SHOW_DESCR_NAME : SHOW_ADDRS_NAME);
    AddressBrowser *ab = (AddressBrowser *)
	FrameGetClientData(FrameGetData(menuitem));

    XtSetSensitive(menuitem, False);
    XtSetSensitive(menuother, True);

    AddressBrowserSetState(ab, idx);

#ifdef OUT_TEXT
    zmXmTextSetString
#else /* !OUT_TEXT */
    set_list_text
#endif /* !OUT_TEXT */
	(AddressBrowserGetOutText(ab),
	AddressBrowserGetOutTextValue(ab));
}

static MenuItem view_items[] = {
    { SHOW_DESCR_NAME, &xmPushButtonWidgetClass,
	toggle_shown, (caddr_t)SHOW_DESCR, 0, (MenuItem *)NULL },
    { SHOW_ADDRS_NAME, &xmPushButtonWidgetClass,
	toggle_shown, (caddr_t)SHOW_ADDRS, 1, (MenuItem *)NULL },
    NULL,
};

#else

static char *view_items[] = { SHOW_DESCR_NAME, SHOW_ADDRS_NAME };

static void
toggle_shown(toggleb, idx)
Widget toggleb;
int *idx;
{
    AddressBrowser *ab = (AddressBrowser *)
	FrameGetClientData(FrameGetData(toggleb));

    AddressBrowserSetState(ab, *idx);

#ifdef OUT_TEXT
    zmXmTextSetString
#else /* !OUT_TEXT */
    set_list_text
#endif /* !OUT_TEXT */
	(AddressBrowserGetOutText(ab),
	AddressBrowserGetOutTextValue(ab));
}

#endif /* BROWSER_VIEW_MENU */

static MenuItem option_items[] = {
    { "om_aliases", &xmPushButtonWidgetClass,
	popup_dialog, (caddr_t)FrameAlias, 1, (MenuItem *)NULL },
    NULL,
};

static MenuItem help_items[] = {
    { "hm_browser", &xmPushButtonWidgetClass,
	DialogHelp, "Address Browser", 1, (MenuItem *)NULL },
    { "hm_index", &xmPushButtonWidgetClass,
#ifdef HAVE_HELP_BROKER
	(void (*)()) DialogCreateHelpIndex, NULL,
#else /* !HAVE_HELP_BROKER */
	DialogHelp, "Help Index",
#endif /* HAVE_HELP_BROKER */
	1, (MenuItem *)NULL },
    NULL,
};

static void
run_browser(intext, ab)
Widget intext;
AddressBrowser *ab;
{
int i , k;

    char *pattern, **hits, **strs;
    char *ldap_pattern[MAX_LDAP_LINES];
    char *addr, *desc, *old_addr, *old_desc;
    char ldap_search_pattern[MAX_LDAP_SEARCH_PATTERN+1];
    int x;
    char *lmax;

    old_addr = AddressBrowserGetAddresses(ab);
    old_desc = AddressBrowserGetDescriptions(ab);
    AddressBrowserSetOutTextValues(ab, NULL, NULL);

    if (!(ab->ldap_def.use_ldap))
      {
        ask_item = intext;
        pattern = XmTextGetString(intext);
        if (!*pattern) {
       	  XtFree(pattern);
	  error(UserErrWarning, catgets(catalog, CAT_MOTIF, 841, "Please supply search pattern."));
	  return;
        }
      }
    else
      {
        ask_item = ab->ldap_def.ldaptext[0];
        k = 0;
        for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
          {
            ldap_pattern[i] = XmTextGetString(ab->ldap_def.ldaptext[i]);
            if (ldap_pattern[i] != 0)
              if (strlen(ldap_pattern[i]) != 0)
                {
                  if (k == 0)
                    {
                      ask_item = ab->ldap_def.ldaptext[i];
                      pattern = ldap_pattern[i];
                    }
                  k++;            
                }
          }
        if (k == 0)
          {
            for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
              if (ldap_pattern[i] != 0)
       	        XtFree(ldap_pattern[i]);
	    error(UserErrWarning, catgets(catalog, CAT_MOTIF, 841, "Please supply search pattern."));
	    return;
          }
      }

    timeout_cursors(TRUE);

    AddressBrowserSetMemTextValue(ab, fetch_cached_addr(pattern));
    XFlush(XtDisplay(AddressBrowserGetMemText(ab)));
    
    if (!(lmax = value_of(VarLookupMax)))
        lmax = "-1";            /* This is defined as "unlimited" */

    if (!(ab->ldap_def.use_ldap))
      x = lookup_run(pattern, NULL, lmax, &hits);
    else
      {
        generate_ldap_search_pattern(ldap_search_pattern,ab,k,ldap_pattern);
        x = lookup_run(ldap_search_pattern, NULL, lmax, &hits);
      }

    if (!(ab->ldap_def.use_ldap))
      XtFree(pattern);
    else
      {
        for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
          if (ldap_pattern[i] != 0)
            XtFree(ldap_pattern[i]);
      }

    switch (x) {
	case -1:
	    error(SysErrWarning,
		catgets( catalog, CAT_MOTIF, 114, "Unable to read from address lookup: %s" ), pattern);
	    /* Fall through */
	case 0: case 5:
	    break;
	case 2: case 3: default:
	    desc = joinv(NULL, hits, "\n");
	    error(UserErrWarning, "%s",
		  desc? desc : catgets(catalog, CAT_MOTIF, 1102, "Unknown lookup error (no output)"));
	    xfree(desc);
	    free_vec(hits);
	    x = -1;
	    break;
	case 4:
	    error(UserErrWarning, catgets(catalog, CAT_MSGS, 356, "No names matched."));
	    free_vec(hits);
	    x = -1;
    }
    if (x >= 0) {
	if (hits) {
	    strs = lookup_split(hits, 0, NULL);
	    desc = joinv(NULL, hits, "\n");
	    if (strs) {
#ifdef C3
		mimeCharSet charset;
		const char *charset_name = value_of(VarLookupCharset), *xdesc;
		charset = GetMimeCharSet(charset_name);
		if (charset != displayCharSet) {
		    size_t x = strlen(desc);
		    xdesc = quick_c3_translate(desc, &x,
					       charset, displayCharSet);
		    if (xdesc != desc)
			ZSTRDUP(desc, xdesc);
		}
#endif /* C3 */
		addr = joinv(NULL, strs, "\n");
		xfree((char *)strs);
	    } else
		addr = desc;
	    free_vec(hits);
	    AddressBrowserSetOutTextValues(ab, addr, desc);
	}

	if (old_addr != old_desc)
	    xfree(old_addr);
	xfree(old_desc);
    }

    timeout_cursors(FALSE);
}

static void
btn_run_browser(btn, ab)
Widget btn;
AddressBrowser *ab;
{
    ask_item = btn;
    run_browser(AddressBrowserGetInText(ab), ab);
}

static void
clear_browser(ab)
AddressBrowser *ab;
{
int i;

    AddressBrowserSetInTextValue(ab, NULL);
    AddressBrowserSetMemTextValue(ab, NULL);
    AddressBrowserSetOutTextValues(ab, NULL, NULL);
    for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
      AddressBrowserSetLDAPTextValue(ab, i, NULL);
}

static void
btn_clear_browser(btn, ab)
Widget btn;
AddressBrowser *ab;
{
int i;
    ask_item = btn;

    AddressBrowserSetInTextValue(ab, NULL);
    AddressBrowserSetMemTextValue(ab, NULL);
    AddressBrowserSetOutTextValues(ab, NULL, NULL);
    for (i=0;i<(ab->ldap_def.n_ldap_patterns);i++)
      AddressBrowserSetLDAPTextValue(ab, i, NULL);
}
 
/*
  This function adds or removes patterns from the ldap dialog.
*/
static void
btn_ldap_browser(btn, ab)
Widget btn;
AddressBrowser *ab;
{
int i;

  (ab->ldap_def.n_ldap_visible)++;
  XtUnmanageChild(XtParent(ab->memtext));
  if (ab->ldap_def.n_ldap_visible > ab->ldap_def.n_ldap_patterns)
    {
      ab->ldap_def.n_ldap_visible = 1;
      for (i=(ab->ldap_def.n_ldap_patterns)-1;i>0;i--)
        XtUnmanageChild(XtParent(ab->ldap_def.ldaptext[i]));
      XtVaSetValues(ab->ldap_def.size_button, XmNlabelString, zmXmStr("More"), NULL);
    }
  if (ab->ldap_def.n_ldap_visible >= ab->ldap_def.n_ldap_patterns)
    XtVaSetValues(ab->ldap_def.size_button, XmNlabelString, zmXmStr("Less"), NULL);
  for (i=1;i<(ab->ldap_def.n_ldap_visible);i++)
    XtManageChild(XtParent(ab->ldap_def.ldaptext[i]));
  XtManageChild(XtParent(ab->memtext));
}

static void
btn_browser_mail(btn, ab)
Widget btn;
AddressBrowser *ab;
{
    int *pos, ct, i, status;
    char buf[HDRSIZ*2], **vec;

    ask_item = btn;
    if (!XmListGetSelectedPos(AddressBrowserGetOutText(ab), &pos, &ct)) {
	error(UserErrWarning, catgets(catalog, CAT_MOTIF, 759, "Please select an address first."));
	return;
    }
    strcpy(buf, "mail ");
    vec = strvec(AddressBrowserGetAddresses(ab), "\n", False);
    for (i = status = 0; i != ct; i++) {
	if (!*(vec[pos[i]-1])) continue;
	if (status++) strcat(buf, ", ");
	strcat(buf, quotezs(vec[pos[i]-1], 0));
    }
    XtFree((char *)pos);
    free_vec(vec);
    status = cmd_line(buf, NULL_GRP);
    if (status) return;
    
    Autodismiss(btn, "browser");
    DismissSetWidget(btn, DismissClose);
}

/* This is a cross between btn_browser_mail() and add_alias() [m_comp2.c]. */
static void
btn_browser_address(btn, which)
Widget btn;
int which;
{
    ZmFrame frame = FrameGetData(btn);
    Compose *compose =
	FrameComposeGetComp(FrameGetFrameOfParent(frame));
    AddressBrowser *ab = (AddressBrowser *) FrameGetClientData(frame);
    int *pos, ct, i, status;
    char buf[HDRSIZ*2], **vec;

    ask_item = btn;
    if (!XmListGetSelectedPos(AddressBrowserGetOutText(ab), &pos, &ct)) {
	error(UserErrWarning,
	    catgets(catalog, CAT_MOTIF, 759, "Please select an address first."));
	return;
    }
    (void) sprintf(buf, "builtin compcmd %%%s %s ",
		    compose->link.l_name, address_headers[which]);
    vec = strvec(AddressBrowserGetAddresses(ab), "\n", False);
    for (i = status = 0; i != ct; i++) {
	if (!*(vec[pos[i]-1])) continue;
	if (status++) strcat(buf, ", ");
	strcat(buf, quotezs(vec[pos[i]-1], 0));
    }
    XtFree((char *)pos);
    free_vec(vec);
    status = cmd_line(buf, NULL_GRP);
    if (status) return;

    Autodismiss(btn, "browser");
    DismissSetFrame(frame, DismissClose);
}

static ActionAreaItem browser_buttons[] = {
    { "Mail",   btn_browser_mail,     NULL    },
    { "Search", btn_run_browser,      NULL    },
    { "Clear",  btn_clear_browser,    NULL    },
    { DONE_STR, PopdownFrameCallback, NULL    },
#ifdef NOT_NOW
    { "Help",   DialogHelp, "Address Browser" },
#endif /* NOT_NOW */
};

static ActionAreaItem browser_buttons_ldap[] = {
    { "Mail",   btn_browser_mail,     NULL    },
    { "Search", btn_run_browser,      NULL    },
    { "Clear",  btn_clear_browser,    NULL    },
    { "Less",   btn_ldap_browser,     NULL    },
    { DONE_STR, PopdownFrameCallback, NULL    },
#ifdef NOT_NOW
    { "Help",   DialogHelp, "Address Browser" },
#endif /* NOT_NOW */
};

static ActionAreaItem compose_browser_buttons[] = {
    { "To",     btn_browser_address,  (caddr_t) TO_ADDR  },
    { "Cc",     btn_browser_address,  (caddr_t) CC_ADDR  },
    { "Bcc",    btn_browser_address,  (caddr_t) BCC_ADDR },
    { "Search", btn_run_browser,      NULL               },
    { "Clear",  btn_clear_browser,    NULL               },
    { DONE_STR, PopdownFrameCallback, NULL               },
};

static ActionAreaItem compose_browser_buttons_ldap[] = {
    { "To",     btn_browser_address,  (caddr_t) TO_ADDR  },
    { "Cc",     btn_browser_address,  (caddr_t) CC_ADDR  },
    { "Bcc",    btn_browser_address,  (caddr_t) BCC_ADDR },
    { "Search", btn_run_browser,      NULL               },
    { "Clear",  btn_clear_browser,    NULL               },
    { "Less",   btn_ldap_browser,     NULL               },
    { DONE_STR, PopdownFrameCallback, NULL               },
};

/*
  This function sets the name on the services selection button to the desired
  ldap service.
*/
void host_menu_set(menuItem, the_choice)
    Widget menuItem;
    int the_choice;
{
    Widget cascade = XmOptionButtonGadget(menuItem);
 
    XtVaSetValues(cascade, XmNlabelString, zmXmStr(ldap_host_names[the_choice]), 0);
    SetNthOptionMenuChoice(menuItem, the_choice);
}


/*
  This function switches ldap services when a service is selected using the
  select ldap service menu button. It changes the ldap resources and 
  reconfigures the patterns and labels in the ldap dialog.
*/
void host_menu_apply(menuItem, the_choice)
    Widget menuItem;
    int the_choice;
{
    AddressBrowser *ab;
    XtVaGetValues(XtParent(menuItem), XmNuserData, &ab, 0);

    if (!load_ldap_resources(ldap_host_names[the_choice],0)  || 
        (ldap_def_read.n_ldap_patterns == 0))
      {
        dummy_up_ldap_resources();
        strcpy(the_current_ldap_host,"host");
        error(SysErrWarning, catgets( catalog, CAT_SHELL, 928, "Cannot locate LDAP host" ));
      }
    else
      strcpy(the_current_ldap_host,ldap_host_names[the_choice]);
    ldap_def_read.use_ldap = True;
    XtVaSetValues(ab->ldap_def.size_button, XmNlabelString, zmXmStr("Less"), NULL);
    copy_ldap_resources(ab);
    set_ldap_labels(ab);
    clear_browser(ab);
    customize_ldap_display(ab);
    remove_unused_ldap_patterns(ab);
}

ZmFrame
CreateAddressBrowser(parent)
Widget parent;
{
    ZmFrame frame;
    AddressBrowser *ab;
    Widget w, widget, main_w, pane, subpane, form, menu_bar, cascade, rc, row_col, label_w;
    Widget ldaptext[MAX_LDAP_LINES], ldaplabel[MAX_LDAP_LINES], intext, outtext, memtext, label;
    Pixmap pix;
    char spaces_buf[MATCHES_LIST_WIDTH+1];
    int i , j;
    char str[16];

    frame = FrameCreate("browse_address_dialog",
	FrameBrowseAddrs,   parent,
	FrameFlags,         FRAME_CANNOT_SHRINK|FRAME_UNMAP_ON_DEL,
	FrameChildClass,    xmMainWindowWidgetClass,
	FrameChild,         &main_w,
#ifdef NOT_NOW
	FrameTitle,         "Address Browser",
#endif /* NOT_NOW */
	FrameIcon,          &addrbook_icon,
	FrameClientData,    ab = zmNew(AddressBrowser),
	/* Don't make this a topLevelShell if it's a child of a Compose */
	(FrameGetType(FrameGetData(parent)) == FrameCompose)?
	    FrameEndArgs : FrameClass,	  	topLevelShellWidgetClass,
	FrameEndArgs);
#ifdef NOT_NOW
    /* Special case to prevent stupid placement */
    XtRemoveCallback(GetTopShell(bboard),
	XmNpopupCallback, place_dialog, hidden_shell);
#endif /* NOT_NOW */

    edit_items[0].callback_data = edit_items[1].callback_data =
    edit_items[3].callback_data = (char *)ab;
    /* create menubar */
    menu_bar = XmCreateMenuBar(main_w, "menu_bar", NULL, 0);
#ifdef NOT_NOW
    (void)BuildPulldownMenu(menu_bar, "file", file_items, NULL);
#endif /* NOT_NOW */
    (void)BuildPulldownMenu(menu_bar, "edit", edit_items, NULL);
#ifdef BROWSER_VIEW_MENU
    (void)BuildPulldownMenu(menu_bar, "view", view_items, NULL);
#endif /* BROWSER_VIEW_MENU */
    (void)BuildPulldownMenu(menu_bar, "options", option_items, NULL);
    cascade = BuildPulldownMenu(menu_bar, "help", help_items, NULL);
    XtVaSetValues(menu_bar, XmNmenuHelpWidget, cascade, NULL);
    XtManageChild(menu_bar);

    pane = XtVaCreateWidget(NULL,
#ifdef SANE_WINDOW
	zmSaneWindowWidgetClass, main_w,
#else /* !SANE_WINDOW */
	xmPanedWindowWidgetClass, main_w,
#endif /* !SANE_WINDOW */
	XmNseparatorOn,	False,
	XmNsashWidth,	1,
	XmNsashHeight,	1,
	NULL);

    form = XtVaCreateManagedWidget(NULL, xmFormWidgetClass, pane,
	XmNskipAdjust,		True,
	XmNmarginWidth,		0,
	NULL);
    subpane = XtVaCreateWidget(NULL,
#ifdef SANE_WINDOW
	zmSaneWindowWidgetClass, form,
#else /* !SANE_WINDOW */
	xmPanedWindowWidgetClass, form,
#endif /* !SANE_WINDOW */
	XmNseparatorOn,		False,
	XmNsashWidth,		1,
	XmNsashHeight,		1,
	XmNmarginWidth,		0,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	NULL);

    widget = XtVaCreateManagedWidget("directions", xmLabelGadgetClass, subpane,
	XmNalignment,        XmALIGNMENT_BEGINNING,
	NULL);

    w = XtVaCreateWidget(NULL, xmLabelGadgetClass, form,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_NONE,
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		subpane,
	XmNalignment,		XmALIGNMENT_END,
	NULL);

    FrameGet(frame, FrameIconPix, &pix, FrameEndArgs);
    XtVaSetValues(w,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		pix,
	XmNuserData,		&addrbook_icon,
	NULL);
    XtManageChild(w);

    copy_ldap_resources(ab);

    row_col = XtVaCreateWidget("ldap_row_col", xmRowColumnWidgetClass, subpane,
        XmNorientation, XmHORIZONTAL,
        XmNmarginHeight, 0,
        NULL);

    label_w = XtVaCreateManagedWidget("ldap_host_label",
        xmLabelGadgetClass, row_col,
        XmNwidth, 55,
        XmNrecomputeSize, False,
        XmNlabelString, zmXmStr("Service:"),
        NULL);

    ab->ldap_def.option_menu = BuildSimpleMenu(row_col, "", ldap_host_names, XmMENU_OPTION, ab, host_menu_apply);
 
#if XmVersion >= 1002
    XtUnmanageChild(XmOptionLabelGadget(ab->ldap_def.option_menu));
#else /* Motif 1.1 */
    XtVaSetValues(XmOptionLabelGadget(ab->ldap_def.option_menu),
                  XmNwidth, 0,
                  XmNheight, 0,
                  XmNmarginWidth, 0,
                  XmNmarginHeight, 0,
                  0);
#endif /* Motif 1.1 */
    XtVaSetValues(ab->ldap_def.option_menu,
                  XmNtopAttachment, XmATTACH_FORM,
                  XmNleftAttachment, XmATTACH_FORM,
#if XmVersion < 1002
                  XmNspacing, 0,
#endif /* Motif 1.1 */
                  0);
    XtVaGetValues(ab->ldap_def.option_menu, XmNsubMenuId, &(ab->ldap_def.option_submenu), 0);
    XtVaSetValues(ab->ldap_def.option_submenu, XmNuserData, ab, 0);
    host_menu_set(ab->ldap_def.option_menu,ab->ldap_def.n_ldap_current_host);
    XtManageChild(ab->ldap_def.option_menu);
    XtManageChild(row_col);

    intext = CreateLabeledText("address_pattern", subpane, NULL, True);
    XtVaSetValues(XtParent(intext), XmNmarginWidth, 0, NULL);

    for (j=0;j<MAX_LDAP_LINES;j++) 
      {
        sprintf(str,"ldap_name_%d",j+1);
        ldaptext[j] = CreateLabeledTextSetWidth(str, subpane, ab->ldap_def.ldap_attribute_name[j], CLT_HORIZ|CLT_REPLACE_NL,LDAP_LABEL_WIDTH,LDAP_PATTERN_WIDTH,&ldaplabel[j]);
        XtVaSetValues(XtParent(ldaptext[j]), XmNmarginWidth, 0, NULL);
      }
    
    memtext = CreateLabeledText("address_recall", subpane, NULL,
	CLT_HORIZ|CLT_REPLACE_NL);
    XtVaSetValues(XtParent(memtext), XmNmarginWidth, 0, NULL);

#ifndef OUT_TEXT
    rc = XtVaCreateManagedWidget("address_matches",
	xmFormWidgetClass, pane,
# ifdef SANE_WINDOW
	XmNallowResize, False,	/* Affects parent saneWindow */
	ZmNextResizable, True,	/* Affects parent saneWindow */ 
# else /* !SANE_WINDOW */
	XmNresizePolicy, XmRESIZE_NONE, 
# endif /* !SANE_WINDOW */
	NULL);
    label = XtVaCreateManagedWidget("address_matches_label",
	xmLabelGadgetClass,	rc,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	NULL);
    {
	Arg args[2];
	XtSetArg(args[0], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); 
	XtSetArg(args[1], XmNselectionPolicy, XmEXTENDED_SELECT);
	outtext = XmCreateScrolledList(rc, "address_matches_list", args, 2);
    }
    XtVaSetValues(XtParent(outtext),
	XmNleftAttachment,	XmATTACH_WIDGET,
	XmNleftWidget,		label,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	NULL);
    for (i = 0; i != MATCHES_LIST_WIDTH; i++) spaces_buf[i] = ' ';
    spaces_buf[i] = 0;
    XmListAddItem(outtext, zmXmStr(spaces_buf), 0);
    XtManageChild(outtext);
#else /* OUT_TEXT */
    outtext = CreateLJustScrolledText("address_matches", pane, NULL, 0);
    XtVaSetValues(outtext,
	XmNeditable,              False,
	XmNcursorPositionVisible, False,
	NULL);
#endif /* OUT_TEXT */

#ifndef BROWSER_VIEW_MENU
    w = CreateToggleBox(pane, False, True, True, toggle_shown, 0,
	    "address_toggles", view_items, 2);

    XtManageChild(w);
    SetPaneMaxAndMin(w); 
#endif /* BROWSER_VIEW_MENU */

    AddressBrowserSetState(ab, SHOW_DESCR);	/* Match the view menu */
    AddressBrowserSetInText(ab, intext);
    AddressBrowserSetOutText(ab, outtext);
    AddressBrowserSetMemText(ab, memtext);
    
    for (j=0;j<MAX_LDAP_LINES;j++) 
      {
        AddressBrowserSetLDAPText(ab,j,ldaptext[j]);
        AddressBrowserSetLDAPLabel(ab,j,ldaplabel[j]);
        XtAddCallback(ab->ldap_def.ldaptext[j], XmNactivateCallback, (XtCallbackProc) run_browser, ab);
      }
    XtAddCallback(intext, XmNactivateCallback, (XtCallbackProc) run_browser, ab);

    {
	Widget action;
	unsigned child;
	
	if (FrameGetType(FrameGetData(parent)) == FrameCompose) {
          if (ab->ldap_def.use_ldap) {
	    compose_browser_buttons_ldap[3].data =
	    compose_browser_buttons_ldap[5].data =
	    compose_browser_buttons_ldap[4].data = (char *)ab;
	    action = CreateActionArea(pane, compose_browser_buttons_ldap,
				      XtNumber(compose_browser_buttons_ldap),
				      "Address Browser");
	    child = XtNumber(compose_browser_buttons_ldap) - 1;
            ab->ldap_def.size_button = GetNthChild(action, 5);
          }
          else {
	    compose_browser_buttons[3].data =
	    compose_browser_buttons[4].data = (char *)ab;
	    action = CreateActionArea(pane, compose_browser_buttons,
				      XtNumber(compose_browser_buttons),
				      "Address Browser");
	    child = XtNumber(compose_browser_buttons) - 1;
          }
	} else {
          if (ab->ldap_def.use_ldap) {
	    browser_buttons_ldap[0].data =
	    browser_buttons_ldap[1].data = 
	    browser_buttons_ldap[3].data = 
	    browser_buttons_ldap[2].data = (char *)ab;
	    action = CreateActionArea(pane, browser_buttons_ldap,
				      XtNumber(browser_buttons_ldap),
				      "Address Browser");
	    child = XtNumber(browser_buttons_ldap) - 1;
            ab->ldap_def.size_button = GetNthChild(action, 3);
          }
          else {
	    browser_buttons[0].data =
	    browser_buttons[1].data = 
	    browser_buttons[2].data = (char *)ab;
	    action = CreateActionArea(pane, browser_buttons,
				      XtNumber(browser_buttons),
				      "Address Browser");
	    child = XtNumber(browser_buttons) - 1;
          }
	}

	FrameSet(frame, FrameDismissButton,
		 GetNthChild(action, child),
		 FrameEndArgs);
    }

    XtManageChild(subpane);
    XtManageChild(form);
    XtManageChild(pane);
    XtManageChild(main_w);
#ifndef OUT_TEXT
    XmListDeleteItem(outtext, zmXmStr(spaces_buf));
#endif /* !OUT_TEXT */

    return frame;
}

ZmFrame
DialogCreateBrowseAddrs(w)
Widget w;
{
    static ZmFrame frame;
    ZmFrame dad;
    char *ldap_service;
    Boolean using_ldap;

    if (!boolean_val(VarLookupService))
	return 0;

    /* If anyone can figure out why it isn't safe to use "w" instead of
     * "ask_item" here, I'd appreciate knowing.  This is copied from
     * DialogCreateCompOptions().
     */

    using_ldap = boolean_val(VarUseLdap);
    ldap_service = value_of(VarLdapService);

/* If using ldap and have not read default ldap resources, read them */
    if (using_ldap)
      {
        if (strlen(the_current_ldap_host) == 0)
          {
            if (!load_ldap_resources(ldap_service,0) || (ldap_def_read.n_ldap_patterns == 0))
              {
                dummy_up_ldap_resources();
                strcpy(the_current_ldap_host,"host");
                error(SysErrWarning, catgets( catalog, CAT_SHELL, 1005, "Cannot locate LDAP host" ));
              }
            else
              strcpy(the_current_ldap_host,ldap_service);
          }
      }

    dad = FrameGetData(ask_item);
    if (FrameGetType(dad) == FrameCompose) {
	Compose *compose = FrameComposeGetComp(dad);

	if (compose) {
	    if (compose->interface->browser) {
                if (((AddressBrowser *)FrameGetClientData(compose->interface->browser))->ldap_def.use_ldap != using_ldap) {
                  ldap_def_read.use_ldap = using_ldap;
                  FrameDestroy(compose->interface->browser,True);
		  compose->interface->browser = CreateAddressBrowser(GetTopShell(ask_item));
                  set_ldap_labels((AddressBrowser *)FrameGetClientData(compose->interface->browser));
                  customize_ldap_display((AddressBrowser *)FrameGetClientData(compose->interface->browser)); 
                }
              }
            else {
                ldap_def_read.use_ldap = using_ldap;
		compose->interface->browser = CreateAddressBrowser(GetTopShell(ask_item));
                set_ldap_labels((AddressBrowser *)FrameGetClientData(compose->interface->browser));
                customize_ldap_display((AddressBrowser *)FrameGetClientData(compose->interface->browser)); 
            }
	    FramePopup(compose->interface->browser);
            remove_unused_ldap_patterns((AddressBrowser *)FrameGetClientData(compose->interface->browser)); 
	} else {
	    error(UserErrWarning,
		catgets(catalog, CAT_MOTIF, 906, "No active composition in this window"));
	}
    } else {
        if (frame)
          {
            if (((AddressBrowser *)FrameGetClientData(frame))->ldap_def.use_ldap != using_ldap)
              { 
                ldap_def_read.use_ldap = using_ldap;
                FrameDestroy(frame,True);
                frame = CreateAddressBrowser(hidden_shell);
                set_ldap_labels((AddressBrowser *)FrameGetClientData(frame));
                customize_ldap_display((AddressBrowser *)FrameGetClientData(frame)); 
              }
          }
	else 
          {
            ldap_def_read.use_ldap = using_ldap;
	    frame = CreateAddressBrowser(hidden_shell);
            set_ldap_labels((AddressBrowser *)FrameGetClientData(frame));
            customize_ldap_display((AddressBrowser *)FrameGetClientData(frame)); 
          }
	FramePopup(frame);
        remove_unused_ldap_patterns((AddressBrowser *)FrameGetClientData(frame)); 
    }

    /* There are multiple copies of this dialog, so don't cache any */
    return 0;
}

#endif /* DSERV */
