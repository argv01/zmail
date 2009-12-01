/* m_fonts.c 	Copyright 1991 Z-Code Software Corp. */

#include "zmail.h"

#ifdef FONTS_DIALOG

#include "zmframe.h"
#include "catalog.h"
#include "dismiss.h"
#include "fonts_gc.h"
#include "zmstring.h"
#include "zm_motif.h"
#include <general/regexpr.h>

#include <Xm/DialogS.h>
#include <Xm/MainW.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#else /* !SANE_WINDOW */
#include <Xm/PanedW.h>
#endif /* !SANE_WINDOW */
#include <Xm/Form.h>
#include <Xm/ScrollBar.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Text.h>
/* #include <Xm/TextF.h> */
#include <Xm/List.h>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>

#if defined( HP700_10 ) || defined( HP700 )
# include <X11/Xlibint.h> /* for struct _XDisplay */
#endif /* HP700_10 */

static u_long assign_mode;
#define ASSIGN_INTERACTIVE	ULBIT(0)
#define ASSIGN_BY_CLASS		ULBIT(1)
#define ASSIGN_LABEL		ULBIT(2)
#define ASSIGN_LABEL_N_FONT	ULBIT(3)
/* hold the list of object types; desensitize when in interactive mode */
static Widget obj_rc;

/* information on missing fonts, so we can warn the user */
#if defined(XmVersion) && XmVersion >= 1002 /* Motif 1.2 or later */
static char **missingCharsets;
static int    missingCount;
#endif

static u_long class_bits;
#define FONT_PUSHBUTTON		ULBIT(0)
#define FONT_LABEL		ULBIT(1)
#define FONT_TOGGLE		ULBIT(2)
#define FONT_TEXT		ULBIT(3)
#define FONT_LIST		ULBIT(4)
#define FONT_MENU		ULBIT(5)

static char *types[] = {
  "PushButtons", "Labels", "ToggleButtons", "Texts", "Lists", "Menus"
};

#define CLASS_NAME_COLUMNS 11
static char *class_names[][CLASS_NAME_COLUMNS] = {
    { "XmPushButton", "XmPushButtonGadget" },
    { "XmLabel", "XmLabelGadget" },
    { "XmToggleButton", "XmToggleButtonGadget" },
    { "XmText", /* "XmTextField" */ },
    { "XmList", NULL },
    /* The specific entries are necessary to override schemes on SGIs */
    { "XmMenuShell*XmRowColumn", 
	  "XmMenuShell*XmLabel",
	  "XmMenuShell*XmLabelGadget",
	  "XmMenuShell*XmPushButton",
	  "XmMenuShell*XmPushButtonGadget",
	  "XmMenuShell*XmCascadeButton",
	  "XmMenuShell*XmCascadeButtonGadget",
	  "XmMenuShell*XmToggleButton",
	  "XmMenuShell*XmToggleButtonGadget",
	  "XmCascadeButton",
	  "XmCascadeButtonGadget"}
};
static WidgetClass *classes[][2] = {
    { &xmPushButtonWidgetClass, &xmPushButtonGadgetClass },
    { &xmLabelWidgetClass, &xmLabelGadgetClass },
    { &xmToggleButtonWidgetClass, &xmToggleButtonGadgetClass },
    { &xmTextWidgetClass, /* &xmTextFieldWidgetClass */ },
    { &xmListWidgetClass, (WidgetClass *)0 },
    { &xmCascadeButtonWidgetClass, &xmCascadeButtonGadgetClass },
};

XrmDatabase fonts_db; /* store font stuff in our own database handle */

#define fontcursor_width 16
#define fontcursor_height 16
#define fontcursor_x_hot 0
#define fontcursor_y_hot 16
static char fontcursor_bits[] = {
   0x00, 0x20, 0x00, 0x50, 0x00, 0x98, 0x00, 0x6c, 0x00, 0x32, 0x00, 0x11,
   0x80, 0x0a, 0x40, 0x05, 0xa0, 0x02, 0x10, 0x01, 0x88, 0x00, 0x44, 0x00,
   0x2c, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x01, 0x00
};

