/* Copyright (c) 1993 Z-Code Software Corp. */
/*
 * mime.c --
 *	 
 *	 This file implements special-purpose functions related to MIME.
 *	 
 *						C.M. Lowery, 1993
 */

#ifndef lint
static char	mime_rcsid[] = "$Id: mime.c,v 2.78 2005/05/31 07:36:42 syd Exp $";
#endif

#if defined(DARWIN)
#include <libgen.h>
#endif

#include "zmail.h"
#include "mime.h"
#include "catalog.h"
#include "strcase.h"
#include "zmcomp.h"
/* XXX ifdef out for !C3 */
#ifndef MAC_OS
#include "c3/c3_trans.h"
#else /* MAC_OS */
#include "c3_trans.h"
#endif

/*
 *	Global data types and variables
 */

/* 
 * MIME types and subtypes.  strings in incoming/outgoing messages 
 * are not case sensitive.
 */

/* str order is significant, see mimeType */
/* Also, if any of these change, be sure to change fileTypeStrs below */
char	*mimeTypeStr[NumMimeType] = {
    "unknown-mime-type",
    "illegal-mime-type",
    "extended-mime-type",
    "application/octet-stream",
    "application/postscript",
    "audio/basic",
    "image/gif",
    "image/jpeg",
    "message/external-body",
    "message/news",
    "message/partial",
    "message/rfc822",
    "multipart/alternative",
    "multipart/appledouble",
    "multipart/digest",
    "multipart/enabled-mail",
    "multipart/encrypted",
    "multipart/mixed",
    "multipart/parallel",
    "multipart/signed",
    "text/plain",
    "video/mpeg"
    };

/* str order is significant, see mimePrimaryType */
char	*mimePrimaryTypeStr[NumMimePrimaryType] = {
    "unknown-mime-primary-type",
    "illegal-mime-primar-type",
    "extended-mime-primary-type",
    "application",
    "audio",
    "image",
    "message",
    "multipart",
    "text",
    "video"
    };

/* str order is significant, see mimeSubType */
/* Also, if any of these change, be sure to change fileTypeStrs below */
char	*mimeSubTypeStr[NumMimeSubType] = {
    "unknown-mime-sub-type",
    "illegal-mime-sub-type",
    "extended-mime-sub-type",
    "octet-stream",
    "postscript",
    "basic",
    "gif",
    "jpeg",
    "external-body",
    "news",
    "partial",
    "rfc822",
    "alternative",
    "digest",
    "mixed",
    "parallel",
    "enabled-mail",
    "plain",
    "mpeg"
    };

/* str order is significant, see mimeEncoding */
char	*mimeEncodingStr[NumMimeEncoding] = {
    "unknown-encoding",
    "illegal-encoding",
    "extended-encoding",
    "7bit",
    "8bit",
    "base64",
    "binary",
    "quoted-printable"
    };

/* XXX this is a gross special case to handle portable newlines correctly,
 * doing encoding/decoding and CRLF translation at the same time
 * This string, of course, should never actually go out in a message
 */
#define BASE64_CRLF "base64-CRLF"
#if 0
    "unknown-character-set",
    "illegal-character-set",
    "extended-character-set",
    "us-ascii",
    "iso-8859-1",
    "iso-8859-2",
    "iso-8859-3",
    "iso-8859-4",
    "iso-8859-5",
    "iso-8859-6",
    "iso-8859-7",
    "iso-8859-8",
    "iso-8859-9",
    "iso-2022-jp"
    };
#endif

/* MIME content-type parameters.  parameter name is not case sensitive.
 * Value is case-sensitive unless otherwise indicated 
 */

/* str order is significant, see mimeTextParam */
char	*mimeTextParamStr[NumMimeTextParam] = {
    "unknown-TextParam",
    "illegal-TextParam",
    "extended-TextParam",
    "charset"			/* value not case sensitive */
    };

/* str order is significant, see mimeExternalParam */
char	*mimeExternalParamStr[NumMimeExternalParam] = {
    "unknown-external-param",
    "illegal-external-param",
    "extended-external-param",
    "access-type",			/* value not case sensitive */
    "expiration",
    "size",
    "permission",			/* value not case sensitive */
    "name",
    "site",
    "dir",
    "mode",				/* value not case sensitive */
    "server",
    "subject"
    };

