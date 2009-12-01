#ifndef INCLUDE_LICENSE_LSUTIL_H
#define INCLUDE_LICENSE_LSUTIL_H


#include <general.h>
#include "features.h"
#include "zctype.h"


#define PASSWORD_CHECK ((u_short)0xab21)
#define typeof_PASSWORD_CHECK u_short	/* for sony compiler bug */


void ls_long_to_key P((unsigned long, char[8]));
void ls_makekey P((const char *, const char *, const char *, char *));

void ls_pack   P((unsigned char[8], short,   Int32,   typeof_PASSWORD_CHECK));
void ls_unpack P((unsigned char[8], short *, Int32 *, typeof_PASSWORD_CHECK *));

#ifdef NETWORK_LICENSE
char *ls_make_tstamp P((long, Int32));
long ls_grok_tstamp P((const char *, Int32));
#endif /* NETWORK_LICENSE */

int AtoUL P((const char *, unsigned long *));
int ls_is_a_password P((const char *));

char *ls_delete_first_arg P((char **));
int ls_argvlen P((char **));
void ls_append_to_malloced_argv P((char ***, char *));
void ls_append_to_malloced_ulv P((unsigned long **, unsigned long));


#endif /* !INCLUDE_LICENSE_LSUTIL_H */
