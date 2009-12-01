/^menu -W main/ { parent = "MainMenu"; pos = 0; continue; }
/^menu -W message/ { parent = "MessageMenu"; pos = 0; continue; }
/^menu -W compose/ { parent = "ComposeMenu"; pos = 0; continue; }
/^button -W main/ { parent = "MainActions"; pos = 0; continue; }
/^button -W message/ { parent = "MessageActions"; pos = 0; continue; }
/^button -W compose/ { parent = "ComposeActions"; pos = 0; continue; }
/^menu -B/ { parent = $3; pos = 0; continue; }
/^button -B/ { parent = $3; pos = 0; continue; }
/^menu.*popup.*main-summaries/ { parent = "MainSummariesPopupMenu"; pos = 0; continue; }
/^menu.*popup.*output-statictext/ { parent = "OutputStatictextPopupMenu"; pos = 0; continue; }
/^menu.*popup.*read-message-body/ { parent = "ReadMessageBodyPopupMenu"; pos = 0; continue; }
/^menu.*popup.*commandline-af/ { parent = "CommandlineAFPopupMenu"; pos = 0; continue; }
/^menu.*popup.*compose-body/ { parent = "ComposeBodyPopupMenu"; pos = 0; continue; }
$0 ~ /^menu.*-name/ || $0 ~ /^button.*-name/ {
    if (parent ~ /MainActions/) continue;
    match($0, /-name [^ ][^ ]*/);
    max = RSTART+RLENGTH;
    name = substr($0, RSTART+6, RLENGTH-6);
    menu = "NULL"; sens = "0"; focus = "0"; value = "0";
    sep = ""; help = ""; submenu = ""; toggle = ""; menuc = "";
    icon = "NULL";
    req = "BT_REQUIRES_SELECTED_MSGS|";
    type = "BtypePushbutton";
    if ($0 ~ /^menu/) menucmd = "BT_MENU_CMD|";
    if (match($0, /-n /)) {
	req = "";
	end = RSTART+RLENGTH; if (end > max) max = end;
    }
    if (match($0, /-menu [^ ]*/)) {
	menu = "\"" substr($0, RSTART+6, RLENGTH-6) "\"";
	end = RSTART+RLENGTH; if (end > max) max = end;
	submenu = "";
	type = "BtypeSubmenu";
    }
    if (match($0, /-icon [^ ]*/)) {
	icon = "\"" substr($0, RSTART+6, RLENGTH-6) "\"";
	end = RSTART+RLENGTH; if (end > max) max = end;
    }
    if (match($0, /-separator/)) {
	sep = "";
	type = "BtypeSeparator";
	end = RSTART+RLENGTH; if (end > max) max = end;
    }
    if (match($0, /-help-menu/)) {
	help = "BT_HELP|";
	end = RSTART+RLENGTH; if (end > max) max = end;
    }
    if (match($0, /-sensitivity '[^']*'/)) {
	sens = "\"" substr($0, RSTART+14, RLENGTH-15) "\"";
	end = RSTART+RLENGTH; if (end > max) max = end;
    }
    if (match($0, /-value '[^']*'/)) {
        value = "\"" substr($0, RSTART+8, RLENGTH-9) "\"";
	toggle = "";
	type = "BtypeToggle";
	end = RSTART+RLENGTH; if (end > max) max = end;
    }
    if (match($0, /-focus-condition '[^']*'/)) {
	end = RSTART+RLENGTH; if (end > max) max = end;
        focus = "\"" substr($0, RSTART+18, RLENGTH-19) "\"";
    }
    script = substr($0, max);
    if (match(script, /^ */)) script = substr(script, RLENGTH+1);
    if (match(script, /'.*'/) && RLENGTH == length(script))
	script = substr(script, 2, RLENGTH-2);
    if (match(script, /".*"/) && RLENGTH == length(script))
 	script = substr(script, 2, RLENGTH-2);
    gsub(/"/, "\\\"", script);
    if (script ~ /^$/)
	script = "NULL";
    else
	script = "\"" script "\"";
    if (pos == 0)
       	parentstr = "\"" parent "\"";
    else
    	parentstr = "NULL";
    printf "{ { \"%s\", 0, 0, }, NULL,", name;
    printf "    %s%s%s%s%s%s0,\n", req, sep, help, toggle, submenu, menucmd;
    printf "    %s, %s, 0, (void_proc) 0, NULL, NULL,\n", parentstr, script;
    printf "    %s, %d,\n", menu, ++pos;
    printf "    (struct DynConditionRec *) %s,\n", sens;
    printf "    (struct DynConditionRec *) %s,\n", value;
    printf "    (struct DynConditionRec *) %s, %s, NULL, %s },\n", focus, icon, type;
    continue;
}
$0 ~ /^$/ && (pos != 0) {
    printf "{ { NULL, }, },\n"; pos = 0;
}
/^menu/ { print "ERROR:no -name:" NR; }
BEGIN { parent = "MainMenu"; printf "zmButton def_buttons[] = {"; }
END { printf "{ { NULL, }, } } ;\n" }
