/* $Id: c3.h,v 1.9 1995/08/16 17:18:21 tom Exp $ */

#ifndef __c3_include__
#define __c3_include__

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
 *  can be found on the first line.
 *
 */

#include "c3_macros.h"

#define C3_SYSNAME		"C3"

#ifdef MAC_OS
char *CSYST_DEF = NULL;
#endif /* MAC_OS */

#define NoMimeCharSet			-1
#define UnknownMimeCharSet              0
#define IllegalMimeCharSet              1
#define UsAscii                         20006
#define FirstRegisteredMimeCharSet      UsAscii
#define Iso_8859_1                      20100
#define Iso_8859_2                      20101
#define Iso_8859_3                      20102
#define Iso_8859_4                      20103
#define Iso_8859_5                      20144
#define Iso_8859_6                      20105
#define Iso_8859_7                      20106
#define Iso_8859_8                      20107
#define Iso_8859_9                      20108
#define LastRegisteredMimeCharSet       Iso_8859_9
#define X_Macintosh                     35000
#define IsRegisteredMimeCharSet(s) ((s) >= FirstRegisteredMimeCharSet &&  \
	(s) <= LastRegisteredMimeCharSet)
#define IsExtendedMimeCharSet(s) ((s) > LastRegisteredMimeCharSet)
#define Iso_646_Gb                      20004
#define Iso_646_No                      20060
#define Iso_646_Yu                      20141
#define Iso_2022_jp                     44444  /* XXX not yet known */
#define X_Iso_10646_2_1                 40000
#define X_Iso_646_Se1                   40001
#define X_Iso_646_Se2                   40002
#define X_IBM_437                       40003
#define X_IBM_850                       40004
#define LastKnownMimeCharSet            X_IBM_850

extern struct ccs_no_id_mime *ccs_noidmime;
extern int ccs_noidmime_length;


#define C3E_SUCCESS		0

/*  Warning states
 */
#define C3W_AAA		    	1
#define C3W_CONVNOTFINAL	1
#define C3W_CPARNOTINIT    	2
#define C3W_CSYSTNOTFIN		3
#define C3W_CSYSTNOTINIT	4
#define C3W_EMPTYSRCSTR		5
#define C3W_INVALEOS		6
#define C3W_MPARNOTINIT		7
#define C3W_SAMECCS		8
#define C3W_ZZZ			C3W_SAMECCS

/* Error states
 */
#define C3E_AAA		  -1

#define C3E_CCSNOINFO	  -1
#define C3E_CPARNOTINIT   -2
#define C3E_CPARVALBAD    -3
#define C3E_CSYSTNOTINIT  -4
#define C3E_CSYSTSTARVE   -5

#define C3E_INTERNAL      -6
#define C3E_INVALACT	  -7
#define C3E_INVALCCS	  -8
#define C3E_INVALCMANO    -9
#define C3E_INVALCPAR	  -10
#define C3E_INVALCPARVAL  -11
#define C3E_INVALCSHANDL  -12
#define C3E_INVALCSYST    -13
#define C3E_INVALLEV	  -14
#define C3E_INVALMSHANDL  -15
#define C3E_INVALOUTC	  -16
#define C3E_INVALSHANDL   -17
#define C3E_INVALSYST	  -18
#define C3E_INVALTGTSTR   -19
#define C3E_INVALTOPIC    -20

#define C3E_NODEFAULT	  -21
#define C3E_NOMEM	  -22
#define C3E_NOPREVCONV    -23

#define C3E_PARNOSTR      -24
#define C3E_PARNOTINT	  -25
#define C3E_POSSOVERLAP   -26

#define C3E_SUBPAREXC	  -27

#define C3E_TGTSTROFLOW	  -28
#define C3E_INVALSRCENC   -29

#define C3E_ZZZ		  C3E_INVALSRCENC

/*  API parameters
 */
#define C3I_SRC_CCS	1
#define C3I_TGT_CCS	2
#define C3I_CTYPE	3

#define C3S_SRC_LB	4
#define C3S_TGT_LB	5
#define C3I_DIRECT	6
#define C3S_SIGN	7
#define C3S_SIGN_SUBST	8
#define C3I_SELECT	9
#define C3S_APPROX	10
#define C3S_DATA_ERR	11

#define C3F_TGT_ENVIRON		21
#define C3F_CYR_LANG		22
#define C3F_SRC_PC_CTRL_CHAR	23
#define C3F_TGT_PC_CTRL_CHAR	24
#define C3F_PRESERVE_ASCII	25
#define C3F_RCGN_646_SWE	26

#define C3X_PARAM_ZZZ		26


/*
 * Values of API parameters
 */
#define C3PV_EHL_PASSIVE	0
#define C3PV_EHL_INFORMATIVE	1
#define C3PV_EHL_SECURE		2
#define C3PV_EHL_WARY		3
#define C3PV_EHL_ZZZ		C3PV_EHL_WARY

#define C3PV_EHA_DELAYED	0
#define C3PV_EHA_IMMEDIATE	1
#define C3PV_EHA_ZZZ		C3PV_EHA_IMMEDIATE

