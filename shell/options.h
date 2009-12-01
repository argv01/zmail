/* options.h    Copyright 1990, 1991 Z-Code Software Corporation */

#ifndef _OPTIONS_H_
#define _OPTIONS_H_

/* Must #include zmail.h before #including this file */

/* Structure to hold assorted information collected from command line flags.  
 *  Other information is held in the following global variables:
 *	cmd_help	General help file, specified by -1
 *	debug		Debugging mode, toggled by -d
 *	glob_flags	Bits set by -C, -E, -i, -S, -t and many commands
 *	hdrs_only	Show headers and exit, specified by -H
 *	iscurses	Curses mode, specified by -V or "fullscreen" command
 *	istool		Tool mode, specified by -t or -T
 *	mailfile	File specified by -u or -f or "folder" command
 *	prog_name	Name under which zmail is running
 *	passive_timeout	Tool mode timeout, specified by -T
 *	tool_help	Tool mode help file, specified by -2
 */

struct zc_flags {
    u_long flg;		/* Set by -v, -h, -U, vars */
    char *init_file;	/* Set by -I or -I! */
    char *src_file;	/* Set by -F */
    int src_n_exit;	/* Set by -F! or -e! */
    char **src_cmds;	/* Set by -e */
    char *folder;	/* Set by -f or -u */
    char *draft;	/* Set by -h */
    char f_flags[20];	/* Set by -r, -N, etc.; passed to folder() */
    char *Subj;		/* Set by -s */
    char *Cc;		/* Set by -c */
    char *Bcc;		/* Set by -b */
    char *attach;	/* Set by -A */
    int source_rc;	/* Set by -n */
    int filter_mode;	/* Set by -f - */
    int use_template;	/* Set by -p */
};

extern int make_tool P((struct zc_flags *));
extern void fix_word_flag P((register char **argp, register char *(*)[2]));
extern void preparse_opts P((int *, char **));
extern void parse_options P((char ***, struct zc_flags *));

#endif /* _OPTIONS_H_ */
