/* @(#)panel.c	(c) copyright	10/18/86 (Dan Heller) */
/* @(#)panels.c	(c) copyright	9/29/89 (Dan Heller) */

#include "mush.h"
/* mouse symbols */
short dat_mouse_left[] = {
    0x1FF8, 0x3FFC, 0x336C, 0x336C, 0x336C, 0x336C, 0x336C, 0x336C, 
    0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FC4, 0x3FFC, 0x1FF8
};

short dat_mouse_middle[] = {
    0x1FF8, 0x3FFC, 0x366C, 0x366C, 0x366C, 0x366C, 0x366C, 0x366C, 
    0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FC4, 0x3FFC, 0x1FF8
};

short dat_mouse_right[] = {
    0x1FF8, 0x3FFC, 0x36CC, 0x36CC, 0x36CC, 0x36CC, 0x36CC, 0x36CC, 
    0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FFC, 0x3FC4, 0x3FFC, 0x1FF8
};

mpr_static(mouse_left,      16, 16, 1, dat_mouse_left);
mpr_static(mouse_middle,    16, 16, 1, dat_mouse_middle);
mpr_static(mouse_right,     16, 16, 1, dat_mouse_right);

Panel_item
    folder_item,	/* change folders */
    folder_text_item,	/* text item for the folder item */
    file_item, 		/* text item for the save item */
    msg_num_item,	/* text item explicitly states which message to read */
    read_item,		/* read the current message */
    save_item,		/* saves messages */
    sub_hdr_item[6];	/* display items that just sit there and give help */

#ifndef NO_WALK_MENUS
Panel
    folder_panel,
    save_panel;
Menu	  folder_menu;	/* Menu of folders for folder button */
Menu	  save_menu;	/* Menu of folders for save button */
Menu	  hdr_save_menu;/* Menu of folders for save option in hdr window */
extern Menu msg_menu;	/* header subwindow menu, defined in hdr_sw.c */
walk_menu_event();
#endif /* NO_WALK_MENUS */

/* These global panel items for letter composition should eventually go away */
Panel_item
    edit_item,		/* edit a message */
    reply_item;		/* reply button -- also called from hdr_sw menu */

extern void
    close_frame(), do_options(), do_compose(), do_send(), do_sort(),
    do_edit(), delete_mail(), respond_mail(), do_help(), do_lpr(),
    do_update(), abort_mail(), do_include(), load_from_file(),
    save_to_file(), tilde_from_menu(), fkey_interposer(), do_mark(),
    close_compose();

extern Panel_setting
    msg_num_done(), file_dir();

Panel
make_hdr_panel(parent, choice_args, button_args)
Frame parent;
char **choice_args, **button_args;
{
    Panel panel = window_create(parent, PANEL,
	WIN_CONSUME_KBD_EVENTS,
	    WIN_LEFT_KEYS, WIN_TOP_KEYS, WIN_RIGHT_KEYS, NULL,
	NULL);
    (void) notify_interpose_event_func(panel, fkey_interposer, NOTIFY_SAFE);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Done", 4, mush_font),
	PANEL_CHOICE_STRINGS,
	    "Close to Icon", "Quit Tool", "Help", NULL,
	PANEL_NOTIFY_PROC, 		toolquit,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Help", 4, mush_font),
	PANEL_CHOICE_STRINGS,
	    "About", "Help with \"help\"", "The Mouse", "Windows",
	    "Message headers", "Message lists", "Folders", NULL,
	PANEL_NOTIFY_PROC, 		do_help,
	NULL);

#ifndef NO_WALK_MENUS
    folder_panel = panel;
    folder_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Folder", 6, mush_font),
	PANEL_NOTIFY_PROC, 		do_file_dir,
	PANEL_EVENT_PROC,		walk_menu_event,
	NULL);
#else /* NO_WALK_MENUS */
    folder_item = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Folder", 6, mush_font),
	PANEL_CHOICE_STRINGS,
	    "System Mailbox", "Main Mailbox", "Last Accessed Folder", NULL,
	PANEL_NOTIFY_PROC, 		do_file_dir,
	NULL);

    add_folder_to_menu(folder_item, 3);
