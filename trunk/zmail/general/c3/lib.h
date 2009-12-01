/* $Id: lib.h,v 1.8 1995/08/03 01:05:21 tom Exp $ */
#ifndef __c3_lib_included__
#define __c3_lib_included__

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


#include "c3.h"
#include "euseq.h"

#define C3_CONV_TABLE_DIR "."

#define C3_MAXCONVTYPES	3

#define C3LIB_LB_CR	0
#define C3LIB_LB_LF	1
#define C3LIB_LB_CRLF	2
#define C3LIB_LB_LFCR	3

#define C3LIB_FILE_TYPE_SRC_DEFTAB	0
#define C3LIB_FILE_TYPE_TGT_DEFTAB	1
#define C3LIB_FILE_TYPE_APPROXTAB	2
#define C3LIB_FILE_TYPE_FACTORTAB	3

#define C3INT_CONV_EXACT	 1
#define C3INT_CONV_APPROX	-1
#define C3INT_CONV_NOT		 0

#define C3INT_STATE_INI		 0

#define C3LIM_NUMLIST		32
#define C3LIM_APPROXSEQS	5

#ifndef _c3bool_defined
typedef short int c3bool;
#define _c3bool_defined
#endif

typedef short state;

struct ccs_no_id_mime
{
    int num;
    char *id;
    char *mime_name;
};

/*
 **********************************
 In-memory CCS data
 **********************************
 */ 

/*
 *  Effect of reading a certain encoding unit while in a certain
 *  state - a resulting character (or UCS2_NOT_A_CHAR) and state
 */
struct parse_data {
    enc_unit ucs;
    state nextState;
};

/*
 *  Collection of reading effects for a certain encoding unit and 
 *  state combination - one effect for each possible resulting
 *  character (normally only one)
 *  
 */
struct parse_data_set {
    short alloc;
    struct parse_data **pdata;	/* Index: char_priority, */
};				/*        max max_char_priority
				 *        (some can be NULL)
				 */

/*
 *  Data for encodding of a given character - conversion
 *  quality and one encoding unit sequence for each
 *  conversion type.
 */
struct encoding_data {
    short conv_quality;		/* C3INT_CONV_EXACT, ..._APPROX */
    euseq **reprstr;		/* A CTYPES-long array of pointers to
				 * euseqs
				 */
};

typedef struct encoding_data char_encoding;

struct encoding_plane
{
    int group_plane_num;
    char_encoding **char_encod[256]; /* Pointer to 256-long array of
        * pointers to 256 char_encodings
	*/
};

struct encoding_plane_set
{
    short alloc;
    short nextIndex;
    struct encoding_plane **plane;  /* Array of pointers to planes
				     */
};


/*
 * Data for one CCS, computed
 * from definition and approximation tables:
 * (TODO just one ctype?)
 */
struct ccs_data {
    int num;			/* CCS number */
    int bitWidth;		/* Bits per encoding unit */
    int byteWidth;		/* Bytes per encoding unit */
    int ctypes;			/* Conversion types */
    int eunitStore;		/* Size of storage for all */
				/* encoding units */
    int states;			/* Number of states */
    int characters;		/* Number of specified characters */
/* TODO    int max_char_prio;		 Maximum value of char_prio */
    /*
     * Help data
     */
    euseq **other_to_ini_state;	/* Strings to go from a state to ini state.
				 * Index: OTHER_STATE, max STATES - 1
				 * Length: STATES - 1 (index 0 not used)
				 */
    /*
     * Reading effects for all encoding units
     * in all states: 
     */
    struct parse_data_set **parse; /* Index:  STATE*EUNITSTORE+EUNIT,
				    *         max (STATES * EUNITSTORE) - 1
				    * Length: STATES * EUNITSTORE
				    */
    /*
     * Encoding data, sparce matrixes
     */
    struct encoding_plane_set *encodings; /* (Just a pointer)
					   */
    struct ccs_data *next;	/* Point to the next allocated block */
};

/*
 * Data for a certain value of a factor parameter
 */
struct factor_data {
    int parameter;		/* Specifies which factor */
    int value;			/* ...and chosen value */
    /*
     * Encoding data
     */
    struct encoding_plane_set *encodings;
    /*
     * Reading effects for all encoding units
     * in all states: 
     */
    struct parse_data_set **parse; /* Use index STATE*STATES+EUNIT */
};

struct c3_stream {
    /*
     * The parameters.
     */
    int src_num;		/* A2 */
    int tgt_num;		/* A3 */

    struct ccs_data	*src_ccs;
    struct ccs_data	*tgt_ccs;

    int    ctype;		/* A4 */
    struct {			/* A5 */
	int  amount;
	int types[C3IL_MAX_SRC_LB];
    } src_lb;
    int    tgt_lb;		/* A6 */
    int    direction;		/* B1 */
    euseq  *signal_in_tgt;	/* B2 */
    euseq  *signal_in_src;
    euseq  *signal_subst_in_tgt; /* B3 */
    int    select;		/* C1 */
    struct {			/* C2 */
	int type;
	euseq *pre;
	euseq *post;
    } approx;
    struct {			/* C3 */
	int tgt_effect;
	char *before;
	char *betw;
	char *after;
    } data_err;
    struct {
	char *name;
	int reqValue;
    } **factors;
    /*
     * State data
     */
    int current_state;
    int impl_conv_mode;
    /*
     * Working data for this specific conversion, computed
     * through combination of the two ccs_data structs.
     */
    c3bool data_complete;
    int max_outstr_length;
    int states;			/* Number of states */
    struct wtableinfo {
	short conv_quality;
	state resultState;
	euseq *seq;
    } *wtable;
    /*	c3_mnemtabdata *mnemTabData; */
};

typedef struct c3_stream c3_stream;

struct bio_ccs_pos_spec
{
    int ccs_no;
    int filpos;
};

struct bio_meta_data
{
    int format_version;
    int meta_data_storage;
    char *conv_system;
    char *iso_time;
    char *comment;
    int n_ccs;
    int ccs_spec_alloc;
    struct bio_ccs_pos_spec **ccs_spec;
    struct ccs_data **ccs_d;
};


void
_c3_debug_inform P(( const char *, const int, const char * ));

c3bool
_c3_update_convinfo_OK P(( char *, c3_stream * ));

void
_c3_register_outcome P(( const char *, const int ));

struct char_data **
_c3_chardata_ref P(( struct ccs_data *, enc_unit ));

euseq *
_c3_find_approx_eunitseq P(( struct encoding_data *,
    struct ccs_data *, int ));

extern
char *
_ccs_id_from_ccs_num P(( const int ));

extern
int
_ccs_num_from_ccs_id P(( const char * ));

extern
char *
_c3_curr_conv_syst_name P(( void ));

extern
int
_ccs_no_from_filename P(( char * ));

/* TODO Temporarily... gets tables in memory
 */

struct cdtabs {
    short alloc;
    short nextIndex;
    struct ccs_data **ccsDataTab;
};

extern
struct cdtabs *
_c3_cdtabs P(( void ));

/***********
 * NOTES
 *
 * [1] These values are set before handling of the
 *     first character specification line in a file and
 *     is thereafter never changed.
 */

char *
_ccs_mimename_from_ccs_no P(( const int));

#endif /* __c3_lib_included__ */
