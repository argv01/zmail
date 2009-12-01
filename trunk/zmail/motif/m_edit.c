/* m_edit.c     Copyright 1990, 1991 Z-Code Software Corp. */

#define HIDE_BCC

/*
 * intercept events in the compose window for auto-
 *	positioning and tilde command recognition.
 */
#include "zmail.h"
#include "zmcomp.h"
#include "zmframe.h"
#include "attach/area.h"
#include "cmdtab.h"
#include "child.h"
#include "catalog.h"
#include "m_comp.h"
#include "strcase.h"
#include "uicomp.h"
#include "zm_motif.h"
#include "dynstr.h"

#include <Xm/Text.h>

#ifdef NOT_NOW
short dat_bentarrow[] = {
    0x007F, 0x007F, 0x007F, 0x0007, 0x0407, 0x0C07, 0x1C07, 0x3807,
    0x7FFF, 0xFFFF, 0x7FFF, 0x3800, 0x1C00, 0x0C00, 0x0400, 0x0000
};
#endif /* NOT_NOW */

static void edit_comp_external();
void do_edit(), fill_hdr_promts();

int gui_spawn_process();
static void msg_child_exit_callback();

typedef struct MsgEdit {
    u_long offset;
    msg_folder *fldr;
    char *file;
} MsgEdit;

#ifdef ZM_CHILD_MANAGER
static void
child_exit_callback(pid)
int pid;
{
    zmChildWaitPid(pid, (WAITSTATUS *)0, 0);
    gui_restore_compose(pid);
}
#endif /* ZM_CHILD_MANAGER */

void
do_spell(item, mapped)
Widget item;
int mapped;
{
    char *file, *edit;
    Widget textsw;
    Compose *compose;

    ask_item = item;

    if ((!(edit = value_of(VarIspeller)) || !*edit)) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 119, "No interactive speller defined." ));
	return;
    }

    FrameGet(FrameGetData(item), FrameClientData, &compose, FrameEndArgs);
    textsw = compose->interface->comp_items[COMP_TEXT_W];
    if (!SaveComposition(compose, ison(compose->flags, AUTOFORMAT))) {
	error(SysErrWarning,
	    catgets(catalog, CAT_MOTIF, 871, "Cannot write to spell check file"));
	return;
    }
    edit_comp_external(item, compose, edit, compose->edfile, mapped);
}

/*ARGSUSED*/
void
do_edit(item, mapped, edit)
Widget item;
int mapped;
char *edit;
{
    int autoformat;
    char *file;
    Widget textsw;
    Compose *compose;

    ask_item = item;
    FrameGet(FrameGetData(item), FrameClientData, &compose, FrameEndArgs);
    if (compose->exec_pid) return;
    /* pf Tue Jun  8 18:48:18 1993: moved this here... */
    textsw = compose->interface->comp_items[COMP_TEXT_W];
    autoformat = ison(compose->flags, AUTOFORMAT);
    if (!SaveComposition(compose, autoformat)) {
	error(SysErrWarning,
	    catgets(catalog, CAT_MOTIF, 120, "Cannot write to editor file"));
	return;
    }
    /* We've overloaded SEND_NOW to accomplish autosend.  Must make
     * sure that it's off before making calls into compose.c.
     */
	turnoff(compose->send_flags, SEND_NOW);	/* Make sure */
    edit_comp_external(item, compose, edit, compose->edfile, mapped);
}

static void
edit_comp_external(item, compose, edit, file, mapped)
Widget item;
Compose *compose;
char *edit, *file;
int mapped;
{
    int argc;
    char **argv;
    struct dynstr dp;

    dynstr_Init(&dp);
    if (edit && *edit) {
	dynstr_Set(&dp, edit);
	dynstr_AppendChar(&dp, ' ');
    } else
	GetWineditor(&dp);
    dynstr_Append(&dp, file);
    argc = 0;
    argv = mk_argv(dynstr_Str(&dp), &argc, FALSE);
    dynstr_Destroy(&dp);
    if (!argv) {
	(void) unlink(file);
	return;
    }

    grab_addresses(compose, EDIT); /* Bart: Tue Sep  8 17:32:14 PDT 1992 */
    /* Bart: Thu May 26 23:26:03 PDT 1994 -- still need this?? */
    AddressAreaWalkRaw(compose->interface->prompter, 0L); /* Bart: Fri Oct  2 13:33:26 PDT 1992 */

    compose->exec_pid = gui_spawn_process(argv);
    if (compose->exec_pid == -1) {
	error(SysErrWarning, *argv);
	compose->exec_pid = 0;
	turnoff(compose->send_flags, SEND_NOW);	/* Override autosend */
	if (!mapped) /* Bart: Fri Oct  2 13:33:31 PDT 1992 */
	    gui_end_edit(compose);
    } else {
	Widget top;
#ifdef ZM_CHILD_MANAGER
	zmChildSetCallback(compose->exec_pid,
	    ZM_CHILD_EXITED, child_exit_callback, (void *)0);
#endif /* ZM_CHILD_MANAGER */
	top = GetTopShell(item);
	if (mapped)
	    (void) IconifyShell(top);
	XtSetSensitive(top, False);
    }
    free_vec(argv);
}

