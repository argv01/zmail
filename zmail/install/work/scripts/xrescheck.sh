#! /bin/sh
#
# Figure out where X resources go, subject to later user modification
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

host=`uname`;
if [ $host = "FreeBSD" ]
then
ZAPPLRESDIR=/usr/X11R6/lib/X11/app-defaults
else
ZAPPLRESDIR=/usr/lib/X11/app-defaults
fi

export ZAPPLRESDIR

if test -n "$OPENWINHOME"
then
    ZAPPLRESDIR=$OPENWINHOME/lib/app-defaults
    if test -d /usr/lib/X11
    then
	cat <<EOM

-------------------------

You appear to have both X11 and OpenWindows installed on your machine.
EOM
	case $ZCARCHITECTURE in
	*ol)
	    cat <<EOM
This is an OpenLook version of $SOFTWARE, so I will install $SOFTWARE
resources in $ZAPPLRESDIR unless you choose otherwise by
specifying a different location.  Do this by answering "No" when asked
if you wish to install in the default locations and specifying the new
location when prompted for the location of X resources.

EOM
	    ;;
	*) cat <<EOM
$SOFTWARE will look for X resources in /usr/lib/X11/app-defaults by
default.  If you plan to install $SOFTWARE resources in the OpenWindows
tree, all users will need to set XAPPLRESDIR to point to:

	$ZAPPLRESDIR

and you will also need to specify this directory for installation of X
resources.  Do this by answering "No" when asked if you wish to install
in the default locations and then specifying the above directory when
prompted for the location of X resources.

EOM
	    ZAPPLRESDIR=/usr/lib/X11/app-defaults
	    ;;
	esac
	echo $n "Press return to continue. $c"
	read ans
    fi
    XAPPLRESDIR=$ZAPPLRESDIR
fi
if test -z "$XAPPLRESDIR"
then
    XAPPLRESDIR=/usr/lib/X11/app-defaults
fi
if test -z "$ZAPPLRESDIR"
then
    ZAPPLRESDIR=$XAPPLRESDIR
fi
