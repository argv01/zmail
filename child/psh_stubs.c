#include "zmail.h"
#include <varargs.h>

int istool = 0;
char debug = 0;

#ifndef VUI
Widget hidden_shell = 0;
#endif /* VUI */

/* Insert an element at the "end" of a circular linked list */
void
insert_link(list, new)
struct link **list, *new;
{
    if (*list) {
	new->l_prev = (*list)->l_prev;
	(*list)->l_prev->l_next = new;
	(*list)->l_prev = new;
	new->l_next = *list;
    } else {
	new->l_prev = new;
	new->l_next = new;
	*list = new;
    }
}

/* Treat a circular linked list as a stack */
void
push_link(list, new)
struct link **list, *new;
{
    insert_link(list, new);
    *list = new;
}

/* Insert a link element in sorted order */
/* XXX NOTE: There is currenlty a difference in the compar functions
 *     required by insert_sorted_link and retrieve_link.  This should
 *     probably be corrected by changing the current retrieve_link to
 *     retrieve_named_link and adding a new retrieve_link that works
 *     like insert_sorted_link with respect to comparisons.
 */
void
insert_sorted_link(list, new, compar)
struct link **list, *new;
int (*compar)();
{
    struct link *tmp;
    int n;

    if (*list) {
	tmp = *list;
	do {
	    if ((n = (*compar)(tmp, new)) > 0)
		break;
	    tmp = tmp->l_next;
	} while (tmp != *list);
	if (tmp == *list)
	    if (n > 0)
		push_link(list, new);
	    else
		insert_link(&tmp, new);
	else
	    insert_link(&tmp, new);
    } else
	insert_link(list, new);
}

/* Remove an element from a circular linked list */
void
remove_link(list, link)
struct link **list, *link;
{
    if (link->l_next == link)
	if (link == *list)
	    *list = 0;
	else
	    error(ZmErrWarning, "link not in list");
    else {
	link->l_next->l_prev = link->l_prev;
	link->l_prev->l_next = link->l_next;
	if (*list == link)
	    *list = (*list)->l_next;
    }
    link->l_next = link->l_prev = 0; /* Sever the connection */
}

/* return nth element in a circular linked list */
struct link *
retrieve_nth_link(list, n)
struct link *list;
int n;
{
    struct link *tmp = list;

    if (list)
	do {
	    if (--n == 0)
		return tmp;
	    tmp = tmp->l_next;
	} while (tmp != list);
    return 0;
}

/* Search for a specified element in a circular linked list */
struct link *
retrieve_link(list, id, compar)
struct link *list;
char *id;
int (*compar)();
{
    struct link *tmp = list;

    if (!compar)
	compar = strcmp;

    if (list && id)
	do {
	    if (tmp->l_name && (*compar)(tmp->l_name, id) == 0)
		return tmp;
	    tmp = tmp->l_next;
	} while (tmp != list);
    return 0;
}

/* Print an error or other warning message.  First argument is the reason
 * for the message, second is printf formatting, rest are to be formatted.
 * If the reason is "Message", automatic appending of "\n" is supressed.
 */
void
/*VARARGS1*/
error(va_alist)
va_dcl
{
    PromptReason reason;
    char buf[BUFSIZ], *fmt;
    va_list args;

    va_start(args);
    reason = (PromptReason) va_arg(args, int);
    fmt = va_arg(args, char *);

#define VSprintf vsprintf
    VSprintf(buf, fmt, args);
    va_end(args);

    if (reason == SysErrWarning || reason == SysErrFatal)
#undef sprintf
	sprintf(buf+strlen(buf), ": %s", strerror(errno));

#ifdef GUI
    if (istool == 2)
	gui_error(reason, buf);
    else if (istool == 3) {
	fprintf(stderr, "%s%s%s",
	    (reason==ZmErrFatal || reason==ZmErrWarning)?
	    "Internal Error: " : "", buf, reason==Message? "" : "\n");
    } else
#endif /* GUI */
    print("%s%s%s",
	(reason==ZmErrFatal || reason==ZmErrWarning)? "Internal Error: " : "",
	    buf, reason==Message? "" : "\n");
    if (reason == SysErrFatal || reason == UserErrFatal)
	cleanup(-1);
    else if (reason == ZmErrFatal)
	cleanup(debug - 1); /* cause abort() if debugging is on */
}

void
gui_error(va_alist)
va_dcl
{
    error(va_alist);
}

void cleanup(sig)
{
    exit(sig);
}

#if defined(CURSES) || defined(GUI)
void
print (va_alist)
va_dcl
{
    va_list args;
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
    (void) vprintf(fmt, args);
    va_end(args);
}

#endif

#ifdef GUI
void
wprint (va_alist)
va_dcl
{
    va_list args;
    char *fmt;

    va_start(args);
    fmt = va_arg(args, char *);
    (void) vprintf(fmt, args);
    va_end(args);
}
#endif /* GUI */







#ifdef SYSV
getdtablesize()
{
    return 32;
}

#undef index
char *
index(s,c)
char *s;
int c;
{
    for (;;s++) {
	if (*s == c)
	    return s;
	if (!*s)
	    return 0;
    }
}

setreuid(r,e)
{
    fprintf(stderr, "(calling setuid(%d) instead of setreuid(%d,%d))\n",r,r,e);
    return setuid(r);
}

setregid(r,e)
{
    fprintf(stderr, "(calling setgid(%d) instead of setregid(%d,%d))\n",r,r,e);
    return setgid(r);
}

/* sys-v doesn't have normal sys_siglist */
char	*sys_siglist[] = {
/* no error */  "no error"
/* SIGHUP */	"hangup"
/* SIGINT */	"interrupt (rubout)"
/* SIGQUIT */	"quit (ASCII FS)"
/* SIGILL */	"illegal instruction (not reset when caught)"
/* SIGTRAP */	"trace trap (not reset when caught)"
/* SIGIOT */	"IOT instruction"
/* SIGEMT */	"EMT instruction"
/* SIGFPE */	"floating point exception"
/* SIGKILL */	"kill (cannot be caught or ignored)"
/* SIGBUS */	"bus error"
/* SIGSEGV */	"segmentation violation"
/* SIGSYS */	"bad argument to system call"
/* SIGPIPE */	"write on a pipe with no one to read it"
/* SIGALRM */	"alarm clock"
/* SIGTERM */	"software termination signal from kill"
/* SIGUSR1 */	"user defined signal 1"
/* SIGUSR2 */	"user defined signal 2"
/* SIGCLD */	"death of a child"
/* SIGPWR */	"power-fail restart"
};

#endif /* SYSV */
