#include "config/features.h"
#ifdef DYNAMIC_HEADERS


#include <X11/Intrinsic.h>
#include <Xm/Frame.h>
#include <Xm/RowColumn.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#define PANED_WINDOW_CLASS zmSaneWindowWidgetClass
#else /* !SANE_WINDOW */
#include <Xm/PanedW.h>
#define PANED_WINDOW_CLASS xmPanedWindowWidgetClass
#endif /* !SANE_WINDOW */
#include "zmstring.h"
#include "addressArea/addressArea.h"
#include "m_comp.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include "zprint.h"


struct HeaderPrompt {
    struct link link;
    Widget menu;
    char **choices;
};


struct DynaPrompt {
    struct HeaderPrompt *headers;
    struct AddressArea *addressing;
    struct Compose *compose;
};


#define OPTION_HEADER_W_NAME	"option_header"
#define CHANGE_HEADER_W_NAME	"set_header"
#define REMOVE_HEADER_W_NAME	"unset_header"

/* XXX SHOULD GO IN RESOURCES */
#define CHANGE_HEADER_LABEL	catgets( catalog, CAT_MOTIF, 105, "Set Header" )
#define REMOVE_HEADER_LABEL	catgets( catalog, CAT_MOTIF, 106, "Unset Header" )


static ActionAreaItem comp_hdr_acts[] = {
    { "Close",   PopdownFrameCallback,       NULL },
    { NULL,	 (void_proc)0,               NULL },
    { NULL,	 (void_proc)0,               NULL },
    { "Help",    DialogHelp,    "Compose Headers" }
};


static void
free_dyn_hdr(w, hf, data)
Widget w;
XtPointer hf;
XtPointer data;
{
    destroy_header((HeaderField *)hf);
}


void
fix_dynamic_hdr_nthpos(pair, id)
     XtPointer pair;
     XtIntervalId *id;
{
    Widget menu = ((Widget *)pair)[0];
    int n = ((int *)pair)[1];

    xfree(pair);
    SetNthOptionMenuChoice(menu, n);
}


void
fix_dynamic_hdr_choice( pair, id )
     XtPointer pair;
     XtIntervalId *id;
{
    Widget menu  = ((Widget *)pair)[0];
    Widget histw = ((Widget *)pair)[1];

    xfree(pair);
    /* SetOptionMenuChoice(menu, XtName(histw));  /* OLIT may need this */
    XtVaSetValues(menu, XmNmenuHistory, histw, NULL);
}


static void
fix_dynamic_hdr_later(menu, histw, histn)
Widget menu, histw;
int histn;
{
    if (histw)
	(void) XtAppAddTimeOut(app, 0, fix_dynamic_hdr_choice,
	    vaptr((char *) menu, histw, NULL));
    else if (histn >= 0)
	(void) XtAppAddTimeOut(app, 0, fix_dynamic_hdr_nthpos,
	    vaptr((char *) menu, (char *) histn, NULL));
}


static void
dynhdr_may_be_empty(menu, rc, hf, no_remove)
Widget menu, rc;
HeaderField *hf;
int no_remove;
{
    Widget histw = XtNameToWidget(rc, OPTION_HEADER_W_NAME);

    if (hf->hf_body && hf->hf_body[0]) {
	XtVaSetValues(histw,
	    XmNlabelString, zmXmStr(hf->hf_body),
	    NULL);
	XtUnmanageChild(XtNameToWidget(rc, REMOVE_HEADER_W_NAME));
    } else {
	XtUnmanageChild(histw);
	histw = XtNameToWidget(rc, REMOVE_HEADER_W_NAME);
    }
    XtManageChild(histw);
    fix_dynamic_hdr_later(menu, histw, -1);
}


