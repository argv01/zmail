:
# generate the signal names, in a slightly different format than
# signames.h.  Also, get the maximum signal number.
# (Thanks, Larry Wall.)

sighdrs=""
test -f /usr/include/signum.h && sighdrs="$sighdrs /usr/include/signum.h"
test -f /usr/include/signal.h && sighdrs="$sighdrs /usr/include/signal.h"
test -f /usr/include/sys/signal.h && sighdrs="$sighdrs /usr/include/sys/signal.h"

set X `cat $sighdrs 2>&1 | sed 's/^#[ 	][ 	]*/#/' | awk '
	$1 ~ /^#define$/ && $2 ~ /^SIG[A-Z0-9]+$/ && $3 ~ /^[1-9][0-9]*$/ {
		sig[$3] = substr($2,4,20)
		if (max < $3 && $3 < 60) {
			max = $3
		}
	}
	$1 ~ /^#define$/ && $2 ~ /^_SIG[A-Z0-9]*$/ && $3 ~ /^[1-9][0-9]*$/ {
		usig[$2] = $3
	}
	$1 ~ /^#define$/ && $2 ~ /^SIG[A-Z0-9]*$/ && $3 ~ /^_SIG[A-Z0-9]+$/ {
		sig[usig[$3]] = substr($2, 4, 20)
	}
	END {
		for (i=1; i<=max; i++) {
			if (sig[i] == "")
				printf "%d", i
			else
				printf "%s", sig[i]
			if (i < max)
				printf " "
		}
		printf "\n"
	}
' 2>/dev/null`

shift
case $# in
	0)
		if test "x$HAVE_CSH" = x1
		then
			set X `echo kill -l | csh -f 2>/dev/null`
			shift
		fi
		;;
esac

case $# in
	0)
		set HUP INT QUIT ILL TRAP IOT EMT FPE KILL BUS SEGV SYS PIPE ALRM TERM
		;;
esac

signames=''
siglist='{"SIG0", '
maxsignum=0
for sig in $*
do
	signames="${signames}\"$sig\",
"
	siglist="${siglist}\"SIG$sig\", "
	maxsignum=`expr $maxsignum + 1`
done
siglist="${siglist}(char *) 0}"
echo ...generated "$*"

rm -f sigarray.h signames.h
echo "char *sys_siglist[] = $siglist;" > sigarray.h
echo "#define MAXSIGNUM $maxsignum" > maxsig.h
echo "$signames" > signames.h
