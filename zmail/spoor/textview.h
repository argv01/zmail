/*
 * $RCSfile: textview.h,v $
 * $Revision: 2.16 $
 * $Date: 1995/06/20 23:32:32 $
 * $Author: spencer $
 */

#ifndef SPOOR_TEXTVIEW_H
#define SPOOR_TEXTVIEW_H

#include <spoor.h>
#include <dlist.h>
#include "view.h"

#define SPTEXTVIEW_BUFLEN (256)

enum spTextview_wrapMode {
    spTextview_nowrap,
    spTextview_wordwrap,
    spTextview_charwrap,
    spTextview_WRAPMODES
};

struct spTextview {
    SUPERCLASS(spView);
    enum spTextview_wrapMode wrapmode;
    int firstVisibleLineMark, anticipatedWidth, textPosMark, colshift;
    int nonvisibleMark, bottomvisible;
    int bufValid, bufpos, bufused, goalcol, wrapcolumn;
    int selecting, selectstart, selectend, showpos;
    int changecount, truncatedlines;
    char echochar; /* character to be echoed if -noecho flag is used, e.g. */
    char buf[SPTEXTVIEW_BUFLEN];
};

#define spTextview_changecount(x) \
    (((struct spTextview *) (x))->changecount)
#define spTextview_anticipatedWidth(t) \
    (((struct spTextview *) (t))->anticipatedWidth)
#define spTextview_textPosMark(t) (((struct spTextview *) (t))->textPosMark)
#define spTextview_wrapmode(t) (((struct spTextview *) (t))->wrapmode)
#define spTextview_wrapcolumn(t) (((struct spTextview *) (t))->wrapcolumn)
#define spTextview_colshift(t) (((struct spTextview *) (t))->colshift)
#define spTextview_selecting(t) (((struct spTextview *) (t))->selecting)
#define spTextview_selectstart(t) (((struct spTextview *) (t))->selectstart)
#define spTextview_selectend(t) (((struct spTextview *) (t))->selectend)
#define spTextview_firstVisibleLineMark(t) \
    (((struct spTextview *) (t))->firstVisibleLineMark)
#define spTextview_showpos(t) (((struct spTextview *) (t))->showpos)
#define spTextview_echochar(t) (((struct spTextview *) (t))->echochar)

extern int m_spTextview_scrollLabel;
extern int m_spTextview_writeFile;
extern int m_spTextview_writePartial;
extern int m_spTextview_prevHardBol;
extern int m_spTextview_nextHardBol;
extern int m_spTextview_nextSoftBol;
extern int m_spTextview_prevSoftBol;
extern int m_spTextview_framePoint;
extern int m_spTextview_nextChange;

enum spTextview_updateFlag {
    spTextview_cursorMotion = spView_UPDATEFLAGS,
    spTextview_scrollMotion,
    spTextview_oneLineChange,
    spTextview_nonvisibleContentChange,
    spTextview_UPDATEFLAGS
};

extern struct spWclass *spTextview_class;

extern struct spWidgetInfo *spwc_Text;
extern struct spWidgetInfo *spwc_EditText;

#define spTextview_NEW() \
    ((struct spTextview *) spoor_NewInstance(spTextview_class))

extern int (*spTextview_enableScrollpct)();

extern void spTextview_InitializeClass();
extern char spTextview_getc();
extern int spTextview_selection();

#endif /* SPOOR_TEXTVIEW_H */
