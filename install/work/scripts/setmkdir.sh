#! /bin/sh
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.
#
# This is intended to be sourced with the "." command from another script.
#
# Determine the best way to make new directory trees.  Order of preference:
#
#   1. mkdir -p               this will be the cleanst solution if it exists
#   2. scripts/mkdirhier.sh   fairly reliable but not 100% bulletproof
#   3. mkdir                  requires parent directory to exist.
#

# first set a fallback
mkdir=mkdir

# now let's see how we can make directory hierarchies...
mkdir -p /tmp/z$$z/z$$z 2>/dev/null
if test -d ./-p
then
    if test -r scripts/mkdirhier.sh
    then
        mkdir='/bin/sh scripts/mkdirhier.sh'
    else
        mkdir=mkdir
    fi
    rmdir ./-p
elif test -d /tmp/z$$z/z$$z
then
    mkdir='mkdir -p'
else
    if test -r scripts/mkdirhier.sh
    then
        mkdir='/bin/sh scripts/mkdirhier.sh'
    else
        mkdir=mkdir
    fi
fi
/bin/rm -rf /tmp/z$$z
