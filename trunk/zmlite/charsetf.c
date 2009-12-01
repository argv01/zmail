#include <spoor.h>
#include <charsetf.h>

#include "c3/c3.h"
#include "c3/c3_trans.h"
#include <composef.h>
#include <msgf.h>
#include <zmlutil.h>
#include <zmail.h>
#include <spoor/splitv.h>
#include <spoor/wrapview.h>
#include <spoor/cmdline.h>
#include <spoor/text.h>
#include <spoor/buttonv.h>
#include <spoor/toggle.h>
#include <spoor/popupv.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <dynstr.h>
#include <zmlite.h>

#include "catalog.h"
#include "mime.h"

static const char charsets_rcsid[] =
    "$Id: charsetf.c,v 2.12 1995/10/20 00:35:14 schaefer Exp $";

static struct glist CharsetNoPos;

#define charset_no_pos(n) (*((int *) glist_Nth(&CharsetNoPos,(n))))

struct spWclass *charsets_class = 0;
struct spWidgetInfo *spwc_Charset = 0;
struct spWidgetInfo *spwc_MessageCharset = 0;
struct spWidgetInfo *spwc_ComposeCharset = 0;
static mimeCharSet message_cs;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

enum {
    DONE_B, CANCEL_B, HELP_B
};


/* 
 * hack here to find the first attachment that looks OK 
 * output is character set in first TextPlain attachment
 */

#define MAX_ATTACH 10

Attach *
get_Text_attach(origap)
    Attach *origap;
{
    int failsafe = 0;
    Attach *ap = origap;

    while (ap && ap->mime_type != TextPlain && 
		ap->a_link.l_next && 
		(Attach *)ap->a_link.l_next != origap &&
		(++failsafe) < MAX_ATTACH) {
	ap = (Attach *)ap->a_link.l_next;
    }
    if (!ap || ap->mime_type != TextPlain)
	ap = origap;
    return (ap);
}

static void
aa_ok(b, self)
    struct spButton *b;
    struct charsets *self;
{
    const char *cs_name;
    mimeCharSet cs;
    int num = spListv_lastclick(self->list);

    if (num < 0 || num > glist_Length(&CharsetNoPos))
	cs = UnknownMimeCharSet;
    else
	cs = charset_no_pos(num);

    cs_name = GetMimeCharSetName(cs);

