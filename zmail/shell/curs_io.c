/* curs_io.c     Copyright 1990, 1991 Z-Code Software Corp. */

/* curs_io.c -- curses based I/O */
#include "zmail.h"
#include "zcsyssel.h" /* for sys/select.h where appropriate */
#include "curs_io.h"
#include "c_bind.h"
#include "glob.h"
#include "zmtty.h"
#include <dynstr.h>
#include <ctype.h>

#include "catalog.h"

#if !defined(M_XENIX) || (defined(M_XENIX) && !defined(CURSES))
char *_unctrl[] = {
    "^@", "^A", "^B", "^C", "^D", "^E", "^F", "^G", "^H", "^I", "^J", "^K",
    "^L", "^M", "^N", "^O", "^P", "^Q", "^R", "^S", "^T", "^U", "^V", "^W",
    "^X", "^Y", "^Z", "^[", "^\\", "^]", "^~", "^_",
    " ", "!", "\"", "#", "$",  "%", "&", "'", "(", ")", "*", "+", ",", "-",
    ".", "/", "0",  "1", "2",  "3", "4", "5", "6", "7", "8", "9", ":", ";",
    "<", "=", ">",  "?", "@",  "A", "B", "C", "D", "E", "F", "G", "H", "I",
    "J", "K", "L",  "M", "N",  "O", "P", "Q", "R", "S", "T", "U", "V", "W",
    "X", "Y", "Z",  "[", "\\", "]", "^", "_", "`", "a", "b", "c", "d", "e",
    "f", "g", "h",  "i", "j",  "k", "l", "m", "n", "o", "p", "q", "r", "s",
    "t", "u", "v",  "w", "x",  "y", "z", "{", "|", "}", "~", "^?"
};
#endif /* !(!M_XENIX || (M_XENIX && !CURSES)) */

#ifndef TERM_USE_NONE
ZTTY_t	Ztty;
#endif /* !TERM_USE_NONE */

char	del_line;	/* tty delete line character */
char	del_word;	/* tty delete word character */
char	del_char;	/* backspace */
char	reprint_line;	/* usually ^R */
char	eofc;		/* usually ^D */
char	lit_next;	/* usually ^V */
char	complete;	/* word completion, usually ESC */
char	complist;	/* completion listing, usually ^D */

void
tty_settings()
{
    int is_tty = isatty(0);

#if defined(MSDOS) || defined(MAC_OS)
    eofc = Ctrl('Z');
    del_line = Ctrl('U');
    del_char = Ctrl('H');
#else
#ifdef TERM_USE_TIO

#if 0				/* this now happens in savetty() */
    eofc = Ztty.c_cc[VEOF];
#endif

#else
#ifdef TERM_USE_SGTTYB
    if (ioctl(0, TIOCGETC, &tchars) != -1)
	eofc = tchars.t_eofc;
    else
#endif /* TERM_USE_SGTTYB */
	eofc = Ctrl('D');
#endif /* TERM_USE_TIO */

    if (!is_tty) {
	del_line = Ctrl('U');
	del_char = Ctrl('H');
    } else {
	del_line = Ztty.sg_kill;
	del_char = Ztty.sg_erase;
    }

#if defined(TERMIO_USE_SGTTYB) && defined(HAVE_LTCHARS)
    if (ioctl(0, TIOCGLTC, &ltchars) != -1) {
	del_word = ltchars.t_werasc;
	reprint_line = ltchars.t_rprntc;
	lit_next = ltchars.t_lnextc;
    } else
#endif /* TERMIO_USE_SGTTYB && HAVE_LTCHARS */
#endif /* MSDOS || MAC_OS */
    {
	del_word = Ctrl('W');
	reprint_line = Ctrl('R');
	lit_next = Ctrl('V');
    }
}

#ifdef GUI_ONLY

int
dyn_Getstr(dstr, pmpt)
struct dynstr *dstr;
char *pmpt;
{
    dynstr_Append(dstr, "");
    return dynstr_Length(dstr);
}

#else /* !GUI_ONLY */

#ifdef Addch
#undef Addch
#endif /* Addch */

#ifndef CURSES

/* Make sure all ifs have matching elses! */

