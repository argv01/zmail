
static char file_id[] =
    "$Id: ccconv.c,v 1.3 1995/07/25 22:44:31 tom Exp $";

/*
 *  WARNING: THIS SOURCE CODE IS PROVISIONAL. ITS FUNCTIONALITY
 *           AND BEHAVIOR IS AT ALFA TEST LEVEL. IT IS NOT
 *           RECOMMENDED FOR PRODUCTION USE.
 *
 *  This code has been produced for the C3 Task Force within
 *  TERENA (formerly RARE) by:
 *  
 *  ++ Peter Svanberg <psv@nada.kth.se>, fax: +46-8-790 09 30
 *     Royal Institute of Technology, Stockholm, Sweden
 *
 *  Use of this provisional source code is permitted and
 *  encouraged for testing and evaluation of the principles,
 *  software, and tableware of the C3 system.
 *
 *  More information about the C3 system in general can be
 *  found e.g. at
 *      <URL:http://www.nada.kth.se/i18n/c3/> and
 *      <URL:ftp://ftp.nada.kth.se/pub/i18n/c3/>
 *
 *  Questions, comments, bug reports etc. can be sent to the
 *  email address
 *      <c3-questions@nada.kth.se>
 *
 *  The version of this file and the date when it was last changed
 *  can be found on (or after) the line starting "static char file_id".
 *
 */

#include <stdio.h>

#ifdef THINK_C
#include <console.h>
extern char *
strdup(
      const char *str
);
#endif

#include <stdlib.h>
#include <string.h>
#ifdef UNIX
#include <strings.h>
#endif

#include "getopt/getopt.h"
#include "c3.h"
#include "util.h"
#include "euseq.h"
#include "fh.h"

/* The name this program was run with. */
char *pgnam;

#define MAXPATHLENGTH 1024

static char shortopts[] = "C:c:t:123rL:l:&%S:D:s:a:e:o:Rb:p:s:N:n:f:h:Vv:m:";

#define OPT_SOURCE	'C'
#define OPT_TARGET	'c'

#define OPT_TYPE	't'
#define OPT_1TO1	'1'
#define OPT_LEGIBLE	'2'
#define OPT_REVERSIBLE	'3'

#define OPT_RECONV	'r'
#define OPT_EOLSOURCE	'L'
#define OPT_EOLTARGET	'l'
#define OPT_SIGNAL	'&'
#define OPT_SIGNALSUBST	'%'
#define OPT_SELECT	's'
#define OPT_APPROX	'a'
#define OPT_DATAERROR	'e'
#define OPT_FACTOR	'i'
#define OPT_SYSTEM	'S'
#define OPT_SYSTEMDIR	'D'

#define OPT_PRESERVE	'p'
#define OPT_OUTPUT	'o'
#define OPT_RECURSIVE	'R'

#define OPT_BACKUP	'b'
#define OPT_BACKUP_NONE -1

#define OPT_PREFIX	'P'
#define OPT_SUFFIX	'X'
#define OPT_NAMESOURCE	'N'
#define OPT_NAMETARGET	'n'


#define OPT_FORCE	'f'

#define OPT_VERBOSITY	'v'
#define OPT_HELP	'h'
#define OPT_VERSION	'V'

#define OPT_MACRO	'M'
#define OPT_OPTIONFILE	'F'

#define OPT_INSTALL	'g'

#define OPT_FORCE_DEFAULT	FH_BACKUP_RENAME_ASK
#define OPT_BACKUP_DEFAULT	FH_BACKUP_METHOD_RENAMESOURCE
#define OPT_TYPE_DEFAULT	2
#define OPT_SYSTEM_DEFAULT	NULL
#define OPT_PREFIX_DEFAULT	""
#define OPT_SUFFIX_DEFAULT	".bak"
#define OPT_VERBOSITY_DEFAULT	2


static struct option longopts[] =
{
  {"source"	, required_argument, NULL, OPT_SOURCE},
  {"target"	, required_argument, NULL, OPT_TARGET},

