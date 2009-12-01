#! /bin/sh
#
# Get the version number of the software to be installed.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

if test -n "$USE_BINARY"
then
    VERSION=`$USE_BINARY -version 2>/dev/null | sed 1q`
elif test -n "$BINARY"
then
    VERSION=`$BINARY -version 2>/dev/null | sed 1q`
fi

if test -z "$VERSION"
then
    if test -z "$ZCAPPDEF" -a -n "$SOFTWARE"
    then
	C=`echo "$SOFTWARE" | sed 's/\(.\).*/\1/'`
	R=`echo "$SOFTWARE" | sed -e 's/[^A-Za-z]//g' -e 's/.\(.*\)/\1/'`
	ZCAPPDEF=$C`echo "$R" | tr '[A-Z]' '[a-z]'`
    fi
    if test -n "$ZCAPPDEF" -a -f lib/$ZCAPPDEF
    then
	VERSION=`grep $ZCAPPDEF.version lib/$ZCAPPDEF | sed 's/.*: *//'`
    fi
    if test -z "$VERSION" -a -f lib/variables
    then
	VERSION=`grep \^Version lib/variables | sed -n '$p' | sed 's/.* //'`
    fi
    VERSION=`echo "$VERSION" | sed -e 's/\([0-9]*\.[0-9A-Za-z]*\)\..*/\1/'`
fi
VERSION=`echo "$VERSION" | sed -e 's/[^0-9]*\([0-9]*\.[0-9A-Za-z]*.*\)/\1/'`
VERSION=`echo "$VERSION" | sed -e 's/\([0-9]*\.[0-9A-Za-z]*\)\..*/\1/'`
test -n "$VERSION" && export VERSION
