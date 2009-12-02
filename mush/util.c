/* Code by Yves Arrouye <arrouye@debian.org> */

#include "mush.h"

#include <pwd.h>

/* Return 1 if we think file is someone's sysspooldir, 0 otherwise. */

issysspoolfile(file)
    const char* file; {
#ifdef HOMEMAIL
    char* p;
#else
    int l;
#endif

    struct passwd* pwd;

    if (!strcmp(file, sysspoolfile)) return 1;

#ifdef HOMEMAIL
    p = rindex(file, '/');
    if (p) ++p; else return 0;

#ifdef POP3_SUPPORT
    if (strcmp(p, POPMAILFILE) && strcmp(p, MAILFILE)) return 0;
#else
    if (strcmp(p, MAILFILE)) return 0;
#endif

    while (*--p == '/');

    while ((pwd = getpwent()) != 0) {
	if (strlen(pwd->pw_passwd) < 3) continue;
	if (!strncmp(pwd->pw_dir, file, p - file + 1)) break;
    }
    endpwent();

    return pwd ? 1 : 0;
#else
    l = strlen(MAILDIR);
    return !strncmp(file, MAILDIR, l)
#ifdef DEL_NOUSER
	&& (pwd = getpwnam(file + l + 1)) && strlen(pwd->pw_passwd) > 2
#endif
	;
#endif /* HOMEMAIL */

return 0;
}