#ifdef NOT_NOW
/* str order is significant, see mimeAccessType */
char	*mimeAccessTypeStr[NumMimeAccessType] = {
    "unknown-access-type",
    "illegal-access-type",
    "extended-access-type",
    "ftp",
    "anon-ftp",
    "tftp",
    "local-file",
    "afs",
    "mail-server"
    };
#endif /* NOT_NOW */

/* str order is significant, see mimeGlobalParam */
char	*mimeGlobalParamStr[NumMimeGlobalParam] = {
    "unknown-GlobalParam",
    "illegal-GlobalParam",
    "extended-GlobalParam",
    "boundary",
    "name"
    };

#ifdef NOT_NOW
/* str order is significant, see mimePartialParam */
char	*mimePartialParamStr[NumMimePartialParam] = {
    "unknown-PartialParam",
    "illegal-PartialParam",
    "extended-PartialParam",
    "id",
    "number",
    "total"
    };
#endif /* NOT_NOW */

typedef struct stringPair {
    char	*fileTypeStr;
    char	*ourTypeStr;
} stringPair;

mimeCharSet	outMimeCharSet = DEFAULT_MIME_CHAR_SET,
		inMimeCharSet = DEFAULT_MIME_CHAR_SET,
		displayCharSet = DEFAULT_MIME_CHAR_SET,
		printerCharSet = DEFAULT_MIME_CHAR_SET,
		fileCharSet = DEFAULT_MIME_CHAR_SET,
		currentCharSet;

/*
 *	Forward declarations
 */

int		IsAsciiSuperset();

/*
 *	Functions
 */

/*
 *-------------------------------------------------------------------------
 * SetCurrentCharSet --
 *	Set the "current" character set.  This is a hack.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Assign the appropriate character set to globals currentCharSet
 *-------------------------------------------------------------------------
 */
void
SetCurrentCharSet(charset)
mimeCharSet	charset;
{
    currentCharSet = charset;
}

/*
 *-------------------------------------------------------------------------
 *
 *  IsAsciiSuperset --
 *	Check whether the given character set is a superset of ASCII.
 *
 *  Results:
 *	Returns appropriate boolean value.  Defaults to false if unknown.
 *
 *  Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
 /* XXX needs some work */
int
IsAsciiSuperset(charSet)
mimeCharSet	charSet;    
{
    if ((charSet == UsAscii) ||
	((charSet >= Iso_8859_1) && (charSet <= Iso_8859_9)) ||
	(charSet == X_Macintosh))
	return(TRUE); 
    return(FALSE);
}

int
init_mime()
{
    SetCurrentCharSet(displayCharSet);
    return 0;
}


/* Get our global character sets */		/* XXX BLEAH! */
int
out_charset_callback(fixedData, eventData)
     char          *fixedData;
     ZmCallbackData eventData;
{

    outMimeCharSet = (eventData->xdata && *((char *)eventData->xdata)) ?
	GetMimeCharSet((char *)eventData->xdata) :
	    Iso_8859_1;
  
    return 0;
}
int
in_charset_callback(fixedData, eventData)
     char          *fixedData;
     ZmCallbackData eventData;
{
    inMimeCharSet = (eventData->xdata && *((char *)eventData->xdata)) ?
	GetMimeCharSet((char *)eventData->xdata) :
	    DEFAULT_MIME_CHAR_SET;
  
    return 0;
}
int
display_charset_callback(fixedData, eventData)
     char          *fixedData;
     ZmCallbackData eventData;
{
#ifdef NOT_YET
    mimeCharSet oldCharSet = displayCharSet;
#endif /* NOT_YET */

    displayCharSet = (eventData->xdata && *((char *)eventData->xdata)) ?
      GetMimeCharSet((char *)eventData->xdata) :
          DEFAULT_MIME_CHAR_SET;
    SetCurrentCharSet(displayCharSet);
#ifndef C3
    set_var(VarOutgoingCharset, "=", GetMimeCharSetName(displayCharSet));
#endif /* !C3 */

#ifdef NOT_YET
    if (oldCharSet != displayCharSet) {
	/* Flush the message header cache of each message -- there is no
	 * call for this at the present time, which is why this is ifdef
	 */
	messageStore_Reset();	/* This is not the right call */
# ifdef GUI
	/* Do the same for the GUI's summary cache */
	gui_flush_hdr_cache(current_folder);
	gui_redraw_hdrs(current_folder, NULL_GRP);
# endif /* GUI */
    }
#endif /* NOT_YET */

    return 0;
}

