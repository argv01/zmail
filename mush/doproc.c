/* @(#)doproc.c		(c) copyright	10/18/86 (Dan Heller) */

/* do main panel item procedures */
#include "mush.h"

extern void start_textsw_edit();
extern Panel get_compose_panel();
void set_comp_items();

extern Panel_item
    file_item, folder_text_item, folder_item, msg_num_item, read_item,
    reply_item, save_item, sub_hdr_item[6];

/* following macro is for the next two procedures */
#define hdr_item(item) \
    (item == sub_hdr_item[0] || item == sub_hdr_item[1] || \
     item == sub_hdr_item[2] || item == sub_hdr_item[3] || \
     item == sub_hdr_item[4] || item == sub_hdr_item[5])

void
delete_mail(item, value, event)
register Panel_item item;
int value;
register Event *event;
{
    int val = value; /* save cuz we reset value immediately */
    u_long bang = ison(glob_flags, IGN_BANG);
    char buf[128];

    (void) panel_set(item, PANEL_VALUE, 0, 0);
    if (hdr_item(item) && event_id(event) != MS_LEFT || val == 2) {
	help(0, "delete", tool_help);
	return;
    }
    /* delete current message */
    wprint(sprintf(buf, "\\%sdelete %s\n",
	((event_id(event) == MS_LEFT || val == 0)? "" : "un"),
	panel_get_value(msg_num_item)) + 1); /* +1 skips the backslash */
    turnon(glob_flags, IGN_BANG);
    (void) cmd_line(buf, msg_list);
    if (!bang)
	turnoff(glob_flags, IGN_BANG);
}

void
read_mail(item, value, event)
Panel_item item;
Event *event;
{
    int this_msg = current_msg;

    /* check "event" in case we were called from hdr_sw.c
     * in which case event would be NULL
     */
    if (event && event_id(event) == MS_RIGHT && item &&
	(item == read_item ||
	(item == sub_hdr_item[0] || item == sub_hdr_item[1]))) {
	(void) help(0, "next", tool_help);
	return;
    }
    if (item && (item == sub_hdr_item[4] || item == sub_hdr_item[5])) {
	(void) help(0, "Menu Read", tool_help);
	return;
    }
    if (!msg_cnt) {
	wprint ("No Mail.\n");
	return;
    }
    if (item && item == read_item || ison(msg[current_msg].m_flags, DELETE))
	(void) next_msg();
    if (this_msg != current_msg || ison(msg[current_msg].m_flags, UNREAD) ||
	    (current_msg < n_array[0] || current_msg > n_array[screen])) {
	set_isread(current_msg);
	(void) do_hdrs(0, DUBL_NULL, NULL);
    }
    if (isoff(msg[current_msg].m_flags, DELETE))
	display_msg(current_msg, (u_long)0);
}

