#include <general.h>
#include <osconfig.h>

char *reverse P((char s[]));
/* remove newline and extra whitespace - return end */
char *no_newln P((char *p));
/* return first char in str2 that exists in str1 */
char *any P((const char *s1, const char *s2));
/* return first char in str2 that exists in str1 */
char *rany P((const char *s1, const char *s2));
int chk_two_lists P((const char *list1, const char *list2, const char *delimiters));
#ifndef HAVE_B_MEMFUNCS
#if !defined(bcopy) && !defined(bzero) && !defined(NO_DECLARE_BCOPY)
int bzero P((VPTR addr, int size));
int bcopy P((CVPTR from, VPTR to, int n));
#endif /* !bcopy && !bzero && !NO_DECLARE_BCOPY */
#endif /* !HAVE_B_MEMFUNCS */
/* do an atoi, but return the last char parsed */
char *my_atoi P((const char *p, int *val));
/* convert a vector of strings into one string */
char *argv_to_string P((register char *p, char **argv));
/* intelligent vector-to-string */
char *smart_argv_to_string P((char *buf, char **argv, const char *chars_to_escape));
char *Sprintf VP((char *buf, const char *fmt, ...));
