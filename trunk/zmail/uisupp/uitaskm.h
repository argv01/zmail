/* UI task meter */

#ifndef _UITASKM_H_
#define _UITASKM_H_

/*
 * $RCSfile: uitaskm.h,v $
 * $Revision: 1.3 $
 * $Date: 1994/04/15 02:27:09 $
 * $Author: pf $
 */

#include "uisupp.h"

struct uitaskm_Update_struct {
    zmFlags state;
    char *main_msg, *sub_msg;
    long percent;
};
typedef struct uitaskm_Update_struct uitaskm_Update_t;

#define uitaskm_EventFlags_Wait ULBIT(0)

#define uitaskm_StateVisible	ULBIT(0)
#define uitaskm_StateMessage	ULBIT(1)
#define uitaskm_StateSubMessage	ULBIT(2)
#define uitaskm_StatePercent	ULBIT(3)
#define uitaskm_StateStop	ULBIT(4)
#define uitaskm_StateContinue	ULBIT(5)
#define uitaskm_StateLong	ULBIT(6)

typedef enum {
    uitaskm_EventNone,
    uitaskm_EventStop,
    uitaskm_EventContinue
} uitaskm_Event_t;

extern zmBool gui_taskm_update P((uitaskm_Update_t *upd));
extern uitaskm_Event_t gui_taskm_get_event P((zmFlags type));

#endif /* _UITASKM_H_ */
