#ifndef _QP_H_
#define _QP_H_

#include "osconfig.h"
#include "bytes.h"
#include "general.h"

typedef enum {
    qpNormal,	/* Scanning plain ASCII bytes */
    qpEqual,	/* Decoded first byte of =hh, looking for second */
    qpByte1	/* Decoded second byte of =hh, looking for third */
} QPStates;

typedef enum {
    qpmNormal,	/* Straight MIME (RFC1521) quoted-printable */
    qpmQCode,	/* RFC1522 `Q' encoding for 8-bit headers */
    qpmQText	/* RFC1521, but output must be text (no null bytes) */
} QPModes;

typedef struct {
    QPModes mode;
    QPStates state;
    Byte lastChar;
} DecQP, *DecQPPtr, **DecQPHandle;

typedef struct {
    QPModes mode;
    short linepos;	/* Position on output line (max of 76 chars each) */
    const char *charsetname;	/* Character set from which we are encoding */
} EncQP, *EncQPPtr, **EncQPHandle;

extern long EncodeQP P((const char *, long, char *, const char *, EncQPPtr)),
	    qp_encode P((char *, long, char *));
extern long DecodeQP P((char *, long, char *, long *, DecQPPtr)),
	    qp_decode P((char *, char *));

#endif /* !_QP_H_ */