void
gui_edit_external(m_no, file, edit)
int m_no;
char *file, *edit;
{
    int argc;
    char **argv;
    struct dynstr dp;
    int pid;

    dynstr_Init(&dp);
    GetWineditor(&dp);
    dynstr_Append(&dp, file);
    argc = 0;
    argv = mk_argv(dynstr_Str(&dp), &argc, FALSE);
    dynstr_Destroy(&dp);
    if (!argv) {
	(void) unlink(file);
	return;
    }

    pid = gui_spawn_process(argv);
    if (pid == -1) {
	error(SysErrWarning, *argv);
    } else {
	MsgEdit *medit = (MsgEdit *) calloc(1, sizeof *medit);
	medit->offset = msg[m_no]->m_offset;
	medit->fldr = current_folder;
	medit->file = savestr(file);
#ifdef ZM_CHILD_MANAGER
	zmChildSetCallback(pid,
	    ZM_CHILD_EXITED, msg_child_exit_callback,
	    (void *)medit);
#endif /* ZM_CHILD_MANAGER  */
	turnon(msg[m_no]->m_flags, EDITING);
    }
    free_vec(argv);
}

#ifdef ZM_CHILD_MANAGER
static void
restore_edited_msg(closure, id)
     XtPointer closure;
     XtIntervalId *id;
{
    MsgEdit *medit = (MsgEdit *) closure;
    int i;
    int mcount = medit->fldr->mf_group.mg_count;
    msg_folder *cur_fldr;
    msg_group tmp;
    
    for (i = 0; i < mcount; i++)
	if (medit->fldr->mf_msgs[i]->m_offset == medit->offset) break;
    if (i == mcount)		/* lost the message... */
	return;
    turnoff(medit->fldr->mf_msgs[i]->m_flags, EDITING);
    init_msg_group(&tmp, msg_cnt, 1);
    (void) add_msg_to_group(&tmp, i);
    if (check_replies(medit->fldr, &tmp, TRUE) >= 0) {
	cur_fldr = current_folder;
	current_folder = medit->fldr;
	if (load_folder(medit->file, FALSE, i, NULL_GRP) > 0) {
	    messageStore_Reset();	/* flush the temporary message text cache */
	    (void) unlink(medit->file);
	}
	current_folder = cur_fldr;
    }
    msg_group_combine(&current_folder->mf_group, MG_SET, &tmp);
    destroy_msg_group(&tmp);
    xfree(medit->file);
    xfree(medit);
    gui_refresh(current_folder, REDRAW_SUMMARIES);
}

static void
msg_child_exit_callback(pid, status, medit)
int pid;
void *status;
MsgEdit *medit;
{
    zmChildWaitPid(pid, (WAITSTATUS *)0, 0);
    XtAppAddTimeOut(app, 0, restore_edited_msg, medit);
}
#endif /* ZM_CHILD_MANAGER */

int
gui_spawn_process(argv)
char **argv;
{
    int pid;
    
#ifndef ZM_CHILD_MANAGER
    /* XXX Can't do this cleanly in SYSV */
    (void) signal(SIGCHLD, sigchldcatcher);
#endif /* ZM_CHILD_MANAGER  */
    if ((pid = zmChildVFork()) != 0) return pid;
    
    (void) signal(SIGINT, SIG_DFL);
    (void) signal(SIGQUIT, SIG_DFL);
#ifdef SIGTSTP
    /* don't want the user to ^Z this guy */
#ifdef _SC_JOB_CONTROL
    if (sysconf(_SC_JOB_CONTROL) >= 0)
#endif /* _SC_JOB_CONTROL */
	(void) signal(SIGTSTP, SIG_IGN);
#endif /* SIGTSTP */
    (void) signal(SIGPIPE, SIG_DFL);
    (void) closefileds_above(2);
#ifdef SYSV_SETPGRP
    if (setpgrp() == -1)
#else /* !SYSV_SETPGRP */
	if (setpgrp(0, getpid()) == -1)
#endif /* SYSV_SETPGRP */
	    error(SysErrWarning, "setpgrp");
#ifdef apollo
    setgid(getgid());
#endif /* apollo */
    execvp(*argv, argv);
    if (errno == ENOENT)
	_exit(-2);
    _exit(-1); return 0;
}

