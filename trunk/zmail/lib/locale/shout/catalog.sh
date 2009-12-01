set -exv

mapping=`sed -n 's/^\#define[ 	][ 	]*\(CAT_[^ 	][^ 	]*\)[ 	][ 	]*\([^ 	].*\)$/-e s:\1:\2:/p' $SRCDIR/include/catalog.h`

sed $mapping $@ | ./shoutify > /tmp/cat$$
gencat -m zmail /tmp/cat$$

mv /tmp/cat$$ cat-src
#rm -f /tmp/cat$$