/* the panel button that says "filename" and "directory", etc... text item */
Panel_setting
file_dir(item, event)
Panel_item item;
Event *event;
{
    register char *p;
    u_long bang = ison(glob_flags, IGN_BANG);
    char buf[MAXPATHLEN];

    if (event_id(event) == ESC) {
	/* file expansion request */
	int n;
	char **files;
	p = panel_get_value(item);
	(void) sprintf(buf, "%s*", p);
	timeout_cursors(1);
	if ((n = filexp(buf, &files)) > 0) {
	    Debug("%d: ",n), print_argv(files);
	    if (n > 1) {
		n = lcprefix(files, 0);
		files[0][n] = 0;
	    }
	    panel_set_value(item, trim_filename(files[0]));
	    free_vec(files);
	} else
	    errbell(n);	/* see curs_io.c */
	timeout_cursors(0);
	return PANEL_NONE;
    }

    if (item == folder_text_item) {
	(void) sprintf(buf, "folder %s %s",
	    (ison(glob_flags, READ_ONLY) || ison(glob_flags, DO_UPDATE) &&
		!ask("Folder has been modified.  Update changes?"))? "!" : "",
		panel_get_value(folder_text_item));
    }
    else if (item == file_item) {
	register char *b = buf;
	char msgstr[BUFSIZ];

	if (event_id(event) == '\n' || event_id(event) == '\r')
	    b += Strcpy(buf, "save  ");
	else
	    b += Strcpy(buf, "write ");
	if ((p = panel_get_value(msg_num_item)) && *p)
	    b += Strcpy(b, p);
	else {
	    if (ison(msg[current_msg].m_flags, DELETE) &&
		    !do_set(set_options, "show_deleted")) {
		(void) sprintf(msgstr, "Message %d deleted -- save anyway?",
					    current_msg+1);
		if (ask(msgstr) != TRUE) {
		    wprint("Message not saved\n");
		    return PANEL_NONE;
		}
	    }
	    b += strlen(sprintf(b, "%d", current_msg+1));
	}
	*b++ = ' ', *b = 0;
	if (!(p = panel_get_value(item)) || !*p &&
	    (!(p = do_set(set_options, "mbox")) || !*p))
		p = DEF_MBOX;
	if (chk_option("verify", "save")) {
	    (void) sprintf(msgstr, "%s in \"%s\"?", buf, trim_filename(p));
	    if (ask(msgstr) != TRUE) {
		wprint("Message not saved\n");
		return PANEL_NONE;
	    }
	}
	(void) strcpy(b, p); /* now add to command */
    }
    turnon(glob_flags, IGN_BANG);
    (void) cmd_line(buf, msg_list);
    if (!bang)
	turnoff(glob_flags, IGN_BANG);
    return PANEL_NONE;
}

#ifndef NO_WALK_MENUS
void
xx_file_dir(item, value)
Panel_item item;
char * value;
{
    char buf[BUFSIZ];
    u_long bang = ison(glob_flags, IGN_BANG);

    if (item == folder_item) {
	(void) sprintf(buf, "folder %s ",
	    (ison(glob_flags, READ_ONLY) || ison(glob_flags, DO_UPDATE) &&
	    !ask("Folder has been modified.  Update changes?"))? "!" : "");
	strcat(buf, value);
    } else if (item == save_item) {
	char msgstr[BUFSIZ], *p;
	register char *p2 = (char *)panel_get_value(msg_num_item);
	(void) strcpy(buf, "save ");

	if (p2 && *p2) {
	    (void) strcat(buf, p2);
	    (void) strcat(buf, " ");
	}
	(void) strcat(buf, value);
	if (chk_option("verify", "save")) {
	    (void) sprintf(msgstr, "Save in %s? ", trim_filename(value));
	    if (ask(msgstr) != TRUE) {
		wprint("Message not saved\n");
		return;
	    }
	}
    }
    turnon(glob_flags, IGN_BANG);
    (void) cmd_line(buf, msg_list);
    if (!bang)
	turnoff(glob_flags, IGN_BANG);
    (void) panel_set(item, PANEL_VALUE, 0, NULL); /* remove last value */
}

/*
 * callback routine for the panel items that need filename input.
 * (folder and save)
 */
void
do_file_dir(item, event)
Panel_item item;
Event *event;
{
    if (item == folder_item) {
	char *p = panel_get_value(folder_text_item);
	if (!*p) {
	    ok_box("Enter folder name.");
	    return;
	}
	xx_file_dir(item, p);
    } else if (item == save_item) {
	if (!strcmp("Filename:", panel_get(file_item,PANEL_LABEL_STRING))) {
	    event_id(event) = '\n';  /* let file_dir think it got a \n */
	    file_dir(file_item, event);
	    return;
	}
    }
    (void) panel_set(item, PANEL_VALUE, 0, NULL); /* remove last value */
}
#else /* NO_WALK_MENUS */

/*
 * callback routine for the panel items that need filename input.
 * (folder and save)
 */