int
printer_charset_callback(fixedData, eventData)
     char          *fixedData;
     ZmCallbackData eventData;
{
    if (eventData->event == ZCB_VAR_UNSET) {
	printerCharSet = displayCharSet;
	return;
    }

    printerCharSet = (eventData->xdata && *((char *)eventData->xdata)) ?
	GetMimeCharSet((char *)eventData->xdata) :
	    displayCharSet;

    return 0;
}
int
file_charset_callback(fixedData, eventData)
     char          *fixedData;
     ZmCallbackData eventData;
{
    if (eventData->event == ZCB_VAR_UNSET) {
	fileCharSet = displayCharSet;
	return;
    }

    fileCharSet = (eventData->xdata && *((char *)eventData->xdata)) ?
	GetMimeCharSet((char *)eventData->xdata) :
	    displayCharSet;

    return 0;
}
int
charset_callback(fixedData, eventData)
     char          *fixedData;
     ZmCallbackData eventData;
{
    if (eventData->event == ZCB_VAR_SET) {
	set_var(VarOutgoingCharset, "=", (char *)(eventData->xdata));
	set_var(VarDisplayCharset, "=", (char *)(eventData->xdata));
	set_var(VarFileCharset, "=", (char *)(eventData->xdata));
	un_set(&set_options, VarTextpartCharset);
	eventData->event = ZCB_CANCEL;
    }
    return 0;
}


/*
 *-------------------------------------------------------------------------
 *
 *  DeriveMimeInfo --
 *	Examine the raw strings loaded into the attach data structure,
 *	determine the relevant mime information, and insert it into the
 *	appropriate fields.
 *
 *  Results:
 *	Returns nothing.
 *
 *  Side effects:
 *	None.
 *-------------------------------------------------------------------------
 */

