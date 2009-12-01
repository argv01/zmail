/*
 * pop.h: Header file for the "pop.c" client POP3 protocol
 * implementation.
 */

#ifndef _POP_H_
#define _POP_H_

#ifdef POP3_SUPPORT

/*
 * Values for the "ispop" field of _PopServer
 */
#define POP3_NONE	0
#define POP3_MINIMAL	1
#define POP3_LAST	2
#define POP3_UIDL	3
#define POP3_ZPOP	4

#define pop_services(ps, x)	((ps)->ispop >= (x))

#include "poplib.h"
#include "popmail.h"

#endif /* POP3_SUPPORT */

#endif /* _POP_H_ */
