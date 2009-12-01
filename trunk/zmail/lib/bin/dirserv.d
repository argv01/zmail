#! /bin/sh -
#
# dirserv.d  - Z-Mail client directory service support script
# Copyright (c) 1993-94 Z-Code Software, a Division of NCD.
#
# $RCSfile: dirserv.d,v $
# $Revision: 2.6 $
# $Date: 1995/05/10 01:16:36 $
# $Author: liblit $
#
# This script listens on a socket for what is presumed to be a connection
# from a client Z-Mail requesting a directory service lookup.
# It runs a directory service program specified as its first argument and
# prints back to the socket first the exit status and then the output of
# the directory service program.
#
# The directory service program is assumed to act exactly as described in
# the UNIX Z-Mail on-line help for the "address_book" variable.
#
# Setting up the socket connections is assumed to be handled by inetd.
# Modify your inetd.conf and /etc/services files to cause this script
# to be started when a connection arrives on port 15213.  Here's a
# sample inetd.conf entry:
#
#      zmailds  stream  tcp  nowait  guest  /usr/local/etc/dirserv.d
#
# And here's a sample /etc/services entry:
#
#      zmailds  15213/tcp   zmaildsd  # Z-Mail directory service
#####################################################################
#
# Default service -- edit as necessary.
SERVICE="${1-/usr/lib/Zmail/macstuff/lookup.pw}"
CR=`echo | tr '\012' '\015'`
#
OUTPUT="${TMPDIR-/tmp}/mds$$"
# Obtain search probe from client
read lookupmax pattern
# Strip trailing CR-NL if any
pattern="`echo \"$pattern\" | tr -d '\015\012'`"
"$SERVICE" "$lookupmax" "$pattern" > "$OUTPUT" 2>&1
# Send out a CR-NL on each line
(echo $? ; cat "$OUTPUT") | sed 's/$/'"$CR"/
rm -f "$OUTPUT"
