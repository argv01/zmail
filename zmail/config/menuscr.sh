#!/bin/sh
root=$1
shift
$root/config/ifdef.sh $root/lib/system.menus $root/lib/menus.tmp "$@"
case "$*" in
*-DMOTIF*)
  sed > $root/lib/menus.tmp2 < $root/lib/menus.tmp \
    -e '/^menu/s/ *-mnemonic . */ /' \
    -e '/^menu/s/ *-M . */ /' \
    -e "/^menu/s/ *-accelerator '[^']*' */ /" \
    -e '/^menu/s/ *-accelerator "[^"]*" */ /' \
    -e "/^menu/s/ *-label '[^']*' */ /" \
    -e '/^menu/s/ *-label "[^"]*" */ /' \
    -e '/^menu/s/ *-label [^ ]* */ /';;
*) cp $root/lib/menus.tmp $root/lib/menus.tmp2
   rm -f $root/lib/menus.tmp;;
esac

if grep 'PRO-ONLY MENU CODE' >/dev/null $root/lib/menus.tmp2
then
    sed '/START OF PRO-ONLY/,/END OF PRO-ONLY/d' \
      $root/lib/menus.tmp2 > $root/lib/zmail.menus
    if (nawk '{ exit 0 }' /dev/null) 2>/dev/null
    then awk=nawk
    elif (gawk '{ exit 0 }' /dev/null) 2>/dev/null
    then awk=gawk
    else awk=awk
    fi
    $awk -f $root/config/defmenus.awk \
      $root/lib/menus.tmp2 > $root/shell/defmenus.h
else
    cp $root/lib/menus.tmp2 $root/lib/zmail.menus
    rm -f $root/lib/menus.tmp2
    echo '/* this space intentionally left blank */' > $root/shell/defmenus.h
fi

rm -f $root/lib/menus.tmp $root/lib/menus.tmp2
