#!/bin/sh

# master_user_file format (sort -f on first field):
#  userid:fullname:nickname:host:phone
#
# msg_groups_file format (sort -f on first field):
#  group:descr:member, member, [...], member

rm -f /tmp/zmcl.$$

if test "$1" -eq -1
then
    case "$COT_SEARCH" in

        fullname)
	    # case-insensitive grep on prefix of fullname field
	    # of master_user_file.  List ID/fullname pairs

	    awk -F: '{print $2, ":", $1}' $MASTER_USER_FILE | \
	        grep -i "^$2" | \
		awk -F: '{print $1, "::", $2}' > /tmp/zmcl.$$
	    # Now fall through to end of script
	    ;;

	userid)
	    # case-insensitive grep on ID field of master_user_file.
	    # If a unique ID is found, output the ID, host, and
	    # phone number on the first line.  On subsequent lines,
	    # output lines from msg_groups_file where the members field
	    # contains an exact (case-insensitive) match for the ID.

	    grep -i "^[^:]*$2" "$MASTER_USER_FILE" | tee /tmp/zmcl.$$ | \
		grep -i "^$2:" > /tmp/zmcl2.$$
	    if test -s /tmp/zmcl2.$$
	    then
		mv /tmp/zmcl2.$$ /tmp/zmcl.$$
	    else
		rm /tmp/zmcl2.$$
	    fi
	    case `wc -l /tmp/zmcl.$$ | awk '{print $1}'` in
	        0)
		    echo "$2"
		    rm -f /tmp/zmcl.$$
		    exit 4
		    ;;
		1)
		    id=`sed 's/:.*//' /tmp/zmcl.$$`
		    cat /tmp/zmcl.$$
		    grep -i "^[^:]*:[^:]*:.*$id" "$MSG_GROUPS_FILE" | \
			sed "s/^\([^:]*\):.*/$id::\1/"
		    rm -f /tmp/zmcl.$$
		    exit 0
		    ;;
		*)
		    echo More than one ID matches
		    rm -f /tmp/zmcl.$$
		    exit 2
		    ;;
	    esac
	    ;;

	group)
	    # case-insensitive grep on prefix of group field of
	    # msg_groups_file.  If unique match is found, list members
	    grep -i "^$2" "$MSG_GROUPS_FILE" | tee /tmp/zmcl.$$ | \
	        grep -i "^$2:" > /tmp/zmcl2.$$
	    if test -s /tmp/zmcl2.$$
	    then
	        mv /tmp/zmcl2.$$ /tmp/zmcl.$$
	    else
	        rm /tmp/zmcl2.$$
	    fi
	    case `wc -l /tmp/zmcl.$$ | awk '{print $1}'` in
	        0)
		    echo "$2"
		    rm -f /tmp/zmcl.$$
		    exit 4
		    ;;
		1)
		    group=`awk -F: '{print $2}' /tmp/zmcl.$$`
		    awk -F: '{print $3}' /tmp/zmcl.$$ | \
			sed 's/ *, */:/g' | \
			tr : '\012' | \
			sed "s:^:$group\\:\\::"
		    rm -f /tmp/zmcl.$$
		    exit 0
		    ;;
		*)
		    echo More than one ID matches
		    rm -f /tmp/zmcl.$$
		    exit 2
		    ;;
	    esac
	    ;;
    esac
else
    # Union of case-insensitive prefix searches on master_user_file and
    # msg_groups_file.
    $ZMLIB/bin/cotlookup "$MASTER_USER_FILE" "$2" | \
        sed 's/^\([^:]*\):\(.*\)/\2::\1/' > /tmp/zmcl.$$
    $ZMLIB/bin/cotlookup "$MSG_GROUPS_FILE" "$2" | \
        sed 's/^\([^:]*\):\(.*\)/\2::\1/' >> /tmp/zmcl.$$
    grep "::$2"'$' /tmp/zmcl.$$ > /tmp/zmcl2.$$
    if test -s /tmp/zmcl2.$$
    then
	mv /tmp/zmcl2.$$ /tmp/zmcl.$$
    else
	rm /tmp/zmcl2.$$
    fi
    # Now fall through to end of script
fi

l=`wc -l /tmp/zmcl.$$ | awk '{print $1}'`
case $l in
    0)
	echo "$2"
	rm -f /tmp/zmcl.$$
	exit 4
	;;
    1)
	cat /tmp/zmcl.$$
	rm -f /tmp/zmcl.$$
	exit 5
	;;
    *)
        if test "$1" -gt 1
	then
	    if test "$l" -gt "$1"
	    then
	        echo Too many matches
	        exit 2
	    fi
	fi
	cat /tmp/zmcl.$$
	rm -f /tmp/zmcl.$$
	exit 0
	;;
esac
