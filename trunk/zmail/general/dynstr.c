/*
 * $RCSfile: dynstr.c,v $
 * $Revision: 2.22 $
 * $Date: 1995/07/14 04:11:53 $
 * $Author: schaefer $
 */

#include <dynstr.h>
#include "excfns.h" 

#ifndef lint
static const char dynstr_rcsid[] =
    "$Id: dynstr.c,v 2.22 1995/07/14 04:11:53 schaefer Exp $";
#endif /* lint */

#undef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define EXTRA (64)

static const char TheEmptyString[] = "";

void
dynstr_Init(d)
    struct dynstr *d;
{
    d->used = d->allocated = 0;
    d->strng = (char *) TheEmptyString;
}

void
dynstr_InitFrom(d, str)
    struct dynstr *d;
    char *str;
{
    d->used = d->allocated = 1 + strlen(str);
    d->strng = str;
}

static void
grow(d, newsize)
    struct dynstr *d;
    int newsize;
{
    if (d->allocated) {
	d->strng = (char *) erealloc(d->strng,
				     d->allocated = newsize, "dynstr");
    } else {
	d->strng = (char *) emalloc(d->allocated = newsize, "dynstr");
	d->strng[0] = '\0';
	d->used = 1;
    }
}

void
dynstr_Set(d, str)
    struct dynstr *d;
    const char *str;
{
    if (str) {
	int slen;

	if (dynstr_EmptyP(d) && (!*str))
	    return;
	slen = strlen(str) + 1;
	if (slen > d->allocated)
	    grow(d, EXTRA + slen);
	d->used = slen;
	strcpy(d->strng, str);
    } else {
	if (d->allocated) {
	    free(d->strng);
	    d->strng = (char *) TheEmptyString;
	    d->allocated = 0;
	    d->used = 0;
	}
    }
}

void
dynstr_Append(d, str)
    struct dynstr *d;
    const char *str;
{
    int slen;

    if (!str)
	return;
    if ((slen = strlen(str)) <= 0)
	return;
    if ((!(d->allocated))
	|| ((d->used + slen) > d->allocated))
	grow(d, EXTRA + d->used + slen);
    strcpy(d->strng + d->used - 1, str);
    d->used += slen;
    d->strng[d->used - 1] = '\0';
}

void
dynstr_AppendN(d, str, n)
    struct dynstr *d;
    const char *str;
    int n;
{
    int actual = n, i;

    for (i = 0; i < n; ++i) {
	if (str[i] == '\0') {
	    actual = i;
	    break;
	}
    }
    if (actual <= 0)
	return;
    if ((!(d->allocated))
	|| ((d->used + actual) > d->allocated))
	grow(d, EXTRA + d->used + actual);
    strncpy(d->strng + d->used - 1, str, (size_t) actual);
    d->used += actual;
    d->strng[d->used - 1] = '\0';
}

int
dynstr_AppendChar(d, c)
    struct dynstr *d;
    int c;
{
    if ((!(d->allocated))
	|| ((d->used + 1) > d->allocated))
	grow(d, EXTRA + d->used + 1);
    d->strng[d->used - 1] = (char) c;
    d->strng[(d->used)++] = '\0';
    return (c);
}

int
dynstr_Chop(d)
    struct dynstr *d;
{
    char result = d->strng[--(d->used) - 1];
    
    d->strng[d->used - 1] = '\0';
    return (result);
}

void
dynstr_ChopN(d, n)
    struct dynstr *d;
    unsigned n;
{
    d->used -= n;    
    d->strng[d->used - 1] = '\0';
}

void
dynstr_KeepN(d, n)
    struct dynstr *d;
    unsigned n;
{
    d->used = n + 1;    
    d->strng[d->used - 1] = '\0';
}

void
dynstr_Destroy(d)
    struct dynstr *d;
{
    if (d->allocated)
	free(d->strng);
}

int
dynstr_Length(d)
    const struct dynstr *d;
{
    return (d->allocated ? (d->used - 1) : 0);
}

#if !defined(SAFE_BCOPY) && !defined(HAVE_MEMMOVE)
void
safe_bcopy(src, dest, n)
    char *src, *dest;
    int n;
{
    if (dest == src)
	return;
    if (dest > src) {
	char *s = src + n;

	dest += n;
	while (s > src)
	    *(--dest) = *(--s);
    } else {
	int i;

	for (i = 0; i < n; ++i)
	    *(dest++) = *(src++);
    }
}
#endif /* !SAFE_BCOPY && !HAVE_MEMMOVE */

static char *
safe_strcpy(dest, src)
    char *dest, *src;
{
    safe_bcopy(src, dest, strlen(src) + 1);
    return (dest);
}

void
dynstr_ReplaceN(d, start, len, str, n)
    struct dynstr *d;
    int start, len;
    const char *str;
    int n;
{
    int i;

    /* determine actual # of bytes to copy */
    for (i = 0; i < n; ++i) {
	if (!str[i])
	    break;
    }
    n = i;

    if (n == len) {
	strncpy(d->strng + start, str, (size_t) len);
    } else if (n < len) {
	strncpy(d->strng + start, str, (size_t) n);
	safe_strcpy(d->strng + start + n, d->strng + start + len);
	d->used -= (len - n);
    } else {
	int need = d->used + n - len;

	if ((!(d->allocated)) || (need > d->allocated)) {
	    grow(d, EXTRA + need);
	}
	safe_strcpy(d->strng + start + n, d->strng + start + len);
	strncpy(d->strng + start, str, (size_t) n);
	d->used += (n - len);
    }
}

void
dynstr_ReplaceChar(d, start, len, c)
    struct dynstr *d;
    int start, len;
    int c;
{
    char ch = (char) c;

    dynstr_ReplaceN(d, start, len, &ch, 1);
}

void
dynstr_Replace(d, start, len, str)
    struct dynstr *d;
    int start, len;
    const char *str;
{
    dynstr_ReplaceN(d, start, len, str, strlen(str));
}

char *
dynstr_GiveUpStr(d)
    struct dynstr *d;
{
    if (d->allocated)
	return (d->strng);
    grow(d, EXTRA);
    return (d->strng);
}
