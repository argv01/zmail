/*
 * $RCSfile: choose1.c,v $
 * $Revision: 2.48 $
 * $Date: 1998/12/07 23:56:32 $
 * $Author: schaefer $
 */

#include <ctype.h>

#include <spoor.h>
#include <choose1.h>

#include <zmlite.h>

#include <zmlutil.h>
#include <filelist.h>

#include <spoor/listv.h>
#include <spoor/list.h>
#include <spoor/buttonv.h>
#include <spoor/button.h>
#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/popupv.h>
#include <spoor/toggle.h>

#include "catalog.h"

#ifndef lint
static const char chooseone_rcsid[] =
    "$Id: choose1.c,v 2.48 1998/12/07 23:56:32 schaefer Exp $";
#endif /* lint */

struct spWclass *chooseone_class = 0;

int m_chooseone_result;
int m_chooseone_setDirectory;

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define Split spSplitview_Create
#define Wrap spWrapview_Create

#if defined( IMAP )
static struct chooseone *chooseself;	/* XXX hack */

/* XXX */

static int mk_dir = 0;

int
SetMakeDir( value )
int	value;
{
	mk_dir = value;
}

int
GetMakeDir()
{
	return( mk_dir );
}

#endif

static int
check(self)
    struct chooseone *self;
{
    int e = 0, s = 0;
    struct stat statbuf;

    if (self->choices
	&& (self->flags & PB_MUST_MATCH)
	&& !(self->flags & PB_MSG_LIST)) {
	int i;
	struct dynstr d;

	e = 1;
	dynstr_Init(&d);
	TRY {
	    for (i = 0; e && (i < spSend_i(spView_observed(self->choices),
					   m_spList_length)); ++i) {
		dynstr_Set(&d, "");
		if (spSend_i(spView_observed(self->choices),
			     m_spList_getNthItem, i, &d) >= 0) {
		    if (!strcmp(dynstr_Str(&(self->chosen)),
				dynstr_Str(&d)))
			e = 0;
		}
	    }
	} FINALLY {
	    dynstr_Destroy(&d);
	} ENDTRY;
	if (e) {
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 69, "You must select one of the choices in the list"));
	}
    } else if (self->flags & (PB_FILE_BOX | PB_FILE_OPTION)) {
#if defined( IMAP )	/* XXX major hack - no time to look at fully */
	if ( !(using_imap && UseIMAP()) ) {
#endif
	if (self->flags & PB_MUST_EXIST) {
	    TRY {
		estat(dynstr_Str(&(self->chosen)),
		      &statbuf, "chooseone/check");
		s = 1;
	    } EXCEPT(ANY) {
		error(UserErrWarning, "%s: %s", dynstr_Str(&(self->chosen)),
		      (char *) except_GetRaisedException());
		s = -1;
		e = 1;
	    } ENDTRY;
	}
	if (self->flags & PB_NOT_A_DIR) {
	    switch (s) {
	      case -1:		/* doesn't exist, so not a dir */
		break;
	      case 1:
		if ((e = ((statbuf.st_mode & S_IFMT) == S_IFDIR))
		    && self->mkdir_toggle)
		    spSend(self, m_chooseone_setDirectory,
			   dynstr_Str(&(self->chosen)));
		break;
	      case 0:
		TRY {
		    estat(dynstr_Str(&(self->chosen)),
			  &statbuf, "chooseone/check");
		    e = ((statbuf.st_mode & S_IFMT) == S_IFDIR);
		} EXCEPT(strerror(ENOENT)) {
		    /* Do nothing */
		} EXCEPT(ANY) {
		    error(UserErrWarning, "%s: %s",
			  dynstr_Str(&(self->chosen)),
			  (char *) except_GetRaisedException());
		    e = 1;
		} ENDTRY;
	    }
	}
#if defined( IMAP )
	}
#endif
	if (!e) {
	    if (self->dir_folder) {
		if (spToggle_state(self->mkdir_toggle)) {
#if defined( IMAP )
		    if ( using_imap && UseIMAP() ) {
			SetMakeDir( 1 );	/* work done in zmlite.c */
		    }
		    else {
#endif
		    e = 1;
		    TRY {
			emkdir(dynstr_Str(&(self->chosen)),
			       0700, catgets(catalog, CAT_LITE, 70, "Create Folder"));
			spSend(self, m_chooseone_setDirectory,
			       dynstr_Str(&(self->chosen)));
		    } EXCEPT(ANY) {
			error(SysErrWarning,
			      catgets(catalog, CAT_LITE, 71, "Could not create directory \"%s\""),
			      dynstr_Str(&(self->chosen)));
		    } ENDTRY;
#if defined( IMAP )
		    }
#endif
		}
#if defined( IMAP )
		else {
			if ( using_imap && UseIMAP() )	
				SetMakeDir( 0 );
		}
#endif
	    } else if (self->newname) {
		if (e = (spSend_i(spView_observed(self->newname),
				  m_spText_length) == 0))
		    error(UserErrWarning, catgets(catalog, CAT_LITE, 72, "Empty new name"));
	    }
	}
    }
    return (e);
}

