! Install this file in /usr/lib/X11/app-defaults/Zmail_ol
!
!
! This Defaults file contains Resource Values for The OPENLOOK version
! of Zmail.  Note the different Accelerator syntax:
!	in Motif:  Ctrl <Key>C
!       in OLIT:   Ctrl <C>
!
!
! This file contains reasonable default settings for various things
! that are X window system specific.  This file is not required to run
! Z-Mail, but if /usr/lib/X11/app-defaults/Zmail_ol exists, it must
! include the MenuBar label resources specified below.
!
! This file may be edited.  Future versions will contain more settings.
!
! Version number of this file -- do NOT remove or alter this setting!
Zmail_ol.version: 2.1b.1826
!
! Fallback resources were NOT used, i.e. all the resource specifications
! are coming from this file.  If this file does not exist or cannot be
! accessed by X, a default configuration will be in effect.  This is what
! "fallback" resources are.  Thus, if you use this file, you override the
! fallback resources.  Unless this file has been edited by your system
! administrator, the resources here match Z-Mail's fallback resources.
Zmail_ol.usedFallbacks: False
!
!
! Done buttons -- Open Look applications dialogs should not have Done
! buttons, they use the pushpin or window menu instead.  However, if you
! want to run Open Look Z-Mail under a window manager that does not provide
! pushpins / window menus, you can set this resource to true.
Zmail_ol.doneButtons: False
!
!
! Input focus -- The Open Look toolkit allows you to operation applications
! in "mouseless" mode, navigating between items using the keyboard only.
! The item that currently has the input focus is indicated by highlighting
! in red on color screens and inverting it on B&W screens.  Many people
! find this distracting, since it looks like a second selected item.
! You can disable this feature altogether by uncommenting the following
! resource, or you can change the highlighting color with the next resource.
!*mouseless: False
!*inputFocusColor: LightGrey
!
!
! Openwin version 2 window decorations.  There's an incompatibility
! between openwin 2 and openwin 3 with regardto title bars, pushpins,
! etc.  Z-Mail is built with version 3, and if you run it under a
! version 2 window manager you may not get the decorations.  If you
! run into this problem, try uncommenting this resource.
!*UseShortOLWinAttr: True
!
!
! Global editing translations
!
!*TextEdit.translations: #override \
!   Ctrl<Key>U: OlAction() kill-to-start-of-line() \n\
!    <Key>osfDelete: OlAction() delete-previous-character() \n\
!    Ctrl<Key>W: OlAction() delete-previous-word()
!
!
! Font specifications
!
! Fixed-width:
*ListPane*font: -*-courier-medium-r-*--12-*
*TextEdit.font: -*-courier-medium-r-*--12-*
*message_window*m_header.font: -*-courier-medium-r-*--12-*
*pinup_window*m_header.font: -*-courier-medium-r-*--12-*
*information_dialog*StaticText.font: -*-courier-bold-r-*--12-*
! Bold:
!*Caption.font: -*-new century schoolbook-bold-r-*--12-*
*Caption.font: lucidasans-bold-12
!*Slider.font: -*-new century schoolbook-bold-r-*--12-*
*Slider.font: lucidasans-bold-12
!*StaticText.font: -*-new century schoolbook-bold-r-*--12-*
*StaticText.font: lucidasans-bold-12
! Possibly bold:
!*CheckBox.font: -*-new century schoolbook-medium-bold-r-*--12-*
!*CheckBox.font: lucidasans-bold-12
!*CheckBox.font: -*-new century schoolbook-medium-r-*--12-*
*CheckBox.font: lucidasans-12
!*RectButton.font: -*-new century schoolbook-medium-bold-r-*--12-*
!*RectButton.font: lucidasans-bold-12
!*RectButton.font: -*-new century schoolbook-medium-r-*--12-*
*RectButton.font: lucidasans-12
! Possibly bold, possibly narrow:
!*MenuButton.font: -*-new century schoolbook-bold-r-*--12-*
!*MenuButton.font: lucidasans-bold-12
!*MenuButton.font: -*-new century schoolbook-r-*--12-*
*MenuButton.font: lucidasans-12
!*MenuButton.font: -monotype-gill-medium-r-normal-sans-0-*
!*OblongButton.font: -*-new century schoolbook-bold-r-*--12-*
!*OblongButton.font: lucidasans-bold-12
!*OblongButton.font: -*-new century schoolbook-r-*--12-*
*OblongButton.font: lucidasans-12
!*OblongButton.font: -monotype-gill-medium-r-normal-sans-0-*
! Other good fonts for the menus:
!  -monotype-gill-medium-r-normal-sans-0-0-0-0-p-0-iso8859-1
!  -monotype-gill sans-medium-r-normal--12-120-75-75-p-56-iso8859-1
!  -adobe-helvetica-medium-r-normal--10-100-75-75-p-56-iso8859-1
!  -linotype-helvetica-medium-r-narrow-sans-12-120-72-72-p-52-iso8859-1
!  -urw-itc avant garde-demi-r-normal-sans-10-100-72-72-p-56-iso8859-1
!
! List Properties
*List*selectable: False
! Restrict width of lists.
*List*prefMaxWidth: 600
! (The actual number doesn't seem to make any difference, but if the
! resource isn't present then lists can be as wide as the screen.)
!
! Multi-click timeout needs to be a little bit larger than the 200 msec
! default, 'cause OLIT is a little slow.
*multiClickTime: 400
!
! Window colors
! Most of the settings here are commented out as they are merely
! suggestions and we don't want to make any presumptions.  Edit
! to your tastes.
!
! The entire program...
*foreground: Black
*background: Grey
! The Z logo in the main window and the Z icons.
*main_window*fr_icon.foreground: Red
*zmail_icons.foreground: Red
! Lists and multi-line Texts get a white background.
*ListPane.background: White
*TextEdit.background: White
! Make all pushbuttons similar..
!*OblongButton.foreground: Blue
!*OblongButton.fontColor: Blue
!
!*main_buttons.foreground: darkgreen
!*action_area.foreground: darkgreen
!
!*error_dialog.background: red
!*help_dialog.background: lightsteelblue
!*information_dialog.background: Grey
!*selection_dialog.background: turquoise
!*prompt_dialog.background: turquoise
!*question_dialog.background: lightgreen
!*warning_dialog.background: hotpink
!
! You can specify background patterns too.
!*Table*backgroundPixmap: parchment.gif32
!
!
! Default Layout, Sizes, and Values
!
! Default Table widget spacing:
*Table.rowSpacing:		10
*Table.columnSpacing:		10
*Table.internalWidth:		10
*Table.internalHeight:		10
! Controls should use the same spacing.
*Control.hPad:			0
*ControlArea.hPad:		0
*Control.vPad:			0
*ControlArea.vPad:		0
*Control.hSpace:		10
*ControlArea.hSpace:		10
*Control.vSpace:		10
*ControlArea.vSpace:		10
! Except when they're in a NoticeShell, an outer border is needed.
*NoticeShell.Control.hPad:	10
*NoticeShell.ControlArea.hPad:	10
*NoticeShell.Control.vPad:	10
*NoticeShell.ControlArea.vPad:	10
! And Controls that are menu bars want a little less spacing.
*menu_bar.hSpace:		5
*menu_bar.vSpace:		5
! Captions need space too.
*Caption.space:			10
! The default scale of 12 is way too large.
*scale:				10
! CheckBoxes should always have the text on the right.
*CheckBox.position: right
!
! Main window (main_window):
!     menu_bar
!        folder_list
!        frame_header
!     message_summaries
!     main_buttons
!      main_output_text
!     command_area
!
! Overall layout is code-generated, since panes can appear and disappear.
! Layout for the main-window frame header:
!      fr_efolder_lbl fr_efolder_text fr_efolder_menu    |     fr_icon
!     fr_messages_lbl fr_messages_text                fr_toggle    |
!
*main_window.frame_header.tableLayout: \
    fr_efolder_lbl	0 0 1 1 rwWhH ;\
    fr_efolder_text	1 0 1 1 lhH ;\
    fr_efolder_menu	2 0 1 1 wWhH ;\
    fr_icon		4 0 1 2 rwWhH ;\
    fr_messages_lbl	0 1 1 1 rwWhH ;\
    fr_messages_text	1 1 1 1 lhH ;\
    fr_toggle		3 1 1 1 rwhH
*main_window.frame_header.internalWidth: 0
*main_window.frame_header.internalHeight: 0
! The format string used to title the main window (see "help prompt")
*main_window*title: Z-Mail
! The format string used to label the icon (see "help prompt")
*main_window*iconName: %t Msgs
! Size of folder status list
*main_window*folder_list*viewHeight: 1
*main_window*folder_list*Scrollbar.height: 1
*main_window*folder_list*prefMinWidth: 600
! Size of header summary display
*main_window*message_summaries*viewHeight: 10
*main_window*message_summaries*prefMinWidth: 600
! Buttons per row in the button area.
*main_window*main_buttons.measure: 8
*main_window*main_buttons.sameSize: none
! Size of scrolled output area
*main_window*main_output_text*linesVisible: 3
*main_window*main_output_text*charsVisible: 70
! The Command Area label
*main_window*command_area.cmd_label.string: Command:
*main_window*command_area.cmd_text*charsVisible: 65
!
! Message display window (message_window/pinup_window):
!                     menu_bar
!      fr_sfolder_lbl fr_sfolder_text     fr_icon
!     fr_messages_lbl fr_messages_text  fr_toggle
!     m_status_box
!     m_header
!                   m_text_scroll
!     m_attachments
!                    action_area
!
*message_window.message_window_child.tableLayout: \
    menu_bar		0 0 3 1 lwhH ;\
    fr_sfolder_lbl	0 1 1 1 rwWthH ;\
    fr_sfolder_text	1 1 1 1 lthH ;\
    fr_icon		2 1 1 1 rwhH ;\
    fr_messages_lbl	0 2 1 1 rwWhH ;\
    fr_messages_text	1 2 1 1 lhH ;\
    fr_toggle		2 2 1 1 rwhH ;\
    m_status_box	0 3 3 1 lwhH ;\
    m_header		0 4 3 1 lwhH ;\
    m_text_scroll	0 5 3 1 ;\
    m_attachments	0 6 3 1 H ;\
    action_area		0 7 3 1 whH
*pinup_window.pinup_window_child.tableLayout: \
    menu_bar		0 0 3 1 lwhH ;\
    fr_sfolder_lbl	0 1 1 1 rwWthH ;\
    fr_sfolder_text	1 1 1 1 lthH ;\
    fr_icon		2 1 1 1 rwhH ;\
    fr_messages_lbl	0 2 1 1 rwWhH ;\
    fr_messages_text	1 2 1 1 lhH ;\
    fr_toggle		2 2 1 1 rwhH ;\
    m_status_box	0 3 3 1 lwhH ;\
    m_header		0 4 3 1 lwhH ;\
    m_text_scroll	0 5 3 1 ;\
    m_attachments	0 6 3 1 H ;\
    action_area		0 7 3 1 whH
!Size of message header (HACK HACK)
*message_window*m_header*charsVisible: 84
*pinup_window*m_header*charsVisible: 84
*message_window*m_header*linesVisible: 2
*pinup_window*m_header*linesVisible: 2
! Size of scrolled display
*message_window*m_text_scroll*charsVisible: 80
*pinup_window*m_text_scroll*charsVisible: 80
*message_window*m_text_scroll*linesVisible: 24
*pinup_window*m_text_scroll*linesVisible: 24
*message_window*m_attachments.XtNlayoutType: OL_FIXEDCOLS
*pinup_window*m_attachments.XtNlayoutType: OL_FIXEDCOLS
*message_window*m_attachments.XtNmeasure: 5
*pinup_window*m_attachments.XtNmeasure: 5
!
! Opened Folders dialog (open_fldrs_dialog):
!     fr_sfolder_lbl fr_sfolder_text  fr_icon
!                     o_list
!     o_opts_lbl  o_opts
!                   action_area
!
*open_fldrs_dialog.open_fldrs_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 1 1 1 rwWthH ;\
    fr_sfolder_text	1 1 1 1 lthH ;\
    fr_icon		2 0 1 3 rwhH ;\
    o_list		0 3 3 1 ;\
    o_opts_lbl		0 4 1 1 rwWthH ;\
    o_opts_box		1 4 2 1 lwhH ;\
    action_area		0 5 3 1 whH
! Height of folder status list
*open_fldrs_dialog*o_list*viewHeight: 5
*open_fldrs_dialog*o_opts_lbl.string: Indexing Options:
*open_fldrs_dialog*o_opts_box*update_index.label: Update If Present
*open_fldrs_dialog*o_opts_box*create_index.label: Create If Needed
*open_fldrs_dialog*o_opts_box*suppress_index.label: Suppress Index
!
! Attachments dialog (attachments_dialog)
!     fr_sfolder_lbl fr_sfolder_text  fr_icon
!                  file_finder
!     a_filetype_lbl a_filetype
!     a_encoding_lbl a_encoding
!      a_comment_lbl a_comment_text
!       a_attach_lbl a_list
!                 action_area
!
*attachments_dialog.attachments_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 0 1 1 rwWthH ;\
    fr_sfolder_text	1 0 1 1 lthH ;\
    fr_icon		2 0 1 1 rwhH ;\
    file_finder		0 1 3 1 ;\
    a_filetype_lbl	0 2 1 1 rwWhH ;\
    a_filetype		1 2 2 1 lhH ;\
    a_encoding_lbl	0 3 1 1 rwWhH ;\
    a_encoding		1 3 2 1 lhH ;\
    a_comment_lbl	0 4 1 1 rwWhH ;\
    a_comment_text	1 4 2 1 lH ;\
    a_attach_lbl	0 5 1 1 rwWthH ;\
    a_list		1 5 2 1 ;\
    action_area		0 6 3 1 whH
! Height of attachments list
*attachments_dialog*a_filetype_lbl.string: File Type:
*attachments_dialog*a_encoding_lbl.string: Encoding:
*attachments_dialog*a_list*viewHeight: 3
*attachments_dialog*a_list*selectable: False
!
! Buttons and Functions dialog (buttons_dialog):
!   b_label                                                              fr_icon
!   b_but_lbl                              b_fun_lbl
!              b_but_list                               b_fun_list
!   b_butname_lbl b_butname_text b_butdel  b_funname_lbl b_funname_text b_fundel
!   b_text_lbl   b_require
!                               b_funtext_scroll
!                                  action_area
!
*buttons_dialog.buttons_dialog_child.tableLayout: \
    b_label		0 0 7 1 lwhH ;\
    fr_icon		7 0 1 1 rwhH ;\
    b_but_lbl		0 1 4 1 lwhH ;\
    b_fun_lbl		4 1 4 1 lwhH ;\
    b_but_list		0 2 4 1 ;\
    b_fun_list		4 2 4 1 ;\
    b_butname_lbl	0 3 1 1 lwWhH ;\
    b_butname_text	1 3 2 1 lH ;\
    b_butdel		3 3 1 1 rwhH ;\
    b_funname_lbl	4 3 1 1 lwWhH ;\
    b_funname_text	5 3 2 1 lH ;\
    b_fundel		7 3 1 1 rwhH ;\
    b_text_lbl		0 4 2 1 lwhH ;\
    b_require		2 4 6 1 lwhH ;\
    b_funtext_scroll	0 5 8 1 ;\
    action_area		0 6 8 1 whH
*buttons_dialog*b_but_lbl.string: Current Buttons:
*buttons_dialog*b_fun_lbl.string: Available Functions:
! Height of lists of buttons and functions (should be the same)
*buttons_dialog*b_but_list*viewHeight: 3
*buttons_dialog*b_but_list*selectable: False
*buttons_dialog*b_fun_list*viewHeight: 3
*buttons_dialog*b_fun_list*selectable: False
! Size of editing area
*buttons_dialog*b_funtext_scroll*charsVisible: 60
*buttons_dialog*b_funtext_scroll*linesVisible: 7
*buttons_dialog*b_require.label: Uses selected messages
*buttons_dialog*b_text_lbl.string: Function Text:
! The default place to save buttons
*buttons_dialog*prompt_dialog*string: ~/.zmailrc
!
! This is an experimental Buttons layout, kept for reference:
!   b_label                       fr_icon
!       b_but_lbl       b_but_list
!   b_butname_lbl b_butname_text b_butdel
!       b_fun_lbl       b_fun_list
!   b_funname_lbl b_funname_text b_fundel
!      b_text_lbl    b_funtext_scroll
!   b_require
!               action_area
!
!*buttons_dialog.buttons_dialog_child.tableLayout: \
!   b_label		0  0 2 1 lwhH ;\
!   fr_icon		2  0 1 1 rwhH ;\
!   b_but_lbl		0  2 1 1 rwWth ;\
!   b_but_list		1  2 2 1 ;\
!   b_butname_lbl	0  3 1 1 rwWhH ;\
!   b_butname_text	1  3 1 1 lH ;\
!   b_butdel		2  3 1 1 rwhH ;\
!   b_fun_lbl		0  5 1 1 rwWth ;\
!   b_fun_list		1  5 2 1 ;\
!   b_funname_lbl	0  6 1 1 rwWhH ;\
!   b_funname_text	1  6 1 1 lH ;\
!   b_fundel		2  6 1 1 rwhH ;\
!   b_text_lbl		0  8 1 1 rwWth ;\
!   b_funtext_scroll	1  8 2 1 ;\
!   b_require		1  9 2 1 lwhH ;\
!   action_area		0 11 3 1 whH
!
! Compose window (compose_window):
!                       menu_bar
!      fr_sfolder_lbl fr_sfolder_text                 fr_icon
!     fr_messages_lbl fr_messages_text  fr_droptarg fr_toggle
!                      c_hdr_prompts
!                      c_text_scroll
!                       action_area
!
*compose_window.compose_window_child.tableLayout: \
    menu_bar		0 0 4 1 lwhH ;\
    fr_sfolder_lbl	0 1 1 1 rwWthH ;\
    fr_sfolder_text	1 1 1 1 lthH ;\
    fr_icon		2 1 3 1 rwhH ;\
    fr_messages_lbl	0 2 1 1 rwWhH ;\
    fr_messages_text	1 2 1 1 lhH ;\
    fr_droptarg		2 2 1 1 rwWhH ;\
    fr_toggle		3 2 1 1 rwWhH ;\
    c_hdr_prompts	0 3 4 1 ;\
    c_text_scroll	0 4 4 1 ;\
    action_area		0 5 4 1 whH
! Size of editing area
*compose_window*c_text_scroll*charsVisible: 80
*compose_window*c_text_scroll*linesVisible: 15
! Layout of subsidiary header-prompts table:
!          h_to_lbl h_to_text      h_address_lbl
!     h_subject_lbl h_subject_text h_address_list
!          h_cc_lbl h_cc_text            |
!         h_bcc_lbl h_bcc_text      action_area
*compose_window.compose_window_child.c_hdr_prompts.tableLayout: \
    h_to_lbl		0 0 1 1 rwWhH ;\
    h_to_text		1 0 1 1 lhH ;\
    h_address_lbl	2 0 1 1 lwhH ;\
    h_subject_lbl	0 1 1 1 rwWhH ;\
    h_subject_text	1 1 1 1 lhH ;\
    h_address_list	2 1 1 2 ;\
    h_cc_lbl		0 2 1 1 rwWhH ;\
    h_cc_text		1 2 1 1 lhH ;\
    h_bcc_lbl		0 3 1 1 rwWhH ;\
    h_bcc_text		1 3 1 1 lhH ;\
    action_area		2 3 1 1 whH
*compose_window.compose_window_child.c_hdr_prompts.internalWidth: 0
*compose_window.compose_window_child.c_hdr_prompts.internalHeight: 0
!
! Compose Options dialog (comp_opts_dialog):
!      fr_sfolder_lbl fr_sfolder_text   fr_icon
!     fr_messages_lbl fr_messages_text     |
!                  o_textopts
!                  o_boolopts
!                 action_area
!
*comp_opts_dialog.comp_opts_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 0 1 1 rwWthH ;\
    fr_sfolder_text	1 0 1 1 lthH ;\
    fr_icon		2 0 1 2 rwhH ;\
    fr_messages_lbl	0 1 1 1 rwWhH ;\
    fr_messages_text	1 1 1 1 lhH ;\
    o_textopts		0 2 3 1 whH ;\
    o_boolopts		0 3 3 1 whH ;\
    action_area		0 4 3 1 whH
*comp_opts_dialog*logfile.label: Log File:
*comp_opts_dialog*record.label: Record File:
*comp_opts_dialog*autosign.label: Autosign
*comp_opts_dialog*return-receipt.label: Return-Receipt
*comp_opts_dialog*autoformat.label: AutoFormat
*comp_opts_dialog*edit-hdrs.label: Edit Headers
*comp_opts_dialog*autodismiss.label: AutoDismiss
*comp_opts_dialog*autoiconify.label: AutoIconify
*comp_opts_dialog*autoclear.label: AutoClear
*comp_opts_dialog*verify.label: Verify
*comp_opts_dialog*verbose.label: Verbose
*comp_opts_dialog*synchronous.label: Synch Send
*comp_opts_dialog*record-user.label: Record-User
!
! Custom Sort dialog (sort_dialog):
!     fr_sfolder_lbl fr_sfolder_text  fr_icon
!     s_by_lbl s_reverse_lbl       s_opts_box
!     s_by_box s_reverse_box           |
!      |        |                      |
!      |        |                  s_order
!                  action_area
!
*sort_dialog.sort_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 1 1 1 lwthH ;\
    fr_sfolder_text	1 1 4 1 lthH ;\
    fr_icon		5 0 1 3 rwhH ;\
    s_by_lbl		0 3 2 1 lwhH ;\
    s_reverse_lbl	2 3 2 1 lwhH ;\
    s_opts_box		4 3 2 3 lwhH ;\
    s_by_box		0 4 2 3 lwhH ;\
    s_reverse_box	2 4 2 3 lwhH ;\
    s_order		4 6 2 1 lt ;\
    action_area		0 7 6 1 whH
*sort_dialog*s_by_lbl.string: Sort By:
*sort_dialog*s_by_box*Date.label: Date
*sort_dialog*s_by_box*Subject.label: Subject
*sort_dialog*s_by_box*Author.label: Author
*sort_dialog*s_by_box*Length.label: Length
*sort_dialog*s_by_box*Priority.label: Priority
*sort_dialog*s_by_box*Status.label: Status
*sort_dialog*s_reverse_lbl.string: Reverse
*sort_dialog*s_opts_box*ignore_case.label: Ignore Case in Sort
*sort_dialog*s_opts_box*use_Re_in_subject.label: Use "Re:" in Subject
*sort_dialog*s_opts_box*use_date_received.label: Sort By Date Received
!
! Date Search dialog (dates_dialog):
!      fr_sfolder_lbl fr_sfolder_text   fr_icon
!     fr_messages_lbl fr_messages_text     |
!     d_year               d_opts_box
!     d_day                     |
!       |                       |
!       |                       |
!       |                   d_funcs
!     d_month                  |
!        |                     |
!     d_which_box              |
!                   d_how_box
!     d_msg
!                   d_results
!                    action_area
!
*dates_dialog.dates_dialog_child.tableLayout: \
    fr_sfolder_lbl	0  0 1 1 rwWthH ;\
    fr_sfolder_text	1  0 2 1 lthH ;\
    fr_icon		3  0 1 2 rwhH ;\
    fr_messages_lbl	0  1 1 1 rwWhH ;\
    fr_messages_text	1  1 2 1 lhH ;\
    d_year		0  2 2 1 whH ;\
    d_opts_box		2  2 2 4 lwhH ;\
    d_day		0  3 2 4 whH ;\
    d_month		0  7 2 2 whH ;\
    d_which_box		0  9 2 1 whH ;\
    d_funcs		2  6 2 4 thH;\
    d_how_box		0 10 4 1 whH ;\
    d_msg		0 11 4 1 lwhH ;\
    d_results		0 12 4 1 ;\
    action_area		0 13 4 1 whH
! Search criteria
*dates_dialog*d_opts_box*constrain_to_messages.label: Constrain to "Messages:"
*dates_dialog*d_opts_box*use_date_received.label: Use Date Message Received
*dates_dialog*d_opts_box*find_non_matching.label: Find Non-Matches
*dates_dialog*d_opts_box*search_all_folders.label: Search All Open Folders
*dates_dialog*d_opts_box*perform_function.label: Perform Function on Result
*dates_dialog*d_how_box*on_date_only.label: On Date Only
*dates_dialog*d_how_box*on_or_before.label: On or Before Date
*dates_dialog*d_how_box*on_or_after.label: On or After Date
*dates_dialog*d_how_box*between_dates.label: Between Dates
! Height of list of functions to perform
*dates_dialog*d_funcs*viewHeight: 3
! Height of list of matched messages
*dates_dialog*d_results*viewHeight: 5
!
! Envelope dialog (envelope_dialog):
!     e_label                     |
!      e_name_lbl e_name_text  fr_icon
!     e_value_lbl e_value_text    |
!               e_list
!             action_area
!
*envelope_dialog.envelope_dialog_child.tableLayout: \
    e_label		0 0 2 1 lwhH ;\
    fr_icon		2 0 1 3 rwhH ;\
    e_name_lbl		0 1 1 1 rwWhH ;\
    e_name_text		1 1 1 1 lH ;\
    e_value_lbl		0 2 1 1 rwWhH ;\
    e_value_text	1 2 1 1 lH ;\
    e_list		0 3 3 1 ;\
    action_area		0 4 3 1 whH
*envelope_dialog*e_label.string: \
    Type name of header and new value and select (Set) or (Unset)
! Height of list of personal headers
*envelope_dialog*e_list*viewHeight: 8
*envelope_dialog*e_list*selectable: False
! The default place to save personal headers
*envelope_dialog*prompt_dialog*string: ~/.zmailrc
!
! Folder Manager dialog (folders_dialog):
!        fr_sfolder_lbl fr_sfolder_text    fr_icon
!       fr_messages_lbl fr_messages_text      |
!                    file_finder
!             f_sysfolder f_mainfolder 
!     f_folder_opts_lbl f_folder_opts_box
!       f_save_opts_lbl f_save_opts_box
!       f_file_opts_lbl f_file_opts_box
!                     action_area
!
*folders_dialog.folders_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 0 1 1 rwWthH ;\
    fr_sfolder_text	1 0 2 1 lthH ;\
    fr_icon		3 0 1 2 rwhH ;\
    fr_messages_lbl	0 1 1 1 rwWhH ;\
    fr_messages_text	1 1 2 1 lhH ;\
    file_finder		0 2 4 1 ;\
    f_sysfolder		1 3 1 1 lwhH ;\
    f_mainfolder	2 3 1 1 lwhH ;\
    f_folder_opts_lbl	0 4 1 1 rwWhH ;\
    f_folder_opts_box	1 4 3 1 lwhH ;\
    f_file_opts_lbl	0 5 1 1 rwWhH ;\
    f_file_opts_box	1 5 3 1 lwhH ;\
    f_save_opts_lbl	0 6 1 1 rwWhH ;\
    f_save_opts_box	1 6 3 1 lwhH ;\
    action_area		0 7 4 1 whH
! Quick foldering buttons
*folders_dialog*f_sysfolder.label: System Folder
*folders_dialog*f_mainfolder.label: Main Mailbox
! Action Area Options
*folders_dialog*f_folder_opts_lbl.string: Open Options:
*folders_dialog*f_folder_opts_box*read_only.label: Open Read Only
*folders_dialog*f_folder_opts_box*add_folder.label: Add Folder
*folders_dialog*f_folder_opts_box*use_index.label: Use Index
*folders_dialog*f_folder_opts_box*merge_folder.label: Merge
*folders_dialog*f_save_opts_lbl.string: Save Options:
*folders_dialog*f_save_opts_box*save_text_only.label: Save Text Only
*folders_dialog*f_save_opts_box*overwrite_file.label: Overwrite File
*folders_dialog*f_file_opts_lbl.string: File Options:
*folders_dialog*f_file_opts_box*Create.label: Create
*folders_dialog*f_file_opts_box*Remove.label: Remove
*folders_dialog*f_file_opts_box*Rename.label: Rename
*folders_dialog*f_file_opts_box*View.label: View
!
! Fonts dialog (fonts_dialog):
!     f_label         fr_icon
!     f_list
!     f_sample_scroll
!     f_types_lbl f_types_box
!      f_mode_lbl f_mode_box
!           action_area
!
*fonts_dialog.fonts_dialog_child.tableLayout: \
    f_label		0 0 3 1 lwhH ;\
    fr_icon		3 0 1 1 rwhH ;\
    f_list		0 1 4 1 ;\
    f_sample_scroll	0 2 4 1 ;\
    f_types_lbl		0 3 1 1 rwWthH ;\
    f_types_box		1 3 3 1 lwhH ;\
    f_mode_lbl		0 4 1 1 rwWthH ;\
    f_mode_box		1 4 3 1 lwhH ;\
    action_area		0 5 4 1 whH
*fonts_dialog*f_types_lbl.string: Objects:
*fonts_dialog*f_types_box.layoutType: fixedcols
*fonts_dialog*f_types_box.measure: 4
*fonts_dialog*f_types_box*PushButtons.label: PushButtons
*fonts_dialog*f_types_box*Labels.label: Labels
*fonts_dialog*f_types_box*ToggleButtons.label: ToggleButtons
*fonts_dialog*f_types_box*Texts.label: Texts
*fonts_dialog*f_types_box*Lists.label: Lists
*fonts_dialog*f_types_box*Menus.label: Menus
*fonts_dialog*f_mode_lbl.string: Set Mode:
*fonts_dialog*f_mode_box*interactive.label: Fonts: Interactive
*fonts_dialog*f_mode_box*object_type.label: Fonts: Object Type
*fonts_dialog*f_mode_box*label_only.label: Label Only
*fonts_dialog*f_mode_box*label_font.label: Label And Font
*fonts_dialog*f_mode_box.layoutType: fixedrows
*fonts_dialog*f_mode_box.measure: 1
*fonts_dialog*prompt_dialog*string: ~/.zmfonts
! The fonts prompt dialog (fonts_dialog*prompt_dialog):
!     p_select_lbl
!     p_select_text
!     p_action_area
*fonts_dialog*prompt_dialog.prompt_dialog_child.tableLayout: \
    p_select_lbl	0 0 1 1 lwhH ;\
    p_select_text	0 1 1 1 lwH ;\
    p_action_area	0 2 2 1 whH
!
! File Finder dialog (file_finder_dialog):
!     f_label  fr_icon
!        file_finder
!        action_area
!
*file_finder_dialog.file_finder_dialog_child.tableLayout: \
    f_label		0 0 1 1 lwhH ;\
    fr_icon		1 0 1 1 rwhH ;\
    file_finder		0 1 2 1 ;\
    action_area		0 2 2 1 whH
!
! Help Index dialog (help_index_dialog):
!     h_label             fr_icon
!     h_list        h_desc_scroll
!     h_topics_lbl h_topics_box
!
*help_index_dialog.help_index_dialog_child.tableLayout: \
    h_label		 0 0 29 1 lwhH ;\
    fr_icon		29 0  1 1 rwhH ;\
    h_list		 0 1 10 1 wW ;\
    h_desc_scroll	10 1 20 1 ;\
    h_topics_lbl	 0 2  1 1 lwWhH ;\
    h_topics_box	 1 2 29 1 lwhH
! Height of index list (normally automatic from *h_desc_scroll*linesVisible)
*help_index_dialog*h_list*viewHeight: 9
! Height of scrolled display
*help_index_dialog*h_desc_scroll*charsVisible: 60
*help_index_dialog*h_desc_scroll*linesVisible: 10
*help_index_dialog*h_topics_lbl.string: Help Category:
*help_index_dialog*h_topics_box*graphical.label: Graphical Interface
*help_index_dialog*h_topics_box*z-script.label: Z-Script Commands
! Help Topics toggle box label
!
! License Registration dialog (license_dialog):
!                          fr_icon
!        l_license_lbl l_license_list
!       l_progname_lbl l_progname_text
!        l_version_lbl l_version_text
!       l_hostname_lbl l_hostname_text
!         l_hostid_lbl l_hostid_text
!     l_expiration_lbl l_expiration_text
!      l_max_users_lbl l_max_users_text
!          l_entry_lbl l_entry_list
!         l_passwd_lbl l_passwd_text
!           l_user_lbl l_user_text
!                action_area
*license_dialog.license_dialog_child.tableLayout: \
    fr_icon		2  0 1 1 rwWhH ;\
    l_license_lbl	0  1 1 1 rwWthH ;\
    l_license_list	1  1 2 1 ;\
    l_progname_lbl	0  2 1 1 rwWhH ;\
    l_progname_text	1  2 2 1 lH ;\
    l_version_lbl	0  3 1 1 rwWhH ;\
    l_version_text	1  3 2 1 lH ;\
    l_hostname_lbl	0  4 1 1 rwWhH ;\
    l_hostname_text	1  4 2 1 lH ;\
    l_hostid_lbl	0  5 1 1 rwWhH ;\
    l_hostid_text	1  5 2 1 lH ;\
    l_expiration_lbl	0  6 1 1 rwWhH ;\
    l_expiration_text	1  6 2 1 lH ;\
    l_max_users_lbl	0  7 1 1 rwWhH ;\
    l_max_users_text	1  7 2 1 lH ;\
    l_entry_lbl		0  8 1 1 rwWthH ;\
    l_entry_list	1  8 2 1 ;\
    l_passwd_lbl	0  9 1 1 rwWhH ;\
    l_passwd_text	1  9 2 1 lH ;\
    l_user_lbl		0 10 1 1 rwWhH ;\
    l_user_text		1 10 2 1 lH ;\
    action_area		0 11 3 1 whH
!*license_dialog*l_license_list*viewHeight: 5
*license_dialog*l_entry_list*viewHeight: 5
!
! Aliases popup (comp_alias_dialog):
!         fr_icon
!       a_list
!     action_area
!
*comp_alias_dialog.comp_alias_dialog_child.tableLayout: \
    fr_icon		2 0 1 1 rwWhH ;\
    a_list		0 1 3 1 ;\
    action_area		0 2 3 1 whH
comp_alias_dialog*a_list.viewHeight: 8
!
! Aliases dialog (alias_dialog):
!     a_label                    |
!      a_name_lbl a_name_text  fr_icon
!     a_value_lbl a_value_text   |
!                a_list
!              action_area
!
*alias_dialog.alias_dialog_child.tableLayout: \
    a_label		0 0 2 1 lwhH ;\
    fr_icon		2 0 1 3 rwhH ;\
    a_name_lbl		0 1 1 1 rwWhH ;\
    a_name_text		1 1 1 1 lH ;\
    a_value_lbl		0 2 1 1 rwWhH ;\
    a_value_text	1 2 1 1 lH ;\
    a_list		0 3 3 1 ;\
    action_area		0 4 3 1 whH
*alias_dialog*a_label.string: \
    Type name of alias and address list and select (Set) or (Unset)
! Height of name and address lists
*alias_dialog*a_list*viewHeight: 8
*alias_dialog*a_list*selectable: False
! The default place to save aliases
*alias_dialog*prompt_dialog*string: ~/.zmailrc
!
! Mail Headers dialog (headers_dialog):
!     h_label                       |
!     h_name_lbl h_name_text     fr_icon
!     h_mode_box                    |
!     h_setings_lbl     h_choices_lbl
!       h_setings_list    h_choices_list
!                action_area
!
*headers_dialog.headers_dialog_child.tableLayout: \
    h_label		0 0 3 1 lwhH ;\
    h_name_lbl		0 1 1 1 rwWhH ;\
    h_name_text		1 1 2 1 lH ;\
    h_mode_box		0 2 3 1 lwhH ;\
    fr_icon		3 0 1 3 rwhH ;\
    h_settings_lbl	0 3 2 1 lwhH ;\
    h_choices_lbl	2 3 2 1 lwhH ;\
    h_settings_list	0 4 2 1 ;\
    h_choices_list	2 4 2 1 ;\
    action_area		0 5 4 1 whH
*headers_dialog*h_mode_box*ignored.label: Ignored Headers
*headers_dialog*h_mode_box*retained.label: Show Only
*headers_dialog*h_settings_lbl.string: Current Settings:
*headers_dialog*h_choices_lbl.string: Available Choices:
! Height of both lists of ignored/retained headers
*headers_dialog*h_settings_list*viewHeight: 8
*headers_dialog*h_settings_list*selectable: False
*headers_dialog*h_choices_list*viewHeight: 8
*headers_dialog*h_choices_list*selectable: False
! The default place to save ignored headers
*headers_dialog*prompt_dialog*string: ~/.zmailrc
!
! Pattern Search dialog (search_dialog):
!      fr_sfolder_lbl fr_sfolder_text   fr_icon
!     fr_messages_lbl fr_messages_text     |
!           s_pat_lbl s_pat_text
!         s_where_lbl s_where_box
!          s_head_lbl s_head_text
!          s_opts_lbl s_opts_box
!         s_funcs_lbl s_funcs
!     s_msg
!                     s_results
!                    action_area
!
*search_dialog.search_dialog_child.tableLayout: \
    fr_sfolder_lbl	0  0 1 1 rwWthH ;\
    fr_sfolder_text	1  0 3 1 lthH ;\
    fr_icon		4  0 1 2 rwhH ;\
    fr_messages_lbl	0  1 1 1 rwWhH ;\
    fr_messages_text	1  1 2 1 lhH ;\
    s_pat_lbl		0  2 1 1 rwWhH ;\
    s_pat_text		1  2 4 1 lH ;\
    s_where_lbl		0  3 1 1 rwWthH ;\
    s_where_box		1  3 4 1 lwhH ;\
    s_head_lbl		0  4 1 1 rwWhH ;\
    s_head_text		1  4 2 1 lH ;\
    s_opts_lbl		0  5 1 1 rwWthH ;\
    s_opts_box		1  5 4 1 lwhH ;\
    s_funcs_lbl		0  6 1 1 rwWthH ;\
    s_funcs		1  6 1 1 l ;\
    s_msg		0  7 5 1 lwhH ;\
    s_results		0  8 5 1 ;\
    action_area		0  9 5 1 whH
*search_dialog*s_where_lbl.string: Search In:
*search_dialog*s_where_box.layoutType: fixedcols
*search_dialog*s_where_box.measure: 2
*search_dialog*s_where_box*entire_message.label: Entire Message
*search_dialog*s_where_box*to_fields.label: To:
*search_dialog*s_where_box*from_fields.label: From:
*search_dialog*s_where_box*subject.label: Subject:
*search_dialog*s_where_box*header.label: Use Header:
*search_dialog*s_opts_lbl.string: Options:
*search_dialog*s_opts_box.layoutType: fixedcols
*search_dialog*s_opts_box.measure: 2
*search_dialog*s_opts_box*constrain_to_messages.label: Constrain to "Messages:"
*search_dialog*s_opts_box*ignore_case.label: Ignore Case in Pattern
*search_dialog*s_opts_box*find_non_matching.label: Find Non-Matches
*search_dialog*s_opts_box*search_all_folders.label: Search All Open Folders
*search_dialog*s_opts_box*perform_function.label: Perform Function on Result
! Height of list of functions to perform
*search_dialog*s_funcs*viewHeight: 2
! Height of list of matched messages
*search_dialog*s_results*viewHeight: 5
!
! Printer dialog (printer_dialog):
!      fr_sfolder_lbl fr_sfolder_text   fr_icon
!     fr_messages_lbl fr_messages_text     |
!       p_toggles_lbl p_toggles_box
!          p_name_lbl p_name_text
!          p_list_lbl          p_list
!                   action_area
!
*printer_dialog.printer_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 0 1 1 rwWthH ;\
    fr_sfolder_text	1 0 1 1 lthH ;\
    fr_icon		2 0 1 2 rwhH ;\
    fr_messages_lbl	0 1 1 1 rwWhH ;\
    fr_messages_text	1 1 1 1 lhH ;\
    p_toggles_lbl	0 2 1 1 rwWthH ;\
    p_toggles_box	1 2 2 1 lwhH ;\
    p_name_lbl		0 3 1 1 rwWhH ;\
    p_name_text		1 3 2 1 lH ;\
    p_list_lbl		0 4 1 1 rwWthH ;\
    p_list		1 4 2 1 ;\
    action_area		0 5 3 1 whH
*printer_dialog*p_toggles_lbl.string: Print Message:
*printer_dialog*p_toggles_box*standard.label: Standard Message Headers
*printer_dialog*p_toggles_box*all_headers.label: All Message Headers
*printer_dialog*p_toggles_box*body_only.label: Message Body Only
!
! Search and Replace dialog (editor_dialog):
!      fr_sfolder_lbl fr_sfolder_text   fr_icon
!     fr_messages_lbl fr_messages_text     |
!        e_search_lbl e_search_text
!       e_replace_lbl e_replace_text
!           e_how_lbl e_how_box
!                     e_case  e_wrap  e_confirm
!     e_instruct
!     e_spell_lbl
!            e_spell_list
!             action_area
!
*editor_dialog.editor_dialog_child.tableLayout: \
    fr_sfolder_lbl	0 0 1 1 rwWthH ;\
    fr_sfolder_text	1 0 5 1 lthH ;\
    fr_icon		6 0 1 2 rwhH ;\
    fr_messages_lbl	0 1 1 1 rwWhH ;\
    fr_messages_text	1 1 5 1 lhH ;\
    e_search_lbl	0 2 1 1 rwWhH ;\
    e_search_text	1 2 6 1 lH ;\
    e_replace_lbl	0 3 1 1 rwWhH ;\
    e_replace_text	1 3 6 1 lH ;\
    e_how_lbl		0 4 1 1 rwWhH ;\
    e_how_box		1 4 6 1 lwhH ;\
    e_case		1 5 2 1 lwhH ;\
    e_wrap		3 5 2 1 lwhH ;\
    e_confirm		5 5 2 1 lwhH ;\
    e_instruct		0 6 7 1 lhH ;\
    e_spell_lbl		0 7 7 1 lwhH ;\
    e_spell_list	0 8 7 1 ;\
    action_area		0 9 7 1 whH
*editor_dialog*e_how_lbl.string: Options:
*editor_dialog*e_how_box*find_next.label: Next Occurrence
*editor_dialog*e_how_box*find_all.label: All Occurrences
*editor_dialog*e_case.label: Ignore Case
*editor_dialog*e_wrap.label: Wrap Around
*editor_dialog*e_confirm.label: Confirm Replace
*editor_dialog*e_spell_lbl.string: Misspelled Words:
!*editor_dialog*e_spell_lbl.string: Mispelt Words:
!
! Task Meter dialog (task_meter_dialog):
!     fr_icon t_title
!     t_progress
!     t_scale_lbl
!            t_scale_nums
!              t_scale
!            action_area
!             t_instruct
!
*task_meter_dialog.task_meter_dialog_child.tableLayout: \
    fr_icon		0 0 1 1 lwWhH ;\
    t_title		1 0 2 1 lwhH ;\
    t_progress		0 1 3 1 lwhH ;\
    t_scale_lbl		0 2 3 1 lwhH ;\
    t_scale_nums	0 3 3 1 hH ;\
    t_scale		0 4 3 1 hH ;\
    action_area		0 5 3 1 whH ;\
    t_instruct		0 6 3 1 whH
!
! Templates dialog (templates_dialog):
!     t_label  fr_icon
!         t_list
!       action_area
!
*templates_dialog.templates_dialog_child.tableLayout: \
    t_label		0 0 1 1 lwhH ;\
    fr_icon		1 0 1 1 rwhH ;\
    t_list		0 1 2 1 ;\
    action_area		0 2 2 1 whH
! Height of list of template names
*templates_dialog*t_list*viewHeight: 8
*templates_dialog*t_list*selectable: False
!
! Toolbox (toolbox_window):
!     fr_sfolder_lbl fr_sfolder_text
!                 t_icons
!
*toolbox_window.toolbox_window_child.tableLayout: \
    fr_sfolder_lbl	0 0 1 1 rwWthH ;\
    fr_sfolder_text	1 0 1 1 lthH ;\
    t_icons		0 1 2 1 whH
!
! Variables dialog (variables_dialog):
!     v_savefile_lbl v_savefile_text  fr_icon
!     v_list  v_desc_lbl
!        |              v_desc_scroll
!        |    v_toggle                 v_inst
!        |    v_prompt      v_text      v_set
!        |    v_expand_lbl v_expand_box
!                   action_area
! or:
!     v_savefile_lbl v_savefile_text  fr_icon
!     v_list  v_desc_lbl
!        |            v_desc_scroll
!        |    v_toggle                 v_inst
!        |    v_prompt       v_slider
!        |
!                   action_area
! or:
!     v_savefile_lbl v_savefile_text  fr_icon
!     v_list  v_desc_lbl
!        |              v_desc_scroll
!        |    v_toggle          |
!        |                 v_multival
!        |                      |
!               action_area
!
*variables_dialog.variables_dialog_child.tableLayout: \
    v_savefile_lbl	 0 0  1 1 lwhH ;\
    v_savefile_text	 1 0 18 1 lhH ;\
    fr_icon		19 0  1 1 rwhH ;\
    v_list		 0 1  1 5 ;\
    v_desc_lbl		 1 1 19 1 lwhH ;\
    v_desc_scroll	 1 2 19 1 ;\
    v_toggle		 1 3  4 1 lwH ;\
    v_inst		 5 3 15 1 rhH ;\
    v_multival		 5 3 15 3 ;\
    v_prompt		 1 4  1 1 lh ;\
    v_slider		 2 4 18 1 lh ;\
    v_text		 2 4 17 1 lH ;\
    v_set		19 4  1 1 rwh ;\
    v_expand_lbl	 1 5 18 1 rwh ;\
    v_expand_box	19 5  1 1 rwh ;\
    action_area		 0 6 20 1 whH
! Height of index list
*variables_dialog*v_list*viewHeight: 	12
*variables_dialog*v_list*selectable:	False
*variables_dialog*v_desc_lbl.string: Variable Description:
*variables_dialog*v_desc_scroll*charsVisible: 60
*variables_dialog*v_desc_scroll*linesVisible: 8
*variables_dialog*v_expand_lbl.string: Expand variable/file references:
*variables_dialog*v_expand_box*Yes.label: Yes
*variables_dialog*v_expand_box*No.label: No
*variables_dialog*v_savefile_text.string: ~/.zmailrc
!
! Color Dialog (color_dialog):
!     p_label                             fr_icon
!      p_samples_lbl p_samples_box p_sample_button
!                     p_tiles
!        p_types_lbl p_types_box
!     p_usecolor_lbl p_usecolor_box
!                   action_area
!
*color_dialog.color_dialog_child.tableLayout: \
    p_label		0 0 3 1 lwhH ;\
    fr_icon		3 0 1 1 rwhH ;\
    p_samples_lbl	0 1 1 1 rwWhH ;\
    p_samples_box	1 1 1 1 lwWhH ;\
    p_sample_button	2 1 2 1 lwhH ;\
    p_tiles		0 2 4 1 wH ;\
    p_types_lbl		0 3 1 1 rwWthH ;\
    p_types_box		1 3 3 1 lwhH ;\
    p_usecolor_lbl	0 4 1 1 rwWhH ;\
    p_usecolor_box	1 4 3 1 lwhH ;\
    action_area		0 5 4 1 whH
*color_dialog*p_samples_lbl.string: Samples:
*color_dialog*p_samples_box*Foreground.label: Foreground
*color_dialog*p_samples_box*Background.label: Background
*color_dialog*p_tiles.layoutType: fixedcols
*color_dialog*p_tiles.measure: 16
*color_dialog*p_sample_button.label: A Sample PushButton
*color_dialog*p_types_lbl.string: Objects:
*color_dialog*p_types_box*PushButtons.string: PushButtons
*color_dialog*p_types_box*Labels.string: Labels
*color_dialog*p_types_box*ToggleButtons.string: ToggleButtons
*color_dialog*p_types_box*Scrollbars.string: Scrollbars
*color_dialog*p_types_box*Managers.string: Managers
*color_dialog*p_types_box*Texts.string: Texts
*color_dialog*p_types_box*Lists.string: Lists
*color_dialog*p_types_box*Scales.string: Scales
*color_dialog*p_types_box*Menus.string: Menus
*color_dialog*p_types_box.layoutType: fixedcols
*color_dialog*p_types_box.measure: 4
*color_dialog*p_usecolor_lbl.string: Use Color:
*color_dialog*p_usecolor_box*Foreground.label: Foreground
*color_dialog*p_usecolor_box*Background.label: Background
*color_dialog*p_mode_lbl.string: Coloring Mode:
*color_dialog*p_mode_box*interactive.label: Interactive
*color_dialog*p_mode_box*interactive.label: Object Type
*color_dialog*color_list.string: \
    White, RosyBrown2, HotPink, VioletRed, Red, Brown, RosyBrown4, Tan, \
    Wheat, GoldenRod, Orange, Gold, Yellow, Green, LimeGreen, \
    MediumAquamarine, SeaGreen, OliveDrab, DarkOliveGreen, LightCyan4, \
    CadetBlue, Aquamarine, Turquoise, Cyan, LightCyan2, LightSteelBlue1, \
    LightSteelBlue2, LightSteelBlue4, SteelBlue, SkyBlue4, SkyBlue3, \
    SkyBlue2, SkyBlue1, Blue, Navy, Purple, Magenta, Violet, Plum, \
    grey90, grey83, grey75, grey65, grey50, grey40, grey35, grey25, Black
*color_dialog*prompt_dialog*string: ~/.zmcolors
! The color prompt dialog (color_dialog*prompt_dialog):
!     p_select_lbl
!     p_select_text
!     p_action_area
*color_dialog*prompt_dialog.prompt_dialog_child.tableLayout: \
    p_select_lbl	0 0 1 1 lwhH ;\
    p_select_text	0 1 1 1 lwH ;\
    p_action_area	0 2 2 1 whH
!
! Information dialog (information_dialog):
!           fr_icon
!     p_text_scroll
!      action_area
!
*information_dialog.information_dialog_child.tableLayout: \
    fr_icon		1 0 1 1 rwWhH ;\
    p_text_scroll	0 1 2 1 ;\
    action_area		0 2 2 1 whH
*information_dialog*p_text_scroll*charsVisible: 80
!
! Paging dialog (paging_dialog):
!     p_text_scroll
!      action_area
!
*paging_dialog.paging_dialog_child.tableLayout: \
    p_text_scroll	0 0 1 1 ;\
    action_area		0 1 1 1 whH
*paging_dialog*p_text_scroll*charsVisible: 80
!
! The file finders (file_finder):
!      f_dir_lbl f_dir      f_dots
!                f_list
!     f_file_lbl f_file_text
!
*file_finder.tableLayout: \
    f_dir_lbl		0 0 1 1 rwWhH ;\
    f_dir		1 0 1 1 lH ;\
    f_dots		2 0 1 1 whH ;\
    f_list		0 1 3 1 ;\
    f_file_lbl		0 2 1 1 rwWhH ;\
    f_file_text		1 2 2 1 lH
*file_finder.internalWidth: 0
*file_finder.internalHeight: 0
*file_finder*f_dir_lbl.string: Directory:
*file_finder*f_dots.label: Hidden Files
*file_finder*f_list*viewHeight: 5
!
! The prompt dialogs (prompt_dialog):
!     p_select_lbl p_select_text
!           p_action_area
!
*prompt_dialog.prompt_dialog_child.tableLayout: \
    p_select_lbl	0 0 1 1 lwh ;\
    p_select_text	1 0 1 1 lwbhH ;\
    p_action_area	0 1 2 1 whH
!
! The selection dialogs (select_dialog):
!       s_list_lbl s_list
!     s_select_lbl s_select_text
!          s_action_area
!
*select_dialog.select_dialog_child.tableLayout: \
    s_list_lbl		0 0 1 1 rwWthH ;\
    s_list		1 0 1 1 ;\
    s_select_lbl	0 1 1 1 rwWhH ;\
    s_select_text	1 1 1 1 lwH ;\
    s_action_area	0 2 2 1 whH
*select_dialog*s_list*prefMinWidth: 300
!
!
! MenuBar Resources
!
! Everything (and we mean -everything-) for the menubars for all windows
! (currently, just main_window and compose_window) are specified here.
! This includes, labels, mnemonics, accelerators and accelerator texts.
! WARNING: You may alter the label values, but DO NOT remove them!
! Accelerators and mnemonics may be safely added or removed, but be sure
! that the acceleratorText changes with the accelerator.
!
! Note that OLIT requires the letter in the accelerator resource to
! be lower-case.
!
! Note also that OLIT mnemonics on menu-buttons behave differently than
! Motif mnemonics.  In Motif they open the menu, in OL they execute
! the default (first) item on the menu.  Because of this, mnemonics
! on menu-buttons are a bad idea; and without them, mnemonics inside
! menus are useless.  Thus all mnemonics in this file are commented out.
!
! Menu bar labels sometimes have trailing whitespace to force more visually
! pleasing layout of the menu bar itself.
!
! Main window:
!
! File menu
!
*menu_bar.file.label: File
!*menu_bar.file.mnemonic: F
! file menu (fm) items
*menu_bar*fm_folders.label: Folder Manager ...
!*menu_bar*fm_folders.mnemonic: F
*menu_bar*fm_folders.accelerator: Ctrl<f>
*menu_bar*fm_folders.acceleratorText: Ctrl+F
*menu_bar*fm_add_folder.label: Add Folder ...
!*menu_bar*fm_add_folder.mnemonic: A
*menu_bar*fm_add_folder.accelerator: Ctrl<a>
*menu_bar*fm_add_folder.acceleratorText: Ctrl+A
*menu_bar*fm_update.label: Update Folder
!*menu_bar*fm_update.mnemonic: U
*menu_bar*fm_close_fldr.label: Close Folder
!*menu_bar*fm_close_fldr.mnemonic: C
*menu_bar*fm_active.label: Opened Folders ...
!*menu_bar*fm_active.mnemonic: O
*menu_bar*fm_save_message.label: Save Messages ...
!*menu_bar*fm_save_message.mnemonic: S
*menu_bar*fm_save_message.accelerator: Ctrl<s>
*menu_bar*fm_save_message.acceleratorText: Ctrl+S
*menu_bar*fm_save_to.label: Save To
*menu_bar*fm_print.label: Print Messages ...
!*menu_bar*fm_print.mnemonic: P
*menu_bar*fm_save_state.label: Save Configuration ...
!*menu_bar*fm_save_state.mnemonic: v
*menu_bar*fm_save_state*prompt_dialog*string: ~/.zmailrc
*menu_bar*fm_spacer.string:
*menu_bar*fm_quit.label: Quit
!*menu_bar*fm_quit.mnemonic: Q
*menu_bar*fm_quit.accelerator: Ctrl<c>
*menu_bar*fm_quit.acceleratorText: Ctrl+C
! File menu for the message reading window.
*menu_bar*fm_iconify.label: Iconify
!*menu_bar*fm_iconify.mnemonic: I
*menu_bar*fm_iconify.accelerator: Ctrl<i>
*menu_bar*fm_iconify.acceleratorText: Ctrl+I
*menu_bar*fm_close.label: Close
!*menu_bar*fm_close.mnemonic: C
*menu_bar*fm_close.accelerator: Ctrl<c>
*menu_bar*fm_close.acceleratorText: Ctrl+C
!
! Edit menu
!
*menu_bar.edit.label: Edit
!*menu_bar.edit.mnemonic: E
! edit menu (em) items
*menu_bar*em_delete.label: Delete
!*menu_bar*em_delete.mnemonic: D
*menu_bar*em_undelete.label: Undelete
!*menu_bar*em_undelete.mnemonic: U
*menu_bar*em_preserve.label: Preserve
!*menu_bar*em_preserve.mnemonic: P
*menu_bar*em_unpreserve.label: Unpreserve
*menu_bar*em_mark.label: Mark
!*menu_bar*em_mark.mnemonic: M
*menu_bar*em_unmark.label: Unmark
*menu_bar*em_priority.label: Prioritize
*emp_a.label: A
!*emp_a.mnemonic: A
*emp_b.label: B
!*emp_b.mnemonic: B
*emp_c.label: C
!*emp_c.mnemonic: C
*emp_d.label: D
!*emp_d.mnemonic: D
*emp_e.label: E
!*emp_e.mnemonic: E
*emp_clear.label: None
!*emp_clear.mnemonic: N
*menu_bar*em_spacer.string:
!
! View menu
!
*menu_bar.view.label: View
!*menu_bar.view.mnemonic: V
! view menu (vm) items
*menu_bar*vm_show.label: Read Message
!*menu_bar*vm_show.mnemonic: R
*menu_bar*vm_pinup.label: Pin-Up Message
!*menu_bar*vm_pinup.mnemonic: P
*menu_bar*vm_spacer.string:
*menu_bar*vm_pattern.label: Pattern Search ...
*menu_bar*vm_date.label: Date Search ...
! message window...
*menu_bar*vm_next.label: Read Next
!*menu_bar*vm_next.mnemonic: N
*menu_bar*vm_next_reference.label: Next Reference
!*menu_bar*vm_next_reference.mnemonic: R
! reference pullright menu
*menu_bar*nm_same_subject.label: By Subject
!*menu_bar*nm_same_subject.mnemonic: S
*menu_bar*nm_same_author.label: By Author
!*menu_bar*nm_same_author.mnemonic: A
*menu_bar*nm_same_msgid.label: By Message ID
!*menu_bar*nm_same_msgid.mnemonic: M
! continue with message window view menu
*menu_bar*vm_next.accelerator: Ctrl<n>
*menu_bar*vm_next.acceleratorText: Ctrl+N
*menu_bar*vm_prev.label: Read Previous
*menu_bar*vm_prev.acceleratorText: P
*menu_bar*vm_prev.accelerator: Ctrl<p>
*menu_bar*vm_prev.acceleratorText: Ctrl+P
*menu_bar*vm_search.label: Search ...
!*menu_bar*vm_search.mnemonic: S
!*menu_bar*vm_search.accelerator: Ctrl Shift<s>
!*menu_bar*vm_search.acceleratorText: Shift+Ctrl+S
!
! Options menu
!
*menu_bar.options.label: Options
!*menu_bar.options.mnemonic: O
! option menu (om) items
*menu_bar*om_variables.label: Variables ...
!*menu_bar*om_variables.mnemonic: V
*menu_bar*om_variables.accelerator: Ctrl<v>
*menu_bar*om_variables.acceleratorText: Ctrl+V
*menu_bar*om_headers.label: Headers ...
!*menu_bar*om_headers.mnemonic: H
*menu_bar*om_envelope.label: Envelope ...
!*menu_bar*om_envelope.mnemonic: E
*menu_bar*om_aliases.label: Aliases ...
!*menu_bar*om_aliases.mnemonic: A
*menu_bar*om_buttons.label: Buttons ...
!*menu_bar*om_buttons.mnemonic: B
*menu_bar*om_buttons.accelerator: Ctrl<b>
*menu_bar*om_buttons.acceleratorText: Ctrl+B
*menu_bar*om_colors.label: Colors ...
!*menu_bar*om_colors.mnemonic: C
*menu_bar*om_fonts.label: Fonts ...
!*menu_bar*om_fonts.mnemonic: F
*menu_bar*om_spacer.string:
*menu_bar*om_toolbox.label: Toolbox ...
!*menu_bar*om_toolbox.mnemonic: T
*menu_bar*om_toolbox.accelerator: Ctrl<t>
*menu_bar*om_toolbox.acceleratorText: Ctrl+T
!
! Compose Menu
!   Note: This handles the main window and the compose window.
!         If you want these two to differ, you must copy relevant
!         resources for *compose_window.menubar.....
!
*menu_bar.compose.label: Compose
!*menu_bar.compose.mnemonic: C
! compose menu (cm) items
*menu_bar*cm_compose.label: New
!*menu_bar*cm_compose.mnemonic: N
*menu_bar*cm_compose.accelerator: Ctrl<n>
*menu_bar*cm_compose.acceleratorText: Ctrl+N
*menu_bar*cm_reply.label: Reply
!*menu_bar*cm_reply.mnemonic: R
! reply menu (rm) items (this menu is also shared by the message window)
*menu_bar.reply.label: Reply 
!*menu_bar.reply.mnemonic: R
*menu_bar*rm_replysender.label: Sender Only
*menu_bar*rm_replyall.label: All Recipients
*menu_bar*rm_sender_inc.label: Sender (Include Msg.)
*menu_bar*rm_all_inc.label: All (Include Msg.)
! reply menu specifics for the message window
*message_window*menu_bar*rm_replysender.accelerator: Ctrl<r>
*pinup_window*menu_bar*rm_replysender.accelerator: Ctrl<r>
*message_window*menu_bar*rm_replysender.acceleratorText: Ctrl+R
*pinup_window*menu_bar*rm_replysender.acceleratorText: Ctrl+R
*message_window*menu_bar*rm_replyall.accelerator: Ctrl Shift<r>
*pinup_window*menu_bar*rm_replyall.accelerator: Ctrl Shift<r>
*message_window*menu_bar*rm_replyall.acceleratorText: Shift+Ctrl+R
*pinup_window*menu_bar*rm_replyall.acceleratorText: Shift+Ctrl+R
!*message_window*menu_bar*rm_sender_inc.accelerator: Meta Ctrl<r>
!*pinup_window*menu_bar*rm_sender_inc.accelerator: Meta Ctrl<r>
!*message_window*menu_bar*rm_sender_inc.acceleratorText: Meta+Ctrl+R
!*pinup_window*menu_bar*rm_sender_inc.acceleratorText: Meta+Ctrl+R
!*message_window*menu_bar*rm_all_inc.accelerator: Meta Ctrl Shift<r>
!*pinup_window*menu_bar*rm_all_inc.accelerator: Meta Ctrl Shift<r>
!*message_window*menu_bar*rm_all_inc.acceleratorText: Meta+Shift+Ctrl+R
!*pinup_window*menu_bar*rm_all_inc.acceleratorText: Meta+Shift+Ctrl+R
*menu_bar*cm_forward.label: Forward
!*menu_bar*cm_forward.mnemonic: F
*menu_bar*cm_spacer.string:
*menu_bar*cm_templates.label: Templates ...
!*menu_bar*cm_templates.mnemonic: T
! forward menu (fwm) items for the message window
*menu_bar.forward.label: Forward
!*menu_bar.forward.mnemonic: F
*menu_bar*fwm_resend.label: Resend ...
!*menu_bar*fwm_resend.mnemonic: R
*menu_bar*fwm_add_cmnts.label: with Comments ...
!*menu_bar*fwm_add_cmnts.mnemonic: C
*menu_bar*fwm_edit.label: Edited ...
!*menu_bar*fwm_edit.mnemonic: E
!
! Sort menu
!
*menu_bar.sort.label: Sort
!*menu_bar.sort.mnemonic: S
! sort menu (sm) items
*menu_bar*sm_date.label: By Date
!*menu_bar*sm_date.mnemonic: D
*menu_bar*sm_subject.label: By Subject
!*menu_bar*sm_subject.mnemonic: S
*menu_bar*sm_author.label: By Author
!*menu_bar*sm_author.mnemonic: A
*menu_bar*sm_length.label: By Message Length
!*menu_bar*sm_length.mnemonic: L
*menu_bar*sm_priority.label: By Priority
!*menu_bar*sm_priority.mnemonic: P
*menu_bar*sm_status.label: By Status
!*menu_bar*sm_status.mnemonic: t
*menu_bar*sm_spacer.string:
*menu_bar*sm_custom.label: Custom Sort ...
!
! Windows menu
!
*main_window*menu_bar.windows.label: Panes
!*main_window*menu_bar.windows.mnemonic: P
! window menu (win) items
*main_window*menu_bar*win_title.label: Folder Status
!*main_window*menu_bar*win_title.mnemonic: F
*main_window*menu_bar*win_control.label: Folder Panel
!*main_window*menu_bar*win_control.mnemonic: P
*main_window*menu_bar*win_list.label: Message Summaries
!*main_window*menu_bar*win_list.mnemonic: S
*main_window*menu_bar*win_panel.label: Button Panel
!*main_window*menu_bar*win_panel.mnemonic: B
*main_window*menu_bar*win_output.label: Output Window
!*main_window*menu_bar*win_output.mnemonic: O
*main_window*menu_bar*win_command.label: Command Line
!*main_window*menu_bar*win_command.mnemonic: L
!
! Help menu
!
*menu_bar.help.label: Help
!*menu_bar.help.mnemonic: H
! help menu (hm) items
*menu_bar*hm_general.label: General ...
!*menu_bar*hm_general.mnemonic: G
!*menu_bar*hm_context.label: On Context
!*menu_bar*hm_context.mnemonic: C
*menu_bar*hm_main_window.label: Main Window ...
!*menu_bar*hm_main_window.mnemonic: W
*menu_bar*hm_folder_field.label: Folder: Field ...
!*menu_bar*hm_folder_field.mnemonic: F
*menu_bar*hm_messages_field.label: Messages: Field ...
!*menu_bar*hm_messages_field.mnemonic: M
*menu_bar*hm_summaries.label: Message Summaries ...
!*menu_bar*hm_summaries.mnemonic: S
*menu_bar*hm_buttons.label: Button Panel ...
!*menu_bar*hm_buttons.mnemonic: B
*menu_bar*hm_output_win.label: Output Window ...
!*menu_bar*hm_output_win.mnemonic: O
*menu_bar*hm_command_area.label: Command Line ...
!*menu_bar*hm_command_area.mnemonic: L
!*menu_bar*hm_lists.label: Message Lists ...
!*menu_bar*hm_lists.mnemonic: M
!*menu_bar*hm_hdr_format.label: Header Format ...
!*menu_bar*hm_hdr_format.mnemonic: H
*menu_bar*hm_about.label: About ...
!*menu_bar*hm_about.mnemonic: A
*menu_bar*hm_index.label: Index ...
!*menu_bar*hm_index.mnemonic: I
!
! Message window:
!
! (see shared menus above)
*menu_bar*hm_message_win.label: Message Window ...
!*menu_bar*hm_message_win.mnemonic: M
!*menu_bar*hm_message_win.accelerator: <F1>
!*menu_bar*hm_message_win.acceleratorText: F1
!
! Compose window:
!
! File menu
!
*compose_window*menu_bar.file.label: File
!*compose_window*menu_bar.file.mnemonic: F
! file menu (fm) items
*compose_window*menu_bar*fm_open.label: Open
!*compose_window*menu_bar*fm_open.mnemonic: O
*compose_window*menu_bar*fm_open*om_insert.label: Insert Text ...
!*compose_window*menu_bar*fm_open*om_insert.mnemonic: I
*compose_window*menu_bar*fm_open*om_replace.label: Replace Text ...
!*compose_window*menu_bar*fm_open*om_replace.mnemonic: R
*compose_window*menu_bar*fm_save.label: Save ...
!*compose_window*menu_bar*fm_save.mnemonic: S
*compose_window.menu_bar*fm_save.accelerator: Ctrl<s>
*compose_window.menu_bar*fm_save.acceleratorText: Ctrl+S
*compose_window*menu_bar*fm_spacer.string:
*compose_window*menu_bar*fm_templates.label: Templates ...
!*compose_window*menu_bar*fm_templates.mnemonic: T
*compose_window*menu_bar*fm_done.label: Done
!*compose_window*menu_bar*fm_done.mnemonic: D
!
! Edit menu
!
*compose_window*menu_bar.edit.label: Edit
!*compose_window*menu_bar.edit.mnemonic: E
! edit menu (em) items
*compose_window*menu_bar*em_cut.label: Cut
!*compose_window*menu_bar*em_cut.mnemonic: C
*compose_window*menu_bar*em_cut.accelerator: Shift Meta<x>
*compose_window*menu_bar*em_cut.acceleratorText: Shift+Meta+X
*compose_window*menu_bar*em_copy.label: Copy
!*compose_window*menu_bar*em_copy.mnemonic: o
*compose_window*menu_bar*em_copy.accelerator: Shift Meta<c>
*compose_window*menu_bar*em_copy.acceleratorText: Shift+Meta+C
*compose_window*menu_bar*em_paste.label: Paste
!*compose_window*menu_bar*em_paste.mnemonic: P
*compose_window*menu_bar*em_paste.accelerator: Shift Meta<v>
*compose_window*menu_bar*em_paste.acceleratorText: Shift+Meta+V
! Select All is shared with main window Edit menu
*menu_bar*em_select_all.label: Select All
!*menu_bar*em_select_all.mnemonic: A
*menu_bar*em_select_all.accelerator: Shift Meta<a>
*menu_bar*em_select_all.acceleratorText: Shift+Meta+A
*compose_window*menu_bar*em_clear.label: Clear
!*compose_window*menu_bar*em_clear.mnemonic: l
*compose_window*menu_bar*em_spacer.string:
*compose_window*menu_bar*em_format.label: Format
!*compose_window*menu_bar*em_format.mnemonic: F
*compose_window*menu_bar*em_search.label: Search/Spell ...
!*compose_window*menu_bar*em_search.mnemonic: S
*compose_window*menu_bar*em_editor.label: Editor
!*compose_window*menu_bar*em_editor.mnemonic: E
!
! Options menu
!
! The items in the options menu are the 4 toggle buttons in the
! compose window.  Note that mnemonics are only used if items are
! in a menu--otherwise they have no functional effect (so we don't
! display them).
*compose_window*menu_bar.options.label: Options
!*compose_window*menu_bar.options.mnemonic: O
! options menu items -- "menubar" isn't specified so resources apply
! whether the item is in a menu or just in the window.
*compose_window*om_autosign.label: Autosign
!*compose_window*menu_bar*om_autosign.mnemonic: A
*compose_window*om_autoformat.label: Autoformat
!*compose_window*menu_bar*om_autoformat.mnemonic: f
*compose_window*om_return-receipt.label: Return-Receipt
!*compose_window*menu_bar*om_return-receipt.mnemonic: R
*compose_window*om_edit-headers.label: Edit Headers
!*compose_window*menu_bar*om_edit-headers.mnemonic: E
*compose_window*om_record-msg.label: Record Message
!*compose_window*menu_bar*om_record-msg.mnemonic: M
*compose_window*om_options.label: Options ...
!
! Include menu
!
*compose_window*menu_bar.include.label: Include
!*compose_window*menu_bar.include.mnemonic: I
! include menu (im) items
*compose_window*menu_bar*im_selected.label: Indented Message(s)
!*compose_window*menu_bar*im_selected.mnemonic: I
*compose_window*menu_bar*im_forward.label: Unmodified Message(s)
!*compose_window*menu_bar*im_forward.mnemonic: U
!
! Compose menu already handled above.
! ...
!
! Deliver menu
!
*compose_window*menu_bar.deliver.label: Deliver
!*compose_window*menu_bar.deliver.mnemonic: D
! deliver menu (dm) items
*compose_window*menu_bar*dm_send.label: Send
!*compose_window*menu_bar*dm_send.mnemonic: S
*compose_window*menu_bar*dm_send.accelerator: Ctrl Shift<s>
*compose_window*menu_bar*dm_send.acceleratorText: Shift+Ctrl+S
*compose_window*menu_bar*dm_send_n_close.label: Send, Close Window
*compose_window*menu_bar*dm_send_n_close.accelerator: Ctrl Shift<d>
*compose_window*menu_bar*dm_send_n_close.acceleratorText: Shift+Ctrl+D
!*compose_window*menu_bar*dm_send_n_reuse.label: Send, Leave intact
!*compose_window*menu_bar*dm_send_n_reuse.accelerator: Ctrl Shift<x>
!*compose_window*menu_bar*dm_send_n_reuse.acceleratorText: Shift+Ctrl+X
!*compose_window*menu_bar*dm_send_n_clear.label: Send, Erase window
!*compose_window*menu_bar*dm_send_n_clear.accelerator: Ctrl Shift<e>
!*compose_window*menu_bar*dm_send_n_clear.acceleratorText: Shift+Ctrl+E
*compose_window*menu_bar*dm_cancel.label: Cancel
!*compose_window*menu_bar*dm_cancel.mnemonic: C
*compose_window*menu_bar*dm_cancel.accelerator: Ctrl<c>
*compose_window*menu_bar*dm_cancel.acceleratorText: Ctrl+C
!
! Headers menu
!
*compose_window*menu_bar.headers.label: Headers
!*compose_window*menu_bar.headers.mnemonic: a
! headers menu (hdrs) items
*compose_window*menu_bar*hdrs_all.label: All
!*compose_window*menu_bar*hdrs_all.mnemonic: A
*compose_window*menu_bar*hdrs_to.label: To
!*compose_window*menu_bar*hdrs_to.mnemonic: T
*compose_window*menu_bar*hdrs_subject.label: Subject
!*compose_window*menu_bar*hdrs_subject.mnemonic: S
*compose_window*menu_bar*hdrs_cc.label: Cc
!*compose_window*menu_bar*hdrs_cc.mnemonic: C
*compose_window*menu_bar*hdrs_bcc.label: Bcc
!*compose_window*menu_bar*hdrs_bcc.mnemonic: B
*compose_window*menu_bar*hdrs_priority.label: Prioritize
!*compose_window*menu_bar*hdrs_priority.mnemonic: P
*compose_window*menu_bar*emp_none.label: Normal
*compose_window*menu_bar*hdrs_spacer.label:
*compose_window*menu_bar*hdrs_custom.label: Custom ...
!*compose_window*menu_bar*hdrs_custom.mnemonic: u
*compose_window*menu_bar*hdrs_aliases.label: Aliases ...
!*compose_window*menu_bar*hdrs_aliases.mnemonic: A
!
! Help menu
!
*compose_window*menu_bar.help.label: Help
!*compose_window*menu_bar.help.mnemonic: H
! help menu (hm) items
*compose_window*menu_bar*hm_compose_win.label: Compose Window ...
!*compose_window*menu_bar*hm_compose_win.mnemonic: C
!*compose_window*menu_bar*hm_compose_win.accelerator: <F1>
!*compose_window*menu_bar*hm_compose_win.acceleratorText: F1
*compose_window*menu_bar*hm_addressing.label: Addressing ...
!*compose_window*menu_bar*hm_addressing.mnemonic: A
