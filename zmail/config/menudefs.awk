BEGIN {
  menucontext = "main";
  buttoncontext = "main";

  specialSyms["Home"] = "osfBeginLine";
  specialSyms["End"]  = "osfEndLine";
  specialSyms["Up"]   = "osfUp";
  specialSyms["Down"] = "osfDown";
  specialSyms["Slash"] = "slash";
}
/^menu -W/ { menucontext = $3; window = $3 "_window*menu_bar*"; next; }
/^button -W/ { buttoncontext = $3; next; }
/^menu -B/ {
    menucontext = $3;
    if ($3 ~ /^Main*/) window = "main_window*menu_bar*";
    else if ($3 ~ /^Message*/) window = "message_window*menu_bar*";
    else if ($3 ~ /^Compose*/) window = "compose_window*menu_bar*";
    else window = "";
    next;
}
/^menu.*popup.*main-summaries/ {
    window = "message_summaries*popup_menu*";
    next;
}
/^menu.*popup.*output-statictext/ {
    window = "main_output_text*popup_menu*";
    next;
}
/^menu.*popup.*read-message-body/ {
    window = "message_text*popup_menu*";
    next;
}
/^menu.*popup.*commandline-af/ {
    window = "command_area*popup_menu*";
    next;
}
/^menu.*popup.*compose-body/ {
    window = "compose_window.body*popup_menu*";
    next;
}
/^menu.*-name/ {
    match($0, /-name [^ ][^ ]*/);
    name = "*" window substr($0, RSTART+6, RLENGTH-6);
    if (match($0, /-label '[^']*'/) || match($0, /-label "[^"]*"/))
	printf "%s.labelString: %s\n", name, substr($0, RSTART+8, RLENGTH-9);
    else if (match($0, /-label [^ ]*/))
	printf "%s.labelString: %s\n", name, substr($0, RSTART+7, RLENGTH-7);
    if (match($0, /-mnemonic ./))
	printf "%s.mnemonic: %s\n", name, substr($0, RSTART+RLENGTH-1, 1);
    if (match($0, /-M ./))
	printf "%s.mnemonic: %s\n", name, substr($0, RSTART+RLENGTH-1, 1);
    if (match($0, /-accelerator '[^']*'/) || match($0, /-accelerator "[^"]*"/)) {
	printf "%s.acceleratorText: %s\n", name,
	       acc = substr($0, RSTART+14, RLENGTH-15);
	gsub(/\+/, " ", acc);
	m = match(acc, / [^ ]*$/);
	keymod = substr(acc, 1, m-1);
	keysym = substr(acc, m+1);
	if (keysym in specialSyms) keysym = specialSyms[keysym];
	printf "%s.accelerator: %s<Key>%s\n", name, keymod, keysym;
    } else if (match($0, /-accelerator/))
	print "ERROR:no quotes around -accelerator:" NR;
    next;
}
/^menu.*separator/ { next; }
/^menu/ { print "ERROR:no -name:" NR; }
END {
    if (menucontext !~ "main") print "ERROR:menucontext != main:" NR;
    if (buttoncontext !~ "main") print "ERROR:buttoncontext != main:" NR;
}
