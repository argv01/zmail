#include "osconfig.h"
#include "file.h"
#include "gui_def.h"
#include "catalog.h"
#include "general.h"
#include "zcstr.h"
#include "zcunix.h"
#include "zmdebug.h"
#include "zmstring.h"
#include "zmtty.h"
#include "zprint.h"

#include <Xm/Text.h>


static Boolean setFocus = 1;	/* UGH! */

//#if XmVersion <= 1002 || XmUPDATE_LEVEL <= 1 /* Motif 1.2.1 or earlier */
static Boolean setString = 0; 	/* UGH 2 */

/* Makes you wish C had dynamic scoping, eh?  -- brl */

void
zmXmTextSetString(text_w, string)
    Widget text_w;
    String string;
{
    setString = 1;
    XmTextSetString(text_w, string);
    setString = 0;
}

void
zmXmTextReplace(text_w, from, to, string)
    Widget text_w;
    XmTextPosition from, to;
    String string;
{
    setString = 1;
    XmTextReplace(text_w, from, to, string);
    setString = 0;
}
//#endif /* Motif 1.2.1 or earlier */


/* Prevent cut/pasting of newlines into single-line-edit-mode texts */
void
newln_cb(w, client_data, tvcbs)
Widget  w;
XtPointer client_data;
XmTextVerifyCallbackStruct *tvcbs;
{
    int i, replace_newline = (int)client_data;

#if XmVersion <= 1002 || XmUPDATE_LEVEL <= 1 /* Motif 1.2.1 or earlier */
    if (setString)
	return;
#endif /* Motif 1.2.1 or earlier */
    
    if ((tvcbs->event && (tvcbs->event->type == KeyPress ||
	    tvcbs->event->type == KeyRelease ||
	    tvcbs->event->type == DestroyNotify)) ||	/* Safety */
	    tvcbs->text->format != FMT8BIT)
	return;

    for (i = 0; i < tvcbs->text->length; i++)
	if (tvcbs->text->ptr[i] == '\n') {
	    if (replace_newline)
		tvcbs->text->ptr[i] = ' ';
	    else {
		/* tvcbs->text->ptr[i] = 0; */
		tvcbs->text->length = i;
	    }
	}
    /* if pasting lots of text, put input focus back here.  I know, this
     * doesn't work for pasting single chars, but since there's no way of
     * differentiating between single-char pastes and typed input, don't
     * degrade performance by calling XmProcessTraversal --all-- the time.
     * (also check the "event" field to be sure it's a user-input action
     * that's causing text to be inserted.)  But tvcbs->event is bogus ...
    if (tvcbs->event && tvcbs->text->length > 1)
     */
    if (setFocus && tvcbs->text->length > 1)
	XmProcessTraversal(w, XmTRAVERSE_CURRENT);
}

void
SetTextString(text_w, string)
Widget text_w;
const char *string;
{
    /*
    if (string) {
	XmTextFieldSetString(text_w, string);
	XmTextFieldSetCursorPosition(text_w, strlen(string));
    } else
	XmTextFieldSetString(text_w, "");
    */
    setFocus = 0;
    /* XXX casting away const */
    zmXmTextSetString(text_w, (char *) string);
    setFocus = 1;

    /* cannot use strlen(string), since string might be multibyte */
    XmTextSetCursorPosition(text_w, XmTextGetLastPosition(text_w));
}

char *
GetTextString(text_w)
Widget text_w;
{
    char *str, *s;

    s = str = XmTextGetString(text_w);
    if (!str) return NULL;
    while (*s && isspace(*s)) s++;
    if (!*s) {
	XtFree(str);
	return NULL;
    }
    return str;
}

void
TextStrCopy(dst_w, src_w)
Widget dst_w, src_w;
{
    char *text;

    text = XmTextGetString(src_w);
    setFocus = 0;
    zmXmTextSetString(dst_w, text);
    setFocus = 1;
    XtFree(text);
}

#include "glob.h"

static XmTextPosition filec_position = -1;

/*
 * Perform file completion on the text widget string
 */