static void
okButton(b, self)
    struct spButton *b;
    struct chooseone *self;
{
    if (self->input) {
	if (spView_getWclass((struct spView *) self->input) ==
	    spwc_Commandfield) {
	    spSend(self->input, m_spView_invokeInteraction,
		   "command-accept", 0, 0, 0);
	}
	dynstr_Set(&(self->chosen), "");
	spSend(spView_observed(self->input), m_spText_appendToDynstr,
	       &(self->chosen), 0, -1);
    } else if (self->files) {
	dynstr_Set(&(self->chosen), "");
	spSend(spView_observed(filelist_choice(self->files)),
	       m_spText_appendToDynstr, &(self->chosen), 0, -1);
	dynstr_Set(&(self->chosen),
		   (char *) spSend_p(self->files, m_filelist_fullpath,
				     dynstr_Str(&(self->chosen))));
    } else if (self->choices) {
	struct dynstr d;
	int r;

	if (((r = spListv_lastclick(self->choices)) < 0)
	    || !intset_Contains(spListv_selections(self->choices), r)) {
	    spSend(self, m_dialog_deactivate, dialog_Cancel);
	    return;
	}
	dynstr_Init(&d);
	spSend(spView_observed(self->choices), m_spList_getNthItem, r, &d);
	if (self->flags & PB_MSG_LIST) {
	    char buf[16];
	    int m;

	    sscanf(dynstr_Str(&d), "%d", &m);
	    sprintf(buf, "%d", m);
	    dynstr_Set(&(self->chosen), buf);
	} else {
	    dynstr_Set(&(self->chosen), dynstr_Str(&d));
	}
	dynstr_Destroy(&d);
    }
    if (!check(self)) {
	spSend(self, m_dialog_deactivate, chooseone_Accept);
	return;
    }
}

static void
searchButton(b, self)
    struct spButton *b;
    struct chooseone *self;
{

    if (self->flags & PB_FILE_OPTION) {
	spSend(self, m_dialog_deactivate, chooseone_FileOptionSearch);
    } else {
	struct stat statbuf;
	struct filelist *f;
	struct spText *t;
	char buf[1 + MAXPATHLEN];
	int len;

	f = self->files;
	t = (struct spText *) spView_observed(filelist_choice(f));
	len = spSend_i(t, m_spText_length);
	spSend(t, m_spText_substring, 0, len, buf);
	buf[len] = '\0';
	TRY {
	    TRY {
		estat(buf, &statbuf, "chooseone/actionActivate");
#ifdef HAVE_LSTAT
	    } EXCEPT(strerror(ENOENT)) {
		elstat(buf, &statbuf, "chooseone/actionActivate");
#endif /* HAVE_LSTAT */
	    } ENDTRY;
	    if ((statbuf.st_mode & S_IFMT) != S_IFDIR) {
		error(UserErrWarning, catgets(catalog, CAT_LITE, 73, "%s:  Not a directory"), buf);
	    } else {
		spSend(self->files, m_filelist_setDirectory, buf);
	    }
	} EXCEPT(ANY) {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 74, "Could not search %s"), buf);
	} ENDTRY;
    }
}

static void
omitButton(b, self)
    struct spButton *b;
    struct chooseone *self;
{
    spSend(self, m_dialog_deactivate, chooseone_Omit);
}