void
do_file_dir(item, value, event)
Panel_item item;
int value;
Event *event;
{
    char buf[BUFSIZ];
    u_long bang = ison(glob_flags, IGN_BANG);

    if (item == folder_item) {
	(void) sprintf(buf, "folder %s ",
	    (ison(glob_flags, READ_ONLY) || ison(glob_flags, DO_UPDATE) &&
	    !ask("Folder has been modified.  Update changes?"))? "!" : "");
	if (event_id(event) == MS_LEFT) {
	    char *p = panel_get_value(folder_text_item);
	    if (!*p) {
		ok_box("Enter folder name.");
		return;
	    }
	    (void) strcat(buf, p);
	} else if (!value)
	    (void) strcat(buf, "%");
	else if (value == 1)
	    (void) strcat(buf, "&");
	else if (value == 2)
	    (void) strcat(buf, "#");
	else
	    (void) strcat(buf, panel_get(item, PANEL_CHOICE_STRING, value));
    } else if (item == save_item) {
	char msgstr[BUFSIZ], *p;
	(void) strcpy(buf, "save ");
	if (event_id(event) == MS_LEFT) {
	    if (!strcmp("Filename:", panel_get(file_item,PANEL_LABEL_STRING))) {
		event_id(event) = '\n';  /* let file_dir think it got a \n */
		file_dir(file_item, event);
		return;
	    }
	} else if (value == 0) {
	    register char *p2 = (char *)panel_get_value(msg_num_item);

	    if (!(p = do_set(set_options, "mbox")) || !*p)
		p = DEF_MBOX;
	    if (p2 && *p2) {
		(void) strcat(buf, p2);
		(void) strcat(buf, " ");
	    }
	    (void) strcat(buf, p);
	} else
	    (void) strcat(buf, p = panel_get(item, PANEL_CHOICE_STRING, value));
	if (chk_option("verify", "save")) {
	    (void) sprintf(msgstr, "Save in %s? ", trim_filename(p));
	    if (ask(msgstr) != TRUE) {
		wprint("Message not saved\n");
		return;
	    }
	}
    }
    turnon(glob_flags, IGN_BANG);
    (void) cmd_line(buf, msg_list);
    if (!bang)
	turnoff(glob_flags, IGN_BANG);
    (void) panel_set(item, PANEL_VALUE, 0, NULL); /* remove last value */
}
#endif /* NO_WALK_MENUS */

/*ARGSUSED*/
void
do_help(item, value, event)
Panel_item item;
register int value;
Event *event;
{
    register char *p, *helpfile = tool_help;
    if (!event || event_id(event) == MS_LEFT)
	value = 0;
    switch(value) {
	case 0: p = "about", helpfile = cmd_help;
	when 1: p = "help";
	when 2: p = "mouse";
	when 3: p = "windows";
	when 4: p = "hdr_format", helpfile = cmd_help;
	when 5: p = "msg_list", helpfile = cmd_help;
	when 6: p = "folder";
	otherwise: p = "general";
    }
    (void) help(0, p, helpfile);
}

/*ARGSUSED*/
void
do_update(item, value, event)
Panel_item item;
int value;
Event *event;
{
    char *argv[2];
    if (event && event_id(event) != MS_LEFT) {
	if (value == 0) {
	    if (check_new_mail() == 0)
		print("No new mail.\n");
	} else
	    (void) help(0, "update", tool_help);
	return;
    }
    argv[0] = "update";
    argv[1] = NULL;
    timeout_cursors(TRUE);
    (void) folder(0, argv, NULL);
    timeout_cursors(FALSE);
}

