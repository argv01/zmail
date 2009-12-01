/*
 * $RCSfile: varf.c,v $
 * $Revision: 2.41 $
 * $Date: 1998/12/07 23:56:33 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <varf.h>

#include <zmlite.h>
#include <zmlutil.h>
#include <zmail.h>

#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/im.h>
#include <spoor/buttonv.h>
#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/button.h>
#include <spoor/toggle.h>
#include <spoor/text.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <dynstr.h>
#include <spoor/wclass.h>

#include "catalog.h"

static const char zmlvarframe_rcsid[] =
    "$Id: varf.c,v 2.41 1998/12/07 23:56:33 schaefer Exp $";

static struct glist VarListPos;

#define var_list_pos(n) (*((int *) glist_Nth(&VarListPos,(n))))

struct spWclass *zmlvarframe_class = 0;

struct zmlvarframe *myself;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
multivalueActivate(self, which)
    struct spButtonv *self;
    int which;
{
    char buf[256];
    struct zmlvarframe *vf = (struct zmlvarframe *) spView_callbackData(self);
    struct dynstr d;

    if (vf->singleval) {
	spButtonv_radioButtonHack(self, which);
    } else {
	spSend(spButtonv_button(self, which), m_spButton_push);
    }
    dynstr_Init(&d);
    TRY {
	spSend(spView_observed(vf->vars), m_spList_getNthItem,
	       spListv_lastclick(vf->vars), &d);
	sprintf(buf, "set %s %s= %s",
		dynstr_Str(&d),
		(vf->singleval ? "" :
		 (spToggle_state(spButtonv_button(self, which)) ? "+" : "-")),
		spButton_label(spButtonv_button(self, which)));
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    ZCommand(buf, zcmd_commandline);
}

static void
freeformActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct zmlvarframe *vf = (struct zmlvarframe *) self->obj;
    char *argv[4];
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(vf->vars), m_spList_getNthItem,
	   spListv_lastclick(vf->vars), &d);
    argv[0] = dynstr_Str(&d);
    argv[1] = "=";
    argv[2] = str;
    argv[3] = NULL;
    add_option(&set_options, argv);
    dynstr_Destroy(&d);
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next",
	   self, NULL, NULL);
}

static void
dofocuslist(self)
    struct zmlvarframe *self;
{
    spSend(self, m_dialog_clearFocusViews);
    spSend(self, m_dialog_addFocusView, self->display);
    spSend(self, m_dialog_addFocusView, self->vars);
    spSend(self, m_dialog_addFocusView, self->descr);
    spSend(self, m_dialog_addFocusView, self->onoff);
    if (self->multivalue
	&& spView_window(self->multivalue)
	&& spToggle_state(spButtonv_button(self->onoff, 0))) {
	spSend(self, m_dialog_addFocusView, self->multivalue);
    } else if (self->freeform
	       && spView_window(self->freeform)
	       && spToggle_state(spButtonv_button(self->onoff, 0))) {
	spSend(self, m_dialog_addFocusView, self->freeform);
    }
}

static void
setupoptions(self, which, jump)
    struct zmlvarframe *self;
    int which, jump;
{
    struct spView *tree = spWrapview_view(self->optionArea);
    char *value;

    spSend(self->optionArea, m_spWrapview_setView, (struct spView *) 0);
    if (tree) {
	KillSplitviewsAndWrapviews(tree);
    }
    value = (ison(variables[which].v_flags, V_READONLY) ?
	     check_internal(variables[which].v_opt) :
	     get_var_value(variables[which].v_opt));
    if (ison(variables[which].v_flags, V_BOOLEAN)) {
	spSend(spButtonv_button(self->onoff, 0), m_spButton_setLabel,
	       catgets(catalog, CAT_LITE, 439, "On/Off"));
    } else {
	spSend(spButtonv_button(self->onoff, 0), m_spButton_setLabel,
	       catgets(catalog, CAT_LITE, 440, "Set/Unset"));
    }
    spToggle_state(spButtonv_button(self->onoff, 0)) = (value ? 1 : 0);
    spSend(self->onoff, m_spView_wantUpdate, self->onoff,
	   1 << spView_fullUpdate);
    if (ison(variables[which].v_flags, V_MULTIVAL)
	|| ison(variables[which].v_flags, V_SINGLEVAL)) {
	int i;

	if (!(self->multivalue)) {
	    self->multivalue = spButtonv_NEW();
	    spButtonv_style(self->multivalue) = spButtonv_multirow;
	    spButtonv_toggleStyle(self->multivalue) = spButtonv_checkbox;
	    spButtonv_callback(self->multivalue) = multivalueActivate;
	    spView_callbackData(self->multivalue) = (struct spoor *) self;
	    spSend(self->multivalue, m_spView_setWclass, spwc_Togglegroup);
	    ZmlSetInstanceName(self->multivalue, "variable-multivalue-tg", 0);

	    if (spView_window(self->optionArea)) {
		int h, w;

		spSend(spView_window(self->optionArea), m_spWindow_size,
		       &h, &w);
		spButtonv_anticipatedWidth(self->multivalue) = w;
	    }
	}
	self->singleval = ison(variables[which].v_flags, V_SINGLEVAL);
	spSend(self->multivalue, m_spView_destroyObserved);
	for (i = 0; i < variables[which].v_num_vals; ++i) {
	    spSend(self->multivalue, m_spButtonv_insert,
		   spToggle_Create(variables[which].v_values[i].v_label,
				   0, 0, 0), i);
	    if (chk_two_lists(value, variables[which].v_values[i].v_label,
			      " ,")) {
		spToggle_state(spButtonv_button(self->multivalue, i)) = 1;
	    }
	}
	spSend(self->optionArea, m_spWrapview_setView, self->multivalue);
    } else if (ison(variables[which].v_flags, V_NUMERIC | V_STRING)) {
	struct spWrapview *wrap;
	char *toplabel = variables[which].v_prompt.v_label;

	if (!(self->freeform)) {
	    spSend(self->freeform = spCmdline_NEW(), m_spView_setObserved,
		   spText_NEW());
	    spCmdline_revert(self->freeform) = 1;
	    spCmdline_fn(self->freeform) = freeformActivate;
	    spCmdline_obj(self->freeform) = (struct spoor *) self;
	    ZmlSetInstanceName(self->freeform, "variable-value-field", 0);
	}
	spSend(spView_observed(self->freeform), m_spText_clear);
	if (value && *value) {
	    spSend(spView_observed(self->freeform), m_spText_insert,
		   0, strlen(value), value, spText_mBefore);
	}
	wrap = spWrapview_NEW();
	spWrapview_boxed(wrap) = 1;
	spWrapview_align(wrap) = spWrapview_TFLUSH | spWrapview_LRCENTER;
	spSend(wrap, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 441, "Press RETURN to set new value"),
	       spWrapview_bottom);
	spSend(wrap, m_spWrapview_setLabel, (toplabel ? toplabel : ""),
	       spWrapview_top);
	spSend(wrap, m_spWrapview_setView, self->freeform);
	spSend(self->optionArea, m_spWrapview_setView, wrap);
    }
    dofocuslist(self);
    if (jump && spView_window(self->onoff))
	spSend(self->onoff, m_spView_wantFocus, self->onoff);
}

static void
varsCallback(self, num, clicktype)
    struct spListv *self;
    int num;
    enum spListv_clicktype clicktype;
{
    struct zmlvarframe *vf = ((struct zmlvarframe *)
			      spView_callbackData(self));
    char *descr;

    spSend(spView_observed(vf->descr), m_spText_clear);
    descr = variable_stuff(var_list_pos(num), NULL);
    spSend(spView_observed(vf->descr), m_spText_insert, 0, -1,
	   descr, spText_mAfter);
    setupoptions(vf, var_list_pos(num), clicktype == spListv_doubleclick);
}

enum {
    DONE_B, SAVE_B, LOAD_B, HELP_B
};

static void
aa_done(b, self)
    struct spButton *b;
    struct zmlvarframe *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_save(b, self)
    struct spButton *b;
    struct zmlvarframe *self;
{
    doSaveState();
}

static void
aa_load(b, self)
    struct spButton *b;
    struct zmlvarframe *self;
{
    char *dflt;
    struct dynstr buf;

    dynstr_Init(&buf);
    TRY {
	if (!(dflt = getenv("ZMAILRC")))
	    if (!(dflt = getenv("MAILRC")))
		dflt = "~/.zmailrc";
	if (!dyn_choose_one(&buf, catgets(catalog, CAT_LITE, 442, "Load application state from:"),
			    dflt, (char **) 0, 0, PB_FILE_OPTION)
	    && dynstr_Str(&buf)[0]) {
	    dynstr_Replace(&buf, 0, 0, "\\source ");
	    ZCommand(dynstr_Str(&buf), zcmd_commandline);
	}
    } FINALLY {
	dynstr_Destroy(&buf);
    } ENDTRY;
}

static void
aa_help(b, self)
    struct spButton *b;
    struct zmlvarframe *self;
{
    zmlhelp("Variables");
}

/*
 * these values must match the buttons that are initialized (reverse 
 * order) in zmlvarframe_initialize.  This is lame, but works - TCJ 
 */

