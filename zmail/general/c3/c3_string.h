#ifndef __c3_string_include__
#define __c3_string_include__

#ifndef NOT_ZMAIL
#include "osconfig.h"

#include "bfuncs.h"
#include "strcase.h"
#include "zmstring.h"

#define _c3_copy_memory(t, s, l)	memcpy((t), (s), (l))
#define _c3_zero_memory(p, l)		memset((p), 0, (l))
#define strcasecmp(s1, s2)		ci_strcmp((s1), (s2))
#define strncasecmp(s1, s2, n)		ci_strncmp((s1), (s2), (n))

#ifndef HAVE_STRDUP
#if !defined(strdup)
#define strdup savestr
#endif
#endif /* HAVE_STRDUP */

#else /* NOT_ZMAIL */
#include <string.h>
/* For all function declarations, if ANSI then use a prototype. */
								  
#if __STDC__                                                      
#define P(args)  args
#else  /* !__STDC__ */
#define P(args)  ()
#endif  /* STDC */

#ifdef UNIX
#include <strings.h>
#endif /* UNIX */


extern
void
bcopy(
    char *b1,
    char *b2,
    int length
);

extern
void
bzero(
      char *b, int length
);

extern
void
_c3_zero_memory(
    void *p,
    size_t length
)
{
    bzero((char *) p, length);
}

extern
void
_c3_copy_memory(
    void *tgt,
    void *src,
    size_t length
)
{
    bcopy(src, tgt, (int) length);
}


extern int
strncasecmp(char *s1, char *s2, int n);

extern char *
rindex(char *s, char c);

#endif /* NOT_ZMAIL */

#ifdef UNIX

#define COPY_MEMORY(tgt, src, no_bytes) \
    bcopy((src), (tgt), (int) (no_bytes))

#else /* !UNIX TODO test for Mac! */

#define COPY_MEMORY(tgt, src, no_bytes) \
    memcpy((tgt), (const char *) (src), (size_t) (no_bytes));

#endif /* !UNIX */

#ifdef THINK_C
# ifdef strdup
#  undef strdup
# endif /* strdup */
char *
strdup(
        const char *str
)
{
    char *m;
    int slen = strlen(str);

    m = malloc(slen+1);
    if (m == NULL)
        return NULL;
    _c3_copy_memory(m, (char *) str, (size_t) (slen+1));
    return m;
}
#endif /* THINK_C */

#endif /* __c3_string_include__ */