#define Addch(c) \
    if (ison(glob_flags, ECHO_FLAG)) \
	{;} \
    else \
	(void) fputc(c, stdout), (void) fflush(stdout)

#else

/* see end of Getstr */
#define Addch(c)  \
    if (iscurses) \
	addch(c), refresh(); \
    else if (ison(glob_flags, ECHO_FLAG)) \
	{;} \
    else \
	(void) fputc(c, stdout), (void) fflush(stdout)
#endif /* CURSES */

static char *lastprompt = 0;

static char
backspace(dstr)
struct dynstr *dstr;
{
    char chopped;

    Addch('\b'); Addch(' '); Addch('\b');
    chopped = dynstr_Chop(dstr);
    if (iscntrl(chopped)) {
	Addch('\b'); Addch(' '); Addch('\b');
	/* Re-increment wrapcolumn -- see Getstr */
	if (wrapcolumn)
	    wrapcolumn++;
    }

    return chopped;
}

int
dyn_Getstr(dstr, pmpt)
struct dynstr *dstr;
char *pmpt;
{
    int c, literal_next = FALSE, lit_bs = FALSE;
    struct cmd_map *curr_map;
    int save_wc = wrapcolumn;
    int sigs_ok = ison(glob_flags, SIGNALS_OK);

    if (ison(glob_flags, NO_INTERACT))
	return (0);

    if (pmpt) {
	ZSTRDUP(lastprompt, pmpt);
	pmpt = lastprompt;
	if (*pmpt)
	    print("%s", pmpt);
    }

    /* make sure everything is flushed before getting input */
    (void) fflush(stdout);

    if (mac_hide) {
	curr_map = NULL_MAP;
	wrapcolumn = 0;
    } else if (ison(glob_flags, IS_GETTING))
	curr_map = bang_map;
    else if (iscurses)
	curr_map = NULL_MAP;
    else
	curr_map = line_map;

    while ((c = m_getchar()) != '\n' && c != '\r' &&
	    c != EOF && !check_intr()) {
	turnoff(glob_flags, SIGNALS_OK);
	/* echo isn't set, so whatever the character, enter it */
	if (ison(glob_flags, QUOTE_MACRO) || ison(glob_flags, ECHO_FLAG)) {
	    dynstr_AppendChar(dstr, c);
	    /* Note: Addch includes ECHO_FLAG test */
	    if (iscntrl(c)) {
		Addch('^');
		Addch(_unctrl[c][1]);
	    } else
		Addch(c);
	}
	/* ^D as the first char on a line or two ^D's in a row is EOF */
	else if (c == eofc && dynstr_EmptyP(dstr))
	    break;
	else if (c == '\\') {
	    literal_next = TRUE, lit_bs = FALSE;
	    dynstr_AppendChar(dstr, '\\');
	    Addch('\\');
    	} else if (c == lit_next) {
	    literal_next = lit_bs = TRUE;
	    dynstr_AppendChar(dstr, '\\');
	    if (!in_macro()) {
		/* if (iscntrl(c)) */
		    Addch('^');
		/* Addch(_unctrl[c][1]); */
	    }
	} else if (literal_next) {
	    struct cmd_map *list;

	    literal_next = FALSE;
	    if (iscntrl(c) || c == del_line || c == del_char || c == del_word
		    || c == lit_next || lit_bs) {
		if (!in_macro() || !lit_bs) {
		    backspace(dstr);
		} else {
		    dynstr_Chop(dstr);
		}
	    } else if (in_macro() && c == MAC_LONG_CMD) {
		dynstr_Chop(dstr);
	    } else {
		/* check to see if user is escaping a map or map! */
		for (list = curr_map; list; list = list->m_next)
		    if (list->m_str[0] == c) {
			if (!in_macro()) {
			    backspace(dstr);
			} else {
			    dynstr_Chop(dstr);
			}
			break;
		    }
	    }
	    /* A literal-next advances the macro offset */
	    dynstr_AppendChar(dstr, c);
	    if (iscntrl(c) || c == del_char) {
		if (iscntrl(c)) {
		    /*
		     * Decrement wrapcolumn because two chars added.
		     * It will be restored from save_wc before return.
		     */
		    if (wrapcolumn > 1)
			wrapcolumn--;
		    Addch('^');
		}
		Addch(_unctrl[c][1]);
	    } else
		Addch(c);
	} else if (complete && (c == complete || c == complist)) {
	    (void) completion(dstr, (c == complist), (c == complete));
	} else if (c == del_line) {
	    if (!dynstr_EmptyP(dstr)) {
		do {
		    backspace(dstr);
		} while (!dynstr_EmptyP(dstr));
	    }
	} else if (c == reprint_line) {
	    wprint("\n%s", dynstr_Str(dstr));
	} else if (c == del_word) { /* word erase */
	    char chopped, last;

	    while (!dynstr_EmptyP(dstr)) {
		chopped = backspace(dstr);
		if (!dynstr_EmptyP(dstr))
		    last = dynstr_Str(dstr)[dynstr_Length(dstr) - 1];
		if (dynstr_EmptyP(dstr)
		    || isspace(last) && !isspace(chopped)
		    || !isalnum(last) && isalnum(chopped))
		    break;
	    }
	} else if (c == del_char ||
		    c == Ctrl('H') ||
		    c == 127 /* Ctrl('?') */) {
	    if (!dynstr_EmptyP(dstr)) {
		backspace(dstr);
	    }
	    /* if iscurses, then backspacing too far cancels a function */
	    else if (dynstr_EmptyP(dstr)
		     && iscurses
		     && isoff(glob_flags, LINE_MACRO)) {
		mac_flush();
		dynstr_Set(dstr, "");
		wrapcolumn = save_wc;
		return -1;
	    }
	} else if (c == '\t')
	    do  {
		/* Yuck -- tabs break map! */
		Addch(' ');
		dynstr_AppendChar(dstr, ' ');
	    } while (dynstr_Length(dstr) % 8);
	else if (in_macro() && c == MAC_LONG_CMD) {
	    char cbuf[MAX_LONG_CMD + 1];

	    if ((c = read_long_cmd(cbuf)) == 0) {
		c = MAC_LONG_CMD;
		goto check_expand;	/* How could I avoid this? */
	    } else if (c > 0) {
		int ok;

		if ((ok = reserved_cmd(cbuf, TRUE)) > 0) {
		    /* Reprint the line */
		    if (iscurses)
			print("%s%s", pmpt, dynstr_Str(dstr));
		    else
			wprint("\r%s", dynstr_Str(dstr));
		    if (sigs_ok)
			turnon(glob_flags, SIGNALS_OK);
		    continue;	/* Get next char without changing count */
		} else if (ok < 0) {
		    dynstr_Set(dstr, "");
		    wrapcolumn = save_wc;
		    return (ok);
		} else
		    goto push_back;
	    } else {
		/*
		 * Ooops.  We read a bunch of stuff we should not
		 * have read, because this isn't really a long command.
		 * Use a trick to push the whole thing back, ala ungetc.
		 * Wouldn't it be nifty if stdio worked this way? :-)
		 */
push_back:
		if (c > 0) {
		    cbuf[c++] = MAC_LONG_END;
		    cbuf[c] = '\0';
		}
		c = MAC_LONG_CMD;
		Ungetstr(cbuf);
		goto check_expand;	/* How could I avoid this goto? */
	    }
	} else {
check_expand:
	    if (!curr_map || !check_map(c, curr_map)) {
	    /* else if (match != MATCH) */
		if (c != '\t' && iscntrl(c)) {
		    Addch('^');
		    Addch(_unctrl[c][1]);
		    /* Decrement wrapcolumn as above */
		    if (wrapcolumn > 1)
			wrapcolumn--;
		} else
		    Addch(c);
		dynstr_AppendChar(dstr, c);
	    }
	}
	/* Null-terminate for macro lookup purposes.
	 * This will be overwritten by the next character.
	 */
	if (line_wrap(dstr))
	    break;
	if (sigs_ok)
	    turnon(glob_flags, SIGNALS_OK);
    }
    (void) fflush(stdout); /* for sys-v folks */

    if (c == eofc || c == EOF || check_intr()) {
	if (feof(stdin))
	    clearerr(stdin);
	wrapcolumn = save_wc;
	return -1;
    }
    if (!dynstr_EmptyP(dstr)
	&& dynstr_Str(dstr)[dynstr_Length(dstr) - 1] == '\\'
	&& isoff(glob_flags, IS_GETTING)) {
	int count2;

	if (isoff(glob_flags, ECHO_FLAG))
	    putchar('\n');
	wrapcolumn = save_wc;
	dynstr_Chop(dstr);	/* Whack the backslash when "\\\n" */
	count2 = dyn_Getstr(dstr, NULL);
	if (count2 == -1)
	    return -1;
	return (dynstr_Length(dstr) + count2);
    }
    if (!iscurses && isoff(glob_flags, ECHO_FLAG))
	putchar('\n');
    /* Should be null-terminated already, but just in case */
    wrapcolumn = save_wc;
    return dynstr_Length(dstr);
}