/*ARGSUSED*/
void
toolquit(item, value, event)
Panel_item item;
int value;
Event *event;
{
    void wmgr_changestate(), wmgr_changelevel();
    register int which;

    if (!value || event_id(event) == MS_LEFT) {
	if (ison(glob_flags, DO_UPDATE)) {
	    do_update(NO_ITEM, 0, NO_EVENT);
	    turnoff(glob_flags, NEW_MAIL);
	}
	check_icons();
	mail_status(0); /* lower flag (if up) print current num of msgs */
	/* wmgr_changestate (window_get(tool, WIN_FD), rootfd, TRUE); */
	/* wmgr_changelevel (window_get(tool, WIN_FD), parentfd, TRUE); */
	close_frame();
	window_set(tool, FRAME_CLOSED, TRUE, NULL);
	is_iconic = ((int) window_get(tool, FRAME_CLOSED));
	return;
    } else if (value == 2) {
	(void) help(0, "quit", tool_help);
	return;
    }
    /* modify this to check for "abort" choice when ternary return values
     * are possible!
     */
    if (isoff(glob_flags, DO_UPDATE) ||
	    ask("Folder has been modified -- update?")) {
	if (!copyback("Quit anyway?", TRUE))
	    return;
    }
    cleanup(0);
}

/*ARGSUSED*/
void
do_mark(item, value, event)
Panel_item item;
int value;
Event *event;
{
    if (event && (event_id(event) == MS_LEFT) || value == 0) {
	int msg_num = event? current_msg : (int)item;
	/* mark message */
	if (ison(msg[msg_num].m_flags, M_PRIORITY(0)))
	    turnoff(msg[msg_num].m_flags, M_PRIORITY(0));
	else
	    turnon(msg[msg_num].m_flags, M_PRIORITY(0));
	(void) do_hdrs(0, DUBL_NULL, NULL);
    } else if (value < 7) {
	/* set priority */
	char buf[90];
	(void) cmd_line(sprintf(buf, "mark -%c %s",
	    value < 6? value + 'A' - 1 : ' ',
	    panel_get_value(msg_num_item)), NULL);
    } else
	(void) help(0, "mark", tool_help);
    if (value != 7 && item)
	panel_set_value(item, 0);
}

/*ARGSUSED*/
void
do_lpr(item, value, event)
Panel_item item;
int value;
Event *event;
{
    char buf[128];

    if (event && (event_id(event) == MS_LEFT)) {
	wprint("Sending message %d to printer...\n", current_msg+1);
	(void) strcpy(buf, "lpr ");
	if (value)
	    (void) sprintf(buf, "lpr \"%s\"", panel_get_value(msg_num_item));
	timeout_cursors(TRUE);
	(void) cmd_line(buf, msg_list);
	timeout_cursors(FALSE);
    } else
	(void) help(0, "printer", tool_help);
}

/* panel selection button pressed to send a letter.
 * we've attached the sign panel item to this item to 1) avoid
 * using a global and 2) make it general enough so that multiple
 * compose windows can have multiple send_items and we can
 * identify which sign/fortune items are associated with this
 * particular letter.  The fortune item is attached to the sign
 * item.
 */
