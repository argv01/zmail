/* sockunix.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.1 $
 * $Date: 1994/12/31 03:43:47 $
 * $Author: wilbur $
 */

#ifndef _SOCKUNIX_H_
#define _SOCKUNIX_H_


//
// function prototypes
//
void *sock_Read P((sock_t sock, int len, int *retlen));
void sock_SetTimeout P((sock_t sock, int tmout));
void sock_ClearTimeout P((sock_t sock));

#endif /* _SOCKUNIX_H_ */