void
DeriveMimeInfo(attach)
    Attach      *attach;	/* the attachment to examine */
{
    char	*paramValue;

    if (attach)	{
	if (attach->mime_content_type_str)
	    attach->orig_mime_content_type_str = 
		savestr(attach->mime_content_type_str);
	ParseContentParameters(attach->mime_content_type_str,
			       &(attach->content_params));
	attach->mime_type = GetMimeType(attach->mime_content_type_str);
	
	/* The content_type field may be used in various places; it is
	 * certainly used in load_attachments
	 * 
	 * The data_type field is currently used to put up the bitmap on 
	 * the message reading window, label it, and figure out what
	 * how to display the part
	 *
	 * The content_abstract field is sometimes used in message summaries
	 */
	if (attach->mime_type > ExtendedMimeType) {
	    ZSTRDUP(attach->content_type, MimeTypeStr(attach->mime_type));
	    ZSTRDUP(attach->data_type, MimeTypeStr(attach->mime_type));
#if 0
	    if (!attach->content_abstract)
		ZSTRDUP(attach->content_abstract, 
		       MimeTypeStr(attach->mime_type));
#endif /* 0 */
	    if (!attach->mime_content_type_str)
		ZSTRDUP(attach->mime_content_type_str, 
		       MimeTypeStr(attach->mime_type));
	} else {
	    ZSTRDUP(attach->content_type, attach->mime_content_type_str);
	    ZSTRDUP(attach->data_type, attach->mime_content_type_str);
#if 0
	    if (!attach->content_abstract)
		ZSTRDUP(attach->content_abstract, 
		       attach->mime_content_type_str);
#endif /* 0 */
	}
	/* For now, just treat some types as Multipart_Mixed */
#if 0
	if ((attach->mime_type == MultipartAlternative) ||
	    (attach->mime_type == MultipartDigest) ||
	    (attach->mime_type == MultipartParallel) ||
	    (attach->mime_type == MultipartEnabledMail))
	    attach->mime_type = MultipartMixed;
#else
	if (MimePrimaryType(attach->mime_type) == MimeMultipart)
            attach->mime_type = MultipartMixed;

#endif
	/* This should be done for any subtype of text */
	if (attach->mime_type == TextPlain) {
	    paramValue = FindParam(MimeTextParamStr(CharSetParam), 
				   &attach->content_params);
	    attach->mime_char_set = GetMimeCharSet(paramValue);
	    if (!paramValue)
		AddContentParameter(&attach->content_params,
				    MimeTextParamStr(CharSetParam),
				    GetMimeCharSetName(attach->mime_char_set));
	}
#ifdef MAC_OS
	/* Special hack to handle binhex content type appropriately
	 * Under Unix, we'll just use a binhex decoder as the displayer
	 * Under MacOS, we treat binhex as an encoding
	 */
	else if ((attach->mime_type == ExtendedMimeType) &&
		 (!(ci_strcmp(attach->mime_content_type_str, 
				 ApplicationBinHexStr)))) {
	    if (attach->mime_encoding_str)
		free(attach->mime_encoding_str);
	    attach->mime_encoding_str = savestr(BinHexStr);
	}
#endif /* MAC_OS */
	attach->mime_encoding = GetMimeEncoding(attach->mime_encoding_str);
	/* attach->encoding_algorithm may be used in other places */
	if (attach->mime_encoding > ExtendedMimeEncoding) {
	    if ((attach->mime_encoding != SevenBit) &&
		(attach->mime_encoding != Binary) &&
		(attach->mime_encoding != EightBit))
		ZSTRDUP(attach->encoding_algorithm, 
		       MimeEncodingStr(attach->mime_encoding));
	    else
		if (attach->encoding_algorithm) {
		    xfree(attach->encoding_algorithm);
		    attach->encoding_algorithm = NULL;
		}
	} else
	    ZSTRDUP(attach->encoding_algorithm, 
		   attach->mime_encoding_str);
	paramValue = FindParam(MimeGlobalParamStr(GlobalNameParam), 
			       &attach->content_params);
	/* XXXX Hack - it already has a temporary content name by now */
	if (paramValue /* && !attach->content_name */) {
	    if (attach->content_name)
		free(attach->content_name);
	    turnoff(attach->a_flags, AT_NAMELESS);
	    attach->content_name = savestr(basename(paramValue));
	}
    }
}


/*
 *-------------------------------------------------------------------------
 *
 *  GetDefaultEncodingStr --
 *	Return the default MIME encoding for the given type, or NULL
 *	if the default is no encoding.
 *
 *  Results:
 *	As described above.
 *
 *  Side effects:
 *	None.
 *
 *-------------------------------------------------------------------------
 */
char *
GetDefaultEncodingStr(typeStr)
    char	*typeStr;
{
    mimePrimaryType	primaryType;
    
    if (typeStr && *typeStr) {
	primaryType = GetMimePrimaryType(typeStr);
	if (primaryType == MimeText)
	    return(MimeEncodingStr(QuotedPrintable));
	else if ((primaryType == MimeMultipart) ||
		 (primaryType == MimeMessage))
	    return(NULL);
    }
    return(MimeEncodingStr(Base64));
}

/*
 *-------------------------------------------------------------------------
 *
 *  ValidateMimeInfo --
 *	Examine the info in the attach data structure,
 *	determine the relevant mime information, and insert it into the
 *	appropriate fields.  If something is illegal, change it.
 *
 *	This is used to validate outgoing messages.
 *
 *  Results:
 *	Returns nothing.
 *
 *  Side effects:
 *	None.
 *-------------------------------------------------------------------------
 */

