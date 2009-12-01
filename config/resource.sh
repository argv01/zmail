# may be obsolete?  pf Wed Sep  8 08:41:09 1993
cp $2 resource.tmp
sed < resource.tmp > $2 's/version: .*/version: '`awk '
    /define/ { if ($3 == "RELEASE")	{ release = $4 }	}
    /define/ { if ($3 == "REVISION")	{ revision = $4 }	}
    /define/ { if ($3 == "PATCHLEVEL")	{ patchlevel = $4 }	}
    END { print release "." revision "." patchlevel }' < $1 | sed 's/"//g'`/ &&
rm resource.tmp
