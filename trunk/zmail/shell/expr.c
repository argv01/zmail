/* expr.c     Copyright 1990, 1991 Z-Code Software Corp. */

#include "zmail.h"
#include "catalog.h"
#include "expr.h"
#include <ctype.h>
#include <general.h>

char *eval_expr P((register const char *, msg_group *));

/* Parse a string (p) to interpret numbers and ranges of numbers (n-m)
 * delimited by whitespace or comma's. Set msg_list bitfields using
 * macros in zmail.h.
 * Return the address of the end of whatever we parsed (in case there's
 * more that the calling routine cares to deal with).
 * Finally, remember that user specifies one more than actual message number
 *
 * If list1 is NULL_GRP, just parse/verify the range of messages and return
 * NULL on failure.  Don't call error() in this case.
 */
char *
zm_range(p, list1)
register const char *p;
struct mgroup *list1;
{
    register int num1 = -1, num2 = -1, except = 0;
    register const char *p2;

    if (!p)
	return "";
    while (*p) {
	if (isdigit(*p) || *p == '$' || *p == '.' || *p == '^') {
	    if (isdigit(*p)) {
		char c;
		p2 = p;
		skipdigits(0);  /* find the end of the digits */
		/* XXX casting away const */
		c = *p, *(char *)p = 0; /* temporarily plug a null */
		if (!(num2 = chk_msg(p2))) {
		    if (list1) {
			error(UserErrWarning,
			    catgets(catalog, CAT_SHELL, 307,
				    "Invalid message number: %s"),
			    p2);
			clear_msg_group(list1);
		    }
		    return NULL;
		}
		/* XXX casting away const */
		*(char *)p = c;
	    } else if (*p == '$')
		p++, num2 = msg_cnt;
	    else if (*p == '.')
		p++, num2 = current_msg+1;
	    else if (*p == '^')
		p++, num2 = 1;
	    if (list1) {
		if (except)
		    rm_msg_from_group(list1, num2-1);
		else
		    add_msg_to_group(list1, num2-1);
	    }
	    if (num1 >= 0) {
		if (num1 > num2) {
		    if (list1) {
			error(UserErrWarning,
			    catgets(catalog, CAT_SHELL, 308,
			    "syntax error: range sequence order reversed."));
			clear_msg_group(list1);
		    }
		    return NULL;
		}
		if (list1) {
		    while (++num1 < num2)
			if (except)
			    rm_msg_from_group(list1, num1-1);
			else
			    add_msg_to_group(list1, num1-1);
		}
		num1 = num2 = -1;
	    }
	}
	/* expressions to evaluate start with a `
	 * p2 points to first char past the last char parsed.
	 */
	if (*p == '`') {
	    if (list1) {
		msg_group list2;
		init_msg_group(&list2, msg_cnt, 1);
		clear_msg_group(&list2);
		if (!(p = eval_expr(p, &list2))) {
		    clear_msg_group(list1);
		    destroy_msg_group(&list2);
		    return NULL;
		} else {
		    if (except)
			msg_group_combine(list1, MG_SUB, &list2);
		    else
			msg_group_combine(list1, MG_ADD, &list2);
		}
		destroy_msg_group(&list2);
	    } else if (!(p = index(++p, '`')))
		return NULL;
	}
	/* NOT operator: `* {5}' (everything except for 5)
	 * `4-16 {8-10}'  (4 thru 16 except for 8,9,10)
	 */
	if (*p == '{' || *p == '}') {
	    if (*p == '{' && (except || num1 >= 0))
		break;
	    if (*p == '}' && !except) {
		if (list1)
		    error(UserErrWarning,
			catgets(catalog, CAT_SHELL, 309,
				"syntax error: missing {")); /* } */
		break;
	    }
	    except = !except;
	} else if (*p == '-')
	    if (num1 >= 0 || num2 < 0
		    || !index(" \t{},.*`$", *(p+1)) && !isdigit(*(p+1)))
		break;
	    else
		num1 = num2;
	else if (*p == ',' || *p == '*') {
	    if (num1 >= 0)
		break;
	    else if (*p == '*') {
		if (list1) {
		    if (except)
			clear_msg_group(list1);
		    else
			for (num1 = 0; num1 < msg_cnt; num1++)
			    add_msg_to_group(list1, num1);
		}
		num1 = -1;
	    }
	} else if (!index(" \t`", *p))
	    break;
	if (*p)
	    skipspaces(1); /* don't make user type stuff squished together */
    }
    if (num1 >= 0 || except) {
	if (list1) {
	    if (except)
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 310,
			    /* { */ "syntax error: unmatched }"));
	    else
		error(UserErrWarning,
		    catgets(catalog, CAT_SHELL, 311,
			    "syntax error: unfinished range"));
	    clear_msg_group(list1);
	}
	return NULL;
    }
    return (char *) p;		/* XXX casting away const */
}

