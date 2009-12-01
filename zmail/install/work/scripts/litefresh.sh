#! /bin/sh
#
# Complete Z-Mail installation or re-installation as a fresh copy
#
# This is intended to be sourced with the "." command from liteinstall.sh.
#
# Copyright 1993-98 NetManage,   All rights reserved.

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

if $RM_IT
then
    if $WARNING
    then
	cat <<EOM

-------------------------

I am now ready to install your new $SOFTWARE.  If you do not wish to
remove the existing copy, you should discontinue installation now.

Nothing has been permanently changed up to this point.  However,
if you continue, the existing installation will be removed and
the new copy installed.  These files will be backed up if they
exist: $backitup

EOM
    else
	cat <<EOM

-------------------------

I am now ready to install your new $SOFTWARE.

Nothing has been permanently changed up to this point.  However,
if you continue, $SOFTWARE will be copied into the locations you have
specified.  These files will be backed up if they exist: $backitup

EOM
    fi
    def=Yes
    echo $n "Proceed with $SOFTWARE installation? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" != "y"
    then
	echo Installation discontinued.
	exit
    fi
    if mkdir /tmp/zmbackup.$$
    then
	for f in $backitup
	do
	    if test -f "$f"
	    then
		echo ""
		echo "Backing up $f in /tmp/zmbackup.$$ ..."
		/bin/cp $f /tmp/zmbackup.$$
	    fi
	done
    else
	echo "Unable to create directory for backups!"
	def=No
	echo $n "Proceed with $SOFTWARE installation? [$def] $c"
	read ans
	ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
	if test "$ans" != "y"
	then
	    echo Installation discontinued.
	    exit
	fi
    fi
    if test "$WARNING" = true -a "$RM_IT" = true
    then
	echo ""
	echo Removing $ZMLIB ...
    fi
    /bin/rm -rf $ZMLIB
else
    cat <<EOM

Your $SOFTWARE library may not match the other files that are
about to be installed.  You should install $ZMLIB
at the earliest possible opportunity.
EOM
fi

if $RM_IT && test -d "$ZMLIB"
then
    cat <<EOM

-------------------------

$ZMLIB still exists.  Proceeding with installation will
overwrite all contents of this directory.

EOM
    def=No
    echo $n "Are you sure you wish to continue? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" != "y"
    then
	echo "Installation discontinued."
	exit
    fi
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

if $RM_IT
then
    echo ""
    echo $n "Installing $ZMLIB ...$c"
    if test ! -d "$ZMLIB"
    then
	$mkdir $ZMLIB || exit 1
    fi
    test -d "$ZMLIB" || exit 1
    ECHO_DOTS=true
    TREE_TO_READ=lib
    TREE_TO_WRITE=$ZMLIB
    . scripts/copytree.sh
#   catalog needs to be same name as executable
    cp $ZMLIB/locale/C/zmlite $ZMLIB/locale/C/zmlite.$ZCARCHITECTURE
#    TREE_TO_READ=doc
#    . scripts/copytree.sh
    if test -f hostid
    then
	cp hostid $ZMLIB/bin || WARNING=true
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
    echo " done."
fi

# Check that the license directories exists
#. scripts/license.sh

if test "$ERROR" != true
then
    echo Resetting owner, group, and mode in $ZMLIB ...

    find $ZMLIB -exec chown $LIBOWNER {} \; || WARNING=true
    find $ZMLIB -exec chgrp $LIBGROUP {} \; || WARNING=true
    find $ZMLIB -type d -exec chmod a+rx {} \; || ERROR=true
    find $ZMLIB -type f -exec chmod a+r {} \; || ERROR=true
    find $ZMLIB/bin -type f -exec chmod a+x {} \; || ERROR=true
fi
echo ""

echo Installing executables in $BIN...
mv $BIN/zmlite $BIN/.zmlite 2>/dev/null
mv $BIN/zmlite.$ZCARCHITECTURE $BIN/.zmlite.$ZCARCHITECTURE 2>/dev/null
mv $BIN/ztermkey $BIN/.ztermkey 2>/dev/null
cat > $BIN/zmlite <<EOM
:
ZMLIB=$ZMLIB
export ZMLIB

NLSPATH=$ZMLIB/locale/C/%N
export NLSPATH

# Uncomment the following if you want Z-Mail Lite to use a different
# user-startup file than \$HOME/.zmailrc (to avoid conflicts with
# settings created by other versions of Z-Mail)
# 
# ZMAILRC=\$HOME/.zmliterc
# export ZMAILRC

exec $BIN/zmlite.$ZCARCHITECTURE \${1+"\$@"}
EOM

if cp zmail $BIN/zmlite.$ZCARCHITECTURE && cp lib/bin/ztermkey $BIN/ztermkey
then
    /bin/rm -f $BIN/.zmlite
    /bin/rm -f $BIN/.zmlite.$ZCARCHITECTURE
    /bin/rm -f $BIN/.ztermkey
    chown $ZMOWNER $BIN/zmlite $BIN/zmlite.$ZCARCHITECTURE $BIN/ztermkey || WARNING=true
    chgrp $ZMGROUP $BIN/zmlite $BIN/zmlite.$ZCARCHITECTURE $BIN/ztermkey || WARNING=true
    chmod a+rx $BIN/zmlite $BIN/zmlite.$ZCARCHITECTURE $BIN/ztermkey || WARNING=true
    if $SGID
    then
        chmod g+s $BIN/zmlite.$ZCARCHITECTURE || WARNING=true
    fi
    if $SGID
    then
        chmod g+s $BIN/zmlite || WARNING=true
    fi
else
    mv $BIN/.zmlite $BIN/zmlite 2>/dev/null
    mv $BIN/.zmlite.$ZCARCHITECTURE $BIN/zmlite.$ZCARCHITECTURE 2>/dev/null
    mv $BIN/.ztermkey $BIN/ztermkey 2>/dev/null
    ERROR=true
fi
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
    echo "-------------------------"
fi

. scripts/locale.sh zmlite

if test "$L_ERROR" = true
then
    ERROR=true
fi

if test "$ERROR" = true
then
    echo ""
    echo An error occurred during installation.  
    echo ""
    exit 1
fi

echo ""

if test "$WARNING" = true
then
    cat <<EOM
A non-fatal error occured during installation.  You may proceed
with $SOFTWARE registration, but you may see warning messages when
using the $SOFTWARE program.  None of these warnings should have
any significant effect on the operation of $SOFTWARE.
EOM
else
    cat <<EOM
No errors were detected during installation into these locations:

    Library:      $ZMLIB
EOM
#    if test "$NETWORK_LICENSE" = false
#    then
#	cat <<EOM
#    Licensing:    $LICENSE_DIR
#EOM
#    fi
    cat <<EOM
    Executables:  $BIN
EOM
fi

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
#    echo Please call NetManage at $PHONE_NUMBER for instructions on
#    echo installation of your Z-Mail Network License Server software.
#    echo ""
#fi
#
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

$SOFTWARE installation completed. 
EOM
    fi
fi

#if test "$NETWORK_LICENSE" = true
#then
#    exit 0
#else
#    echo ""
#fi

#. scripts/register.sh

#cat <<EOM
#
#$SOFTWARE installation and registration script finished.
#
#EOM

exit 0
