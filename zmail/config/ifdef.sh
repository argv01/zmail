#!/bin/sh
#
# Shell script to perform substitutions on Z-mail data files.
# Lines in the input file starting with "@@" introduce special sequences:
#
# usage: ifdef.sh input-file output-file cpp-command cpp-command-args
#
# @@# comment text
# - comment (will not appear in the output of this script)
#
# @@ifdef sym
# ...
# @@else [arbitrary text]
# ...
# @@endif [arbitrary text]
# - conditional text inclusion, just like an #ifdef in C.  The @@else
#   is optional, of course.
#
# @@ifndef sym
# ...
# @@else [arbitrary text]
# ...
# @@endif [arbitrary text]
# - just like an #ifndef.
#
# @@if exp
# ...
# @@else [arbitrary text]
# ...
# @@endif [arbitrary text]
# - conditional text inclusion.  If exp is true, the text is included.
#   The exp takes the following form:
#
#   exp := ( exp )
#       := exp || exp
#       := exp && exp
#       := exp == exp
#       := exp < exp
#       := exp > exp
#       := ! exp
#       := symbol          (evaluates to true if the symbol is defined,
#                           otherwise false)
#
# @@gencount base sym sym sym ...
# - counts up the number of "sym"s that are defined, adds that to
#   to "base", and outputs the result on a line by itself.  Intended
#   for use in the variables file.
#
# The strings @<RELEASE>@, @<REVISION>@, and @<PATCHLEVEL>@ are
# replaced (anywhere in the text) with the appropriate version information.
#
infile="$1"
outfile="$2"
if test -f $outfile; then chmod +w $outfile; fi

shift; shift
(
  (
  # form a "testfile" to get the values of all the cpp symbols we need
  echo '#define _ifdef_sh_'
  echo '#include "osconfig.h"'
  echo '#include "features.h"'
  echo '#include "shell/version.c"'
  echo '#include "config.h"'
  echo '#ifdef MOTIF'
  echo '#include <Xm/Xm.h>'
  echo '#endif'
  for i in `sed -e '/^@@if/{s/[()!<>]/ & /g;s/||/ & /g;s/ && / & /g;s/ == / & /g;}' $infile |
	    sed -n 's/^@@if[ndef]*[ 	]//p'`
  do
     case $i in
     [a-zA-Z_]*) echo "
#ifdef $i
@@define_$i $i
#endif
" ;;
     esac
  done
  echo "@@vers_release RELEASE"
  echo "@@vers_revision REVISION"
  echo "@@vers_patchlevel PATCHLEVEL"
  ) > /tmp/ifdef$$.c
  # call the preprocessor
  $* /tmp/ifdef$$.c > /tmp/ifdefs$$
  vrelease=`awk '/^@@vers_release/ { print $2 }' /tmp/ifdefs$$`
  vrevision=`sed -n 's/^@@vers_revision[ 	]*"\([^ 	]*\)".*/\1/p' /tmp/ifdefs$$`
  vpatchlevel=`awk '/^@@vers_patchlevel/ { print $2 }' /tmp/ifdefs$$`
  cat /tmp/ifdefs$$
  rm -f /tmp/ifdef$$.c /tmp/ifdefs$$
  echo '@@-'
  sed -e "s/@<RELEASE>@/$vrelease/" \
      -e "s/@<REVISION>@/$vrevision/" \
      -e "s/@<PATCHLEVEL>@/$vpatchlevel/" $infile
) | sed -n -e 's/@@define_//p' -e '/@@-/,$'p |
    sed -e '/@@if/{s/[()!<>]/ & /g;s/||/ & /g;s/ && / & /g;}' |
