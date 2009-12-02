/* @(#)curs_io.c	(c) copyright 3/18/87 (Dan Heller) */

/* curs_io.c -- curses based I/O */
#include "mush.h"
#include "bindings.h"
#include "glob.h"

static backspace();

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
#endif /* !M_XENIX || (M_XENIX && !CURSES) */

char	del_line;	/* tty delete line character */
char	del_word;	/* tty delete word character */
char	del_char;	/* backspace */
char	reprint_line;	/* usually ^R */
char	eofc;		/* usually ^D */
char	lit_next;	/* usually ^V */
char	complete;	/* word completion, usually ESC */
char	complist;	/* completion listing, usually ^D */

tty_settings()
{
    int is_tty = isatty(0);

    if (is_tty)
	savetty();

#if defined(SYSV) || defined(AIX)
    eofc = _tty.c_cc[VEOF];
#else
#ifdef BSD
    if (ioctl(0, TIOCGETC, &tchars) != -1)
	eofc = tchars.t_eofc;
    else
#endif /* BSD */
	eofc = CTRL('D');
#endif /* SYSV */

    if (!is_tty) {
	del_line = CTRL('U');
	del_char = CTRL('H');
    } else {
	del_line = _tty.sg_kill;
	del_char = _tty.sg_erase;
    }

#ifdef TIOCGLTC
#ifndef AIX	/* Just in case */
#ifndef AUX	/* AUX defines TIOCGLTC but doesn't use it */
    if (ioctl(0, TIOCGLTC, &ltchars) != -1) {
	del_word = ltchars.t_werasc;
	reprint_line = ltchars.t_rprntc;
	lit_next = ltchars.t_lnextc;
    } else
#endif /* AUX */
#endif /* AIX */
#endif /* TIOCGLTC */
    {
	del_word = CTRL('W');
	reprint_line = CTRL('R');
	lit_next = CTRL('V');
    }
}

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

/*
 * get a string of at most 'length' chars.
 * allow backspace-space-backspace, kill word and kill line
 * (options set by user in stty).
 * length is the max length this string can get. offset is from beginning
 * of string.
 * input of ^D returns -1; otherwise, return the number of chars in string.
 */