#endif /* NO_WALK_MENUS */

    folder_text_item = panel_create_item(panel, PANEL_TEXT,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_FONT, 		mush_font,
	PANEL_LABEL_STRING,		"Filename:",
	PANEL_VALUE_DISPLAY_LENGTH, 	28,
	PANEL_NOTIFY_STRING, 		"\n\r\033",
	PANEL_NOTIFY_PROC, 		file_dir,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Update", 6, mush_font),
	PANEL_CHOICE_STRINGS, 		"New Mail", "Help", NULL,
	PANEL_NOTIFY_PROC, 		do_update,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Options", 7, mush_font),
	PANEL_CHOICE_STRINGS,		"Variables", "Headers", "Aliases", NULL,
	PANEL_NOTIFY_PROC, 		do_options,
	NULL);

    msg_num_item = panel_create_item(panel, PANEL_TEXT,
	PANEL_ATTRIBUTE_LIST,		choice_args,
	PANEL_LABEL_STRING,		"Range:",
	PANEL_MENU_CHOICE_STRINGS, 	"Help", NULL,
	PANEL_VALUE_DISPLAY_LENGTH, 	17,
	PANEL_VALUE_STORED_LENGTH, 	80,
	PANEL_LABEL_FONT, 		mush_font,
	PANEL_NOTIFY_STRING, 		"\n\r",
	PANEL_NOTIFY_PROC, 		msg_num_done,
	NULL);

    sub_hdr_item[0] = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,    		&mouse_left,
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC,    		read_mail,
	NULL);
    sub_hdr_item[1] = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_STRING,   		"Read ",
	PANEL_MENU_TITLE_IMAGE, 	&mouse_left,
	PANEL_CHOICE_STRINGS,		"Help", NULL,
	PANEL_NOTIFY_PROC,    		read_mail,
	NULL);
    sub_hdr_item[2] = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,    		&mouse_middle,
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC,    		delete_mail,
	NULL);
    sub_hdr_item[3] = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_STRING,   		"Delete ",
	PANEL_MENU_TITLE_IMAGE, 	&mouse_middle,
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC,    		delete_mail,
	NULL);
    sub_hdr_item[4] = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,    		&mouse_right,
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC,    		read_mail,
	NULL);
    sub_hdr_item[5] = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_STRING,   		"Menu   ",
	PANEL_MENU_TITLE_IMAGE,	    	&mouse_right,
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC,    		read_mail,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Mark", 4, mush_font),
	PANEL_CHOICE_STRINGS,
	    "Toggle Mark", "Priority A", "Priority B", "Priority C",
	    "Priority D", "Priority E", "Clear Priority", "Help", NULL,
	PANEL_NOTIFY_PROC, 		do_mark,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Printer", 7, mush_font),
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC, 		do_lpr,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Sort", 4, mush_font),
	PANEL_CHOICE_STRINGS,
	    "By Status", "By Author", "By Size", "By Subject",
	    "By Subject (ignore Re:)", "By Date",
	    "By Priority", "Value of $sort", "Help", NULL,
	PANEL_NOTIFY_PROC, 		do_sort,
	NULL);

    window_fit_height(panel);
    return panel;
}

Panel
make_main_panel(parent, choice_args, button_args)
Frame parent;
char **choice_args, **button_args;
{
    /* main panel stuff: */
    Panel panel = window_create(parent, PANEL,
	WIN_CONSUME_KBD_EVENTS,
	    WIN_LEFT_KEYS, WIN_TOP_KEYS, WIN_RIGHT_KEYS, NULL,
	NULL);
    (void) notify_interpose_event_func(panel, fkey_interposer, NOTIFY_SAFE);

    read_item = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Next", 4, mush_font),
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC, 		read_mail,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Delete", 6, mush_font),
	PANEL_CHOICE_STRINGS, 		"Delete",
					"Undelete",
					"Help", NULL,
	PANEL_NOTIFY_PROC, 		delete_mail,
	NULL);

#ifndef NO_WALK_MENUS
    save_panel = panel;
    save_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_LABEL_IMAGE, panel_button_image(panel, "Save", 4, mush_font),
	PANEL_NOTIFY_PROC, 		do_file_dir,
	PANEL_EVENT_PROC,		walk_menu_event,
	NULL);

    create_folder_menus();
