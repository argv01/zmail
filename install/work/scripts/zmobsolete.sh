#! /bin/sh
#
# Move aside obsolete files from an existing installation.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

if test -z "$ZMLIB"
then
    echo 1>&2 "I do not know where your Z-Mail Library is (ZMLIB not set)."
    exit 1
fi
if test -z "$HOW_TO_MOVE"
then
    HOW_TO_MOVE='/bin/rm -f $file_to_write'
fi
while read file_to_write
do
    test -f $file_to_write && eval "$HOW_TO_MOVE"
done <<EOF
$ZMLIB/COPYRIGHTS
$ZMLIB/InstallNLS.txt
$ZMLIB/RELNOTES
$ZMLIB/RELNOTES.ps
$ZMLIB/SLA
$ZMLIB/emacs.zmailrc
$ZMLIB/locking.zmailrc
$ZMLIB/sample.lib
$ZMLIB/samples/zscript/usedraft.zsc
EOF
for file_to_write in $ZMLIB/man/*.[14]
do
    test -f "$file_to_write" && eval "$HOW_TO_MOVE"
done