static void
change_dynamic_hdr(w, call_data, cbs)
Widget w;
XtPointer call_data;
XmAnyCallbackStruct cbs;
{
    struct DynaPrompt *dynamic = (struct DynaPrompt *) FrameGetClientData(FrameGetData(w));
    Compose *compose = dynamic->compose;
    HeaderField *pf, *hf;
    struct HeaderPrompt *hp;
    Widget rc, menu;
    char **choices, **pp;
    int n;

    rc = XtParent(w);

    XtVaGetValues(w, XmNuserData, &pf, NULL);
    hp = (struct HeaderPrompt *)
	retrieve_link(dynamic->headers, pf->hf_name, 0);
    menu = hp->menu;
    choices = hp->choices;
    ask_item = menu;

    hf = lookup_header(&compose->headers, pf->hf_name, ", ", TRUE);
    if (!hf) {
	/* XXX This is a hack to keep edit_hdrs from killing us! */
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 107, "Cannot find header %s" ), pf->hf_name);
	fix_dynamic_hdr_later(menu,
	    XtNameToWidget(rc, REMOVE_HEADER_W_NAME), -1);
	return;
    }

    if (strcmp(XtName(w), CHANGE_HEADER_W_NAME) == 0) {
	ZSTRDUP(hf->hf_body, pf->hf_body);
	if ((n = dynamic_header(compose, hf)) != 0)
	    XtSetSensitive(w, False);
	else
	    dynhdr_may_be_empty(menu, rc, hf,
		(pf->hf_body && *(pf->hf_body) == ':'));
    } else if (strcmp(XtName(w), REMOVE_HEADER_W_NAME) == 0) {
	hf->hf_body[0] = 0;
    } else {
	XmString xm_value;
	char *value;

	XtVaGetValues(w, XmNlabelString, &xm_value, NULL);
	XmStringGetLtoR(xm_value, xmcharset, &value);

	n = (hf->hf_body && strcmp(value, hf->hf_body) != 0);

	if (n) {
	    if (pf->hf_body)
		(void) interactive_header(hf, pf->hf_body+1, value);
	    else
		ZSTRDUP(hf->hf_body, value);
	}
	XtFree(value);
	if (!n)
	    return;
	if (hf->hf_body && (pp = vindex(choices, hf->hf_body)))
	    fix_dynamic_hdr_later(menu, (Widget)0, pp - choices);
	else
	    dynhdr_may_be_empty(menu, rc, hf, pf->hf_body != 0);
    }
}


static Widget
dynamic_hdr_menu(parent, label, n, pchoices, pf, hf)
Widget parent;
char *label;
int n;			/* Number of choices */
char ***pchoices;
HeaderField *pf;	/* Copy of HeaderField struct */
HeaderField *hf;	/* Current value of the header */
{
    char *value = hf->hf_body, buf[256];
    char **choices = *pchoices, **names = DUBL_NULL, **pp;
    int i = 0, x = n, change_idx = -1, unset_idx = -1, option_idx = -1;
    Widget w, menu, *kids;

    if (n > 0) {
	names = (char **)calloc(n+1, sizeof(char *));
	for (i = 0; choices[i]; i++) {
	    (void) sprintf(buf, "choice%d", i);
	    names[i] = savestr(buf);
	}
	names[i] = NULL;
    } else
	i = 0;

    /* Always create one field for an arbitrary label value */
    option_idx = n;
    n = catv(n, &names, 1, unitv(OPTION_HEADER_W_NAME));
    i = catv(i, &choices, 1, unitv(value));

    if (x == 0 || pf->hf_body) {
	change_idx = n;
	n = catv(n, &names, 1, unitv(CHANGE_HEADER_W_NAME));
	i = catv(i, &choices, 1, unitv(CHANGE_HEADER_LABEL));
    }
    if (!value || !*value || !pf->hf_body) {
	unset_idx = n;
	n = catv(n, &names, 1, unitv(REMOVE_HEADER_W_NAME));
	i = catv(i, &choices, 1, unitv(REMOVE_HEADER_LABEL));
    }
    menu = BuildSimpleMenu(parent, hf->hf_name, names, XmMENU_OPTION,
			    pf, change_dynamic_hdr);
    if (label)
	XtVaSetValues(menu, XmNlabelString, zmXmStr(label), NULL);
    else
	XtVaSetValues(menu,
	    XmNlabelString, zmXmStr(zmVaStr("%s:", hf->hf_name)),
	    NULL);

    XtVaGetValues(menu, XmNsubMenuId, &w, NULL);
    XtVaSetValues(w,
	XmNpacking, XmPACK_COLUMN,
	XmNalignment, XmALIGNMENT_END,
	NULL);
    XtVaGetValues(w, XmNchildren, &kids, NULL);
    for (i = 0; choices[i]; i++) {
	if (i == option_idx && (!value || !*value ||
		(pp = vindex(choices, value)) && pp - choices < option_idx)) {
	    XtUnmanageChild(kids[i]);
	    if (value && *value)
		option_idx = pp - choices;
	} else
	    XtVaSetValues(kids[i],
		XmNlabelString, zmXmStr(choices[i]),
		NULL);
    }
    if (option_idx < 0 && (pp = vindex(choices, value)))
	option_idx = pp - choices;

    XtManageChild(menu);

    if (!value || !*value)
	SetNthOptionMenuChoice(menu, unset_idx);
    else if (option_idx >= 0)
	SetNthOptionMenuChoice(menu, option_idx);
    else if (change_idx >= 0)
	SetNthOptionMenuChoice(menu, change_idx);
    else
	SetNthOptionMenuChoice(menu, 0);	/* Error? */

    *pchoices = choices;
    
    return menu;
}


