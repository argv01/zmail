/* @(#)options.h    (c) copyright 10/10/88 (Dan Heller, Bart Schaefer) */

/* Must #include mush.h before #including this file */

/* Structure to hold assorted information collected from command line flags.  
 *  Other information is held in the following global variables:
 *	cmd_help	General help file, specified by -1
 *	debug		Debugging mode, toggled by -d
 *	glob_flags	Bits set by -C, -e, -i, -S, -t and many commands
 *	hdrs_only	Show headers and exit, specified by -H
 *	iscurses	Curses mode, specified by -C or "curses" command
 *	istool		Tool mode, specified by -t or -T
 *	mailfile	File specified by -u or -f or "folder" command
 *	prog_name	Name under which mush is running
 *	time_out	Tool mode timeout, specified by -T
 *	tool_help	Tool mode help file, specified by -2
 */

struct mush_flags {
    u_long flg;		/* Set by -v, -h, -U, vars */
    char *init_file;	/* Set by -I or -I! */
    char *src_file;	/* Set by -F */
    int src_n_exit;	/* Set by -F! */
    char *folder;	/* Set by -f or -u */
    char *draft;	/* Set by -h */
    char f_flags[10];	/* Set by -r, -N, etc.; passed to folder() */
    char *Subj;		/* Set by -s */
    char *Cc;		/* Set by -c */
    char *Bcc;		/* Set by -b */
    int source_rc;	/* Set by -n */
#ifdef POP3_SUPPORT
    int pop3;		/* Set by -p and -P */
#endif
};