Getstr(String, length, offset)
char String[];
register int length;
{
    register int c, literal_next = FALSE, lit_bs = FALSE;
    struct cmd_map *curr_map;
    int count = offset, save_wc = wrapcolumn;

    char* eightbits = do_set(set_options, "eightbits");

    (void) fflush(stdout); /* make sure everything is flushed before getting input */

    if (mac_hide) {
	curr_map = NULL_MAP;
	wrapcolumn = 0;
    } else if (ison(glob_flags, IS_GETTING))
	curr_map = bang_map;
    else if (iscurses)
	curr_map = NULL_MAP;
    else
	curr_map = line_map;

    while ((c = m_getchar()) != '\n' && c != '\r' && c != EOF &&
	    isoff(glob_flags, WAS_INTR)) {
	/* echo isn't set, so whatever the character, enter it */
	if (ison(glob_flags, QUOTE_MACRO) || ison(glob_flags, ECHO_FLAG)) {
	    if (count < length) {
		String[count++] = c;
		/* Note: Addch includes ECHO_FLAG test */
		if (iscntrl(c)) {
		    if (c < sizeof(_unctrl) / sizeof(_unctrl[0])) {
		    	Addch('^');
		    	Addch(_unctrl[c][1]);
		    } else if (eightbits) {
			Addch(c);
		    } else {
			char hex[] = "0123456789abcdef";

			Addch('\\');
			Addch('x');
			Addch(hex[c / 020]);
			Addch(hex[c % 020]);
		    }
		} else
		    Addch(c);
	    } else {
		print("\nWarning: string too long. Truncated at %d chars.",
		    length);
		if (ison(glob_flags, QUOTE_MACRO)) {
		    mac_flush();
		    m_ungetc(reprint_line);
		    continue;
		} else
		    break;
	    }
	}
	/* ^D as the first char on a line or two ^D's in a row is EOF */
	else if (c == eofc && !count)
	    break;
	else if (c == '\\' && count < length) {
	    literal_next = TRUE, lit_bs = FALSE;
	    Addch(String[count++] = '\\');
    	} else if (c == lit_next && count < length) {
	    literal_next = lit_bs = TRUE;
	    String[count++] = '\\';
	    if (!in_macro()) {
		/* if (iscntrl(c)) */
		    Addch('^');
		/* Addch(_unctrl[c][1]); */
	    }
	} else if (literal_next) {
	    struct cmd_map *list;

	    literal_next = FALSE;
	    if (iscntrl(c) || c == del_line || c == del_char || c == del_word
		    || c == lit_next || lit_bs)
		if (!in_macro() || !lit_bs)
		    backspace(String, &count, eightbits);
		else
		    --count;
	    else if (in_macro() && c == MAC_LONG_CMD)
		--count;
	    /* check to see if user is escaping a map or map! */
	    else
		for (list = curr_map; list; list = list->m_next)
		    if (list->m_str[0] == c) {
			if (!in_macro())
			    backspace(String, &count, eightbits);
			else
			    --count;
			break;
		    }
	    /* A literal-next advances the macro offset */
	    String[count++] = c;
	    if (iscntrl(c) || c == del_char) {
		if (c < sizeof(_unctrl) / sizeof(_unctrl[0])) {
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
		} else if (eightbits) {
		    Addch(c);
		} else {
		    char hex[] = "0123456789abcdef";

		    Addch('\\');
		    Addch('x');
		    Addch(hex[c / 020]);
		    Addch(hex[c % 020]);
		}
	    } else
		Addch(c);
	} else if (complete && (c == complete || c == complist)) {
	    (void) completion(String, &count, (c == complist), (c == complete));
	} else if (c == del_line) {
	    if (count) {
		do
		    backspace(String, &count, eightbits);
		while (count);
	    }
	} else if (c == reprint_line)
	    String[count] = 0, wprint("\n%s", String);
	else if (c == del_word) /* word erase */
	    while (count) {
		backspace(String, &count, eightbits);
		if (!count ||
		    isspace(String[count-1]) && !isspace(String[count]) ||
		    !isalnum(String[count-1]) && isalnum(String[count]))
		    break;
	    }
	else if (c == del_char || c == CTRL('H') || c == 127 /* CTRL('?') */) {
	    if (count)
		backspace(String, &count, eightbits);
	    /* if iscurses, then backspacing too far cancels a function */
	    else if (!count && iscurses && isoff(glob_flags, LINE_MACRO)) {
		mac_flush();
		String[0] = '\0';
		wrapcolumn = save_wc;
		return -1;
	    }
	} else if (count == length)
	    bell();
	else if (c == '\t')
	    do  {
		/* Yuck -- tabs break map! */
		Addch(' ');
		String[count] = ' ';
	    } while (++count % 8 && count < length);
	else if (in_macro() && c == MAC_LONG_CMD) {
	    char cbuf[MAX_LONG_CMD + 1];

	    if ((c = read_long_cmd(cbuf)) == 0) {
		c = MAC_LONG_CMD;
		goto check_expand;	/* How could I avoid this? */
	    } else if (c > 0) {
		int ok;

		String[count] = '\0';
		if ((ok = reserved_cmd(cbuf, TRUE)) > 0) {
		    /* Reprint the line */
		    if (iscurses)
			print(":%s", String);
		    else
			wprint("\r%s", String);
		    continue;	/* Get next char without changing count */
		} else if (ok < 0) {
		    String[offset] = '\0';
		    wrapcolumn = save_wc;
		    return ok;
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
		    if (c < sizeof(_unctrl) / sizeof(_unctrl[0])) {
		    	Addch('^');
		    	Addch(_unctrl[c][1]);
		    } else if (eightbits) {
			Addch(c);
		    } else {
			char hex[] = "0123456789abcdef";

			Addch('\\');
			Addch('x');
			Addch(hex[c / 020]);
			Addch(hex[c % 020]);
		    }
		    /* Decrement wrapcolumn as above */
		    if (wrapcolumn > 1)
			wrapcolumn--;
		} else
		    Addch(c);
		String[count++] = c;
	    }
	}
	/* Null-terminate for macro lookup purposes.
	 * This will be overwritten by the next character.
	 */
	String[count] = '\0';
	if (line_wrap(String, &count))
	    break;
    }
    (void) fflush(stdout); /* for sys-v folks */

    if (c == eofc || c == EOF || ison(glob_flags, WAS_INTR)) {
	if (feof(stdin))
	    clearerr(stdin);
	wrapcolumn = save_wc;
	return -1;
    }
    if (count && String[count-1] == '\\') {
	int count2;
	if (isoff(glob_flags, ECHO_FLAG))
	    putchar('\n');
	wrapcolumn = save_wc;
	/*
	 * NOTE: If the offset passed here is ever made greater than 0,
	 * the value of wrapcolumn must again be changed/restored ...
	 */
	if ((count2 = Getstr(&String[count-1], length - count + 1, 0)) == -1)
	    return -1;
	return count + count2;
    }
    if (!iscurses && isoff(glob_flags, ECHO_FLAG))
	putchar('\n');
    /* Should be null-terminated already, but just in case */
    String[count] = '\0';
    wrapcolumn = save_wc;
    return count;
}

static
backspace(str, n, eb)
register char *str;
int *n;
char* eb;
{
    (*n)--;
    Addch('\b'); Addch(' '); Addch('\b');
    if (iscntrl(str[*n])) {
	if (str[*n] >= sizeof(_unctrl) / sizeof(_unctrl[0])) {
	    if (eb) {
		return 0;
	    }

	    Addch('\b'); Addch(' '); Addch('\b');
	    Addch('\b'); Addch(' '); Addch('\b');
	}
	Addch('\b'); Addch(' '); Addch('\b');
	/* Re-increment wrapcolumn -- see Getstr */
	if (wrapcolumn)
	    wrapcolumn++;
    }
}