static void
retryButton(b, self)
    struct spButton *b;
    struct chooseone *self;
{
    if (self->input) {
	dynstr_Set(&(self->chosen), "");
	spSend(spView_observed(self->input), m_spText_appendToDynstr,
	       &(self->chosen), 0, -1);
    } else if (self->files) {
	dynstr_Set(&(self->chosen), "");
	spSend(spView_observed(filelist_choice(self->files)),
	       m_spText_appendToDynstr, 0, -1, &(self->chosen));
    } else if (self->choices) {
	struct dynstr d;
	int r;

	if (((r = spListv_lastclick(self->choices)) < 0)
	    || !intset_Contains(spListv_selections(self->choices), r)) {
	    spSend(self, m_dialog_deactivate, dialog_Cancel);
	    return;
	}
	dynstr_Init(&d);
	spSend(spView_observed(self->choices), m_spList_getNthItem, r, &d);
	if (self->flags & PB_MSG_LIST) {
	    char buf[16];
	    int m;

	    sscanf(dynstr_Str(&d), "%d", &m);
	    sprintf(buf, "%d", m);
	    dynstr_Set(&(self->chosen), buf);
	} else {
	    dynstr_Set(&(self->chosen), dynstr_Str(&d));
	}
	dynstr_Destroy(&d);
    }
    if (!check(self)) {
	spSend(self, m_dialog_deactivate, chooseone_Retry);
	return;
    }
}

static void
cancelButton(b, self)
    struct spButton *b;
    struct chooseone *self;
{
    spSend(self, m_dialog_deactivate, dialog_Cancel);
}

static void
chooseone_initialize(self)
    struct chooseone *self;
{
    ZmlSetInstanceName(self, "ask", self);

    self->flags =   (unsigned long) 0;
    self->files =   (struct filelist *) 0;
    self->choices = (struct spListv *) 0;
    self->input =   (struct spCmdline *) 0;
    self->dir_folder = 0;
#if defined( IMAP )
    self->imap_local = 0;
    self->imap_toggle = 0;
#endif
    self->rwro = 0;
    self->newname = 0;
    self->rw_toggle = 0;
    self->mkdir_toggle = 0;

    dynstr_Init(&(self->chosen));

    self->query = spTextview_NEW();
    spSend(self->query, m_spView_setObserved, spText_NEW());
    spSend(spView_observed(self->query), m_spText_setReadOnly, 1);
}

static void
chooseone_finalize(self)
    struct chooseone *self;
{
    struct spView *v = dialog_view(self);

    spSend(self, m_dialog_setView, 0);
    if (v) {
	KillSplitviewsAndWrapviews(v);
    }
    if (self->dir_folder) {
	spSend(self->dir_folder, m_spView_destroyObserved);
	spoor_DestroyInstance(self->dir_folder);
    }
#if defined( IMAP )
    if (self->imap_local) {
	spSend(self->imap_local, m_spView_destroyObserved);
	spoor_DestroyInstance(self->imap_local);
    }
#endif
    if (self->rwro) {
	spSend(self->rwro, m_spView_destroyObserved);
	spoor_DestroyInstance(self->rwro);
    }
    if (self->newname) {
	spSend(self->newname, m_spView_destroyObserved);
	spoor_DestroyInstance(self->newname);
    }
    if (self->query) {
	spSend(self->query, m_spView_destroyObserved);
	spoor_DestroyInstance(self->query);
    }
    if (self->files) {
	spoor_DestroyInstance(self->files);
    }
    if (self->choices) {
	struct spList *l = (struct spList *) spView_observed(self->choices);

	spoor_DestroyInstance(self->choices);
	spoor_DestroyInstance(l);
    }
    if (self->input) {
	spSend(self->input, m_spView_destroyObserved);
	spoor_DestroyInstance(self->input);
    }
    dynstr_Destroy(&(self->chosen));
}

static void
filelistActivate(self, str)
    struct filelist *self;
    char *str;
{
    struct chooseone *co = (struct chooseone *) filelist_obj(self);

    dynstr_Set(&(co->chosen),
	       (char *) spSend_p(self, m_filelist_fullpath, str));
    if (co->newname) {
	spSend(co->newname, m_spView_wantFocus, co->newname);
    } else if (!check(co)) {
	spSend(co, m_dialog_deactivate, chooseone_Accept);
	return;
    }
}

static void
choicesCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct chooseone *co = (struct chooseone *) spView_callbackData(self);
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(self), m_spList_getNthItem, which, &d);

    if (co->flags & PB_MSG_LIST) {
	char buf[10];
	int m;

	sscanf(dynstr_Str(&d), "%d", &m);
	sprintf(buf, "%d", m);
	dynstr_Set(&(co->chosen), buf);
    } else {
	if (co->input) {
	    spSend(spView_observed(co->input), m_spText_clear);
	    spSend(spView_observed(co->input), m_spText_insert,
		   0, -1, dynstr_Str(&d), spText_mBefore);
	}
	dynstr_Set(&(co->chosen), dynstr_Str(&d));
    }
    dynstr_Destroy(&d);
    if (clicktype == spListv_doubleclick)
	if (!check(co)) {
	    spSend(co, m_dialog_deactivate, chooseone_Accept);
	    return;
	}
}

