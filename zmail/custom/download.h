/* download.h	Copyright 1994 Z-Code Software Corp. */

/*
 * $Revision: 2.1 $
 * $Date: 1994/12/31 03:24:01 $
 * $Author: wilbur $
 */

#ifndef _DOWNLOAD_H_
#define _DOWNLOAD_H_

//
// function prototypes
//
void zync_describe_files P((PopServer listener, const char *dirname));
void zync_update_files P((PopServer provider, const char *dirname, void (*backup)(const char *)));
int zync_moi P((PopServer server));

#endif /* _DOWNLOAD_H_ */

