#! /bin/sh
#
# Complete a Z-Mail upgrade
#
# This is intended to be sourced with the "." command from zminstall.sh.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

if test -z "$SOFTWARE"
then
    if test -f install.sh
    then
	cd ..
    fi
    if test -f doinstall.sh
    then
	. doinstall.sh
    else
	echo "I can't run by myself and I can't find doinstall.sh!" 1>&2
	exit 1
    fi
    exit 0
fi

. scripts/getversion.sh

case $OVERSION in
2.0*) cat <<EOM

Due to incompatibilities between $OVERSION and $NVERSION, it is
necessary to completely reinstall $SOFTWARE when doing this upgrade, or
to install the two versions in different locations.  To install in
different locations, simply specify a new location for the $SOFTWARE
library and executables when asked to do so.

EOM
    def=Yes
    echo $n "Revert to full installation? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" = "n"
    then
	def=No
	echo $n "Proceed with upgrade? [$def] $c"
	read ans
	ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
	if test "$ans" != "y"
	then
	    echo "Installation discontinued."
	    exit 1
	fi
    else
	ZC_DOING_UPGRADE=false
	. scripts/fresh.sh
	exit 1
    fi 
    ;;
2.1.[0-5]) ;;
esac

#. scripts/nlscheck.sh

cat << EOM

-------------------------

I am now ready to upgrade your $SOFTWARE.  If you do not wish to
alter the existing copy, you should discontinue installation now.

Nothing has been permanently changed up to this point.  However, if
you continue, the existing installation will be upgraded by the new
copy.  Backup copies will be made of any files that have changed since
your last installation.  Backups will have the same name as the
original, with the addition of a '-' at the end of the filename.  Any
files already ending with a '-' may be overwritten.

EOM

def=Yes
echo $n "Do you wish to continue the upgrade? [$def] $c"
read ans
ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
if test "$ans" != "y"
then
    echo "Upgrade discontinued."
    exit
fi

# Everything OK, so here we go ....

. scripts/setmkdir.sh

cat <<EOM

-------------------------

Checking existence of various system directories ...
EOM

ERROR=false
WARNING=false

if test ! -d "$USRLOCAL"
then
    $mkdir $USRLOCAL || exit 1
fi
if test ! -d "$BIN"
then
    $mkdir $BIN || exit 1
fi

# See if we are limited to 14-char file names.
( echo OK > /tmp/123456789.12345 ) 2>/dev/null
if test -f /tmp/123456789.12345
then
    HOW_TO_MOVE='mv $file_to_write ${file_to_write}-'
else
    NAME14='\(.*/[^/][^/][^/][^/][^/][^/][^/][^/][^/][^/][^/][^/][^/]\)[^/]*$'
    SUB14="s@$NAME14@\1@"
    HOW_TO_MOVE='mv $file_to_write `echo $file_to_write | sed '"'$SUB14'"'`-'
fi

HOW_TO_COPY='/bin/cp $file_to_read $file_to_write'
HOW_TO_SAVE='cmp -s $file_to_read $file_to_write || '"$HOW_TO_MOVE"

# Now we're ready to go.

echo ""
echo $n "Upgrading $ZMLIB ...$c" 1>&2
if test ! -d "$ZMLIB"
then
    $mkdir $ZMLIB || exit 1
fi

if test -f scripts/obsolete.sh
then
    . scripts/obsolete.sh
fi

ECHO_DOTS=true
TREE_TO_READ=lib
TREE_TO_WRITE=$ZMLIB
. scripts/copytree.sh

#TREE_TO_READ=doc
TREE_TO_WRITE=$ZMLIB
#. scripts/copytree.sh

if test -f hostid
then
    cp hostid $ZMLIB || WARNING=true
    chmod a+x $ZMLIB/hostid || WARNING=true
fi
if $SGID
then
    cat <<EOM >$ZMLIB/lock.zmailrc
##
# This file is created during $SOFTWARE installation
# but is NOT backed up during the upgrade procedure.
#
# If you modify this file, your changes may be lost.
##

