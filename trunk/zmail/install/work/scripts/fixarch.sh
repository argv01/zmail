#! /bin/sh
#
# Make architecture/operating-system dependend sanity checks.
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1993-94 Z-Code Software, a Division of NCD.  All rights reserved.

while test -z "$SANITY_OK"
do
    SANITY_OK=$$
    export SANITY_OK

    REAL_SH=sh
    export REAL_SH

    PATH=/etc:/usr/etc:"$PATH"

    case $ZCARCHITECTURE in
    apollo)
	;;
    atari)
	;;
    oli*)
	;;
    osf1*)
	;;
    hp*)
	;;
    dgux|dgintel)
	;;
    m88k)
	;;
    mips)
	;;
    *[34]86*|inter)
	;;
    darwin*)
	;;
    freebsd*)
	;;
    linux*)
	;;
    pyr*)
	# OSX has no -x in test ...
	if test -f /.attbin/sh
	then
	    REAL_SH=/.attbin/sh
	    echo ""
	    echo Your ucb sh may be very old and braindamaged.
	    echo Executing the att sh just in case ...
	    exec /.attbin/sh $0
	fi
	;;
    rs6000|aix*)
	USRLOCAL=/usr/lpp
	USRLIB=/usr/lpp
	;;
    sco*)
	;;
    seq*)
	;;
    sgi*|irix*)
	;;
    sun4ol)
	;;
    sun*)
	;;
    sol*)
	;;
    sony)
	case `strings /bin/sh | grep '^DATE'` in
	*1978*)
	    echo ""
	    echo Your machine does not have a shell that can run this script.
	    echo Please complain to Sony after installing $SOFTWARE by hand. 
	    echo See instructions in your $SOFTWARE Installation Guide.
	    echo '(No, there are not instructions for how to complain.)'
	    exit 1
	    ;;
	esac
	;;
    ultrix)
	if test -f /bin/sh5
	then
	    REAL_SH=/bin/sh5
	    echo ""
	    echo Your /bin/sh may be very old and braindamaged.
	    echo Executing /bin/sh5 just in case ...
	    exec /bin/sh5 $0
	fi
	;;
    unixw)
	  ;;
    *)
	echo ""
	echo "I can not determine the architecture and operating system"
	echo "that are required by this version of $SOFTWARE.  Please enter"
	echo "one of the following keywords to identify the architecture"
	echo "or operating system of your machine:"
	echo ""
	echo "aix        dgux        hp          irix        solaris"
	echo "sunos      hp68k       hpPA10      hpPA11      dgux"
	echo "inter      irix5       linux       m88k        mips"
	echo "ncr486     osf1        pyrosx      pyrsvr4     rs6000"
	echo "sco        seqptx      seqptx21    sgi         solar22"
	echo "solar23    solar24x86  sunos411    sunos413    ultrix"
	echo "unixw      darwin      freebsd"
	echo ""
	echo $n "Architecture or operating system: $c"
	read ans
	ZCARCHITECTURE=`echo $ans | tr '[A-Z]' '[a-z]'`
	SANITY_OK=
    esac
done

BIN=$USRLOCAL/bin
