#ifndef _ZMSOCK_H_
#define _ZMSOCK_H_

/*
 * $RCSfile: zcsock.h,v $
 * $Revision: 2.15 $
 * $Date: 2005/04/27 08:46:11 $
 * $Author: syd $
 */

/* delete all this crap */
/* make this zcsock.h */

#include <general.h>

#undef msg

#ifndef MAC_OS
# ifndef _WINDOWS
#  include <sys/types.h>
#  if defined(MIPS) && !defined(BSD) && !defined(_ABI_SOURCE)
#   include <bsd/netinet/in.h>
#  else /* !MIPS */
#   ifndef ZC_INCLUDED_NETINET_IN_H
#    define ZC_INCLUDED_NETINET_IN_H
#    include <netinet/in.h>
#   endif /* !ZC_INCLUDED_NETINET_IN_H */
#  endif /* !MIPS */
#  include <sys/socket.h>
#  define SOCKET int
#  define SOWRITE(S, BUF, CT) write(S, BUF, CT)
#  define SOREAD(S, BUF, CT) read(S, BUF, CT)
#  define SOCLOSE(S) close(S)
#  define SOERROR() (errno)
#  define SOERRORSTR() (strerror(errno))
typedef int Socket;
# else /* _WINDOWS */
#  undef u_long
#  undef u_short
#  include <winsock.h>
#  include "custom\zwsock.h"
#  define SOWRITE(S, BUF, CT) lpfnsend(S, BUF, CT, 0)
#  define SOREAD(S, BUF, CT) lpfnrecv(S, BUF, CT, 0)
#  define SOCLOSE(S) lpfnclosesocket(S)
#  define SOERROR() (lpfnWSAGetLastError())
#  define SOERRORSTR() (wsa_sockerror())
#  define endhostent()
#  define endservent()
#  define sethostent(X)
#  define setservent(X)
extern char *wsa_sockerror(), *wsa_sockerror_str P((int));
typedef SOCKET Socket;
# endif /* _WINDOWS */
#else /* MAC_OS */
# include "MacTCPCommonTypes.h"
# define SOERRORSTR() (itoa(SOERROR()))
# define endhostent()
# define endservent()
# define sethostent(X)
# define setservent(X)
typedef StreamPtr Socket; 
typedef StreamPtr sock_t;
#define sock_Error -1
#define SOCLOSE(S)	close_port(S)
	/* 2/12/95 gsf -- don't use NLS now, undef to find errs */
#undef sock_Close
#undef sock_Create
#endif /* MAC_OS */

/* this is TOTAL BULL! */
#ifdef msg_cnt
#define msg (current_folder->mf_msgs)
#endif /* msg_cnt */

typedef unsigned short sock_Port;

#if !defined(_WINDOWS) && !defined(MAC_OS)

typedef int sock_t;
#define sock_Error -1
#define sock_Create(A, B, C) socket(A, B, C)
#define sock_Bind(A, B, C) bind(A, B, C)
#define sock_Connect(A, B, C) connect(A, B, C)
#define sock_Listen(A, B) listen(A, B)
#define sock_Accept(A, B, C) accept(A, B, C)
#define sock_Close(A) close(A)
#define sock_Write(A, B, C) send(A, B, C, 0)
#define sock_GetSockName(A, B, C) getsockname(A, B, C)
#define sock_GetPeerName(A, B, C) getpeername(A, B, C)

#else /* !_WINDOWS && !MAC_OS */
# ifdef _WINDOWS

typedef SOCKET sock_t;
#define sock_Error SOCKET_ERROR
#define sock_Create(A, B, C) lpfnsocket(A, B, C)
#define sock_Close(A) lpfnclosesocket(A)

# endif /* _WINDOWS */
#endif /* !_WINDOWS && !MAC_OS */

#define sock_EXC_CANCELLED "sock_Cancelled"

extern void *sock_Read P((sock_t, int, int *));
extern void sock_SetTimeout P((sock_t, int));
extern void sock_ClearTimeout P((sock_t));
extern void sock_AddrToStr P ((char *, CVPTR));
extern void sock_AddrPortToStr P ((char *, char *, sock_Port));

#endif /* _ZMSOCK_H_ */
