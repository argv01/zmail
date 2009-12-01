/* walktag.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifdef SPTX21
#define NEED_U_LONG
#endif /* SPTX21 */

#include "osconfig.h"
#include "zmstring.h"
#include "addressArea.h"
#include "vars.h"
#include "walktag.h"
#include "zm_motif.h"
#include "zmcomp.h"

/*
 * Functions to walk to each of the "tagged" positions -- the header tags
 * or labels -- in the "raw" address header editing text widget.  This is
 * the set of callbacks, etc. that skip from To: to Subject: to Cc: etc.
 * each time carriage return is pressed in the header text widget.  The
 * names are rather obscure because most of them were copied from m_edit.c,
 * which in turn got them from some really ancient SunView code.
 *
 * Hence "textsw" for "text sub window" ... the "tilde" references are to
 * the corresponding "tilde commands" in the CLI interface.
 *
 * Relish this, your history lesson for the day.
 */

/* Set and get the current state of "walking" the raw headers area.
 * Valid values for "state" are 0 (zero) and any value returned by a
 * previous call to this function; any other values produce undefined
 * results.  Assigns the parameter state and returns the old state.
 */
unsigned long
AddressAreaWalkRaw(prompter, state)
struct AddressArea *prompter;
unsigned long state;
{
    unsigned long oldstate = prompter->pos_flags;

    prompter->pos_flags = state;

    return oldstate;
}

/* Return the byte position in the textsw of the header specified */
XmTextPosition
header_position(textsw, str)
Widget textsw;
char *str;
{
    register char *buf, *b, *B, *p, *p2;
    int add_newline = 0;
    XmTextPosition pos = 0L, ret_pos = 0L;

    buf = XmTextGetString(textsw);
    for (B = b = buf; *B; b = B) {
	/* get a line at a time from the textsw */
	if (p = index(b, '\n'))
	    *p = 0, B = p+1;
	else
	    add_newline++, B += strlen(b);
	p = b;
	skipspaces(0);
	/* Questionable what to do here -- treat newline-terminated
	 * whitespace as continued header or as separator from body?
	 * Currently treated as end of headers (separator).
	 */
	if (!*p) { /* newline alone -- end of headers */
	    if (!str || !*str)
		ret_pos = pos + 1;
	    break;
	}
	pos += B - b; /* advance position to next line */
	if (str && *b != ' ' && *b != '\t') {
	    /* strcmp ignoring case */
	    for (p2 = str; *p && *p2 && lower(*p2) == lower(*p); ++p, ++p2)
		;
	    /* MATCH is true if p2 is at the end of str and *p is ':' */
	    if (*p2 || *p != ':') {
		if (!*p2 && (!(p2 = any(p, ": \t")) || isspace(*p2))) {
		    /* Not a legal or continued header */
		    pos -= B - b; /* restore position to this line */
		    break;
		}
	    } else
		ret_pos = pos - 1;
	}
    }
    if (!ret_pos && str && *str) {
	char buf2[32];

	/* coudn't find the header -- add it */
	p = buf2;
	if (add_newline) {
	    pos = XmTextGetLastPosition(textsw);
	    *p++ = buf[strlen(buf) - 1], *p++ = '\n';
	}
	for (p2 = str; *p2; ++p2) {
	    if (p2 == str || p2[-1] == '-')
		*p++ = upper(*p2);
	    else
		*p++ = *p2;
	}
	*p++ = ':', *p++ = ' ', *p++ = '\n', *p = 0;
	zmXmTextReplace(textsw, pos - add_newline, pos, buf2);
	ret_pos = pos + strlen(buf2) - 1 - add_newline;
    }
    XtFree(buf);
    return ret_pos;
}

static char *tilde_hdrs[] = {
#define POSITION_TO	ULBIT(0)
    "to",
#define POSITION_SUBJ	ULBIT(1)
    "subject",
#define POSITION_CC	ULBIT(2)
    "cc",
#define POSITION_BCC	ULBIT(3)
    "bcc",
#define POSITION_FCC	ULBIT(4)
    "fcc"
};
#define POSITION_END	ULBIT(5)
#define POSITION_ALL \
    (POSITION_TO | POSITION_SUBJ | POSITION_CC | POSITION_BCC | POSITION_END)
#define TOTAL_POSITIONS	6

/*
 * position_flags identifies which header is requested by the calling func.
 * use header_position to find the position of the header associated with
 * with the flags.
 */
static void
go_to_next_pos(textsw, position_flags)
Widget textsw;
u_long *position_flags;
{
    XmTextPosition pos;
    u_long pos_flags;
    int i = 0;
    static Boolean srching = False;	/* see below */

    if (srching)
	return;

    while (i < TOTAL_POSITIONS && isoff(*position_flags, ULBIT(i)))
	i++;
    if (i == TOTAL_POSITIONS)
	return;
    if (i < ArraySize(tilde_hdrs)) {
	/* header_position() may attempt to add missing headers, which
	 * will cause a callback to motion_verify(), which may in turn
	 * call this function.  Toggle srching to prevent infinite loop.
	 */
	srching = True;
	pos = header_position(textsw, tilde_hdrs[i]);
	srching = False;
    } else
	pos = header_position(textsw, NULL);
    turnoff(*position_flags, ULBIT(i));
    /* Setting the position will cause a call to motion_verify().
     * Save/restore the position flags so it doesn't clobber them.
     */
    pos_flags = *position_flags;
    *position_flags = 0; /* save progress_callback() some work */

    /* we couldn't have called this without intending to
     * set focus to the compose Text widget.
     */
    SetTextInput(textsw);	/* Changes cursor position */

    XmTextSetCursorPosition(textsw, pos);
    XmTextShowPosition(textsw, pos);

    *position_flags = pos_flags;
}