void
gui_end_edit(compose)
Compose *compose;
{
    Widget textsw, comp_shell;
    ZmFrame comp_frame;
    int autosendit = 0;

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
	abort_mail(compose->interface->comp_items[COMP_TEXT_W], False);
	return;
    }
    (void) close_edfile(compose);

    textsw = compose->interface->comp_items[COMP_TEXT_W];
    comp_shell = GetTopShell(textsw);
    comp_frame = FrameGetData(comp_shell);

    reset_opts_menu(comp_shell, compose);

    LoadComposition(compose);

    /* This is kinda inefficient -- we suck the file into the text only
     * to write it straight out again.  Should probably fix.		XXX
     */
    if (autosendit) {
	if (do_send(textsw, False) == 0)
	    return;
    }

    SetTextPosLast(textsw);
    FrameSet(comp_frame,
	FrameFlagOn, FRAME_IS_OPEN,
	FrameEndArgs);
    XtSetSensitive(comp_shell, True);
    FramePopup(comp_frame);
    AddressAreaUse(compose->interface->prompter, compose);
    draw_attach_area(compose->interface->attach_area, comp_frame);
}

void
restore_comp_frame(pid)
int pid;
{
    extern Compose *comp_list;
    Compose *compose = comp_list;

    if (!compose)
	return;
    timeout_cursors(True);
    do {
	if (compose->exec_pid == pid) {
	    compose->exec_pid = 0;
	    gui_end_edit(compose);
	    SetTextInput(compose->interface->comp_items[COMP_TEXT_W]);
	    break;
	} else
	    compose = (Compose *)(compose->link.l_next);
    } while (compose != comp_list);
    timeout_cursors(False);
}