void
ValidateMimeInfo(attach)
    Attach      *attach; /* the attachment to examine */
{
    if (!attach)
	return;
#ifdef NOT_NOW
    /* Load X-Zm stuff into the MIME data structures 
     */
    if (!attach->mime_content_type_str) {
	if (attach->content_type)
	    ZSTRDUP(attach->mime_content_type_str, attach->content_type);
	else
	    ZSTRDUP(attach->mime_content_type_str, attach->data_type);
    }
    if (!attach->mime_encoding) {
	if (attach->encoding_algorithm)
	    ZSTRDUP(attach->mime_encoding_str, attach->encoding_algorithm);
    }
#endif
    if (!attach->mime_type) {
#ifdef NOT_NOW
	if (!attach->data_type) {
	    attach->data_type = GetTypeStrFromFile(attach->a_name, bin);
	    attach->mime_type = GetMimeType(attach->data_type);
	    /*
	      attach->encoding_algorithm = DefaultEncoding(attach->mime_type);
	      */
	    attach->encoding_algorithm = 
		DefaultEncoding(attach_data_type(attach));
	}
	else
	    attach->mime_type = GetMimeType(attach_data_type(attach));
#else
	attach->mime_type = GetMimeType(attach_data_type(attach));
#endif
    }
    /* Fix the data_type if it isn't there */
    if (!attach->data_type) {
	attach->data_type = ((attach->mime_type == TextPlain) ? "TEXT" :
			     (attach->mime_type > ExtendedMimeType) ? 
			     MimeTypeStr(attach->mime_type) :
			     attach->content_type);
    }
    /* 
     * Special hack to make sure content type binhex is handled correctly
     */
    if ((attach->mime_type == ExtendedMimeType) &&
	(!(ci_strcmp(attach_data_type(attach), 
			ApplicationBinHexStr)))) {
	if (attach->encoding_algorithm)
	    free(attach->encoding_algorithm);
	attach->encoding_algorithm = savestr(BinHexStr);
	attach->mime_encoding = GetMimeEncoding(attach->encoding_algorithm);
    } else if (!attach->mime_encoding) {
	attach->mime_encoding = GetMimeEncoding(attach->encoding_algorithm);
    }
    /* 
     * Don't encode subparts of type multipart or message 
     */
    if (attach->encoding_algorithm) {
	if (attach->mime_type > ExtendedMimeType) {
	    if ((MimePrimaryType(attach->mime_type) == MimeMultipart) ||
		(MimePrimaryType(attach->mime_type) == MimeMessage)) {
		xfree(attach->encoding_algorithm);
		attach->encoding_algorithm = NULL;
#if 0
		/* XXX handle portable newlines correctly; this is a
		 * gross special case
		 */
	    } else if (NeedsPortableNewLines(attach->mime_type) &&
		       (attach->mime_encoding == BASE64)) {
		ZSTRDUP(attach-encoding_algorithm, BASE64_CRLF);
#endif
	    }
	} else if (attach->mime_type == ExtendedMimeType)
	    if (GetMimePrimaryType(attach_data_type(attach)) == 
		MimeMultipart ||
		GetMimePrimaryType(attach_data_type(attach)) == MimeMessage) {
		xfree(attach->encoding_algorithm);
		attach->encoding_algorithm = NULL;
	    }    
    }
#ifdef NOT_NOW
    /* PrintContentParameters() doesn't do quoting, and it calls
     * strcpyStrip().  Bleah.  Put this back when that's fixed.
     */
    if (attach->content_name && isoff(attach->a_flags, AT_NAMELESS)) {
	char *cname = backwhack(attach->content_name);
	if (!cname) cname = attach->content_name;
	AddContentParameter(&attach->content_params,
			    MimeGlobalParamStr(GlobalNameParam),
			    cname);
    }
#endif /* NOT_NOW */
}

/*
 *-------------------------------------------------------------------------
 *
 *  GetClosestMimeType --
 *	Return the MIME type which should be used to interpret a message
 *	of the given type, if the type is unrecognized.
 *
 *  Results:
 *	Always returns a mimeType.
 *
 *  Side effects:
 *	None
 *
 *-------------------------------------------------------------------------
 */