void
filec_cb(w, client_data, tvcbs)
Widget	w;
XtPointer client_data;
XmTextVerifyCallbackStruct *tvcbs;
{
    char *string;	/* The string to be completed */
    char buf[MAXPATHLEN], *b = buf, **exp;
    int n, f, len, prefix, trim, overstrike;

#if XmVersion <= 1002 || XmUPDATE_LEVEL <= 1 /* Motif 1.2.1 or earlier */
    if (setString)
	return;
#endif /* Motif 1.2.1 or earlier */
    
    if (tvcbs->text->format != FMT8BIT)
	return;

    if (tvcbs->text->length != 1 || !tvcbs->event) {
	filec_position = -1;	/* Just in case */
	return; /* need to handle this recursively, skip it for now */
    }

    /* Prevent recursion */
    if (filec_position >= 0) {
	if (complete && tvcbs->text->ptr[0] == complete) {
	    filec_position = -1;
	    tvcbs->doit = False;
	}
	return;
    }

    if (debug > 10)
	Debug("input text was: %s\nlength: %d startPos: %ld endPos: %ld\n" ,
	    ctrl_strcpy(buf,
		zmVaStr("%*.*s",		/* text->ptr is not  */
			tvcbs->text->length,	/* \0-terminated, so */
			tvcbs->text->length,	/* do some tricks to */
			tvcbs->text->ptr),	/* generate a string */
		0),
	    tvcbs->text->length,
	    tvcbs->startPos,
	    tvcbs->endPos);

    if (!complete || tvcbs->text->ptr[0] != complete)
	return;

    string = XmTextGetString(w);

    if (!*string || !tvcbs->startPos) {
	XtFree(string);
	tvcbs->doit = False;
	errbell(-1);
	return;
    }

    /* Look right for a delimiter */
    n = tvcbs->endPos;
    while (string[n] && !index(DELIM, string[n]))
	n++;
    tvcbs->endPos = n;

    /* Look left for a delimiter */
    n = tvcbs->startPos;
    while (n > 0 && !index(DELIM, string[--n]))
	;
    if (n > 0 || index(DELIM, string[n]))
	n++;

    /* Note:  To complete the word containing the cursor rather than
     * the word to the left of the cursor, change the line below to:
     * b = buf + (len = tvcbs->endPos - n);
     */
    b = buf + (len = tvcbs->startPos - n);
    (void) strncpy(buf, &string[n], len);
    *b = 0;
    Debug("expanding (%s) ... " , buf);
    if (!any(buf, FMETA)) {
	overstrike = (*buf == '+' || *buf == '~' || *buf == '%');
	trim = (overstrike && len > 1);
	if (!overstrike || len > 1)
	    *b++ = '*', *b = 0;
	/* Previous behavior for '+' completions (trailing '/'):
	if (len > 1 || *buf != '~' || *buf != '%')
	    *b++ = '*', *b = 0;
	*/
	f = filexp(buf, &exp);
	if (*--b == '*')
	    *b = 0; /* We need the original buf below */
    } else {
	overstrike = 1;
	trim = (*buf == '+' || *buf == '~');
	/*
	 * Check first to see if the base pattern matches.
	 * If not, append a '*' and try again.
	 * Don't expand all matches in the latter case.
	 */
	if ((f = filexp(buf, &exp)) < 1) {
	    *b++ = '*', *b = 0;
	    f = filexp(buf, &exp);
	    *--b = 0; /* We need the original buf below */
	}
    }
    f = fignore(f, &exp);
    if (f < 0) {
	Debug("globbing error!\n%s" , string);
	free_vec(exp);
	XtFree(string);
	tvcbs->doit = False;
	errbell(-1);
	return;
    } else if (f > 0) {
	Debug("result is: " ), print_argv(exp);
	if (f > 1)
	    prefix = lcprefix(exp, overstrike ? 0 : len);
	else
	    prefix = 0;
	if (strlen(exp[0]) > len) {
	    if (overstrike && (prefix || f == 1)) {
		char *tmpv[3];
		tmpv[0] = buf;
		if (trim)
		    tmpv[1] = trim_filename(exp[0]);
		else
		    tmpv[1] = exp[0];
		tmpv[2] = NULL;
		/* Back up as far as is necessary */
		len = lcprefix(tmpv, 0);
		/* If nothing will be erased, we may need to beep */
		if (n + len == tvcbs->startPos) {
		    if (!tmpv[1][len])
			errbell(0);
		}
		/* Erase the stuff that will complete */
		tvcbs->startPos = n + len;
	    }
	    if (f == 1) {
		while (f--) {
		    if (trim)
			b = trim_filename(exp[f]);
		    else
			b = exp[f];
#ifndef SLOW
		    tvcbs->text->length = strlen(b + len);
		    tvcbs->text->ptr =
			XtRealloc(tvcbs->text->ptr, tvcbs->text->length + 1);
		    strcpy(tvcbs->text->ptr, b + len);
#else /* SLOW */
		    ZSTRDUP(string, b + len);
#endif /* SLOW */
		}
	    } else {
		if (prefix > len) {
		    exp[0][prefix] = 0;
		    Debug("\ncompletion is (%s)\n" , exp[0]);
#ifndef SLOW
		    if (trim) {
			tvcbs->text->length =
			    strlen(trim_filename(exp[0]) + len);
			tvcbs->text->ptr =
			    XtRealloc(tvcbs->text->ptr, tvcbs->text->length);
			strcpy(tvcbs->text->ptr, trim_filename(NULL) + len);
		    } else {
			tvcbs->text->length = strlen(&exp[0][len]);
			tvcbs->text->ptr =
			    XtRealloc(tvcbs->text->ptr, tvcbs->text->length);
			strcpy(tvcbs->text->ptr, &exp[0][len]);
		    }
#else /* SLOW */
		    if (trim)
			ZSTRDUP(string, trim_filename(exp[0]) + len);
		    else
			ZSTRDUP(string, &exp[0][len]);
#endif /* SLOW */
		} else {
		    Debug("no longer prefix\n" );
		    tvcbs->doit = False;
		}
		/* Special case because "+" always tries to expand "+*"
		 * to get listings and avoid getpath()'s trailing '/'.
		 * No error bell is needed in those cases.
		 */
		if (strcmp(buf, "+") != 0)
		    errbell(0);
	    }
	} else {
	    Debug("no longer prefix\n" );
	    tvcbs->doit = False;
	    errbell(0);
	}
    } else {
	Debug("no match\n" );
	tvcbs->doit = False;
	errbell(0);
    }

    if (tvcbs->doit) {
#ifndef SLOW
	filec_position = tvcbs->startPos + tvcbs->text->length;
#else /* SLOW */
	filec_position = tvcbs->startPos + strlen(string);
	zmXmTextReplace(w, tvcbs->startPos, tvcbs->endPos, string);
	XmTextSetCursorPosition(w, filec_position);
	/* The above actually may not have caused a motion, so: */
	filec_position = -1;	/* permit other motions and filec */
	tvcbs->doit = False;
#endif /* SLOW */
    }

    if (debug > 10)
	Debug("output text is: %s\nlength: %d startPos: %ld endPos: %ld\n" ,
#ifndef SLOW
	    ctrl_strcpy(buf,
		zmVaStr("%*.*s",		/* text->ptr is not  */
			tvcbs->text->length,	/* \0-terminated, so */
			tvcbs->text->length,	/* do some tricks to */
			tvcbs->text->ptr),	/* generate a string */
		0),
	    tvcbs->text->length,
#else /* SLOW */
	    ctrl_strcpy(buf, string, 0),
	    strlen(string),
#endif /* SLOW */
	    tvcbs->startPos,
	    tvcbs->endPos);

    free_vec(exp);
    XtFree(string);
    return;
}

void
filec_motion(w, client_data, tvcbs)
Widget	w;
XtPointer client_data;
XmTextVerifyCallbackStruct *tvcbs;
{
#if XmVersion <= 1002 || XmUPDATE_LEVEL <= 1 /* Motif 1.2.1 or earlier */
    if (setString)
	return;
#endif /* Motif 1.2.1 or earlier */
    
    if (debug > 10)
	Debug("filec_pos = %ld\n" , filec_position);
    if (filec_position >= 0) {
	tvcbs->doit = (filec_position == tvcbs->newInsert);
	if (!tvcbs->doit)
	    XmTextSetCursorPosition(w, filec_position);
	filec_position = -1;
    }
}
