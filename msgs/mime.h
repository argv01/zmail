/*                                  Copyright 1993 Z-Code Software Corp.
 * mime.h --
 *
 *	This is the header file for users of variables and
 *	procedures exported from mime.c.
 *	
 *						C.M. Lowery, 1993
 *	$Id: mime.h,v 2.32 2005/05/09 09:15:20 syd Exp $
 */


#ifndef _MIME_H
#define _MIME_H

#include "callback.h"
#include "mimetype.h"
#include <general.h>

extern int		IsAsciiSuperset P((mimeCharSet));
extern char		*GetDefaultEncodingStr();
extern int		init_mime();
extern int		out_charset_callback P((char *, ZmCallbackData));
extern int		in_charset_callback P((char *, ZmCallbackData));
extern int		display_charset_callback P((char *, ZmCallbackData));
extern int		printer_charset_callback P((char *, ZmCallbackData));
extern int		file_charset_callback P((char *, ZmCallbackData));
extern int		charset_callback P((char *, ZmCallbackData));
extern int		SortMimeEncodings();
extern int		SortMimeTypes();
extern void		SetCurrentCharSet P((mimeCharSet));
extern mimeCharSet	GetMimeCharSet P((const char *));
extern const char  	*GetMimeCharSetName P((mimeCharSet));
extern mimeEncoding	GetMimeEncoding();
extern mimePrimaryType	MimePrimaryType();
extern mimeSubType	MimeSubType();
extern mimePrimaryType	GetMimePrimaryType();
extern mimeSubType	GetMimeSubType();
extern mimeType		GetClosestMimeType();
extern mimeType		GetMimeType();
extern void		DeriveMimeInfo();
extern void		ValidateMimeInfo();

/*
 * C3_TRANSLATION_WANTED only checks if should translate or not.  Does 
 * not check if C3 actually knows about the character sets
 */
#define C3_TRANSLATION_WANTED(src, tgt) \
	(((src) != (tgt)) && \
	 (((src) != UsAscii) || !IsAsciiSuperset((tgt)) ))

#define C3_TRANSLATION_POSSIBLE(src, tgt) \
	(c3_is_known_to_c3((src)) && \
         c3_is_known_to_c3((tgt)))

#define C3_TRANSLATION_REQUIRED(src, tgt) \
	(C3_TRANSLATION_WANTED(src, tgt) && \
         C3_TRANSLATION_POSSIBLE(src, tgt))


#include "mmailext.h"

#endif /* _MIME_H */
/* DON'T ADD STUFF AFTER THIS #endif */
