#include "attach.h"
#include "mimetype.h"
#include "partial.h"
#include "vars.h"
#include "zclimits.h"
#include "zcunix.h"		/* for stat */
#include "zm_ask.h"
#include "zmail.h"		/* for istool */
#include "zmcomp.h"
#include "zmopt.h"


/* integer division with rounding, protected from possible overflow */
static unsigned long
safe_div(numerator, denominator)
    register const unsigned long numerator;
    register const unsigned long denominator;
{
    register const unsigned long rounder = denominator - 1;
    register unsigned long safe_numerator;

    if (rounder <= ULONG_MAX - numerator)
	safe_numerator = numerator + rounder;
    else
	safe_numerator = ULONG_MAX;

    return safe_numerator / denominator;
}


/* length of a named file, in bytes */
static unsigned long
filename_size(filename)
    const char *filename;
{
    struct stat status;

    if (stat(filename, &status))
	return 0;
    else
	/* watch out for negative-length files (!) */
	return MAX(status.st_size, 0);
}


unsigned long
compose_guess_size(composition)
    const struct Compose *composition;
{
    unsigned long total = safe_div(filename_size(composition->edfile), 1024);
    const struct Attach *attachment;

    if (attachment = composition->attachments) do {
	unsigned long additional = safe_div(filename_size(attachment->a_name), 1024);

	if (!ci_strcmp(attachment->encoding_algorithm, MimeEncodingStr(Base64)))
	    /* overflow here possible but very unlikely */
	    additional = additional / 3 * 4;

	/* overflow here more likely, so be careful */
	if (ULONG_MAX - total > additional)
	    total += additional;
	else
	    return ULONG_MAX;
    } while ((attachment = (const struct Attach *) attachment->a_link.l_next) != composition->attachments);

    return total;
}




#ifdef PARTIAL_SEND

#ifdef HAVE_STDLIB_H
#include <stdlib.h>		/* for atoi() or strtoul() */
#else /* !HAVE_STDLIB_H */
extern int atoi P((const char *));
extern unsigned long int strtoul P((const char *, char **, int));
#endif /* HAVE_STDLIB_H */

static unsigned long
count_of(variable)
    const char *variable;
{
    const char * const value = value_of(variable);
    return (value && *value) ?
#ifdef HAVE_STRTOUL
	strtoul(value, 0, 10)
#else /* !HAVE_STRTOUL */
	atoi(value)
#endif /* !HAVE_STRTOUL */
	: 0;
}


static enum AskAnswer
ask_partial_confirm(estimate, suggested, splitsize)
    unsigned long estimate;
    unsigned long suggested;
    unsigned long *splitsize;
{
    enum AskAnswer answer = ask(SendSplit, istool ?
				zmVaStr("%s\n\n%s\n\n%s", 
					catgets(catalog, CAT_MSGS, 965, "\
This message is very large (approximately %lu K).\n\
Some mail systems can not receive messages of this\n\
size."), 
					catgets(catalog, CAT_MSGS, 966, "\
Choose Send Split below to send the message in\n\
approximately %lu pieces of %lu K each. When the\n\
pieces have been delivered to the recipients they\n\
will be reassembled."), 
					catgets(catalog, CAT_MSGS, 967, "\
Choose Send Whole below to send the file as is.\n\
Choose Cancel to cancel sending the message.")) :
				catgets(catalog, CAT_MSGS, 968, "\
Message is roughly %ld kilobytes long.\n\
Split it into about %ld fragments of %ld kilobytes each?"),
				estimate,
				safe_div(estimate, suggested),
				suggested);

    switch (answer) {
    case AskYes:
	*splitsize = count_of(VarSplitSize);
	if (!*splitsize) *splitsize = count_of(VarSplitLimit);
	return AskYes;
    case AskNo:
	*splitsize = 0;
	return AskYes;
    default:
	return AskNo;
    }
}


enum AskAnswer
partial_confirm(composition)
    struct Compose *composition;
{
    const char *splitter;
    
    if (composition->splitsize || !(splitter = value_of(VarSplitSendmail)) || !*splitter)
	return AskYes;
    else {
	unsigned long estimate;
	unsigned long limit = count_of(VarSplitLimit);
	unsigned long splitsize = count_of(VarSplitSize);
	if (!limit) limit = splitsize;
	if (!splitsize) splitsize = limit;

	if (splitsize && limit && (estimate = compose_guess_size(composition)) > limit)
	    if (chk_option(VarVerify, "split_send"))
#ifdef PARTIAL_SEND_DIALOG
		if (istool)
		    return gui_partial_confirm(estimate, splitsize, &composition->splitsize);
		else
#endif /* PARTIAL_SEND_DIALOG */
		    return ask_partial_confirm(estimate, splitsize, &composition->splitsize);
	    else
		composition->splitsize = splitsize;

	return AskYes;
    }
}

#endif /* PARTIAL_SEND */
