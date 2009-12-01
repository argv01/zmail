#!/usr/local/bin/perl

## Preamble
@_ = split(/\/+/, $0);
$ProgramName = pop(@_);
$Version = ' $Revision: 2.5 $ ';
$Version =~ s/^\D*(\d+(\.\d+)*).*/\1/;

$Verbose = 0;

## Parse options
while ($_ = $ARGV[0], /^-/) {
    shift;
    last if /^--$/;

    if (/^-I(.*)/) {
	push(@Includes, ($1 || shift));
    } elsif (/^-V$/) {
	print "$ProgramName, version $Version\n";
	exit(0);
    } elsif (/^-(v+)$/) {
	$Verbose += length($1);
    }
}

@Includes = ('.') unless @Includes;

&EnqueueFiles(@ARGV);

while ($Thisfile = &ProcessFile) {
    warn "Processing $Thisfile\n" if $Verbose;
    open(FILE, "$Thisfile") || die "Could not open \"$Thisfile\" ($!)\n";
    while (<FILE>) {
	if (($type, $include) = &ParseInclude) {
	    if ($found = &FindInclude($type, $include)) {
		warn " Processing include of \"$include\"\n" if ($Verbose > 1);
		&Includes($Thisfile, $found);
	    } else {
		warn "  Ignoring include of \"$include\"\n" if ($Verbose > 2);
	    }
	}
    }
}

warn "Performing transitive closure...\n" if ($Verbose > 1);

open(SORT, "|sort -u");
foreach $file (@ARGV) {
    next unless ($file =~ /^(.+)\.cc?$/);
    $root = $1;
    %Done = ();
    foreach $include (&GetIncludes($file)) {
	$Done{$include} = 1;
	&PrintDepends($root, $include);
    }
}
close(SORT);
wait;				# Don't exit before "sort" does

########################################################################

sub PrintDepends {
    local($root, $include) = @_;
    local($subinclude);

    print SORT "$root.o: $include\n";
    foreach $subinclude (&GetIncludes($include)) {
	if (!$Done{$subinclude}) {
	    $Done{$subinclude} = 1;
	    &PrintDepends($root, $subinclude);
	}
    }
}

sub EnqueueFiles {
    push(@Filequeue, @_);
}

sub ProcessFile {
    local($x);

    while (@Filequeue
	   && ($x = shift(@Filequeue))) {
	if (!$Processedfile{$x}) {
	    $Processedfile{$x} = 1;
	    return $x;
	}
    }
    '';
}

sub FindInclude {
    local($type, $include) = @_;
    local($filename);
    local($found);

    if ($found = $Found{$include}) {
	return $found;
    }

    if ($include =~ /^\//) {
	$filename = &Canonicalize($include);
	$found = $filename if (-f $filename);
    } elsif ($include =~ /^\.\.?\//) {
	$filename = &Canonicalize(&JoinPaths(&FileDir($Thisfile), $include));
	$found = $filename if (-f $filename);
    } else {
	foreach $dir (@Includes) {
	    $filename = &Canonicalize(&JoinPaths($dir, $include));
	    if (-f $filename) {
		$found = $filename;
		last;
	    }
	}
	if (!$found && ($type eq 'dquote')) {
	    $filename = &Canonicalize(&JoinPaths(&FileDir($Thisfile),
						 $include));
	    $found = $filename if (-f $filename);
	}
    }

    if ($found) {
	$Found{$include} = $found;
    }
    $found;
}

sub Canonicalize {
    local($file) = @_;
    local(@x, @y);

    @x = split(/\//, $file);
    if ($x[0] eq '') {
	$y[0] = '';
	shift(@x);
    }
    while (@x) {
	if ($x[0] eq '') {
	    shift(@x);
	} elsif ($x[0] eq '.') {
	    shift(@x);
	} elsif (($x[1] eq '..')
		 && ($x[0] ne '..')) {
	    shift(@x);
	    shift(@x);
	} else {
	    push(@y, shift(@x));
	}
    }
    join('/', @y);
}

sub FileDir {
    local($file) = @_;
    local(@x);

    @x = split(/\//, $file);
    pop(@x);
    join('/', @x) || '.';
}

sub JoinPaths {
    local(@paths, @parts);
    local($x, $y);
    local($result) = '';

    $result = '/' if ($_[0] =~ /^\//);
    foreach $x (@_) {
	@parts = split(/\/+/, $x);
	foreach $y (@parts) {
	    push(@paths, $y) if $y;
	}
    }
    $result .= join('/', @paths);
}

sub Includes {
    local($file, $include) = @_;

    $Includes{$file} .= "$include ";
    &EnqueueFiles($include);
}

sub GetIncludes {
    local($file) = @_;
    local($x) = $Includes{$file};

    chop($x);
    split(' ', $x);
}

sub ParseInclude {
    if (/^#\s*include\s+<([^>]+)>/) {
	return ('angle', $1);
    } elsif (/^#\s*include\s+\"([^\"]+)\"/) {
	return ('dquote', $1);     
    }
    ();
}