/*
 * convert a message list to an ascii string.
 */
char *
list_to_str(list)
struct mgroup *list;
{
    int n, m;
    char *str, *s;

    /* Guess how large a string we need.
     * A message number string is at most strlen(itoa(msg_cnt)) bytes.
     * A sequence of message numbers N-M is at most twice that, plus 1.
     * There's a comma after every sequence, so add one more.
     * The greatest number of sequences is (msg_cnt/3).
     * Round up to (msg_cnt/3+1) so that integer division doesn't give 0.
     * Factoring gives the final formula:
     *    2 * (msg_cnt / 3 + 1) * (strlen(itoa(msg_cnt)) + 1)
     */
    if (!(s = str = (char *)
	    malloc(2 * (msg_cnt / 3 + 1) * (strlen(itoa(msg_cnt)) + 1))))
	return NULL;

    for (m = -1, n = 0; n < msg_cnt; n++) {
	if (msg_is_in_group(list, n)) {
	    if (m == -1) {
		sprintf(str, "%d", (m = n) + 1);
		str += strlen(str);
	    }
	    continue;
	}
	if (m == -1)
	    continue;
	if (n - m > 2) {
	    sprintf(str, "-%d", n);
	    str += strlen(str);
	} else if (n - m == 2) {
	    sprintf(str, ",%d", n);
	    str += strlen(str);
	}
	*str++ = ',';
	m = -1;
    }
    if (m > -1 && m != n - 1) {
	if (n - m > 2)
	    *str++ = '-';
	else
	    *str++ = ',';
	str += Strcpy(str, itoa(msg_cnt));
    } else if (m != n - 1 && str > s)
	str--; /* kill the trailing comma */
    *str = 0;

    return s;
}

/* Convert a string of message metacharacters to a msg_group.
 * Returns the end-position of the string parsed, for historical
 * reasons and so the caller can continue parsing.
 */
char *
str_to_list(list, str)
struct mgroup *list;
const char *str;
{
    char *p2, ch;
    const char *p = str, *end;

    /* find the end of the message list */
    skipmsglist(0);
    end = p;
    while (*end && end != str && !isspace(*end))
	--end;
    /* XXX casting away const */
    ch = *end, *(char *)end = '\0'; /* temporarily plug with nul */
    p = str; /* reset to the beginning */
    /*
     * if zm_range returns NULL, an invalid message was specified
     */
    if (!(p2 = zm_range(p, list))) {
	/* XXX casting away const */
	*(char *)end = ch; /* just in case */
	return NULL;
    }
    /*
     * if p2 == p (and p isn't $ or ^ or .), then no message list was
     * specified.  set the current message in such cases if we're not piping
     */
    if (p2 == p) {
	if (*p == '$')
	    add_msg_to_group(list, msg_cnt-1);
	else if (*p == '^')
	    add_msg_to_group(list, 0);
	else if (*p == '.' || isoff(glob_flags, IS_PIPE))
	    add_msg_to_group(list, current_msg);
    }
    /* XXX casting away const */
    *(char *)end = ch;
    return p2;
}

/* evaluate expressions:
 * mail> delete `pick -f root`     deletes all messages from root.
 * mail> save * {`pick -s "Re:"`}  save all message that don't have "Re:"
 *				   in the subject header.
 * mail> save `pick -x -s "Re:"`   same
 * args as follows:
 *   p should point to the first ` -- check for it.
 *   on tells whether to turn bits on or off if messages match.
 */