  {"type"	, required_argument, NULL, OPT_TYPE},
  {"1to1"	, no_argument, NULL, OPT_1TO1},
  {"legible"	, no_argument, NULL, OPT_LEGIBLE},
  {"reversible"	, no_argument, NULL, OPT_REVERSIBLE},

  {"reconv"	, no_argument, NULL, OPT_RECONV},
  {"eolsource"	, required_argument, NULL, OPT_EOLSOURCE},
  {"eoltarget"	, required_argument, NULL, OPT_EOLTARGET},
  {"signal"	, required_argument, NULL, OPT_SIGNAL},
  {"signalsubst", required_argument, NULL, OPT_SIGNALSUBST},
  {"select"	, required_argument, NULL, OPT_SELECT},
  {"approx"	, required_argument, NULL, OPT_APPROX},
  {"dataerror"	, required_argument, NULL, OPT_DATAERROR},
  {"factor"	, required_argument, NULL, OPT_FACTOR},
  {"system"	, required_argument, NULL, OPT_SYSTEM},
  {"systemdir"	, required_argument, NULL, OPT_SYSTEMDIR},

  {"preserve"	, required_argument, NULL, OPT_PRESERVE},
  {"output"	, required_argument, NULL, OPT_OUTPUT},
  {"recursive"	, no_argument, NULL, OPT_RECURSIVE},
  {"backup"	, required_argument, NULL, OPT_BACKUP},
  {"prefix"	, required_argument, NULL, OPT_PREFIX},
  {"suffix"	, required_argument, NULL, OPT_SUFFIX},
  {"namesource"	, required_argument, NULL, OPT_NAMESOURCE},
  {"nametarget"	, required_argument, NULL, OPT_NAMETARGET},

  {"force"	, required_argument, NULL, OPT_FORCE},
  {"verbosity"	, required_argument, NULL, OPT_VERBOSITY},
  {"help"	, required_argument, NULL, OPT_HELP},
  {"version"	, no_argument, NULL, OPT_VERSION},

  {"macro"	, required_argument, NULL, OPT_MACRO},
  {"optionfile"	, required_argument, NULL, OPT_OPTIONFILE},

  {"install"	, required_argument, NULL, OPT_INSTALL},

  {NULL, 0, NULL, 0}
};

extern int opt_verbosity;

#define VERB_NONE  0
#define VERB_MINI  1
#define VERB_NORM  2
#define VERB_DEB1  3
#define VERB_DEB2  4

#define C3VERBOSE(level, format, id, value) \
    if (opt_verbosity >= level) \
    { \
	 fprintf(stderr, "%s: " format "\n", id, value); \
    }

