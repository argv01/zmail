/*
 * $RCSfile: dynhdrs.c,v $
 * $Revision: 2.26 $
 * $Date: 1995/10/05 05:28:55 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <dynhdrs.h>

#include <zmlite.h>

#include <composef.h>
#include <zmlutil.h>

#include <dynstr.h>
#include <spoor/popupv.h>
#include <spoor/menu.h>
#include <spoor/buttonv.h>
#include <spoor/button.h>
#include <spoor/cmdline.h>
#include <spoor/textview.h>
#include <spoor/text.h>
#include <spoor/wrapview.h>
#include <spoor/splitv.h>

#include "catalog.h"

#ifndef lint
static const char dynhdrs_rcsid[] =
    "$Id: dynhdrs.c,v 2.26 1995/10/05 05:28:55 liblit Exp $";
#endif /* lint */

struct spWclass *dynhdrs_class = 0;

static catalog_ref removestring = catref(CAT_LITE, 848, "Unset Header");

struct menudata {
    char *label, *colonpart, *name;
    int nchoices;
};

enum {
    DONE_B, HELP_B
};

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
choice(self, button, dh, md)
    struct spMenu *self;
    struct spButton *button;
    struct dynhdrs *dh;
    struct menudata *md;
{
    Compose *comp = zmlcomposeframe_comp(spIm_view(ZmlIm));
    HeaderField *hf = lookup_header(&(comp->headers), md->name, ", ", 1);
    int selection = spButtonv_selection(self);
    int dhsel = spButtonv_selection(dh->dynhdrs);
    struct dynstr d;

    if (!hf)
	return;
    dynstr_Init(&d);
    TRY {
	if ((selection >= 0)
	    && (selection < md->nchoices)) { /* one of the values */
	    if (hf->hf_body && strcmp(spButton_label(button), hf->hf_body)) {
		if (md->colonpart) {
		    char *v;

		    (void) interactive_header(hf, md->colonpart + 1,
					      spButton_label(button));
		    v = get_var_value(VAR_HDR_VALUE);
		    dynstr_Set(&d, zmVaStr("%s %s",
					   md->label,
					   (v && *v) ? v : catgets(catalog, CAT_LITE, 645, "(empty)")));
		} else {
		    free(hf->hf_body);
		    hf->hf_body = savestr(spButton_label(button));
		    dynstr_Set(&d, zmVaStr("%s %s",
					   md->label, hf->hf_body));
		}
	    } else {
		dynstr_Set(&d, zmVaStr("%s %s",
				       md->label, spButton_label(button)));
	    }
	} else {			/* Unset */
	    dynstr_Set(&d, zmVaStr("%s %s", md->label, catgetref(removestring)));
	    if (hf->hf_body)
		hf->hf_body[0] = '\0';
	}
	spButtonv_selection(dh->dynhdrs) = dhsel; /* not sure why this
						   * bulletproofing is
						   * necessary */
	spSend(spButtonv_button(dh->dynhdrs, dhsel), m_spButton_setLabel,
	       dynstr_Str(&d));
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
nochoice(self, button, dh, md)
    struct spMenu *self;
    struct spButton *button;
    struct dynhdrs *dh;
    struct menudata *md;
{
    Compose *comp = zmlcomposeframe_comp(spIm_view(ZmlIm));
    HeaderField *hf = lookup_header(&(comp->headers), md->name, ", ", 1);
    char *oldbody = hf->hf_body;
    int dhsel = spButtonv_selection(dh->dynhdrs);

    if (!hf)
	return;
    if (spButtonv_selection(self) == 0) { /* "set header" */
	hf->hf_body = savestr(md->colonpart);
	if (!dynamic_header(comp, hf) && hf->hf_body && *(hf->hf_body)) {
	    free(oldbody);
	    spButtonv_selection(dh->dynhdrs) = dhsel; /* not sure why this
						       * bulletproofing is
						       * necessary */
	    spSend(spButtonv_button(dh->dynhdrs, dhsel), m_spButton_setLabel,
		   zmVaStr("%s %s", md->label, hf->hf_body));
	} else {
	    if (hf->hf_body)
		free(hf->hf_body);
	    hf->hf_body = oldbody;
	}
    } else {			/* "Unset" */
	if (hf->hf_body)
	    hf->hf_body[0] = '\0';
	spButtonv_selection(dh->dynhdrs) = dhsel; /* not sure why this
						   * bulletproofing is
						   * necessary */
	spSend(spButtonv_button(dh->dynhdrs, dhsel), m_spButton_setLabel,
	       zmVaStr("%s %s", md->label, catgetref(removestring)));
    }
}

static void
aa_done(b, self)
    struct spButton *b;
    struct dynhdrs *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct dynhdrs *self;
{
    zmlhelp("Dynamic Headers");
}

static void
hdrFn(self, str)
    struct spCmdline *self;
    char *str;
{
    spSend(self, m_spView_invokeInteraction, "text-beginning-of-line",
	   self, NULL, NULL);
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next",
	   self, NULL, NULL);
}

static char field_template[] = "%*s ";
static void
dynhdrs_initialize(self)
    struct dynhdrs *self;
{
    struct spWrapview *wrap;
    struct spSplitview *spl, *tree;
    int i;
    Compose *c;
    struct zmlcomposeframe *cf = (struct zmlcomposeframe *) spIm_view(ZmlIm);
    char *str[4];
    int field_length;

