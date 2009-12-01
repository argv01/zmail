/* sock_adr.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.1 $
 * $Date: 1994/12/31 03:43:46 $
 * $Author: wilbur $
 */

#ifndef _SOCK_ADR_H_
#define _SOCK_ADR_H_

#include <general.h>	/* for P() macro */
#include <zcsock.h>
#include <zctype.h>
#include <zcunix.h>
#include <zcalloc.h>

//
// function prototypes
//

void sock_AddrToStr P((char *buf, CVPTR addr));
void sock_AddrPortToStr P((char *buf, char *s, sock_Port port));

#endif /* _SOCK_ADR_H_ */
