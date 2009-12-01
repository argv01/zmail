#! /bin/sh
#
# This script expects its arguments to be directory pathnames.  For each
# argument, it creates all the directories leading up to the target
# directory before creating the directory itself.
#
# adapted from IRIX4 X11R4 /usr/bin/X11/mkdirhier
#
# This probably will not do the right thing if the directory name has
# space characters in it, unless they're well-escaped.  Hopefully we'll
# never need to find out.

for dir in "$@"
do
    parts=`echo "$dir" | sed 's,//*,/,g' | sed 's,\(.\)/,\1 ,g'`
    path=""
    for p in $parts
    do
        if test x"$path" = x
        then
            dir=$p
        else
            dir=$path/$p
        fi
        if test ! -d $dir
        then
            mkdir $dir
            chmod a+rx $dir;
        fi
        path=$dir
    done
done
