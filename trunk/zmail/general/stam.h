/* 
 * $RCSfile: stam.h,v $
 * $Revision: 2.4 $
 * $Date: 1995/02/17 02:46:08 $
 * $Author: bobg $
 */

#ifndef STAM_H
# define STAM_H

#include <hashtab.h>
#include <sklist.h>

struct stam {
    struct hashtab statenames;
    struct dlist states;
    struct sklist transitions;
    int state;
};

enum {
    stam_BeginState = 0,
    stam_EndState = 1
};

extern void stam_Init P((struct stam *,
			 int (*) NP((struct stam *, int)),
			 void (*) NP((struct stam *, int, int, int)),
			 int, int));
extern void stam_Destroy P((struct stam *));

extern int stam_AddState P((struct stam *, char *));
extern int stam_AddToken P((struct stam *, char *));
extern int stam_GetState P((struct stam *, char *));
extern int stam_GetToken P((struct stam *, char *));
extern void stam_AddTransition P((struct stam *, int, int, int));
extern void stam_Execute P((struct stam *, int));

#endif /* STAM_H */
