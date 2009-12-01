/* c_bind.h  -- command bindings -- Copyright 1991 Z-Code Software Corp. */

#ifndef _C_BIND_H_
#define _C_BIND_H_

#include <general.h>

#define MAX_BIND_LEN 20   /* max length a string can be to bind to a command */
#define MAX_MACRO_LEN 256 /* max length of a macro bound to a command */

/* to see if a key sequence matches, prefixes or misses a set binding */
#define NO_MATCH	0
#define MATCH		1
#define A_PREFIX_B	2
#define B_PREFIX_A	3

/*
 * Constants to define fullscreen mode functions.
 */
#ifdef NULL_MAP
#undef NULL_MAP
#endif /* NULL_MAP */
#define NULL_MAP	(struct cmd_map *)0

#define C_ERROR		(-1L)	/* Out of bounds value */
#define C_NULL		0L	/* THIS MUST BE THE FIRST ITEM */
#define C_ALIAS		1L
#define C_PREV_MSG	2L
#define C_BIND		3L
#define C_BIND_MACRO	4L
#define C_BOTTOM_PAGE	5L
#define C_CHDIR		6L
#define C_COPY_MSG	7L
#define C_COPY_LIST	8L
#define C_DELETE_MSG	9L
#define C_DELETE_LIST	10L
#define C_DISPLAY_MSG	11L
#define C_DISPLAY_NEXT	12L
#define C_EXIT		13L
#define C_EXIT_HARD	14L
#define C_FIRST_MSG	15L
#define C_FOLDER	16L
#define C_FOLDER_MENU   17L
#define C_GOTO_MSG	18L
#define C_IGNORE	19L
#define C_JOBS_MENU	20L
#define C_LAST_MSG	21L
#define C_CURSES_ESC	22L
#define C_PRINT_MSG	23L
#define C_MACRO		24L
#define C_MAIL		25L
#define C_MAIL_FLAGS	26L
#define C_MAP		27L
#define C_MAP_BANG	28L
#define C_MARK_MSG	29L
#define C_OWN_HDR	30L
#define C_NEXT_MSG	31L
#define C_PRESERVE	32L
#define C_QUIT		33L
#define C_QUIT_HARD	34L
#define C_REDRAW	35L
#define C_REPLY_SENDER	36L
#define C_REPLY_ALL	37L
#define C_REPLY_MENU	38L
#define C_RETAIN	39L
#define C_REVERSE	40L
#define C_SAVE_MSG	41L
#define C_SAVE_LIST	42L
#define C_SAVEOPTS	43L
#define C_PREV_SCREEN	44L
#define C_NEXT_SCREEN	45L
#define C_CONT_SEARCH	46L
#define C_PREV_SEARCH	47L
#define C_NEXT_SEARCH	48L
#define C_SHELL_ESC	49L
#define C_SORT		50L
#define C_REV_SORT	51L
#define C_SOURCE	52L
#define C_TOP_MSG	53L
#define C_TOP_PAGE	54L
#define C_UNBIND	55L
#define C_UNDEL_MSG	56L
#define C_UNDEL_LIST	57L
#define C_UPDATE	58L
#define C_USER_BUTTON	59L
#define C_VAR_SET	60L
#define C_VERSION	61L
#define C_WRITE_MSG	62L
#define C_WRITE_LIST	63L
#define C_HELP		64L	/* THIS MUST BE THE LAST ITEM */

struct cmd_map {
    /* long so glob_flags can be saved in mac_stack */
    long m_cmd;   /* the command this is mapped to  */
    char *m_str;  /* the string user types (cbreak) */
    char *x_str;  /* the string executed if a macro */
    struct cmd_map *m_next;
};

#ifdef CURSES

/*
 * Pointers to the current active command or macro and to the map list.
 *  This ought to be handled by having getcmd() return struct cmd_map *,
 *  but curses_command() depends too heavily on getcmd() returning int.
 */
extern struct cmd_map *active_cmd, *cmd_map;

#endif /* CURSES */

/* This must be OUTSIDE the #ifdef CURSES -- needed in other modes */
extern struct cmd_map *mac_hide;

/*
 * Special bracketing recognized within an executing
 *  macro as surrounding a fullscreen-mode function name
 */
#define MAC_LONG_CMD	'['
#define MAC_LONG_END	']'
#define MAC_GET_STR	"getstr"
#define MAC_GET_LINE	"getline"
#define MAX_LONG_CMD	32

/*
 * External declarations for map and map! purposes
 */
extern char *c_macro P((const char *, register char *, register struct cmd_map *));
extern struct cmd_map *line_map, *bang_map;

#endif /* _C_BIND_H_ */
