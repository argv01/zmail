/*
 * $RCSfile: tsearch.c,v $
 * $Revision: 2.43 $
 * $Date: 1995/09/09 21:23:16 $
 * $Author: liblit $
 */

#include <spoor.h>
#include <tsearch.h>

#include <dynstr.h>
#include <regexpr.h>

#include <spoor/splitv.h>
#include <spoor/text.h>
#include <spoor/cmdline.h>
#include <spoor/wrapview.h>
#include <spoor/popupv.h>
#include <spoor/button.h>
#include <spoor/listv.h>
#include <spoor/list.h>

#include <msgf.h>
#include <composef.h>
#include <zmlite.h>

#include <zmail.h>

#include "catalog.h"

#ifndef lint
static const char tsearch_rcsid[] =
    "$Id: tsearch.c,v 2.43 1995/09/09 21:23:16 liblit Exp $";
#endif /* lint */

struct spWclass *tsearch_class = 0;

int m_tsearch_setText;
int m_tsearch_setTextPos;
int m_tsearch_textPos;

#define SPELL_TMP "zm.spl"

#define isreadonly(ts) (spText_readOnly(spView_observed((ts)->theText)))

#define DONE_B(ts)    ((ts),0)
#define SEARCH_B(ts)  ((ts),1)
#define REPLACE_B(ts) (isreadonly(ts) ? -1 : 2)
#define SPELL_B(ts)   (isreadonly(ts) ? -1 : 3)
#define CLEAR_B(ts)   (isreadonly(ts) ?  2 : 4)
#define HELP_B(ts)    (isreadonly(ts) ?  3 : 5)

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static void
doSearch(self)
    struct tsearch *self;
{
    static struct dynstr saved;
    static int initialized = 0;
    struct dynstr d;
    
    if (!initialized) {
	initialized = 1;
	dynstr_Init(&saved);
    }
    dynstr_Init(&d);
    spSend(spView_observed(self->probe), m_spText_appendToDynstr, &d, 0, -1);
    spSend(self->theText, m_spView_invokeInteraction,
	   "text-deselect", 0, 0, 0);
    if (dynstr_Length(&d) > 0) {
	char *errmsg;
	char fastmap[256];
	int pos;
	regexp_t rxpbuf = (regexp_t) emalloc(sizeof (*rxpbuf),
					     "text search");
	TRY {
	    rxpbuf->buffer = NULL;
	    rxpbuf->allocated = 0;
	    rxpbuf->translate = NULL;
	    rxpbuf->fastmap = fastmap;

	    if ((!dynstr_Length(&saved)) || strcmp(dynstr_Str(&d),
						   dynstr_Str(&saved))) {
		dynstr_Set(&saved, dynstr_Str(&d));
		spSend(spView_observed(self->theText), m_spText_setMark,
		       spTextview_textPosMark(self->theText), 0);
	    }
	    if (errmsg = re_compile_pattern(dynstr_Str(&d), dynstr_Length(&d),
					    rxpbuf)) {
		error(UserErrWarning, errmsg);
	    } else {
		int start = spText_markPos(((struct spText *)
					    spView_observed(self->theText)),
					   spTextview_textPosMark(self->theText));
		int end;

		re_compile_fastmap(rxpbuf);
		if (((pos = spSend_i(spView_observed(self->theText),
				     m_spText_rxpSearch, rxpbuf, start,
				     &end)) >= 0)
		    || ((pos = spSend_i(spView_observed(self->theText),
					m_spText_rxpSearch, rxpbuf,
					0, &end)) >= 0)) {
		    self->lastfind.pos = pos;
		    self->lastfind.len = end - pos;

		    spSend(spView_observed(self->theText),
			   m_spText_setMark,
			   spTextview_textPosMark(self->theText), pos);
		    spSend(self->theText, m_spView_invokeInteraction,
			   "text-start-selecting", 0, 0, 0);
		    spSend(spView_observed(self->theText),
			   m_spText_setMark,
			   spTextview_textPosMark(self->theText), end);
		    spSend(self->theText, m_spView_invokeInteraction,
			   "text-stop-selecting", 0, 0, 0);
		    spSend(self->theText, m_spView_wantUpdate,
			   self->theText, 1 << spTextview_cursorMotion);
		    spSend(ZmlIm, m_spIm_showmsg,
			   zmVaStr(catgets(catalog, CAT_LITE, 418, "Pattern found at offset %d"),
				   pos), 15, 2, 0);
		    if (spView_window(self->replacement))
			spSend(self->replacement, m_spView_wantFocus,
			       self->replacement);
		    else
			spSend(self->theText, m_spView_wantFocus,
			       self->theText);
		} else {
		    spSend(ZmlIm, m_spIm_showmsg, catgets(catalog, CAT_LITE, 419, "Pattern not found"), 15, 1, 5);
		}
	    }
	} FINALLY {
	    if (rxpbuf->buffer)
		free(rxpbuf->buffer);
	    free(rxpbuf);
	} ENDTRY;
    } else {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 420, "Specify a search pattern"));
    }
    dynstr_Destroy(&d);
}