awk '
  BEGIN { reading_defs = 1; suppress = level = 0; }
  /^@@#/ { next; }
  /^@@-/ { reading_defs = 0; firstline = NR; next; }
  reading_defs > 0 {
      if ($2 == "")
	  defined[$1] = 1;
      else
	  defined[$1] = $2;
      next;
  }
  /^@@if / {
      vsp = 0;
      osp = 0;
      for (i = 2; i <= NF; i++) {
	  if ($i ~ /^([\(<>!]|\|\||&&|==)$/)
	      ostack[osp++] = $i;
	  else {
	      if ($i == ")") {
		  if (ostack[--osp] != "(") osp = 20; # error
	      } else if ($i ~ /^[0-9]+$/)
		  vstack[vsp++] = $i;
	      else
		  vstack[vsp++] = defined[$i];

	      while (1) {
	      	  if (osp == 0 || ostack[osp-1] == "(") {
		      break;
		  } else if (ostack[osp-1] == "||") {
		      b = vstack[--vsp];
		      a = vstack[--vsp];
		      val = 0;
		      if (a || b) val = 1;
		      vstack[vsp++] = val;
		      osp--;
		  } else if (ostack[osp-1] == "&&") {
		      b = vstack[--vsp];
		      a = vstack[--vsp];
		      val = 0;
		      if (a && b) val = 1;
		      vstack[vsp++] = val;
		      osp--;
		  } else if (ostack[osp-1] == "!") {
		      val = 0;
		      if (!vstack[vsp-1]) val = 1;
		      vstack[vsp-1] = val;
		      osp--;
		  } else if (ostack[osp-1] == ">") {
		      b = vstack[--vsp];
		      a = vstack[--vsp];
		      val = 0;
		      if (a > b) val = 1;
		      vstack[vsp++] = val;
		      osp--;
		  } else if (ostack[osp-1] == "<") {
		      b = vstack[--vsp];
		      a = vstack[--vsp];
		      val = 0;
		      if (a < b) val = 1;
		      vstack[vsp++] = val;
		      osp--;
		  } else if (ostack[osp-1] == "==") {
		      b = vstack[--vsp];
		      a = vstack[--vsp];
		      val = 0;
		      if (a == b) val = 1;
		      vstack[vsp++] = val;
		      osp--;
		  } else {
		      osp = 20; # error
		      break;
		  }
	      }
	  }
      }
      if (vsp != 1 || osp != 0)
	  printf "@@! illegal expression on line %d\n", NR-firstline;
      else {
	  fail = 0;
	  if (!vstack[0]) fail = 1;
	  stacklines[level] = NR-firstline;
	  stack[level++] = fail;
	  if (fail) suppress++;
	  next;
      }
  }
  /^@@ifdef/ {
      fail = 0;
      if (NF != 2) printf "@@! syntax error on line %d\n", NR-firstline;
      if (!defined[$2]) fail = 1;
      stacklines[level] = NR-firstline;
      stack[level++] = fail;
      if (fail) suppress++;
      next;
  }
  /^@@ifndef/ {
      fail = 0;
      if (NF != 2) printf "@@! syntax error on line %d\n", NR-firstline;
      if (defined[$2]) fail = 1;
      stacklines[level] = NR-firstline;
      stack[level++] = fail;
      if (fail) suppress++;
      next;
  }
  /^@@else/ {
      if (!level) printf "@@! else not inside ifdef on line %d\n",NR-firstline;
      if (stack[level-1] == 0) { suppress++; stack[level-1] = 1; }
      else if (stack[level-1] == 1) { suppress--; stack[level-1] = 0; }
      next;
  }
  /^@@gencount/ {
      if (suppress) next;
      count = $2;
      for (i = 3; i <= NF; i++)
	  if (defined[$i]) count++;
      print count;
      next;
  }
  /^@@endif/ {
      if (!level) printf "@@! endif with no matching ifdef on line %d\n", NR-firstline;
      else if (stack[--level]) suppress--;
      next;
  }
  /^@@#/ { next; }
  /^@@/ {
      printf "@@! Illegal preprocessor directive at line %d\n", NR-firstline;
      next;
  }
  suppress == 0 { print; }
  END { if (level > 0) printf "@@! Unterminated ifdef at line %d\n", stacklines[level-1]; }
' > ifdef.$$

# check for errors, report them to stderr, and exit with error status
# if we found any.

if sed -n 's/^@@! //p' ifdef.$$ | grep . 1>&2
then rm -f ifdef.$$; exit 1
else 
    case $outfile in
	-) cat ifdef.$$; rm -f ifdef.$$;;
	*) rm -f $outfile; mv ifdef.$$ $outfile; chmod -w $outfile;;
    esac
    exit 0
fi
