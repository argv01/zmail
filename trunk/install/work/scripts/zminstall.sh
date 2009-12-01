#! /bin/sh
#
# Install Z-Mail
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

case $ZC_INTRODUCTION in
$$) ;;
*)
    ZC_INTRODUCTION=$$
    export ZC_INTRODUCTION

    SOFTWARE='OpenZMail Classic'
    export SOFTWARE

    USRLOCAL=/usr/local
    USRLIB=/usr/lib
    export USRLOCAL USRLIB

    USE_BINARY=./zmail
    . scripts/getversion.sh
    USE_BINARY=''
    if test -n "$VERSION"
    then
	echo "You appear to be installing Version $VERSION of $SOFTWARE."
	echo ""
    fi
    echo $n "Press return to continue. $c"
    read ans
    ;;
esac

#. scripts/actkey.sh
. scripts/beroot.sh

# Now jump through hoops to make sure this all works right
case $ZC_INSTALL_PID in
$$) ;;
*)
    ZMSHIPDIR=`pwd`
    export ZMSHIPDIR

    ZCARCHITECTURE=`basename $ZMSHIPDIR | sed 's/z.*\.//'`
    export ZCARCHITECTURE

    echo ""
    echo Performing sanity checks ...

    . scripts/fixarch.sh
    . scripts/zmarch.sh

    # Do this after fixarch.sh because fixarch.sh may re-exec us.
    ZC_INSTALL_PID=$$
    export ZC_INSTALL_PID
    ;;
esac

# Now, finally, initialize defaults and get on with it

. scripts/upcheck.sh
. scripts/xrescheck.sh

# Input the locations where everthing should be installed

case $ZC_DOING_UPGRADE in
true)
    RM_IT=false
    ans=n
    cat <<EOM

-------------------------

To upgrade existing $SOFTWARE software, I need to know where to find it.
I will use information taken from your environment if possible.  Please
confirm or correct each of the following locations for your existing
$SOFTWARE installation.
EOM
    ;;
*)
#    . scripts/nlscheck.sh
    RM_IT=true
    cat <<EOM

-------------------------

If you wish to customize locations for the installation on this machine,
you may do so at this time.  Otherwise, $SOFTWARE will be installed in the
following default directories:

    Library:      $ZMLIB
    X Resources:  $XAPPLRESDIR
    Executables:  $BIN

EOM
    def=Yes
    echo $n "Do you wish to install $SOFTWARE in the above locations? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    ;;
esac

if test "$ans" != "y"
then
    if test "$ZC_DOING_UPGRADE" != true
    then
	cat <<EOM

-------------------------

$SOFTWARE expects to find initialization data and online help
in $ZMLIB.  If you change the location of $ZMLIB,
$SOFTWARE users will need to set the environment variable ZMLIB
to the new name.
EOM

    fi
    while :
    do
    cat <<EOM

-------------------------

Enter directory for $SOFTWARE library:
EOM
    def=$ZMLIB
    echo $n "[$def] $c"
    read ans
    if test ! -z "$ans"
    then
	if test "$ZMLIB" = "$ZMSHIPDIR"
	then
	    cat <<EOM

Sorry, I can't install $SOFTWARE in $ZMSHIPDIR.
If you wish to install $SOFTWARE in $ZMSHIPDIR,
please begin by reading your $SOFTWARE tape or CDROM into a different
directory, and then re-execute this installation script from there.

Installation aborted.
EOM
	    exit 1
	else
	    ZMLIB=$ans
	fi
    fi

    if test "$ZC_DOING_UPGRADE" = true
    then
	if ! test -d "$ZMLIB"
	then
	    echo "I can't find the directory $ZMLIB."
	    echo "Please enter the correct location for the $SOFTWARE library,"
	    echo "or proceed as if this $SOFTWARE were a new installation."
	    echo ""
	    continue
	    exit 1
	fi
	if test ! -f $ZMLIB/variables
	then
	    echo "I can't find the file $ZMLIB/variables."
	    echo "Please enter the correct location for the $SOFTWARE library,"
	    echo "or proceed as if this $SOFTWARE were a new installation."
	    echo ""
	    continue
	    exit 1
	fi
    else
	cat <<EOM

-------------------------

The $SOFTWARE executables may be installed in any directory that
normally appears in the PATH environment variable.
EOM
    fi
    cat <<EOM