# Set use of file locking for SGID $ZMGROUP
set dot_lock
EOM
fi
echo " done." 1>&2

# Check that the license directories exists
#. scripts/license.sh

if test "$ERROR" != true
then
    echo ""
    echo $n "Resetting owner, group, and mode in $ZMLIB ...$c"

    find $ZMLIB -exec chown $LIBOWNER {} \; || WARNING=true
    echo $n ".$c"
    find $ZMLIB -exec chgrp $LIBGROUP {} \; || WARNING=true
    echo $n ".$c"
    find $ZMLIB -type d -exec chmod a+rx {} \; || ERROR=true
    echo $n ".$c"
    find $ZMLIB -type f -exec chmod a+r {} \; || ERROR=true
    echo $n ".$c"
    find $ZMLIB/bin -type f -exec chmod a+x {} \; || ERROR=true
    echo " done."
fi
echo ""

# Install X resources
. scripts/xresinstall.sh

echo ""
echo $n "Replacing executables in $BIN ...$c"
if $BACKUP_EXECUTABLES
then
    mv $BIN/zmail $BIN/zmail- 2>/dev/null
else
    mv $BIN/zmail $BIN/.zmail 2>/dev/null
fi
if cp zmail $BIN/zmail.bin
then
    /bin/rm -f $BIN/.zmail
    chown $ZMOWNER $BIN/zmail.bin || WARNING=true
    chgrp $ZMGROUP $BIN/zmail.bin || WARNING=true
    chmod a+x $BIN/zmail.bin || WARNING=true
    if $SGID
    then
	chmod g+s $BIN/zmail.bin || WARNING=true
    fi
cat > $BIN/zmail <<EOM || ERROR=true
:
ZMLIB=$ZMLIB
export ZMLIB
                                                                                
NLSPATH=$ZMLIB/locale/C/%N
export NLSPATH

exec $BIN/zmail.bin \${1+"\$@"}
EOM
    chown $ZMOWNER $BIN/zmail || WARNING=true
    chgrp $ZMGROUP $BIN/zmail || WARNING=true
    chmod a+x $BIN/zmail || WARNING=true
#    if $SGID
#    then
#	chmod g+s $BIN/zmail || WARNING=true
#    fi

else
    if $BACKUP_EXECUTABLES
    then
	mv $BIN/zmail- $BIN/zmail 2>/dev/null
    else
	mv $BIN/.zmail $BIN/zmail 2>/dev/null
    fi
    ERROR=true
fi
if test -f zmail.small
then
    if $BACKUP_EXECUTABLES
    then
	mv $BIN/zmail.small $BIN/zmail.small- 2>/dev/null
    else
	mv $BIN/zmail.small $BIN/.zmail.small 2>/dev/null
    fi
    if cp zmail.small $BIN/zmail.small.bin
    then
	/bin/rm -f $BIN/.zmail.small
	chown $ZMOWNER $BIN/zmail.small.bin || WARNING=true
	chgrp $ZMGROUP $BIN/zmail.small.bin || WARNING=true
	chmod a+x $BIN/zmail.small.bin || WARNING=true
	if $SGID
	then
	    chmod g+s $BIN/zmail.small.bin || WARNING=true
	fi
    else
	if test -f $BIN/.zmail.small
	then
         cp $BIN/.zmail.small $BIN/zmail.small.bin
	    echo ""
	    echo "Existing zmail.small left in $BIN/.zmail.small"
	fi
	WARNING=true
    fi
cat > $BIN/zmail.small <<EOM
:
ZMLIB=$ZMLIB
export ZMLIB
                                                                                
NLSPATH=$ZMLIB/locale/C/%N
export NLSPATH

exec $BIN/zmail.small.bin \${1+"\$@"}
EOM
	chown $ZMOWNER $BIN/zmail.small || WARNING=true
	chgrp $ZMGROUP $BIN/zmail.small || WARNING=true
	chmod a+x $BIN/zmail.small || WARNING=true
	if $SGID
	then
	    chmod g+s $BIN/zmail.small || WARNING=true
	fi

fi
echo " done."

