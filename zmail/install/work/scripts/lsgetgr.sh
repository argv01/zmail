#! /bin/sh
#
# Figure out how to get ls to report the group name in column 4
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All Rights Reserved.

if test -z "$LSGETGR"
then
    # Figure out how to get the group name from ls
    ls='ls -ld'
    if test `eval $ls . | awk '{ print NF }'` -eq 8
    then
	ls='ls -ldg'
    fi
    LSGETGR=true
fi