    if (self->composep) {
	if (cs_name != NULL && (cs != comp_current->out_char_set))
	    ZCommand(zmVaStr("\\compcmd charset %s", cs_name), zcmd_commandline);
    }
    else {
	mimeCharSet saved_charset;
        Msg *mymsg = (Msg *) spSend_p(spIm_view(ZmlIm), m_zmlmsgframe_msg);
	Attach *ap;


	if (mymsg->m_attach != NULL) {
#if 0 
  	    ap = get_Text_attach(mymsg->m_attach);
	    saved_charset = ap->mime_char_set;
	    ap->mime_char_set = cs;
	/* XXX not sure how to get an update of the message cache yet */
	    message_HeaderCacheInsert(mymsg, 
#endif
	    ZCommand(zmVaStr("\\set __override_charset_label = %s ; display %d ; unset __override_charset_label", cs_name, (current_msg+1)), zcmd_commandline);
#if 0 
	    ap->mime_char_set = saved_charset;
#endif
	}
	else {
	    saved_charset = inMimeCharSet;
	    inMimeCharSet = cs;
	    ZCommand(zmVaStr("\\display %d", (current_msg+1)), zcmd_commandline);
	    inMimeCharSet = saved_charset;
	}
    }
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
listCallback(self, num, clicktype)
    struct spListv *self;
    int num;
    enum spListv_clicktype clicktype;
{
    struct charsets *cs = (struct charsets *) spView_callbackData(self);

    if (clicktype == spListv_doubleclick) {
	aa_ok(NULL, cs);
    }
}

#if 0 
/* only do this via zscript right now.  Keep here just in case though */
static void
aa_sender(b, self)
    struct spButton *b;
    struct charsets *self;
{
    const char *cs_name;
    mimeCharSet cs;

    if (self->composep) {
    	cs = inMimeCharSet;		/* XXX this isn't correct !!! */
    	cs_name = GetMimeCharSetName(cs);

	if (cs_name != NULL && (cs != comp_current->out_char_set))
	    ZCommand(zmVaStr("\\compcmd charset %s", cs_name), zcmd_commandline);
    }
    spSend(self, m_dialog_deactivate, dialog_Close);
}
#endif

static void
aa_cancel(b, self)
    struct spButton *b;
    struct charsets *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
aa_help(b, self)
    struct spButton *b;
    struct charsets *self;
{
    if (self->composep)
    	zmlhelp("charset_compose");
    else
    	zmlhelp("charset_message");
}


static void
setup_list(self)
    struct charsets *self;
{
    struct spListv *list = (struct spListv *) spView_observed(self->list);
    char *list_entry;
    int list_length;
    int l, i, pos = 0;
    int c3OK;

    list_length = c3_cs_list_length();

    for (i = 0; i < list_length; i++) { 
	mimeCharSet cs = c3_cs_list(i);
	/* 
	 * if compose window list all know charsets
	 * else if message, only list the ones c3 knows about
	 * and can do something about them, plus 
	 * the current charset, plus the display charset
	 */
	if (self->composep)
	    c3OK = C3_TRANSLATION_POSSIBLE(displayCharSet, cs) ||
		   C3_TRANSLATION_POSSIBLE(fileCharSet, cs);
	else
	    c3OK = (C3_TRANSLATION_POSSIBLE(cs, displayCharSet) ||
		    cs == message_cs ||
		    cs == displayCharSet);
	if (self->composep || c3OK) {
	    glist_Set(&CharsetNoPos, pos, &cs);
	    list_entry = zmVaStr("%-30s %c(%s)", 
#ifdef C3
			c3_nicename_from_cs(cs),
#else
			cs,
#endif
			(c3OK ? ' ' : '*'),
			GetMimeCharSetName(cs));
	    if (pos < spSend_i(list, m_spList_length))
	        spSend(list, m_spList_replace, pos, list_entry);
	    else
	        spSend(list, m_spList_append, list_entry);
	    pos++;
    	}
    }

    /* Now `pos' is the desired length of the list */
    while ((l = spSend_i(list, m_spList_length)) > pos)
	spSend(list, m_spList_remove, l - 1);
}

static char charset_err_BadContext[] = "charset_err_BadContext";

static void
charsets_initialize(self)
    struct charsets *self;
{
    struct spText *t;
    int minh=0, minw=0, maxh=0, maxw=0, besth=0, bestw=0;

    if (self->composep =
	spoor_IsClassMember(spIm_view(ZmlIm), 
			    (struct spClass *) zmlcomposeframe_class)) {
	spSend(self, m_spView_setWclass, spwc_ComposeCharset);
    } else if (spoor_IsClassMember(spIm_view(ZmlIm),
				   (struct spClass *) zmlmsgframe_class)) {
	spSend(self, m_spView_setWclass, spwc_MessageCharset);
    } else {
	RAISE(charset_err_BadContext, 0);
    }

    ZmlSetInstanceName(self, "charset", self);

    spSend(self->list = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spListv_okclicks(self->list) = ((1 << spListv_click)
					| (1 << spListv_doubleclick));
    spListv_callback(self->list) = listCallback;
    spView_callbackData(self->list) = (struct spoor *) self;
    ZmlSetInstanceName(self->list, "charset-list", self);

    setup_list(self);

    spSend(self->instructions = spTextview_NEW(), m_spView_setObserved,
	       t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setActionArea,
	       ActionArea(self,
			  catgets(catalog, CAT_LITE, 64, "Ok"), aa_ok,
			  catgets(catalog, CAT_LITE, 65, "Cancel"), aa_cancel,
			  catgets(catalog, CAT_LITE, 17, "Help"), aa_help,
			  0));
	spButtonv_selection(dialog_actionArea(self)) = 0;
	ZmlSetInstanceName(dialog_actionArea(self), "charset-aa", self);

	spWrapview_boxed(self) = 1;
	spWrapview_highlightp(self) = 1;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 868, "Character Sets"), spWrapview_top);

       spSend(self->list, m_spView_desiredSize,
	      &minh, &minw, &maxh, &maxw, &besth, &bestw);

	spSend(self, m_dialog_setView, 
	      Split(self->instructions,
	      self->list,
	      2, 0, 0,			/* not good, but should be OK */
	      spSplitview_topBottom,
	      spSplitview_boxed, spSplitview_SEPARATE));

    } dialog_ENDMUNGE;

    spSend(self, m_dialog_addFocusView, self->list);
}


static void
charsets_finalize(self)
    struct charsets *self;
{
    struct spView *v = dialog_view(self);
    struct spList *l = (struct spList *) spView_observed(self->list);

    spSend(self, m_dialog_setView, 0);
    KillSplitviewsAndWrapviews(v);
    spoor_DestroyInstance(self->list);
    spoor_DestroyInstance(l);
}

