#include "osconfig.h"

#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif /* HAVE_UTSNAME */
#ifdef HAVE_SYS_SYSTEMINFO_H
#include <sys/systeminfo.h>
#endif /* HAVE_SYS_SYSTEMINFO_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef HAVE_GETHOSTID

static unsigned long
_get_host_id()
{
#ifdef _SC_BSD_NETWORK
    if (sysconf(_SC_BSD_NETWORK) >= 0)
	return gethostid();
    else
	return 0;
#else /* _SC_BSD_NETWORK */
    return gethostid();
#endif /* _SC_BSD_NETWORK */
}

#else /* !HAVE_GETHOSTID */

#define _get_host_id()	0

#endif /* !HAVE_GETHOSTID */

#ifdef SI_HW_SERIAL

unsigned long
gethardwareid()
{
    unsigned long hid;
    int seriallen = 0;
    char buf[257];	/* Man page recommends this size */

    if ((seriallen = sysinfo(SI_HW_SERIAL, buf, 257)) > 1 &&
	seriallen < 258 && sscanf(buf, "%lu", &hid) == 1 && hid >= 100)
	    ;
    else
	hid = _get_host_id();
    return hid;
}

#else /* !SI_HW_SERIAL */

#ifdef HAVE_XUTSNAME	/* AIX */

unsigned long
gethardwareid()
{
    unsigned long hid;
    struct xutsname xname;

    if (unamex(&xname) >= 0)
	hid = xname.nid;
    else
	hid = _get_host_id();
    return hid;
}

#else /* !HAVE_XUTSNAME */

unsigned long
gethardwareid()
{
    return _get_host_id();
}

#endif /* !HAVE_XUTSNAME */
#endif /* SI_HW_SERIAL */
