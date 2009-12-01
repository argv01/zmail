#ifndef _MAILSERV_H_
#define _MAILSERV_H_

/*
 * $RCSfile: mailserv.h,v $
 * $Revision: 2.8 $
 * $Date: 1995/03/14 20:16:11 $
 * $Author: fox $
 */

#ifndef LF
# define LF 10
#endif /* LF */

#define ZYNCPORT	15232

struct mailserver {
    char *user;
    char *password;
};
typedef struct mailserver *mailserver_t;

extern mailserver_t mserv;

#define mailserver_GetPassword(MS) ((MS)->password)
#define mailserver_SetPassword(MS, X) ((MS)->password = str_replace(&(MS)->password, (X)))
#define mailserver_UnsetPassword(MS) ((MS)->password = str_replace(&(MS)->password, NULL))
#define mailserver_GetUsername(MS) ((MS)->user)
#define mailserver_SetUsername(MS, X) ((MS)->user = str_replace(&(MS)->user, (X)))

int pop_check_connect P((const char *host, const char *user, const char *pword));

#endif /* _MAILSERV_H_ */
