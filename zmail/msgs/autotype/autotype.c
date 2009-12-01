#include "autotype.h"
#include "osconfig.h"

#if !defined(_WINDOWS) && !defined(MAC_OS)
#include "extern.h"
#ifdef OZ_DATABASE
#include "oz.h"
#endif /* OZ_DATABASE */
#endif /* !_WINDOWS && !MAC_OS */
#include "analysis.h"


typedef const char * (*Autotyper)P((const char *));

Autotyper Autotypers[] = {
#if !defined(_WINDOWS) && !defined(MAC_OS)
#ifdef OZ_DATABASE
    autotype_via_oz,
#endif /* OZ_DATABASE */
    autotype_via_extern,
#endif /* !_WINDOWS && !MAC_OS */
#ifdef _WINDOWS
    /* nothing yet */
#endif /* _WINDOWS */
#ifdef MAC_OS
    autotype_via_mactype,
#endif /* MAC_OS */
    autotype_via_analysis
};


const char *
autotype(filename)
    const char *filename;
{
    unsigned int typer = 0;
    const char *guess = 0;

    get_attach_keys(0, 0, 0);	/* make sure config has been read in */
    for (typer = 0; !guess && typer < sizeof(Autotypers)/sizeof(*Autotypers); typer++)
	guess = Autotypers[typer](filename);

    return guess ? guess : "application/octet-stream";
}