/*
 * Get the set of variable names which should appear in the dialog.
 */

static void
set_selectable_vars(self, category)
    struct zmlvarframe *self;
    unsigned long category;
{
    int n, pos = 0, l;
    struct spList *list = (struct spList *) spView_observed(self->vars);

    for (n = 0; n < n_variables; n++) {
	glist_Set(&VarListPos, pos, &n);
    	if (ison(variables[n].v_flags, V_GUI|V_MAC|V_MSWINDOWS|V_CURSES)
	    && !ison(variables[n].v_flags, V_VUI|V_TTY)
	    || category && !(variables[n].v_category & category)) {
	    int t = -1;
	} else {
	    if (pos < spSend_i(list, m_spList_length))
		spSend(list, m_spList_replace, pos, variables[n].v_opt);
	    else
		spSend(list, m_spList_append, variables[n].v_opt);
	    pos++;
    	}
    }
    /* Now `pos' is the desired length of the list */
    while ((l = spSend_i(list, m_spList_length)) > pos)
	spSend(list, m_spList_remove, l - 1);
}

static struct {
    catalog_ref name;
    long value;
} categories[] = {
    { catref(CAT_LITE, 899, "All"), 0 },
    { catref(CAT_LITE, 900, "User Interface"), ULBIT(2) },
    { catref(CAT_LITE, 901, "Message"), ULBIT(1) },
    { catref(CAT_LITE, 902, "Compose"), ULBIT(0) },
    { catref(CAT_LITE, 903, "Z-Script"), ULBIT(3) },
    { catref(CAT_LITE, 904, "Localization"), ULBIT(4) },
    { catref(CAT_LITE, 905, "LDAP"), ULBIT(5) },
    { catref(CAT_LITE, 906, "POP"), ULBIT(6) },
    { catref(CAT_LITE, 907, "IMAP"), ULBIT(7) },
    { catref(CAT_LITE, 908, "User Preferences"), ULBIT(8) },
};