#else /* NO_WALK_MENUS */
    {
	char *mbox = do_set(set_options, "mbox");
	if (!mbox || !*mbox)
	    mbox = DEF_MBOX;
	save_item = panel_create_item(panel, PANEL_CHOICE,
	    PANEL_ATTRIBUTE_LIST, 	choice_args,
	    PANEL_LABEL_IMAGE, panel_button_image(panel, "Save", 4, mush_font),
	    PANEL_CHOICE_STRINGS, 	trim_filename(mbox), NULL,
	    PANEL_NOTIFY_PROC, 		do_file_dir,
	    NULL);
    }

    add_folder_to_menu(save_item, 1);
#endif /* NO_WALK_MENUS */

    file_item = panel_create_item(panel, PANEL_TEXT,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_FONT, 		mush_font,
	PANEL_SHOW_MENU,		TRUE,
	PANEL_LABEL_STRING, 		"Filename:",
	PANEL_MENU_CHOICE_STRINGS,	"Save message without message header",
					NULL,
	PANEL_VALUE_DISPLAY_LENGTH, 	28,
	PANEL_NOTIFY_STRING, 		"\n\r\033",
	PANEL_NOTIFY_PROC, 		file_dir,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Compose", 7, mush_font),
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC,		do_compose,
	NULL);

    reply_item = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Reply", 5, mush_font),
	PANEL_CHOICE_STRINGS,
	    "Sender Only", "Sender Only (include msg)",
	    "All Recipients", "All Recipients (include msg)", "Help", NULL,
	PANEL_NOTIFY_PROC, 		respond_mail,
	NULL);

    window_fit_height(panel);
    return panel;
}

Panel
make_compose_panel(parent, choice_args, button_args)
Frame parent;
char **choice_args, **button_args;
{
    Panel panel = window_create(parent, PANEL, NULL);
    Panel_item filename_item, fortune_item, sign_item, send_item;

    (void) notify_interpose_event_func(panel, fkey_interposer, NOTIFY_SAFE);

    if (do_set(set_options, "compose_icon"))
	(void) panel_create_item(panel, PANEL_CHOICE,
	    PANEL_ATTRIBUTE_LIST, 		choice_args,
	    PANEL_LABEL_IMAGE,
		panel_button_image(panel, "Close", 5, mush_font),
	    PANEL_CHOICE_STRINGS,
		"Close to Icon", "Quit", "Help", NULL,
	    PANEL_NOTIFY_PROC, 		close_compose,
	    NULL);
    else
	(void) panel_create_item(panel, PANEL_BUTTON,
	    PANEL_ATTRIBUTE_LIST, 		button_args,
	    PANEL_LABEL_IMAGE,
		panel_button_image(panel, "Close", 5, mush_font),
	    PANEL_NOTIFY_PROC, 		close_frame,
	    NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Compose", 7, mush_font),
	PANEL_CHOICE_STRINGS, 		"Help", NULL,
	PANEL_NOTIFY_PROC, 		do_compose,
	NULL);

    send_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_SHOW_ITEM, 		FALSE,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Send", 4, mush_font),
	PANEL_NOTIFY_PROC, 		do_send,
	NULL);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_SHOW_ITEM,		FALSE,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Abort", 5, mush_font),
	PANEL_NOTIFY_PROC, 		abort_mail,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_SHOW_ITEM, 		FALSE,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Include", 7, mush_font),
	PANEL_CHOICE_STRINGS, 		"Include Message",
					"Forward Message",
					"Help", NULL,
	PANEL_NOTIFY_PROC, 		do_include,
	NULL);

    edit_item = panel_create_item(panel, PANEL_BUTTON,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_SHOW_ITEM, 		FALSE,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Edit", 4, mush_font),
	PANEL_NOTIFY_PROC, 		do_edit,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_SHOW_ITEM,		FALSE,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Headers", 7, mush_font),
	PANEL_CHOICE_STRINGS,	
	    "ALL      ~h", "To:      ~t", "Subject: ~s",
	    "Cc:      ~c", "Bcc:     ~b", "Fcc:", NULL,
	PANEL_NOTIFY_PROC,		tilde_from_menu,
	NULL);

    sign_item = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_DISPLAY_LEVEL,		PANEL_ALL,
	PANEL_SHOW_MENU_MARK,		TRUE,
	PANEL_SHOW_ITEM, 		FALSE,
	PANEL_LABEL_STRING,		"Autosign:",
	PANEL_CHOICE_STRINGS,		"Off", "On", NULL,
	NULL);
    panel_set_value(sign_item, !!do_set(set_options, "autosign"));
    /* Create a link to avoid global */
    panel_set(send_item, PANEL_CLIENT_DATA, sign_item, NULL);

    filename_item = panel_create_item(panel, PANEL_TEXT,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_SHOW_ITEM,		FALSE,
	PANEL_LABEL_STRING,		"Filename:",
	PANEL_VALUE_DISPLAY_LENGTH, 	30,
	PANEL_NOTIFY_STRING, 		"\033",
	PANEL_NOTIFY_PROC, 		file_dir,
	NULL);

    (void) panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_SHOW_ITEM,		FALSE,
	PANEL_CLIENT_DATA,		filename_item,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Import", 6, mush_font),
	PANEL_CHOICE_STRINGS,		"Insert", "Replace", NULL,
	PANEL_NOTIFY_PROC,		load_from_file,
	NULL);

    (void) panel_create_item(panel, PANEL_BUTTON,
	PANEL_ATTRIBUTE_LIST, 		button_args,
	PANEL_SHOW_ITEM,		FALSE,
	PANEL_CLIENT_DATA,		filename_item,
	PANEL_LABEL_IMAGE,
	    panel_button_image(panel, "Export", 6, mush_font),
	PANEL_NOTIFY_PROC,		save_to_file,
	NULL);

    fortune_item = panel_create_item(panel, PANEL_CHOICE,
	PANEL_ATTRIBUTE_LIST, 		choice_args,
	PANEL_DISPLAY_LEVEL,		PANEL_ALL,
	PANEL_SHOW_MENU_MARK,		TRUE,
	PANEL_SHOW_ITEM, 		FALSE,
	PANEL_LABEL_STRING,		"Fortune:",
	PANEL_CHOICE_STRINGS,		"Off", "On", NULL,
	NULL);
    panel_set_value(fortune_item, !!do_set(set_options, "fortune"));
    /* Create a link to avoid global */
    panel_set(sign_item, PANEL_CLIENT_DATA, fortune_item, NULL);

    window_fit_height(panel);
    return panel;
}

