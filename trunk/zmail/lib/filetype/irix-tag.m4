divert(-1)


Yes, this really is an m4 script.  Why?  Because I could.  Actually,
m4 deals with conditionals and macro definitions very nicely, and also
has just the right kind of built-in arithmetic primitives.  Just think
of this as a somewhat obfuscated shell script, where the shell happens
to be a macro preprocessor.

The output of this script would be meaningless, which is why we use
`divert(-1)' above to suppress it.  The side-effects of a call to
`syscmd' are all that really count.  I could have left out most of the
internal commentary, and used `dnl' all over the place, but that just
makes things ugly and hard to read.

--------------------

The macro `executable' should be defined, on the command line, to the
name of the binary to be tagged.  If it is undefined, we assume zmail:

ifdef(`executable',,`define(executable,zmail)')

--------------------

MediaMail is usually built as Basic, for bundling.  But MediaMail Pro
does really exist, and is built for SGI-internal use only.  So if only
`MEDIAMAIL' is defined, with neither `SGI_CUSTOM' nor `ZMAIL_BASIC',
what are we to do?  Answer: assume we are talking Basic.

ifdef(`MEDIAMAIL',
`ifdef(`SGI_CUSTOM',,
`define(`ZMAIL_BASIC')')')

--------------------

SGI has assigned us tags 0x00e50000 through 0x00e5ffff.  Think of the
lowest one as a base, which gives us an ample 16 low-end bits to play
with.  We will fill those flexible bits in later.  First, though,
define our working base:

define(tagBase,0x00e50000)

--------------------

The brains of the operation.  The `mask' macro takes two arguments.
The first is a bit position.  The second is the name of a macro that
may or may not be defined.  If the macro is defined, `mask' emits an
arithmetic fragment that adds in the specified bit position.  So it
might emit `+ 2 ** 3' to mask in bit position 3, for example.

The whole idea behind this is to map (un)defined macros onto (un)set
bit positions within a single integer.  That integer will then be used
to tag the zmail binary.  So different build configurations have
different macros, which means they end up with distinguishable tags.

define(mask,`ifdef(`$2',+ 2 ** $1 )')

--------------------

The other brains of the operation.  Start with our assigned `tagBase'.
Using the `mask' macro defined above, fill in some of the flexible
bits based on configuration options from the makefile.  Pass the
result down to tag(1) along with the name of our target binary.
Ta-da!

syscmd(`tag 'eval(
tagBase
mask(0,`MEDIAMAIL')
mask(1,`ZMAIL_BASIC')
mask(2,`GUI')
mask(3,`MOTIF')
) executable)

--------------------

Finally, exit with the same success code as that tag(1) command.  If
it worked, we worked; if it failed, we failed.

m4exit(sysval)