#endif /* !GUI_ONLY */

/*
 * Get a "string" of at most 'length' chars.
 * Print the prompt indicated by "pmpt", if any
 *  (Bart: Thu Feb 18 15:42:35 PST 1993).
 * Allow backspace-space-backspace, kill word and kill line
 *  (options set by user in stty).
 * Param "length" is the max length this string can get.
 * Param "offset" is where to insert characters, from beginning of string.
 * Input of ^D returns -1; otherwise, return the number of chars in string.
 *
 * Mon May 10 12:55:05 1993 (bobg) -- Make this a wrapper for dyn_Getstr.
 * Presume that offset is always zero (it's always called that way,
 * presently).
 */
int
Getstr(pmpt, string, length, offset)
char *pmpt, *string;
int length, offset;
{
    struct dynstr d;
    int result;

    dynstr_Init(&d);
    result = dyn_Getstr(&d, pmpt);
    *string = 0;
    strncat(string, dynstr_Str(&d), length);
    dynstr_Destroy(&d);
    return (result);
}

#ifndef GUI_ONLY

#undef Addch

/*
 * Check to see if what the user is typing is supposed to be expanded
 * into a longer string.  The first char is 'c' and the map list to use
 * is in map_list.  Continue looping (reading chars from stdin or a
 * currently active mapping) until a match happens or we've determined
 * that there is no match.
 */
