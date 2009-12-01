/* statbar.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "zmstring.h"
#include "statbar.h"
#include "zfolder.h"
#include "vars.h"
#include "extsumm.h"
#include "glist.h"

#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/MainW.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#endif /* SANE_WINDOW */

/* Avoid zmail.h for now */
extern char *format_prompt P((msg_folder *, const char *));
extern char *format_hdr P((int, const char *, int));

/* I don't know what this means, but it came from Z-Windows */
#define SB_DEFAULT_FMT "w1,s"

struct statusPane {
    Widget label, sep;
    char *text;		/* Needed? */
    esummseg_t *seg;
};

struct statusBar {
    Widget sbar, form;
    struct glist panes;
    esumm_t esumm;
    char *varname;
    ZmCallback cb;
    XtWorkProcId refresh_id;
};

#define SB_FRACTION_BASE	1000		/* Must be divisible by 100 */

static void
statusPane_Layout(sp, position)
StatusPane *sp;
int position;
{
    if (position > 0) {
	XtVaSetValues(sp->sep,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, position,
	    NULL);
    }
}

/* Add a new pane to the status bar.
 * Assumes panes are created in order, left to right.
 */
static void
statusPane_Create(statbar, position)
StatusBar *statbar;
int position;
{
    StatusPane spane;

    spane.label =
	XtVaCreateManagedWidget("status_item",
	    xmLabelWidgetClass, statbar->form,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    NULL);

    if (position == 0) {
	spane.sep = 0;
	XtVaSetValues(spane.label,
	    XmNleftAttachment, XmATTACH_FORM,
	    NULL);
    } else {
	spane.sep = XtVaCreateManagedWidget(NULL,
	    xmSeparatorWidgetClass, statbar->form,
	    XmNorientation, XmVERTICAL,
	    XmNshadowType, XmSHADOW_ETCHED_IN,	/* OUT? */
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    /* XmNrightAttachment, XmATTACH_SELF, */
	    NULL);
	XtVaSetValues(spane.label,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, spane.sep,
	    NULL);
	XtVaSetValues(((StatusPane *)glist_Last(&statbar->panes))->label,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, spane.sep,
	    NULL);
	statusPane_Layout(&spane, position);
    }
    glist_Add(&(statbar->panes), &spane);
}

static void
statusPane_Refresh(sp)
StatusPane *sp;
{
    SAVE_RESIZE(GetTopShell(sp->label));
    SET_RESIZE(False);

    if (sp->text && *(sp->text))
	XtVaSetValues(sp->label, XmNlabelString, zmXmStr(sp->text), NULL);
    else	/* Workaround for Motif geometry bug on empty labels */
	XtVaSetValues(sp->label, XmNlabelString, zmXmStr(" "), NULL);

    RESTORE_RESIZE();
}

static void
statusBar_InitPanes(sbar)
StatusBar *sbar;
{
    sbar->form =
	XtVaCreateManagedWidget("status_bar",
	    xmFormWidgetClass, sbar->sbar,
	    XmNmarginHeight, 0,
	    XmNmarginWidth, 0,
	    XmNfractionBase, SB_FRACTION_BASE,
	    NULL);
    /* We choose 5 here because that's what extsumm does */
    glist_Init(&sbar->panes, sizeof(StatusPane), 5);
}

static void
statusBar_DestroyPanes(sbar)
StatusBar *sbar;
{
    /* If any panes were ever created, don't resize new ones away. */
    if (glist_Length(&sbar->panes) > 0)
	XtVaSetValues(sbar->sbar, XmNresizeHeight, False, NULL);

    glist_Destroy(&sbar->panes);
    ZmXtDestroyWidget(sbar->form);
}

void
statusBar_Destroy(sbar)
StatusBar *sbar;
{
    /* if (sbar->refresh_id)
	XtRemoveWorkProc(sbar->refresh_id); */
    ZmXtDestroyWidget(sbar->sbar);
    /* glist_Destroy(&sbar->panes); */
    /* esumm_Destroy(&sbar->esumm); */
}

static void
statusBar_DestroyCB(w, sbar, data)
Widget w;
XtPointer sbar, data;
{
    if (((StatusBar *)sbar)->refresh_id)
	XtRemoveWorkProc(((StatusBar *)sbar)->refresh_id);
    glist_Destroy(&(((StatusBar *)sbar)->panes));
    esumm_Destroy(&(((StatusBar *)sbar)->esumm));
}

