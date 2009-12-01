#include <ctype.h>
#include <dynstr.h>
#include <errno.h>
#include <excfns.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "fatal.h"
#include "files.h"
#include "options.h"


int
file_exists(const char *name)
{
    struct stat junk;

    return !stat(name, &junk);
}


void
filename_disarm(char *name)
{
    char *sweep;
    const char unsafeChars[] = "~`'\"!*(){}[]|<>\\/?&;^$";

    if (name[0] == '.') name[0] = '_';
    
    for (sweep = name; *sweep; sweep++)
	if (!isgraph(*sweep) || strchr(unsafeChars, *sweep))
	    *sweep = '_';
}


void
filename_uniqify(char *name)
{
    if (file_exists(name)) {
	
	const unsigned length = strlen(name);
	char *suffix = &name[length];
	unsigned long distinguisher = 0;

	do
	    sprintf(suffix, ".%u", ++distinguisher);
	while (file_exists(name) && distinguisher);

    }
}


int
mkdir_exist(const char *name, mode_t mode)
{
    struct stat status;

    if (stat(name, &status)) {
	errno = 0;
	return mkdir(name, mode);
    } else
	if (S_ISDIR(status.st_mode))
	    return 0;
	else {
	    errno = ENOTDIR;
	    return -1;
	}
}


int
mkdirhier(char *name, mode_t mode)
{
    if (name && *name) {
	char *segment = name + 1;

	errno = 0;

	while (!errno && *segment && (segment = strchr(segment+1, '/'))) {
	    *segment = '\0';
	    mkdir_exist(name, mode);
	    *segment = '/';
	}
	if (!errno) mkdir_exist(name, mode);
	return errno;
    } else
	return 0;
}


FILE *
popen_delivery()
{
    FILE *connection;
    
    if (deliveryCmd)
	connection = epopen(deliveryCmd, "w", WHERE(popen_delivery));
    else {
	struct dynstr collect;
	struct dynstr user;

	dynstr_Init(&collect);
	dynstr_Set(&collect, "/bin/mail -s -d \"$USER\"");

	dynstr_Init(&user);
	dynstr_Append(&user, "USER=");
	dynstr_Append(&user, getpwuid(getuid())->pw_name);
	eputenv(dynstr_Str(&user), WHERE(popen_delivery));

	connection = epopen(dynstr_Str(&collect), "w", WHERE(popen_delivery));

	dynstr_Destroy(&user);
	dynstr_Destroy(&collect);
    }
    return connection;
}