#define fontcursor_mask_width 16
#define fontcursor_mask_height 16
#define fontcursor_mask_x_hot 0
#define fontcursor_mask_y_hot 16
static char fontcursor_mask_bits[] = {
   0x00, 0x20, 0x00, 0x70, 0x00, 0xf8, 0x00, 0x7c, 0x00, 0x3e, 0x00, 0x1f,
   0x80, 0x0f, 0xc0, 0x07, 0xe0, 0x03, 0xf0, 0x01, 0xf8, 0x00, 0x7c, 0x00,
   0x3c, 0x00, 0x1e, 0x00, 0x06, 0x00, 0x01, 0x00
};

static Widget sample, specify;

static int assign_font P((Widget, XmFontList, char *, Widget));
static int set_type P((Widget, XmFontList, char *));
static void start_assign P((Widget));
static void change_mode P((void));
static void list_select P((Widget, Widget, XmListCallbackStruct *));
static void set_specified_fonts P((Widget, Widget, XmListCallbackStruct *));
static void save_fonts P((Widget));

static ActionAreaItem actions[] = {
    { "Assign", start_assign, NULL },
#ifndef MEDIAMAIL
    { "Save",   save_fonts, NULL },
#endif /* !MEDIAMAIL */
    { DONE_STR, PopdownFrameCallback, NULL },
    { "Help",   DialogHelp,   "Fonts Dialog" },
};

#include "bitmaps/fonts.xbm"
ZcIcon fonts_icon = {
    "fonts_icon", 0, fonts_width, fonts_height, fonts_bits
};

#define BROKEN_FONTS_VENDOR "DECWINDOWS Digital"
#define BROKEN_FONTS_REGEXP "^-[^-]*-[^-]*-[^-]*-[^-]*-[^-]*-[^-]*-0-0-"