#include "glob.h"

#ifndef NO_WALK_MENUS

static
Menu_item
make_folder_item(path)
char *path;
{
    Menu_item		mi;
    Menu_item		sub_mi;
    Menu		next_menu;
    char		**names, **np;
    struct stat 	s_buf;
    char		buf[MAXPATHLEN];

    if (glob(path, "*/{.,..}")) {
	return NULL;
    }

    /* don't add a folder to the list if user can't read it */
    if (stat(path, &s_buf) == -1 || !(s_buf.st_mode & S_IREAD)) {
	return NULL;
    }
    mi = menu_create_item(
	MENU_STRING,		savestr(basename(trim_filename(path))),
	MENU_CLIENT_DATA,	NULL,
	MENU_RELEASE,		/* no value */
	NULL);
    if ((s_buf.st_mode & S_IFMT) == S_IFDIR) {
	int cnt = 0;
	next_menu = menu_create(MENU_NOTIFY_PROC, menu_return_item, NULL);
	sprintf(buf, "%s/{.*,*}", path);
	if (filexp(buf, &names) > 0) {
	    for (np = names; np && *np; np++) {
		if ((sub_mi = make_folder_item(*np)) != NULL) {
		    menu_set(next_menu, MENU_APPEND_ITEM, sub_mi, NULL);
		    ++cnt;
		}
	    }
	    free_vec(names);
	}
	if (! cnt) {
	    menu_destroy(next_menu);
	    menu_set(mi, MENU_INACTIVE, TRUE, NULL);
	} else {
	    menu_set(mi, MENU_PULLRIGHT, next_menu, NULL);
	}
    } else if (test_folder(path, NULL)) {
	menu_set(mi, MENU_CLIENT_DATA, savestr(path), NULL);
    } else {
	menu_destroy(mi);
	mi = NULL;
    }
    return mi;
}