Enter directory for $SOFTWARE executables:
EOM
    def=$BIN
    echo $n "[$def] $c"
    read ans
    if test ! -z "$ans"
    then
	BIN=$ans
    fi
    if test ! -d $BIN -o "$ZC_DOING_UPGRADE" = true -a ! -f $BIN/zmail
    then
	if test "$ZC_DOING_UPGRADE" = true
	then
	    echo    "WARNING: $BIN/zmail does not exist."
	else
	    echo    "WARNING: $BIN does not exist."
	fi
	echo $n "         Install in $BIN anyway? [No] $c"
	read ans
	ans=`echo $ans | sed 's/\(.\).*/\1/' | tr YN yn`
	if test "$ans" = y
	then
	    break
	else
	    BIN=$def
	fi
    else
	if "$ZC_DOING_UPGRADE" = true
	then
	    echo $n "Make backup copies of the old executables? [No] $c"
	    read ans
	    ans=`echo $ans | sed 's/\(.\).*/\1/' | tr YN yn`
	    if test "$ans" = y
	    then
		BACKUP_EXECUTABLES=true
	    else
		BACKUP_EXECUTABLES=false
	    fi
	else
	    BACKUP_EXECUTABLES=true
	fi
	break
    fi
    done

    USRLOCAL=`echo $BIN | sed -e 's:/[^/]*$::' -e 's:^$:/:'`

    if test ! -f $ZAPPLRESDIR/$ZCAPPDEF
    then
	cat <<EOM

-------------------------

You do not appear to have an application defaults file for $SOFTWARE
in $ZAPPLRESDIR/$ZCAPPDEF.
EOM
    elif test "$ZC_DOING_UPGRADE != true
    then
	cat <<EOM

-------------------------
EOM
    fi
    if test ! -f $ZAPPLRESDIR/$ZCAPPDEF -o "$ZC_DOING_UPGRADE != true
    then
	cat <<EOM

$SOFTWARE expects to find application default X resources in
$ZAPPLRESDIR.  If you change the location of
$ZAPPLRESDIR, $SOFTWARE users will need to set the
environment variable XAPPLRESDIR to the new name.
EOM
    fi
    cat <<EOM

Enter directory for X application defaults:
EOM
    def=$XAPPLRESDIR
    echo $n "[$def] $c"
    read ans
    if test ! -z "$ans"
    then
	XAPPLRESDIR=$ans
    fi

    cat <<EOM

-------------------------

You have specified the following locations:

    Library:      $ZMLIB
    X Resources:  $XAPPLRESDIR
    Executables:  $BIN

These directories will be created if they do not already exist.

EOM
    def=Yes
    echo $n "Do you wish to continue installation? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" != "y"
    then
	echo "Installation discontinued."
	exit
    fi
fi

case $PATH in
$BIN:*) ;;
*:$BIN) ;;
*:$BIN:*) ;;
*) cat <<EOM

WARNING: $BIN
         does not appear in your PATH environment variable.
         This may cause problems with user registration in the future.

EOM
    echo $n "Press return to continue. $c"
    read ans
    ;;
esac

echo ""

# Now make sure the existing file system is in a safe state
# This definition intentionally begins with a newline!
backitup="
    $ZMLIB/attach.types
    $ZMLIB/system.zmailrc
    $XAPPLRESDIR/$ZCAPPDEF"

if test -d "$ZMLIB"
then
    if test "$ZC_DOING_UPGRADE" != true
    then
	WARNING=true
	cat <<EOM
-------------------------

$ZMLIB already exists from a previous installation.
In order to continue installation, the existing $ZMLIB
must be removed, or you can restart the installation
and change the installation locations.

EOM
    fi
    if test "$ZMLIB" = "$LICENSE_LIB"
    then
	cat <<EOM
Installing a new copy of $SOFTWARE will replace the current registration
information and may interfere with $SOFTWARE sessions currently in
progress.

EOM
    fi
    def=Yes
    if "$ZC_DOING_UPGRADE"
    then
	cat <<EOM
Proceed with caution.  For your convenience, backup copies will be
made of any files that have changed since your last installation.
Backups will have the same name as the original, with the addition of
a '-' at the end of the filename.  Any files already ending with a '-'
may be overwritten.

Note that no matter what you say now, $ZMLIB will
not be altered until I say "I am now ready to upgrade your $SOFTWARE".

EOM
	echo $n "Will it be OK to upgrade $ZMLIB? [$def] $c"
    else
	cat <<EOM
Proceed with caution.  For your convenience, backup copies of the
following files will be made if they exist and you choose to continue:
$backitup

Note that no matter what you say now, $ZMLIB will
not be altered until I say "I am now ready to install your new $SOFTWARE".