mimeType
GetClosestMimeType(charPtr)
char	*charPtr;
{
    mimeType	returnVal;
    mimePrimaryType	primaryType;
    
    returnVal = GetMimeType(charPtr);
    if (returnVal > ExtendedMimeType)	{
	/* For now, just treat some types as Multipart_Mixed */
	if ((returnVal == MultipartAlternative) || 
	    (returnVal == MultipartDigest) ||
	    (returnVal == MultipartParallel) ||
	    (returnVal == MultipartEnabledMail))
	    returnVal = MultipartMixed;
    } else if (returnVal == ExtendedMimeType) {
	if (primaryType = GetMimePrimaryType(charPtr) > 
	    ExtendedMimePrimaryType)
	    switch (primaryType) {
	      case MimeMultipart: 
		returnVal = MultipartMixed; break;
	      case MimeText: 
		returnVal = TextPlain; break;
	      default: 
		returnVal = ApplicationOctetStream; 
	    }
    } else if (ci_strcmp("text", charPtr) == 0) {
	/* It is somewhat common for things to come in labelled simply
	 * as "Content-type: text"
	 */
	returnVal = TextPlain;
    } else if (ci_strcmp("multipart", charPtr) == 0) {
	/* Just in case something comes in labelled simply
	 * as "Content-type: multipart"
	 */
	returnVal = MultipartMixed;
    }
    return (returnVal);
}

/* Eventually the mime type will be a structure, so this stuff
 * will be much simpler
 */

mimePrimaryType
MimePrimaryType(type)
    mimeType	type;
{
    mimePrimaryType	returnVal;

    switch (type) {
      case UnknownMimeType:
	returnVal = UnknownMimePrimaryType; break;
      case IllegalMimeType:	
	returnVal = IllegalMimePrimaryType; break;
      case ExtendedMimeType:
	returnVal = ExtendedMimePrimaryType; break;
      case ApplicationOctetStream:	
      case ApplicationPostscript:	
	returnVal = MimeApplication; break;
      case AudioBasic:		
	returnVal = MimeAudio; break;
      case ImageGif:
      case ImageJpeg:
	returnVal = MimeImage; break;
      case MessageExternalBody:
      case MessageNews:
      case MessagePartial:
      case MessageRfc822:
	returnVal = MimeMessage; break;
      case MultipartAlternative:
      case MultipartAppledouble:
      case MultipartDigest:
      case MultipartEnabledMail:
      case MultipartMixed:
      case MultipartParallel:
      case MultipartSigned:
	returnVal = MimeMultipart; break;
      case TextPlain:
	returnVal = MimeText; break;
      case VideoMpeg:
	returnVal = MimeVideo; break;
      default:
	returnVal = UnknownMimePrimaryType;
    }
    return (returnVal);
}

mimeSubType
MimeSubType(type)
    mimeType	type;
{
    mimeSubType	returnVal;
    
    switch (type) {
      case UnknownMimeType:
	returnVal = UnknownMimeSubType; break;
      case IllegalMimeType:	
	returnVal = IllegalMimeSubType; break;
      case ExtendedMimeType:
	returnVal = ExtendedMimeSubType; break;
      case ApplicationOctetStream:	
	returnVal = MimeOctetStream; break;
      case ApplicationPostscript:	
	returnVal = MimePostscript; break;
      case AudioBasic:		
	returnVal = MimeBasic; break;
      case ImageGif:
	returnVal = MimeGif; break;
      case ImageJpeg:
	returnVal = MimeJpeg; break;
      case MessageExternalBody:
	returnVal = MimeExternalBody; break;
      case MessageNews:
	returnVal = MimeNews; break;
      case MessagePartial:
	returnVal = MimePartial; break;
      case MessageRfc822:
	returnVal = MimeRfc822; break;
      case MultipartAlternative:
	returnVal = MimeAlternative; break;
      case MultipartDigest:
	returnVal = MimeDigest; break;
      case MultipartMixed:
	returnVal = MimeMixed; break;
      case MultipartParallel:
	returnVal = MimeParallel; break;
      case MultipartEnabledMail:
	returnVal = MimeEnabledMail; break;
      case TextPlain:
	returnVal = MimePlain; break;
      case VideoMpeg:
	returnVal = MimeMpeg; break;
      default:
	returnVal = UnknownMimeSubType;
    }
    return (returnVal);
}