void
help(int level)
{
    switch(level)
    {
    case 0:
	fprintf(stderr, "! %s - Convert text files between any coded character sets\n", pgnam);
	break;
    case 1:
	fprintf(stderr, "\
! Ex.: \"%s -source iso-8859-1 -target apple-1 -type 2 file.txt\" converts \n\
! file.ext from Latin-1 to Macintosh character set using the legible conversion\n\
! type. Old file becomes file.txt.bak . \"%s -help 2\" for more help.\n", pgnam, pgnam);
	break;
    case 2:
	fprintf(stderr, "\
! %s -- CONVERT TEXT FILES BETWEEN ANY CODED CHARACTER SETS:\n\
! Converts plain text from one coded character set to another\n\
! coded character set. It can be used either as a filter,\n\
! transforming stdin to stdout, or as program that reads and\n\
! writes explicitly specified files. Conversion parameters\n\
! determine the exact transformation performed. The coded\n\
! character set of the source text and the coded character set\n\
! desired in the target text must be given. The other parameters\n\
! have default values and include the choice between three\n\
! conversion types, the option of reconverting back to the\n\
! original file, and specification of end of line convention in\n\
! source and target text. A set of conversion tables, forming the\n\
! conversion system used, fully specifies all possible\n\
! transformations. \"%s -help 3\" gives more information.\n\
! Version: Ap45   Date: 1994-12-15\n", pgnam, pgnam);
	break;

    case 3:
	fprintf(stderr, "\
! %s -- CONVERT TEXT FILES BETWEEN ANY CODED CHARACTER SETS --------------!\n\
!                                                                             !\n\
! Usage:                                                                      !\n\
!    %s mandatoryopts [convopt...] [interactopt...]                       !\n\
!    %s mandatoryopts [convopt...] [fileopt...] [interactopt...] files... !\n\
!    %s helpopt...                                                        !\n\
!                                                                             !\n\
! mandatoryopts = -source charset  - existing coded character set             !\n\
!               | -target charset  - desired coded character set              !\n\
! convopt       = -type convtype   - conversion type 1..3                     !\n\
!               | -1to1            -   1:  1-to-1 conversion                  !\n\
!               | -legible         -   2*: legible conversion                 !\n\
!               | -reversible      -   3:  reversible conversion              !\n\
!               | -reconv          - reconvert to original file               !\n\
!               | -eolsource eol   - cr | lf | crlf* | lfcr                   !\n\
!               | -eoltarget eol   - cr | lf | crlf* | lfcr                   !\n\
!               | -system convsyst - name of conversion system to use         !\n\
! fileopt       = -output file     - new name for target file                 !\n\
!               | -force fcode     - protect | ask* | overwrite | destroy     !\n\
!               | -backup bcode    - renamesource* | renametarget | nokeep    !\n\
!               | -prefix string   - to prepend to file name ('' *)           !\n\
!               | -suffix string   - to append to file name ('.bak' *)        !\n\
! interactopt   = helpopt                                                     !\n\
!               | -verbosity level - 0: quiet; 1: error msgs and warnings     !\n\
! helpopt       = -help [level]    - 1: remainder; 2: note; 3: ready reckoner !\n\
!               | -version         - show version information                 !\n\
!                                                                             !\n\
! charset = ascii | x-iso-646-se1 | x-iso-646-se2 | iso-646-no | iso-646-gb   !\n\
!    | iso-646-yu | iso-8859-1 | iso-8859-2 | iso-8859-5 | x-ibm-437          !\n\
!    | x-ibm-850 | apple-1                                                    !\n\
!                                                                             !\n\
! Version: Ap45   Date: 1994-12-15       *default value                       !\n\
!-----------------------------------------------------------------------------!\n", pgnam, pgnam, pgnam, pgnam);
	break;

    }    
}

#define RBUFSIZE BUFSIZ
#define WBUFSIZE BUFSIZ*2

void
ccconvfile(
    fhandle fi,
    fhandle fo,
    int streamh
)
{
    int rbuf_bytes;
    int wbuf_bytes, bytes_w;
    char rbuf[RBUFSIZE], wbuf[WBUFSIZE];
    char *rbp = NULL, *rbpnew;
    char *outc_str;
    char *oc_mess;
    
    while (TRUE) {
	if (rbp == NULL)
	{
	    if (fh_is_eof(fi))
	    {
	        break;
	    }
	    fh_read_bytes(fi, RBUFSIZE, rbuf, &rbuf_bytes);
	    if (rbuf_bytes == 0)
		break;
	    else
		rbp = rbuf;
	}    
	c3_bconv(streamh, wbuf, rbp, WBUFSIZE, &wbuf_bytes, rbuf_bytes);
	fh_write_bytes(fo, wbuf_bytes, wbuf, &bytes_w);
	if (wbuf_bytes != bytes_w)
	{
	    /* TODO */
	    C3VERBOSE(VERB_MINI, "Write error!%c", pgnam, ' ');
	    exit(2);
	}
	switch (c3_outcome_code(&outc_str))
	{
	case C3E_SUCCESS:
	    rbp = NULL;
	    break;
	case C3E_TGTSTROFLOW:
	    rbpnew = c3_exception_data();
	    rbuf_bytes -= rbpnew - rbp;
	    rbp = rbpnew;
	    break;
	default:
	    if (c3_outcome_code(&oc_mess) < 0)
	    {
		/* TODO */
		C3VERBOSE(VERB_MINI, "c3_bconv outcome: \"%s\"", pgnam, oc_mess)
		C3VERBOSE(VERB_MINI, "Exectution interrupted%c", pgnam, ' ')
		exit(2);
	    }
	}
    }
}