void
statusBar_Init(sbar, parent)
StatusBar *sbar;
Widget parent;
{
    if (!sbar)
	return;

    sbar->sbar =
	XtVaCreateWidget("status_bar",
	    xmRowColumnWidgetClass, parent,
	    XmNmarginHeight, 0,
	    XmNmarginWidth, 0,
	    XmNresizeWidth, False,	/* Required for sane geometry */
	    XmNuserData, sbar,
#ifdef SANE_WINDOW
	    ZmNhasSash, False,		/* In case parent is a SaneWindow */
#endif /* SANE_WINDOW */
	    XmNskipAdjust, True,	/* In case parent is a PanedWindow */
	    NULL);
    XtAddCallback(sbar->sbar, XmNdestroyCallback, statusBar_DestroyCB, sbar);

    statusBar_InitPanes(sbar);

    esumm_Init(&sbar->esumm);

    sbar->varname = 0;
    sbar->cb = 0;
    sbar->refresh_id = 0;
}

StatusBar *
statusBar_Create(parent)
Widget parent;
{
    StatusBar *sbar = (StatusBar *) malloc(sizeof(StatusBar));

    statusBar_Init(sbar, parent);
    return sbar;
}

void
statusBar_Delete(sbar)
StatusBar *sbar;
{
    statusBar_Destroy(sbar);
    xfree(sbar);
}

static void
statusBar_DeleteCB(w, sbar, data)
Widget w;
XtPointer sbar, data;
{
    /* statusBar_Delete((StatusBar *)sbar); */
    xfree(sbar);
}

void
statusBar_Layout(sbar, preserve)
StatusBar *sbar;
int preserve;
{
    int i;
    long tw = esumm_GetTotalWidth(&sbar->esumm);
    esummseg_t *seg;
    int pos = 0;
    int nsegs = esumm_GetSegCount(&sbar->esumm);
    int npanes = glist_Length(&sbar->panes);
    Arg args[3];

    if (preserve) {
	/* We can't preserve if we're reducing the number of segments */
	if (npanes > nsegs)
	    preserve = 0;
    }
    if (!preserve) {
	statusBar_DestroyPanes(sbar);
	statusBar_InitPanes(sbar);
	npanes = 0;
    }

    SAVE_RESIZE(GetTopShell(sbar->sbar));
    SET_RESIZE(False);

    XtSetArg(args[0], XmNlabelString, zmXmStr(" "));

    esumm_FOREACH(&sbar->esumm, seg, i) {
	StatusPane *sp;
	if (i < npanes) {
	    sp = (StatusPane *)glist_Nth(&sbar->panes, i);
	    statusPane_Layout(sp, pos);
	} else {
	    statusPane_Create(sbar, pos);
	    sp = (StatusPane *)glist_Last(&sbar->panes);
	}
	sp->seg = seg;
	if (esummseg_GetFlags(seg, ESUMSF_RIGHT_JUST))
	    XtSetArg(args[1], XmNalignment, XmALIGNMENT_END);
	else if (esummseg_GetFlags(seg, ESUMSF_CENTER_JUST))
	    XtSetArg(args[1], XmNalignment, XmALIGNMENT_CENTER);
	else
	    XtSetArg(args[1], XmNalignment, XmALIGNMENT_BEGINNING);
	XtSetValues(sp->label, args, 2);
	pos += (SB_FRACTION_BASE * esummseg_GetWidth(seg)) / tw;
    }

    RESTORE_RESIZE();
}

void
statusBar_SetMainText(sbar, str)
StatusBar *sbar;
const char *str;
{
    StatusPane *sp;
    int i;

    if (!sbar)
	return;

    glist_FOREACH(&sbar->panes, StatusPane, sp, i) {
	if (!sp->seg)
	    continue;
	if (esummseg_GetType(sp->seg) != esumm_Status)
	    continue;
	/* XXX casting away const */
	sp->text = (char *) str;
	statusPane_Refresh(sp);
    }
}

