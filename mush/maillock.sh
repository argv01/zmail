#! /bin/sh

me=`basename $0`

usage() {
    >&2 echo "usage: $me -l, --lock"
    >&2 echo "       $me -u, --unlock"
    exit 1
}

while [ $# -ne 0 ]
do
    case "$1" in
	--lock|-l)
	    test -z "$lockarg" || usage
	    lockarg="-l"
	    ;;
	--unlock|-u)
	    test -z "$lockarg" || usage
	    lockarg="-u"
	    ;;
  	-f)
	    test $# -gt 1 || usage
	    mailbox="$2"
	    shift
	    ;;
  	*)
	    usage
	    ;;
    esac
    shift
done

test ! -z "$lockarg" || usage

PATH=${PATH}:/usr/local/sbin:/usr/sbin:/sbin

if [ -z "$mailbox" ]
then
    : ${USER:=$LOGNAME}
    : ${USER:=`whoami`}

    mailbox=/var/mail/$USER
fi

exec dotlock $lockarg $mailbox

