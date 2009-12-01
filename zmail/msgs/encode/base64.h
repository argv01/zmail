/* base64.h	Copyright 1994 Z-Code Software, a Division of NCD */

#ifndef _BASE64_H_
#define _BASE64_H_

#include "osconfig.h"
#include "bytes.h"
#include "general.h"

/*
 * Bit extracting macros
 */
#define Bot2(b) ((b)&0x3)
#define Bot4(b) ((b)&0xf)
#define Bot6(b) ((b)&0x3f)
#define Top2(b) Bot2((b) >> 6)
#define Top4(b) Bot4((b) >> 4)
#define Top6(b) Bot6((b) >> 2)

/*
 * state buffer for encoding
 */
typedef struct
{
    Byte partial[4];
    short partialCount;
    short bytesOnLine;
} Enc64, *Enc64Ptr, **Enc64Handle;

/*
 * state buffer for decoding
 */
typedef struct
{
    short decoderState;	/* which of 4 bytes are we seeing now? */
    long invalCount;	/* how many bad chars found so far? */
    long padCount;	/* how many pad chars found so far? */
    Byte partial;	/* partially decoded byte from/for last/next time */
} Dec64, *Dec64Ptr, **Dec64Handle;

extern long Encode64 P((const char *, long, char *, const char *, Enc64Ptr));
extern long Decode64 P((char *, long, char *, long *, Dec64Ptr));

#endif /* !_BASE64_H_ */
