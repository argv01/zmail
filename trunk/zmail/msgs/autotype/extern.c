#ifdef SPTX21
#define _XOS_H_
#endif /* SPTX21 */

#include "osconfig.h"
#include "vars.h"
#include "zmail.h"		/* for zmVaStr() :-( */
#include "zmaddr.h"		/* for strs_from_program(), of all things */
#include "zmopt.h"
#include "zmstring.h"


const char *
autotype_via_extern(filename)
    const char *filename;
{
    const char *typer = value_of(VarAutotyper);
    char **results = NULL;
    static char *guess = NULL;

    if (guess) free(guess);
    
    if (typer) {
	int status = strs_from_program(zmVaStr("%s %s", typer, filename), &results);
	int length = vlen(results);

	if (!status && length == 1)
	    guess = savestr(results[0]);
	else
	    guess = NULL;
    } else
	guess = NULL;
    
    return guess;
}
