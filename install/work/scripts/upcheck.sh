#! /bin/sh
#
# Check whether this is an upgrade or a new installation.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

while test -f scripts/upgrade.sh -a -z "$ZC_DOING_UPGRADE"
do
    cat <<EOM

Please enter a number corresponding to your choice.  Is this installation:

    1) A new installation of $SOFTWARE
    2) An upgrade to an existing copy of $SOFTWARE

EOM
    echo $n "? $c"
    read ans
    ans=`echo $ans`
    case $ans in
    1) ZC_DOING_UPGRADE=false;;
    2) ZC_DOING_UPGRADE=true;;
    esac
done
