/*
 * $RCSfile: multikey.c,v $
 * $Revision: 2.30 $
 * $Date: 1995/07/25 22:02:10 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <multikey.h>
#include <dynstr.h>
#include <spoor/text.h>
#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/im.h>
#include <spoor/button.h>
#include <spoor/buttonv.h>
#include <spoor/popupv.h>
#include <spoor/cursim.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include <zmlite.h>
#include <zmlutil.h>
#include <zmail.h>
#include <fsfix.h>

#include "catalog.h"

#ifndef lint
static const char multikey_rcsid[] =
    "$Id: multikey.c,v 2.30 1995/07/25 22:02:10 bobg Exp $";
#endif /* lint */

struct spWclass *multikey_class = 0;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
sequence_focus(sequence)
    struct spTextview *sequence;
{
    struct multikey *self = spView_callbackData(sequence);
    int c;
    char *charname;

    spSend(spView_observed(self->instructions), m_spText_clear);
    spSend(spView_observed(self->instructions), m_spText_insert, -1, -1,
	   catgets(catalog, CAT_LITE, 301, "Press the key you wish to define, then wait 1/2 second.  "),
	   spText_mNeutral);
    spSend(spView_observed(self->instructions), m_spText_insert, -1, -1,
	   catgets(catalog, CAT_LITE, 302, "The character sequence for the key will appear."),
	   spText_mNeutral);

    spSend(spView_observed(sequence), m_spText_clear);
    spKeysequence_Truncate(&(self->ks), 0);
    spSend(ZmlIm, m_spIm_forceUpdate, 0);
    spKeysequence_Add(&(self->ks),
		      c = spSend_i(ZmlIm, m_spIm_getRawChar));
    charname = spKeyname(c, 1);
    spSend(spView_observed(sequence), m_spText_insert,
	   -1, strlen(charname), charname, spText_mNeutral);
    spSend(ZmlIm, m_spIm_forceUpdate, 0);
    while (spSend_i(ZmlIm, m_spIm_checkChar, 0, 500000)) {
	spKeysequence_Add(&(self->ks),
			  c = spSend_i(ZmlIm, m_spIm_getRawChar));
	spSend(spView_observed(sequence), m_spText_insert, -1,
	       1, " ", spText_mNeutral);
	charname = spKeyname(c, 1);
	spSend(spView_observed(sequence), m_spText_insert,
	       -1, strlen(charname), charname, spText_mNeutral);
	spSend(ZmlIm, m_spIm_forceUpdate, 0);
    }
    spSend(dialog_actionArea(self), m_spView_wantFocus,
	   dialog_actionArea(self));
}

static void
keyname_activate(keyname)
    struct spCmdline *keyname;
{
    struct multikey *self = spView_callbackData(keyname);

    spSend(self->sequence, m_spView_wantFocus, self->sequence);
}

static void
keyname_focus(keyname)
    struct spCmdline *keyname;
{
    struct multikey *self = spView_callbackData(keyname);

    spSend(spView_observed(self->instructions), m_spText_clear);
    spSend(spView_observed(self->instructions), m_spText_insert, -1, -1,
	   catgets(catalog, CAT_LITE, 303, "Enter the Z-Script name of the key you wish to define.  "),
	   spText_mNeutral);
    spSend(spView_observed(self->instructions), m_spText_insert, -1, -1,
	   catgets(catalog, CAT_LITE, 304, "Press Enter when ready."), spText_mNeutral);
}

static void
keynames_focus(keynames)
    struct spListv *keynames;
{
    struct multikey *self = spView_callbackData(keynames);

    spSend(spView_observed(self->instructions), m_spText_clear);
    spSend(spView_observed(self->instructions), m_spText_insert, -1, -1,
	   catgets(catalog, CAT_LITE, 305, "Select the name of a key to define, or enter the\nkey name directly in the Key Name field."), spText_mNeutral);
}

static void
action_focus(action)
    struct spButtonv *action;
{
    struct multikey *self = spView_callbackData(action);

    spSend(spView_observed(self->instructions), m_spText_clear);
    spSend(spView_observed(self->instructions), m_spText_insert, -1, -1,
	   catgets(catalog, CAT_LITE, 307, "Press Define to define the key, Done to dismiss,\nLoad to load definitions, Save to save them."),
	   spText_mNeutral);
}