static void
add_dynamic_hdrs(parent, headers, chain)
Widget parent;
HeaderField **headers;
struct HeaderPrompt **chain;
{
    HeaderField *hf, *tmp, *hfnext;
    Widget rc = 0, menu;
    char **choices, *body, *label, *p;
    int n, call_it = 0;

    if (hf = *headers) {
	/* What do we do about multiple copies of the same header??  XXX */
	do {
	    hfnext = (HeaderField *)(hf->hf_link.l_next);
	    if (ison(hf->hf_flags, DYNAMIC_HEADER)) {
		p = hf->hf_body;
		/* do this here in case hf is removed from chain - pf */
		if ((n = dynhdr_to_choices(&choices, &label, &p, hf->hf_name))
			< 0) {
		    if (n == -2) {
			remove_link(headers, hf);
			destroy_header(hf);
		    }
		    continue;
		} else {
		    if (*p == ':') {
			if (label)
			    *label = 0;
			body = savestr(p);
			(void) no_newln(body);	/* Strip trailing spaces */
			call_it = 1;
			if (label)
			    *label = '(' /*)*/;
		    } else if (!choices) {
			body = savestr(zmVaStr(":ask -i %s", VAR_HDR_VALUE));
			call_it = n;
		    } else
			body = NULL;
		}
		if (!choices)
		    n = 0;
		tmp = create_header(savestr(hf->hf_name), body);
		if (!rc) {
		    /*
		    parent = XtCreateManagedWidget(NULL,
			xmScrolledWindowWidgetClass, parent, NULL, 0);
		    */
		    rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, parent,
			    XmNpacking, XmPACK_COLUMN,
			    XmNorientation, XmVERTICAL,
			    XmNalignment, XmALIGNMENT_END,
			    NULL);
		}
		/*
		 * This behavior isn't what cray wanted ...
		 *
		if (call_it)
		    (void) dynamic_header(compose, hf);
		else if (choices)
		    (void) strcpy(hf->hf_body, choices[0]);
		 *
		 * ... so let's always go with an empty body.
		 */
		hf->hf_body[0] = 0;
		turnoff(hf->hf_flags, DYNAMIC_HEADER);
		if (label) {
		    /* Label is pointing into the body we just mashed
		     * into an empty string, so we don't need to worry
		     * about how we munge it.
		     */
		    p = rindex(++label, /*(*/ ')');
		    if (p == label)
			label = NULL;
		    else
			*p = ':';
		}
		menu = dynamic_hdr_menu(rc, label, n, &choices, tmp, hf);
		XtAddCallback(menu, XmNdestroyCallback, free_dyn_hdr, tmp);

		/* This should be a function XXX */
		{
		    struct HeaderPrompt *hp =
			(struct HeaderPrompt *) malloc(sizeof(*hp));
		    hp->link.l_name = savestr(hf->hf_name);
		    hp->menu = menu;
		    hp->choices = choices;
		    
		    insert_link(chain, hp);
		}
	    }
	} while ((hf = hfnext) != *headers);
    }
    if (rc)
	XtManageChild(rc);
}