/*ARGSUSED*/
void
do_send(item, value, event)
Panel_item item;
int value;
register Event *event;
{
    Panel panel = (Panel)panel_get(item, PANEL_PARENT_PANEL);
    Panel_item sign_item = (Panel_item)panel_get(item, PANEL_CLIENT_DATA);
    Panel_item fortune_item =
	(Panel_item)panel_get(sign_item, PANEL_CLIENT_DATA);
    Textsw textsw = (Textsw)panel_get(panel, PANEL_CLIENT_DATA);
    char *argv[5], buf[64];
    char *file = (char *)window_get(textsw, TEXTSW_CLIENT_DATA);
    char *p, *oldsign = NULL, *oldfortune = NULL;

    if (textsw_store_file(textsw, file, 0, 0)) {
	error("Can't save to %s", file);
	return;
    }
    /* check if user changed variables before sending */
    if (p = do_set(set_options, "autosign"))
	oldsign = savestr(p);
    if (panel_get_value(sign_item) && !oldsign)
	cmd_line(strcpy(buf, "\\set autosign"), NULL);
    else if (!panel_get_value(sign_item) && oldsign)
	cmd_line(strcpy(buf, "\\unset autosign"), NULL);
    if (p = do_set(set_options, "fortune"))
	oldfortune = savestr(p);
    if (panel_get_value(fortune_item) && !oldfortune)
	(void) cmd_line(strcpy(buf, "\\set fortune"), NULL);
    else if (!panel_get_value(fortune_item) && oldfortune)
	(void) cmd_line(strcpy(buf, "\\unset fortune"), NULL);
    turnoff(glob_flags, IS_GETTING);
    argv[0] = "mail";
    argv[1] = "-Uh";
    argv[2] = file;
    argv[3] = NULL;
    clear_msg_list(msg_list);
    timeout_cursors(TRUE);
    if (do_mail(3, argv, msg_list) == 0) {
	(void) unlink(file);
	set_comp_items(panel);
    }
    if (panel_get_value(sign_item) && !oldsign)
	(void) cmd_line(strcpy(buf, "\\unset autosign"), NULL);
    else if (!panel_get_value(sign_item) && oldsign) {
	argv[0] = "set";
	argv[1] = "autosign";
	if (*oldsign) {
	    argv[2] = "=";
	    argv[3] = oldsign;
	    argv[4] = NULL;
	    (void) set(4, argv, NULL);
	} else {
	    argv[2] = NULL;
	    (void) set(2, argv, NULL);
	}
    }
    if (panel_get_value(fortune_item) && !oldfortune)
	cmd_line(strcpy(buf, "\\unset fortune"), NULL);
    else if (!panel_get_value(fortune_item) && oldfortune) {
	argv[0] = "set";
	argv[1] = "fortune";
	if (*oldfortune) {
	    argv[2] = "=";
	    argv[3] = oldfortune;
	    argv[4] = NULL;
	    (void) set(4, argv, NULL);
	} else {
	    argv[2] = NULL;
	    (void) set(2, argv, NULL);
	}
    }
    xfree(oldsign), xfree(oldfortune);
    timeout_cursors(FALSE);
}

/*ARGSUSED*/
void
do_include(item, value, event)
Panel_item item;
int value;
Event *event;
{
    extern FILE *ed_fp;
    char *p, buf[64], *file;
    Textsw textsw = (Textsw)panel_get(panel_get(item, PANEL_PARENT_PANEL),
	PANEL_CLIENT_DATA);

    if (event && event_id(event) == MS_LEFT)
	value = 0;
    if (value == 2) {
	(void) help(0, "include", tool_help);
	return;
    }
    p = panel_get_value(msg_num_item);
    (void) sprintf(buf, "%c%c%s", *escape, value == 0? 'i' : 'f', p? p : "");

    file = (char *)window_get(textsw, TEXTSW_CLIENT_DATA);
    if (textsw_store_file(textsw, file, 0, 0)) {
	(void) ask("Something's wrong... Click anything.");
	return;
    }
    if (ed_fp) {
	(void) ask("tmpfile already in use... Click anything.");
	(void) fclose(ed_fp);
    }
    if (!(ed_fp = mask_fopen(file, "a"))) {
	error("Cannot open %s to append msg.", file);
	return;
    }
    (void) add_to_letter(buf);
    (void) fclose(ed_fp), ed_fp = NULL_FILE;
#ifdef SUN_4_0 /* SunOS 4.0+ */
    window_set(textsw, TEXTSW_FILE_CONTENTS, file, NULL);
#else /* SUN_4_0 */
    textsw_load_file(textsw, file, 1, 0, 0);
#endif /* SUN_4_0 */
    window_set(textsw, TEXTSW_UPDATE_SCROLLBAR, NULL);
    (void) unlink(file);
}

