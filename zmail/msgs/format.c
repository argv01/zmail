/* format.c	Copyright 1993 Z-Code Software Corp. */

#include "zmail.h"
#include "catalog.h"
#include "format.h"
#include "glob.h"

#include <ctype.h>

#ifndef lint
static char	format_rcsid[] =
    "$Id: format.c,v 2.19 1995/09/07 06:10:50 liblit Exp $";
#endif

/* format_unindent_lines(lines_vec, prefix_length)
 *
 * given a vector of lines (without trailing '\n's), this routine skips
 * the first prefix_length characters in each line.  The return value
 * is a string containing all the lines, separated by newlines.
 * lines_vec gets freed in this routine, and the return value must be freed
 * by the caller.
 *
 * For example, to unindent the lines in str:
 *
 * char **vec = strvec(str, "\n", 0);
 * str = format_unindent_lines(vec, format_prefix_length(vec, True));
 *
 * Notice that the trailing '\n' (if any) gets lost.
 */
char *
format_unindent_lines(lines, plen)
char **lines;
int plen;
{
    int i;
    char *ret;

    for (i = 0; lines[i]; i++)
	lines[i] += plen;
    ret = joinv(NULL, lines, "\n");
    for (i = 0; lines[i]; i++)
	lines[i] -= plen;
    free_vec(lines);
    return ret;
}

/* format_prefix_length(lines_vec, all_spaces_ok)
 *
 * given a vector of lines, this routine return the prefix length of all
 * the lines.  This value can be used in a call to format_unindent_lines.
 * If all_spaces_ok is nonzero, then a prefix of all whitespace is valid;
 * otherwise it isn't.  (For example, if lines_vec = ("  foo", "  bar", 0),
 * format_prefix_length(lines_vec, True) == 2, but ...(lines_vec, False) == 0.
 */
int
format_prefix_length(lines, all_sp_ok)
char **lines;
int  all_sp_ok;
{
    int i, n, nonspace = 0;

    n = lcprefix(lines, 0);
    if (!n) return 0;
    for (i = 0; i < n; i++) {
	if (!isspace(lines[0][i]))
	    nonspace = 1;
	if (ispunct(lines[0][i]) && !index("-,;()", lines[0][i]))
	    break;
    }
    if (!nonspace && all_sp_ok) return n;
    return (i == n) ? 0 : n;
}

/* format_indent_lines(lines_vec, prefix)
 *
 * indents the lines in lines_vec with the given prefix, returning a
 * dynamically allocated string containing all the lines, separated by
 * newlines.  lines_vec gets freed.
 */
char *
format_indent_lines(lines, pfix)
char **lines;
char *pfix;
{
    int i;
    char *ret;
    
    ret = savestr("");
    for (i = 0; lines[i]; i++) {
	strapp(&ret, pfix);
	strapp(&ret, lines[i]);
	if (lines[i+1]) strapp(&ret, "\n");
    }
    free_vec(lines);
    return ret;
}

#define TABSTOP 8

/* format_fill_lines(lines_vec, wrapcolumn)
 *
 * fills (formats) the lines in lines_vec, wrapping lines at the given
 * wrapcolumn.  Returns a dynamically allocated string containing all
 * the lines, separated by newlines.  lines_vec gets freed.
 */
char *
format_fill_lines(lines, wrap)
char **lines;
int wrap;
{
    int plen, length;
    char *str, *out;
    char *pfix = NULL;

    plen = format_prefix_length(lines, FALSE);
    if (plen) {
	pfix = savestr(lines[0]);
	pfix[plen] = 0;
	wrap -= plen;
    }
    str = format_unindent_lines(lines, plen);
    length = strlen(str);
    out = (char *) calloc(2*length+1, 1);
    fmt_string(str, out, length, wrap, TABSTOP, TRUE);
    xfree(str);
    if (!plen) return out;
    lines = strvec(out, "\n", 0);
    xfree(out);
    str = (lines) ? format_indent_lines(lines, pfix) : savestr("");
    xfree(pfix);
    return str;
}


#define PIPE_TMP "zm.pipe"

/* format_pipe_str(text_string, command_string)
 *
 * pipes the text in a string through a UNIX command, returning the output.
 * If there is no output, reports an error and returns NULL.  Otherwise,
 * returns a dynamically allocated string containing all the output.
 */
char *
format_pipe_str(in, cmd)
char *in, *cmd;
{
    char *file = NULL, *out, *p;
    char **vec = DUBL_NULL;
    char buf[MAXPATHLEN];
    FILE *pipe_fp, *fp;
    int n;

    /* stick the text in a tempfile */
    if (!(fp = open_tempfile(PIPE_TMP, &file))) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 152, "Cannot open temporary file" ));
	return 0;
    }
    fputs(in, fp);
    (void) fclose(fp);

    (void) sprintf(buf, "( %s ) < %s", cmd, file);
    if ((pipe_fp = popen(buf, "r")) == NULL_FILE) {
	error(SysErrWarning, catgets( catalog, CAT_MSGS, 307, "Cannot run \"%s\"" ), cmd);
	xfree(file);
	return 0;
    }
    
    n = 0;
    do {	/* Reading from a pipe, don't fail on SIGCHLD */
	errno = 0;
	for (; p = fgets(buf, sizeof buf, pipe_fp); n++) {
	    int l = strlen(p)-1;
	    if (p[l] == '\n') p[l] = 0;
	    /* Catv handles n == 0 case just fine */
	    if (catv(n, &vec, 1, unitv(buf)) != n + 1) {
		error(ZmErrWarning, catgets( catalog, CAT_MSGS, 537, "Cannot allocate list of words" ));
		break;
	    }
	}
    } while (errno == EINTR && !feof(pipe_fp));
    if (!n) {
	out = NULL;
	error(ZmErrWarning, catgets( catalog, CAT_MSGS, 538, "No output from command" ));
    } else {
	out = joinv(NULL, vec, "\n");
	free_vec(vec);
    }
    (void) pclose(pipe_fp);
    (void) unlink(file);
    xfree(file);
    return out;
}
