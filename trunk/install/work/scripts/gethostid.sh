#! /bin/sh
#
# Get the hostid of the current machine (or 0 if none).
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

UNAME=`(uname -a) 2>/dev/null`
case "$UNAME" in
AIX*) hostid=`uname -m`;;
IRIX*\ 5.*)
    hostid=`/sbin/sysinfo | tail +2 | sed -e 's/ *//g' \
					  -e 's/00000000$//' \
					  -e 's/00000000$//' \
					  -e 's/00000000$//' \
					  -e 's/00000000$//'`;;
*) hostid=`(./hostid || /usr/ucb/hostid || hostid) 2>/dev/null || echo 0`;;
esac
HOSTID=$hostid
export HOSTID