#undef Addch

/*
 * Check to see if what the user is typing is supposed to be expanded
 * into a longer string.  The first char is 'c' and the map list to use
 * is in map_list.  Continue looping (reading chars from stdin or a
 * currently active mapping) until a match happens or we've determined
 * that there is no match.
 */
check_map(c, map_list)
char c;
struct cmd_map *map_list;
{
    char mbuf[MAX_MACRO_LEN], *p = mbuf;
    struct cmd_map *list;
    int m, n, match;

    *p++ = c;

    while (isoff(glob_flags, WAS_INTR)) {
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
line_wrap(string, count)
char *string;	/* The string to be wrapped */
int *count;	/* Offset of string terminator */
{
    char *tail = NULL;
    int n = *count;

    if (wrapcolumn < 1 || *count <= wrapcolumn
	    || isoff(glob_flags, IS_GETTING)	/* Wrap only in msg body */
	    || ison(glob_flags, QUOTE_MACRO)	/* Don't wrap quoted macros */
	    || ison(glob_flags, ECHO_FLAG))	/* Can't wrap in echo mode */
	return 0;

    /* Back up past the wrapcolumn point */
    for (; n > wrapcolumn; --n)
	;
    /* Look for a space */
    while (n && !isspace(string[n]))
	--n;
    /* If no break found, return no wrap */
    if (!n)
	return 0;
    tail = &string[n+1];
    /* Skip the break char and any whitespace */
    while (n && isspace(string[n]))
	--n;
    ++n; /* move back into the whitespace */
    /* Erase the stuff that will wrap */
    while (*count > n)
	backspace(string,count);
    string[*count] = '\0';
    /* Push the tail, if any */
    if (*tail)
	Ungetstr(tail);
    return 1;
}

/*
 * Error bell used by completion()
 */
errbell(ret)
int ret;
{
    if (ret < 0 || !chk_option("quiet", "complete,completion"))
	bell();
    return ret;
}

/*
 * Perform word completion on the input string
 */
completion(string, count, showlist, ignore)
char *string;	/* The string to be completed */
int *count;	/* Offset of string terminator */
int showlist;	/* Display list, complete if also ignore */
int ignore;	/* Ignore the fignore matches, do complete */
{
    char buf[MAXPATHLEN], *b = buf, **exp;
    int n = *count, f, len, prefix, trim, overstrike, expandall;

    if (!*string || !*count)
	return errbell(-1);

    /* Look for a delimiter */
    while (n > 0 && !index(DELIM, string[--n]))
	;
    if (n > 0 || index(DELIM, string[n]))
	n++;
    b = buf + (len = Strcpy(buf, &string[n]));
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
	Debug("globbing error!\n%s", string);
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
		while (pfx && exp[0][pfx - 1] != '/')
		    --pfx;
	    putchar('\n');
	    if (columnate(f, exp, pfx) < 0)
		(void) errbell(-1);
	    /* Reprint the line */
	    if (iscurses) {
		wprint(":%s", string);
		turnon(glob_flags, CNTD_CMD);
	    } else {
		if (isoff(glob_flags, IS_GETTING))
		    mail_status(1);
		wprint("%s", string);
	    }
	    if (!ignore)
		overstrike = 0;
	} 
	if (ignore || !showlist) {
	    if (expandall || strlen(exp[0]) > len) {
		if (!showlist)
		    Debug("%s", string);
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
		    if (n + len == *count) {
			if (!expandall && !tmpv[1][len])
			    (void) errbell(0);
		    }
		    /* Erase the stuff that will complete */
		    while (*count > n + len)
			backspace(string,count);
		    string[*count] = '\0';
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
			    Debug("\ncompletion is (%s)\n%s", exp[0], string);
			if (trim)
			    Ungetstr(trim_filename(exp[0]) + len);
			else
			    Ungetstr(&exp[0][len]);
		    } else if (!showlist)
			Debug("\nno longer prefix\n%s", string);
		    /* Special case because "+" always tries to expand "+*"
		     * to get listings and avoid getpath()'s trailing '/'.
		     * No error bell is needed in those cases.
		     */
		    if (strcmp(buf, "+") != 0)
			(void) errbell(0);
		}
	    } else {
		Debug("no longer prefix\n%s", string);
		(void) errbell(0);
	    }
	}
    } else {
	Debug("no match\n%s", string);
	(void) errbell(0);
    }
    free_vec(exp);
    return 1;
}

fignore(argc, argvp)
int argc;
char ***argvp;
{
    char *fign = do_set(set_options, "fignore");
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
		(void) strdup(flist[i], buf);
	    }
	}
	Debug("ignoring "), print_argv(flist);
	fcnt = gdiffv(argc, argvp, fcnt, flist);
	free_vec(flist);
	if (fcnt == 0)
	    fcnt = argc;
	else {
	    free_elems(&((*argvp)[fcnt]));
	    (*argvp)[fcnt] = NULL;
	}
    }
    return fcnt;
}