#define C3IV_CTYPE_UNDEF	0
#define C3IV_CTYPE_SINGLE	1
#define C3IV_CTYPE_LEGIBLE	2
#define C3IV_CTYPE_UNIQUE	3

#define C3IV_DIRECT_CONV	0
#define C3IV_DIRECT_RECONV	1

#define C3IV_SELECT_ALL			0
#define C3IV_SELECT_PART_WITHOUT_TRACE	2
#define C3IV_SELECT_PART_WITH_TRACE	3

#define C3SV_APPROX_TYPE_NO		0
#define C3SV_APPROX_TYPE_ONE		1
#define C3SV_APPROX_TYPE_TWO		2

#define C3IV_DATA_ERR_IGN		0
#define C3IV_DATA_ERR_COPY		1
#define C3IV_DATA_ERR_FIX		2
#define C3IV_DATA_ERR_ECODE		3
#define C3IV_DATA_ERR_SCODE		4
#define C3IV_DATA_ERR_ESCODE		5
#define C3IV_DATA_ERR_SECODE		6


/*
 * Other
 */

#ifndef _c3bool_defined
typedef short int c3bool;
#define _c3bool_defined
#endif

#define C3IL_MAX_STREAM_HANDLES 16
#define C3IL_MIN_MEMORY
#define C3IL_MAX_SRC_LB 6

#define UCS2_NOT_A_CHAR 0xffff
#define UCS4_NOT_A_CHAR 0x0000ffff


struct stream_properties {
	/*
	 * The parameters (except A1)
	 */
	int    src_cs;	/* A2 */
	int    tgt_cs;	/* A3 */
	int    ctype;	/* A4 */
	char  *src_lb;	/* A5 */
	char  *tgt_lb;	/* A6 */
	int    direct;	/* B1 */
	char  *sign;	/* B2 */
	char  *sign_dubst;/* B3 */
	int    select;	/* C1 */
	char  *approx[2];   /* C2 */
	struct {
		int    ints[2]; /* C3, number subparameters */
		char  *strs[3]; /* C3, string subparameters */
	} data_err;
	struct {
		char *name;
		int value;
	} **factors;
	/*
	 * About the conversion operation which the above
	 * conversion parameters impy
	 */
	int max_string_length; /* Max/min length of any string output
	int min_string_length;  * when a byte is input to the "conversion
				* machine". */
};

extern struct stream_properties *c3_stream_properties P(( int ));


struct api_info {
	/*
	 * API current values
	 */
	char  *dialog_language;
	char  *dialog_charset;
	char  *csyst;	/* A1 */
	int    outcome_handling_level;
	/*
	 * API defaults
	 */
	char  *d_dialog_language;
	char  *d_dialog_charset;
	char  *d_csyst;	/* A1 */
	/*
	 * API possible values
	 */
	char **dialog_language_values;
	char **dialog_charset_values;
	char  *csyst_values;	/* A1 */
};

extern struct api_info * c3_api_info P(( void ));


struct c3struct_values_param {
    int   *src_ccs;  /* parameter A2: source character set */
    int   *tgt_ccs;  /* parameter A3: target character set */
    int   *ctype;    /* parameter A4: conversion type */
    int   *direct;   /* parameter B1: conversion direction */
    int   *select;   /* parameter C1: conversion selectiveness */
    struct {               /* for each factor:         */
	int factor;         /*     its parameter number */
	int highest_value;  /*     its highest value    */
    } **factors;
};

extern struct c3struct_values_param * c3_values_param P((void));


struct c3struct_charset_properties {
    int encoding_unit_length;  /* number of bytes in an encoding unit */
    char *ccs_id;	       /* official C3 id */
    char *mime_name;           /* name registered for MIME use */
};

extern struct c3struct_charset_properties * 
c3_charset_properties P(( const int ));

extern char *
charset_topic P(( const int, const char *));

struct convop_properties {
	int is_finalizable;
	struct {
		int enc_units;
		int illegal_units;
		int chars;
	} src_info;
	 struct {
		int enc_units;
		int chars;
		int errors;
	} tgt_info;
};

extern struct convop_properties *
c3_latest_convop_properties P(( const int));

void
c3_initialize P(( const char *));

int
c3_create_stream P((void));

void
c3_set_iparam P(( const int, const int, const int ));

void
c3_set_sparam P(( const int, const int, const char * ));

char *
c3_bconv P((const int, char *, const char *,
	const unsigned int , int *, const unsigned int ));

char *
c3_strnconv P(( const int, char *const, const char *,
	const unsigned int, const unsigned int ));


void
c3_finalize_stream P(( const int ));

void
c3_finalize P((void));

void
c3_set_exception_handling P(( const int, const int ));
        /* The desired level of general exception handling.
         */
        /* The desired activation time for the general
         * exception handling.
         */


int
c3_outcome_code P(( char **));
        /* The message used in the general outcome handling
         * for the outcome of the previous genuine API call.
         */

char *
c3_exception_data P((void));

extern int
c3_ccs_number P(( const char *, const char *));

extern c3bool
_c3_ccs_is_available P(( int ));
#endif
