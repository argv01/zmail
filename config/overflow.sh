#!/bin/sh

# Certain implementations of the Borne shell have a 1K limit on the
# length of the target word for case statements.  That presents a
# problem for our configure script, which easily exceeds this limit
# while accumulating $DEFS.
#
# This script attempts to hit this buffer limit in whatever shell
# interprets it.  The script exits zero (success) only if the shell
# seems to be ok.  Configure uses this as a utility, and tries to
# switch to a better shell if the current one seems deficient.

  
long=x

for loop in 0 1 2 3 4 5 6 7 8 9 0; do
  long="$long$long"
done

case "$long" in
  *x*) exit 0;;
  *)   exit 1;;
esac