ZmFrame
DialogCreateFontSetter(parent)
Widget parent;
{
    Widget form, pane, list, directions;
    Pixmap pix;
    ZmFrame newframe;
    XmStringTable items = 0;
    regexp_t pattern = (regexp_t) calloc(1, sizeof(*pattern));
    const int filtering = !(strncmp(ServerVendor(display), BROKEN_FONTS_VENDOR, sizeof(BROKEN_FONTS_VENDOR) - 1)
			    || re_compile_pattern(BROKEN_FONTS_REGEXP, sizeof(BROKEN_FONTS_REGEXP) - 1, pattern));
    int allFontCount, safeFontCount;
    char **safeFonts, **allFonts = XListFonts(display, "*", 32767, &allFontCount);
    unsigned filter;
    
    if (allFontCount == 0)
      {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 183, "Can't seem to load any fonts!" ));
	timeout_cursors(False);
	return (ZmFrame)NULL;
      }

    /*
     * Unfortunately, X does not take kindly to its font name lists
     * being qsort()'ed or otherwise tickled.  So we make a copy of
     * the list and sort that instead.
     */
    safeFonts = (char **) malloc(allFontCount * sizeof(char *));
    for (filter = 0, safeFontCount = 0; filter < allFontCount; filter++)
      /*
       * While copying, filter out scalable fonts if it looks like our
       * X server cannot handle them (i.e., was made by DEC).
       * Hopefully, the compiler will factor out the constant portion
       * of the "if" conditional during optimization....
       */
      if (!filtering || re_match(pattern, allFonts[filter], strlen(allFonts[filter]), 0, 0) == -1)
	safeFonts[safeFontCount++] = savestr(allFonts[filter]);

    free(pattern);
    XFreeFontNames(allFonts);
    /* Shrink a little if font names were indeed filtered out. */
    safeFonts = (char **) realloc(safeFonts, safeFontCount * sizeof(char *));

    qsort((VPTR) &safeFonts[0], safeFontCount, sizeof(safeFonts[0]),
	  (int (*)P((CVPTR, CVPTR))) strptrcmp);

    reference_init();  /* start up font reference counting */

    newframe = FrameCreate("fonts_dialog", FrameFontSetter, parent,
	FrameIcon,	&fonts_icon,
#ifdef SANE_WINDOW
	FrameChildClass,   zmSaneWindowWidgetClass,
#endif /* SANE_WINDOW */
	FrameClass,	applicationShellWidgetClass,
	FrameIsPopup,   False,
	FrameFlags,     FRAME_CANNOT_SHRINK,
	FrameChild,	&pane,
	FrameEndArgs);
    XtVaSetValues(GetTopShell(pane), XmNdeleteResponse, XmUNMAP, NULL);
    XtVaSetValues(pane,
	XmNseparatorOn,	True,
	XmNsashWidth,	10,
	XmNsashHeight,	10,
	NULL);

    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);
    FrameGet(newframe, FrameIconPix, &pix, FrameEndArgs);
    {
      Widget directions = XtVaCreateManagedWidget("directions", xmLabelGadgetClass, form,
	  XmNalignment,       XmALIGNMENT_BEGINNING,
	  XmNleftAttachment,  XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  NULL);
      Widget icon_w = XtVaCreateManagedWidget(fonts_icon.var, xmLabelWidgetClass, form,
	  XmNlabelType,       XmPIXMAP,
	  XmNlabelPixmap,     pix,
	  XmNuserData,        &fonts_icon,
#ifdef NOT_NOW
	  XmNleftAttachment,  XmATTACH_WIDGET,
	  XmNleftWidget,      directions,
#endif /* NOT_NOW */
	  XmNalignment,       XmALIGNMENT_END,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  NULL);
      XtVaSetValues(directions,
	  XmNrightAttachment, XmATTACH_WIDGET,
	  XmNrightWidget,	    icon_w,
	  NULL);
      
      FrameSet(newframe,
	  FrameFlagOn,         FRAME_SHOW_ICON,
	  FrameIconItem,       icon_w,
	  NULL);
    }
    XtManageChild(form);
    
    {
      Arg args[4];
      unsigned liberate;
      XmStringTable safeFontTable = ArgvToXmStringTable(safeFontCount, safeFonts);
      Widget rowcol = XtVaCreateManagedWidget(NULL, xmFormWidgetClass,
	  pane, 0);
#ifdef SANE_WINDOW
      XtVaSetValues(rowcol,
	ZmNextResizable, True,
	ZmNhasSash,	 True,
	XmNallowResize,	 False,
	NULL);
#endif /* SANE_WINDOW */

      /* Get rid of our local, sorted font name list. */
      for (liberate = safeFontCount; liberate--; )
	free(safeFonts[liberate]);
      free(safeFonts);
      
      XtSetArg(args[0], XmNitems, safeFontTable);
      XtSetArg(args[1], XmNitemCount, safeFontCount);
      XtSetArg(args[2], XmNvisibleItemCount, 5);
#if defined(XmVersion) && XmVersion >= 1002  /* Motif 1.2 or later */
      XtSetArg(args[3], XmNselectionPolicy, XmEXTENDED_SELECT);
#else
      XtSetArg(args[3], XmNselectionPolicy, XmBROWSE_SELECT);
#endif /* Motif 1.2 or later */
      list = XmCreateScrolledList(rowcol, "font_list", args, 4);
      XtManageChild(list);
      XmStringFreeTable(safeFontTable);
      
      specify = CreateLabeledTextForm("specify", rowcol, NULL);
      XtVaSetValues(XtParent(list),
	  XmNtopAttachment,      XmATTACH_FORM,
	  XmNbottomAttachment,	 XmATTACH_WIDGET,
	  XmNbottomWidget,	 XtParent(specify),
	  XmNleftAttachment,	 XmATTACH_FORM,
	  XmNrightAttachment,	 XmATTACH_FORM,
	  NULL);
      XtVaSetValues(XtParent(specify),
	  XmNbottomAttachment,	XmATTACH_FORM,
	  XmNleftAttachment,	XmATTACH_FORM,
	  XmNrightAttachment,	XmATTACH_FORM,
	  NULL);
    }
    
    {
      Arg args[2];
      
      XtSetArg(args[0], XmNeditMode, XmMULTI_LINE_EDIT);
      XtSetArg(args[1], XmNeditable, True);
      
      sample = XmCreateScrolledText(pane, "sample", args, 2);
#ifdef SANE_WINDOW
      XtVaSetValues(XtParent(sample),
	  ZmNextResizable, True,
	  XmNskipAdjust,   True,
	  XmNallowResize,  False,
	  NULL);
#endif /* SANE_WINDOW */
      XtManageChild(sample);
    }

    ListInstallNavigator(list);