char *
eval_expr(p, new_list)
register const char *p;
struct mgroup *new_list;
{
    register char *p2, **argv;
    int 	  argc;
    u_long	  save_flags = glob_flags;

    if (!(p2 = index(++p, '`'))) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 312, "unmatched backquote (`)" ));
	return NULL;
    }
    *p2 = 0;

    skipspaces(0);
    if (!*p) {
	error(UserErrWarning, catgets( catalog, CAT_SHELL, 91, "Invalid null command." ));
	return NULL;
    }
    turnon(glob_flags, DO_PIPE);
    turnoff(glob_flags, IS_PIPE);
    /* ignore sigs only because if user interrupts the zm_command,
     * the longjmp will corrupt the stack and the program is hosed.
     * fix is to have layers of jmp_bufs to return to different levels.
     */
    turnon(glob_flags, IGN_SIGS);
    if (*p && (argv = make_command(p, TRPL_NULL, &argc))) {
	clear_msg_group(new_list);
	(void) zm_command(argc, argv, new_list);
    }
    if (isoff(save_flags, IGN_SIGS))
	turnoff(glob_flags, IGN_SIGS);
    if (ison(save_flags, DO_PIPE))
	turnon(glob_flags, DO_PIPE);
    if (ison(save_flags, IS_PIPE))
	turnon(glob_flags, IS_PIPE);
    *p2 = '`';
    return p2+1;
}

int
eq_to(lhs, rhs)
char *lhs, *rhs;
{
    int l = atoi(lhs);
    int r = atoi(rhs);

    if ((l || isdigit(*lhs)) && (r || isdigit(*rhs)))
	return l == r;
    else
	return !strcmp(lhs, rhs);
}

int
lthan(lhs, rhs)
char *lhs, *rhs;
{
    int l = atoi(lhs);
    int r = atoi(rhs);

    if ((l || isdigit(*lhs)) && (r || isdigit(*rhs)))
	return l < r;
    else if (!(isdigit(*lhs) || isdigit(*rhs)))
	return strcmp(lhs, rhs) < 0;
    return 0;
}

int
lt_or_eq(lhs, rhs)
char *lhs, *rhs;
{
    int l = atoi(lhs);
    int r = atoi(rhs);

    if ((l || isdigit(*lhs)) && (r || isdigit(*rhs)))
	return l <= r;
    else if (!(isdigit(*lhs) || isdigit(*rhs)))
	return strcmp(lhs, rhs) <= 0;
    return 0;
}

int
gthan(lhs, rhs)
char *lhs, *rhs;
{
    int l = atoi(lhs);
    int r = atoi(rhs);

    if ((l || isdigit(*lhs)) && (r || isdigit(*rhs)))
	return l > r;
    else if (!(isdigit(*lhs) || isdigit(*rhs)))
	return strcmp(lhs, rhs) > 0;
    return 0;
}

int
gt_or_eq(lhs, rhs)
char *lhs, *rhs;
{
    int l = atoi(lhs);
    int r = atoi(rhs);

    if ((l || isdigit(*lhs)) && (r || isdigit(*rhs)))
	return l >= r;
    else if (!(isdigit(*lhs) || isdigit(*rhs)))
	return strcmp(lhs, rhs) >= 0;
    return 0;
}

#include "regexpr.h"

static struct re_pattern_buffer re_buf;		/* Re-use malloc'd buffers */

/* Substitute sub for pat in str, return allocated result */
char *
re_subst(str, pat, sub)
char *str, *pat, *sub;
{
    struct re_registers re_regs;
    int match, len, tot;
    char *re_ret;

    if (!str || !pat)
	return NULL;
    len = strlen(str);

    if (re_compile_pattern(pat, strlen(pat), &re_buf))
	return NULL;
    match = re_search(&re_buf, str, len, 0, len, &re_regs);
    if (match < 0)
	return NULL;
    if (!sub) {
	match = (re_regs.start[1] >= 0)? 1 : 0;
	return savestrn(str + re_regs.start[match],
			re_regs.end[match] - re_regs.start[match]);
    }
    tot = re_regs.start[0] + strlen(sub) + len - re_regs.end[0] + 1;
    if (re_ret = (char *) malloc(tot))
	(void) sprintf(re_ret, "%*.*s%s%s", re_regs.start[0], re_regs.start[0],
				str, sub, str+re_regs.end[0]);
    return re_ret;
}

