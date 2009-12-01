#! /bin/sh
#
# Figure out the hostname of the current machine, export it as HOSTNAME.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

if test -z "$HOSTNAME"
then
    hostname=`( hostname || uname -n || uuname -l ) 2>/dev/null`
    if test -z "$hostname"
    then
	echo ""
	echo "I can't seem to figure out this machine's hostname ..."
	hostname="unknown"
    fi
    HOSTNAME=$hostname
    export HOSTNAME
fi
