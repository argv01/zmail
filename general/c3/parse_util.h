/* $Id: parse_util.h,v 1.4 1995/07/31 23:53:53 tom Exp $ */
#ifndef __util_include__
#define __util_include__
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


#ifdef THINK_C

#include <Memory.h>
#define MEMALLOC(bytes_count) malloc((size_t) bytes_count)

#endif /* THINK_C */

#ifdef UNIX

#define MEMALLOC(bytes_count) malloc((size_t) bytes_count)

#endif /* UNIX */

#include "lib.h"

#ifndef _c3bool_defined
typedef short int c3bool;
#define _c3bool_defined
#endif

/*
 **********************************
 Approximation data
 **********************************
 */ 

struct approxseq_set {
    short alloc;
    short nextIndex;
    euseq **seq;
};

typedef struct approxseq_set *char_approximation; /* A CTYPES-long
				      * array of pointers to
				      * struct approxseq_set
				      */

struct char_plane
{
    int group_plane_num;
    char_approximation **char_appr[256]; /* Pointer to 256-long array of
        * pointers to 256 char_approximations
	*/
    /* CHAR_APPR contains all approximation sequences for the
     * character plane (a set of 256*256 characters) specified
     * by GROUP_PLANE_NUM. They are stored as a sparce matrix
     * where only the lines (256 characters) actually containing
     * approximations are allocated. For each character and
     * conversion type (amount: CTYPES) there is
     * a set of character sequences, each approximating --
     * within different character repertoires -- a certain
     * character in a certain conversion type. The order in the
     * set is of (approximately) increasing repertoire size,
     * i.e.  a search should normally start with the last
     * sequence.
     */
};

struct char_plane_set
{
    short alloc;
    short nextIndex;
    struct char_plane **plane;  /* Array of pointers to planes
				 */
};

/*
 **********************************
 Approximation file data
 **********************************
 */ 
/*
 * For storing the contents of the approximation table
 */
struct c3_app_table {
    int ctypes;			/* Number of conversion types [note 1] */
    int chars;			/* Number of approximated characters */

    struct char_plane_set *approxes;
};

/*
 **********************************
 Definition file data
 **********************************
 */ 

/*
 * One representation of a character in a
 * certain CCS
 */
struct repr_data {
    short char_priority;
    state reqStartState;
    enc_unit encUnit;
    state resultEndState;	    
};

/*
 *  The set of all representations of a character
 *  in a certain CCS
 */
struct repr_data_set {
    short alloc;
    short nextIndex;
    struct repr_data **repr;
};

typedef struct repr_data_set char_representation; 

struct repr_plane
{
    int group_plane_num;
    char_representation **char_repr[256]; /* Pointer to 256-long array of
        * pointers to 256 char_representations
	*/
    /* CHAR_REPR contains all representation data for the
     * character plane (a set of 256*256 characters) specified
     * by GROUP_PLANE_NUM. They are stored as a sparce matrix
     * where only the lines (256 characters) actually containing
     * data is allocated. For each character there is
     * a set of encoding unit sequences, each giving a
     * representation for the character. The order in the
     * set is of decreasing encoding priority, i.e.  
     * the first set has highest priority (and shall allways
     * be used when encoding this character).
     */
};

struct repr_plane_set
{
    short alloc;
    short nextIndex;
    struct repr_plane **plane;  /* Array of pointers to planes
				 */
};

/*
 * For storing the contents of a definition table
 * for one CCS
 */
struct c3_def_table {
    int num;			/* CCS number */
    int bitWidth;		/* Bits per encoding unit */
    int ctypes;			/* Number of conversion types [note 1] */
    struct char_plane_set *approxes;
    struct repr_plane_set *repres;
    int states;			/* Number of states [note 1] */
    enc_unit *transition;	/* Use index START_STATE*STATES+END_STATE
				 * to reach the encoding unit which makes
				 * the transition from START_STATE to
				 * END_STATE.
				 * (TODO Assumes there is just one way to go
				 *  from one state to another)
				 */
};

struct ccs_name_set {
    int ccs_no;
    short alloc;    
    short nextIndex;
    euseq **name;
};

/*
 * For storing the contents of a names table
 */
struct c3_names_table {
    char *system_name;
    short alloc;
    short nextIndex;
    struct ccs_names_set **names;
};

/*** ***/

struct c3_numlist {
    int length;
    int list[C3LIM_NUMLIST];
};

/* Data for mnemonics (TODO temporary solution) */

struct c3_mnemdata {
    enc_unit ucs;
    euseq *eustr;
};

typedef struct c3_mnemdata c3_mnemdata;

struct c3_mnemtabdata {
    c3_mnemdata *tab;
    c3_mnemdata *firstUnused;
    c3_mnemdata *firstUnallocated;
    c3_mnemdata *current;
};

typedef struct c3_mnemtabdata c3_mnemtabdata;

extern
struct c3_app_table *
c3_parsed_approx_table P(( char * ));

extern
struct c3_def_table *
c3_parsed_def_table P(( char *, int ));

extern
c3bool
read_tableOK P(( char *, struct ccs_data *, int, int, int, 
    int, c3bool, c3_mnemtabdata * ));

extern
c3bool
c3_start_compiling_OK P(( char * ));

extern
c3bool
c3_compile_OK P(( int, int ));

extern
c3bool
c3_end_compiling_OK P(( void ));

extern
struct encoding_plane *
enc_plane_ref P(( struct encoding_plane_set *, int, int));

/***********
 * NOTES
 *
 * [1] These values are set before handling of the
 *     first character specification line in a file and
 *     are thereafter never changed.
 */

#endif /* __util_include */
