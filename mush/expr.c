/* @(#)expr.c	2.3	(c) copyright 10/15/86 (Dan Heller) */

#include "mush.h"

char *eval_expr();

/* Parse a string (p) to interpret numbers and ranges of numbers (n-m)
 * delimited by whitespace or comma's. Set msg_list bitfields using
 * macros in mush.h.
 * Return the address of the end of whatever we parsed (in case there's
 * more that the calling routine cares to deal with).
 * Finally, remember that user specifies one more than actual message number
 */
char *
do_range(p, list1)
register char *p, *list1;
{
    register int num1 = -1, num2 = -1, except = 0;
    register char *p2;
    char list2[MAXMSGS_BITS];

    if (!p)
	return "";
    while (*p) {
	if (isdigit(*p) || *p == '$' || *p == '.' || *p == '^') {
	    if (isdigit(*p)) {
		char c;
		p2 = p;
		skipdigits(0);  /* find the end of the digits */
		c = *p, *p = 0; /* temporarily plug a null */
		if (!(num2 = chk_msg(p2))) {
		    clear_msg_list(list1);
		    return NULL;
		}
		*p = c;
	    } else if (*p == '$')
		p++, num2 = msg_cnt;
	    else if (*p == '.')
		p++, num2 = current_msg+1;
	    else if (*p == '^')
		p++, num2 = 1;
	    if (except)
		unset_msg_bit(list1, num2-1);
	    else
		set_msg_bit(list1, num2-1);
	    if (num1 >= 0) {
		if (num1 > num2) {
		    print("syntax error: range sequence order reversed.\n");
		    clear_msg_list(list1);
		    return NULL;
		}
		while (++num1 < num2)
		    if (except)
			unset_msg_bit(list1, num1-1);
		    else
			set_msg_bit(list1, num1-1);
		num1 = num2 = -1;
	    }
	}
	/* expressions to evaluate start with a `
	 * p2 points to first char passed the last char parsed.
	 */
	if (*p == '`') {
	    clear_msg_list(list2);
	    if (!(p = eval_expr(p, list2))) {
		clear_msg_list(list1);
		return NULL;
	    } else {
		if (except)
		    bitput(list2, list1, msg_cnt, &=~); /* MACRO */
		else
		    bitput(list2, list1, msg_cnt, |=); /* MACRO */
	    }
	}
	/* NOT operator: `* {5}' (everything except for 5)
	 * `4-16 {8-10}'  (4 thru 16 except for 8,9,10)
	 */
	if (*p == '{' || *p == '}') {
	    if (*p == '{' && (except || num1 >= 0))
		break;
	    if (*p == '}' && !except) {
		print("syntax error: missing {\n"); /* } */
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
		if (except)
		    clear_msg_list(list1);
		else
		    for (num1 = 0; num1 < msg_cnt; num1++)
			set_msg_bit(list1, num1);
		num1 = -1;
	    }
	} else if (!index(" \t`", *p))
	    break;
	if (*p)
	    skipspaces(1); /* don't make user type stuff squished together */
    }
    if (num1 >= 0 || except) {
	if (except)
	    /* { */ print("syntax error: unmatched }\n");
	else
	    print("syntax error: unfinished range\n");
	clear_msg_list(list1);
	return NULL;
    }
    return p;
}

/*
 * convert a message list to an ascii string.
 */
void
list_to_str(list, str)
char list[], *str;
{
    int n, m = -1;

    for (n = 0; n < msg_cnt; n++) {
	if (msg_bit(list, n)) {
	    if (m == -1)
		str += strlen(sprintf(str, "%d", (m = n) + 1 ));
	    continue;
	}
	if (m == -1)
	    continue;
	if (n - m > 2)
	    str += strlen(sprintf(str, "-%d", n));
	else if (n - m == 2)
	    str += strlen(sprintf(str, " %d", n));
	*str++ = ' ';
	m = -1;
    }
    if (m > -1 && m != n - 1) {
	if (n - m > 2)
	    *str++ = '-';
	else
	    *str++ = ' ';
	str += Strcpy(str, itoa(msg_cnt));
    }
    *str = 0;
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
register char *p, new_list[];
{
    register char *p2, **argv;
    int 	  argc;
    u_long	  save_flags = glob_flags;

    if (!(p2 = index(++p, '`'))) {
	print("unmatched backquote (`)\n");
	return NULL;
    }
    *p2 = 0;

    skipspaces(0);
    if (!*p) {
	print("Invalid null command\n");
	return NULL;
    }
    turnon(glob_flags, DO_PIPE);
    /* ignore sigs only because if user interrupts the do_command,
     * the longjmp will corrupt the stack and the program is hosed.
     * fix is to have layers of jmp_bufs to return to different levels.
     */
    turnon(glob_flags, IGN_SIGS);
    if (*p && (argv = make_command(p, TRPL_NULL, &argc)))
	(void) do_command(argc, argv, new_list);
    glob_flags = save_flags;
    *p2 = '`';
    return p2+1;
}
