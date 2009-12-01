#ifndef _UICOMP_H_
#define _UICOMP_H_

/*
 * $RCSfile: uicomp.h,v $
 * $Revision: 1.3 $
 * $Date: 1994/09/22 06:27:28 $
 * $Author: liblit $
 */

#include <general.h>
#include "uisupp.h"

enum uicomp_flavor
{
    uicomp_To,
    uicomp_Cc,
    uicomp_Bcc,
    uicomp_Unknown
};

extern char *uicomp_flavor_names[] /* = { "To:", "Cc:", "Bcc:", NULL } */ ;


struct Compose;


extern enum uicomp_flavor uicomp_seek_flavor P((char**));
extern char *uicomp_make_bland P((const char *));
extern enum uicomp_flavor uicomp_predominant_flavor P((char **[3]));

void   uicomp_vector_to_triple P((enum uicomp_flavor,       char **, enum uicomp_flavor *, char **[3]));
void   uicomp_string_to_triple P((enum uicomp_flavor, const char *,  enum uicomp_flavor *, char **[3]));
char **uicomp_triple_to_vector P((char **[3], unsigned *));
char  *uicomp_triple_to_string P((char **[3], unsigned *));
char  *uicomp_vector_to_string P((char **));

void uicomp_free_triple P((char **[3]));

void   uicomp_vector_to_compose P((char **, struct Compose *));
char **uicomp_compose_to_vector P((struct Compose *, unsigned *));

void uicomp_triple_to_compose P((char **[3], struct Compose *));
void uicomp_compose_to_triple P((struct Compose *, char **[3]));

zmBool uicomp_expand_triple P((char **[3], char **[3], zmBool, zmBool));
void uicomp_merge_triple P(( char **[3], struct Compose *));


#endif /* _UICOMP_H_ */