/* Return true if pat appears in str, false otherwise */
int
re_glob(str, pat, redo)
char *str, *pat;
int redo;	/* Need to recompile pat? */
{
    int match, len;

    if (!str || !pat)
	return 0;

    len = strlen(str);

    if (!re_buf.buffer)
	redo = 1;

    if (redo && re_compile_pattern(pat, strlen(pat), &re_buf))
	return 0;
    match = re_search(&re_buf, str, len, 0, len, (struct re_registers *)0);
    if (match < 0)
	return 0;
    return 1;
}

/*
 * Z-Script command:  match pattern string
 *
 * Assigns to the zscript variables $__match_1 through $__match_9
 * the substrings selected by parenthesized subpatterns of pattern.
 * If there are fewer than 9 substrings, unsets the __match variables
 * numbered higher than the last match.  Also assigns to $__match_0
 * the substring matched by the entire pattern, and to $__matches
 * the number of substrings that were matched (0 if only the entire
 * pattern matched, or unsets it if no patterns matched).
 *
 * Does NOT unset $__match_* on failure!  Only $__matches is unset
 * in this case.
 *
 * Returns 0 on success, > 0 on no match, < 0 on error.
 */
int
zm_match(argc, argv, unused)
int argc;
char **argv;
struct mgroup *unused;
{
    struct re_registers re_regs;
    int i, len, tot;
    char *re_ret;
    static char matchvar[] = "__match_0",
		matchlen[] = "__len_0",
		matchbeg[] = "__start_0";
#define MATCHSTR(x) (matchvar[8] = '0'+(x), matchvar)
#define MATCHLEN(x) (matchlen[6] = '0'+(x), matchlen)
#define MATCHPOS(x) (matchbeg[8] = '0'+(x), matchbeg)
#define MATCHCOUNT "__matches"

    if (argc < 3) {
	return -1;
    }
    if (re_compile_pattern(argv[1], strlen(argv[1]), &re_buf)) {
	return -1;
    }
    len = strlen(argv[2]);
    if (argv[0][0] == 's') {	/* scan */
	/* Look for the pattern in a file or a message */
	return 1;
    } else {			/* match */
	if (re_search(&re_buf, argv[2], len, 0, len, &re_regs) < 0) {
	    un_set(&set_options, MATCHCOUNT);
	    return 1;
	}
    }
    len = re_regs.end[0] - re_regs.start[0] + 8; /* 8 --> room for %d */
    if (!(re_ret = (char *) malloc(len))) {
	error(SysErrWarning, argv[0]);
	return -1;
    }
    for (tot = i = 0; i < RE_NREGS; i++) {
	if (re_regs.start[i] < 0) {
	    un_set(&set_options, MATCHSTR(i));
	    un_set(&set_options, MATCHPOS(i));
	    un_set(&set_options, MATCHLEN(i));
	    continue;
	}
	tot++;
	len = re_regs.end[i] - re_regs.start[i];
	sprintf(re_ret, "%d", re_regs.start[i]);
	set_var(MATCHPOS(i), "=", re_ret);
	sprintf(re_ret, "%d", len);
	set_var(MATCHLEN(i), "=", re_ret);
	if (len == 0) {
	    set_var(MATCHSTR(i), "=", "");
	    continue;
	}
	strncpy(re_ret, argv[2] + re_regs.start[i], len);
	re_ret[len] = 0;
	set_var(MATCHSTR(i), "=", re_ret);
    }
    sprintf(re_ret, "%d", tot - 1);
    set_var(MATCHCOUNT, "=", re_ret);
    xfree(re_ret);
    return 0;
}
