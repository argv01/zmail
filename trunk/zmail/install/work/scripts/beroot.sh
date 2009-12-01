#! /bin/sh
#
# Make sure that we are running as the superuser.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

case $ROOTPID in
$$)
    echo ""
    echo "Stepping quickly into a nearby phone booth, I emerge as SuperUser!"
    echo "I am now acting as root."
    ;;
*)
    ROOTPID=$$
    export ROOTPID

    cat <<EOM

-------------------------

I must be root to perform this operation.  If you are already root,
the script will proceed.  Otherwise, you will need to enter your
system's root password here.

EOM
    cat <<EOM > /tmp/rm-me$$
This is a test to see if I am already running as root.
EOM
    if (chmod 0 /tmp/rm-me$$ && cat /tmp/rm-me$$) >/dev/null 2>&1
    then
	rm -f /tmp/rm-me$$
	echo "Thank you!  I'm already root."
    else
	rm -f /tmp/rm-me$$
	if test -z "$ZCINSTALL_DO_NOT_SU"
	then
	    exec su root -c "exec sh $0" || exit 1
	else
	    echo NOT SU-ING TO ROOT
	fi
    fi
    ;;
esac
