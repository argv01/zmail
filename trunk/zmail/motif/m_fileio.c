/* m_fileio.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "zmstring.h"
#include "zmcomp.h"
#include "m_comp.h"
#include "addressArea/addressArea.h"

#include <Xm/Text.h>

/* Revamped guts of OpenFile().  See below for details. */
Boolean
LoadFile(text_w, filename, insert, start, stop)
Widget text_w;
char *filename;
int insert;
long start, stop;	/* offsets in the file */
{
    struct stat statbuf;/* Information on a file. */
    char *file_string;	/* Contents of file. 	  */
    char *oldfile = NULL; /* old filename text object was editing */
    FILE *fp;	/* Pointer to open file   */

    ask_item = text_w;

    if (!filename || !*filename) {
	error(UserErrWarning, catgets( catalog, CAT_MSGS, 24, "No file specified." ));
	return False;
    }
    if (stat(filename, &statbuf) == -1) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 74, "Cannot stat %s" ), filename);
	return False;
    }
    if (S_ISDIR(statbuf.st_mode)) {
	error(UserErrWarning, catgets(catalog, CAT_MSGS, 427, "%s is a directory"), filename);
	return False;
    }

    if (start < 0)
	start = 0;
    if (stop < 0 || stop > statbuf.st_size)
	stop = statbuf.st_size;
    if (start >= stop) {
	if (insert < 1)
	    zmXmTextSetString(text_w, "");
	return True;
    }

    if (!(fp = fopen(filename, "r"))) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 398, "Cannot open \"%s\"" ), filename);
	return False;
    }

    /* read the file string */
    if (!(file_string = (char *) XtMalloc((unsigned)(stop + 1)))) {
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 76, "Cannot alloc enough space for file" ));
	fclose(fp);
	return False;
    }
    fread(file_string, sizeof(char), stop, fp);
    file_string[stop] = 0;
    fclose(fp);

    /* added the file string to the text widget */
    if (insert > 0) {
       XmTextPosition position;
       XtVaGetValues(text_w, XmNcursorPosition, &position, NULL);
       zmXmTextReplace(text_w, position, position, file_string + start);
    } else {
	/* Workaround for Motif text widget bug */
	if (file_string[start] == '\n')
	    zmXmTextSetString(text_w, NULL);
	zmXmTextSetString(text_w, file_string + start);
	if (insert != -1) {
	    XtVaGetValues(text_w, XmNuserData, &oldfile, NULL);
	    ZSTRDUP(oldfile, filename);
	    XtVaSetValues(text_w, XmNuserData, oldfile, NULL);
	}
    }
    XtFree(file_string);

    /* XtVaSetValues(text_w, XmNeditable, True, NULL); */
    XtSetSensitive(text_w, True);
    ask_item = tool;
    return True;
}

/*-------------------------------------------------------------
**	OpenFile
**		Open the present file.  Returns true if file
**  exists and open is sucessful.
**
**  If insert == -1, replace text, but don't change userData
** This has now been broken out into a second function
*/
Boolean
OpenFile(text_w, filename, insert)
Widget text_w;
char *filename;
int insert;
{
    return LoadFile(text_w, filename, insert, 0, -1);
}

#define TABSTOP 8	/* Jef -- get this from the OLIT text widget? */

/* SaveFile() --save the contents of the text_w passed in the filename
 * passed.  If filename is NULL, get the one in the text_w's XmNuserData.
 *
 * Bart: Thu May 26 01:02:07 PDT 1994
 * This used to do a bunch of stuff we weren't using (see #if 0 block)
 * so I chopped that and replaced it with the ability to append to, instead
 * of overwriting, the file.  Cleverly avoid changing the prototype.
 */
Boolean
SaveFile(text_w, filename, how, do_format)
Widget text_w;
char *filename; /* if NULL, write to file is text's XmNuserData */
char *how;	/* "w" (or NULL) to overwrite, "a" to append */
int do_format;
{
    char *file_string = NULL;	/* Contents of file. */
    FILE *tfp;		/* Pointer to open temporary file. */
    long len, wlen = 0;
    Widget old_ask = ask_item;
    int add_newline;

    ask_item = text_w;

    if (!filename || !*filename) {
	XtVaGetValues(text_w, XmNuserData, &filename, NULL);
	if (!filename || !*filename) {
	    error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 77, "No current file name." ));
	    return False;
	}
    }

#if 0
    /* Obsolete stuff -- nobody was using it at all */
    if (query) {
	AskAnswer answer;
	answer = gui_ask(AskYes,
	    *query? query : catgets( catalog, CAT_MOTIF, 78, "Text has been modified. Update Changes?" ));
	if (answer == AskCancel)
	    return False;
	if (answer == AskNo)
	    return True;
    }
