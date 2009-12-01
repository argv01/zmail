#! /bin/sh
#
# Copyright 1996-1998 NetManage, Inc.  All rights reserved.

# Make sure sh is running properly
true || exec sh $0
export PATH || eval "echo Switching to /bin/sh ... && exec sh $0"
if test "$0" = sh -o -z "$0"
then
    echo 'Please use "sh doinstall" not "sh <doinstall".'
    exit 1
fi

if test -z "$ORIGINAL_SCRIPT_NAME"
then
    ORIGINAL_SCRIPT_NAME="$0"
    export ORIGINAL_SCRIPT_NAME
fi

. scripts/echon.sh

. scripts/install.sh