static void
inputActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct chooseone *co = (struct chooseone *) spCmdline_obj(self);

    if (co->flags & PB_TRY_AGAIN) {
	spSend(spView_observed(co->input), m_spText_appendToDynstr,
	       &(co->chosen), 0, -1);
	if (!check(co)) {
	    spSend(co, m_dialog_deactivate, chooseone_Retry);
	    return;
	}
    } else {
	dynstr_Set(&(co->chosen), str);
	if (!check(co)) {
	    spSend(co, m_dialog_deactivate, chooseone_Accept);
	    return;
	}
    }
}

static void
chooseone_desiredSize(self, arg)
    struct chooseone *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int screenw = 80, screenh = 24;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(chooseone_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    if (*besth && (self->files || self->choices))
	*besth += 2;
    if ((!*besth) || (*besth > (screenh - 8)))
	*besth = MAX(screenh - 8, *minh);
    if ((*bestw < (screenw - 14)) && (self->files || self->input))
	*bestw = screenw - 14;
}

static void
chooseone_setDirectory(self, arg)
    struct chooseone *self;
    spArgList_t arg;
{
    char *dir;

    dir = spArg(arg, char *);
    if (self->files)
	spSend(self->files, m_filelist_setDirectory, dir);
}

static void
chooseone_result(self, arg)
    struct chooseone *self;
    spArgList_t arg;
{
    struct dynstr *result = spArg(arg, struct dynstr *);

    dynstr_Set(result, "");
    if (self->input) {
	spSend(spView_observed(self->input), m_spText_appendToDynstr,
	       result, 0, -1);
    } else if (self->files) {
	dynstr_Set(result, dynstr_Str(&(self->chosen)));
    } else if (self->choices) {
	int r;

	if (((r = spListv_lastclick(self->choices)) >= 0)
	    && intset_Contains(spListv_selections(self->choices), r)) {
	    spSend(spView_observed(self->choices), m_spList_getNthItem,
		   r, result);
	    if (self->flags & PB_MSG_LIST) {
		char buf[16];
		int m;

		sscanf(dynstr_Str(result), "%d", &m);
		sprintf(buf, "%d", m);
		dynstr_Set(result, buf);
	    }
	}
    }
}

struct spWidgetInfo *spwc_Ask = 0;
struct spWidgetInfo *spwc_MsgAsk = 0;
struct spWidgetInfo *spwc_ListAsk = 0;
struct spWidgetInfo *spwc_FileAsk = 0;
struct spWidgetInfo *spwc_InputAsk = 0;
struct spWidgetInfo *spwc_NewfolderAsk = 0;
struct spWidgetInfo *spwc_ImapLocalAsk = 0;
struct spWidgetInfo *spwc_AddfolderAsk = 0;
struct spWidgetInfo *spwc_RenamefolderAsk = 0;
struct spWidgetInfo *spwc_CommandAsk = 0;
struct spWidgetInfo *spwc_AddressAsk = 0;

static void
chooseone_activate(self, arg)
    struct chooseone *self;
    spArgList_t arg;
{
    spSuper(chooseone_class, self, m_dialog_activate);
    if (self->files) {
	spSend(filelist_list(self->files), m_spView_wantFocus,
	       filelist_list(self->files));
    }
}

void
chooseone_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (chooseone_class)
	return;
    chooseone_class =
	spWclass_Create("chooseone", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct chooseone)),
			chooseone_initialize,
			chooseone_finalize,
			spwc_Ask = spWidget_Create("Ask",
						   spwc_Popup));
    m_chooseone_result = spoor_AddMethod(chooseone_class,
					 "result",
					 catgets(catalog, CAT_LITE, 75, "get result"),
					 chooseone_result);
    m_chooseone_setDirectory =
	spoor_AddMethod(chooseone_class, "setDirectory",
			NULL,
			chooseone_setDirectory);
    spoor_AddOverride(chooseone_class, m_spView_desiredSize, NULL,
		      chooseone_desiredSize);
    spoor_AddOverride(chooseone_class, m_dialog_activate, 0,
		      chooseone_activate);

    spwc_MsgAsk = spWidget_Create("MsgAsk", spwc_Ask);
    spwc_ListAsk = spWidget_Create("ListAsk", spwc_Ask);
    spwc_FileAsk = spWidget_Create("FileAsk", spwc_Ask);
    spwc_InputAsk = spWidget_Create("InputAsk", spwc_Ask);
    spwc_NewfolderAsk = spWidget_Create("NewfolderAsk", spwc_FileAsk);
    spwc_ImapLocalAsk = spWidget_Create("ImapLocalAsk", spwc_FileAsk);
    spwc_AddfolderAsk = spWidget_Create("AddfolderAsk", spwc_FileAsk);
    spwc_RenamefolderAsk = spWidget_Create("RenamefolderAsk", spwc_FileAsk);
    spwc_CommandAsk = spWidget_Create("CommandAsk", spwc_InputAsk);
    spwc_AddressAsk = spWidget_Create("AddressAsk", spwc_InputAsk);

    spButtonv_InitializeClass();
    spButton_InitializeClass();
    spCmdline_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spPopupView_InitializeClass();
    filelist_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
    spToggle_InitializeClass();
}

