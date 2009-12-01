#!/bin/sh

echo ""

if [ $# -ne 1 ]
then
echo "Warning: number of arguments to locale.sh should be 1"
arg="zmail"
else
arg=$1
fi

#first copy to ZMLIB

echo "Copying catalog files to $ZMLIB"
if [ $arg = zmail ]
then
cp lib/locale/C/zmail $ZMLIB/locale/C/zmail.bin
cp lib/locale/C/zmail $ZMLIB/locale/C/zmail.small.bin
else
cp lib/locale/C/zmlite $ZMLIB/locale/C/zmlite.$ZCARCHITECTURE
fi

# remainder is to hedge our bets on RH 9

foo1=$LANG
foo2=`echo $foo1 | cut -f 1 -d.` 
foo3=`echo $foo2 | cut -f 1 -d_`

base=`uname`
if [ $base = "Linux" ]
then
langdir=/usr/share/locale
langdir2=/usr/X11R6/lib/X11/locale
else
echo "Unable to determine OS"
echo "Language specific catalog files were not copied. Please contact xyz@openzmail.org"
fi

found=0

if [ $langdir ]
then
if [ ! -d $langdir ]
then
echo "Warning: $langdir does not exist"
fi

echo "Attempting to copy to $langdir/$foo2/LC_MESSAGES"
if [ -d $langdir/$foo2/LC_MESSAGES ]
then
echo "Copying to $langdir/$foo2/LC_MESSAGES"
if [ $arg = zmail ]
then
cp lib/locale/C/zmail $langdir/$foo2/LC_MESSAGES/zmail.bin
cp lib/locale/C/zmail $langdir/$foo2/LC_MESSAGES/zmail.small.bin
if [ -f $langdir/$foo2/LC_MESSAGES/zmail.bin ]
then
echo "Catalog copied to $langdir/$foo2/LC_MESSAGES" 
found=1
else 
echo "Warning: Unable to copy to $langdir/$foo2/LC_MESSAGES" 
fi
else
cp lib/locale/C/zmlite $langdir/$foo2/LC_MESSAGES/zmlite.$ZCARCHITECTURE
if [ -f $langdir/$foo2/LC_MESSAGES/zmlite.$ZCARCHITECTURE ]
then
echo "Catalog copied to $langdir/$foo2/LC_MESSAGES" 
found=1
else 
echo "Warning: Unable to copy to $langdir/$foo2/LC_MESSAGES" 
fi
fi
fi

echo "Attempting to copy to $langdir/$foo3/LC_MESSAGES"
if [ -d $langdir/$foo3/LC_MESSAGES ]
then
echo "Copying to $langdir/$foo3/LC_MESSAGES"
if [ $arg = zmail ]
then
cp lib/locale/C/zmail $langdir/$foo3/LC_MESSAGES/zmail.bin
cp lib/locale/C/zmail $langdir/$foo3/LC_MESSAGES/zmail.small.bin
if [ -f $langdir/$foo3/LC_MESSAGES/zmail.bin ]
then
echo "Catalog copied to $langdir/$foo3/LC_MESSAGES" 
found=1
else 
echo "Warning: Unable to copy to $langdir/$foo3/LC_MESSAGES" 
fi
else
cp lib/locale/C/zmlite $langdir/$foo3/LC_MESSAGES/zmlite.$ZCARCHITECTURE
if [ -f $langdir/$foo3/LC_MESSAGES/zmlite.$ZCARCHITECTURE ]
then
echo "Catalog copied to $langdir/$foo3/LC_MESSAGES" 
found=1
else 
echo "Warning: Unable to copy to $langdir/$foo3/LC_MESSAGES" 
fi
fi
fi

echo "Attempting to copy to $langdir/C"
if [ -d $langdir/C ]
then
echo "Copying to $langdir/C"
if [ $arg = zmail ]
then
cp lib/locale/C/zmail $langdir/C/zmail.bin
cp lib/locale/C/zmail $langdir/C/zmail.small.bin
if [ -f $langdir/C/zmail.bin ]
then
echo "Catalog copied to $langdir/C" 
found=1
else 
echo "Warning: Unable to copy to $langdir/C" 
fi
else
cp lib/locale/C/zmlite $langdir/C/zmlite.$ZCARCHITECTURE
if [ -f $langdir/C/zmlite.$ZCARCHITECTURE ]
then
echo "Catalog copied to $langdir/C" 
found=1
else 
echo "Warning: Unable to copy to $langdir/C" 
fi
fi
fi
fi

if [ $langdir2 ]
then
if [ ! -d $langdir2 ]
then
echo "Warning: $langdir2 does not exist"
fi

echo "Attempting to copy to $langdir2/C"
if [ -d $langdir2/C ]
then
echo "Copying to $langdir2/C"
if [ $arg = zmail ]
then
cp lib/locale/C/zmail $langdir2/C/zmail.bin
cp lib/locale/C/zmail $langdir2/C/zmail.small.bin
if [ -f $langdir2/C/zmail.bin ]
then
echo "Catalog copied to $langdir2/C" 
found=1
else 
echo "Warning: Unable to copy to $langdir2/C" 
fi
else
cp lib/locale/C/zmlite $langdir2/C/zmlite.$ZCARCHITECTURE
if [ -f $langdir2/C/zmlite.$ZCARCHITECTURE ]
then
echo "Catalog copied to $langdir2/C" 
found=1
else 
echo "Warning: Unable to copy to $langdir2/C" 
fi
fi
fi
fi

if [ $found -eq 0 ]
then
echo "Warning: Unable to copy catalog files. Please contact xyz@openzmail.org"
else
echo "Copying successful"
fi