static void
charsets_desiredSize(self, arg)
    struct charsets *self;
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

    spSuper(charsets_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    if (*besth < (screenh - 14))
	*besth = screenh - 14;
    if (*bestw < (screenw - 16))
	*bestw = screenw - 16;
}

static void
select_charset(self, cs)
    struct charsets *self;
    mimeCharSet cs;
{
    int i,l,gl;

    l = spSend_i(spView_observed(self->list),  m_spList_length);
    gl = glist_Length(&CharsetNoPos);
    for (i = 0;  i < MIN(l,gl); i++)
	if (cs == charset_no_pos(i)) {
	    char buf[10];
	    spSend(self->list, m_spListv_select, i); 
	    sprintf(buf, "%d", i + 1);
	    spSend(self->list, m_spView_invokeInteraction,
		"list-click-line", self, buf, 0);

	    break;
   	 }
}

static void
charsets_activate(self, arg)
    struct charsets *self;
    spArgList_t arg;
{
    if (self->composep =
	spoor_IsClassMember(spIm_view(ZmlIm), 
			    (struct spClass *) zmlcomposeframe_class)) {
	spSend(self, m_spView_setWclass, spwc_ComposeCharset);
    } else if (spoor_IsClassMember(spIm_view(ZmlIm),
				   (struct spClass *) zmlmsgframe_class)) {
	spSend(self, m_spView_setWclass, spwc_MessageCharset);
    } else {
	RAISE(charset_err_BadContext, 0);
	return;
    }

    spSend(spView_observed(self->instructions), m_spText_clear);
    if (self->composep) {
	message_cs = NoMimeCharSet;

	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 869, "Outgoing Character Sets"), spWrapview_top);
	spSend(spView_observed(self->instructions), m_spText_insert, 0, -1,
		   zmVaStr(catgets(catalog, CAT_LITE, 870, "Default: %s  Current: %s\nSelect a character set for this message:"), GetMimeCharSetName(outMimeCharSet), GetMimeCharSetName(comp_current->out_char_set)),
		   spText_mAfter);

    	setup_list(self);
	select_charset(self, comp_current->out_char_set);
    }
    else {

	char *orig_csname;
        Attach *myattach = ((Msg *) spSend_p(spIm_view(ZmlIm), m_zmlmsgframe_msg))->m_attach;
	spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 871, "Incoming Character Sets"), spWrapview_top);
	if (myattach == NULL) {
	    orig_csname = catgets(catalog, CAT_LITE, 872, "None");
	    message_cs = inMimeCharSet;
	}
	else {
	    Attach *ap = get_Text_attach(myattach);
	    orig_csname = FindParam(MimeTextParamStr(CharSetParam),
			&ap->content_params),
	    message_cs = ap->mime_char_set;
	}
    	setup_list(self);
	select_charset(self, message_cs);
#ifdef NOT_NOW
	spSend(spView_observed(self->instructions), m_spText_insert, 0, -1,
		zmVaStr(catgets(catalog, CAT_LITE, 873, "Original: %s  Current: %s\nSelect a character set for this message:"), 
	    	orig_csname, GetMimeCharSetName(message_cs)), spText_mAfter);
#else
	spSend(spView_observed(self->instructions), m_spText_insert, 0, -1,
		zmVaStr(catgets(catalog, CAT_LITE, 874, "Original: %s\nSelect a character set for this message:"), 
	    	orig_csname), spText_mAfter);
#endif
    }

    spButtonv_selection(dialog_actionArea(self)) = 0;
    spSuper(charsets_class, self, m_dialog_activate);
}



void
charsets_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (charsets_class)
	return;
    charsets_class =
	spWclass_Create("charsets", NULL,
			(struct spClass *) dialog_class,
			(sizeof (struct charsets)),
			charsets_initialize,
			charsets_finalize,
			spwc_Charset = spWidget_Create("Charset",
						       spwc_Popup));
    spwc_ComposeCharset = spWidget_Create("ComposeCharset",
					  spwc_Charset);
    spwc_MessageCharset = spWidget_Create("MessageCharset", 
					  spwc_Charset);

    spoor_AddOverride(charsets_class,
		      m_dialog_activate, NULL,
		      charsets_activate);
    spoor_AddOverride(charsets_class,
		      m_spView_desiredSize, NULL,
		      charsets_desiredSize);

    spSplitview_InitializeClass();
    spWrapview_InitializeClass();
    spCmdline_InitializeClass();
    spText_InitializeClass();
    spButtonv_InitializeClass();
    spToggle_InitializeClass();
    spPopupView_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();

    glist_Init(&CharsetNoPos, (sizeof (int)), 32);
}