#endif /* 0 */

    if (!how || !*how)
	how = "w";

    if ((tfp = fopen(filename, how)) == NULL_FILE) {
	error(SysErrWarning,
		catgets( catalog, CAT_MOTIF, 79, "Warning: unable to open file, text not saved" ));
	return False;
    }
    /* get the text string */
    file_string = XmTextGetString(text_w);
    /* Do not use XmTextGetLastPosition() in place of strlen().
     * XmTextGetLastPosition() will tell you the length in characters,
     * but we need the size in bytes.  If multibyte characters are
     * floating around, length and size are two different things.
     */
    len = strlen(file_string);
    add_newline = (len && file_string[len-1] != '\n');

    /* write to a temp file */
    if (len) {
	if (do_format) {
	    /* Bart: Sun Jun 14 18:08:40 PDT 1992		XXX
	     * This function is not generic -- it has to know whether
	     * to skip the message headers before formatting the body.
	     */
#if 0
	    Compose *compose = FrameComposeGetComp(FrameGetData(text_w));
	    char *p;
#endif /* !0 */
	    char *out;
	    short wrap = 0;

	    wrap = get_wrap_column(text_w);

#if 0
	    /* Skip the message headers if present */
	    p = file_string;
	    if (wrap > 0 &&	/* How could this possibly fail? */
		    ison(compose->flags, EDIT_HDRS))
		while (p = index(p, '\n'))
		    if (p[1] == '\n' || !p[1])
			break;
		    else
			++p;
	    if (wrap > 0 && p != NULL) {
		wlen = strlen(++p);
		/* Heuristic to get a "big enough" output  buffer */
		out = (char *) calloc(wlen + 2 * ((wlen/wrap) + 1), sizeof(char));
		fmt_string(p, out, wlen, wrap, TABSTOP, FALSE);
		len = (p - file_string) + (wlen = strlen(out)) -
		fwrite(file_string, sizeof(char), (int)(p - file_string), tfp);
		wlen = fwrite(out, sizeof(char), (int)wlen, tfp);
		xfree(out);
	    } else
#else /* !0 */
	    if (wrap > 0) {
		/* Heuristic to get a "big enough" output  buffer */
		out = (char *)
		    calloc(len + 2 * ((len/wrap) + 1), sizeof(char));
		fmt_string(file_string, out, len, wrap, TABSTOP, FALSE);
		wlen = fwrite(out, sizeof(char), len = strlen(out), tfp);
		xfree(out);
	    } else
#endif /* !0 */
		wlen = fwrite(file_string, sizeof(char), (int)len, tfp);
	} else
	    wlen = fwrite(file_string, sizeof(char), (int)len, tfp);
    }
    if (wlen != (int)len)
	error(SysErrWarning, catgets( catalog, CAT_MOTIF, 80, "Did not completely rewrite file" ));
    else {
	if (len && add_newline)
	    fputc('\n', tfp);
    }

    /* flush and close the temp file */
    fflush(tfp);
    fclose(tfp);

    if (file_string)
	XtFree(file_string);	/* free the text string */

    ask_item = old_ask;
    return True;
}

/* CloseFile -- set the text string to "". and desensitize the Text widget */
void
CloseFile(text_w)
Widget text_w;
{
    char *filename;
    /* zero out the text string in the text widget. caution: causes a value
     * changed callack.
     */
    /* zmXmTextSetString(text_w, NULL); */

    XtVaGetValues(text_w, XmNuserData, &filename, NULL);
    /* free the file name */
    if (filename != NULL)
	xfree(filename);
    XtVaSetValues(text_w, XmNuserData, NULL, NULL);
    /* set text to insensitive */
    /* XtVaSetValues(text_w, XmNeditable, False, NULL); */
    XtSetSensitive(text_w, False);
}

/* Z-Mail Compose-window specific operations */

Boolean
SaveComposition(compose, autoformat)
Compose *compose;
int autoformat;
{
    char *how = "w";

    if (ison(compose->flags, EDIT_HDRS)) {
	Widget text_w = AddressAreaGetRaw(compose->interface->prompter);
#if XmVersion >= 1002
	XmTextPosition trim, last = XmTextGetLastPosition(text_w);

	if (XmTextFindString(text_w, 0, "\n\n", XmTEXT_FORWARD, &trim)) {
	    zmXmTextReplace(text_w, trim+2, last, "");
	} else {
	    char tail[2];

	    if (XmTextGetSubstring(text_w, last - 1, 1, sizeof(tail), tail)
			== XmCOPY_SUCCEEDED &&
		    tail[0] == '\n')
		trim = last - 1;
	    else
		trim = last;
	    
	    zmXmTextReplace(text_w, trim, last, "X\n\n");
	    zmXmTextReplace(text_w, trim, trim+1, "");
	}
#else /* Motif 1.1 */
	char *wasted_effort = XmTextGetString(text_w), *gap;

	/* The waste of effort is that there are no substring operations
	 * on text widgets, so we have to pull the ENTIRE text and diddle
	 * with it to assure that the string ends with a blank line.  At
	 * that point, it's safe to write it to the file.
	 */

	gap = strstr(wasted_effort, "\n\n");
	if (gap) {
	    gap[2] = 0;
	    zmXmTextSetString(text_w, wasted_effort);
	} else {
	    XmTextPosition pos1 = XmTextGetLastPosition(text_w);
	    XmTextPosition pos2 = pos1;

	    if (wasted_effort[strlen(wasted_effort) - 1]
		    == '\n') {
		pos1--;
	    }
	    /* Workaround for REALLY STUPID Motif bug.  It treats as
	     * many newlines as you care to throw at it as a single
	     * newline unless you also throw in at least one non-
	     * newline character.  AAIIIEEEE!
	     */
	    zmXmTextReplace(text_w, pos1, pos2, "X\n\n");
	    zmXmTextReplace(text_w, pos1, pos1+1, "");
	}
	XtFree(wasted_effort);
#endif /* Motif 1.1 */

	if (SaveFile(text_w, compose->edfile, how, False) == 0)
	    return 0;
	how = "a";
    }
    if (SaveFile(compose->interface->comp_items[COMP_TEXT_W],
		 compose->edfile, how, autoformat) == 0)
	return 0;

    return 1;
}

Boolean
LoadComposition(compose)
Compose *compose;
{
    if (ison(compose->flags, EDIT_HDRS)) {
	AddressAreaWalkRaw(compose->interface->prompter, 0);
	if (LoadFile(AddressAreaGetRaw(compose->interface->prompter),
		     compose->edfile, False, 0, compose->body_pos) == 0)
	    return 0;
    }
    if (LoadFile(compose->interface->comp_items[COMP_TEXT_W],
		 compose->edfile, False, compose->body_pos, -1) == 0)
	return 0;

    return 1;
}
