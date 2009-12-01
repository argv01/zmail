#include <zmail.h>
#include <general.h>
#include <uitaskm.h>

#ifndef lint
static char	uitaskm_rcsid[] =
    "$Id: uitaskm.c,v 1.6 1994/05/17 01:05:44 pf Exp $";
#endif

struct uitaskm_State_struct {
    struct uitaskm_State_struct *next;
    uitaskm_Update_t update;
    int level, nest;
};
typedef struct uitaskm_State_struct uitaskm_State_t;

uitaskm_State_t zero_state = {
    /* level is -2 to ensure that the task meter will never be brought
     * up with this state at the top of the stack (-2 is greater than
     * all possible values of intr_level)
     */
    NULL, { 0, NULL, NULL, 0 }, -2, 0
};
uitaskm_State_t *stack = &zero_state;

/* save, restore current folder */

int
gui_handle_intrpt(flags, nest, str, percentage)
u_long flags;
int nest;
char *str;
long percentage;
{
    static int started = 0;
    uitaskm_State_t *ust = stack;
    uitaskm_Event_t event;
    uitaskm_Update_t *upd;
    msg_folder *fldr = current_folder;

    if (ison(flags, INTR_ON)) {
	ust = (uitaskm_State_t *) calloc(sizeof *ust, 1);
	turnon(ust->update.state, uitaskm_StateMessage|uitaskm_StateVisible);
	ust->update.main_msg = savestr(str);
	ust->next = stack;
	ust->level = percentage;
	ust->nest = nest;
	if (ison(flags, INTR_LONG) ||
		ison(stack->update.state, uitaskm_StateLong))
	    turnon(ust->update.state, uitaskm_StateLong);
	stack = ust;
	if (intr_level == -1 || intr_level > percentage)
	    return 0;
	started = True;
	gui_taskm_update(&ust->update);
	current_folder = fldr;
	return 0;
    }
    if (ison(flags, INTR_OFF)) {
	while (stack->nest > nest) {
	    stack = stack->next;
	    xfree(ust->update.main_msg);
	    xfree(ust);
	}
	if (started)
	    gui_taskm_update(&stack->update);
	current_folder = fldr;
	return 0;
    }
    if (!started || intr_level > ust->level)
	return 0;
    upd = &ust->update;
    upd->state = upd->state & (uitaskm_StateLong|uitaskm_StateSubMessage);
    turnon(upd->state,
	uitaskm_StateVisible|uitaskm_StateMessage|uitaskm_StateContinue);
    if (ison(flags, INTR_RANGE))
	turnon(upd->state, uitaskm_StatePercent);
    upd->percent = percentage;
    if (isoff(flags, INTR_NONE))
	turnon(upd->state, uitaskm_StateStop);
    if (ison(flags, INTR_MSG)) {
	turnon(upd->state, uitaskm_StateSubMessage);
	upd->sub_msg = str;
    }
    gui_taskm_update(upd);
    upd->sub_msg = NULL;
    event = gui_taskm_get_event(0);
    current_folder = fldr;
    if (event == uitaskm_EventStop) {
	turnon(glob_flags, WAS_INTR);
	return True;
    }
    if (event == uitaskm_EventContinue) {
	turnoff(upd->state, uitaskm_StateVisible);
	gui_taskm_update(upd);
	current_folder = fldr;
	while (ust->next) {
	    ust->level = -1;
	    ust = ust->next;
	}
    }
    return False;
}

/* gui_task_meter -- z-script interface for popping up, down and updating
 * the task meter and the messages inside.
 * usage: task_meter [-on|-off|-check] [message]
 * -on turns task meter on
 * -off turns task meter off
 * -check returns whether the stop button was pressed
 * -wait returns when the stop button was pressed
 *
 * e.g.

   function getfiles() {
       ask -input remote "What host do you want to get files from?"
       ask -input files "What files do you want to get?"
       task_meter -on Copying $files
       # does this for each file or until getfile return -1
       foreach file ($files) getfile
       task_meter -off Done.
    }

    function getfile() {
       task_meter -check Getting $1 ...
       if $status == -1    # user must have clicked on Stop
	   ask "Stop getting files?"
	   if $status != 0
	       return -1
	   endif
       sh rcp ${remote}:$1 $1
       return 0;
    }

    sh -bg recordaiff filename
    set pid = $status
    task_meter -on Recording... press Stop to stop recording.
    loop
	sh -text size ls -l filename
	task_meter -check Record data size = $size
	if $status == -1
	    break
	endif
    endloop
    kill SIGINT $pid
    task_meter -off Done.

 */
gui_task_meter(argc, argv)
int argc;
char **argv;
{
    const char *argv0;
    char message[256];
    int val, old_intr_level = intr_level;
    u_long flags = 0L;

    argv0 = *argv;
    message[0] = 0;
    while (argv && *++argv) {
	if (**argv == '-')
	    switch (argv[0][1]) {
		case 'o' :
		    if (argv[0][2] == 'f')
			turnon(flags, INTR_OFF);
		    else if (argv[0][2] == 'n')
			turnon(flags, INTR_ON);
		    else {
			error(UserErrWarning, "%s: -on or -off", argv0);
			return -1;
		    }
		when 'n' :
		    turnon(flags, INTR_NONE);
		when 'c' :
		    turnon(flags, INTR_CHECK);
		when 'w' :
		    turnon(flags, INTR_WAIT);
		when '?' :
		    /* XXX casting away const */
		    return help(0, (VPTR) argv0, cmd_help);
	    }
	else {
	    (void) argv_to_string(message, argv);
	    break;
	}
    }
    if (!flags ||
	    ison(flags, INTR_ON) + ison(flags, INTR_CHECK) +
	    ison(flags, INTR_OFF) > 1) {
	error(UserErrWarning, catgets( catalog, CAT_GUI, 24, "You must specify -on, -off or -check." ));
	return -1;
    }
    /* allow "task_meter -wait" without requiring -on */
    if (isoff(flags, INTR_ON+INTR_OFF+INTR_CHECK) && ison(flags, INTR_WAIT))
	turnon(flags, INTR_ON);
    if (ison(flags, INTR_WAIT)) /* don't allow "-off -wait" */
	turnoff(flags, INTR_OFF);
    if (message[0])
	turnon(flags, INTR_MSG);

    intr_level = 1;
    val = handle_intrpt(flags, message, 2);
    if (ison(flags, INTR_ON)) {
	turnoff(flags, INTR_ON+INTR_OFF);
	turnon(flags, INTR_CHECK);
	val = handle_intrpt(flags, message, 2);
    }
    intr_level = old_intr_level;
    return val;
}