EOM
	echo $n "Will it be OK to remove $ZMLIB? [$def] $c"
    fi
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" != "y"
    then
	if "$ZC_DOING_UPGRADE"
	then
	    echo Installation discontinued.
	    exit 1
	fi
	def=No
	echo $n "Do you wish to install other files anyway? [$def] $c"
	read ans
	ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
	if test "$ans" != "y"
	then
	    echo Installation discontinued.
	    exit 1
	fi
	RM_IT=false
    fi
else
    WARNING=false
fi

echo ""
echo "-------------------------"
echo ""

# Check for the ownership and permissions we should attach to everything

if $ZC_DOING_UPGRADE
then
    . scripts/lsgetgr.sh
    def=`eval $ls $ZMLIB/variables 2>/dev/null | awk '{ print $3 }'`
    if test -z "$def"
    then
	def=root
    fi
else
    cat <<EOM
Some platforms or sites have special accounts for installed software.
These accounts have names like "bin" or "daemon".  You may wish to
have one of these special accounts own the $SOFTWARE library, or you may
wish to assign ownership to an adminstrative person's account.

Most sites use the "root" username for ownership of these libraries.

EOM
    def=root
fi

getpasswd='((ypcat passwd) 2>/dev/null || cat /etc/passwd)'
getgroup='((ypcat group) 2>/dev/null || cat /etc/group)'

while :
do
    echo $n "Which username should own the files in $ZMLIB? [$def] $c"
    read ans
    ans=`echo $ans`
    if test -z "$ans"
    then
	LIBOWNER=$def
    else
	echo "Checking on $ans ..."
	if ( eval $getpasswd | grep -s "^${ans}:" ) >/dev/null 2>&1
	then
	    LIBOWNER=$ans
	else
	    echo "$ans is not a known account.  Please try another."
	    continue
	fi
    fi
    break
done

echo ""
if $ZC_DOING_UPGRADE
then
    . scripts/lsgetgr.sh
    def=`eval $ls $ZMLIB/variables 2>/dev/null | awk '{ print $4 }'`
else
    def=
fi
if test -z "$def"
then
    echo "Checking group database ..."
    gid=`eval $getpasswd | grep "^${LIBOWNER}:" | sed 's/[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/' | sed 1q`
    group=`eval $getgroup | sed -n "/[^:][^:]*:[^:][^:]*:${gid}:.*/p" | sed 's/:.*//' | sed 1q`
fi

if test -n "$group"
then
    def=`echo "$group" | sed 1q`
else
    def=`eval $getgroup | sed 1q | sed 's/:.*//'`
fi

if test "$ZC_DOING_UPGRADE" != true
then
    cat <<EOM

The account "$LIBOWNER" appears to be a member of the group "$def".
You may wish to assign the $SOFTWARE library to a different group if
access is to be restricted to a particular group of users.

EOM
fi

while :
do
    echo $n "Which group should own ${ZMLIB}? [$def] $c"
    read ans
    ans=`echo $ans`
    if test -z "$ans"
    then
	LIBGROUP=$def
    else
	echo "Checking group $ans ..."
	if (eval $getgroup | grep -s "^${ans}:") >/dev/null 2>&1
	then
	    LIBGROUP=$ans
	else
	    echo "$ans is not a known group.  Please try another."
	    continue
	fi
    fi
    break
done

if $ZC_DOING_UPGRADE
then
    def=`eval $ls $BIN/zmail 2>/dev/null | awk '{ print $3 }'`
else
    def=
fi
if test -z "$def"
then
    echo ""
    echo "Checking user database ..."
    if (eval $getpasswd | grep -s '^bin:') >/dev/null 2>&1
    then
	def=bin
    else
	def=$LIBOWNER
    fi
fi

echo ""
while :
do
    echo $n "Which account should own the $SOFTWARE executables? [$def] $c"
    read ans
    ans=`echo $ans`
    if test -z "$ans"
    then
	ZMOWNER=$def
    else
	echo "Checking on $ans ..."
	if (eval $getpasswd | grep -s "^${ans}:") >/dev/null 2>&1
	then
	    ZMOWNER=$ans
	else
	    echo "$ans is not a known user.  Please try another."
	    continue
	fi
    fi
break
done

. scripts/sgidmail.sh

echo ""
while :
do
    echo $n "Which group should own the executables? [$def] $c"
    read ans
    ans=`echo $ans`
    if test -z "$ans"
    then
	ZMGROUP=$def
    else
	echo "Checking group $ans ..."
	if (eval $getgroup | grep -s "^${ans}:") >/dev/null 2>&1
	then
	    ZMGROUP=$ans
	else
	    echo "$ans is not a known group.  Please try another."
	    continue
	fi
    fi
    break
done

if $ZC_DOING_UPGRADE
then
    . scripts/upgrade.sh
else
    . scripts/fresh.sh
fi

exit 0