static void
popup(caller, frame)
     Widget caller;
     ZmFrame frame;
{
    Compose *compose = (Compose *)FrameGetClientData(FrameGetFrameOfParent(frame));
    struct DynaPrompt *dynamic = (struct DynaPrompt *) FrameGetClientData(frame);

    AddressAreaUse(dynamic->addressing, compose);

    /* This shouldn't be necessary, but I hate Motif. */
    XtManageChild(FrameGetChild(frame));
}


static void
popdown(caller, addressing)
     Widget caller;
     struct AddressArea *addressing;
{
    AddressAreaFlush(addressing);
    AddressAreaUse(addressing, 0);
}


static void
dynamic_destroy(dynamic)
    struct DynaPrompt *dynamic;
{
    struct HeaderPrompt *hp;

    if (hp = dynamic->headers)
	do {
	    remove_link(&dynamic->headers, hp);
	    free_vec(hp->choices);
	    xfree(hp);
	} while (hp = dynamic->headers);

    /* This is handled by callback set up in AddressAreaCreate():
    AddressAreaDestroy(dynamic->addressing);
    */
    xfree(dynamic);
}


static void
comp_dynamic_headers(item)
Widget item;
{
    ZmFrame newframe, dad;
    extern ZcIcon envelope_icon;
    Widget shell, pane;
    Compose *compose;

    shell = GetTopShell(item);
    dad = FrameGetData(shell);
    compose = (Compose *)FrameGetClientData(dad);
    if (compose) {
	/* If edit_hdrs is on, toggle it off, which blows away any dynamic
	 * headers dialog that already exists.  Then regenerate the dynamic
	 * headers dialog, which at that point probably won't have anything
	 * in it but To: Subject: etc.  This isn't ideal, but ...
	 */
	if (ison(compose->flags, EDIT_HDRS)) {
	    turnoff(compose->flags, EDIT_HDRS);
	    ZmCallbackCallAll("compose_state", ZCBTYPE_VAR, ZCB_VAR_SET, compose);
	} else if (compose->interface->dynamics) {
	    newframe = compose->interface->dynamics;
	    FramePopup(newframe);
	    return;
	}

	timeout_cursors(TRUE);

	{
	    struct DynaPrompt *dynamic = (struct DynaPrompt *) calloc(1, sizeof(*dynamic));
	    dynamic->compose = compose;
	
	    newframe = FrameCreate("comp_hdrs_dialog",
		FrameCompHdrs,	  shell,
		FrameIcon,	  &envelope_icon,
		FrameChild,	  &pane,
		FrameRefreshProc, generic_frame_refresh,
		FrameFlags,	  FRAME_SHOW_ICON | FRAME_CANNOT_RESIZE | FRAME_DIRECTIONS,
		FrameClientData,  dynamic,
		FrameFreeClient,  dynamic_destroy,
		FrameEndArgs);

	    compose->interface->dynamics = newframe;

	    {
		Widget border = XtCreateWidget(NULL, xmFrameWidgetClass, pane, NULL, 0);
		Widget layout = XtCreateWidget(NULL, PANED_WINDOW_CLASS, border, NULL, 0);
		dynamic->addressing = AddressAreaCreate(layout, NULL, NULL);
		XtManageChild(layout);
		XtManageChild(border);
	    }

	    {
		Widget frameShell = GetTopShell(pane);
		XtAddCallback(frameShell, XmNpopupCallback,   (XtCallbackProc) popup,   newframe);
		XtAddCallback(frameShell, XmNpopdownCallback, (XtCallbackProc) popdown, dynamic->addressing);
	    }
	    
	    add_dynamic_hdrs(pane, &compose->headers, &dynamic->headers);
	    
	    CreateActionArea(pane, comp_hdr_acts, XtNumber(comp_hdr_acts),
			     "Compose Headers");
	    
	    /* Don't manage pane until the popupCallback.  I hate Motif. */
	    /* XtManageChild(pane); */
	    FrameCopyContext(dad, newframe);
	    FramePopup(newframe);

	}
	timeout_cursors(FALSE);
    }
}


ZmFrame
DialogCreateDynamicHdrs()
{
    comp_dynamic_headers(ask_item);
    return (ZmFrame) 0;
}


#endif /* DYNAMIC_HEADERS */