void
tilde_from_menu(prompter, value)
struct AddressArea *prompter;
int value;
{
    Widget textsw;

    if (prompter->rawArea && ! XtIsManaged(prompter->rawArea)) {
	switch (value) {
	    case 1: default: AddressAreaGotoAddress(prompter, uicomp_To);
	    when 2: AddressAreaGotoSubject(prompter);
	    when 3: AddressAreaGotoAddress(prompter, uicomp_Cc);
	    when 4: AddressAreaGotoAddress(prompter, uicomp_Bcc);
	}
	return;
    }
    textsw = AddressAreaGetRaw(prompter);

    if (value == 0)
	prompter->pos_flags = POSITION_ALL;
    else
	prompter->pos_flags = ULBIT(value - 1) | POSITION_END;
    go_to_next_pos(textsw, &prompter->pos_flags);
}

/* This callback handles progressing through the header lines in a text
 * widget when Edit Headers is being used.  It corresponds to progress()
 * in addressArea/traverse.c.
 */
void
progress_callback(textsw, prompter, cbs)
Widget textsw;
struct AddressArea *prompter;
XmTextVerifyCallbackStruct *cbs;
{
#if XmREVISION >= 2 
    char header_text[8], c = 0;
#else /* !XmREVISION >= 2 */
    char *header_text, c = 0;
#endif /* !XmREVISION >= 2 */
    register XmTextPosition pos, pos1, pos2;

    /* Avoid triggering when SaveComposition() calls zmXmTextSetString(). */
    if (prompter->compose && prompter->compose->autosave_ct >= autosave_ct)
	return;

    if (prompter->pos_flags == 0L)
	return;

    if (cbs->reason == XmCR_MODIFYING_TEXT_VALUE && cbs->text->ptr)
	c = cbs->text->ptr[0];

    /* check for auto-next-header: when you hit CR on To: go to Subject: */
    if (cbs->reason == XmCR_MODIFYING_TEXT_VALUE) {
	if (c == '\n' || c == '\r' || c == '\t') {
	    if (prompter->pos_flags == POSITION_END &&
		    prompter->progressLast)
		SetTextInput(*(prompter->progressLast));
	    else
		go_to_next_pos(textsw, &prompter->pos_flags);
	    cbs->doit = False;
	}
	return;
    }
    /* Bart: Tue May 31 12:04:36 PDT 1994
     * At this point we must be moving the insertion cursor or something.
     * I have no idea how else we'd ever get here.
     */

    /* we're still processing this header -- continue to do so unless
     * the event in question changes the line# of the insertion point.
     * first get current position...
     */
    pos1 = cbs->currInsert, pos2 = cbs->newInsert;
    if (pos2 < pos1)
	pos = pos2, pos2 = pos1, pos1 = pos;

#if !defined(XmREVISION) || (XmREVISION < 2) 
    header_text = XmTextGetString(textsw);
#endif /* !XmREVISION || (XmREVISION < 2) */

    while (pos1 < pos2) {
#if defined(XmREVISION) && (XmREVISION >= 2) 
	if (cbs->reason == XmCR_MODIFYING_TEXT_VALUE && 
		(XmTextGetSubstring(textsw, pos1,
				1,
				sizeof(header_text),
				header_text)
		    != XmCOPY_SUCCEEDED) ||
		header_text[0] == '\n' ||
		header_text[0] == '\r' ) {
	    prompter->pos_flags = 0L;
	    break;
	}
	/* Bart: Wed May 25 22:57:40 PDT 1994
	 * Ben asserts that this doesn't work when the textsw contains
	 * multibyte characters.  I believe him, but how else can you
	 * test whether the character at a given position has a specific
	 * value?  There isn't any XmText op to get a substring in 1.1.
	 */
#else /* !(XmREVISION && (XmREVISION >= 2)) */
	if (header_text[pos1] == '\n' || header_text[pos1] == '\r') {
	    prompter->pos_flags = 0L;
	    break;
	}
#endif /* !(XmREVISION && (XmREVISION >= 2)) */
	else
	    pos1++;
    }

#if !defined(XmREVISION) || (XmREVISION < 2)
    XtFree(header_text);
#endif /* !XmREVISION || (XmREVISION < 2) */
}

/*
 * start the compose textsw.
 */
void
start_textsw_edit(textsw, position_flags)
Widget textsw;
u_long *position_flags;
{
    *position_flags = 0L;

    turnon(*position_flags, POSITION_TO);
    if (boolean_val(VarAsk) || boolean_val(VarAsksub))
	turnon(*position_flags, POSITION_SUBJ);
    if (boolean_val(VarAskcc))
	turnon(*position_flags, POSITION_CC);
    
    turnon(*position_flags, POSITION_END);
    go_to_next_pos(textsw, position_flags);
}