static void
newname_cb(c, str)
    struct spCmdline *c;
    char *str;
{
    struct chooseone *self = (struct chooseone *) spCmdline_obj(c);

    spSend(dialog_actionArea(self), m_spView_wantFocus,
	   dialog_actionArea(self));
}

void
imap_toggle_cb_hack(self, which)
    struct spButtonv *self;
    int which;
{
    int i, update = 0;

    for (i = 0; i < spButtonv_length(self); ++i) {
        if (i == which) {
            if (!spToggle_state(spButtonv_button(self, i))) {
                update = 1;
                spToggle_state(spButtonv_button(self, i)) = 1;
		if ( i == 0 )
			SetUseIMAP( 1 );
		else
			SetUseIMAP( 0 );
            }
        } else {
            if (spToggle_state(spButtonv_button(self, i))) {
                update = 1;
                spToggle_state(spButtonv_button(self, i)) = 0;
            }
        }
    }
    if (update) {
#if defined( IMAP )
    if ( UseIMAP() )
        spSend(chooseself->files, m_filelist_setDirectory, chooseself->files->imapdir);
    else
        spSend(chooseself->files, m_filelist_setDirectory, chooseself->files->dir);
#endif

    }
}

struct chooseone *
chooseone_Create(query, dflt, flags, choices, n_choices, flags2)
    char *query, *dflt;
    unsigned long flags;
    char **choices;
    int n_choices;
    unsigned long flags2;
{
    struct chooseone *result = chooseone_NEW();
    struct spSplitview *split;
    char *prompt;
    int i, wc = 0;
#if defined( IMAP )
    int lcl_using_imap = 0;
#endif

#if defined( IMAP )
    chooseself = result;
    SetUseIMAP( 0 );
    if ( !(flags & PB_NOIMAP) && !InWriteToFile() )
	lcl_using_imap = 1;
#endif
    result->flags = flags;
    spSend(spView_observed(result->query), m_spText_clear);
    spSend(spView_observed(result->query), m_spText_insert, 0,
	   -1, query, spText_mNeutral);

    if (flags & PB_FILE_BOX) {
	result->files = filelist_NEW();
	filelist_fn(result->files) = filelistActivate;
	filelist_obj(result->files) = (struct spoor *) result;
	filelist_flags(result->files) = flags;
	if (dflt) {
	    spSend(result->files, m_filelist_setDefault, dflt);
	}
    } else {
	if ((n_choices > 1)
	    || (flags & PB_MSG_LIST)
	    || ((n_choices == 1)
		&& dflt
		&& strcmp(choices[0], dflt))) {
	    spSend(result->choices = spListv_NEW(), m_spView_setObserved,
		   spList_NEW());
	    spView_callbackData(result->choices) = (struct spoor *) result;
	    spListv_callback(result->choices) = choicesCallback;
	    spListv_okclicks(result->choices) = ((1 << spListv_click)
						 | (1 << spListv_doubleclick));
	    if (flags & PB_MSG_LIST) {
		for (i = 0;
		     i < ((msg_folder *) spSend_p(result,
						  m_dialog_folder))->mf_count;
		     ++i) {
		    if (msg_is_in_group(((msg_group *)
					 spSend_p(result, m_dialog_mgroup)),
					i)) {
			spSend(spView_observed(result->choices),
			       m_spList_append, compose_hdr(i));
		    }
		}
	    } else if (dflt) {
		char clickme[10];

		clickme[0] = '\0';
		for (i = 0; i < n_choices; ++i) {
		    spSend(spView_observed(result->choices),
			   m_spList_append, choices[i]);
		    if (!strcmp(choices[i], dflt))
			sprintf(clickme, "%d", 1 + i);
		}
		if (clickme[0])
		    spSend(result->choices, m_spView_invokeInteraction,
			   "list-click-line", result, clickme, 0);
	    }
	}
	if (!((flags & PB_MSG_LIST) || (flags & PB_NO_TEXT))) {
	    spSend(result->input = spCmdline_NEW(), m_spView_setObserved,
		   spText_NEW());
	    spCmdline_fn(result->input) = inputActivate;
	    spCmdline_obj(result->input) = (struct spoor *) result;
	    if (flags & PB_NO_ECHO)
	      spTextview_echochar(result->input) = '*';
	    if (dflt) {
		spSend(spView_observed(result->input), m_spText_insert,
		       0, -1, dflt, spText_mNeutral);
	    } else if (n_choices == 1) {
		spSend(spView_observed(result->input), m_spText_insert,
		       0, -1, choices[0], spText_mBefore);
	    }
	}
    }

    dialog_MUNGE(result) {
	spSend(result, m_dialog_setActionArea,
	       ActionArea(result,
			  catgets(catalog, CAT_LITE, 64, "Ok"), okButton,
			  0));
	ZmlSetInstanceName(dialog_actionArea(result), "ask-aa", result);

	if (flags & (PB_FILE_OPTION | PB_FILE_BOX)) {
	    spSend(result, m_dialog_insertActionAreaItem,
		   spButton_Create(catgets(catalog, CAT_LITE, 25, "Search"),
				   searchButton, result), -1, 0);
	}
	if (flags & PB_TRY_AGAIN) {
	    spSend(result, m_dialog_insertActionAreaItem,
		   spButton_Create(catgets(catalog, CAT_LITE, 78, "Retry"),
				   retryButton, result), -1, 0);
	    spSend(result, m_dialog_insertActionAreaItem,
		   spButton_Create(catgets(catalog, CAT_LITE, 79, "Omit"),
				   omitButton, result), -1, 0);
	}

	spSend(result, m_dialog_insertActionAreaItem,
	       spButton_Create(catgets(catalog, CAT_LITE, 65, "Cancel"),
			       cancelButton, result), -1, 0);

	spSend(result, m_dialog_setView, split = spSplitview_NEW());
	spWrapview_highlightp(result) = 1;
	spWrapview_boxed(result) = 1;

	if (flags & PB_FILE_BOX) {
	    spSend(result, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 81, "File Finder"),
		   spWrapview_top);
	    prompt = catgets(catalog, CAT_LITE, 82, "File: ");
	} else if (flags & PB_FILE_OPTION) {
	    spSend(result, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 83, "File Selection"),
		   spWrapview_top);
	    prompt = catgets(catalog, CAT_LITE, 82, "File: ");
	} else if (flags & PB_MSG_LIST) {
	    spSend(result, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 85, "Message List Selection"),
		   spWrapview_top);
	    prompt = catgets(catalog, CAT_LITE, 86, "Message: ");
	} else if (flags & PB_TRY_AGAIN) {
	    spSend(result, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 87, "Selection"), spWrapview_top);
	    prompt = catgets(catalog, CAT_LITE, 88, "Selection: ");
	} else {
	    spSend(result, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 89, "Input"), spWrapview_top);
	    prompt = catgets(catalog, CAT_LITE, 90, "Input: ");
	}
	if (result->files || result->choices) {
	    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

	    spSend(result->query, m_spView_desiredSize,
		   &minh, &minw, &maxh, &maxw, &besth, &bestw);
	    if (result->input) {
		split = SplitAdd(split, result->query, besth, 0, 0,
				 spSplitview_topBottom, spSplitview_boxed,
				 spSplitview_SEPARATE);
		spSend(split, m_spSplitview_setup,
		       (result->files ?
			(struct spView *) result->files :
			(struct spView *) result->choices),
		       Wrap(result->input, 0, 0, prompt, 0, 0, 0, 0),
		       1, 1, 0,
		       spSplitview_topBottom, spSplitview_boxed,
		       spSplitview_SEPARATE);
	    } else {
#if defined( IMAP )
		struct spView *bottom , *near_bottom;
#else
		struct spView *bottom;
#endif
		if (flags2 & chooseone_CreateFolder) {
		    result->dir_folder =
			spButtonv_Create(spButtonv_horizontal,
					 (result->mkdir_toggle =
					  spToggle_Create(catgets(catalog, CAT_LITE, 91, "Create Directory"),
							  0, 0, 0)),
					 spToggle_Create(catgets(catalog, CAT_LITE, 92, "Create Mail Folder"),
							 0, 0, 1),
					 0);
		    spButtonv_callback(result->dir_folder) =
			spButtonv_radioButtonHack;
		    spButtonv_toggleStyle(result->dir_folder) = spButtonv_checkbox;
#if defined( IMAP )
		    if ( lcl_using_imap && using_imap ) {
		    result->imap_local =
			spButtonv_Create(spButtonv_horizontal,
					 (result->imap_toggle =
					  spToggle_Create(catgets(catalog, CAT_LITE, 96, "IMAP"),
							 0, 0, UseIMAP())),
					 spToggle_Create(catgets(catalog, CAT_LITE, 97, "Local"),
							 0, 0, !UseIMAP()),
					 0);

		    spButtonv_toggleStyle(result->imap_local) = spButtonv_checkbox;

		    spButtonv_callback(result->imap_local) =
			imap_toggle_cb_hack;

		    near_bottom = ((struct spView *)
			      Split(result->files, result->dir_folder,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		    bottom = ((struct spView *)
			      Split(near_bottom, result->imap_local,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		    }
		    else
#endif
		    bottom = ((struct spView *)
			      Split(result->files, result->dir_folder,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		} else if (flags2 & chooseone_OpenFolder) {
		    result->rwro =
			spButtonv_Create(spButtonv_horizontal,
					 (result->rw_toggle =
					  spToggle_Create(catgets(catalog, CAT_LITE, 93, "Read/Write"),
							  0, 0, 1)),
					 spToggle_Create(catgets(catalog, CAT_LITE, 94, "Read Only"),
							 0, 0, 0),
					 0);
		    spButtonv_callback(result->rwro) = spButtonv_radioButtonHack;
		    spButtonv_toggleStyle(result->rwro) = spButtonv_checkbox;
#if defined( IMAP ) 
		    if ( lcl_using_imap && using_imap ) {
		    result->imap_local =
			spButtonv_Create(spButtonv_horizontal,
					 (result->imap_toggle =
					  spToggle_Create(catgets(catalog, CAT_LITE, 96, "IMAP"),
							  0, 0, UseIMAP())),
					 spToggle_Create(catgets(catalog, CAT_LITE, 97, "Local"),
							 0, 0, !UseIMAP()),
					 0);
		    spButtonv_callback(result->imap_local) =
			imap_toggle_cb_hack;

		    spButtonv_toggleStyle(result->imap_local) = spButtonv_checkbox;
		    near_bottom = ((struct spView *)
			      Split(result->files, result->rwro,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		    bottom = ((struct spView *)
			      Split(near_bottom, result->imap_local,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		    }
		    else
#endif
		    bottom = ((struct spView *)
			      Split(result->files, result->rwro,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		} else if (flags2 & chooseone_RenameFolder) {
		    result->newname = spCmdline_Create(newname_cb);
		    spCmdline_obj(result->newname) = (struct spoor *) result;
#if defined( IMAP )
		    if ( lcl_using_imap && using_imap ) {
		    result->imap_local =
			spButtonv_Create(spButtonv_horizontal,
					 (result->imap_toggle =
					  spToggle_Create(catgets(catalog, CAT_LITE, 96, "IMAP"),
							  0, 0, UseIMAP())),
					 spToggle_Create(catgets(catalog, CAT_LITE, 97, "Local"),
							 0, 0, !UseIMAP()),
					 0);
		    spButtonv_callback(result->imap_local) =
			imap_toggle_cb_hack;

		    spButtonv_toggleStyle(result->imap_local) = spButtonv_checkbox;
		    near_bottom = ((struct spView *)
			      Split(result->files, result->imap_local,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		    bottom = ((struct spView *)
			      Split(near_bottom,
				    Wrap(result->newname,
					 0, 0, catgets(catalog, CAT_LITE, 95, "New name: "), 0,
					 0, 0, 0),
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_plain, 0));
		    }
		    else
#endif
		    bottom = ((struct spView *)
			      Split(result->files,
				    Wrap(result->newname,
					 0, 0, catgets(catalog, CAT_LITE, 95, "New name: "), 0,
					 0, 0, 0),
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_plain, 0));
		} else {
#if defined( IMAP )
		    if ( lcl_using_imap && using_imap ) {
		    result->imap_local =
			spButtonv_Create(spButtonv_horizontal,
					 (result->imap_toggle =
					  spToggle_Create(catgets(catalog, CAT_LITE, 96, "IMAP"),
							  0, 0, UseIMAP())),
					 spToggle_Create(catgets(catalog, CAT_LITE, 97, "Local"),
							 0, 0, !UseIMAP()),
					 0);

		    spButtonv_callback(result->imap_local) =
			imap_toggle_cb_hack;

		    spButtonv_toggleStyle(result->imap_local) = spButtonv_checkbox;
		    bottom = ((struct spView *)
			      Split(result->files, result->imap_local,
				    1, 1, 0,
				    spSplitview_topBottom,
				    spSplitview_boxed,
				    spSplitview_SEPARATE));
		    }
		    else
#endif
		    bottom = (result->files ?
			      (struct spView *) result->files :
			      (struct spView *) result->choices);
		}

		spSend(split, m_spSplitview_setup,
		       result->query, bottom,
		       (besth ? besth : minh), 0, 0,
		       spSplitview_topBottom, spSplitview_boxed,
		       spSplitview_SEPARATE);
	    }
	} else {
	    spSend(split, m_spSplitview_setup, result->query, result->input,
		   1, 1, 0, spSplitview_topBottom, spSplitview_boxed,
		   spSplitview_SEPARATE);
	}
	spSend(result, m_dialog_clearFocusViews);
	if (result->files) {
	    spSend(result, m_dialog_addFocusView, filelist_list(result->files));
	    spSend(result, m_dialog_addFocusView, filelist_choice(result->files));
	    ZmlSetInstanceName(filelist_list(result->files), "ask-fl", result);
	    ZmlSetInstanceName(filelist_choice(result->files),
			       "ask-filename-field", result);
	    spSend(result, m_spView_setWclass, spwc_FileAsk);
	    wc = 1;
	}
	if (result->dir_folder) {
	    spSend(result, m_dialog_addFocusView, result->dir_folder);
	    ZmlSetInstanceName(result->dir_folder, "ask-mkdir-mkfolder-rg",
			       result);
	    spSend(result->dir_folder, m_spView_setWclass, spwc_Radiogroup);
	    spSend(result, m_spView_setWclass, spwc_NewfolderAsk);
	}
#if defined( IMAP )
	if (result->imap_local) {
	    spSend(result, m_dialog_addFocusView, result->imap_local);
	    ZmlSetInstanceName(result->imap_local, "ask-imap-local",
			       result);
	    spSend(result->imap_local, m_spView_setWclass, spwc_Radiogroup);
	    spSend(result, m_spView_setWclass, spwc_ImapLocalAsk);
	}
#endif
	if (result->rwro) {
	    spSend(result, m_dialog_addFocusView, result->rwro);
	    ZmlSetInstanceName(result->rwro, "ask-read-onlyness-rg", result);
	    spSend(result->rwro, m_spView_setWclass, spwc_Radiogroup);
	    spSend(result, m_spView_setWclass, spwc_AddfolderAsk);
	}
	if (result->newname) {
	    spSend(result, m_dialog_addFocusView, result->newname);
	    ZmlSetInstanceName(result->newname, "ask-newname-field", result);
	    spSend(result, m_spView_setWclass, spwc_RenamefolderAsk);
	}
	if (result->choices) {
	    spSend(result, m_dialog_addFocusView, result->choices);
	    if (flags & PB_MSG_LIST) {
		ZmlSetInstanceName(result->choices, "ask-summaries", result);
		spSend(result->choices, m_spView_setWclass,
		       spwc_MessageSummaries);
		if (!wc)
		    spSend(result, m_spView_setWclass, spwc_MsgAsk);
	    } else {
		ZmlSetInstanceName(result->choices, "ask-choices", result);
		if (!wc)
		    spSend(result, m_spView_setWclass,
			   ((flags & PB_TRY_AGAIN) ?
			    spwc_AddressAsk :
			    spwc_ListAsk));
	    }
	    wc = 1;
	}
	if (result->input) {
	    spSend(result, m_dialog_addFocusView, result->input);
	    if (flags2 & chooseone_Command) {
		ZmlSetInstanceName(result->input, "ask-command-field",
				   result);
		spSend(result->input, m_spView_setWclass, spwc_Commandfield);
		if (!wc)
		    spSend(result, m_spView_setWclass, spwc_CommandAsk);
	    } else {
		ZmlSetInstanceName(result->input, "ask-input-field", result);
		if (!wc) {
		    spSend(result, m_spView_setWclass,
			   ((flags & PB_TRY_AGAIN) ?
			    spwc_AddressAsk :
			    spwc_InputAsk));

		}
	    }
	    wc = 1;
	}
    } dialog_ENDMUNGE;
    return (result);
}