#if defined(XmVersion) && XmVersion >= 1002  /* Motif 1.2 or later */
    XtAddCallback(list, XmNextendedSelectionCallback,
		  (XtCallbackProc) list_select, specify);
#else
    XtAddCallback(list, XmNbrowseSelectionCallback,
		  (XtCallbackProc) list_select, specify);
#endif /* Motif 1.2 or later */
    
    XtAddCallback(specify, XmNactivateCallback,
		  (XtCallbackProc) set_specified_fonts, sample);
    XmListSelectPos(list, 1, True);
    
    form = XtVaCreateWidget(NULL, xmFormWidgetClass, pane, NULL);
    {
	Widget toggle_box;
	obj_rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, form,
	    XmNleftAttachment,   XmATTACH_FORM,
	    XmNtopAttachment,    XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtVaCreateManagedWidget("obj_label", xmLabelGadgetClass, obj_rc, NULL);
	toggle_box = CreateToggleBox(obj_rc, False, True, False, (void_proc)0,
	    &class_bits, NULL, types, XtNumber(types));
	XtVaSetValues(toggle_box,
	    XmNnumColumns,  3,
	    XmNpacking,     XmPACK_COLUMN,
	    XmNorientation, XmHORIZONTAL,
	    NULL);
	XtManageChild(toggle_box);
	XtManageChild(obj_rc);
    }
    assign_mode = ASSIGN_BY_CLASS;
    {
	char *choices[4];
	Widget rc = XtVaCreateWidget(NULL, xmRowColumnWidgetClass, form,
	    XmNleftAttachment,   XmATTACH_WIDGET,
	    XmNleftWidget,	 obj_rc,
	    XmNrightAttachment,  XmATTACH_FORM,
	    XmNtopAttachment,    XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM,
	    NULL);
	XtVaCreateManagedWidget("set_mode", xmLabelGadgetClass, rc, NULL);
	/* Bart: Wed Aug 19 15:59:42 PDT 1992 -- changes in CreateToggleBox */
	choices[0] = "interactive";
	choices[1] = "object_type";
	choices[2] = "label_only";
	choices[3] = "label_font";
	{
	  Widget toggles = CreateToggleBox(rc, False, False, True,
	    change_mode, &assign_mode, NULL, choices, XtNumber(choices));
	  XtVaSetValues(toggles, XmNnumColumns,	 2,
				 XmNpacking,	 XmPACK_COLUMN,
				 XmNorientation, XmHORIZONTAL,
				 NULL);
	  XtManageChild(toggles);
	}
	XtManageChild(rc);
    }
    XtManageChild(form);
 
    {
	Widget actionArea = CreateActionArea(pane, actions, XtNumber(actions), "Font Dialog");
	FrameSet(newframe, FrameDismissButton, GetNthChild(actionArea, XtNumber(actions) - 2),
		 FrameEndArgs);
    }

    XtManageChild(pane);
    return newframe;
}

static void
change_mode()
{
    XtSetSensitive(obj_rc, ison(assign_mode, ASSIGN_BY_CLASS) != 0);
}


static void
change_widget_font( widget, newList )
     Widget widget;
     XmFontList newList;
{
  /* Swap in the new font list */
  XmFontList oldList;
  Boolean oldResize;

  /* Motif bug workaround -- Form widgets can go into an infinite loop
   * trying to adjust the size of their children when the font changes.
   * Temporarily prevent those children from resizing while the font
   * change is applied, then restore the original resizability state.
   * This is a harmless no-op if the widget's parent is not a Form.
   */
  XtVaGetValues( widget,
		 XmNfontList, &oldList,
		 XmNresizable, &oldResize,
		 (VPTR)0 );
  XtVaSetValues( widget,
		 XmNresizable, False,
  		 XmNfontList,  newList,
		 (VPTR)0 ); 
  XtVaSetValues( widget,
		 XmNresizable, oldResize,
		 (VPTR)0 ); 

  /* Update reference counts, possibly garbage collecting */
  reference_remove( oldList );
  reference_add(    newList );
}