mimePrimaryType
GetMimePrimaryType(typeStr)
    char	*typeStr;
{
    int		i;
    char	*charPtr;
    
    if (typeStr && *typeStr) {
	charPtr = index(typeStr, '/');
	if (charPtr) {
	    if ((charPtr != typeStr) && *++charPtr) {
		if (index(charPtr, '/'))
		    return IllegalMimePrimaryType;
		for (i = 3; i < NumMimePrimaryType; i++) 
		    if (ci_strncmp(MimePrimaryTypeStr(i), typeStr, 
				      charPtr-typeStr-1) == 0)
			return (mimePrimaryType) i;
		if (ci_strncmp(typeStr, EXTENSION_PREFIX, 
				  strlen(EXTENSION_PREFIX))==0)
		    return ExtendedMimePrimaryType;
	    }
	} else
	    return IllegalMimePrimaryType;
    }
    return DEFAULT_MIME_PRIMARY_TYPE;
}

mimeSubType
GetMimeSubType(typeStr)
    char	*typeStr;
{
    int		i;
    char	*charPtr;
    
    if (typeStr && *typeStr) {
	charPtr = index(typeStr, '/');
	if (charPtr) {
	    if ((charPtr != typeStr) && *++charPtr) {
		if (index(charPtr, '/'))
		    return IllegalMimeSubType;
		for (i = 3; i < NumMimeSubType; i++) 
		    if (ci_strcmp(MimeSubTypeStr(i), charPtr) == 0)
			return (mimeSubType) i;
		if (ci_strncmp(charPtr, EXTENSION_PREFIX, 
				  strlen(EXTENSION_PREFIX))==0)
		    return ExtendedMimeSubType;
	    }
	} else
	    return IllegalMimeSubType;
    }
    return UnknownMimeSubType;
}

int
NeedsPortableNewlines(mimeTypeValue)
    mimeType	mimeTypeValue;
{
    /* The latter two are weird; message & multipart should never be encoded, 
     * but they really are line-oriented, so if they ARE encoded (and for PEM,
     * it is even legitimate) they should use portable newlines.
     */
    return ((MimePrimaryType(mimeTypeValue) == MimeMultipart) ||
	    (MimePrimaryType(mimeTypeValue) == MimeText) ||
	    (MimePrimaryType(mimeTypeValue) == MimeMessage));
}

/*
 *-------------------------------------------------------------------------
 *
 *  GetMimeCharSet, GetMimeEncoding, GetMimeType --
 *	Given a string, look up and return the enum value for the parameter.
 *
 *  Results:
 *	Always returns a value of the right type.  Return the default 
 *	type if the input string is NULL.
 *
 *  Side effects:
 *	None.
 *-------------------------------------------------------------------------
 */


/* Gets the mime charset name
 */

const char *
GetMimeCharSetName(cs)
    mimeCharSet 	cs;
{
    return ((const char *)c3_mimename_from_cs(cs));
}

/* Gets the charset number.  If not known, it will add it
 * to the internal table prefixed with x-
 * if the name is NULL or empty, it returns the default charset
 */

/* XXX add for !C3 here */

mimeCharSet
GetMimeCharSet(csname)
    const char	*csname;
{
    mimeCharSet	charset;

    if (csname && *csname) {
	if (!IsKnownMimeCharSetName(csname, &charset)) {
	    charset = c3_add_to_table(csname, catgets(catalog, CAT_LITE, 866, "Unknown Character Set"));
	}
	return (charset);
    }
    return DEFAULT_MIME_CHAR_SET;
}

mimeEncoding
GetMimeEncoding(charPtr)
    char	*charPtr;
{
    int i;
    
    if (charPtr && *charPtr) {
	for (i = 3; i < NumMimeEncoding; i++)
	    if (ci_strcmp(MimeEncodingStr(i), charPtr) == 0)
		return (mimeEncoding) i;
	if (ci_strncmp(charPtr, EXTENSION_PREFIX, 
			  strlen(EXTENSION_PREFIX))==0)
	    return ExtendedMimeEncoding;
	return IllegalMimeEncoding;
    }
    /* On incoming, if no encoding was specified, then it is none */
    return SevenBit;
}

