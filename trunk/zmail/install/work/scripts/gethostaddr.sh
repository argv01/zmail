#! /bin/sh
#
# Figure out the network address of the current machine, export HOSTADDR.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

# First try "arp", then try grepping the hosts database.
hostaddr=""
if test "$hostname" != "unknown"
then
    hostaddr=`( arp $hostname | sed -e "s/).*//" -e "s/.*(//" ) 2>/dev/null`
    if test -z "$hostaddr"
    then
	hostaddr=`( ( ypcat hosts || cat /etc/hosts ) |
			egrep "^[^#].*[ 	]$hostname([ 	]|"'$)' |
			sed "s/[ 	].*//") 2>/dev/null`
    fi
fi
if test -z "$hostaddr"
then
    echo ""
    echo "I can't seem to figure out this machine's network address ..."
    hostaddr="unknown"
fi
HOSTADDR=$hostaddr
export HOSTADDR
