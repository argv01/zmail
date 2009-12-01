/*
 * $RCSfile: zmcot.c,v $
 * $Revision: 2.28 $
 * $Date: 1995/07/08 01:23:20 $
 * $Author: spencer $
 */

#include <spoor.h>
#include <zmcot.h>
#include <spoor/text.h>
#include <spoor/splitv.h>
#include <spoor/list.h>
#include <spoor/listv.h>
#include <spoor/event.h>

#include <dynstr.h>
#include <strcase.h>
#include <zmlite.h>
#include <zmlutil.h>
#include <dirserv.h>
#include <zfolder.h>

#include "catalog.h"

#ifdef ZMCOT

#ifndef lint
static const char zmcot_rcsid[] =
    "$Id: zmcot.c,v 2.28 1995/07/08 01:23:20 spencer Exp $";
#endif /* lint */

struct spWclass *zmcot_class = 0;
struct spWclass *chevdir_class = 0;

#define Split spSplitview_Create
#define Wrap spWrapview_Create

extern char *get_name_n_addr(), *bang_form();

int m_zmcot_clear;
int m_zmcot_append;
int m_zmcot_setmsg;

static struct spEvent *active_event = 0;

static int
go_inactive(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    char *str = get_var_value("inactive_timeout");
    int secs = (str ? atoi(str) : 60);

    if (secs < 5)
	secs = 5;
    ZCommand(zmVaStr("set timeout = %d", secs), zcmd_ignore);
    active_event = 0;
    return (1);
}

void
zmcot_active()
{
    char *str = get_var_value("inactive_interval");
    int secs;

    secs = (str ? atoi(str) : 300);
    if (secs < 30)
	secs = 30;

    if (active_event && spEvent_inqueue(active_event)) {
	spSend(active_event, m_spEvent_cancel, 1);
    }
    active_event = spEvent_Create((long) secs, (long) 0, 1,
				  go_inactive, 0);
    spSend(ZmlIm, m_spIm_enqueueEvent, active_event);

    str = get_var_value("active_timeout");
    secs = (str ? atoi(str) : 5);
    if (secs < 1)
	secs = 1;
    ZCommand(zmVaStr("set timeout = %d", secs), zcmd_ignore);
}

static void
jumpNext(self, str)
    struct spCmdline *self;
    char *str;
{
    spSend(ZmlIm, m_spView_invokeInteraction, "focus-next",
	   self, 0, 0);
}

static void
grabTo(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    static struct dynstr d;
    static int initialized = 0;

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }
    dynstr_Set(&d, "");
    spSend(spView_observed(self->to), m_spText_appendToDynstr, &d, 0, -1);
    if (!dynstr_EmptyP(&d)) {
	dynstr_Replace(&d, 0, 0, "set lastAuthorSearch = '");
	dynstr_AppendChar(&d, '\'');
	ZCommand(dynstr_Str(&d), zcmd_ignore);
    }
}

static void
zmcot_initialize(self)
    struct zmcot *self;
{
    struct spSplitview *split;
    struct spWrapview *wrap;
    struct spText *t;

    self->is_from_trader = 0;

    self->currentDisplayed.fldr = (msg_folder *) 0;
    self->currentDisplayed.num = -1;

    self->lastDisplayed.fldr = (msg_folder *) 0;
    self->lastDisplayed.num = -1;