static int
assign_font(widget, fontList, fontNames, handle)
     Widget widget;
     XmFontList fontList;
     char *fontNames;
     Widget handle;
{
    static Cursor cursor;
    int do_loop = !widget;
    Boolean changedDismiss = False;

    if (do_loop && !cursor && !(cursor = create_cursor(
	    fontcursor_bits, fontcursor_width, fontcursor_height,
	    fontcursor_mask_bits, fontcursor_mask_width, fontcursor_mask_height,
	    fontcursor_x_hot, fontcursor_y_hot))) {
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 187, "can't create cursor for fonts." ));
	return -1;
    }
    /* Bart: Tue Jul 14 17:23:44 PDT 1992
     * XmTrackingLocate() doesn't seem to find the right widgets,
     * so we're trying our own.
     */
    while (!do_loop || (widget = XtTrackingLocate(tool, cursor, False))) {
	char *WidgetString(), *w_path, *new_label = NULL;
	unsigned char type;
	Pixmap pix;
	Boolean valid;
	Boolean modified = False;

	ask_item = widget;
	if (!(valid = ValidWidgetName(XtName(widget))) ||
	    !(w_path = WidgetString(widget)) || !*w_path) {
	    /* can't set a font on a widget with no name.  The value will
	     * not be able to be saved in the database, so why confuse
	     * the user.  This is a bug on our part, so someone should
	     * go thru the code and make sure all "visible" widgets that
	     * can have text can also have a name.
	     */
	    if (isoff(assign_mode, ASSIGN_BY_CLASS))
		error(ZmErrWarning,
		      catgets( catalog, CAT_MOTIF, 188, "This widget has an invalid %s (%s), so it cannot be modified." ),
		      valid? catgets( catalog, CAT_MOTIF, 189, "path" ) : catgets( catalog, CAT_MOTIF, 190, "name" ), valid? w_path : XtName(widget));
	    return 0;
	}

	if (ison(assign_mode, ASSIGN_LABEL | ASSIGN_LABEL_N_FONT)) {
	    ZcIcon *zc_icon = 0, *new_icon = 0;
	    char *s;
	    int status;
	    XmString old_label;
	    if (!XtIsSubclass(widget, xmLabelWidgetClass) &&
		!XtIsSubclass(widget, xmLabelGadgetClass)) {
		error(UserErrWarning, catgets( catalog, CAT_MOTIF, 191, "This object (%s) cannot be changed." ),
		    XtName(widget));
		break;
	    }
	    XtVaGetValues(widget,
		XmNlabelString, &old_label,
		XmNlabelType,   &type,
		XmNlabelPixmap,	&pix,
		XmNuserData,    &zc_icon,
		NULL);
	    if (type == XmSTRING) {
		status = XmStringGetLtoR(old_label, xmcharset, &s);
		XmStringFree(old_label);
		if (!status || !s || !*s) {
		    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 192, "Error getting object's label." ));
		    XtFree(s);
		    break;
		}
	    } else
		s = NULL;
	    if ((new_label = PromptBox(GetTopShell(widget),
			type == XmSTRING? catgets( catalog, CAT_MOTIF, 193, "Label:" )
				        : catgets( catalog, CAT_MOTIF, 194, "New Bitmap File:" ),
			s, NULL, 0,
			type == XmSTRING? 0 :
				PB_FILE_BOX|PB_MUST_EXIST|PB_NOT_A_DIR,
			0)) &&
		*new_label) {
		if (type == XmSTRING) {
		    XtVaSetValues(widget,
			XmNlabelString, zmXmStr(new_label), NULL);
		    modified = True;
		} else {
		    Pixel fg, bg;
		    Pixmap newpix;
		    XtVaGetValues(widget,
			XmNforeground, &fg,
			XmNbackground, &bg,
			NULL);
		    if (zc_icon &&
			    XtClass(widget) != xmToggleButtonWidgetClass &&
			    XtClass(widget) != xmToggleButtonGadgetClass &&
			    (XtIsSubclass(widget, xmLabelWidgetClass) ||
			    XtIsSubclass(widget, xmLabelGadgetClass))) {
			/* Potential leak here, but nothing we can do about
			 * it -- the zc_icon may be shared by other labels,
			 * so we can't muck with it's internals, nor can we
			 * free it.  C'est la vie.
			 */
			new_icon = zmNew(ZcIcon);
			*new_icon = *zc_icon;
			new_icon->var = savestr(zc_icon->var);
			new_icon->filename = savestr(new_label);
			new_icon->pixmap = 0;
			load_icons(widget, new_icon, 1, &new_icon->pixmap);
			newpix = new_icon->pixmap;
			if (newpix != XmUNSPECIFIED_PIXMAP) {
			    if (pix != zc_icon->pixmap)
				XmDestroyPixmap(XtScreen(widget), pix);
			    XtVaSetValues(widget, XmNuserData, new_icon, NULL);
			} else {
			    xfree(new_icon->var);
			    xfree(new_icon->filename);
			    xfree(new_icon);
			}
		    } else {
			newpix =
			    XmGetPixmap(XtScreen(widget), new_label, fg, bg);
			if (newpix != XmUNSPECIFIED_PIXMAP)
			    XmDestroyPixmap(XtScreen(widget), pix);
		    }
		    if (newpix != XmUNSPECIFIED_PIXMAP) {
			XtVaSetValues(widget, XmNlabelPixmap, newpix, NULL);
			modified = True;
		    } else {
		      XtFree(s);
		      XtFree(new_label);
			return -1;
		    }
		}
	    }
	    XtFree(s);
	    if (!new_label)
		break;
	    do_loop = 0;
	}

	if (isoff( assign_mode, ASSIGN_LABEL )) /* we *are* assigning font */
	  {
	    change_widget_font( widget, fontList );
	    modified = True;
	    
	    if (ison( assign_mode, ASSIGN_INTERACTIVE | ASSIGN_LABEL_N_FONT ))
	      {
		XrmPutStringResource(&display->db, zmVaStr("%s.%s", w_path, XmNfontList), fontNames);
		XrmPutStringResource(&fonts_db,    zmVaStr(NULL),                         fontNames);
	      }
	  }
	
	if (ison( assign_mode, ASSIGN_LABEL | ASSIGN_LABEL_N_FONT ))
	  {
	    /* assigning label (or pixmap) */
	    XrmPutStringResource(&display->db, zmVaStr("%s.%s", w_path, (type == XmSTRING)? XmNlabelString : XmNlabelPixmap), new_label);
	    XrmPutStringResource(&fonts_db,    zmVaStr(NULL), new_label);
	  }
 	if (new_label) XtFree(new_label);

	if (modified && !changedDismiss) {
	    DismissSetWidget(handle, DismissClose);
	    changedDismiss = True;
	}

	if (!do_loop)
	  break;
    }
    return 0;
}


