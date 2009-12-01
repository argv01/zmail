#! /bin/sh
#
# Figure out how to do echo without newline
#
# This is intended to be sourced with the "." command from another script.

if test -z "$ECHON"
then
    c=`echo "hi there\c" | sed 's/[^c]//g'`
    if test -z "$c"
    then
	n=''
	c='\c'
    else
	n='-n'
	c=''
    fi
    ECHON=true
fi