/* Perform a Z-Script mail-editing commmand on the indicated composition.
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
{
    TYPE_POSITION pos =
	TEXT_GET_LAST_POS(compose->interface->comp_items[COMP_TEXT_W]);
    int tilde_value;

    /* Don't do anything if an editor is running */
    if (compose->exec_pid)
	return EDMAIL_ABORT;

    /* These don't need saving of the file, ever.  However, they must not
     * return 1 unless (a) it is known that no I/O via compose->ed_fp is
     * necessary or (b) open_edfile() has been called.
     *
     * Bart: Thu Apr 21 21:46:48 PDT 1994
     * The above became incorrect when gui_end_edit() expanded to cover
     * some other error cases.  It's no longer safe to return 1 (that is,
     * EDMAIL_STATESAVED) when the body and header widgets haven't been
     * written to the file and/or compose structure.
     */
    switch (code) {
	case '\n':
	    TEXT_REPLACE(compose->interface->comp_items[COMP_TEXT_W],
		pos, pos, "\n");
	    /* Fall through */
	case ' ':
	    TEXT_REPLACE(compose->interface->comp_items[COMP_TEXT_W],
		pos, pos, param);
	    return EDMAIL_COMPLETED;
	case '.':
	    return do_send(compose->interface->comp_items[COMP_ACTION_AREA], 0) == 0?
		EDMAIL_COMPLETED : EDMAIL_ABORT;
	case 1:
	    return EDMAIL_UNCHANGED;
	case 2:
	    return EDMAIL_UNCHANGED;
	case 3: {	       	/* spell */
	    GuiItem w = compose->interface->comp_items[COMP_TEXT_W];
	    do_spell(w, window_is_visible(GetTopShell(w)));
	    return EDMAIL_COMPLETED;
	}
	case 'a':
	    if (SaveFile(compose->interface->comp_items[COMP_TEXT_W],
		    param, "a", ison(compose->flags, AUTOFORMAT)))
		return EDMAIL_COMPLETED;
	    return EDMAIL_ABORT;
	case 'A':
	    return EDMAIL_UNCHANGED;
	case 'b':
	    if (param) break;
	    tilde_from_menu(compose->interface->prompter, 4);	/* Urk */
	    return EDMAIL_COMPLETED;
	case 'c':
	    if (param) break;
	    tilde_from_menu(compose->interface->prompter, 3);	/* Urk */
	    return EDMAIL_COMPLETED;
	case 'e': case 'v': {
	    GuiItem w = compose->interface->comp_items[COMP_TEXT_W];
	    do_edit(w, window_is_visible(GetTopShell(w)), param);
	    if (ison(compose->flags, DOING_INTERPOSE))
		interposer_thwart = TRUE;
	    return EDMAIL_COMPLETED;
	}
	case 'F':
	    return EDMAIL_UNCHANGED;
	case 'k':
	    return EDMAIL_UNCHANGED;
	case 'L':
	    return EDMAIL_UNCHANGED;
	case 'l':
	    return EDMAIL_UNCHANGED;
	case 'P':
	    return EDMAIL_UNCHANGED;
	case 'p':
	    return EDMAIL_ABORT;
	case 'q': case 'x':
	    if (ison(compose->flags, DOING_INTERPOSE))
		return EDMAIL_UNCHANGED;
	    abort_mail(compose->interface->comp_items[COMP_ACTION_AREA], code == 'x');
	    return EDMAIL_COMPLETED;
	case 'R':
	    return EDMAIL_UNCHANGED;
	case 'r':
	    OpenFile(compose->interface->comp_items[COMP_TEXT_W], param, True);
	    return EDMAIL_COMPLETED;
	case 's':
	    if (param) break;
	    tilde_from_menu(compose->interface->prompter,
		2);	/* Urk */
	    return EDMAIL_COMPLETED;
	case 'S':
	    return EDMAIL_UNCHANGED;
	case 't':
	    if (param) break;
	    tilde_from_menu(compose->interface->prompter,
		1);	/* Urk */
	    return EDMAIL_COMPLETED;
	case 'T':
	    return EDMAIL_UNCHANGED;
	case 'w':
	    if (SaveFile(compose->interface->comp_items[COMP_TEXT_W],
		    param, "w", ison(compose->flags, AUTOFORMAT)))
		return EDMAIL_COMPLETED;
	    return EDMAIL_ABORT;
    }

    /* We have to sync the file with the compose structure */
    /* pf Thu Oct 28 20:19:25 1993
     * pass False as the autoformat argument to SaveComposition, since
     * we don't want to break lines unless we're sending or passing this
     * composition to an external editor.
     */
    if (!SaveComposition(compose, False)) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 120, "Cannot write to editor file" ));
	return EDMAIL_ABORT;
    }
    resume_compose(compose);
    if (reload_edfile() != 0) {
	abort_mail(compose->interface->comp_items[COMP_ACTION_AREA], False);
	wprint(catgets( catalog, CAT_MOTIF, 83, "Message not sent.\n" ));
	return EDMAIL_ABORT;
    }
    suspend_compose(compose);

    /* Except on 'C', this misses doing the directory check on
     * any address that may already have been in the text widget
     * before the compcmd to stuff that widget was activated.
     * Not sure how to fix it, maybe it'll fall out when we re-
     * design the Compose window.  Meanwhile, it's rare.
     */
    grab_addresses(compose, ACTIVE);

    /* These manipulate addresses -- I hate this special casing */
    switch (code) {
	case 'b':
	    tilde_value = 4;
	    break;
	case 'c':
	    tilde_value = 3;
	    break;
	case 's':
	    tilde_value = 2;
	    break;
	case 't':
	    tilde_value = 1;
	    break;
	default:
	    tilde_value = 0;
	    break;
    }

    switch (code) {
	case 4:
	    return EDMAIL_STATESAVED;
	case 'a':
	    return EDMAIL_STATESAVED;
	case 'b':
	case 'c':
	case 'C':
	case 's':
	case 't':
	    if (tilde_value)
		tilde_from_menu(compose->interface->prompter,
		    tilde_value);
	    return EDMAIL_STATESAVED;
	case 'd':
	    return EDMAIL_STATESAVED;
	case 'D':
	    return EDMAIL_STATESAVED;
	case 'E':
	    return EDMAIL_STATESAVED;
	case 'f':
	    return EDMAIL_STATESAVED;
	case 'H':
	    return EDMAIL_STATESAVED;
	case 'I':
	    return EDMAIL_STATESAVED;
	case 'i':
	    return EDMAIL_STATESAVED;
	case 'M':
	    return EDMAIL_STATESAVED;
	case 'm':
	    return EDMAIL_STATESAVED;
	case 'O':
	    return EDMAIL_STATESAVED;
	case 'z':
	    return EDMAIL_ABORT;
	case '|':
	    return EDMAIL_STATESAVED;
	case '\0':	/* Get header */
	    return EDMAIL_STATESAVED;
    }

    return EDMAIL_COMPLETED;
}