#define OPT_NOT_IMPLEMENTED \
    fprintf(stderr, "%s: Option \"%s\" is not yet implemented.\n", \
	    pgnam, argv[optind-2]); \
    exit(2);

void
main(int argc, char **argv)
{
    int optc;
    int sh;
    fhandle fin = -1, fout = -1;

    int opt_source = 0,
    opt_target =  0,
    opt_install =  0,
    opt_type = OPT_TYPE_DEFAULT,
    opt_select = FALSE,
    opt_reconv = C3IV_DIRECT_CONV,
    opt_backup = OPT_BACKUP_DEFAULT,
    opt_force = OPT_FORCE_DEFAULT,
    opt_help = -1;

    char *opt_output = NULL;
    c3bool outfname_given = FALSE;

    char *opt_eolsource = NULL,
    *opt_eoltarget = NULL,
    *opt_signal = NULL,
    *opt_signalsubst = NULL,
    *opt_prefix = OPT_PREFIX_DEFAULT,
    *opt_suffix = OPT_SUFFIX_DEFAULT,
    *opt_system = OPT_SYSTEM_DEFAULT;

    char *oc_mess;

    opt_verbosity = OPT_VERBOSITY_DEFAULT;

#ifdef THINK_C
    argc = ccommand(&argv);
#endif

    euseq_set_extra_allocate(3);
    pgnam = strrchr(argv[0], '/');
    if (pgnam == NULL)
	pgnam = argv[0];
    else
	pgnam++;

    optind = 0;
    opterr = 0;

    while ((optc = getopt_long_only (argc, argv,
				shortopts, longopts, (int *) 0)) != EOF)
    {
	switch (optc)
	{
	case 0:			/* Long option. (?) */
	    C3VERBOSE(VERB_DEB2, "Strange case: optc = %d", pgnam, optc)
	    break;

	case OPT_SOURCE:
	    opt_source = atoi (optarg);
	    if (opt_source == 0)
	    {
		opt_source = c3_ccs_number("C3", optarg);
	    }
	    if (opt_source == -1)
	    {
		C3VERBOSE(VERB_MINI, "Unknown coded character set %s",
			pgnam, optarg);
		exit(2);
	    }

	    C3VERBOSE(VERB_DEB2, "Source ccs = %d", pgnam, opt_source)
	    break;

        case OPT_TARGET:
	    opt_target = atoi (optarg);
	    if (opt_target == 0)
	    {
		opt_target = c3_ccs_number("C3", optarg);
	    }
	    if (opt_target == -1)
	    {
		C3VERBOSE(VERB_MINI, "Unknown coded character set %s",
			pgnam, optarg);
		exit(2);
	    }
	    C3VERBOSE(VERB_DEB2, "Target ccs = %d", pgnam, opt_target)
	    break;

	case OPT_INSTALL:
	    opt_install = atoi (optarg);
	    if (opt_install == 0)
	    {
		opt_install = c3_ccs_number("C3", optarg);
	    }
	    if (opt_install == -1)
	    {
		C3VERBOSE(VERB_MINI, "Unknown coded character set %s",
			pgnam, optarg);
		exit(2);
	    }

	    C3VERBOSE(VERB_DEB2, "Installing ccs = %d", pgnam, opt_install)

	    break;

	case OPT_TYPE:
	    opt_type = atoi(optarg);
	    if (opt_type <= 0 || opt_type > 3)
	    {
		C3VERBOSE(VERB_MINI, "Unknown conversion type %s",
			pgnam, optarg);
		exit(2);
	    }
	    C3VERBOSE(VERB_DEB2, "Conversion type %d", pgnam, opt_type)
	    break;

	case OPT_1TO1:
	    opt_type = 1;
	    C3VERBOSE(VERB_DEB2, "Conversion type %d", pgnam, opt_type)
	    break;

	case OPT_LEGIBLE:
	    opt_type=2;
	    C3VERBOSE(VERB_DEB2, "Conversion type %d", pgnam, opt_type)
	    break;

	case OPT_REVERSIBLE:
	    opt_type=3;
	    C3VERBOSE(VERB_DEB2, "Conversion type %d", pgnam, opt_type)
	    break;

        case OPT_RECONV:
            opt_reconv = C3IV_DIRECT_RECONV;
	    C3VERBOSE(VERB_DEB2, "Reconversion %d", pgnam, opt_reconv)
	    break;

	case OPT_EOLSOURCE:
	    opt_eolsource = strdup(optarg);
	    C3VERBOSE(VERB_DEB2, "EOL source %s", pgnam, opt_eolsource)
	    /* TODO Parse here? */
	    break;

	case OPT_EOLTARGET:
	    opt_eoltarget = strdup(optarg);
	    C3VERBOSE(VERB_DEB2, "EOL target %s", pgnam, opt_eoltarget)
	    /* TODO Parse here? */
	    break;

	case OPT_SIGNAL:
	    opt_signal = strdup(optarg);
	    C3VERBOSE(VERB_DEB2, "Signal %s", pgnam, opt_signal)
	    /* TODO Parse here? */
	    break;

	case OPT_SIGNALSUBST:
	    opt_signalsubst = strdup(optarg);
	    C3VERBOSE(VERB_DEB2, "Signal substitute %s", pgnam, opt_signalsubst)
	    /* TODO Parse here? */
	    break;

	case OPT_SELECT:
	    OPT_NOT_IMPLEMENTED

	    opt_select = 1;
	    if (optarg != NULL) opt_select = atoi(optarg);
	    C3VERBOSE(VERB_DEB2, "Select level = %d", pgnam, opt_select)
	    break;

	case OPT_APPROX:
	case OPT_DATAERROR:
	case OPT_FACTOR:
	    OPT_NOT_IMPLEMENTED

	    break;


	case OPT_SYSTEM:
	    opt_system = optarg;
	    C3VERBOSE(VERB_DEB2, "Conversion system %s", pgnam, opt_system)
	    break;


	case OPT_SYSTEMDIR:
	case OPT_PRESERVE:
	    OPT_NOT_IMPLEMENTED

	    break;

        case OPT_OUTPUT:
	    opt_output = optarg;
	    C3VERBOSE(VERB_DEB2, "Output %s", pgnam, opt_output)
	    outfname_given = TRUE;
	    break;

	case OPT_RECURSIVE:
	    OPT_NOT_IMPLEMENTED

	    break;

	case OPT_FORCE:
	    if (strcasecmp(optarg, "destroy") == 0
		|| strcasecmp(optarg, "d") == 0)
	    {
		opt_force = FH_BACKUP_RENAME_DESTROY;
	    }
	    else if (strcasecmp(optarg, "overwrite") == 0
		|| strcasecmp(optarg, "o") == 0)
	    {
		opt_force = FH_BACKUP_RENAME_OVERWRITE;
	    }
	    else if (strcasecmp(optarg, "ask") == 0
		|| strcasecmp(optarg, "a") == 0)
	    {
		opt_force = FH_BACKUP_RENAME_ASK;
	    }
	    else if (strcasecmp(optarg, "protect") == 0
		     || strcasecmp(optarg, "p") == 0)
	    {
		opt_force = FH_BACKUP_RENAME_PROTECT;
	    }
	    else
	    {
		C3VERBOSE(VERB_MINI, "Unknown option value \"%s\"",
			pgnam, optarg);
		exit(2);
	    }
	    C3VERBOSE(VERB_DEB2, "Force %d", pgnam, opt_force)
	    break;

	case OPT_BACKUP:
	    if (strcasecmp(optarg, "nokeep") == 0
		|| strcasecmp(optarg, "n") == 0)
	    {
		opt_backup = FH_BACKUP_METHOD_NOKEEP;
	    }
	    else if (strcasecmp(optarg, "renamesource") == 0
		     || strcasecmp(optarg, "s") == 0)
	    {
		opt_backup = FH_BACKUP_METHOD_RENAMESOURCE;
	    }
	    else if (strcasecmp(optarg, "renametarget") == 0)
	    {
		opt_backup = FH_BACKUP_METHOD_RENAMETARGET;
	    }
	    else
	    {
		C3VERBOSE(VERB_MINI, "Unknown option value \"%s\"",
			pgnam, optarg);
		exit(2);
	    }
	    C3VERBOSE(VERB_DEB2, "Backup %d", pgnam, opt_backup)
	    break;

	case OPT_PREFIX:
	    opt_prefix = strdup(optarg);
	    C3VERBOSE(VERB_DEB2, "Prefix %s", pgnam, opt_prefix)
	    break;

	case OPT_SUFFIX:
	    opt_suffix = strdup(optarg);
	    C3VERBOSE(VERB_DEB2, "Suffix %s", pgnam, opt_suffix)
	    break;

	case OPT_NAMESOURCE:
	case OPT_NAMETARGET:
	    OPT_NOT_IMPLEMENTED
	    break;

	case OPT_VERBOSITY:
	    opt_verbosity = atoi(optarg);
	    C3VERBOSE(VERB_DEB2, "Verbosity level %d", pgnam, opt_verbosity)
	    break;

	case OPT_HELP:
	    opt_help = atoi(optarg);
	    C3VERBOSE(VERB_DEB2, "Help level %d", pgnam, opt_help)
	    break;

	case OPT_VERSION:
	    fprintf(stderr, "%s: Version info: %s\n", pgnam, file_id);
	    exit(2);

	case '?':
	    if (strcasecmp(argv[optind-1], "-help") != 0)
	    {
		C3VERBOSE(VERB_MINI, "Command line parsing ambigous - exiting.%c",
			pgnam, ' ');
	    }
            opt_help = 1;
	    break;

	default:
	    C3VERBOSE(VERB_MINI, "Unhandled getopt case, optc = %x",
		    pgnam, optc);
	    opt_help = 3;
	    break;

	}
    }

    if (opt_help >= 0)
    {
	help(opt_help);
	exit(1);
    }   

    if (opt_install == 0 &&
	(opt_source <= 0 || opt_target <= 0 || opt_type == -1))
    {
	if (opt_help < 0)
	{
	    C3VERBOSE(VERB_MINI, 
		    "Source, target and/or conversion type not given%c",
		    pgnam, ' ')
	    C3VERBOSE(VERB_MINI, 
		    "(Use \"%s -help\" for help)",
		    pgnam, pgnam)
	}
	exit(1);
	
    }
    if (opt_install == 0 && opt_source == opt_target)
    {
	C3VERBOSE(VERB_MINI, 
		"Same source and target coded character set given%c",
		pgnam, ' ')
	exit(1);
    }

    c3_initialize(opt_system);
    if (c3_outcome_code(&oc_mess) < 0)
    {
	/* TODO */
	C3VERBOSE(VERB_MINI, 
		"Problems while initializing: \"%s\"",
		pgnam, oc_mess)
	C3VERBOSE(VERB_MINI, "Exectution interrupted%c", pgnam, ' ')
	exit(2);
    }
/*    if (opt_install > 0)
    {
	if ( ! c3_start_compiling_OK(opt_system))
	{
	    exit(2);
	}
	if ( ! c3_compile_OK(opt_source, C3LIB_FILE_TYPE_SRC_DEFTAB))
	{
	    exit(2);
	}
	if ( ! c3_compile_OK(opt_target, C3LIB_FILE_TYPE_SRC_DEFTAB))
	{
	    exit(2);
	}
	if ( ! c3_end_compiling_OK())
	{
	    exit(2);
	}
 TODO	exit(0);
    }
*/
    sh = c3_create_stream();
    C3VERBOSE(VERB_DEB2, "Created stream %d", pgnam, sh)
    c3_set_iparam(sh, C3I_SRC_CCS, opt_source);
    c3_set_iparam(sh, C3I_TGT_CCS, opt_target);
    c3_set_iparam(sh, C3I_CTYPE, opt_type);
    c3_set_iparam(sh, C3I_DIRECT, opt_reconv);

    if (opt_eolsource != NULL)
	c3_set_sparam(sh, C3S_SRC_LB, opt_eolsource);
    if (opt_eoltarget != NULL)
	c3_set_sparam(sh, C3S_TGT_LB, opt_eoltarget);


    if (optind == argc && outfname_given)
    {					/* No infile but output */
	C3VERBOSE(VERB_MINI, "Option \"-output\" not allowed when filtering%c",
		pgnam, ' ');
	exit(2);
    }
    else if (optind < argc - 1 && outfname_given)
    {
	C3VERBOSE(VERB_MINI,
		"Option \"-output\" not allowed together with more than one input file%c",
		pgnam, ' ');
	exit(2);
    }

    if (optind == argc)
    {
        C3VERBOSE(VERB_DEB2, "Now handling <stdin> to <stdout> %c", pgnam, ' ')
	fin = fh_unix_std_hdl(FH_UNIX_STDIN);
	fout = fh_unix_std_hdl(FH_UNIX_STDOUT);
	ccconvfile(fin, fout, sh);
	fh_close_OK(fin);
	fh_close_OK(fout);
    }
    else {
	char *fname;
	for (fname=argv[optind]; optind < argc; optind++,fname=argv[optind])
	{
            C3VERBOSE(VERB_NORM, "File \"%s\"", pgnam, fname)
	    if (outfname_given && strcmp(opt_output, "-") == 0)
	    {
		C3VERBOSE(VERB_NORM, "Output to <stout>%c", pgnam, ' ')
		fout = fh_unix_std_hdl(FH_UNIX_STDOUT);
		opt_backup = OPT_BACKUP_NONE;
	    }
	    else if (outfname_given)
	    {
		if (strcmp(fname, opt_output) == 0)
		{
		    C3VERBOSE(VERB_MINI,
			    "Given output file = in file, not handled%c",
			    pgnam, ' ')
		    exit(2);
		}
		fout=fh_opened_by_name_hdl(opt_output, FH_OPEN_WRITE,
					    -1, opt_force);
		if (fout < 0) {
		    C3VERBOSE(VERB_MINI, "Can not open file \"%s\"", pgnam,
			    opt_output);
		    perror(pgnam);
		    exit(2);
		}
		C3VERBOSE(VERB_NORM, "Output to file \"%s\"", pgnam, opt_output)
		if (opt_backup != FH_BACKUP_METHOD_NOKEEP)
		{
		    opt_backup = OPT_BACKUP_NONE;
		}
	    }
	    else
	    {
		fout = fh_opened_temporary_hdl("ccconv", fname);
		if (fout < 0) {
		    C3VERBOSE(VERB_MINI, "Can not open temporary file%c",
			    pgnam, ' ');
		    perror(pgnam);
		    continue;
		}
	    }

	    fin=fh_opened_by_name_hdl(fname, FH_OPEN_READ, -1, 0);
	    if (fin < 0) {
		C3VERBOSE(VERB_MINI, "Can not open file \"%s\"", pgnam, fname)
		perror(pgnam);
		continue;
	    }

	    ccconvfile(fin, fout, sh);
	    fh_close_OK(fin);
	    fh_close_OK(fout);
	    if (opt_backup != OPT_BACKUP_NONE)
	    {
		if (outfname_given && opt_backup == FH_BACKUP_METHOD_NOKEEP)
		{
		    C3VERBOSE(VERB_NORM, "Removing file \"%s\"",
			    pgnam, fname)
		    fh_remove(fin);
		}
		else if (! fh_handle_updated_file_OK(fin, fout, opt_backup,
						 opt_prefix, opt_suffix,
						 opt_force))
		{
		    C3VERBOSE(VERB_MINI, "Backup handling error%c", pgnam, ' ');
		}
	    }
	}
    }


    c3_finalize_stream(sh);

    c3_finalize();

}