int
check_map(c, map_list)
int c;
struct cmd_map *map_list;
{
    char mbuf[MAX_MACRO_LEN], *p = mbuf;
    struct cmd_map *list;
    int m, n, match;

    *p++ = (char) c;

    while (!check_intr()) {
	m = 0;
	*p = 0; /* make sure it's null terminated */
	/*
	 * loop thru the list of maps and check to see if the typed
	 * char matches the mapping.  If it matches completely, substitute
	 * the stuff in x_str and return.  If a partial match occurs, then
	 * read the next char until a timeout or no match.
	 */
	for (list = map_list; list; list = list->m_next) {
	    if ((match = prefix(mbuf, list->m_str)) == MATCH) {
		/* Must turn on flags BEFORE pushing */
		line_macro(list->x_str);
		return 1;
	    } else if (match != NO_MATCH)
		m++; /* something matched partially */
	}
	if (!m)
	    break;
	/* see if there's anything on the queue to read... */
	if (mac_pending()
#if !defined(SELECT) && !defined(M_UNIX)
#ifdef FIONREAD
	    || !ioctl(0, FIONREAD, &n) && n > 0
#else
#ifdef M_XENIX
	    || rdchk(0) > 0
#endif /* M_XENIX */
#endif /* FIONREAD */
#endif /* SELECT */
					       )
	    *p++ = m_getchar();
	else {
	/* The user has typed the first part of a map or macro.  Give him
	 * a chance to finish it.
	 */
#if defined(BSD) || defined(M_UNIX) || defined(SELECT)
	    /* If the system has select(), use it.  It's much faster and
	     * more aesthetic since there is no mandatory timeout.
	     */
	    struct timeval timer;
#ifdef FD_SET
	    fd_set rmask, wmask, xmask;
	    FD_SET(0, &rmask);	/* Test stdin for read */
	    FD_ZERO(&wmask);	/* Don't care about write */
	    FD_ZERO(&xmask);	/* Don't care about exception */
#else
	    int rmask = 1, wmask = 0, xmask = 0;
#endif /* FD_SET */
	    timer.tv_sec = 1;
	    timer.tv_usec = 0;
	    n = select(1, &rmask, &wmask, &xmask, &timer);
#else /* !SELECT */
#ifdef FIONREAD
	    /* system doesn't have select(), so use FIONREAD to see if
	     * there are any chars on the queue to read.
	     */
	    (void) sleep(1);
	    (void) ioctl(0, FIONREAD, &n);
#else
#ifdef M_XENIX
	    (void) sleep(1);
	    n = rdchk(0);
#else

	    /* system has neither select() nor FIONREAD, so just set n
	     * and force the user to either complete the map or fail it
	     * without a timeout.  Chars won't echo till he does one or
	     * the other.
	     */
	    n = 1;
#endif /* M_XENIX  */
#endif /* FIONREAD */
#endif /* SELECT */
	    if (n > 0)
		/* don't read all 'n' chars -- there may be a match early */
		*p++ = m_getchar();	/* To flush macros and reset flags */
	    else /* still nothing to read? User doesn't want to use map */
		break;
	}
    }
    /* no match or a timeout.  This isn't a map, just return. */
    *p = 0;
    if (mbuf[1])
	(void) mac_push(mbuf + 1);
    return 0;
}

