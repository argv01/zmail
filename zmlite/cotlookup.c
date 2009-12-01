/*
 * Program for binary-searching an address file.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include <general/general.h>

#ifndef lint
static const char cotlookup_rcsid[] =
    "$Id: cotlookup.c,v 2.5 1995/04/23 01:49:30 bobg Exp $";
#endif /* lint */

char *ProgramName;

static void
sendtime_lookup(fp, probe)
    FILE *fp;
    char *probe;
{
    struct stat statbuf;
    long start, end, mid, mid1;
    int ch, cmp;

    if (fstat(fileno(fp), &statbuf)) {
	fprintf(stderr, "%s: Could not stat file (%s)\n",
		ProgramName, strerror(errno));
	return;
    }

    start = 0;
    end = statbuf.st_size;

    while (end > start) {
	mid1 = mid = (start + end) / 2;

      retry:
	fseek(fp, mid, SEEK_SET);
	if (mid > 0)
	    do {
		ch = fgetc(fp);
		++mid;
	    } while ((ch != EOF) && (ch != '\n'));
	if ((ch == EOF) || (mid > end)) {
	    if (mid1 == start)
		break;
	    mid = mid1 = (start + mid1) / 2;
	    goto retry;
	}
	cmp = match(fp, probe, 0); /* match must move the file pointer back */
	if (!cmp) {
	    break;
	} else if (cmp < 0) {
	    end = mid - 1;
	} else {
	    if (!(start = mid))
		++start;
	}
    }
    while (!match(fp, probe, 1)) {
	while (((ch = fgetc(fp)) != EOF) && (ch != '\n'))
	    putchar(ch);
	putchar('\n');
    }
}

main(argc, argv)
    int argc;
    char **argv;
{
    char *tmp = (char *) rindex(argv[0], '/');
    char *file, *probe;
    FILE *fp;
    int sendtime = 0, user = 0, c, retval = 0;
    int i = 1;

    if (tmp)
	ProgramName = tmp + 1;
    else
	ProgramName = argv[0];

    if (!(file = argv[i++])) {
	fprintf(stderr, "%s: Too few arguments\n", ProgramName);
	exit(1);
    }
    if (!(probe = argv[i++])) {
	fprintf(stderr, "%s: Too few arguments\n", ProgramName);
	exit(1);
    }

    if (!(fp = fopen(file, "r"))) {
	fprintf(stderr, "%s: Could not open \"%s\" (%s)\n",
		ProgramName, file, strerror(errno));
	exit(1);
    }

    sendtime_lookup(fp, probe);	/* this once was one of many modes */

    fclose(fp);
    exit(retval);
}

#define EOS1(c) ((c) == '\0')
#define EOS2(c) (((c) == EOF) \
		 || ((c) == '\n') \
		 || isspace(c) \
		 || ispunct(c))

int
match(fp, probe, prefix)
    FILE *fp;
    char *probe;
    int prefix;			/* prefix match considered exact */
{
    long pos = ftell(fp);
    int ch1, ch2, result;
    char *p = probe;

    while (1) {
	ch1 = *(p++);
	ch2 = fgetc(fp);
	if (EOS1(ch1)) {
	    if (EOS2(ch2)) {
		result = 0;
		break;
	    }
	    result = (prefix ? 0 : -1);
	    break;
	}
	if (EOS2(ch2)) {
	    result = 1;
	    break;
	}
	if (isascii(ch1) && isalpha(ch1) && isupper(ch1))
	    ch1 = tolower(ch1);
	if (isascii(ch2) && isalpha(ch2) && isupper(ch2))
	    ch2 = tolower(ch2);
	if (result = (ch1 - ch2))
	    break;
    }
    fseek(fp, pos, SEEK_SET);
    return (result);
}
