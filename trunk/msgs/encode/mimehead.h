/* mimehead.h	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifndef _MIMEHEAD_H_
#define _MIMEHEAD_H_

#include "osconfig.h"
#include "qp.h"
#include "base64.h"
#include "dynstr.h"
#include "mimetype.h"

extern int contains_nonascii P((const char *, const char *));
extern int is_structured_header P((const char *));

extern char *encode_header P((char *, char *, mimeCharSet));
extern char *encode_structured P((char *, long, mimeCharSet));
extern long encode_QCode P((const char *, long, mimeCharSet, struct dynstr *, long));
extern long encode_BCode P((const char *, long, mimeCharSet, struct dynstr *, long));

extern char *decode_header P((const char *, const char *));
extern void decode_token P((char *, long, mimeCharSet *, struct dynstr *));

#endif /* !_MIMEHEAD_H_ */
