:

set -e

mapping=`sed -n 's/^\#define[ 	][ 	]*\(CAT_[^ 	][^ 	]*\)[ 	][ 	]*\([^ 	].*\)$/-e s:\1:\2:/p' $SRCDIR/include/catalog.h`
sed $mapping $@
