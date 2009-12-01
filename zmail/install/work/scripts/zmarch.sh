#! /bin/sh
#
# Set up Z-Mail install environment based on architecture/operating-system.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

ZCAPPDEF=Zmail
export ZCAPPDEF

while :
do
    case $ZCARCHITECTURE in
    apollo)
	SGID=mail
	;;
    atari)
	SGID=mail
	;;
    hp*)
	SGID=mail
	;;
    dgux|dgintel)
	;;
    linux*)
	;;
    m88k)
	;;
    mips)
	;;
    ncr486|i386|inter)
	SGID=mail
	;;
    oli*)
	;;
    osf1*)
	;;
    pyr*)
	;;
    rs6000|aix*)
	;;
    seq*)
	;;
    sco*)
	;;
    sgi*|irix*)
	;;
    sun4ol|sol2*ol)
	ZCAPPDEF=Zmail_ol
	;;
    sun*)
	;;
    sol*)
	;;
    sony)
	;;
    ultrix)
	;;
    unixw)
	;;
    *)
	. scripts/fixarch.sh
	if test -z "$SANITY_OK"
	then
	    continue
	fi
    esac
    break
done

if test -z "$ZMLIB"
then
    case $SOFTWARE in
    *Lite) ZMLIB=$USRLIB/Zmlite;;
    *)     ZMLIB=$USRLIB/Zmail;;
    esac
else
    ZMLIB_FROM_ENV=$ZMLIB
fi
export ZMLIB
