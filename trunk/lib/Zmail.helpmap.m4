dnl	This file is preprocessed by m4.
dnl
ifdef(`MEDIAMAIL',`ifdef(`SGI_CUSTOM',,`define(`ZMAIL_BASIC')')')dnl
define(book,ifdef(`ZMAIL_BASIC',MediaMailHelp,MediaMailProHelp))dnl
define(AppName,ifdef(`MEDIAMAIL',MediaMail,Z-Mail))dnl
0;book;Overview;0;General;Zmail.gui.general
0;book;About AppName;0;About_This_Program;Zmail.gui.about;Zmail.gui.about_this_program
0;book;Using Help;0;Help;Zmail.gui.help
0;book;The AppName Window;0;Main_Window;Zmail.gui.main_window;Zmail.gui.main_window
0;book;Anatomy of the AppName Window;1;Anatomy_Main_Window;Zmail.gui.anatomy_of_the_main_window
0;book;The Folder Panel;2;Folder_Panel;Zmail.gui.main_folder_panel
0;book;The Messages Field;2;Messages__Field;Zmail.gui.messages:_field;Zmail.gui.message_list;Zmail.gui.message_lists
0;book;The Folder Status List;2;Folder_Status;Zmail.gui.main_folder_status
0;book;The Message Summaries List;2;Message_Summaries;Zmail.gui.message_summaries
0;book;The Button Panel;2;Button_Panel;Zmail.gui.main_button_panel
0;book;The Output Area;2;Output_Area;Zmail.gui.output_area;Zmail.gui.main_the_output_window
ifdef(`ZMAIL_BASIC',,0;book;The Command Line;1;Command_Line;Zmail.gui.command_line;Zmail.gui.command:_line;Zmail.gui.command:_field;Zmail.gui.command_area
)dnl
0;book;The Status Bar;2;Status_Bar;Zmail.gui.main_status_bar
0;book;Minimizing the AppName Window;1;Main_Window_Minimize;Zmail.gui.minimizing_the_main_window
0;book;Reading Messages;0;Reading_Messages;Zmail.gui.reading_messages;Zmail.gui.read_messages;Zmail.gui.show_messages;Zmail.gui.view_messages
0;book;The Message Window;1;Message_Display_Window;Zmail.gui.message_window;Zmail.gui.message_display_window;Zmail.gui.pinup_window
0;book;The Folder Panel;2;Message_Folder_Panel;Zmail.gui.message_folder_panel
0;book;The Headers Panel;2;Headers_Panel;Zmail.gui.message_headers_panel
0;book;The Message Body;2;Message_Body;Zmail.gui.message_body
0;book;The Attachments Panel;2;Message_Attachment_Panel;Zmail.gui.message_attachment_panel
0;book;The Button Panel;2;Message_Button_Panel;Zmail.gui.message_button_panel
0;book;The Status Bar;2;Message_Status_Bar;Zmail.gui.message_status_bar
0;book;Reading Other Messages;1;Next;Zmail.gui.next
0;book;Receiving Attachments;1;Receiving_Attachments;Zmail.gui.receiving_attachments
0;book;Displaying Attachments;2;Displaying_Attachments;Zmail.gui.displaying_attachments
0;book;Saving Attachments;2;Saving_Attachments;Zmail.gui.saving_attachments
0;book;Printing Attachments;2;Printing_Attachments;Zmail.gui.printing_attachments
0;book;Deleting Attachments;2;Deleting_Attachments;Zmail.gui.deleting_attachments
0;book;Receiving Large Messages;1;Receiving_Large_Messages;Zmail.gui
0;book;Managing Your Messages;0;Managing_Messages;Zmail.gui.managing_your_messages
0;book;Saving Messages;1;Saving_Messages;Zmail.gui.saving_messages;Zmail.gui.save_dialog;Zmail.gui.save_dialog
0;book;Preserving Messages;1;Preserve;Zmail.gui.preserve;Zmail.gui.unpreserve
0;book;Assigning Marks and Priorities;1;Marks_and_Priorities;Zmail.gui.marks_and_priorities;Zmail.gui.priority
0;book;Deleting Messages;1;Delete;Zmail.gui.delete
0;book;Printing Messages;1;Printing_Messages;Zmail.gui.printing_messages
0;book;Using the Print Dialog;2;Print_Dialog;Zmail.gui.print_dialog;Zmail.gui.printer_dialog
0;book;Sorting Messages;1;Sorting;Zmail.gui.sorting;Zmail.gui.custom_sort;Zmail.gui.sort_dialog
0;book;Searching Within a Message;1;Searching;Zmail.gui.searching;Zmail.gui.search_text_dialog
0;book;Replying to a Message;1;Replying_to_a_Message;Zmail.gui.replying_to_a_message;Zmail.gui.replying
0;book;Forwarding a Message;1;Forwarding_a_Message;Zmail.gui.forwarding_a_message
0;book;Composing Messages;0;Composing_Messages;Zmail.gui.composing_messages
0;book;The Compose Window;1;Compose_Window;Zmail.gui.compose_window
0;book;The Folder Panel;2;Comp_Folder_Panel;Zmail.gui.compose_folder_panel
0;book;The Tool Bar;2;Comp_Tool_Bar;Zmail.gui.compose_tool_bar
0;book;The Message Headers Panel;2;Comp_Headers;Zmail.gui.compose_headers
0;book;The Message Body;2;Comp_Message_Body;Zmail.gui.compose_body
0;book;The Attachments Panel;2;Comp_Attachment_Panel;Zmail.gui.compose_attachment_panel
0;book;The Button Panel;2;Comp_Button_Panel;Zmail.gui.compose_button_panel
0;book;The Status Bar;2;Comp_Status_Bar;Zmail.gui.compose_status_bar
0;book;Addressing;1;Addressing;Zmail.gui.addressing
ifdef(`ZMAIL_BASIC',,0;book;Using the Address Browser;2;Address_Browser;Zmail.gui.address_browser;Zmail.gui.browser
)dnl
0;book;Entering Addresses;1;Address_Entering;Zmail.gui.address_entry
0;book;Editing Addresses;1;Address_Editing;Zmail.gui.address_editing
0;book;Including Messages and Files;1;Including_Messages_and_Files;Zmail.gui.including_messages_and_files;Zmail.gui.including
0;book;Sending Attachments;1;Sending_Attachments;Zmail.gui.sending_attachments
0;book;Attaching an Existing File;2;Attaching_Existing;Zmail.gui.attaching_an_existing_file
0;book;Creating a New Attachment;2;Attaching_New;Zmail.gui.creating_a_new_attachment
0;book;Editing Text;1;Editing_Text;Zmail.gui.editing_text
0;book;Formatting Text;1;Formatting_text;Zmail.gui.formatting_text
0;book;Saving a Draft;1;Saving_Draft;Zmail.gui.saving_a_draft
0;book;Using a Different Editor;1;Different_Editor;Zmail.gui.using_a_different_editor
0;book;Using Search and Replace;1;Search_and_Replace;Zmail.gui.search_and_replace;Zmail.gui.editor_dialog
0;book;Checking Spelling;1;Spell_Checking;Zmail.gui.spell_checking;Zmail.gui.spelling_checker
0;book;Sending a Message;1;Sending_a_Message;Zmail.gui.sending_a_message
0;book;Sending Large Messages;2;Sending_Large_Messages;Zmail.gui
0;book;Setting Compose Options;1;Compose_Options;Zmail.gui.compose_options;Zmail.gui.comp_opts_dialog
0;book;Using Folders;0;Folders;Zmail.gui.folders
0;book;Creating a New Folder;1;Creating_Folders;Zmail.gui.creating_folders
0;book;Opening Folders;1;Opening_Folders;Zmail.gui.opening_folders;Zmail.gui.open_folder_dialog
0;book;Changing Among Open Folders;1;Folder_Popup;Zmail.gui.folder_popup;Zmail.gui.opened_folders
0;book;Updating Folders;1;Updating;Zmail.gui.updating;Zmail.gui.update
0;book;Closing Folders;1;Closing_Folders;Zmail.gui.closing_folders
0;book;Renaming Folders;1;Renaming_Folders;Zmail.gui.renaming_folders;Zmail.gui.rename_folder_dialog
0;book;Removing Folders;1;Removing_Folders;Zmail.gui.removing_folders
0;book;Customizing AppName;0;Customizing;Zmail.gui.customize
0;book;Setting Variables;1;Variables;Zmail.gui.variables;Zmail.gui.variable;Zmail.gui.variables_dialog
0;book;Changing Headers;1;Headers;Zmail.gui.headers
0;book;Changing Displayed Headers;2;Changing_Displayed_Headers;Zmail.gui.changing_displayed_headers;Zmail.gui.headers_dialog;Zmail.gui.ignored_headers;Zmail.gui.retained_headers
0;book;Creating Custom Headers;2;Creating_Custom_Headers;Custom_Headers;Zmail.gui.custom_headers;Zmail.gui.envelope;Zmail.gui.envelope_dialog
0;book;Customizing Fonts;1;Fonts_Dialog;Zmail.gui.fonts_dialog;Zmail.gui.font_dialog
0;book;Saving Your Configuration;1;Saving_Config;Zmail.gui.saving_your_configuration
ifdef(`ZMAIL_BASIC',,0;book;Defining Buttons;1;Buttons_Dialog;Zmail.gui.buttons_dialog
0;book;Defining Menus;1;Menus_Dialog;Zmail.gui.menus_dialog
0;book;Creating Message Filters;1;Filters;Zmail.gui.filters
0;book;Using the Filters Dialog;2;Filters_Dialog;Zmail.gui.filters_dialog;Zmail.gui.filters_dialog
)dnl
0;book;Aliases;0;Aliases;Zmail.gui.aliases;Zmail.gui.aliases_dialog
0;book;Creating Aliases;1;Creating_Aliases;Zmail.gui.creating_aliases;Zmail.gui.alias_dialog
0;book;Mailing to Aliases;1;Mailing_to_Aliases;Zmail.gui.mailing_to_aliases;Zmail.gui.comp_alias_dialog;Zmail.gui.using_aliases
0;book;Searching Among Messages;0;Searching_Among;Zmail.gui.searching_among
0;book;Searching for a Pattern;1;Search_for_Pattern;Zmail.gui.search_for_pattern;Zmail.gui.search_by_pattern;Zmail.gui.search_dialog;Zmail.gui.pattern_search_dialog
0;book;Using Extended Pattern Matching;2;Extended_Pattern_Matching;Zmail.gui.extended_pattern_matching;Zmail.gui.regular_expressions;Zmail.gui.regex
0;book;Searching by Date;1;Search_by_Date;Zmail.gui.search_by_date;Zmail.gui.search_for_date;Zmail.gui.dates_dialog
0;book;Using the Toolbox;0;Toolbox;Zmail.gui.toolbox;Zmail.gui.toolbox_window
0;book;Attachments;0;Attachments;Zmail.gui.attachments
0;book;Using the Attachments Dialog;1;Attachments_Dialog;Zmail.gui.attachments_dialog;Zmail.gui.attachments_dialog
0;book;Attachment Types;1;Attachment_Types;Zmail.gui.attachment_types
0;book;MIME;1;MIME;Zmail.gui.mime
0;book;Templates;0;Templates;Zmail.gui.templates;Zmail.gui.templates_dialog
0;book;Locating Files;0;File_Finder;Zmail.gui.file_finder;Zmail.gui.file_finder
0;book;Using the Text Pager;0;Text_Pager;Zmail.gui.text_pager;Zmail.gui.paging_dialog
0;book;Creating a Signature;0;Creating_a_Signature;Zmail.gui.creating_a_signature
0;book;MediaMail and Your .forward File;0;MediaMail_and_Dot_Forward;Zmail.gui
0;book;Creating an X-Face;0;X_Face;Zmail.gui.x_face
0;book;Using AppName Commands;0;Commands;Zmail.gui.commands;Zmail.gui.command;Zmail.gui.cmd_help
ifdef(`ZMAIL_BASIC',,0;book;?;1;?;Zmail.cmd.?
0;book;H;1;headers;Zmail.cmd.headers
0;book;Pipe;1;pipe;.
0;book;about;1;about;Zmail.cmd.about
0;book;alias;1;alias;Zmail.cmd.alias;Zmail.cmd.unalias;Zmail.cmd.expand;Zmail.cmd.group
0;book;alternates;1;alternates;Zmail.cmd.alternates;Zmail.cmd.alts
0;book;alts;1;alternates;.
0;book;arith;1;arith;Zmail.cmd.arith
0;book;ask;1;ask;Zmail.cmd.ask
0;book;attach;1;attach;Zmail.cmd.attach
0;book;await;1;await;Zmail.cmd.await
0;book;builtin;1;builtin;Zmail.cmd.builtin
0;book;button;1;button;Zmail.cmd.button;Zmail.cmd.unbutton
0;book;calc;1;calc;Zmail.cmd.calc
0;book;cd;1;cd;Zmail.cmd.cd
0;book;chroot;1;chroot;Zmail.cmd.chroot
0;book;close;1;folder;.
0;book;cmd;1;cmd;Zmail.cmd.cmd;Zmail.cmd.uncmd
0;book;co;1;save;.
0;book;compcmd;1;compcmd;Zmail.cmd.compcmd
0;book;copy;1;save;.
0;book;debug;1;debug;Zmail.cmd.debug
0;book;`define';1;function;.
0;book;delete;1;delete;Zmail.cmd.delete;Zmail.cmd.undelete;Zmail.cmd.u;Zmail.cmd.dp;Zmail.cmd.dt
0;book;detach;1;detach;Zmail.cmd.detach
0;book;dialog;1;dialog;Zmail.cmd.dialog
0;book;disable;1;disable;Zmail.cmd.disable;Zmail.cmd.enable
0;book;display;1;read;.
0;book;dp;1;delete;.
0;book;dt;1;delete;.
0;book;e;1;edit;.
0;book;each;1;each;Zmail.cmd.each
0;book;echo;1;echo;Zmail.cmd.echo;Zmail.cmd.error
0;book;edit;1;edit;Zmail.cmd.edit;Zmail.cmd.e;Zmail.cmd.edit_msg
0;book;edit_msg;1;edit;.
0;book;else;1;if;.
0;book;enable;1;disable;.
0;book;endif;1;if;.
0;book;error;1;echo;.
0;book;`eval';1;`eval';Zmail.cmd.`eval'
0;book;exit;1;exit;Zmail.cmd.exit;Zmail.cmd.return;Zmail.cmd.quit;Zmail.cmd.q;Zmail.cmd.xit;Zmail.cmd.x
0;book;expand;1;alias;.
0;book;f;1;from;.
0;book;fg;1;jobs;.
0;book;filter;1;filter;Zmail.cmd.filter;Zmail.cmd.unfilter
0;book;flags;1;flags;Zmail.cmd.flags;Zmail.cmd.hide;Zmail.cmd.unhide
0;book;fo;1;folder;.
0;book;folder;1;folder;Zmail.cmd.folder;Zmail.cmd.fo;Zmail.cmd.update;Zmail.cmd.open;Zmail.cmd.shut;Zmail.cmd.close
0;book;folders;1;folders;Zmail.cmd.folders;Zmail.cmd.fo;Zmail.cmd.update;Zmail.cmd.open;Zmail.cmd.shut;Zmail.cmd.close
0;book;foreach;1;foreach;Zmail.cmd.foreach
0;book;from;1;from;Zmail.cmd.from;Zmail.cmd.f
0;book;function;1;function;Zmail.cmd.function;Zmail.cmd.`define';Zmail.cmd.`undefine';Zmail.cmd.unfunction;Zmail.cmd.functions
0;book;functions;1;function;.
0;book;general;1;help;.
0;book;group;1;alias;.
0;book;h;1;headers;.
0;book;headers;1;headers;Zmail.cmd.headers;Zmail.cmd.h;Zmail.cmd.h
0;book;help;1;help;Zmail.cmd.help;Zmail.cmd.general
0;book;hide;1;flags;.
0;book;history;1;history;Zmail.cmd.history
0;book;iconify;1;iconify;Zmail.cmd.iconify
0;book;if;1;if;Zmail.cmd.if;Zmail.cmd.else;Zmail.cmd.endif
0;book;ignore;1;ignore;Zmail.cmd.ignore;Zmail.cmd.unignore
ifdef(`ZMAIL_BASIC',,0;book;interpose;1;interpose;Zmail.cmd.interpose;Zmail.cmd.uninterpose
)dnl
0;book;jobs;1;jobs;Zmail.cmd.jobs;Zmail.cmd.fg;Zmail.cmd.resume
0;book;license;1;license;Zmail.cmd.license
0;book;lpr;1;lpr;Zmail.cmd.lpr
0;book;ls;1;ls;Zmail.cmd.ls
0;book;m;1;mail;.
0;book;mail;1;mail;Zmail.cmd.mail;Zmail.cmd.m
0;book;map;1;map;Zmail.cmd.map;Zmail.cmd.unmap
0;book;map!;1;map_;Zmail.cmd.map!;Zmail.cmd.unmap!
0;book;mark;1;mark;Zmail.cmd.mark;Zmail.cmd.unmark
0;book;menu;1;menu;Zmail.cmd.menu;Zmail.cmd.unmenu
0;book;merge;1;merge;Zmail.cmd.merge
0;book;message list;1;Message_Lists;Zmail.cmd.messages_list;Zmail.cmd.message_list
0;book;message summary `format';1;Cmd_Header_Format;Zmail.cmd.header_format;Zmail.cmd.hdr_fmt
0;book;msg_list;1;msg_list;Zmail.cmd.msg_list
0;book;my_hdr;1;my_hdr;Zmail.cmd.my_hdr;Zmail.cmd.un_hdr
0;book;next;1;read;.
0;book;open;1;folder;.
0;book;p;1;read;.
0;book;page;1;page;Zmail.cmd.page
0;book;path;1;path;Zmail.cmd.path
0;book;pick;1;pick;Zmail.cmd.pick;Zmail.cmd.search
0;book;pinup;1;read;.
0;book;pipe;1;pipe;Zmail.cmd.pipe;Zmail.cmd.pipe;Zmail.cmd.pipe_msg
0;book;pipe_msg;1;pipe;.
0;book;pre;1;preserve;.
0;book;preserve;1;preserve;Zmail.cmd.preserve;Zmail.cmd.pre;Zmail.cmd.unpreserve;Zmail.cmd.unpre
0;book;previous;1;read;.
0;book;print;1;read;.
0;book;printenv;1;printenv;Zmail.cmd.printenv
0;book;prompt;1;prompt;Zmail.cmd.prompt
0;book;pwd;1;pwd;Zmail.cmd.pwd
0;book;q;1;exit;.
0;book;quit;1;exit;.
0;book;r;1;reply;.
0;book;read;1;read;Zmail.cmd.read;Zmail.cmd.display;Zmail.cmd.pinup;Zmail.cmd.print;Zmail.cmd.p;Zmail.cmd.type;Zmail.cmd.t;Zmail.cmd.top;Zmail.cmd.next;Zmail.cmd.previous
0;book;redraw;1;redraw;Zmail.cmd.redraw
0;book;remove;1;remove;Zmail.cmd.remove;Zmail.cmd.rmfolder
0;book;rename;1;rename;Zmail.cmd.rename
0;book;reply;1;reply;Zmail.cmd.reply;Zmail.cmd.r;Zmail.cmd.replysender;Zmail.cmd.respond
0;book;replysender;1;reply;.
0;book;respond;1;reply;.
0;book;resume;1;jobs;.
0;book;retain;1;retain;Zmail.cmd.retain;Zmail.cmd.unretain
0;book;return;1;exit;.
0;book;rmfolder;1;remove;.
0;book;save;1;save;Zmail.cmd.save;Zmail.cmd.w;Zmail.cmd.write;Zmail.cmd.copy;Zmail.cmd.co
0;book;saveopts;1;saveopts;Zmail.cmd.saveopts;Zmail.cmd.source
0;book;screencmd;1;screencmd;Zmail.cmd.screencmd
0;book;search;1;pick;.
0;book;set;1;set;Zmail.cmd.set;Zmail.cmd.unset
0;book;setenv;1;setenv;Zmail.cmd.setenv
0;book;sh;1;shell;.
0;book;shell;1;shell;Zmail.cmd.shell;Zmail.cmd.sh
0;book;`shift';1;`shift';Zmail.cmd.shift
0;book;shut;1;folder;.
0;book;sort;1;sort;Zmail.cmd.sort
0;book;sound;1;sound;Zmail.cmd.sound
0;book;source;1;saveopts;.
0;book;stop;1;stop;Zmail.cmd.stop
0;book;stty;1;stty;Zmail.cmd.stty
0;book;t;1;read;.
0;book;task_meter;1;task_meter;Zmail.cmd.task_meter
0;book;textedit;1;textedit;Zmail.cmd.textedit
0;book;top;1;read;.
0;book;trap;1;trap;Zmail.cmd.trap
0;book;type;1;read;.
0;book;u;1;delete;.
0;book;un_hdr;1;my_hdr;.
0;book;unalias;1;alias;.
0;book;unbutton;1;button;.
0;book;uncmd;1;cmd;.
0;book;`undefine';1;function;.
0;book;undelete;1;delete;.
0;book;undigest;1;undigest;Zmail.cmd.undigest
0;book;unfilter;1;filter;.
0;book;unfunction;1;function;.
0;book;unhide;1;flags;.
0;book;uniconify;1;uniconify;Zmail.cmd.uniconify
0;book;unignore;1;ignore;.
ifdef(`ZMAIL_BASIC',,0;book;uninterpose;1;interpose;.
)dnl
0;book;unmap;1;map;.
0;book;unmap!;1;map_;.
0;book;unmark;1;mark;.
0;book;unmenu;1;menu;.
0;book;unpre;1;preserve;.
0;book;unpreserve;1;preserve;.
0;book;unretain;1;retain;.
0;book;unset;1;set;.
0;book;unsetenv;1;unsetenv;Zmail.cmd.unsetenv
0;book;update;1;folder;.
0;book;variables;1;variables;Zmail.cmd.variables
0;book;version;1;version;Zmail.cmd.version
0;book;w;1;save;Zmail.cmd.save
0;book;write;1;save;.
0;book;x;1;exit;.
0;book;xit;1;exit;.
)dnl