/*
 * Convert comma-delimited list to semicolon-delimited, and add a
 * closing colon if there was at least one delimiter
 */

static void
convert_punctuation( names )
     char *names;
{
  char isFontSet = 0;
  register char *scan;

  for (scan = &names[1]; *scan; scan++)
    if (*scan == ',')
      {
	*scan = ';';
	isFontSet = 1;
      }

  if (isFontSet)
    {
      names = (char *) realloc( names, scan - names + 2 );
      strcat( names, ":" );
    }
}


#if defined(XmVersion) && XmVersion >= 1002  /* Motif 1.2 or later */
static int
accept_partial()
{
  if (missingCount)
    {
      char *missing = 0;
      unsigned setNum;
      AskAnswer answer;
      
      for (setNum = 0; setNum < missingCount; setNum++)
	strapp(&missing, zmVaStr("\n   %s", missingCharsets[setNum]));

      answer = ask(WarnNo, catgets(catalog, CAT_MOTIF, 918, "You have not selected any fonts for the following character sets:\n   %s\n\nDo you wish to use these fonts anyway?"), missing);
      xfree(missing);
      return answer == AskYes;
    }
  else
    return 1;
}
#endif /* Motif 1.2 or later */


static void
start_assign(button)
     Widget button;
{
  int n, m;
  XmFontList fontList;
  char *fontNames;
  
#if defined(XmVersion) && XmVersion >= 1002  /* Motif 1.2 or later */
  if (accept_partial())
#endif /* Motif 1.2 or later */
    {
      XtVaGetValues(sample, XmNfontList, &fontList, 0);
      XtVaGetValues(specify, XmNvalue, &fontNames, 0);
      convert_punctuation(fontNames);
      
      ask_item = button;
      if (isoff(assign_mode, ASSIGN_BY_CLASS)) {
	(void) assign_font(0, fontList, fontNames, button);
	XtFree(fontNames);
	return;
      } else if (class_bits == NO_FLAGS) {
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 186, "Select at least one class to change font." ));
	XtFree(fontNames);
	return;
      }
      
      timeout_cursors(True);
      if (set_type((Widget)0, fontList, fontNames) == 0) {
	  DismissSetWidget(button, DismissClose);
	  for (n = 0; n < XtNumber(types); n++)
	      if (ison(class_bits, ULBIT(n)))
		  for (m = 0; m < CLASS_NAME_COLUMNS; m++)
		      if (class_names[n][m]) {
			  XrmPutStringResource(&fonts_db,
					       zmVaStr("%s*%s.%s", ZM_APP_CLASS,
						       class_names[n][m], XmNfontList),
					       fontNames);
			  XrmPutStringResource(&display->db, zmVaStr(NULL), fontNames);
		      }
      }
      XtFree(fontNames);
      timeout_cursors(False);
    }
}