/*
 * Check for line wrap.  This should happen only in composition mode and
 * only when the variable wrapcolumn has a value greater than zero.  Line
 * wrap is implemented using Ungetstr [that is, mac_push()].
 *
 * Returns 1 if the line was wrapped, 0 if not.
 */
int
line_wrap(dstr)
struct dynstr *dstr;
{
    char *tail;
    int n = dynstr_Length(dstr);

    if (wrapcolumn < 1
	|| dynstr_Length(dstr) <= wrapcolumn
	|| isoff(glob_flags, IS_GETTING) /* Wrap only in msg body */
	|| ison(glob_flags, QUOTE_MACRO) /* Don't wrap quoted macros */
	|| ison(glob_flags, ECHO_FLAG))	 /* Can't wrap in echo mode */
	return 0;

    /* Back up past the wrapcolumn point */
    for (; n > wrapcolumn; --n)
	;
    /* Look for a space */
    while (n && !isspace(dynstr_Str(dstr)[n]))
	--n;
    /* If no break found, return no wrap */
    if (!n)
	return 0;
    tail = &(dynstr_Str(dstr)[n+1]);
    /* Skip the break char and any whitespace */
    while (n && isspace(dynstr_Str(dstr)[n]))
	--n;
    ++n; /* move back into the whitespace */
    /* Push the tail, if any */
    if (*tail)
	Ungetstr(tail);
    /* Erase the stuff that will wrap */
    while (dynstr_Length(dstr) > n)
	backspace(dstr);
    return 1;
}

#endif /* !GUI_ONLY */

/*
 * Error bell used by completion()
 */
int
errbell(ret)
int ret;
{
    if (ret < 0 || !chk_option(VarQuiet, "complete"))
	bell();
    return ret;
}

#ifndef GUI_ONLY

/*
 * Perform word completion on the input string
 */
