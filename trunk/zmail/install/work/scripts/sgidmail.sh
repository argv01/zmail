#! /bin/sh
#
# Determine if a mail program needs to be installed set-group-id
#
# This is intended to be sourced with the "." command from another script.
#
# Copyright 1992-94 Z-Code Software, a Division of NCD.  All rights reserved.

if test -z "$SOFTWARE"
then
    SOFTWARE=Z-Mail	# So our output text looks sensible
fi

group=""	# Tested below

if test -n "$SGID"
then
    def=$SGID
    SGID=true
else
    echo ""
    echo "Examining your mail delivery area ..."

    . scripts/lsgetgr.sh

    mail='/var/mail /usr/mail /var/spool/mail /usr/spool/mail'
    perm=`eval $ls $mail 2>/dev/null | sed -n '/^d/p' | sed 1q`
    if test -n "$perm"
    then
	mail=`echo "$perm" | awk '{ print $9 }'`
	group=`echo "$perm" | awk '{ print $4 }'`
	perm=`echo "$perm" | awk '{ print $1 }'`
    else
	perm=`eval $ls $mail 2>/dev/null | sed 1q`
	if test -n "$perm"
	then
	    mail=`echo "$perm" | awk '{ print $9 }'`
	    group=`echo "$perm" | awk '{ print $4 }'`
	    perm=`echo "$perm" | awk '{ print $1 }'`
	fi
    fi
    case $perm in
    l*)
	ls="${ls}L"
	perm=`eval $ls $mail 2>/dev/null | sed -n '/^d/p' | sed 1q`
	if test -n "$perm"
	then
	    mail=`echo "$perm" | awk '{ print $9 }'`
	    group=`echo "$perm" | awk '{ print $4 }'`
	    perm=`echo "$perm" | awk '{ print $1 }'`
	fi
	;;
    esac
    cat <<EOM

-------------------------

EOM
    case $perm in
    d???????w?) cat <<EOM
Your $mail is writable by any user.

This probably means that your mail delivery agent uses lock files
in this system delivery area to synchronize file access with mail
user agents like $SOFTWARE.
EOM
	case $perm in
	*t) cat <<EOM

This is fine.
EOM
	    ;;
	*) cat <<EOM

However, permissions allow other users to remove mail files from
this directory.  This is a security problem.  See your operating
system documentation for an appropriate remedy; one possible
solution may be the command:

	chmod 3777 $mail
EOM
	    ;;
	esac
	SGID=false
	;;
    d????w????) cat <<EOM
Your $mail is group-writable by the group "$group".

This probably means that your mail delivery agent uses lock files
in this system delivery area to synchronize file access with mail
user agents like $SOFTWARE.

This is fine.
EOM
	SGID=""
	;;
    d?????????) cat <<EOM
Your $mail is writable only by its owner.

This may be a problem if you are using NFS for mail delivery.
$SOFTWARE will not be able to create lock files in $mail
for secure mailbox access.  Even if you are not using NFS, there
may be problems if mailbox files in $mail are removed.

You should NOT use NFS for mail delivery if you are concerned about
loss of mail.  One possible solution is to make $mail
group-writable and arrange for your mail delivery agent to create
lock files during mail delivery.  Check the documentation for your
mail delivery agent to determine whether this is possible.

EOM
	echo $n "Press return to continue. $c"
	read ans
	SGID=false
	;;
    *) cat <<EOM
I cannot locate your mail delivery directory.  $SOFTWARE users may
need to set the environment variable MAIL to the name of the file
where their mail is delivered by the local delivery agent.

You can also set the environment variable MAIL in the $SOFTWARE system
configuration file $ZMLIB/system.zmailrc.
EOM
	echo $n "Press return to continue. $c"
	read ans
	SGID=""
	;;
    esac
fi

if test -z "$SGID"
then
    cat <<EOM

If your mail delivery agent uses lock files to synchronize mail
delivery you will need to give $SOFTWARE permission to write lock files
into $mail.

This is normally done by putting the $SOFTWARE executables in group
"mail" (or whatever group owns $mail) and setting the
modes of the executables for "set-group-id-upon-execution".

If you plan to use NFS-mounted spool directories, the use of lock
files is recommended.  Check the documentation for your delivery
agent to see if it can be configured to use file locking.

EOM
    def=Yes
    echo $n "Should $SOFTWARE be installed set-group-id? [$def] $c"
    read ans
    ans=`echo $ans $def | sed 's/\(.\).*/\1/' | tr YN yn`
    if test "$ans" != "n"
    then
	SGID=true
	if test -z "$group"
	then
	    group=`eval $getgroup | grep mail`
	fi
    else
	SGID=false
	gid=`eval $getpasswd | grep "^$ZMOWNER:" | \
		sed 's/[^:]*:[^:]*:[^:]*:\([^:]*\):.*/\1/'`
	group=`eval $getgroup | sed -n "/[^:][^:]*:[^:][^:]*:$gid:.*/p" | \
		sed 's/:.*//'`
    fi
    if test -z "$group"
    then
	group=`eval $ls $mail 2>/dev/null | sed 1q | awk '{ print $4 }'`
	if test -z "$group"
	then
	    def=$LIBGROUP
	else
	    def=$group
	fi
    else
	def=mail
    fi
else
    cat <<EOM

It does not appear that you need to install $SOFTWARE with any special
permissions.  However, if your mail delivery agent uses lock files to
synchronize mail delivery, you may need to modify the $SOFTWARE system
configuration file $ZMLIB/system.zmailrc to include the line:

	set dot_lock

-------------------------

EOM
fi
