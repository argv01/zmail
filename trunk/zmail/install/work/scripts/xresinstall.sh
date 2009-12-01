#! /bin/sh
#
# Install X resources in the appropriate place (see xrescheck.sh).
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

# Check for existence of XAPPLRESDIR
if test -n "$XAPPLRESDIR"
then
    echo Installing X Resources in $XAPPLRESDIR...

    XAPPLRESPARENT=`echo $XAPPLRESDIR | sed 's:/[^/][^/]*/*$::'`
    if test ! -d "$XAPPLRESPARENT"
    then
        cat <<EOM
Parent directory does not exist: $XAPPLRESPARENT

EOM
        def=Yes
        echo $n "Create all directories up to $XAPPLRESDIR? [$def] $c"
        read ans
        ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
        if test "$ans" != "n"
        then
            $mkdir $XAPPLRESDIR
	else
	    echo ""
	    XAPPLRESDIR=""
        fi
    elif test ! -d "$XAPPLRESDIR"
    then
	# Note!  Explicitly NOT $mkdir here!
	mkdir $XAPPLRESDIR || XAPPLRESDIR=""
    fi
fi
if test -n "$XAPPLRESDIR"
then
    if cp lib/$ZCAPPDEF $XAPPLRESDIR/$ZCAPPDEF
    then
	chown $LIBOWNER $XAPPLRESDIR/$ZCAPPDEF || WARNING=true
	chgrp $LIBGROUP $XAPPLRESDIR/$ZCAPPDEF || WARNING=true
	chmod 644 $XAPPLRESDIR/$ZCAPPDEF || WARNING=true
    else
	WARNING=true
	if test -f $XAPPLRESDIR/$ZCAPPDEF
	then
	    ERROR=true
	fi
    fi
    if test -d lib/schemes
    then
	TREE_TO_READ=lib/schemes
	TREE_TO_WRITE=/usr/lib/X11/schemes/Base
	echo Installing new Schemes in $TREE_TO_WRITE ...
	. scripts/copytree.sh
	unset TREE_TO_WRITE
	unset TREE_TO_READ
    fi
else
    echo "I can't seem to find a place to install X resources."
    echo "Check that X11 is properly installed on your system."
    WARNING=true
fi

. scripts/xnlspath.sh