/*ARGSUSED*/
void
do_compose(item, value, event)
Panel_item item;
int value;
Event *event;
{
    char buf[5];

    if (event && event_id(event) != MS_LEFT) {
	(void) help(0, "compose", tool_help);
	return;
    }
    open_compose();
    if (!compose_frame)
	return;	/* open failed */
    clear_msg_list(msg_list);
    if (do_mail(0, DUBL_NULL, msg_list) == 0) {
	Panel panel = get_compose_panel();
	Textsw textsw = (Textsw)panel_get(panel, PANEL_CLIENT_DATA);
	start_textsw_edit(textsw, TRUE);
	set_comp_items(panel);
    }
}

/*
 * notify proc for reply button -- also called from select.c (do_menu()) ,
 * in which case "event" is null and "value" contains the message
 * number of the message to reply to.
 */
/*ARGSUSED*/
void
respond_mail(item, value, event)
Panel_item item;
int value;
Event *event;
{
    int tmp = current_msg;
    char buf[256];

    if (event && event_id(event) == MS_LEFT)
	value = 0;
    if (event && value == 4) {
	(void) help(0, "respond", tool_help);
	return;
    }
    if (!msg_cnt) {
	wprint("No messages to respond to.\n");
	return;
    }
    if (ison(glob_flags, IS_GETTING)) {
	wprint("Finish editing current message first.\n");
	return;
    }
    if (!event)
	tmp = value, value = 0;
    open_compose();
    if (!compose_frame)
	return;	/* open failed */
    (void) sprintf(buf, "%s %s %d",
	(value == 2 || value == 3)? "\\replyall" : "\\replysender",
	(value == 1 || value == 3)? "-i": NO_STRING, tmp+1);
    if (cmd_line(buf, NULL) != -1) {
	Panel panel = get_compose_panel();
	Textsw textsw = (Textsw)panel_get(panel, PANEL_CLIENT_DATA);
	wprint("Responding to message %d\n", tmp+1);
	start_textsw_edit(textsw, FALSE);
	set_comp_items(panel);
    }
}

/*ARGSUSED*/
void
load_from_file(item, value, event)
Panel_item item;
int value;
Event *event;
{
    int x = 0;
    Textsw textsw;
    Panel_item filename_item = (Panel_item)panel_get(item, PANEL_CLIENT_DATA);
    char *file, *p = panel_get_value(filename_item);
#ifndef SUN_4_0 /* SunOS 4.0+ */
    char *sfile, buf[128];
    extern FILE *ed_fp;
#endif /* SUN_4_0 */
    
    if (!*p) {
	wprint("Specify Filename.\n");
	return;
    }
    file = getpath(p, &x);
    if (x == 1)
	wprint("%s: is a directory.\n", p);
    else if (x == -1)
	wprint("%s: %s\n", p, file);
    if (x)
	return;
    timeout_cursors(TRUE);
    textsw = (Textsw)panel_get(panel_get(item, PANEL_PARENT_PANEL),
	PANEL_CLIENT_DATA);
    if (event_id(event) != MS_LEFT && value == 1)
	/* replace */
	textsw_load_file(textsw, file, 1, 0, 0);
    else {
	/* insert */
#ifdef SUN_4_0 /* SunOS 4.0+ */
	window_set(textsw, TEXTSW_INSERT_FROM_FILE, file, NULL);
#else /* SUN_4_0 */
	/* best we can do with pre 4.0 is save the current file
	 * and append the new file onto the end.
	 */
	sfile = (char *)window_get(textsw, TEXTSW_CLIENT_DATA);
	if (textsw_store_file(textsw, sfile, 0, 0)) {
	    (void) ask("Can't save file... Click anything.");
	    return;
	}
	if (ed_fp) {
	    (void) ask("tmpfile already in use... Click anything.");
	    fclose(ed_fp);
	}
	if (!(ed_fp = mask_fopen(sfile, "a"))) {
	    error("Cannot open %s.", sfile);
	    return;
	}
	(void) sprintf(buf, "%c%c%s", *escape, 'r', trim_filename(p));
	(void) add_to_letter(buf);
	(void) fclose(ed_fp), ed_fp = NULL_FILE;
	textsw_load_file(textsw, sfile, 1, 0, 0);
	(void) unlink(sfile);
#endif /* SUN_4_0 */
    }
    window_set(textsw, TEXTSW_UPDATE_SCROLLBAR, NULL);
    panel_set_value(item, 0);
    timeout_cursors(FALSE);
}