/* set the "font" of all widgets of a particular class...
 * recursive function.
 * return 0 on success, -1 on failure. */
static int
set_type(widget, fontList, fontNames)
Widget widget;
XmFontList fontList;
char *fontNames;
{
    int n, m;
    Widget *kids, menu, child;
    u_long flags;
    FrameTypeName type;

    if (!widget) {
	ZmFrame frame, next;

	/* loop thru all *open* frames except this one */
	frame = frame_list;
	do  {
	    next = nextFrame(frame);
	    FrameGet(frame,
		FrameType,  &type,
		FrameFlags, &flags,
		FrameChild, &child,
		FrameEndArgs);

	    if (isoff(flags, FRAME_WAS_DESTROYED) && type != FrameFontSetter)
		set_type(GetTopShell(child), fontList, fontNames);
	    frame = next;
	} while (frame_list && frame != frame_list);
	return 0;
    }

    if (XtIsComposite(widget)) {
	XtVaGetValues(widget,
	    XmNchildren, &kids,
	    XmNnumChildren, &n,
	    NULL);
	while (n--)
	    if (set_type(kids[n], fontList, fontNames) == -1)
		return -1;
    }
    for (n = 0; n < XtNumber(types); n++)
	if (ison(class_bits, ULBIT(n)))
	    for (m = 0; m < 2; m++)
		if (classes[n][m] && XtIsSubclass(widget, *classes[n][m])) {
		    if (assign_font(widget, fontList, fontNames, widget) == -1)
			return -1;
		    if (ison(class_bits, FONT_MENU) &&
			(XmIsCascadeButton(widget) ||
			 XmIsCascadeButtonGadget(widget))) {
			u_long old_bits = class_bits;
			turnon(class_bits, FONT_LABEL+FONT_TOGGLE);
			XtVaGetValues(widget, XmNsubMenuId, &menu, NULL);
			if (menu)
			    set_type(menu, fontList, fontNames);
			class_bits = old_bits;
		    }
		}
    return 0;
}

static void
list_select(widget, specify, cbs)
    Widget widget;
    Widget specify;
    XmListCallbackStruct *cbs;
{
#if defined(XmVersion) && XmVersion >= 1002  /* Motif 1.2 or later */
  int item;
  char *selection;

  /* Build a comma-delimited string out of the array of selected
     items.  Almost a job for joinv(), but not quite. */

  zmXmTextSetString(specify, "");
  
  for (item = 0; item < cbs->selected_item_count; item++)
    {
      char *selection;
      if (item) XmTextInsert(specify, XmTextGetLastPosition(specify), ", ");
      XmStringGetLtoR(cbs->selected_items[item], xmcharset, &selection);
      XmTextInsert(specify, XmTextGetLastPosition(specify), selection);
      XtFree(selection);
    }
#else
  char *selection;
  
  XmStringGetLtoR(cbs->item, xmcharset, &selection);
  SetTextString(specify, selection);
  XtFree(selection);
#endif /* Motif 1.2 or later */
  XtCallActionProc(specify, "activate", 0, 0, 0);
}