static
void
destroy_folder_item(menu_item, menu_type)
Menu_item menu_item;
Menu_attribute menu_type;
{
    char	*ptr;

    if (menu_type == MENU_ITEM) {
	if ((ptr = (char *)menu_get(menu_item, MENU_STRING)) != NULL) {
	    free(ptr);
	}
	if ((ptr = (char *)menu_get(menu_item, MENU_CLIENT_DATA)) != NULL) {
	    free(ptr);
	}
    }
    return;
}

create_folder_menus()
{
    int 	  item_number;
    Menu	  menu;
    Menu_item	  menu_item;
    int 	  nitems;
    char	 *mbox;
    char	 *tmp = NULL;
    char	 *p;
    static int	  menus_exist = 0;

    if (menus_exist) {
	/* remove duplicated menu items from save_menu */
	for (item_number = (int)menu_get(save_menu, MENU_NITEMS) ;
	      item_number > 1 ; --item_number) {
	    menu_set(save_menu, MENU_REMOVE, item_number, NULL);
	}
	/* remove duplicated menu items from hdr_save_menu */
	for (item_number = (int)menu_get(hdr_save_menu, MENU_NITEMS) ;
	      item_number > 1 ; --item_number) {
	    menu_set(save_menu, MENU_REMOVE, item_number, NULL);
	}
	menu_destroy_with_proc(hdr_save_menu, destroy_folder_item);
	menu_destroy_with_proc(save_menu, destroy_folder_item);
	menu_destroy_with_proc(folder_menu, destroy_folder_item);
    }

    if (!(p = do_set(set_options, "folder")) || !*p) {
	p = DEF_FOLDER;
    }
    if (p) {
	int x = 0;
	tmp = getpath(p, &x);
	if (x == -1) {
	    if (errno != ENOENT)
		print("%s: %s\n", p, tmp);
	    tmp = NULL;
	}
    }
    menu = NULL;
    menu_item = NULL;
    if (tmp != NULL) {
	if ((menu_item = make_folder_item(tmp)) != NULL) {
	    if ((menu = menu_get(menu_item, MENU_PULLRIGHT)) != NULL) {
	        /* $folder was a directory, use the pullright
		 * instead of the directory menu item.
		 */
		/* "unhook" the pullright (so it is not released) */
		menu_set(menu_item, MENU_PULLRIGHT, NULL, NULL);
		/* destroy the menu item */
		menu_destroy_with_proc(menu_item, destroy_folder_item);
		menu_item = NULL;
	    }
	}
    }
    if (menu == NULL) {
	menu = menu_create(MENU_NOTIFY_PROC, menu_return_item, NULL);
	if (menu_item != NULL) {
	    menu_set(menu, MENU_APPEND_ITEM, menu_item, NULL);
	}
    }

    /* create save_menu */
    save_menu = menu_create(MENU_NOTIFY_PROC, menu_return_item, NULL);
    /* add magic first item */
    mbox = do_set(set_options, "mbox");
    if (!mbox || !*mbox) {
	mbox = DEF_MBOX;
    }
    menu_item = menu_create_item(
	MENU_STRING,		savestr(trim_filename(mbox)),
	MENU_CLIENT_DATA,	savestr(mbox),
	MENU_RELEASE,		/* no value */
	NULL);
    menu_set(save_menu, MENU_APPEND_ITEM, menu_item, NULL);
    /* copy menu for save_menu */
    nitems = (int)menu_get(menu, MENU_NITEMS);
    for (item_number = 1 ; item_number <= nitems ; ++item_number) {
	menu_set(save_menu,
	    MENU_APPEND_ITEM, menu_get(menu, MENU_NTH_ITEM, item_number),
	    NULL);
    }

    /* create hdr_save_menu */
    hdr_save_menu = menu_create(MENU_NOTIFY_PROC, menu_return_item, NULL);
    /* add magic first item */
    menu_item = menu_create_item(
	MENU_STRING,		savestr("use Filename:"),
	MENU_CLIENT_DATA,	savestr(""),	/* magic */
	MENU_RELEASE,		/* no value */
	NULL);
    menu_set(hdr_save_menu, MENU_APPEND_ITEM, menu_item, NULL);
    /* copy save_menu for hdr_save_menu */
    nitems = (int)menu_get(save_menu, MENU_NITEMS);
    for (item_number = 1 ; item_number <= nitems ; ++item_number) {
	menu_set(hdr_save_menu,
	    MENU_APPEND_ITEM, menu_get(save_menu, MENU_NTH_ITEM, item_number),
	    NULL);
    }
    /* Make sure the header subwindow menu exists so we can tack on a
     * pullright for Save.
     */
    if (! msg_menu) {
	get_msg_menu();
    }
    if ((menu_item = menu_find(msg_menu, MENU_STRING, "Save", NULL))
	    != NULL) {
	menu_set(menu_item, MENU_PULLRIGHT, hdr_save_menu, NULL);
    }

    /* insert folder-specific initial options to menu */
    folder_menu = menu;
    menu_item = menu_create_item(
	MENU_STRING,		savestr("System Mailbox"),
	MENU_CLIENT_DATA,	savestr("%"),
	MENU_RELEASE,		/* no value */
	NULL);
    menu_set(folder_menu, MENU_INSERT, 0, menu_item, NULL);
    menu_item = menu_create_item(
	MENU_STRING,		savestr("Main Mailbox"),
	MENU_CLIENT_DATA,	savestr("&"),
	MENU_RELEASE,		/* no value */
	NULL);
    menu_set(folder_menu, MENU_INSERT, 1, menu_item, NULL);
    menu_item = menu_create_item(
	MENU_STRING,		savestr("Last Accessed Folder"),
	MENU_CLIENT_DATA,	savestr("#"),
	MENU_RELEASE,		/* no value */
	NULL);
    menu_set(folder_menu, MENU_INSERT, 2, menu_item, NULL);

    menus_exist = 1;
    return;
}

