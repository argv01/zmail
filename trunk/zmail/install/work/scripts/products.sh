#! /bin/sh
#
# Register software activation keys
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

. scripts/echon.sh

PRODUCTS="
	Z-Mail
	Z-Mail Lite
	Z-Fax
	Z-Mail Network License Server 	(ZCNLS)
	"
while test -z "$SOFTWARE"
do
    cat <<EOM

Please enter the name of the Z-Mail Software Package you are registering:
    $PRODUCTS
EOM
    echo $n "Software Package: $c"
    read SOFTWARE
    case $SOFTWARE in
	*[Ll]ite) SOFTWARE="Z-Mail Lite";;
	*[Mm]ail) SOFTWARE="Z-Mail";;
	*[Ff]ax)  SOFTWARE="Z-Fax";;
	*[Nn]et*) SOFTWARE="ZCNLS";;
	ZCNLS)    ;;
	*)        SOFTWARE="";;
    esac
done

if test -z "$LICENSE_FILE"
then
    LICENSE_FILE=license.data
fi
if test -z "$LICENSE_DIR"
then
    if test -f .register.env
    then
	. .register.env
    elif test -f $ZMLIB/.register.env
    then
	. $ZMLIB/.register.env
    elif test -d /usr/lib/Zcode/license
    then
	LICENSE_DIR=/usr/lib/Zcode/license;
    else
	case $SOFTWARE in
	*Lite) LICENSE_DIR=/usr/lib/Zmail/license;;
	*Mail) LICENSE_DIR=/usr/lib/Zmail/license;;
	*Fax)  LICENSE_DIR=/usr/lib/Zfax/license;;
	ZCNLS) LICENSE_DIR=/usr/lib/Zcode/license;;
	esac
    fi
fi

if test -n "$BIN"
then
    DBIN="$BIN/"
else
    DBIN=""
fi

# This register stuff isn't quite right for ZCNLS, but ...
REGISTER=$LICENSE_DIR/bin/register
case $SOFTWARE in
*Lite)	USER_READABLE_SOFTNAME="Z-Mail Lite";;
*Mail)	USER_READABLE_SOFTNAME="Z-Mail";;
*Fax)	USER_READABLE_SOFTNAME="Z-Fax";;
ZCNLS)	USER_READABLE_SOFTNAME="Network License Server";;
esac
case $SOFTWARE in
*Lite)	SOFTNAME="Z-Mail";	BINARY=${DBIN}zmlite;;
*Mail)	SOFTNAME="Z-Mail";	BINARY=${DBIN}zmail;;
*Fax)	SOFTNAME="Z-Fax";	BINARY=${DBIN}zfax;;
ZCNLS)	SOFTNAME="ZCNLSD";	BINARY="$REGISTER";;
esac

if test -z "$DBIN" -a "$BINARY" != "$REGISTER"
then
    for DBIN in `echo $PATH | sed 's/:/ /g'`
    do
	if test -f $DBIN/$BINARY
	then
	    BINARY=$DBIN/$BINARY
	    break
	fi
    done
fi
export SOFTWARE SOFTNAME BINARY
