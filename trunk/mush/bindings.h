/* bindings.h  -- command bindings */

#define MAX_BIND_LEN 20   /* max length a string can be to bind to a command */
#define MAX_MACRO_LEN 256 /* max length of a macro bound to a command */

/* to see if a key sequence matches, prefixes or misses a set binding */
#define NO_MATCH	0
#define MATCH		1
#define A_PREFIX_B	2
#define B_PREFIX_A	3

/*
 * Constants to define curses mode functions.
 */
#ifdef NULL_MAP
#undef NULL_MAP
#endif /* NULL_MAP */
#define NULL_MAP	(struct cmd_map *)0

#define C_ERROR		(-1L)
#define C_NULL		0L
#define C_GOTO_MSG	1L
#define C_WRITE_LIST	2L
#define C_WRITE_MSG	3L
#define C_SAVE_LIST	4L
#define C_SAVE_MSG	5L
#define C_COPY_LIST	6L
#define C_COPY_MSG	7L
#define C_DELETE_LIST	8L
#define C_DELETE_MSG	9L
#define C_UNDEL_LIST	10L
#define C_UNDEL_MSG	11L
#define C_REDRAW	12L
#define C_REVERSE	13L
#define C_NEXT_MSG	14L
#define C_PREV_MSG	15L
#define C_FIRST_MSG	16L
#define C_LAST_MSG	17L
#define C_TOP_PAGE	18L
#define C_BOTTOM_PAGE	19L
#define C_NEXT_SCREEN	20L
#define C_PREV_SCREEN	21L
#define C_SOURCE	22L
#define C_SAVEOPTS	23L
#define C_NEXT_SEARCH	24L
#define C_PREV_SEARCH	25L
#define C_CONT_SEARCH	26L
#define C_PRESERVE	27L
#define C_REV_SORT	28L
#define C_SORT		29L
#define C_QUIT_HARD	30L
#define C_QUIT		31L
#define C_EXIT_HARD	32L
#define C_EXIT		33L
#define C_UPDATE	34L
#define C_FOLDER	35L
#define C_SHELL_ESC	36L
#define C_CURSES_ESC	37L
#define C_PRINT_MSG	38L
#define C_CHDIR		39L
#define C_VAR_SET	40L
#define C_IGNORE	41L
#define C_ALIAS		42L
#define C_OWN_HDR	43L
#define C_VERSION	44L
#define C_MAIL_FLAGS	45L
#define C_MAIL		46L
#define C_REPLY_ALL	47L
#define C_REPLY_SENDER	48L
#define C_DISPLAY_NEXT	49L
#define C_DISPLAY_MSG	50L
#define C_TOP_MSG	51L
#define C_BIND_MACRO	52L
#define C_BIND		53L
#define C_UNBIND	54L
#define C_MAP_BANG	55L
#define C_MAP		56L
#define C_MACRO		57L
#define C_MARK_MSG	58L
#define C_HELP		59L/* THIS MUST BE THE LAST ITEM */

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
 *  macro as surrounding a curses function name
 */
#define MAC_LONG_CMD	'['
#define MAC_LONG_END	']'
#define MAC_GET_STR	"getstr"
#define MAC_GET_LINE	"getline"
#define MAX_LONG_CMD	32

/*
 * External declarations for map and map! purposes
 */
extern char *c_macro();
extern struct cmd_map *line_map, *bang_map;