static Boolean
statusBar_RefreshProc(sbar)
StatusBar *sbar;
{
    StatusPane *sp;
    int i, mno;
    msg_folder *save_folder = current_folder;

    if (!sbar)
	return True;

    mno = chk_msg(FrameGetMsgsStr(FrameGetData(sbar->sbar)))-1;
    current_folder = FrameGetFolder(FrameGetData(sbar->sbar));

    glist_FOREACH(&sbar->panes, StatusPane, sp, i) {
	if (!sp->seg)
	    continue;
	if (esummseg_GetType(sp->seg) == esumm_FolderFmt) {
	    /* Should optimize this somehow			XXX
		&& ison(flags, FREF_FOLDER_DATA)
	    */
	    sp->text = format_prompt(current_folder,
		esummseg_GetFormat(sp->seg));
	    statusPane_Refresh(sp);
	}
	if (esummseg_GetType(sp->seg) == esumm_MsgFmt) {
	    /* Should optimize this somehow			XXX
		&& ison(flags, FREF_FOLDER_DATA)
	    */
	    if (mno < 0)
		sp->text = "";
	    else
		sp->text = format_hdr(mno, esummseg_GetFormat(sp->seg), False);
	    statusPane_Refresh(sp);
	}
    }

    current_folder = save_folder;
    sbar->refresh_id = 0;

    return True;
}

void
statusBar_Refresh(sbar, flags)
StatusBar *sbar;
long flags;		/* Presently ignored */
{
    if (!sbar)
	return;
    if (sbar->refresh_id)
	return;
    sbar->refresh_id = XtAppAddWorkProc(app,
					(XtWorkProc) statusBar_RefreshProc,
					(XtPointer) sbar);
}

void
statusBar_SetFormat(sbar, str)
StatusBar *sbar;
char *str;
{
    if (!str || !*str)
	str = SB_DEFAULT_FMT;
    if (esumm_Parse(&sbar->esumm, str))
	statusBar_Layout(sbar, True);
    else
	statusBar_Layout(sbar, False);
    statusBar_Refresh(sbar, 0L);
}

static void
statusBar_VarChangedCB(sbar, data)
VPTR sbar;
ZmCallbackData data;
{
    statusBar_SetFormat((StatusBar *)sbar, data->xdata);
}

void
statusBar_SetVarName(sbar, varname)
StatusBar *sbar;
char *varname;
{
    if (sbar->cb)
	ZmCallbackRemove(sbar->cb);
    sbar->cb = 0;
    if (varname) {
	sbar->varname = varname;
	sbar->cb = ZmCallbackAdd(varname, ZCBTYPE_VAR,
	    statusBar_VarChangedCB, (VPTR) sbar);
	statusBar_SetFormat(sbar, value_of(varname));
    }
}

void
statusBar_SetHelpKey(sbar, key)
StatusBar *sbar;
const char *key;
{
    DialogHelpRegister(sbar->sbar, key);
}

static void
statusBar_VisibleCB(sbar, data)
VPTR sbar;
ZmCallbackData data;
{
    SAVE_RESIZE(GetTopShell(((StatusBar *)sbar)->sbar));
    SET_RESIZE(True);

    /* XXX Yuck -- poking inside of chk_option() here */
    if (chk_two_lists(data->xdata, "status_bar",  "\t ,"))
	XtManageChild(((StatusBar *)sbar)->sbar);
    else
	XtUnmanageChild(((StatusBar *)sbar)->sbar);

    RESTORE_RESIZE();
}

StatusBar *
StatusBarCreate(parent)
Widget parent;
{
    StatusBar *sbar = statusBar_Create(parent);
    char *formatvar = 0, *parentvar = 0;

    XtAddCallback(parent, XmNdestroyCallback, statusBar_DeleteCB, sbar);

    if (XtClass(parent) == xmMainWindowWidgetClass)
	XtVaSetValues(parent, XmNmessageWindow, sbar->sbar, NULL);

    /* Managed to keep this out of ToolBarCreate, but since we've already
     * got the frame stuff included above, might as well use it ...
     */
    switch (FrameGetType(FrameGetData(parent))) {
	case FrameMain:
	    formatvar = VarMainStatusBarFmt;
	    parentvar = VarMainPanes;
	    break;
	case FrameCompose:
	    formatvar = VarCompStatusBarFmt;
	    parentvar = VarComposePanes;
	    break;
	case FramePageMsg: case FramePinMsg:
	    formatvar = VarMsgStatusBarFmt;
	    parentvar = VarMessagePanes;
	    break;
	default:
	    formatvar = VarStatusBarFmt;
	    XtManageChild(sbar->sbar);
    }
    if (parentvar) {
	XtAddCallback(sbar->sbar, XmNdestroyCallback, remove_callback_cb,
		      ZmCallbackAdd(parentvar,
				    ZCBTYPE_VAR, statusBar_VisibleCB,
				    (VPTR) sbar));
	if (chk_option(parentvar, "toolbar"))
	    XtManageChild(sbar->sbar);
    }
    statusBar_SetVarName(sbar, formatvar);

    return sbar;
}
