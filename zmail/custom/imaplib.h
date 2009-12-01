/* imaplib.h	Copyright 1997-1998 NetManage, Inc. */

/*
 * $Revision: 2.1 $
 * $Date: 1998/12/07 22:43:05 $
 * $Author: schaefer $
 */

#ifndef _IMAPLIB_H_
#define _IMAPLIB_H_

/*
 * Header file for the "imaplib.c" client IMAP 
 * protocol implementation.
 */
#if defined( IMAP )

extern char imap_error[];

extern int imap_debug;

/* Folder lists */

typedef struct _folders {
        int     delimiter;
        char    *name;
        long    attributes;
	int	level;
	struct _folders *parent;
        struct _folders **sibs;
	int	nsibs;
} Folders;

#define ROOT 0	/* mustn't collide with c-client's LATT* defines */

#endif /* IMAP */

#endif /* _IMAPLIB_H_ */
