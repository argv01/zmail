/* uu.h	Copyright 1994 Z-Code Software, a Divison of NCD */

#ifndef _UU_H_
#define _UU_H_

/* $Id: uu.h,v 1.3 1994/07/20 00:25:02 pf Exp $ */

#include "osconfig.h"
#include "bytes.h"
#include "general.h"

/* File modes for EncUU.mode */

#define UUEN_DECODE_MODE	0644
#define SAFE_DECODE_MODE	0600

/*
 * state buffer for encoding
 */
typedef struct
{
    Byte partial[4];		/* Left if input not a multiple of 3 bytes */
    short partialCount;		/* Number of bytes in partial */
    Byte outLine[((45/3)*4)+2];	/* We have to output a full line at a time */
    short bytesOnLine;		/* Number of bytes in outLine */
    short bytesInput;		/* Number of INPUT bytes in outLine */
    long lastPos;		/* Total bytes processed so far */
    char *fileName;		/* Name to give the output object */
    int mode;			/* UNIX file permissions */
} EncUU, *EncUUPtr, **EncUUHandle;

/* Decoding states */

#define UU_START_STATE	000
#define UU_SEEK_NEWLINE	001
#define UU_READ_NEWLINE	002
#define UU_PAST_NEWLINE 004
#define UU_DECODE_LINE	010

#define UU_FILENAME_LEN 100

/*
 * state buffer for decoding
 */
typedef struct
{
    short decodeState;	/* see above */
    Byte foundBegin;	/* boolean -- found "begin" line */
    Byte foundEnd;	/* boolean -- found "end" line */
    Byte atEnd;		/* boolean -- "end" line MUST be next */
    long invalCount;	/* how many bad chars found so far? */
    Byte partial[4];	/* partially decoded quad from/for last/next time */
    short partialCount;	/* how long a partial quad found so far? */
    short bytesValid;	/* number of valid output bytes from this input */
    char fileName[UU_FILENAME_LEN];
    	 		/* Name read from input stream (returned) */
    int mode;		/* UNIX file permissions from input (returned) */
} DecUU, *DecUUPtr, **DecUUHandle;

extern long EncodeUU P((char *, long, char *, char *, EncUUPtr));
extern long DecodeUU P((char *, long, char *, long *, DecUUPtr));

#endif /* !_UU_H_ */