static void
keynamesCallback(keynames, which, clicktype)
    struct spListv *keynames;
    int which;
    enum spListv_clicktype clicktype;
{
    struct multikey *self = spView_callbackData(keynames);
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(keynames), m_spList_getNthItem, which, &d);
    spSend(spView_observed(self->keyname), m_spText_clear);
    spSend(spView_observed(self->keyname), m_spText_insert, -1, -1,
	   dynstr_Str(&d), spText_mBefore);
    dynstr_Destroy(&d);
    if (clicktype == spListv_doubleclick)
	spSend(self->keyname, m_spView_wantFocus, self->keyname);
}

enum {
    DONE_B, DEFINE_B, LOAD_B, SAVE_B
};

static void
aa_done(b, self)
    struct spButton *b;
    struct multikey *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_define(b, self)
    struct spButton *b;
    struct multikey *self;
{
    struct dynstr tmp;

    if ((spKeysequence_Length(&(self->ks)) == 0)
	|| (spSend_i(spView_observed(self->keyname), m_spText_length) == 0)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 309, "Choose a key name and type its key first"));
	return;
    }
    dynstr_Init(&tmp);
    TRY {
	dynstr_Set(&tmp, "\\<");
	spSend(spView_observed(self->keyname),
	       m_spText_appendToDynstr, &tmp, 0, -1);
	dynstr_Append(&tmp, ">");
	spSend(ZmlIm, m_spIm_addTranslation, &(self->ks),
	       spKeysequence_Parse(0, dynstr_Str(&tmp), 1));
	spSend(ZmlIm, m_spIm_showmsg, catgets(catalog, CAT_LITE, 24, "Done"), 15, 2, 5);
    } EXCEPT(strerror(EINVAL)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 311, "Bad key sequence specification: %s"),
	      dynstr_Str(&tmp));
    } FINALLY {
	dynstr_Destroy(&tmp);
    } ENDTRY;
}

static void
aa_load(b, self)
    struct spButton *b;
    struct multikey *self;
{
    ZCommand("\\multikey -l", zcmd_ignore);
}

static void
aa_save(b, self)
    struct spButton *b;
    struct multikey *self;
{
    char *term;
    struct dynstr filename, tmp;

    dynstr_Init(&filename);
    dynstr_Init(&tmp);
    TRY {
	if ((term = getenv("LITETERM"))
	    || (term = getenv("TERM"))) {
	    dynstr_Append(&filename, get_var_value(VarHome));
	    dynstr_Append(&filename, PSEP);
	    dynstr_Append(&filename, ".multikey");
	    dynstr_Append(&filename, PSEP);
	    dynstr_Append(&filename, term);
	}
	if (dyn_choose_one(&tmp,
			   catgets(catalog, CAT_LITE, 312, "Save term-specific multikeys to what file?"),
			   term ? dynstr_Str(&filename) : 0, 0, 0,
			   PB_FILE_OPTION | PB_NOT_A_DIR) == 0) {
	    dynstr_Replace(&tmp, 0, 0, "\\multikey -s ");
	    ZCommand(dynstr_Str(&tmp), zcmd_ignore);
	}
    } FINALLY {
	dynstr_Destroy(&tmp);
	dynstr_Destroy(&filename);
    } ENDTRY;
}

static void
aa_help(b, self)
    struct spButton *b;
    struct multikey *self;
{
    zmlhelp("Multikey Dialog");
}

static char field_template[] = "%*s ";
static void
multikey_initialize(self)
    struct multikey *self;
{
    struct dynstr d;
    struct spText *t;
    char buf[16], *p, *e;
    int i;
    int field_length;
    char *str[2];

    ZmlSetInstanceName(self, "multikey", self);

    spKeysequence_Init(&(self->ks));

