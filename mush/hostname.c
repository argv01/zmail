/* hostname.c	from init.c (c) copyright 1986 (Dan Heller) */

#include "mush.h"
#include <pwd.h>

#if defined(BSD) || defined(HPUX) || defined(IRIX4) || defined(__linux__)
#include <netdb.h>
#endif /* BSD || HPUX || IRIX4 || __linux__ */

#if defined(SYSV) && !defined(HPUX) && !defined(IRIX4)
#include <sys/utsname.h>
#endif /* SYSV && !HPUX && !IRIX4 */

void
init_host()
{
#if defined(SYSV) && !defined(HPUX) && !defined(IRIX4) && !defined(__linux__)
    struct utsname ourhost;
#else
    char ourhost[128];
#endif /* SYSV && !HPUX && !IRIX4 */
    register const char *p;
    struct passwd 	*entry;
    int			cnt;
#if defined(BSD) || defined(HPUX) || defined(IRIX4) || defined(__linux__)
    struct hostent 	*hp;
#endif /* BSD || HPUX || IRIX4 */

#if defined(BSD) || defined(HPUX) || defined(IRIX4) || defined(__linux__)
    (void) gethostname(ourhost, sizeof ourhost);
    if (!(hp = gethostbyname(ourhost))) {
	if (ourname = (char **)calloc((unsigned)2, sizeof (char *)))
	    strdup(ourname[0], ourhost);
    } else {
	int n = -1;
	cnt = 2; /* 1 for ourhost and 1 for NULL terminator */

        for (p = hp->h_name; p && *p; p = hp->h_aliases[++n])
            if (strcmp(ourhost, p)) /* if host name is different */
                cnt++;

        if (ourname = (char **)malloc((unsigned)cnt * sizeof (char *))) {
            n = -1;
            cnt = 0;
            ourname[cnt++] = savestr(ourhost);
            for (p = hp->h_name; p && *p; p = hp->h_aliases[++n])
                if (strcmp(ourhost, p)) /* if host name is different */
                    ourname[cnt++] = savestr(p);
            ourname[cnt++] = NULL;
        }
    }
#else
#ifdef SYSV
    if (ourname = (char **)calloc((unsigned)2, sizeof (char *))) {
	if ((uname (&ourhost) >= 0) && (*ourhost.nodename))
	    ourname[0] = savestr(ourhost.nodename);
	else {
	    /* Try to use uuname -l to get host's name if uname didn't work */
	    char buff[50];
	    char *p;
	    FILE *F;

	    if (F = popen("exec uuname -l", "r")) {
		if ((fgets(buff, sizeof buff, F) == buff) &&
			(p = strchr(buff, '\n'))) {
		    *p = '\0';		/* eliminate newline */
		    ourname[0] = savestr (buff);
		}
	    (void)pclose(F);
	    }
	}
    }
#endif /* SYSV */
#endif /* BSD || HPUX || IRIX4 */
}