    spSend(self->to = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spSend(self->subject = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spSend(self->composeBody = spTextview_NEW(), m_spView_setObserved,
	   spText_NEW());
    spSend(self->receiveBody = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spSend(self->status.top = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spSend(self->status.fldr = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spSend(self->rechdrs.to = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);
    spSend(self->rechdrs.from = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spTextview_wrapmode(self->rechdrs.from) = spTextview_nowrap;
    spSend(t, m_spText_setReadOnly, 1);
    spSend(self->rechdrs.subject = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spTextview_wrapmode(self->rechdrs.subject) = spTextview_nowrap;
    spSend(t, m_spText_setReadOnly, 1);
    spSend(self->rechdrs.date = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);

    dialog_MUNGE(self) {
	spSend(self, m_dialog_setView, split = spSplitview_NEW());

	split = SplitAdd(split, self->status.top, 1, 0, 0,
			 spSplitview_topBottom, spSplitview_plain, 0);
	split = SplitAdd(split, self->status.fldr, 1, 0, 0,
			 spSplitview_topBottom, spSplitview_plain, 0);
	wrap = spWrapview_NEW();
	spSend(wrap, m_spWrapview_setLabel, CATGETS("To:"), spWrapview_left);
	spWrapview_highlightp(wrap) = 1;
	spSend(wrap, m_spWrapview_setView, self->to);
	split = SplitAdd(split, wrap, 1, 0, 0, spSplitview_topBottom,
			 spSplitview_plain, 0);
	wrap = spWrapview_NEW();
	spSend(wrap, m_spWrapview_setLabel, CATGETS("Subject:"), spWrapview_left);
	spWrapview_highlightp(wrap) = 1;
	spSend(wrap, m_spWrapview_setView, self->subject);
	split = SplitAdd(split, wrap, 1, 0, 0, spSplitview_topBottom,
			 spSplitview_plain, 0);

	split = SplitAdd(split, self->receiveBody, 50, 1, 1,
			 spSplitview_topBottom, spSplitview_plain, 0);
	spSend(split, m_spSplitview_setup, self->composeBody,
	       Split(Split(Wrap(self->rechdrs.from, NULL, NULL,
					    CATGETS("   From:"), 0, 1, 0, 0),
				   Wrap(self->rechdrs.to, NULL, NULL,
					    "  To:", 0, 1, 0, 0),
				   40, 0, 0, spSplitview_leftRight,
				   spSplitview_plain, 0),
			 Split(Wrap(self->rechdrs.subject, NULL, NULL,
					    CATGETS("Subject:"), 0, 1, 0, 0),
				   Wrap(self->rechdrs.date, NULL, NULL,
					    CATGETS("Date:"), 0, 1, 0, 0),
				   40, 0, 0, spSplitview_leftRight,
				   spSplitview_plain, 0),
			 1, 0, 0, spSplitview_topBottom, spSplitview_plain, 0),
	       2, 1, 0, spSplitview_topBottom,
	       spSplitview_plain, 0);
    } dialog_ENDMUNGE;

    spCmdline_fn(self->to) = spCmdline_fn(self->subject) = jumpNext;

    self->timeEvent = (struct spEvent *) 0;

    ZmlSetInstanceName(self, "zmcot", 0);

    ZmlSetInstanceName(self->receiveBody, "zmcot-body", self);
    ZmlSetInstanceName(self->to, "zmcot-to", self);
    ZmlSetInstanceName(self->subject, "zmcot-subject", self);
    ZmlSetInstanceName(self->composeBody, "zmcot-compose-body", self);

    spSend(self, m_dialog_addFocusView, self->receiveBody);
    spSend(self, m_dialog_addFocusView, self->to);
    spSend(self, m_dialog_addFocusView, self->subject);
    spSend(self, m_dialog_addFocusView, self->composeBody);
}

static void
zmcot_finalize(self)
    struct zmcot *self;
{
    /* To do: deallocate everything */
}

static catalog_ref monthmap[] = {
    CATREF("Jan"),
    CATREF("Feb"),
    CATREF("Mar"),
    CATREF("Apr"),
    CATREF("May"),
    CATREF("Jun"),
    CATREF("Jul"),
    CATREF("Aug"),
    CATREF("Sep"),
    CATREF("Oct"),
    CATREF("Nov"),
    CATREF("Dec")
};

static int
doTime(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    char buf[128];
    struct zmcot *chev = (struct zmcot *) spEvent_data(ev);
    time_t now;
    struct tm *tmstruct;
    static char hostnamebuf[128], *hostname = NULL;

    if (!hostname) {
	if (gethostname(hostnamebuf, 127) == 0)
	    hostname = hostnamebuf;
    }
    spSend(spView_observed(chev->status.top), m_spText_clear);
    now = time(0);
    tmstruct = localtime(&now);
    sprintf(buf,
	    "%02d-%s-%02d  %d:%02d:%02d    CHEVRON Z-MAIL %s %26.26s",
	    tmstruct->tm_mday, catgetref(monthmap[tmstruct->tm_mon]),
	    (tmstruct->tm_year % 100), tmstruct->tm_hour,
	    tmstruct->tm_min, tmstruct->tm_sec,
	    (chev->is_from_trader ? "**TRADER**" : "LITE      "),
	    (hostname ? hostname : ""));
    spSend(spView_observed(chev->status.top), m_spText_insert, 0,
	   -1, buf, spText_mNeutral);
    spSend(ev, m_spEvent_setup, (long) 1, (long) 0, 1,
	   doTime, chev);
    spSend(im, m_spIm_enqueueEvent, ev);
    return (0);
}

static void
zmcot_clear(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    spSend(spView_observed(self->receiveBody), m_spText_clear);
    spSend(spView_observed(self->rechdrs.to), m_spText_clear);
    spSend(spView_observed(self->rechdrs.from), m_spText_clear);
    spSend(spView_observed(self->rechdrs.subject), m_spText_clear);
    spSend(spView_observed(self->rechdrs.date), m_spText_clear);
}

static void
zmcot_append(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    char *buf;

    buf = spArg(arg, char *);
    if (spSend_i(spView_observed(self->receiveBody),
		 m_spText_length) == 0) {
	while (buf && *buf && (*buf == '\n'))
	    ++buf;		/* no leading newlines in the body */
    }
    spSend(spView_observed(self->receiveBody), m_spText_insert, -1,
	   -1, buf, spText_mAfter);
}

static void
sendMsg(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct dynstr dto, dsubject;

    dynstr_Init(&dto);
    dynstr_Init(&dsubject);
    TRY {
	spSend(spView_observed(self->to), m_spText_appendToDynstr,
	       &dto, 0, -1);
	spSend(spView_observed(self->subject), m_spText_appendToDynstr,
	       &dsubject, 0, -1);
	if (dynstr_EmptyP(&dsubject)) {
	    ZCommand(zmVaStr("mail %s", dynstr_Str(&dto)),
		     zcmd_commandline);
	} else {
	    ZCommand(zmVaStr("mail -s '%s' %s",
			     quotezs(dynstr_Str(&dsubject), '\''),
			     dynstr_Str(&dto)),
		     zcmd_commandline);
	}
	ZCommand("compcmd send", zcmd_ignore);
	if (!atoi(get_var_value(VarStatus))) { /* Did "mail" succeed */
	    wprint("Message sent.\n");
	    spSend(self, m_spView_invokeInteraction,
		   "zmcot-clear-composition", NULL, NULL, NULL);
	    spSend(self, m_spView_invokeInteraction, "zmcot-to",
		   NULL, NULL, NULL);
	} else {
	    stop_compose();
	}
    } FINALLY {
	dynstr_Destroy(&dto);
	dynstr_Destroy(&dsubject);
    } ENDTRY;
}

static void
clearComp(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(spView_observed(self->to), m_spText_clear);
    spSend(spView_observed(self->subject), m_spText_clear);
    spSend(spView_observed(self->composeBody), m_spText_clear);
}

static void
Refresh(self)
    struct zmcot *self;
{
    if (RefreshReason == PROPAGATE_SELECTION)
	return;

    if (((msg_folder *)
	 spSend_p(self, m_dialog_folder)) == self->currentDisplayed.fldr) {
	if (isoff(self->msgflags, DELETE)
	    && ison(msg[self->currentDisplayed.num]->m_flags, DELETE)
	    && (self->msgoffset == msg[self->currentDisplayed.num]->m_offset)
	    && boolean_val(VarAutoprint)) {
	    int num = f_next_msg(current_folder,
				 self->currentDisplayed.num, 1);

	    if (num >= 0) {
		ZCommand(zmVaStr("read %d", num + 1), zcmd_use);
		mail_status(0);
	    }
	}
    }
}

static void
zmcot_receiveNotification(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    struct spObservable *obs;
    int event;

    obs = spArg(arg, struct spObservable *);
    event = spArg(arg, int);
    spSuper(zmcot_class, self, m_spObservable_receiveNotification,
	    obs, event);
    if ((obs == (struct spObservable *) ZmlIm)
	&& (event == dialog_refresh)) {
	Refresh(self);
    }
}

static void
zmcot_setmsg(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    int num;
    char *p;
    msg_group mg;

    num = spArg(arg, int);
    self->lastDisplayed.fldr = self->currentDisplayed.fldr;
    self->lastDisplayed.num = self->currentDisplayed.num;
    self->currentDisplayed.fldr = current_folder;
    self->currentDisplayed.num = num;
    self->msgoffset = current_folder->mf_msgs[num]->m_offset;
    self->msgflags = current_folder->mf_msgs[num]->m_flags;

    self->is_from_trader = !!header_field(num, "X-Trader");

    init_msg_group(&mg, 1, 0);
    TRY {
	spSend(self, m_dialog_setmgroup, &mg);
    } FINALLY {
	destroy_msg_group(&mg);
    } ENDTRY;

    spSend(spView_observed(self->receiveBody), m_spText_clear);

    p = to_field(num);
    spSend(spView_observed(self->rechdrs.to), m_spText_clear);
    if (p) {
	char addr_buf[128], bang_buf[128], *name;

	while (p = get_name_n_addr(p, NULL, addr_buf)) {
	    (void) bang_form(bang_buf, addr_buf);
	    if (name = rindex(bang_buf, '!'))
		++name;
	    else
		name = bang_buf;
	    strcat(name, " ");
	    spSend(spView_observed(self->rechdrs.to), m_spText_insert,
		   -1, -1, name, spText_mAfter);
	    while ((*p == ',')
		   || (*p == ' ')
		   || (*p == '\t')
		   || (*p == '\n'))
		++p;
	}
    }
    p = from_field(num);
    spSend(spView_observed(self->rechdrs.from), m_spText_clear);
    if (p)
	spSend(spView_observed(self->rechdrs.from), m_spText_insert,
	       0, -1, p, spText_mAfter);
    p = subj_field(num);
    spSend(spView_observed(self->rechdrs.subject), m_spText_clear);
    if (p)
	spSend(spView_observed(self->rechdrs.subject), m_spText_insert,
	       0, -1, p, spText_mAfter);
    p = header_field(num, "date");
    spSend(spView_observed(self->rechdrs.date), m_spText_clear);
    if (p)
	spSend(spView_observed(self->rechdrs.date), m_spText_insert,
	       0, -1, p, spText_mAfter);
}

static void
copyMsg(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    u_long flags = INDENT | FOLD_ATTACH | NO_HEADER | NO_SEPARATOR;
    char *tmpdir;
    char *filename;
    FILE *fp = (FILE *) 0;
    int dummy;
    struct dynstr dsubj;

    if (self->currentDisplayed.num < 0)
	return;
    if (!(tmpdir = value_of(VarTmpdir)))
	if (!(tmpdir = getenv("TMPDIR")))
	    tmpdir = "/tmp";
    filename = savestr(zmVaStr("%s/zmc.%d",
			       varpath(tmpdir, &dummy),
			       getpid()));
    dynstr_Init(&dsubj);
    TRY {
	fp = efopen(filename, "w", "copyMsg");
	copy_msg(self->currentDisplayed.num, fp, flags, NULL);
	fclose(fp);
	fp = (FILE *) 0;
	spSend(spView_observed(self->composeBody), m_spText_readFile,
	       filename);

#if 0				/* First they want a leading newline,
				 * then they don't... sheesh. */
	spSend(spView_observed(self->composeBody), m_spText_insert,
	       0, -1, "\n", spText_mNeutral);
#endif

	spSend(spView_observed(self->rechdrs.subject),
	       m_spText_appendToDynstr, &dsubj, 0, -1);
	if (ci_strncmp(dynstr_Str(&dsubj), "re: ", 4))
	    dynstr_Replace(&dsubj, 0, 0, "Re: ");
	if (!dynstr_EmptyP(&dsubj)) {
	    spSend(spView_observed(self->subject), m_spText_clear);
	    spSend(spView_observed(self->subject), m_spText_insert,
		   0, -1, dynstr_Str(&dsubj), spText_mAfter);
	}
	spSend(self->to, m_spView_wantFocus, self->to);
    } FINALLY {
	if (fp)
	    fclose(fp);
	unlink(filename);
	free(filename);
	dynstr_Destroy(&dsubj);
    } ENDTRY;
}

static void
chevRedraw(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (self->lastDisplayed.num >= 0)
	ZCommand(zmVaStr("read %d", self->lastDisplayed.num + 1),
		 zcmd_use);
}

static void
chevDelete(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int start, next;

    start = current_msg;
    ZCommand("delete %s", zcmd_use);
    if (((next = next_msg(start, 1)) == start)
	|| (next < 0)
	|| (next >= msg_cnt)) {
	next = next_msg(start, -1);
    }
    if ((next >= 0) && (next < msg_cnt))
	ZCommand(zmVaStr("read %d", next + 1), zcmd_use);
}

static void
jumpToTo(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self->to, m_spView_wantFocus, self->to);
}

static void
chevscan(self, forwardp)
    struct zmcot *self;
    int forwardp;
{
    int num;

    if (current_folder != self->currentDisplayed.fldr)
	return;
    while (1) {
	if (((num = next_msg(self->currentDisplayed.num,
			     forwardp ? 1 : -1)) >= 0)
	    && (num != self->currentDisplayed.num)) {
	    char buf[16];

	    sprintf(buf, "\\read %d", num + 1);
	    ZCommand(buf, zcmd_use);
	    if (spSend_i(ZmlIm, m_spIm_checkChar, (long) 0, (long) 750000)) {
		/* interrupted */
		spSend(ZmlIm, m_spIm_getChar); /* get rid of the character */
		return;
	    }
	} else {
	    spSend(ZmlIm, m_spIm_showmsg,
		   CATGETS("No more MAIL this direction, try another direction"),
		   15, 2, 5);
	    return;
	}
    }
}

static void
chevScanFwd(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    chevscan(self, 1);
}

static void
chevScanRev(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    chevscan(self, 0);
}

extern void chevdir_InitializeClass();

static void
zmcotDir(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct chevdir *cd = (struct chevdir *) spoor_NewInstance(chevdir_class);

    TRY {
	spSend(cd, m_dialog_interactModally);
    } FINALLY {
	spoor_DestroyInstance(cd);
    } ENDTRY;
}

struct spWidgetInfo *spwc_Zmcot = 0;

static int
read_dot(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    if (current_folder && (msg_cnt > 0)) {
	if (ison(msg[current_msg].m_flags, DELETE)) {
	    spSend(Dialog(&ZmcotDialog), m_spView_invokeInteraction,
		   "zmcot-delete", 0, 0, 0);
	} else {
	    ZCommand(zmVaStr("read %d", current_msg + 1), zcmd_commandline);
	}
    }
    return (1);
}

static void
zmcot_enter(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    spSuper(zmcot_class, self, m_dialog_enter);
    if (current_folder && (msg_cnt > 0)) {
	spSend(ZmlIm, m_spIm_enqueueEvent,
	       spEvent_Create((long) 0, (long) 0, 1,
			      read_dot, 0));
    }
    if (!(self->timeEvent))
	self->timeEvent = spEvent_NEW();
    spSend(self->timeEvent, m_spEvent_setup, (long) 0, (long) 0,
	   1, doTime, self);
    spSend(ZmlIm, m_spIm_enqueueEvent, self->timeEvent);
}

static void
zmcot_leave(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    spSuper(zmcot_class, self, m_dialog_leave);
    if (self->timeEvent && spEvent_inqueue(self->timeEvent))
	spSend(self->timeEvent, m_spEvent_cancel, 0);
}

static void
dialog_goto_zmcot(self, requestor, data, keys)
    struct dialog *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(Dialog(&ZmcotDialog), m_dialog_enter);
}

static void
zmcot_toggle_mail(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    char *dmf = get_var_value("default_mail_folder");

    if (!dmf)
	return;
    if (strcmp(current_folder->mf_name, fullpath(dmf, 0))) {
	ZCommand("open $default_mail_folder", zcmd_commandline);
    } else {
	ZCommand("open #", zcmd_commandline);
    }
}

static void
zmcot_toggle_new_mail(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    char *nmf = get_var_value("new_mail_folder");

    if (!nmf)
	return;
    if (strcmp(current_folder->mf_name, fullpath(nmf, 0))) {
	ZCommand("open $new_mail_folder", zcmd_commandline);
    } else {
	ZCommand("open #", zcmd_commandline);
    }
}

static void
zmcot_clear_all(self, requestor, data, keys)
    struct zmcot *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self, m_spView_invokeInteraction, "zmcot-clear-composition",
	   requestor, data, keys);
    spSend(spView_observed(self->receiveBody), m_spText_clear);
    spSend(spView_observed(self->rechdrs.to), m_spText_clear);
    spSend(spView_observed(self->rechdrs.from), m_spText_clear);
    spSend(spView_observed(self->rechdrs.subject), m_spText_clear);
    spSend(spView_observed(self->rechdrs.date), m_spText_clear);
}

static void
zmcot_setfolder(self, arg)
    struct zmcot *self;
    spArgList_t arg;
{
    msg_folder *new = spArg(arg, msg_folder *);

    spSuper(zmcot_class, self, m_dialog_setfolder, new);
    if (spView_window(ZmlIm)) {
	int h, w, i, l;
	char *f;
	static catalog_ref label = CATREF("Folder: ");

	spSend(spView_window(ZmlIm), m_spWindow_size, &h, &w);
	f = spSend_p(self, m_dialog_foldername);
	l = (f ? strlen(f) : 0);
	l += strlen(catgetref(label));
	spSend(spView_observed(self->status.fldr), m_spText_clear);
	spSend(spView_observed(self->status.fldr), m_spText_insert,
	       0, 1, " ", spText_mAfter);
	for (i = 0; i < (((w - l) / 2) - 1); ++i)
	    spSend(spView_observed(self->status.fldr), m_spText_insert,
	       -1, 1, "-", spText_mAfter);
	spSend(spView_observed(self->status.fldr), m_spText_insert,
	       -1, -1, catgetref(label), spText_mAfter);
	if (f)
	    spSend(spView_observed(self->status.fldr), m_spText_insert,
		   -1, -1, f, spText_mAfter);
	for (i = 0; i < (((w - l) / 2) - 1); ++i)
	    spSend(spView_observed(self->status.fldr), m_spText_insert,
	       -1, 1, "-", spText_mAfter);
	if (spView_window(self)) {
	    spSend(ZmlIm, m_spIm_enqueueEvent,
		   spEvent_Create((long) 0, (long) 0, 1,
				  read_dot, 0));
	}
    }
}

void
zmcot_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (zmcot_class)
	return;
    zmcot_class =
	spWclass_Create("zmcot", NULL,
			dialog_class,
			(sizeof (struct zmcot)),
			zmcot_initialize,
			zmcot_finalize,
			spwc_Zmcot = spWidget_Create("Zmcot",
						     spwc_Screen));
    m_zmcot_clear =
	spoor_AddMethod(zmcot_class, "clear",
			NULL,
			zmcot_clear);
    m_zmcot_append =
	spoor_AddMethod(zmcot_class, "append",
			NULL,
			zmcot_append);
    m_zmcot_setmsg =
	spoor_AddMethod(zmcot_class, "setmsg",
			NULL,
			zmcot_setmsg);
    spoor_AddOverride(zmcot_class,
		      m_dialog_setfolder, 0, zmcot_setfolder);
    spoor_AddOverride(zmcot_class,
		      m_dialog_enter, NULL,
		      zmcot_enter);
    spoor_AddOverride(zmcot_class,
		      m_dialog_leave, NULL,
		      zmcot_leave);
    spoor_AddOverride(zmcot_class, m_spObservable_receiveNotification,
		      NULL, zmcot_receiveNotification);
    spWrapview_InitializeClass();
    spSplitview_InitializeClass();
    spTextview_InitializeClass();
    spText_InitializeClass();
    spCmdline_InitializeClass();
    spEvent_InitializeClass();
    chevdir_InitializeClass();

    spWidget_AddInteraction(spwc_Screen, "goto-zmcot", dialog_goto_zmcot,
			    CATGETS("Go to Zmcot screen"));

    spWidget_AddInteraction(spwc_Zmcot, "zmcot-clear-all",
			    zmcot_clear_all, CATGETS("Clear all fields"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-toggle-mail",
			    zmcot_toggle_mail,
			    CATGETS("Switch to or from default_mail_folder"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-toggle-new-mail",
			    zmcot_toggle_new_mail,
			    CATGETS("Switch to or from new_mail_folder"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-to", jumpToTo,
			    CATGETS("Go to To: header"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-send", sendMsg,
			    CATGETS("Send message"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-clear-composition", clearComp,
			    CATGETS("Clear composition area"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-copy-msg", copyMsg,
			    CATGETS("Copy message to composition area"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-redraw", chevRedraw,
			    CATGETS("Redraw previously-displayed message"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-delete", chevDelete,
			    CATGETS("Delete message"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-scan-forward", chevScanFwd,
			    CATGETS("Scan forward"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-scan-backward", chevScanRev,
			    CATGETS("Scan backward"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-grab-to", grabTo,
			    CATGETS("Note contents of To: header"));
    spWidget_AddInteraction(spwc_Zmcot, "zmcot-directory", zmcotDir,
			    CATGETS("Directory browser"));
}

struct chevdir {
    SUPERCLASS(dialog);
    struct spCmdline *username, *fullname, *group;
    struct spTextview *extra;
    struct spListv *matches;
};

/* Copied from motif/m_help.c */
static char *
lcase_strstr(buf, str)
char *buf, *str;
{
    int len = strlen(str);
    
    for (; *buf; buf++)
	if (!ci_strncmp(buf, str, len)) return buf;
    return NULL;
}

static void
chevdirSearch(self, requestor, data, keys)
    struct chevdir *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct dynstr probe;
    int e = 0, skip = 0;
    char *setenv_argv[4];

    setenv_argv[0] = "setenv";
    setenv_argv[3] = 0;
    spSend(ZmlIm, m_spIm_showmsg, CATGETS("Searching..."), 15, 1, 0);
    spSend(spView_observed(self->matches), m_spText_clear);
    spSend(spView_observed(self->extra), m_spText_clear);
    dynstr_Init(&probe);
    LITE_BUSY {
	setenv_argv[1] = "MASTER_USER_FILE";
	setenv_argv[2] = get_var_value("master_user_file");
	Setenv(3, setenv_argv);
	setenv_argv[1] = "MSG_GROUPS_FILE";
	setenv_argv[2] = get_var_value("msg_groups_file");
	Setenv(3, setenv_argv);
	setenv_argv[1] = "COT_SEARCH";
	TRY {
	    if (spSend_i(spView_observed(self->fullname), m_spText_length)) {
		/* case-insensitive grep on prefix of fullname field
		 * of master_user_file.  List ID/Full-name pairs.
		 */
		setenv_argv[2] = "fullname";
		spSend(spView_observed(self->fullname),
		       m_spText_appendToDynstr, &probe, 0, -1);
	    } else if (spSend_i(spView_observed(self->username),
				m_spText_length)) {
		/* case-insensitive grep on user-id(s) field of
		 * msg_groups_file.  Only if the matching ID in every
		 * line is identical, list all groups of which this
		 * ID is a member.  Also find the ID in the
		 * master_user_file, parse out the ID, host, and
		 * phone number, and display those in special
		 * widgets.
		 */
		setenv_argv[2] = "userid";
		spSend(spView_observed(self->username),
		       m_spText_appendToDynstr, &probe, 0, -1);
		skip = 1;
	    } else if (spSend_i(spView_observed(self->group),
				m_spText_length)) {
		/* case-insensitive prefix grep on groups.
		 * If unique match is found, list members
		 */
		setenv_argv[2] = "group";
		spSend(spView_observed(self->group),
		       m_spText_appendToDynstr, &probe, 0, -1);
	    } else {
		error(UserErrWarning, CATGETS("Enter a search pattern"));
		e = 1;
	    }
	    if (!e) {
		char **hits = 0;

		Setenv(3, setenv_argv);
		switch (lookup_run(dynstr_Str(&probe), 0, "-1", &hits)) {
		  case DSRESULT_MATCHES_FOUND:
		  case DSRESULT_ONE_MATCH:
		    if (hits) {
			char **strs = lookup_split(hits + skip, 0, 0);
			int i;

			for (i = skip; hits[i]; ++i)
			    spSend(spView_observed(self->matches),
				   m_spList_append,
				   zmVaStr("%s\t%s", strs[i - skip], hits[i]));
			xfree(strs);
			if (skip) {
			    /* Parse ID, Host, Name, & Phone out of hits[0] */
			    struct dynstr name, id, host, phone;
			    char *p = index(hits[0], ':'), *h;

			    dynstr_Init(&id);
			    dynstr_Init(&name);
			    dynstr_Init(&host);
			    dynstr_Init(&phone);
			    TRY {
				do { /* to allow non-local exit */
				    if (!p)
					break;
				    dynstr_AppendN(&id, hits[0], p - hits[0]);
				    h = p + 1;
				    if (!(p = index(h, ':')))
					break;
				    dynstr_AppendN(&name, h, p - h);
				    h = p + 1;
				    if (!(p = index(h, ':')))
					break;
				    h = p + 1;
				    if (!(p = index(h, ':')))
					break;
				    dynstr_AppendN(&host, h, p - h);
				    h = p + 1;
				    dynstr_Append(&phone, h);
				    spSend(spView_observed(self->extra),
					   m_spText_insert, 0, -1,
					   zmVaStr(CATGETS("ID: %s    NAME: %s    HOST: %s    PHONE: %s"),
						   dynstr_Str(&id),
						   dynstr_Str(&name),
						   dynstr_Str(&host),
						   dynstr_Str(&phone)),
					   spText_mAfter);
				} while (0); /* to allow non-local exit */
			    } FINALLY {
				dynstr_Destroy(&id);
				dynstr_Destroy(&name);
				dynstr_Destroy(&host);
				dynstr_Destroy(&phone);
			    } ENDTRY;
			}
		    }
		    break;
		  case DSRESULT_NO_MATCHES:
		    error(UserErrWarning, CATGETS("No matching entries"));
		    break;
		  case DSRESULT_TOO_MANY:
		    error(UserErrWarning, CATGETS("Too many matches"));
		    break;
		  case DSRESULT_BAD_ARGS:
		    error(UserErrWarning, CATGETS("Wrong number of arguments"));
		    break;
		  default:
		    error(SysErrWarning, CATGETS("Error looking up address"));
		    break;
		}
		if (hits)
		    free_vec(hits);
	    }
	} FINALLY {
	    dynstr_Destroy(&probe);
	} ENDTRY;
    } LITE_ENDBUSY;
    spSend(ZmlIm, m_spIm_showmsg, CATGETS("Searching...done"), 15, 1, 0);
}

static void
chevdirUse(self, requestor, data, keys)
    struct chevdir *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    int i;
    struct dynstr d;
    struct zmcot *zmcot = (struct zmcot *) Dialog(&ZmcotDialog);
    char buf[128];

    dynstr_Init(&d);
    TRY {
	for (i = 0; i < spSend_i(spView_observed(self->matches),
				 m_spList_length); ++i) {
	    if (intset_Contains(spListv_selections(self->matches), i)) {
		dynstr_Set(&d, "");
		spSend(spView_observed(self->matches), m_spList_getNthItem,
		       i, &d);
		sscanf(dynstr_Str(&d), "%s", buf);
		if (spSend_i(spView_observed(zmcot->to), m_spText_length) > 0)
		    spSend(spView_observed(zmcot->to), m_spText_insert,
			   -1, -1, ", ", spText_mNeutral);
		spSend(spView_observed(zmcot->to), m_spText_insert, -1, -1,
		       buf, spText_mNeutral);		
		spSend(ZmlIm, m_spIm_showmsg, zmVaStr(CATGETS("Using address %s"),
						      buf),
		       15, 1, 5);
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
chevdirClear(self, requestor, data, keys)
    struct chevdir *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(spView_observed(self->username), m_spText_clear);
    spSend(spView_observed(self->group), m_spText_clear);
    spSend(spView_observed(self->fullname), m_spText_clear);
    spSend(spView_observed(self->matches), m_spText_clear);
    spSend(spView_observed(self->extra), m_spText_clear);
}

static void
chevdir_aa_done(b, self)
    struct spButton *b;
    struct chevdir *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static void
chevdir_aa_search(b, self)
    struct spButton *b;
    struct chevdir *self;
{
    spSend(self, m_spView_invokeInteraction, "chevdir-search", self, 0, 0);
}

static void
chevdir_aa_use(b, self)
    struct spButton *b;
    struct chevdir *self;
{
    spSend(self, m_spView_invokeInteraction, "chevdir-use", self, 0, 0);
}

static void
chevdir_aa_clear(b, self)
    struct spButton *b;
    struct chevdir *self;
{
    spSend(self, m_spView_invokeInteraction, "chevdir-clear", self, 0, 0);
}

static void
chevdir_initialize(self)
    struct chevdir *self;
{
    struct spSplitview *v;
    struct spList *l;
    struct spText *t;

    spSend(self->username = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spSend(self->fullname = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    spSend(self->group = spCmdline_NEW(), m_spView_setObserved,
	   spText_NEW());
    self->matches = spListv_NEW();
    l = spList_NEW();
    spSend(self->matches, m_spView_setObserved, l);

    spSend(self->extra = spTextview_NEW(), m_spView_setObserved,
	   t = spText_NEW());
    spSend(t, m_spText_setReadOnly, 1);

    spSend(spView_observed(self->username), m_spObservable_addObserver, self);
    spSend(spView_observed(self->fullname), m_spObservable_addObserver, self);
    spSend(spView_observed(self->group), m_spObservable_addObserver, self);

    spCmdline_fn(self->username) = jumpNext;
    spView_callbackData(self->username) = (struct spView *) self;

    spCmdline_fn(self->fullname) = jumpNext;
    spView_callbackData(self->fullname) = (struct spView *) self;

    spCmdline_fn(self->group) = jumpNext;
    spView_callbackData(self->fullname) = (struct spView *) self;

    spView_callbackData(self->matches) = (struct spView *) self;

    spSend(self, m_dialog_setActionArea,
	   ActionArea(self,
		      CATGETS("Done"), chevdir_aa_done,
		      CATGETS("Search"), chevdir_aa_search,
		      CATGETS("Use"), chevdir_aa_use,
		      CATGETS("Clear"), chevdir_aa_clear,
		      0));

    v = Split(Wrap(self->username, 0, 0, CATGETS("Nknm:"), 0, 1, 0, 0),
	      Split(Wrap(self->fullname, 0, 0, CATGETS("Name:"), 0, 1, 0, 0),
		    Wrap(self->group, 0, 0, CATGETS("Group:"), 0, 1, 0, 0),
		    30, 0, 0,
		    spSplitview_leftRight, spSplitview_plain, 0),
	      50, 1, 0,
	      spSplitview_leftRight, spSplitview_plain, 0);

    spSend(self, m_dialog_setView,
	   Split(Split(v,
		       self->extra, 1, 1, 0,
		       spSplitview_topBottom,
		       spSplitview_plain, 0),
		 self->matches,
		 8, 1, 0,
		 spSplitview_topBottom,
		 spSplitview_boxed,
		 spSplitview_SEPARATE));
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;
    spSend(self, m_spWrapview_setLabel, CATGETS("Directory Search"), spWrapview_top);

    ZmlSetInstanceName(self, "chevdir", self);
    ZmlSetInstanceName(dialog_actionArea(self), "chevdir-aa", self);
    ZmlSetInstanceName(self->matches, "chevdir-matches", self);

    spSend(self, m_dialog_clearFocusViews);
    spSend(self, m_dialog_addFocusView, self->username);
    spSend(self, m_dialog_addFocusView, self->fullname);
    spSend(self, m_dialog_addFocusView, self->group);
    spSend(self, m_dialog_addFocusView, self->matches);
}

static void
chevdir_activate(self, arg)
    struct chevdir *self;
    spArgList_t arg;
{
    spSuper(chevdir_class, self, m_dialog_activate);
    spSend(self->username, m_spView_wantFocus, self->username);
}

static void
chevdir_desiredSize(self, arg)
    struct chevdir *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(chevdir_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    *bestw = 74;
}

static void
chevdir_receiveNotification(self, arg)
    struct chevdir *self;
    spArgList_t arg;
{
    struct spObservable *o = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(chevdir_class, self, m_spObservable_receiveNotification,
	    o, event, data);

    if ((o == spView_observed(self->username))
	&& spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->fullname), m_spText_clear);
	spSend(spView_observed(self->group), m_spText_clear);
    } else if ((o == spView_observed(self->fullname))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->username), m_spText_clear);
	spSend(spView_observed(self->group), m_spText_clear);
    } else if ((o == spView_observed(self->group))
	       && spSend_i(o, m_spText_length)) {
	spSend(spView_observed(self->username), m_spText_clear);
	spSend(spView_observed(self->fullname), m_spText_clear);
    }
}

struct spWidgetInfo *spwc_ZmcotDir = 0;

void
chevdir_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (chevdir_class)
	return;
    chevdir_class =
	spWclass_Create("chevdir", NULL,
			dialog_class,
			(sizeof (struct chevdir)),
			chevdir_initialize, 0,
			spwc_ZmcotDir = spWidget_Create("ZmcotDir",
							spwc_Popup));
    spoor_AddOverride(chevdir_class,
		      m_spObservable_receiveNotification, NULL,
		      chevdir_receiveNotification);
    spoor_AddOverride(chevdir_class,
		      m_dialog_activate, NULL,
		      chevdir_activate);
    spoor_AddOverride(chevdir_class, m_spView_desiredSize, NULL,
		      chevdir_desiredSize);

    spCmdline_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();

    spWidget_AddInteraction(spwc_ZmcotDir, "chevdir-search", chevdirSearch,
			    "Perform directory search");
    spWidget_AddInteraction(spwc_ZmcotDir, "chevdir-use", chevdirUse,
			    "Use selected address(es) in composition");
    spWidget_AddInteraction(spwc_ZmcotDir, "chevdir-clear", chevdirClear,
			    "Clear fields");

    spWidget_bindKey(spwc_ZmcotDir, spKeysequence_Parse(0, "\\<f1>", 1),
		     "dialog-close", 0, 0, "1/Done", 0);
    spWidget_bindKey(spwc_ZmcotDir, spKeysequence_Parse(0, "\\<f2>", 1),
		     "chevdir-search", 0, 0, "2/Search", 0);
    spWidget_bindKey(spwc_ZmcotDir, spKeysequence_Parse(0, "\\<f3>", 1),
		     "chevdir-use", 0, 0, "3/Use", 0);
    spWidget_bindKey(spwc_ZmcotDir, spKeysequence_Parse(0, "\\<f4>", 1),
		     "chevdir-clear", 0, 0, "4/Clear", 0);
}

#endif /* ZMCOT */