if test -f lib/bin/metamail && test -f lib/mailcap && test ! -f /etc/mailcap
then
    cat <<EOM

-------------------------

This version of $SOFTWARE supplies the "metamail" program for processing
messages formatted with the Multipurpose Internet Mail Extensions (MIME)
conventions.  In order for metamail to function properly, a configuration
file named "mailcap" needs to be installed in /etc on your system.  This
file is also included in your $SOFTWARE distribution.

You should be aware that the functioning of metamail is highly dependent
on the local environment.  After installing the mailcap file, you should
examine it and make changes accordingly.  Manual pages describing the
metamail package are supplied in $ZMLIB/man.

EOM
    def=No
    echo $n "Would you like to install the /etc/mailcap file? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" = "y"
    then
	cp lib/mailcap /etc/mailcap || WARNING=true
    fi
    echo ""
fi

. scripts/locale.sh zmail

if test "$L_ERROR" = true
then
    ERROR=true
fi

if test "$ERROR" = true
then
    echo "-------------------------"
    echo ""
    echo An error occurred during the upgrade.  
    echo ""
    exit 1
fi

echo ""

if test "$WARNING" = true
then
    cat <<EOM
-------------------------

A non-fatal error occured during the upgrade.  You may proceed
with $SOFTWARE registration, but you may see warning messages when
using the $SOFTWARE program.  None of these warnings should have
any significant effect on the operation of $SOFTWARE.
EOM
else
    cat <<EOM
-------------------------

No errors were detected during upgrade into these locations:

    Library:      $ZMLIB
EOM
#    if test "$NETWORK_LICENSE" = false
#    then
#	cat <<EOM
#    Licensing:    $LICENSE_DIR
#EOM
    fi
    cat <<EOM
    X Resources:  $XAPPLRESDIR
    Executables:  $BIN
EOM

# Dump our environment, appropriately munged, so register.sh can grab it.
# This is a hack, because if register.sh is not running in $ZMLIB, it will
# not be able to find this file anyway.

cat <<EOM > $ZMLIB/.register.env
BIN=$BIN
LICENSE_LIB=$LICENSE_LIB
LICENSE_DIR=$LICENSE_DIR
USRLOCAL=$USRLOCAL
USRLIB=$USRLIB
PATH=${PATH}:$BIN
ZMLIB=$ZMLIB
ZMSHIPDIR=$ZMSHIPDIR
ZMPASSWD=$ZMPASSWD
EOM

#if test -f "$ZCNLINST" && test "$DO_NLSINSTALL" = true
#then
#    echo ""
#    echo "-------------------------"
#    echo ""
#    echo "You may optionally install the Z-Mail Network License Server."
#    echo ""
#    def=Yes
#    echo $n "Install the network server now, on this machine? [$def] $c"
#    read ans
#    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
#    if test "$ans" = "y"
#    then
#	cd $ZCNLSDIR
#	exec $REAL_SH `basename $ZCNLINST`
#    fi
#elif test "$NETWORK_LICENSE" = true
#then
#    echo ""
#    echo Please call $ZC_SW_COMPANY at $PHONE_NUMBER for instructions on
#    echo installation of your Z-Mail Network License Server software.
#    echo ""
#fi

#if test "$NETWORK_LICENSE" = true
#then
#    cat <<EOM
#
#You must install the network license server in order to register your
#activation keys and users.  If the license server is to be installed
#on another machine, register your keys and users there.
#EOM
#fi
#

if test -d "$ZMSHIPDIR"
then
    if test "$ZMSHIPDIR" != "$ZMLIB"
    then
	cat <<EOM

$SOFTWARE installation is complete.  
    fi
fi

#if test "$NETWORK_LICENSE" != true
#then
#    echo ""
#    def=No
#    echo $n "Do you need to register a new Z-Mail activation key? [$def] $c"
#    read ans
#    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
#    if test "$ans" = "y"
#    then
#	echo ""
#	. scripts/register.sh
#fi
#fi

#cat <<EOM
#
#$SOFTWARE installation and registration script finished.

EOM

exit 0
