/*
 *  $RCSfile: xface.h,v $
 *  $Revision: 1.1 $
 *  $Date: 1994/02/25 20:52:57 $
 *  $Author: pf $
 *
 *  UnCompface - 48x48x1 image decompression
 *
 *  Copyright (c) James Ashton - Sydney University - June 1990.
 *
 *  Written 11th November 1989.
 *
 *  Permission is given to distribute these sources, as long as the
 *  copyright messages are not removed, and no monies are exchanged. 
 *
 *  No responsibility is taken for any errors on inaccuracies inherent
 *  either to the comments or the code of this program, but if reported
 *  to me, then an attempt will be made to fix them.
 */

#ifndef XFACE_H
#define XFACE_H

#include <general.h>

/* define the face size - 48x48x1 */
#define XF_WIDTH 48
#define XF_HEIGHT XF_WIDTH

extern int uncompface P((char *)) ;

#endif /* !XFACE_H */
