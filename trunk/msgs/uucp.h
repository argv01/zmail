#ifndef _UUCP_H_
#define _UUCP_H_

/*
 * $RCSfile: uucp.h,v $
 * $Revision: 2.4 $
 * $Date: 1994/03/26 02:59:29 $
 * $Author: fox $
 */

struct uucpsend_struct {
    int seqno;
    char **addrs;
    char *rootdir, *mailserver;
    char *datafile, *controlfile, *remotefile, *bodyfile;
};
typedef struct uucpsend_struct *uucpsend_t;

#define uucpsend_GetHostname(X) zm_gethostname()
#define uucpsend_GetSeqNo(X) ((X)->seqno)
#define uucpsend_SetSeqNo(X, Y) ((X)->seqno = (Y))
#define uucpsend_GetAddrs(X) ((X)->addrs)
#define uucpsend_SetAddrs(X, Y) ((X)->addrs = (Y))
#define uucpsend_GetMailServer(X) ((X)->mailserver)
#define uucpsend_SetMailServer(X, Y) ((X)->mailserver = (Y))
#define uucpsend_SetDataFilePtr(X, Y) ((X)->datafileptr = (Y))
#define uucpsend_GetControlFile(X) ((X)->controlfile)
#define uucpsend_SetControlFile(X, Y) ((X)->controlfile = (Y))
#define uucpsend_GetDataFile(X) ((X)->datafile)
#define uucpsend_SetDataFile(X, Y) ((X)->datafile = (Y))
#define uucpsend_GetRemoteFile(X) ((X)->remotefile)
#define uucpsend_SetRemoteFile(X, Y) ((X)->remotefile = (Y))
#define uucpsend_GetUsername(X) zlogin
#define uucpsend_GetRootDir(X) ((X)->rootdir)
#define uucpsend_SetRootDir(X, Y) ((X)->rootdir = (Y))
#define uucpsend_SetBodyFileName(X, Y) ((X)->bodyfile = (Y))
#define uucpsend_GetBodyFileName(X)((X)->bodyfile)

extern uucpsend_t uucpsend_Start P ((char *, char **));
extern FILE *uucpsend_GetDataFilePtr P ((uucpsend_t));
extern FILE *uucpsend_GetHdrFilePtr P ((uucpsend_t));

extern int uucpsend_Finish P ((uucpsend_t));

#endif /* _UUCP_H_ */