#if defined(XmVersion) && XmVersion >= 1002 /* Motif 1.2 or later */
static char **
get_font_names(str)
  char *str;
{
  char **names;
  char *save, *p, *end;
  int cnt = 0;

  if (!str || !*str)
    return DUBL_NULL;

  names = DUBL_NULL;
  /* when we exit this loop, there is always one more piece to be copied */
  for (save = str; end = any(save, ",;"); cnt++)
  {
    *end = '\0';
    if (catv(cnt, &names, 1, unitv(save)) < 0)
      return DUBL_NULL;
    for (save = ++end; isspace(*save); save++)
      ;
    if ('\0' == *save)
      return names;
  }
  if (catv(cnt, &names, 1, unitv(save)) < 0)
    return DUBL_NULL;
  return names;
}
#endif /* Motif 1.2 or later */

static void
set_specified_fonts(widget, sample, cbs) 
    Widget widget;
    Widget sample;
    XmListCallbackStruct *cbs;
{
    char *fontNames = XmTextGetString(widget);
    XmFontList fontList;
    
#if defined(XmVersion) && XmVersion >= 1002 /* Motif 1.2 or later */
    {
      char **allFonts  = get_font_names(fontNames);
      char **badFonts  = 0;
      char **goodFonts = 0;
      char *p = NULL;

      if (allFonts)
	{
	  unsigned element;
	  for (element = 0; allFonts[element]; element++)
	    {
	      XFontStruct *found = XLoadQueryFont(display, allFonts[element]);
	      if (found)
		{
		  XFreeFont(display, found);
		  vcat(&goodFonts, unitv(allFonts[element]));
		}
	      else
		vcat(&badFonts, unitv(allFonts[element]));
	    }

	  if ((p = joinv(NULL, badFonts, "\n   ")) != NULL)
          {
	    error(UserErrWarning, zmVaStr(catgets(catalog, CAT_MOTIF, 763, "Cannot locate any fonts with the following names:\n\n   %s"), p));
	    free(p);
          }

	  p = joinv(NULL, goodFonts, ",");

	  free_vec(allFonts);
	  free_vec(badFonts);
	  free_vec(goodFonts);
	}
      XtFree((char *)missingCharsets);
      fontList = XmFontListAppendEntry(0,
        XmFontListEntryCreate(xmcharset, XmFONT_IS_FONTSET,
			      XCreateFontSet(display, (p ? p : fontNames),
					     &missingCharsets, &missingCount,
					     0)));
      if (p) free(p);
    }
#else /* not Motif 1.2 or later */
    {
      XFontStruct *fontStruct = XLoadQueryFont(display, fontNames);
      if (!fontStruct)
	{
	  ask_item = 0;
	  error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 196, "Cannot locate any font named\n\"%s\"." ), fontNames);
	  return;
	}
      fontList = XmFontListCreate(fontStruct, xmcharset);
    }
#endif /* Motif 1.2 or later */
    
    if (fontList) change_widget_font( sample, fontList );
    
    XtFree( fontNames );
}


extern int save_load_colors_n_fonts();

static void
save_fonts(button)
    Widget button;
{
    ask_item = button;
    if (fonts_db)
	save_load_colors_n_fonts(button, VarFontsDB, &fonts_db, FONTS_FILE, PainterSave);
    else
	error(UserErrWarning,
	      catgets( catalog, CAT_MOTIF, 198, "You haven't changed any fonts yet." ));
}

int
save_load_fonts(w, disposition)
Widget w;
enum PainterDisposition disposition;
{
    ask_item = w;
    if (disposition == PainterLoad || fonts_db)
	return save_load_colors_n_fonts(w, VarFontsDB, &fonts_db,
					FONTS_FILE, disposition);
}
#endif /* FONTS_DIALOG */