/*ARGSUSED*/
void
save_to_file(item, value, event)
Panel_item item;
Event *event;
{
    Panel_item filename_item = panel_get(item, PANEL_CLIENT_DATA);
    char *file = panel_get_value(filename_item);
    FILE *fp;
    Textsw textsw = (Textsw)panel_get(panel_get(item, PANEL_PARENT_PANEL),
	PANEL_CLIENT_DATA);

    if (!*file) {
	wprint("Specify Filename\n");
	return;
    }
    timeout_cursors(TRUE);
    /* append to file -- no confirmation necessary */
    if (fp = open_file(file, FALSE, TRUE)) {
	char buf[BUFSIZ];
	Textsw_index next_pos = 0, tmp;
	Textsw_index length =
	    (Textsw_index)window_get(textsw, TEXTSW_LENGTH);
	do  {
	    tmp = next_pos;
	    next_pos = (Textsw_index) window_get(textsw, TEXTSW_CONTENTS,
		next_pos, buf, sizeof(buf));
	    if (fwrite(buf, sizeof(char), (int)(next_pos - tmp), fp) == 0)
		error("%s may be incomplete", file);
	} while (next_pos < length);
	(void) close_lock(file, fp);
	wprint("Wrote %d bytes to %s\n", length, trim_filename(file));
    }
    timeout_cursors(FALSE);
}

void
abort_mail(item, event)
Panel_item item;
Event *event;
{
    Panel panel = (Panel)panel_get(item, PANEL_PARENT_PANEL);
    Textsw textsw = (Textsw)panel_get(panel, PANEL_CLIENT_DATA);
    wprint("Aborted letter.\n");
    textsw_reset(textsw, 0, 0);
    rm_edfile(0);
    set_comp_items(panel);
}

/* set the compose panel items */
void
set_comp_items(panel)
Panel panel;
{
    Panel_item item, next;
    Textsw textsw = (Textsw)panel_get(panel, PANEL_CLIENT_DATA);
    int getting = ison(glob_flags, IS_GETTING) != 0;
    int i = 0;

    window_set(textsw, TEXTSW_READ_ONLY, !getting, NULL);
    /* remove next line when multiple composes become a reality */
    (void) panel_set(reply_item, PANEL_SHOW_ITEM, !getting, NULL);
    /* skip "close" item */
    item = (Panel_item) panel_get(panel, PANEL_FIRST_ITEM);
    for (item = (Panel_item) panel_get(item, PANEL_NEXT_ITEM);
	item; item = next) {
	next = (Panel_item) panel_get(item, PANEL_NEXT_ITEM);
	(void) panel_set(item,
	    PANEL_SHOW_ITEM, (i++ < 1)? !getting : getting, NULL);
    }
}

/*
 * Ask a yes/no question and return an answer: TRUE or FALSE.
 */
ask(question)
char *question;
{
#ifdef SUN_4_0 /* SunOS 4.0+ */
    return alert_prompt(tool, (Event *)NULL,
	ALERT_MESSAGE_STRINGS,	question, NULL,
	ALERT_BUTTON_YES,	"Yes",
	ALERT_BUTTON_NO,	"No",
	NULL) == ALERT_YES;
#else /* SUN_4_0 */
    Event event;
    struct prompt prompt;
    Rect *rect = (Rect *)window_get(tool, WIN_RECT);
    char buf[MAXPATHLEN];

    (void) sprintf(buf,
	    "%s  \nPress LEFT Button to Confirm.  Anything else to cancel.",
	    question);
    prompt.prt_rect.r_left = rect->r_left + (rect->r_width / 3);
    prompt.prt_rect.r_top = rect->r_top + (rect->r_height / 3);
    prompt.prt_rect.r_width = prompt.prt_rect.r_height = PROMPT_FLEXIBLE;
    prompt.prt_font = mush_font;
    prompt.prt_text = buf;

    menu_prompt(&prompt, &event, window_get(tool, WIN_FD));
    return event_id(&event) == MS_LEFT;
#endif /* SUN_4_0 */
}

