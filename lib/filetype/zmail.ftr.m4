undefine(`include')dnl
define(rule,`$1	$2')dnl
define(typeRule,`rule(TYPE,$1Executable)')dnl
define(matchRule,`rule(MATCH,ifelse($2,,tag,(tag & 0xffff`'eval(0xffff ^ 0x$2,16,4))) == 0x`'eval(0xe50000+0x$1,16,8);)')dnl
define(legendRule,`rule(LEGEND,$1 Executable)')dnl
define(superRule,`rule(SUPERTYPE,ZmailExecutable $1Executable)')dnl
define(helpRule,`rule(MENUCMD,":644:Help Overview" /usr/lib/sysmon/sgihelpcmd -b $1Help -k General)')dnl
define(cmdRule,`rule(CMD $1,$2)')dnl
define(openRules,`cmdRule(OPEN,ifelse($2,,,$2 )$1)
cmdRule(ALTOPEN,`launch ifelse($2,,,-h "$2" )-c $1')')dnl
define(dropRule,`rule(DROPIF,MailFile MailFolder Directory)
cmdRule(DROP,if [ $ARGC -eq 1 ]; then
		  $1 -folder $SELECTED
		else
		  inform -header $TARGET -icon warning "$TARGET can only take one dropped folder at a time."
		fi)')dnl
define(iconPart,`{
	    include("../iconlib/generic.exec.$1.fti");
	    include("iconlib/mail-pouch.$1.fti");
	  }')dnl
define(iconRule,`rule(ICON,{
	  if (opened) iconPart(open) else iconPart(closed)
	})')dnl
define(guiRules,superRule(GenericWindowed)
openRules($LEADER -gui $REST)
dropRule($TARGET -gui))dnl
define(ttyRules,superRule(tty)
openRules($LEADER $REST,winterm -c)
dropRule(winterm -c $TARGET))dnl
dnl
dnl
dnl
dnl
define(zmlib,`${ZMLIB-/usr/ifdef(`MEDIAMAIL',,local/)lib/ifdef(`VUI',Zmlite,Zmail)}')dnl
define(mplib,`${MP_PROLOGUE-zmlib/mp}')dnl
dnl
dnl
dnl
dnl
TYPE	MailFolder
MATCH	string(0,5) == "From " && ascii;
LEGEND	mail folder
SUPERTYPE	MailFile
openRules(${ZMAIL-ifdef(`MEDIAMAIL',MediaMail,ifdef(`MOTIF',zmail,ifdef(`VUI',zmlite,zmail.small)))} ifdef(`MOTIF',-gui )-folder $LEADER)
DROPIF	MailFile MailFolder
CMD DROP	cat $SELECTED >> $TARGET
ICON	{
 	  include("../system/iconlib/MailFile.shadow.fti");
	  if (opened)
	    {
	      include("../system/iconlib/MailFile.open.fti");
	    }
	  else
	    {
	      include("../system/iconlib/MailFile.closed.fti");
	    }
        }

CONVERT	MailFolder PostScriptFile
COST	100
FILTER	export MP_PROLOGUE; MP_PROLOGUE="mplib" eval ${MP-zmlib/bin/mp\ -m}


typeRule(MediaMail)
matchRule(d,2)
legendRule(MediaMail)
helpRule(MediaMail)
guiRules
iconRule


typeRule(ZmailMotif)
matchRule(c)
legendRule(Z-Mail for Motif)
guiRules
iconRule


typeRule(ZmailLite)
matchRule(4)
legendRule(Z-Mail Lite)
ttyRules
iconRule


typeRule(Zmail)
matchRule(0,ffff)
legendRule(Z-Mail)
ttyRules
iconRule
