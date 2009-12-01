/* cmdtab.h	Copyright 1993 Z-Code Software Corp.  All rights reserved */

#ifndef __CMDTAB_H__
#define __CMDTAB_H__

#define CMD_INTERACTIVE		ULBIT(0)	/* Is *always* interactive */
#define CMD_REQUIRES_MSGS	ULBIT(1)	/* *Always* needs messages */

#define CMD_INPUT_TEXT		ULBIT(2)	/* Text pipe -- none, yet */
#define CMD_INPUT_NO_MSGS	ULBIT(3)	/* No input message pipe */

#define CMD_OUTPUT_TEXT		ULBIT(4)	/* ... and message pipe, too */
#define CMD_OUTPUT_PAGED	ULBIT(4)	/* Outputs through pager */
#define CMD_OUTPUT_NO_MSGS	ULBIT(6)	/* No output message pipe */
#define CMD_OUTPUT_NO_ARGS	ULBIT(7)	/* Output iff argc == 1 */

#define CMD_X11_GUI_ONLY	ULBIT(8)
#define CMD_LITE_VUI_ONLY	ULBIT(9)
#define CMD_GUI_OR_VUI_ONLY	ULBIT(10)
#define CMD_FULLSCREEN_ONLY	ULBIT(11)

#define CMD_COMPATIBILITY	ULBIT(12)	/* Archaic but still here */
#define CMD_HIDDEN		ULBIT(13)	/* Not shown in help page */

#define CMD_CALLS_PROGRAM	ULBIT(14)	/* Invokes external program */
#define CMD_REALLY_KEYWORD	ULBIT(15)	/* A lexical keyword */

#define CMD_SHOW_OTHERNAMES	ULBIT(16)	/* Don't hide other names */

#define CMD_ADMIN		ULBIT(17)	/* In system.zmailrc only */

/* Common overlap cases */
#define CMD_PAGES_MSGS	CMD_REQUIRES_MSGS|CMD_OUTPUT_PAGED
#define CMD_SHOWS_MSGS	CMD_REQUIRES_MSGS|CMD_OUTPUT_TEXT
#define CMD_TEXT_ONLY	CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS|CMD_OUTPUT_TEXT
#define CMD_UCB_MAIL	CMD_COMPATIBILITY|CMD_HIDDEN
#define CMD_CURSES	CMD_TEXT_ONLY|CMD_FULLSCREEN_ONLY
#define CMD_DEFINITION	CMD_TEXT_ONLY|CMD_OUTPUT_NO_ARGS
#define CMD_HELPFUL	CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS|CMD_OUTPUT_PAGED
#define CMD_X11_SPECIAL	CMD_TEXT_ONLY|CMD_X11_GUI_ONLY|CMD_HIDDEN

typedef struct zmInterpose {
    struct link link;
    char *callback;
    u_long flags;
} zmInterpose;

#define INTERPOSER_ACTIVE	ULBIT(0)
#define INTERPOSER_REMOVED	ULBIT(1)
#define INTERPOSER_STOWED	ULBIT(2)	/* For saveopts */

typedef struct zmCommand {
    char *command;		/* Name of the command */
    int (*func)();		/* The function to call */
    u_long flags;		/* Bitflags describing behavior */
    zmInterpose *pre_calls;	/* Call before executing */
    zmInterpose *post_calls;	/* Call after executing */
} zmCommand;

typedef zmCommand *zmCommandTabEntry;	/* For hash table */
typedef struct zmInterposeTable {
    struct link link;
    zmInterpose *interposers;
} zmInterposeTable;

extern int
    interposer_status,
    interposer_thwart,
    interposer_depth;

extern zmInterpose **fetch_interposer_list P((const char *, zmInterposeTable **));
extern zmCommand *zscript_add P((zmCommand *));
extern void
    zscript_add_table P((zmCommand *, int)),
    zscript_initialize P((void)),
    destroy_interpose_table P((zmInterposeTable *));
extern int zm_interpose_cmd P((int, char **, zmInterposeTable **));
extern int call_all_interposers P((zmInterpose **, char **, msg_group *));
extern int interpose_on_msgop P((char *, int, const char *));

#endif  /* _CMDTAB_H_ */