static void
display_activate(self, button, vf, data)
    struct spMenu *self;
    struct spButton *button;
    struct zmlvarframe *vf;
    VPTR data;
{
    char *label = spButton_label(button);
    int i;

    for (i = 0; i < ArraySize(categories); ++i) {
	if (!strcmp(catgetref(categories[i].name), label)) {
	    spSend(spButtonv_button(vf->display, 0), m_spButton_setLabel,
		   label);
	    set_selectable_vars(vf, categories[i].value);
	    spSend(vf->vars, m_spView_invokeInteraction, "list-click-line",
		   vf->vars, "1", 0);
	    return;
	}
    }
}

/*
  This function can be called from an external source to select a category
  to be the current category. It is used from the setup menu to select and
  display a category of variables necessary to set in a setup.
*/
void
select_category_num(i)
int i;
{
  spSend(spButtonv_button(myself->display, 0), m_spButton_setLabel,
         catgetref(categories[i].name));
  set_selectable_vars(myself, categories[i].value);
  spSend(myself->vars, m_spView_invokeInteraction, "list-click-line",
         myself->vars, "1", 0);
}

static void
onoffActivate(self, which)
    struct spButtonv *self;
    int which;
{
    struct zmlvarframe *vf = (struct zmlvarframe *) spView_callbackData(self);
    struct dynstr d;

    spSend(spButtonv_button(self, which), m_spButton_push);
    dynstr_Init(&d);
    TRY {
	spSend(spView_observed(vf->vars), m_spList_getNthItem,
	       spListv_lastclick(vf->vars), &d);
	if (spToggle_state(spButtonv_button(self, which))) {
	    dynstr_Replace(&d, 0, 0, "\\set ");
	    if (vf->multivalue && spView_window(vf->multivalue)) {
		int i, first = 1;

		dynstr_Append(&d, " = '");
		for (i = 0; i < spButtonv_length(vf->multivalue); ++i) {
		    if (spToggle_state(spButtonv_button(vf->multivalue,
							i))) {
			char *label;

			label = spButton_label(spButtonv_button(vf->multivalue,
								i));
			if (!first)
			    dynstr_AppendChar(&d, ',');
			first = 0;
			dynstr_Append(&d, quotezs(label, '\''));
		    }
		}
		dynstr_AppendChar(&d, '\'');
	    } else if (vf->freeform && spView_window(vf->freeform)) {
		struct dynstr d2;

		dynstr_Append(&d, " = '");
		dynstr_Init(&d2);
		TRY {
		    spSend(spView_observed(vf->freeform),
			   m_spText_appendToDynstr, &d2, 0, -1);
		    dynstr_Append(&d, quotezs(dynstr_Str(&d2), '\''));
		} FINALLY {
		    dynstr_Destroy(&d2);
		} ENDTRY;
		dynstr_AppendChar(&d, '\'');
	    }
	    ZCommand(dynstr_Str(&d), zcmd_commandline);
	} else {
	    ZCommand(zmVaStr("\\unset %s", dynstr_Str(&d)),
		     zcmd_commandline);
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    dofocuslist(vf);
    spSend(self, m_spView_wantFocus, self);
}

static void
zmlvarframe_initialize(self)
    struct zmlvarframe *self;
{
    struct spText *t;
    int i, minh = 0, maxh = 0, minw = 0, maxw = 0, besth = 0, bestw = 0;
    int menuw = 0, thisw;
    const char *str;
    struct spMenu *menu;

    ZmlSetInstanceName(self, "variables", 0);
    myself = self;

    self->selected = -1;
    self->multivalue = (struct spButtonv *) 0;
    self->freeform = (struct spCmdline *) 0;

    spSend(self->vars = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_okclicks(self->vars) = (1 << spListv_click);
    spListv_callback(self->vars) = varsCallback;
    spView_callbackData(self->vars) = (struct spoor *) self;
    ZmlSetInstanceName(self->vars, "variable-list", 0);

    self->onoff = spButtonv_NEW();
    ZmlSetInstanceName(self->onoff, "variable-onoff-btnpanel", 0);

    spSend(self->descr = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    ZmlSetInstanceName(self->descr, "variable-description", 0);

    self->optionArea = spWrapview_NEW();
    spWrapview_align(self->optionArea) = spWrapview_TBCENTER;

    set_selectable_vars(self, 0);

    spButtonv_style(self->onoff) = spButtonv_vertical;
    spButtonv_toggleStyle(self->onoff) = spButtonv_checkbox;
    spButtonv_callback(self->onoff) = onoffActivate;
    spView_callbackData(self->onoff) = (struct spoor *) self;
    
    spSend(self->onoff, m_spButtonv_insert,
	   spToggle_Create(catgets(catalog, CAT_LITE, 440, "Set/Unset"),
			   0, 0, 0),
	   0);

    self->display = spMenu_NEW();
    ZmlSetInstanceName(self->display, "variable-category-menu", self);
    spMenu_cancelfn(self->display) = 0;
    spButtonv_style(self->display) = spButtonv_vertical;

    menu = spMenu_NEW();
    spSend(menu, m_spView_setWclass, spwc_PullrightMenu);
    ZmlSetInstanceName(menu, "variable-category-pullright", self);
    for (i = 0; i < ArraySize(categories); ++i) {
	str = catgetref(categories[i].name);
	spSend(menu, m_spMenu_addFunction,
	       spButton_Create(str, 0, 0),
	       display_activate, -1, self, 0);
	if ((thisw = strlen(str)) > menuw)
	    menuw = thisw;
    }
    spSend(self->display, m_spMenu_addMenu,
	   spButton_Create(" ", 0, 0), menu, 0); /* menu code will add
						  * pullright arrow, which we
						  * don't want... */
    /* ...so set the label after setting up the submenu */
    spSend(spButtonv_button(self->display, 0), m_spButton_setLabel,
	   catgetref(categories[0].name));

    spSend(self->vars, m_spView_desiredSize,
	   &minh, &minw, &maxh, &maxw, &besth, &bestw);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setView,
	       Split(Split(Wrap(self->display, 0, 0, 0, 0,
				0, 1, 0),
			   Wrap(self->vars,
				catgets(catalog, CAT_LITE, 444, "Variables"),
				0, 0, 0,
				0, 1, 0),
			   3, 0, 0,
			   spSplitview_topBottom, spSplitview_plain, 0),
		     Split(Wrap(self->descr,
				catgets(catalog, CAT_LITE, 445, "Variable Description"),
				0, 0, 0,
				0, 1, 0),
			   Split(Wrap(self->onoff, 0, 0, 0, 0,
				      0, 0, spWrapview_TBCENTER),
				 self->optionArea,
				 18, 0, 0,
				 spSplitview_leftRight,
				 spSplitview_plain, 0),
			   60, 0, 1,
			   spSplitview_topBottom, spSplitview_plain, 0),
		     MAX(bestw, menuw) + 2, 0, 0,
		     spSplitview_leftRight, spSplitview_plain, 0));
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			  catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			  catgets(catalog, CAT_LITE, 315, "Load"), aa_load,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
    } dialog_ENDMUNGE;
    ZmlSetInstanceName(dialog_actionArea(self), "variables-aa", self);
    spSend(self->vars, m_spView_invokeInteraction,
	   "list-click", self, "1", 0);
}

static void
zmlvarframe_finalize(self)
    struct zmlvarframe *self;
{
    /* To do: undo everything above */
}

static void
zmlvarframe_enter(self, arg)
    struct zmlvarframe *self;
    spArgList_t arg;
{
    spSuper(zmlvarframe_class, self, m_dialog_enter);
    spSend(self->vars, m_spView_wantFocus, self->vars);
}

struct spWidgetInfo *spwc_Vars = 0;

void
zmlvarframe_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmlvarframe_class)
	return;
    zmlvarframe_class =
	spWclass_Create("zmlvarframe", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct zmlvarframe)),
			zmlvarframe_initialize,
			zmlvarframe_finalize,
			spwc_Vars = spWidget_Create("Vars", spwc_Screen));

    spoor_AddOverride(zmlvarframe_class, m_dialog_enter, NULL,
		      zmlvarframe_enter);

    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spIm_InitializeClass();
    spButtonv_InitializeClass();
    spTextview_InitializeClass();
    spCmdline_InitializeClass();
    spButton_InitializeClass();
    spToggle_InitializeClass();
    spText_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();

    glist_Init(&VarListPos, (sizeof (int)), 32);
}
