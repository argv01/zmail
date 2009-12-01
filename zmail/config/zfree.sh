#! /bin/sh
{
echo "char *def_startup[] = {"
sed	-e 's/^[ 	]*//' \
	-e '/^source $ZMLIB\/locale.zmailrc/r lib/locale.zmailrc' \
	-e '/^source $ZMLIB\/locale.zmailrc/d' \
	-e '/^source $ZMLIB\/zmail.menus/r lib/zmail.menus' \
	-e '/^source $ZMLIB\/zmail.menus/d' \
    < lib/system.zmailrc | \
sed	-e 's/^[ 	]*//' \
	-e '/^#/d' \
	-e '/^$/d' \
	-e 's/\(\\\)/\\\1/g' \
	-e 's/"/\\"/g' \
	-e 's/^/"/' \
	-e 's/$/",/'
echo "NULL"
echo "};"
} > shell/zstartup.h
