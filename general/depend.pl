#!/usr/local/bin/perl

while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;
    if (/^-I(.+)/) {
	push(@IncludeDirs, $1);
    } elsif (/^-D(.+)/) {
	# ignore -D's
    } elsif (/^-S(.*)/) {
	$srcdir = $1;
    } else {
	die "Unknown option `$_'\n";
    }
}

while (<>) {
    if (/^\#\s*include\s+(.*)/) {
	$inclusion = $1;
	if ($inclusion =~ /^[<"]([^>"]+)[>"]/) {
	    $inclusion = $1;
	    if (@IncludeDirs) {
		foreach $dir (@IncludeDirs) {
		    if (($inclusion =~ /^(\.\.?)?\//)
			|| ($dir eq '.')) {
			if (-f $inclusion) {
			    if ($dep{$ARGV}) {
				$dep{$ARGV} .= " $inclusion";
			    } else {
				$dep{$ARGV} = $inclusion;
			    }
			    last;
			}
		    } elsif (-f "$dir/$inclusion") {
			$inclusion = "$dir/$inclusion";
			if ($dep{$ARGV}) {
			    $dep{$ARGV} .= " $inclusion";
			} else {
			    $dep{$ARGV} = $inclusion;
			}
			last;
		    }
		}
	    } elsif (-f $inclusion) {
		if ($dep{$ARGV}) {
		    $dep{$ARGV} .= " $inclusion";
		} else {
		    $dep{$ARGV} = $inclusion;
		}
	    }
	} elsif ($inclusion =~ /^"([^\"]+)"/) {
	    $inclusion = $1;
	    if ($inclusion !~ /^\//) {
		if ($ARGV =~ /^(.*)\/([^\/]+)$/) {
		    $inclusion = "$1/$inclusion";
		}
	    }
	    if (-f $inclusion) {
		if ($dep{$ARGV}) {
		    $dep{$ARGV} .= " $inclusion";
		} else {
		    $dep{$ARGV} = $inclusion;
		}
	    }
	}
    }
}

foreach $key (keys %dep) {
    next unless ($key =~ /^(.+)\.c$/);
    $rootname = $1;
    @deps = split(/\s+/, $dep{$key});
    foreach $dep (@deps) {
	&AddDep("$rootname.o", $dep);
    }
}

foreach $key (keys %targets) {
    @_ = split(/\s+/, $targets{$key});
    foreach (@_) {
    	s:$srcdir:\$(SRCDIR):;
	print "$key: $_\n";
    }
}

sub AddDep {
    local($target, $dependency) = @_;
    local($d) = $targets{$target};
    local(@d) = split(/\s+/, $d);

    foreach (@d) {
	return if ($_ eq $dependency);
    }
    if ($d) {
	$targets{$target} .= " $dependency";
    } else {
	$targets{$target} = $dependency;
    }
    if ($d = $dep{$dependency}) {
	@d = split(/\s+/, $d);
	foreach $d (@d) {
	    &AddDep($target, $d);
	}
    }
}