    ZmlSetInstanceName(self, "dynamicheaders", self);

    glist_Init(&(self->menudata), (sizeof (struct menudata *)), 4);

    spSend(self->to = spCmdline_NEW(), m_spView_setObserved,
	   spView_observed(cf->headers.to));
    spSend(self->subject = spCmdline_NEW(), m_spView_setObserved,
	   spView_observed(cf->headers.subject));
    spSend(self->cc = spCmdline_NEW(), m_spView_setObserved,
	   spView_observed(cf->headers.cc));
    spSend(self->bcc = spCmdline_NEW(), m_spView_setObserved,
	   spView_observed(cf->headers.bcc));
    spCmdline_fn(self->to) = hdrFn;
    spCmdline_fn(self->subject) = hdrFn;
    spCmdline_fn(self->cc) = hdrFn;
    spCmdline_fn(self->bcc) = hdrFn;
    ZmlSetInstanceName(self->to, "dynamic-to-header-field", self);
    ZmlSetInstanceName(self->subject, "dynamic-subject-header-field", self);
    ZmlSetInstanceName(self->cc, "dynamic-cc-header-field", self);
    ZmlSetInstanceName(self->bcc, "dynamic-bcc-header-field", self);

    self->dynhdrs = spMenu_NEW();
    spMenu_cancelfn(self->dynhdrs) = 0;
    spButtonv_style(self->dynhdrs) = spButtonv_vertical;
    ZmlSetInstanceName(self->dynhdrs, "dynamicheaders-menu", self);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
                          catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
                          catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	ZmlSetInstanceName(dialog_actionArea(self), "dynamicheaders-aa", self);

	if (spoor_IsClassMember(spIm_view(ZmlIm),
				(struct spClass *) zmlcomposeframe_class)) {
	    struct menudata *md;
	    HeaderField *hf;
	    int n;
	    char **choices, *p, *label, buf[256], *body;
	    struct spMenu *m;

	    c = zmlcomposeframe_comp(spIm_view(ZmlIm));
	    if (hf = c->headers) {
		do {
		    if (isoff(hf->hf_flags, DYNAMIC_HEADER))
			continue;
		    p = hf->hf_body;
		    if ((n = dynhdr_to_choices(&choices, &label,
					       &p, hf->hf_name)) < 0) {
			if (n == -2) {
			    remove_link(&(c->headers), hf);
			    destroy_header(hf);
			}
			continue;
		    }
		    if (*p == ':') {
			if (label)
			    *label = '\0';
			body = savestr(p);
			(void) no_newln(body);
			if (label)
			    *label = '(';
		    } else if (!choices) {
			body = savestr(zmVaStr(":ask -i %s", VAR_HDR_VALUE));
		    } else {
			body = NULL;
		    }
		    if (!choices)
			n = 0;
		    hf->hf_body[0] = '\0';
		    turnoff(hf->hf_flags, DYNAMIC_HEADER);
		    if (label) {
			p = rindex(++label, ')');
			if (p == label)
			    label = NULL;
			else
			    *p = ':';
		    }
		    if (!label)
			label = hf->hf_name;
		    m = spMenu_NEW();
		    if (label) {
			char *t = (char *) emalloc(2 + strlen(label),
						   "dynhdrs_initialize");

			strcpy(t, label);
			if (label == hf->hf_name)
			    strcat(t, ":");
			label = t;
		    }
		    md = (struct menudata *) emalloc(sizeof (struct menudata),
						     "dynhdrs_initialize");
		    md->name = savestr(hf->hf_name);
		    md->label = label;
		    md->colonpart = body;
		    md->nchoices = n;
		    glist_Add(&(self->menudata), &md);
		    if (n) {
			for (i = 0; i < n; ++i) {
			    spSend(m, m_spMenu_addFunction,
				   spButton_Create(choices[i], 0, 0),
				   choice, i,
				   self,
				   *((struct menudata **)
				     glist_Last(&(self->menudata))));
			}
		    } else {
			spSend(m, m_spMenu_addFunction,
			       spButton_Create(catgets(catalog, CAT_LITE, 814, "Set Header"), 0, 0),
			       nochoice, 0, self,
			       *((struct menudata **)
				 glist_Last(&(self->menudata))));
			   
		    }
		    spSend(m, m_spMenu_addFunction,
			   spButton_Create((char *) catgetref(removestring),
					   0, 0),
			   (n ? choice : nochoice), spButtonv_length(m),
			   self,
			   *((struct menudata **)
			     glist_Last(&(self->menudata))));
		    sprintf(buf, "%s %s", label ? label : "", catgetref(removestring));
		    spSend(self->dynhdrs, m_spMenu_addMenu,
			   spButton_Create(buf, 0, 0), m,
			   spButtonv_length(self->dynhdrs));
		} while ((hf = ((HeaderField *) (hf->hf_link.l_next))) !=
			 c->headers);
	    }
	}

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 815, "Dynamic Headers"), spWrapview_top);

	tree = spSplitview_NEW();
        str[0] = catgets(catalog, CAT_LITE, 125, "To:");
        str[1] = catgets(catalog, CAT_LITE, 126, "Subject:");
        str[2] = catgets(catalog, CAT_LITE, 127, "Cc:");
        str[3] = catgets(catalog, CAT_LITE, 128, "Bcc:");
        field_length = max (strlen(str[0]), strlen(str[1]));
        field_length = max ( max ( strlen(str[2]), field_length), strlen(str[3]));


	wrap = spWrapview_NEW();
	spSend(wrap, m_spWrapview_setLabel, 
               zmVaStr(field_template, field_length, str[0]), spWrapview_left);
	spSend(wrap, m_spWrapview_setView, self->to);

	spl = SplitAdd(tree, wrap, 1, 0, 0, spSplitview_topBottom,
		       spSplitview_plain, 0);

	wrap = spWrapview_NEW();
	spSend(wrap, m_spWrapview_setLabel, 
              zmVaStr(field_template, field_length, str[1]), spWrapview_left);
	spSend(wrap, m_spWrapview_setView, self->subject);

	spl = SplitAdd(spl, wrap, 1, 0, 0, spSplitview_topBottom,
		       spSplitview_plain, 0);

	wrap = spWrapview_NEW();
	spSend(wrap, m_spWrapview_setLabel, 
               zmVaStr(field_template, field_length, str[2]), spWrapview_left);
	spSend(wrap, m_spWrapview_setView, self->cc);

	if (spButtonv_length(self->dynhdrs)) {
	    spl = SplitAdd(spl, wrap, 1, 0, 0, spSplitview_topBottom,
			   spSplitview_plain, 0);

	    wrap = spWrapview_NEW();
	    spSend(wrap, m_spWrapview_setLabel, 
                   zmVaStr(field_template, field_length, str[3]), spWrapview_left);
	    spSend(wrap, m_spWrapview_setView, self->bcc);

	    spSend(spl, m_spSplitview_setup, wrap, self->dynhdrs,
		   1, 0, 0, spSplitview_topBottom, spSplitview_boxed,
		   spSplitview_SEPARATE);
	} else {
	    spSend(spl, m_spSplitview_setup,
		   wrap, Wrap(self->bcc, NULL, NULL, 
                   zmVaStr(field_template, field_length, str[3]), NULL,
			      0, 0, 0),
		   1, 0, 0, spSplitview_topBottom, spSplitview_plain, 0);
	}

	spSend(self, m_dialog_setView, tree);
    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->to);
    spSend(self, m_dialog_addFocusView, self->subject);
    spSend(self, m_dialog_addFocusView, self->cc);
    spSend(self, m_dialog_addFocusView, self->bcc);
    if (spButtonv_length(self->dynhdrs))
	spSend(self, m_dialog_addFocusView, self->dynhdrs);
}