static void
doReplace(self)
    struct tsearch *self;
{
    struct dynstr repl;
    
    if (self->lastfind.pos < 0) {
	error(UserErrWarning,
	      catgets(catalog, CAT_LITE, 421, "First find an occurrence of the search pattern"));
	return;
    }

    spSend(spView_observed(self->theText), m_spText_delete,
	   self->lastfind.pos, self->lastfind.len);
    dynstr_Init(&repl);
    spSend(spView_observed(self->replacement), m_spText_appendToDynstr,
	   &repl, 0, -1);
    spSend(spView_observed(self->theText), m_spText_insert,
	   self->lastfind.pos, -1, dynstr_Str(&repl), spText_mBefore);

    spSend(spView_observed(self->theText),
	   m_spText_setMark,
	   spTextview_textPosMark(self->theText),
	   self->lastfind.pos + dynstr_Length(&repl));
    spSend(self->theText, m_spView_wantUpdate,
	   self->theText, 1 << spTextview_cursorMotion);
    spSend(ZmlIm, m_spIm_showmsg,
	   zmVaStr(catgets(catalog, CAT_LITE, 422, "Replaced at offset %d"), self->lastfind.pos),
	   15, 1, 5);
    
    self->lastfind.pos = -1;

    dynstr_Destroy(&repl);
    spSend(self->probe, m_spView_wantFocus, self->probe);
}

static void
probeActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct tsearch *ts = (struct tsearch *) spCmdline_obj(self);

    doSearch(ts);
}

static void
wordsCallback(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct tsearch *ts = (struct tsearch *) spView_callbackData(self);
    struct dynstr d;

    dynstr_Init(&d);
    spSend(spView_observed(self), m_spList_getNthItem, which, &d);
    spSend(spView_observed(ts->probe), m_spText_clear);
    spSend(spView_observed(ts->probe), m_spText_insert, 0, -1,
	   dynstr_Str(&d), spText_mBefore);
    dynstr_Destroy(&d);
    spSend(ts->probe, m_spView_wantFocus, ts->probe);
}

static void
replacementActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    struct tsearch *ts = (struct tsearch *) spCmdline_obj(self);

    doReplace(ts);
}

static void
doSpell(self)
    struct tsearch *self;
{
    char *file = NULL;
    FILE *fp, *pp = (FILE *) 0;

    spSend(spView_observed(self->words), m_spText_clear);
    if (!(fp = open_tempfile(SPELL_TMP, &file))) {
	error(SysErrWarning, catgets(catalog, CAT_LITE, 423, "Can't open tempfile."));
	return;
    }
   
