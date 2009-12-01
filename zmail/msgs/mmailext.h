/*                                  Copyright 1993 Z-Code Software Corp.
 * mmailext.h --
 *
 *	This is the header file for users of variables and
 *	procedures exported from mmailext.c.
 *	
 *						C.M. Lowery, 1993
 *	$Id: mmailext.h,v 2.11 2005/05/09 09:15:20 syd Exp $
 */


#ifndef _MMAILEXT_H
#define _MMAILEXT_H

#include <general.h>

struct Attach;

void	AddContentParameter P((mimeContentParams *, const char *, const char *));
int	CopyContentParameters P((mimeContentParams *, mimeContentParams *));
void	InsertContentParameters P((char *, char *, struct Attach *));
void	ParseContentParameters P((char *, mimeContentParams *));
void	FreeContentParameters();
int	PrintContentParameters P((char *, mimeContentParams *));
char	*FindParam P((const char *, const mimeContentParams *));
char	*Cleanse P((char *));
char	*StripLeadingSpace P((char *));
void 	StripTrailingSpace P((char *));
int	strcpyStrip P((char *, const char *, int));

#endif /* _MMAILEXT_H */
/* DON'T ADD STUFF AFTER THIS #endif */
