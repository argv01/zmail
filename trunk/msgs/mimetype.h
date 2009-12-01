/*                                  Copyright 1993 Z-Code Software Corp.
 * mimetype.h --
 *
 *	This is the header file for users of mime variables.
 *	'extern' variables are defined in mime.c
 *
 *						C.M. Lowery, 1993
 *	$Id: mimetype.h,v 2.13 2005/05/09 09:15:20 syd Exp $
 */

#ifndef MIMETYPE_H
#define MIMETYPE_H

#include "callback.h"
#ifndef MAC_OS
#include "c3/c3.h"
#else /* MAC_OS */
#include "c3.h"
#endif /* MAC_OS */

#define MIME_VERSION "1.0"

#define EXTENSION_PREFIX "x-"

#define MAX_BOUNDARY_LEN 70

/* enum order is significant, see mimeTypeStr */
typedef enum {
UnknownMimeType,
IllegalMimeType,
ExtendedMimeType,
ApplicationOctetStream,
ApplicationPostscript,
AudioBasic,
ImageGif,
ImageJpeg,
MessageExternalBody,
MessageNews,
MessagePartial,
MessageRfc822,
MultipartAlternative,
MultipartAppledouble,
MultipartDigest,
MultipartEnabledMail,
MultipartEncrypted,
MultipartMixed,
MultipartParallel,
MultipartSigned,
TextPlain,
VideoMpeg,
NumMimeType /* counter, must be last */
} mimeType;

extern	char	*mimeTypeStr[(int)NumMimeType];

#define	MimeTypeStr(x) mimeTypeStr[(int)(x)]
#define DEFAULT_MIME_TYPE TextPlain

/* enum order is significant, see mimePrimaryTypeStr */
typedef enum {
UnknownMimePrimaryType,
IllegalMimePrimaryType,
ExtendedMimePrimaryType,
MimeApplication,
MimeAudio,
MimeImage,
MimeMessage,
MimeMultipart,
MimeText,
MimeVideo,
NumMimePrimaryType /* counter, must be last */
} mimePrimaryType;

extern	char	*mimePrimaryTypeStr[(int)NumMimePrimaryType];

#define	MimePrimaryTypeStr(x) mimePrimaryTypeStr[(int)(x)]
#define DEFAULT_MIME_PRIMARY_TYPE MimeApplication

/* enum order is significant, see mimeSubTypeStr */
typedef enum {
UnknownMimeSubType,
IllegalMimeSubType,
ExtendedMimeSubType,
MimeOctetStream,
MimePostscript,
MimeBasic,
MimeGif,
MimeJpeg,
MimeExternalBody,
MimeNews,
MimePartial,
MimeRfc822,
MimeAlternative,
MimeDigest,
MimeMixed,
MimeParallel,
MimeEnabledMail,
MimePlain,
MimeMpeg,
NumMimeSubType /* counter, must be last */
} mimeSubType;

extern	char	*mimeSubTypeStr[(int)NumMimeSubType];

#define	MimeSubTypeStr(x) mimeSubTypeStr[(int)(x)]

/* enum order is significant, see mimeEncodingStr */
typedef enum {
UnknownMimeEncoding,
IllegalMimeEncoding,
ExtendedMimeEncoding,
SevenBit,
EightBit,
Base64,
Binary,
QuotedPrintable,
NumMimeEncoding /* counter, must be last */
} mimeEncoding;

extern	char	*mimeEncodingStr[(int)NumMimeEncoding];

#define	MimeEncodingStr(x) mimeEncodingStr[(int)(x)]
#define DEFAULT_MIME_ENCODING SevenBit


#define mimeCharSet long

#define DEFAULT_MIME_CHAR_SET UsAscii

#define OverrideCharSet "__override_charset_label"

/* enum order is significant, see mimeTextParamStr */
typedef enum {
UnknownMimeTextParam,
IllegalMimeTextParam,
ExtendedMimeTextParam,
CharSetParam,
NumMimeTextParam /* counter, must be last */
} mimeTextParam;

extern	char	*mimeTextParamStr[(int)NumMimeTextParam];
#define	MimeTextParamStr(x) mimeTextParamStr[(int)(x)]

/* enum order is significant, see mimeExternalParamStr */
typedef enum {
UnknownMimeExternalParam,
IllegalMimeExternalParam,
ExtendedMimeExternalParam,
AccessTypeParam,
ExpirationParam,
SizeParam,
PermissionParam,
NameParam,
SiteParam,
DirParam,
ModeParam,
ServerParam,
SubjectParam,
NumMimeExternalParam /* counter, must be last */
} mimeExternalParam;

extern	char	*mimeExternalParamStr[(int)NumMimeExternalParam];
#define	MimeExternalParamStr(x) mimeExternalParamStr[(int)(x)]

#ifdef NOT_NOW
/* enum order is significant, see mimeAccessTypeStr */
typedef enum {
UnknownMimeAccessType,
IllegalMimeAccessType,
ExtendedMimeAccessType,
Ftp,
AnonFtp,
Tftp,
LocalFile,
Afs,
MailServer,
NumMimeAccessType /* counter, must be last */
} mimeAccessType;

extern	char	*mimeAccessTypeStr[(int)NumMimeAccessType];
#define	MimeAccessTypeStr(x) mimeAccessTypeStr[(int)(x)]
#endif /* NOT_NOW */

/* enum order is significant, see mimeGlobalParamStr */
typedef enum {
UnknownMimeGlobalParam,
IllegalMimeGlobalParam,
ExtendedMimeGlobalParam,
BoundaryParam,
GlobalNameParam,
NumMimeGlobalParam /* counter, must be last */
} mimeGlobalParam;

extern	char	*mimeGlobalParamStr[(int)NumMimeGlobalParam];
#define	MimeGlobalParamStr(x) mimeGlobalParamStr[(int)(x)]

#ifdef NOT_NOW
/* enum order is significant, see mimePartialParamStr */
typedef enum {
UnknownMimePartialParam,
IllegalMimePartialParam,
ExtendedMimePartialParam,
IdParam,
NumberParam,
TotalParam,
NumMimePartialParam /* counter, must be last */
} mimePartialParam;

extern	char	*mimePartialParamStr[(int)NumMimePartialParam];
#define	MimePartialParamStr(x) mimePartialParamStr[(int)(x)]
#endif /* NOT_NOW */

/* This is used to hold the parameters from the content-type message
 * header
 */
typedef struct mimeContentParams {
    char	**paramNames;
    char	**paramValues;
    int		numParamsUsed;
    int		numParamsAlloced;
} mimeContentParams;

/* Special hack to handle binhex content type appropriately, by
 * using binhex decoder.  This use of an encoding as a content-type
 * doesn't fit into the normal MIME model, but is used by some Mac
 * mailers, like Eudora and Mac Z-Mail
 */

#define ApplicationBinHexStr	"application/mac-binhex40"
#define BinHexStr		"x-binhex"

extern mimeCharSet	outMimeCharSet, inMimeCharSet,
  			displayCharSet, printerCharSet,
			fileCharSet, currentCharSet;
#endif