    TRY {
	char *spell_prog = get_var_value(VarSpeller);
	char buf[2 * MAXPATHLEN + 4];

	(void) fclose(fp);
	spSend(self->theText, m_spTextview_writeFile, file);
	if (!spell_prog)
	    spell_prog = SPELL_CHECKER;
	sprintf(buf, "%s %s", spell_prog, file);
	if (!(pp = popen(buf, "r"))) {
	    error(SysErrWarning, catgets(catalog, CAT_LITE, 424, "Can't use spelling checker now"));
	} else {
	    int n;
	    char *p;

	    timeout_cursors(1);
	    spSend(ZmlIm, m_spIm_showmsg, catgets(catalog, CAT_LITE, 425, "Checking spelling..."),
		   15, 1, 0);
	    spSend(ZmlIm, m_spIm_forceUpdate, 0);
	    for (n = 0; p = fgets(buf, sizeof(buf), pp); ++n) {
		p[strlen(p) - 1] = '\0';
		spSend(spView_observed(self->words), m_spList_append, p);
	    }
	    spSend(ZmlIm, m_spIm_showmsg,
		   zmVaStr(catgets(catalog, CAT_LITE, 426, "Found %d spelling errors"), n),
		   15, 2, 5);
	    spSend(self->words, m_spView_wantFocus, self->words);
	}
    } FINALLY {
	unlink(file);
	xfree(file);
	if (pp)
	    pclose(pp);
    } ENDTRY;
}

static void
aa_done(b, self)
    struct spButton *b;
    struct tsearch *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_search(b, self)
    struct spButton *b;
    struct tsearch *self;
{
    doSearch(self);
}

static void
aa_replace(b, self)
    struct spButton *b;
    struct tsearch *self;
{
    doReplace(self);
}

static void
aa_spell(b, self)
    struct spButton *b;
    struct tsearch *self;
{
    doSpell(self);
}

static void
aa_clear(b, self)
    struct spButton *b;
    struct tsearch *self;
{
    spSend(spView_observed(self->probe), m_spText_clear);
    spSend(spView_observed(self->replacement), m_spText_clear);
    spSend(spView_observed(self->words), m_spText_clear);
    /* Indicate no search has taken place */
}

static void
aa_help(b, self)
    struct spButton *b;
    struct tsearch *self;
{
    if (spView_getWclass((struct spView *) self) == spwc_Textsearch)
	zmlhelp("Searching");
    else
	zmlhelp("Search and Replace");
}

static void
tsearch_initialize(self)
    struct tsearch *self;
{
    ZmlSetInstanceName(self, "textsearch", self);

    self->lastfind.pos = -1;
    self->lastfind.len = -1;

    self->theText = spTextview_NEW();
    ZmlSetInstanceName(self->theText, "textsearch-text", self);

