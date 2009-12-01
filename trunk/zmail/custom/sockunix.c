#include <zctype.h>
#include <zcsock.h>
#include <zcunix.h>
#include <zcalloc.h>
#include <zcsig.h>
#include <general.h>
#include <except.h>

void *
sock_Read(sock, len, retlen)
sock_t sock;
int len;
int *retlen;
{
    static char *buf;
    static int maxlen = 1;

    *retlen = -1;
    if (!buf) {
	buf = malloc(maxlen);
	if (!buf)
	    return NULL;
    }
    if (len > maxlen) {
	maxlen = len;
	buf = realloc(buf, maxlen);
	if (!buf)
	    return NULL;
    }
    *retlen = read(sock, buf, len);
    if (*retlen <= 0)
	return NULL;
    return (void *)buf;
}

int old_alarm_time;
RETSIGTYPE (*old_alarm_fun)();

static
RETSIGTYPE
alarm_hook(sig)
int sig;
{
    RAISE(sock_EXC_CANCELLED, NULL);
}

void
sock_SetTimeout(sock, tmout)
sock_t sock;
int tmout;
{
    old_alarm_time = alarm((unsigned) tmout);
    old_alarm_fun = signal(SIGALRM, alarm_hook);
}

void
sock_ClearTimeout(sock)
sock_t sock;
{
    alarm(old_alarm_time);
    signal(SIGALRM, old_alarm_fun);
}
