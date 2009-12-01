/*
 * $RCSfile: dppopen.h,v $
 * $Revision: 2.1 $
 * $Date: 1995/03/06 19:16:37 $
 * $Author: bobg $
 */

#ifndef DPPOPEN_H
# define DPPOPEN_H

extern struct dpipe *dputil_popen P((const char *,
				     const char *));
extern int dputil_pclose P((struct dpipe *));

#endif /* DPPOPEN_H */
