#! /bin/sh
#
# ensure existence of nls directory if necessary
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1995 Z-Code Software, a Division of NCD.  All rights reserved.

grep "^$ZCARCHITECTURE\$" >/dev/null 2>&1 <<EOM
apollo
dgux
m88k
mR40
ncr486
osf1
osf1v30
pyrsvr4
irix51
irix52
irix53
solar23
solar24
solar24x86
solar25x86
solar25
sunos413
ultrix
unixw
EOM

if [ $? = 0 ]
then
    cat <<EOM


There is a known interaction between X11R5 clients and X11R4 display
servers which causes a crash when the user pastes text into the client;
this is because the /usr/lib/X11/nls directory, which is part of the
X11R5 distribution, does not usually exist on an X11R4 machine.  If the
nls directory is in a non-standard location, the user must set the
environment variable XNLSPATH to point to this directory.

EOM
    if test -d /usr/lib/X11/nls
    then
        cat <<EOM

You appear to have a /usr/lib/X11/nls directory on this machine, and do
not need to perform any special actions.  However, you may choose to
make an nls directory available for users who might run $SOFTWARE on an
X11R4 display server.
EOM
    else
        cat <<EOM

You do NOT have a /usr/lib/X11/nls directory on this machine.  Without
an nls directory, it is highly likely that $SOFTWARE will crash if a user
attempts to do any cut and paste operations.
EOM
    fi
    cat <<EOM

An nls directory has been included in this distribution and will be
installed in the directory \$ZMLIB/nls.  If desired, the $SOFTWARE
startup sequence can be modified to append \$ZMLIB/nls to the XNLSPATH
environment variable to make these files available to the user.

EOM
    def=Yes
    echo $n "Do you want to modify the $SOFTWARE startup sequence? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" != "n"
    then
        echo $n "Creating $ZMLIB/custom.zmailrc... $c"
        cat <<EOM >>$ZMLIB/custom.zmailrc
if \$?XNLSPATH
  setenv XNLSPATH "\${XNLSPATH}:\$ZMLIB/nls"
else
  setenv XNLSPATH "\$ZMLIB/nls"
endif
EOM
        echo "done."
    fi
fi
