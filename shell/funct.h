/* funct.h	Copyright 1992 Z-Code Software Corp. */

#ifndef _FUNCT_H_
#define _FUNCT_H_

#include "linklist.h"
#include "zmopt.h"
#include "zcstr.h"

#ifdef ZMAIL_INTL
enum func_help_priority {
  HelpPrimary,
  HelpFallback
};
#endif /* ZMAIL_INTL */

typedef struct zmFunction {
    struct link f_link;
    char **f_cmdv;
#ifdef ZMAIL_INTL
    enum func_help_priority help_priority;
#endif /* ZMAIL_INTL */
    char **help_text;
} zmFunction;

#define new_function() zmNew(zmFunction)
#define next_function(f) link_next(zmFunction,f_link,f)

extern zmFunction *function_list, *folder_filters, *new_filters;

#define lookup_function(n) \
    (zmFunction *)retrieve_link((struct link *)function_list, n, \
	(int (*)()) strcmp)

extern void free_funct();

/* Stuff for handling user-defined function calls */

extern Option *zmfunc_args;

#define ZFN_GOT_MSGS	ULBIT(0)	/* see get_msg_list() */
#define ZFN_FREE_ARGV	ULBIT(1)	/* see zm_shift() */
#define ZFN_DIRECT_CALL	ULBIT(2)	/* see call_deferred() */
#define ZFN_INIT_ARGS	ULBIT(3)	/* arguments taken from command
					   line (not from a function call) */
#endif /* _FUNCT_H_ */