mimeType
GetMimeType(charPtr)
char	*charPtr;
{
    int i;
    
    if (charPtr && *charPtr)
	{
	    for (i = 3; i < NumMimeType; i++)
		if (ci_strcmp(MimeTypeStr(i), charPtr) == 0)
		    return (mimeType) i;
	    if (GetMimePrimaryType(charPtr) == ExtendedMimePrimaryType)
		return ExtendedMimeType;
	    else if (GetMimePrimaryType(charPtr) > 
		     ExtendedMimePrimaryType)
		/* 
		 * good type, non-matching subtype
		 * Normally, the subtype must have an x-prefix; a subtype 
		 * which goes with another primary type is not acceptable
		 * 
		 * For now, we'll relax this to treat any unknown subtype
		 * as an extended type, because new subtypes are being 
		 * registered at such a dizzying pace
		 */
		if ((GetMimeSubType(charPtr) == ExtendedMimeSubType) ||
		    (GetMimeSubType(charPtr) == UnknownMimeSubType))
		    return ExtendedMimeType;
	    return IllegalMimeType;
	}
    return DEFAULT_MIME_TYPE;
}

/*
 *-------------------------------------------------------------------------
 *
 *  CompareMimeTypes, CompareMimeCodes, SortMimeTypes, SortMimeCodes --
 *	Given a pointer to a null terminated character array,
 *	sort the types or codes so that standard MIME stuff is first
 *	in alphabetical order,followed by extended and non-standard stuff
 *	in alphabetical order.  Don't go over a maximum size;
 *	we don't want to loop forever if the array isn't null-terminated.
 *
 *  Results:
 *	Returns 0 on success, -1 on failure.
 *
 *  Side effects:
 *	Alters the array as described.
 *
 *-------------------------------------------------------------------------
 */

static int 
CompareMimeTypes(type1PtrPtr, type2PtrPtr)
char **type1PtrPtr, **type2PtrPtr;
{
    mimeType	mimeType1;
    mimeType	mimeType2;

    mimeType1 = GetMimeType(*type1PtrPtr);
    mimeType2 = GetMimeType(*type2PtrPtr);
    if (((mimeType1 > ExtendedMimeType) && (mimeType2 > ExtendedMimeType)) ||
	((mimeType1 <= ExtendedMimeType) && (mimeType2 <= ExtendedMimeType)))
	return(strcmp(*type1PtrPtr, *type2PtrPtr));
    else if (mimeType1 > ExtendedMimeType)
	return -1;
    else
	return 1;
}

static int 
CompareMimeEncodings(encoding1PtrPtr, encoding2PtrPtr)
const char * const *encoding1PtrPtr, * const *encoding2PtrPtr;
{
    mimeEncoding	mimeEncoding1;
    mimeEncoding	mimeEncoding2;

    mimeEncoding1 = GetMimeEncoding(*encoding1PtrPtr);
    mimeEncoding2 = GetMimeEncoding(*encoding2PtrPtr);
    if (((mimeEncoding1 > ExtendedMimeEncoding) && 
	 (mimeEncoding2 > ExtendedMimeEncoding)) ||
	((mimeEncoding1 <= ExtendedMimeEncoding) && 
 	 (mimeEncoding2 <= ExtendedMimeEncoding)))
	return(strcmp(*encoding1PtrPtr, *encoding2PtrPtr));
    else if (mimeEncoding1 > ExtendedMimeEncoding)
	return -1;
    else
	return 1;
}

int
SortMimeThings(keys, predicate)
    char **keys;
    int (*predicate) NP((CVPTR, CVPTR));
{
    int cnt;
    char **charPtrPtr;

    if (keys) {
	for (cnt = 0, charPtrPtr = keys; *charPtrPtr; charPtrPtr++, cnt++)
	    if (cnt > 1024) return -1;
	qsort(keys, cnt, sizeof(char *), (int (*)NP((CVPTR, CVPTR))) predicate);
    }
    return 0;
}


int
SortMimeTypes(keys)
char	**keys;
{
    return SortMimeThings(keys,
			  ((int (*) NP((CVPTR, CVPTR)))
			   CompareMimeTypes));
}

int
SortMimeEncodings(keys)
char	**keys;
{
    return SortMimeThings(keys,
			  ((int (*) NP((CVPTR, CVPTR)))
			   CompareMimeEncodings));
}

/*
 *-------------------------------------------------------------------------
 *
 *   General-purpose functions for manipulating data structures
 *
 *-------------------------------------------------------------------------
 */
