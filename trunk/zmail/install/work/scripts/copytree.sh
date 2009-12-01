#! /bin/sh
#
# Copy one directory tree to another, possibly saving the target on the way
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

#
# The caller must define variables that tell what to copy:
#	$TREE_TO_READ	Top directory of tree to be copied
#	$TREE_TO_WRITE	Top directory of destination tree
#
# The caller may define variable that are /bin/sh commands:
#	$HOW_TO_SAVE	What to do if a destination file exists
#	$HOW_TO_COPY	How to perform the copy
#
# The caller may define variables that trace the procedure:
#	$COPY_TRACE	File name in which to record trace
#	$CLEAN_TRACE	File name in which to record removal commands
#
# The HOW variables should refer to $file_to_read and $file_to_write;
# they are processed via the "eval" shell construct to expand these.
#
# If HOW_TO_SAVE is not defined, the destination is probably overwritten.
# HOW_TO_COPY defaults to '/bin/cp $file_to_read $file_to_write'
#
# The caller may define ECHO_DOTS=true to cause a "." to be output to
# standard error as each file is copied.
#

# Sanity

if test -z "$TREE_TO_READ"
then
    echo "${0}: No TREE_TO_READ" 1>&2
    exit 1
fi
if test -z "$TREE_TO_WRITE"
then
    echo "${0}: No TREE_TO_WRITE" 1>&2
    exit 1
fi

# Initialize

TREE_TO_READ=`echo $TREE_TO_READ | sed 's@/*$@@'`
TREE_TO_WRITE=`echo $TREE_TO_WRITE | sed 's@/*$@@'`
if test -z "$HOW_TO_COPY"
then
    HOW_TO_COPY='/bin/cp $file_to_read $file_to_write'
fi

# More sanity

if test ! -d "$TREE_TO_READ"
then
    echo "${0}: $TREE_TO_READ is not a directory" 1>&2
    exit 1
fi
if test ! -d "$TREE_TO_WRITE"
then
    if mkdir $TREE_TO_WRITE
    then
	echo Created $TREE_TO_WRITE/ ... 1>&2
    else
	echo "${0}: Cannot create $TREE_TO_WRITE" 1>&2
	exit 1
    fi
fi
if test -n "$COPY_TRACE"
then
    if echo "echo Copying $TREE_TO_READ to $TREE_TO_WRITE" >> "$COPY_TRACE"
    then
	echo "${0}: Recording copy in $COPY_TRACE ..." 1>&2
    else
	echo "${0}: Cannot write trace file $COPY_TRACE" 1>&2
	exit 1
    fi
fi
if test -n "$CLEAN_TRACE"
then
    if echo "echo Removing $TREE_TO_WRITE" >> "$CLEAN_TRACE"
    then
	echo "${0}: Creating cleanup file $CLEAN_TRACE ..." 1>&2
    else
	echo "${0}: Cannot write cleanup file $CLEAN_TRACE" 1>&2
	exit 1
    fi
fi

# Check for output of a stream of dots

if test "$ECHO_DOTS" = true
then
    # Figure out how to do echo without newline (from echon.sh)

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
    ECHO_DOTS='echo $n ".$c" 1>&2'
    ECHO_LINE='echo "" 1>&2'
else
    ECHO_DOTS=':'
    ECHO_LINE=':'
fi

# Copy the tree

find $TREE_TO_READ -print | \
sed  -e 's@\(.*\)@\1 \1@' -e "s@$TREE_TO_READ/*@@" | \
while read file_to_write file_to_read
do
    if test -z "$file_to_read"
    then
	continue
    fi

    file_to_write=$TREE_TO_WRITE/$file_to_write
    if test -d $file_to_read
    then
	if test ! -d $file_to_write
	then
	    eval "$ECHO_LINE"
	    if mkdir $file_to_write
	    then
		echo $n "Created $file_to_write/ ...$c" 1>&2
	    else
		break
	    fi
	fi
    else
	if test -f $file_to_write
	then
	    if test -n "$HOW_TO_SAVE"
	    then
		if eval "$HOW_TO_SAVE" && test -n "$COPY_TRACE"
		then
		    echo "$HOW_TO_SAVE" >> "$COPY_TRACE"
		fi
	    fi
	fi
	if eval "$HOW_TO_COPY" && test -n "$COPY_TRACE"
	then
	    echo "$HOW_TO_COPY" >> "$COPY_TRACE"
	fi
	if test -n "$CLEAN_TRACE"
	then
	    echo "rm $file_to_write" >> "$CLEAN_TRACE"
	fi
	eval "$ECHO_DOTS"
    fi
done