static void
dynhdrs_finalize(self)
    struct dynhdrs *self;
{
    int i;
    struct spView *v = dialog_view(self);
    struct menudata *md;

    spSend(self, m_dialog_setView, NULL);
    KillSplitviewsAndWrapviews(v);
    spSend(self->dynhdrs, m_spView_destroyObserved);
    spoor_DestroyInstance(self->to);
    spoor_DestroyInstance(self->subject);
    spoor_DestroyInstance(self->cc);
    spoor_DestroyInstance(self->bcc);
    spoor_DestroyInstance(self->dynhdrs);
    for (i = 0; i < glist_Length(&(self->menudata)); ++i) {
	md = *((struct menudata **) glist_Nth(&(self->menudata), i));
	if (md->label)
	    free(md->label);
	if (md->colonpart)
	    free(md->colonpart);
	if (md->name)
	    free(md->name);
	free(md);
    }
    glist_Destroy(&(self->menudata));
}

static void
dynhdrs_desiredSize(self, arg)
    struct dynhdrs *self;
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

    spSuper(dynhdrs_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    if (*bestw < (screenw - 10))
	*bestw = screenw - 10;
}

struct spWidgetInfo *spwc_Dynamicheaders = 0;

void
dynhdrs_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (dynhdrs_class)
	return;
    dynhdrs_class =
	spWclass_Create("dynhdrs", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct dynhdrs)),
			dynhdrs_initialize,
			dynhdrs_finalize,
			spwc_Dynamicheaders = spWidget_Create("Dynamicheaders",
							      spwc_Popup));

    spoor_AddOverride(dynhdrs_class, m_spView_desiredSize, NULL,
		      dynhdrs_desiredSize);

    zmlcomposeframe_InitializeClass();
    spPopupView_InitializeClass();
    spMenu_InitializeClass();
    spButtonv_InitializeClass();
    spButton_InitializeClass();
    spCmdline_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spWrapview_InitializeClass();
    spSplitview_InitializeClass();
}