    self->instructions = spTextview_NEW();
    spSend(self->instructions, m_spView_setObserved, t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spTextview_wrapmode(self->instructions) = spTextview_wordwrap;

    self->sequence = spCmdline_NEW();
    spSend(self->sequence, m_spView_setObserved, t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spView_callbacks(self->sequence).receiveFocus = sequence_focus;
    spView_callbackData(self->sequence) = self;

    self->keyname = spCmdline_NEW();
    spCmdline_revert(self->keyname) = 1;
    spSend(self->keyname, m_spView_setObserved, t = spText_NEW());
    spCmdline_fn(self->keyname) = keyname_activate;
    spView_callbacks(self->keyname).receiveFocus = keyname_focus;
    spView_callbackData(self->keyname) = self;
    ZmlSetInstanceName(self->keyname, "keyname-field", self);

    spSend(self->keynames = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->keynames) = keynamesCallback;
    spView_callbacks(self->keynames).receiveFocus = keynames_focus;
    spView_callbackData(self->keynames) = self;
    spListv_okclicks(self->keynames) = ((1 << spListv_click)
					| (1 << spListv_doubleclick));
    ZmlSetInstanceName(self->keynames, "keynames-list", self);

    dynstr_Init(&d);
    for (i = 0; spCursesIm_defaultKeys[i].name; ++i) {
	p = spCursesIm_defaultKeys[i].name + 2; /* point past the "\<" */
	e = index(p, '>');
	dynstr_Set(&d, "");
	dynstr_AppendN(&d, p, e - p);
	spSend(spView_observed(self->keynames), m_spList_append,
	       dynstr_Str(&d));
    }
    dynstr_Destroy(&d);

    for (i = 1; i <= spCursesIm_MAXAUTOFKEY; ++i) {
	sprintf(buf, "f%d", i);
	spSend(spView_observed(self->keynames), m_spList_append, buf);
    }

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 314, "Define"), aa_define,
			  catgets(catalog, CAT_LITE, 315, "Load"), aa_load,
			  catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	spButtonv_selection(dialog_actionArea(self)) = 1;
	ZmlSetInstanceName(dialog_actionArea(self), "multikey-aa", self);

	spView_callbacks(dialog_actionArea(self)).receiveFocus = action_focus;

        str[0] = catgets(catalog, CAT_LITE, 319, "Key Name:");
        str[1] = catgets(catalog, CAT_LITE, 320, "Sequence:");
        field_length = max(strlen(str[0]), strlen(str[1]));
	spSend(self, m_dialog_setView,
	       Split(Wrap(self->keynames, catgets(catalog, CAT_LITE, 318, "Key Names"), NULL,
			  NULL, NULL, 1, 1, 0),
		     Split(self->instructions,
			   Split(Wrap(self->keyname,
				      NULL, NULL,
				      zmVaStr(field_template, field_length, str[0]), NULL,
				      0, 1, 0),
				 (self->seqwrap =
				  Wrap(self->sequence,
				       NULL, NULL,
				       zmVaStr(field_template, field_length, str[1]), NULL,
				       0, 1, 0)),
				 3, 0, 0, spSplitview_topBottom,
				 spSplitview_plain, 0),
			   3, 0, 0, spSplitview_topBottom,
			   spSplitview_plain, 0),
		     16, 0, 0, spSplitview_leftRight,
		     spSplitview_plain, 0));
    } dialog_ENDMUNGE;
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 321, "Define Multikey"), spWrapview_top);

    spSend(self, m_dialog_addFocusView, self->keynames);
    spSend(self, m_dialog_addFocusView, self->keyname);
    spSend(self, m_dialog_addFocusView, self->sequence);
}

static void
multikey_finalize(self)
    struct multikey *self;
{
    /* To do: undo everything above */
    spKeysequence_Destroy(&(self->ks));
}

static void
multikey_desiredSize(self, arg)
    struct multikey *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(multikey_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    *bestw = 70;
    *maxh = 15;
}

struct spWidgetInfo *spwc_Multikey = 0;

void
multikey_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (multikey_class)
	return;
    multikey_class =
	spWclass_Create("multikey", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct multikey)),
			multikey_initialize,
			multikey_finalize,
			spwc_Multikey = spWidget_Create("Multikey",
							spwc_Popup));
    spoor_AddOverride(multikey_class, m_spView_desiredSize, NULL,
		      multikey_desiredSize);

    spText_InitializeClass();
    spTextview_InitializeClass();
    spCmdline_InitializeClass();
    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spIm_InitializeClass();
    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spPopupView_InitializeClass();
    spCursesIm_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