void
ok_box(buf)
char *buf;
{
#ifdef SUN_4_0
    (void) alert_prompt(tool, (Event *)NULL,
	ALERT_MESSAGE_STRINGS,	buf, NULL,
	ALERT_BUTTON_YES,		"Ok",
	NULL);
#else /* SUN_4_0 */
    Event event;
    struct prompt prompt;
    Rect *rect = (Rect *)window_get(tool, WIN_RECT);
    (void) strcat(buf, "  \nPress LEFT Button to Continue.");
    prompt.prt_rect.r_left = rect->r_left + (rect->r_width / 3);
    prompt.prt_rect.r_top = rect->r_top + (rect->r_height / 3);
    prompt.prt_rect.r_width = prompt.prt_rect.r_height = PROMPT_FLEXIBLE;
    prompt.prt_font = mush_font;
    prompt.prt_text = buf;
    menu_prompt(&prompt, &event, window_get(tool, WIN_FD));
#endif /* SUN_4_0 */
}

Panel_setting
msg_num_done(item, event)
Panel_item item;
Event *event;
{
    char buf[82];
    u_long bang = ison(glob_flags, IGN_BANG);
    register char *p;
    int n;

    if (event_id(event) != '\n' && event_id(event) != '\r') {
	(void) help(0, "message range", tool_help);
	return PANEL_NONE;
    }
    (void) sprintf(buf, "headers %s", (p = (char *)panel_get_value(item)));
    (void) panel_set(item, PANEL_VALUE, NO_STRING, NULL);
    if (!(n = chk_msg(p)))
	return PANEL_NONE;
    current_msg = --n;
    turnon(glob_flags, IGN_BANG);
    (void) cmd_line(buf, msg_list);
    if (!bang)
	turnoff(glob_flags, IGN_BANG);
    (void) display_msg(n, (u_long)0);
    return PANEL_NONE;
}

void
do_sort(item, value, event)
Panel_item item;
int value;
Event *event;
{
    char *argv[3], list[MAXMSGS_BITS];
    char *p = (char *)panel_get_value(msg_num_item);
    int n = 0;

    if (p && *p) {
	argv[0] = p;
	argv[1] = NULL;
	n = get_msg_list(argv, list);
    }
    argv[0] = "sort";
    argv[2] = NULL;

    if (event_id(event) == MS_LEFT)
	value = 0;
    switch(value) {
	case 0: argv[1] = "S";
	when 1: argv[1] = "a";
	when 2: argv[1] = "l";
	when 3: argv[1] = "R";
	when 4: argv[1] = "s";
	when 5: argv[1] = "d";
	when 6: argv[1] = "p";
	when 7: if (!(argv[1] = do_set(set_options, "sort"))) return;
	when 8: (void) help(0, "sort", tool_help);
    }
    if (value != 8) {
	if (n > 0) {
	    turnon(glob_flags, IS_PIPE);
	    (void) sort(2, argv, list);
	    turnoff(glob_flags, IS_PIPE);
	} else
	    (void) sort(2, argv, NULL);
	(void) do_hdrs(0, DUBL_NULL, NULL);
    }
    (void) panel_set(item, PANEL_VALUE, 0, NULL);
}

void
do_options(item, value, event)
Panel_item item;
int value;
Event *event;
{
    if (event_id(event) == MS_LEFT) {
	view_options();
	return;
    }
    switch (value) {
	case 0:
	    view_options();
	when 1:
	    do_ignore();
	when 2:
	    do_aliases();
    }
}
