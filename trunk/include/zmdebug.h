/*
 * Run-Time Debugging Controls
 *
 * debug.h	Copyright 1990-1994, Z-Coe Software Corp.
 *
 */


#ifndef INCLUDE_ZMDEBUG_H
#define INCLUDE_ZMDEBUG_H


#include "osconfig.h"

extern char debug;	/* debug causes various print statements in code */

extern void zmDebug VP((char *, ...));

#define Debug		if (debug == 0) {;} else zmDebug



#endif /* !INCLUDE_ZMDEBUG_H */