static
walk_menu_event(item, event)
Panel_item item;
Event *event;
{
    char	 *folder_name;
    Menu_item 	  selection;
    Menu	  menu;
    Panel	  panel;
    void xx_file_dir();

    if (event_id(event) == MS_RIGHT && event_is_down(event)) {
	if (item == folder_item) {
	    menu = folder_menu;
	    panel = folder_panel;
	} else {
	    menu = save_menu;
	    panel = save_panel;
	}
	selection = (Menu_item)menu_show(menu, panel, event, 0);
	if (! selection) {
	    /* no selection was made */
	    return;
	}
	if ((folder_name = (char *)menu_get(selection, MENU_CLIENT_DATA))
		!= NULL) {
	    xx_file_dir(item, folder_name);
	}
    } else {
	panel_default_handle_event(item, event);
    }
}

#else /* NO_WALK_MENUS */

static
add_path_to_menu(item, path, n)
Panel_item item;
char *path;
int *n;
{
    char		**names, **np;
    struct stat 	s_buf;
    char		buf[MAXPATHLEN];

    /* don't add a folder to the list if user can't read it */
    if (stat(path, &s_buf) == -1 || !(s_buf.st_mode & S_IREAD))
	return;
    if ((s_buf.st_mode & S_IFMT) == S_IFDIR) {
	sprintf(buf, "%s/{.*,*}", path);
	if (filexp(buf, &names) > 0) {
	    for (np = names; np && *np; np++) {
		if (!glob(*np, "*/{.,..}"))
		    add_path_to_menu(item, *np, n);
	    }
	    free_vec(names);
	}
    } else if (test_folder(path, NULL))
	panel_set(item,
	    PANEL_CHOICE_STRING, (*n)++, savestr(trim_filename(path)),
	    NULL);
}

/*
 * Open the user's mail folder (either user set or default path) and find all
 * the files (assumed to be mail folders) and add them to the menu list of
 * folders to use.
 */
add_folder_to_menu(item, n)
Panel_item item;
{
    char	*tmp = NULL, *p;

    if (!(p = do_set(set_options, "folder")) || !*p)
	p = DEF_FOLDER;
    if (p) {
	int x = 0;
	tmp = getpath(p, &x);
	if (x == -1) {
	    if (errno != ENOENT)
		print("%s: %s\n", p, tmp);
	    tmp = NULL;
	}
    }
    if (tmp) {
	add_path_to_menu(item, tmp, &n);
    }
}
#endif /* NO_WALK_MENUS */