    spSend(self->probe = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spCmdline_fn(self->probe) = probeActivate;
    spCmdline_obj(self->probe) = (struct spoor *) self;
    ZmlSetInstanceName(self->probe, "textsearch-pattern-field", self);

    spSend(self->replacement = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spCmdline_fn(self->replacement) = replacementActivate;
    spCmdline_obj(self->replacement) = (struct spoor *) self;
    ZmlSetInstanceName(self->replacement, "textsearch-replacement-field",
		       self);

    spSend(self->words = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_callback(self->words) = wordsCallback;
    spView_callbackData(self->words) = (struct spoor *) self;
    spListv_okclicks(self->words) = (1 << spListv_click);
    ZmlSetInstanceName(self->words, "textsearch-word-list", self);

    self->probeWrap = spWrapview_NEW();
    self->textWrap = spWrapview_NEW();

    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_dialog_setView,
	   Split(self->probeWrap,
		 self->textWrap,
		 5, 1, 0,
		 spSplitview_topBottom,
		 spSplitview_boxed,
		 spSplitview_SEPARATE));
}

static void
tsearch_finalize(self)
    struct tsearch *self;
{
    /* Undo everything! */
}

static char field_template[] = "%*s ";
static void
tsearch_setText(self, arg)
    struct tsearch *self;
    spArgList_t arg;
{
    struct spText *t = spArg(arg, struct spText *);
    struct spView *v;
    struct spView *p = spView_parent(self->probeWrap);
    struct spButtonv *oldaa;
    char *str[2];
    int field_length;


    spSend(self->theText, m_spView_setObserved, t);
    if (!t)
	return;
    if (v = spWrapview_view(self->probeWrap)) {
	spSend(self->probeWrap, m_spWrapview_setView, NULL);
	KillSplitviewsAndWrapviews(v);
    }
    if (v = spWrapview_view(self->textWrap)) {
	spSend(self->textWrap, m_spWrapview_setView, NULL);
	KillSplitviewsAndWrapviews(v);
    }
    str[0] = catgets(catalog, CAT_LITE, 428, "Search:");
    str[1] = catgets(catalog, CAT_LITE, 431, "Replace:");
    if (spText_readOnly(t)) {
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 427, "Text Search"), spWrapview_top);
	if (spoor_IsClassMember(p, spSplitview_class)) {
	    struct spView *c1, *c2;

	    c1 = spSplitview_child(p, 0);
	    c2 = spSplitview_child(p, 1);
	    spSend(p, m_spSplitview_setup, c1, c2,
		   1, (c1 == (struct spView *) self->probeWrap) ? 0 : 1, 0,
		   spSplitview_topBottom,
		   spSplitview_boxstyle(p), spSplitview_borders(p));
	}
	spSend(self->probeWrap, m_spWrapview_setView, self->probe);
        field_length = strlen(str[0]);
	spSend(self->probeWrap, m_spWrapview_setLabel, 
               zmVaStr(field_template, field_length, str[0]),
	       spWrapview_left);
	spSend(self->textWrap, m_spWrapview_setView, self->theText);
    } else {
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 429, "Search/Replace/Spell"),
	       spWrapview_top);
	if (spoor_IsClassMember(p, spSplitview_class)) {
	    struct spView *c1, *c2;

	    c1 = spSplitview_child(p, 0);
	    c2 = spSplitview_child(p, 1);
	    spSend(p, m_spSplitview_setup, c1, c2,
		   2, (c1 == (struct spView *) self->probeWrap) ? 0 : 1, 0,
		   spSplitview_topBottom,
		   spSplitview_boxstyle(p), spSplitview_borders(p));
	}
	spSend(self->probeWrap, m_spWrapview_setLabel, "", spWrapview_left);
        field_length = max(strlen(str[0]), strlen(str[1]));
	spSend(self->probeWrap, m_spWrapview_setView,
	       Split(Wrap(self->probe, NULL, NULL, 
                     zmVaStr(field_template, field_length, str[0]),
                     NULL, 0, 0, 0),
		     Wrap(self->replacement, NULL, NULL, 
                     zmVaStr(field_template, field_length, str[1]),
                     NULL, 0, 0, 0),
		     1, 0, 0,
		     spSplitview_topBottom, spSplitview_plain, 0));
	spSend(self->textWrap, m_spWrapview_setView,
	       Split(self->theText,
		     Wrap(self->words, catgets(catalog, CAT_LITE, 432, "Misspellings"),
			  NULL, NULL, NULL, 1, 1, 0),
		     20, 1, 0,
		     spSplitview_leftRight, spSplitview_plain, 0));
    }

    if (oldaa = ((struct spButtonv *)
		 spSend_p(self, m_dialog_setActionArea,
			ActionArea(self,
				   catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
				   catgets(catalog, CAT_LITE, 25, "Search"), aa_search,
				   0)))) {
	spSend(oldaa, m_spView_destroyObserved);
	spoor_DestroyInstance(oldaa);
    }
    spButtonv_selection(dialog_actionArea(self)) = 1;
    ZmlSetInstanceName(dialog_actionArea(self), "textsearch-aa", self);

    if (spText_readOnly(t)) {
	spSend(self, m_spView_setWclass, spwc_Textsearch);
    } else {
	spSend(self, m_spView_setWclass, spwc_TextsearchReplace);
	spSend(dialog_actionArea(self), m_spButtonv_insert,
	       spButton_Create(catgets(catalog, CAT_LITE, 435, "Replace"), aa_replace, self), -1);
	spSend(dialog_actionArea(self), m_spButtonv_insert,
	       spButton_Create(catgets(catalog, CAT_LITE, 436, "Spell"), aa_spell, self), -1);
    }

    spSend(dialog_actionArea(self), m_spButtonv_insert,
	   spButton_Create(catgets(catalog, CAT_LITE, 26, "Clear"), aa_clear, self), -1);
    spSend(dialog_actionArea(self), m_spButtonv_insert,
	   spButton_Create(catgets(catalog, CAT_LITE, 17, "Help"), aa_help, self), -1);

    spSend(self, m_dialog_clearFocusViews);
    spSend(self, m_dialog_addFocusView, self->probe);
    if (!isreadonly(self))
	spSend(self, m_dialog_addFocusView, self->replacement);
    spSend(self, m_dialog_addFocusView, self->theText);
    if (!isreadonly(self))
	spSend(self, m_dialog_addFocusView, self->words);
}

