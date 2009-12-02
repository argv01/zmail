/* @(#)fkeys.c		(c) copyright 10/18/86 (Dan Heller) */

#include "mush.h"

#define L(n)		KEY_LEFTFIRST+(n)-1
#define R(n)		KEY_RIGHTFIRST+(n)-1
#define F(n)		KEY_TOPFIRST+(n)-1
#define BREAK_KEY	KEY_TOPLAST

static int func_key();

Notify_value
fkey_interposer(client, event, arg, type)
Frame client;
Event *event;
Notify_arg arg;
Notify_event_type type;
{
    if ((event_is_key_left(event) || event_is_key_right(event) ||
	event_is_key_top(event)) &&
	event_is_down(event) && func_key(event_id(event)))
	    return NOTIFY_DONE;

    return notify_next_event_func(client, event, arg, type);
}

/*
 * Execute commands defined by a function key.
 * Left keys:
 * L1 = (null)  can't be set
 * L2 ... L10
 * Top function keys
 * F1 ... F9, BREAK/backspace (key not definable)
 * Right function keys
 * R1 ... R15
 * Usually, the last Function key displays the others' settings.
 */
static int
func_key(key)
register int key;
{
    register char **argv, *p;
    char buf[256];
    int n;

    if (key >= KEY_LEFTFIRST && key <= KEY_LEFTLAST)
	buf[0] = 'L', n = key - KEY_LEFTFIRST;
    else if (key >= KEY_TOPFIRST && key <= KEY_TOPLAST)
	buf[0] = 'F', n = key - KEY_TOPFIRST;
    else if (key >= KEY_RIGHTFIRST && key <= KEY_RIGHTLAST)
	buf[0] = 'R', n = key - KEY_RIGHTFIRST;
    (void) sprintf(buf+1, "%d", n+1);

    if (!(p = do_set(fkeys, buf))) {
	if (!chk_option("quiet", "fkey"))
	    wprint("Function key \"%s\" not set.\n", buf);
	return FALSE;
    }
    /* make_command will screw up "p", so copy it first */
    (void) strcpy(buf, p);
    Debug("(%s) \"%s\": ", key, p), turnon(glob_flags, CONT_PRNT);
    if (argv = make_command(buf, TRPL_NULL, &n))
	(void) do_command(n, argv, msg_list);
    return TRUE;
}