int
completion(dstr, showlist, ignore)
struct dynstr *dstr;	/* The string to be completed */
int showlist;		/* Display list, complete if also ignore */
int ignore;		/* Ignore files as per fignore, do complete */
{
    char buf[MAXPATHLEN], *b = buf, **exp;
    int n = dynstr_Length(dstr), f, len, prefix, trim, overstrike, expandall;

    if (dynstr_Length(dstr) == 0)
	return errbell(-1);

    /* Look for a delimiter */
    while (n > 0 && !index(DELIM, dynstr_Str(dstr)[--n]))
	;
    if (n > 0 || index(DELIM, dynstr_Str(dstr)[n]))
	n++;
    if (dynstr_Length(dstr) - n >= MAXPATHLEN)
	return errbell(-1);
    b = buf + (len = Strcpy(buf, &(dynstr_Str(dstr)[n])));
    Debug("\nexpanding (%s) ... ", buf);
    if (!any(buf, FMETA)) {
	expandall = 0;
	overstrike = (*buf == '+' || *buf == '~' || *buf == '%');
	trim = (overstrike && len > 1);
	if (!overstrike || len > 1 || (*buf == '+' && showlist))
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
	    expandall = 0;
	} else
	    expandall = !showlist;
    }
    if (ignore)
	f = fignore(f, &exp);
    if (f < 0) {
	Debug("globbing error!\n%s", dynstr_Str(dstr));
	free_vec(exp);
	return errbell(-1);
    } else if (f > 0) {
	Debug("result is: "), print_argv(exp);
	if (!expandall && f > 1)
	    prefix = lcprefix(exp, overstrike ? 0 : len);
	else
	    prefix = 0;
	if (showlist && (f > 1 || !ignore)) {
	    int pfx = prefix;
	    if (!expandall)
		while (pfx && !is_dsep(exp[0][pfx - 1]))
		    --pfx;
	    putchar('\n');
	    if (columnate(f, exp, pfx, TRPL_NULL) < 0)
		(void) errbell(-1);
	    /* Reprint the line */
	    wprint("%s%s",
		lastprompt? lastprompt : iscurses? ":" : "", dynstr_Str(dstr));
	    if (iscurses)
		turnon(glob_flags, CNTD_CMD);
	    if (!ignore)
		overstrike = 0;
	}
	if (ignore || !showlist) {
	    if (expandall || strlen(exp[0]) > len) {
		if (!showlist || f == 1)
		    Debug("%s", dynstr_Str(dstr));
		if (overstrike && (prefix || expandall || f == 1)) {
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
		    if (n + len == dynstr_Length(dstr)) {
			if (!showlist && !expandall && !tmpv[1][len])
			    (void) errbell(0);
		    }
		    /* Erase the stuff that will complete */
		    while (dynstr_Length(dstr) > n + len)
			backspace(dstr);
		}
		if (expandall || f == 1) {
		    /* Unget the names IN REVERSE ORDER! */
		    while (f--) {
			if (trim)
			    b = trim_filename(exp[f]);
			else
			    b = exp[f];
			if (f) {
			    Ungetstr(b);
			    Ungetstr(" ");
			} else
			    Ungetstr(b + len);
		    }
		} else {
		    if (prefix > len) {
			exp[0][prefix] = 0;
			if (!showlist)
			    Debug("\ncompletion is (%s)\n%s",
				exp[0], dynstr_Str(dstr));
			if (trim)
			    Ungetstr(trim_filename(exp[0]) + len);
			else
			    Ungetstr(&exp[0][len]);
		    } else if (!showlist)
			Debug("\nno longer prefix\n%s", dynstr_Str(dstr));
		    /* Special case because "+" always tries to expand "+*"
		     * to get listings and avoid getpath()'s trailing '/'.
		     * No error bell is needed in those cases.
		     */
		    if (strcmp(buf, "+") != 0)
			(void) errbell(0);
		}
	    } else {
		Debug("no longer prefix\n%s", dynstr_Str(dstr));
		(void) errbell(0);
	    }
	}
    } else {
	Debug("no match\n%s", dynstr_Str(dstr));
	(void) errbell(0);
    }
    free_vec(exp);
    return 1;
}

#endif /* !GUI_ONLY */

int
fignore(argc, argvp)
int argc;
char ***argvp;
{
    char *fign = value_of(VarFignore);
    char **flist, buf[MAXPATHLEN], *b = buf;
    int fcnt, i;

    if (argc < 2 || !fign || !*fign)
	return argc;
    if (!argvp || !*argvp && !**argvp)
	return -1;
    
    if ((flist = mk_argv(fign, &fcnt, FALSE)) && fcnt > 0) {
	*b++ = '*';
	for (i = 0; i < fcnt; i++) {
	    if (flist[i][0] == '.' && !any(flist[i], FMETA)) {
		(void) strcpy(b, flist[i]);
		(void) ZSTRDUP(flist[i], buf);
	    }
	}
	Debug("ignoring "), print_argv(flist);
	fcnt = gdiffv(argc, argvp, fcnt, flist);
	free_vec(flist);
	if (fcnt == 0)
	    fcnt = argc;
	else if (fcnt < argc) {
	    free_elems(&((*argvp)[fcnt]));
	    (*argvp)[fcnt] = NULL;
	}
    }
    return fcnt;
}