static void
tsearch_setTextPos(self, arg)
    struct tsearch *self;
    spArgList_t arg;
{
    int pos;

    pos = spArg(arg, int);
    spSend(spView_observed(self->theText), m_spText_setMark,
	   spTextview_textPosMark(self->theText), pos);
}

static int
tsearch_textPos(self, arg)
    struct tsearch *self;
    spArgList_t arg;
{
    return (spText_markPos((struct spText *) spView_observed(self->theText),
			   spTextview_textPosMark(self->theText)));
}

static void
tsearch_activate(self, arg)
    struct tsearch *self;
    spArgList_t arg;
{
    if (CurrentDialog
	&& spoor_IsClassMember(CurrentDialog,
			       (struct spClass *) zmlmsgframe_class)) {
	spSend(self, m_tsearch_setText,
	       spView_observed(zmlmsgframe_body(spIm_view(ZmlIm))));
    } else if (CurrentDialog
	       && spoor_IsClassMember(CurrentDialog,
				      (struct spClass *) zmlcomposeframe_class)) {
	spSend(self, m_tsearch_setText,
	       spView_observed(zmlcomposeframe_body(spIm_view(ZmlIm))));
    }
    spSend(self->probe, m_spView_wantFocus, self->probe);
    spSuper(tsearch_class, self, m_dialog_activate);
}

static void
tsearch_desiredSize(self, arg)
    struct tsearch *self;
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

    spSuper(tsearch_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    *minh = MAX(16, *minh);
    *besth = screenh - 4;
    *bestw = screenw - 4;
}

static void
tsearch_deactivate(self, arg)
    struct tsearch *self;
    spArgList_t arg;
{
    int val = spArg(arg, int);

    spSuper(tsearch_class, self, m_dialog_deactivate, val);
    spSend(self, m_tsearch_setText, 0);
}

struct spWidgetInfo *spwc_Textsearch = 0;
struct spWidgetInfo *spwc_TextsearchReplace = 0;

void
tsearch_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (tsearch_class)
	return;
    tsearch_class =
	spWclass_Create("tsearch", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct tsearch)),
			tsearch_initialize,
			tsearch_finalize,
			spwc_Textsearch = spWidget_Create("Textsearch",
							  spwc_Popup));

    spwc_TextsearchReplace = spWidget_Create("TextsearchReplace",
					     spwc_Textsearch);

    m_tsearch_setText =
	spoor_AddMethod(tsearch_class, "setText",
			NULL,
			tsearch_setText);
    m_tsearch_setTextPos =
	spoor_AddMethod(tsearch_class, "setTextPos",
			NULL,
			tsearch_setTextPos);
    m_tsearch_textPos =
	spoor_AddMethod(tsearch_class, "textPos",
			NULL,
			tsearch_textPos);

    spoor_AddOverride(tsearch_class,
		      m_dialog_deactivate, NULL,
		      tsearch_deactivate);
    spoor_AddOverride(tsearch_class, m_dialog_activate, NULL,
		      tsearch_activate);
    spoor_AddOverride(tsearch_class, m_spView_desiredSize, NULL,
		      tsearch_desiredSize);

    zmlmsgframe_InitializeClass();
    zmlcomposeframe_InitializeClass();
    spSplitview_InitializeClass();
    spText_InitializeClass();
    spCmdline_InitializeClass();
    spWrapview_InitializeClass();
    spPopupView_InitializeClass();
    spButton_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
