;
%{

static char file_id[] =
	"$Id: parse.yacc,v 1.5 1995/08/16 17:18:30 tom Exp $";

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

#ifdef THINK_C
#define F_OK            0       /* does file exist */
#define X_OK            1       /* is it executable by caller */
#define W_OK            2       /* is it writable by caller */
#define R_OK            4       /* is it readable by caller */
extern int	access(/* char *path, int amode */);
#else
#include <unistd.h>
#endif

#include <stdlib.h>

#include <string.h>

extern char *
_ccs_id_from_ccs_no(const int ccs_no);

#define MAX_NO_CHARS     	550
#define MAX_UCS_EU_VALUE	0xFFFF
#define C3LIM_APP_INI_ALLOC	3
#define C3LIM_REPR_INI_ALLOC	2

#define ERROUTF stderr

#include "c3_macros.h"
#include "c3.h"
#include "euseq.h"
#include "parse_util.h"

static struct
{
    struct c3_def_table *ccsDefTable;
    struct c3_app_table *approxTable;
    struct c3_names_table *namesTable;
    struct ccs_data *ccsData;

    bool is_mnem_reconversion;
    int parse_ctype;
    char *curr_conv_syst;

    int curr_ccs_num;
    int curr_ccs_bitWidth;
    c3_mnemtabdata *mnemTabData;

    /*
     * File data
     */
    int file_type;
    bool is_deffile;
    int comment_ccs;
    int radix;

    int first_specline_passed;
    int factor_no;
    int factor_val;
    bool factor_src_correct;

    bool factor_tgt_correct;
    bool factor_val_correct;
    int ctypes;			/* Number of conv types in this file */
    int states;			/* Number of states in this file */
} gl = 
{
    NULL, NULL, NULL, NULL,
    FALSE, 0, NULL,
    -1, -1, NULL,
    -1, FALSE, 20100, 10,
    FALSE, 0, 0, FALSE,
    FALSE, FALSE, 0, 0
};

static struct c3_numlist g_numlist;

extern int opt_verbosity;

int
existing_state(
    char *statename
);

struct ccs_name_set *
new_ccs_name_set(
    euseq *e,		  
    int n_seqs_alloc
);

struct ccs_name_set *
added_ccs_name_set(
    struct ccs_name_set *a,
    euseq *e
);

struct approxseq_set *
new_approxseq_set(
    euseq *e,		  
    int n_seqs_alloc
);

struct approxseq_set *
added_approxseq_set(
    struct approxseq_set *a,
    euseq *e
);

char_approximation *
new_char_approximation(
    struct approxseq_set *a,
    int ctype,
    int ctypes
);

char_approximation *
added_char_approximation(
    char_approximation *c,
    struct approxseq_set *a,
    int ctype
);

struct repr_data_set *
new_repr_data_set(
    struct repr_data *r,
    int seqs_to_alloc
);

struct repr_data_set *
added_repr_data_set(
    struct repr_data_set *rds,
    struct repr_data *rd
);

char_approximation *
_c3_approx_access(
    struct char_plane_set *c_p_s,
    enc_unit ucs
);

void
_c3_approx_assign(
    struct char_plane_set *c_p_s,
    enc_unit ucs,
    char_approximation *c_a
);

void
_c3_repr_assign(
    struct repr_plane_set *r_p_s,
    enc_unit ucs,
    struct repr_data_set *rdt
);

bool
_c3_is_defApproxed(
    enc_unit ucs
);

%}

%union	{
	    char *str;
	    long num;
	    struct c3_numlist *numlist;
	    euseq *eus;
	    enc_unit eunit;
	    struct char_data *chardata;
	    struct approxseq_set *approxSeqSet;
            char_approximation *charApprox;
	    struct repr_data *reprdata;
	    struct repr_data_set *reprdataSet;
	    struct ccs_name_set *ccsNameSet;
	};


%token EOSTMT
%token AFILESTART, EFILESTART

%token <num> NUM

%token D_COMMENT_CCS, D_RADIX NUM, D_APPROX, D_CCS, D_FACTOR
%token D_CCS_WIDTH, D_CCS_STATE, D_FACTOR_SRC_ADD
%token D_FACTOR_TGT_ADD, D_FACTOR_SRC_RMV, D_FACTOR_TGT_RMV
%token D_FACTOR_VALUE_ADD, D_FACTOR_VALUE_RMV, D_END_OF_TABLE
%token D_CONV_TYPES, D_REVERSIBLE_TYPE, D_CONV_SYST
%token FALLBACK
%token <str> D_IDENTIFIER
%token <eunit> UCSCHARSPEC
%token <eus> LITT
%token <eus> SIGNAL
%token <eunit> EU
%token APPROX_APPROX_OP, DEF_APPROX_OP, EQUAL_OP, APPROX_INVARIANT
%token <num> CHAR_PRIO

%type <numlist> numlist
%type <approxSeqSet> one_ctype_alts
%type <approxSeqSet> def_one_ctype_alts
%type <charApprox> app_all_ctypes
%type <charApprox> def_all_ctypes
%type <eus> charseq
%type <eus> char
%type <reprdata> repr_spec
%type <reprdataSet> repr_spec_alts
%type <num> state
%type <num> charprio
%type <ccsNameSet> names
%type <eunit> app_approxed_char
%type <eunit> def_approxed_char
%type <eunit> def_equed_char

%%

file:		start lines

start:		AFILESTART EOSTMT
		| EFILESTART EOSTMT {
		    VERBOSE(VERB_MINI, "E-type files not supported%c",
			    "start-parse", ' ')
		    _c3_register_outcome("start-parse", C3E_INTERNAL);
		    exit(1);
		}
		;

lines:		line
		| lines EOSTMT line
		;

line:		directline
		| app_approxline
		| def_approxline
		| chareqline
		| names_line
    		|
		;

names_line:	NUM names
		{
		    $2->ccs_no = $1;
		    add_ccs_names(gl.namesTable, $2);
		}


names:		LITT
		{
		    $$ = new_ccs_name_set($1, C3LIM_APP_INI_ALLOC);
		}
		| names LITT
		{
		    $$ = added_ccs_name_set($1, $2);
		}
		;


directline:	D_COMMENT_CCS NUM
		{
		    gl.comment_ccs = $2;
		}
		| D_RADIX NUM
		{
		    gl.radix = $2;
		}
		| D_APPROX
		{
		    if (gl.file_type != C3LIB_FILE_TYPE_APPROXTAB)
		    {
			semerror("Unexpectingly found a approximation file");
		    }
		}
		| D_CCS numlist
		{
		    if (gl.file_type != C3LIB_FILE_TYPE_SRC_DEFTAB
			&& gl.file_type != C3LIB_FILE_TYPE_TGT_DEFTAB)
		    {
			semerror("Unexpectingly found a definition table");
		    }
		    else
		    {
			if ($2->length > 1)
		    	{
			    semerror(
			    "CCS for more than one ccs not yet implemented");
			}
			if (gl.is_deffile && $2->list[0] != gl.curr_ccs_num)
			{
			    semerror("Wrong CCS line found in deftab");
			}
		    }
		}
		| D_FACTOR NUM
		{
		    if (gl.file_type != C3LIB_FILE_TYPE_FACTORTAB)
		    {
			semerror("Unexpectingly found a factor file");
		    }
		    else if (gl.factor_no != $2)
		    {
			semerror("Found wrong factor file");
		    }
		    else
		    {
			gl.factor_src_correct = FALSE;
			gl.factor_tgt_correct = FALSE;
			gl.factor_val_correct = FALSE;
		    }
		}
		| D_CCS_WIDTH NUM
		{
		    if (! gl.is_deffile)
		    {
			semerror("Unexpectingly found CCS_WIDTH");
		    }
		    else
		    {
			switch($2) {
			case 7:
			case 8:
			case 14:
			case 16:
			case 32:
			    break;
			default:
			    {
				semerror("Illegal CCS_WIDTH value");
			    }
			    break;
			}
			gl.curr_ccs_bitWidth = $2;
		    }
		}
		| D_CCS_STATE numlist statenamelist NUM
		{
		    if (! gl.is_deffile)
		    {
			semerror("Unexpectingly found CCS_STATE");
		    }
		    else
		    {
			semerror("CCS_STATE not yet implemented");
		    }
		}
		| D_CCS_STATE numlist statenamelist
		{
		    if (! gl.is_deffile)
		    {
			semerror("Unexpectingly found CCS_STATE");
		    }
		    else
		    {
			semerror("CCS_STATE not yet implemented");
		    }
		}
		| D_FACTOR_SRC_ADD numlist
		{
/* TODO Must know both SRC and TRG data? */
		    if(!gl.factor_src_correct
		       && in_numlist(gl.ccsDefTable->num, $2))
		    {
		        gl.factor_src_correct = TRUE;
		    }
		}
		| D_FACTOR_TGT_ADD numlist
		{
		    if(!gl.factor_tgt_correct
		       && in_numlist(gl.ccsDefTable->num, $2))
		    {
		        gl.factor_tgt_correct = TRUE;
		    }
		}
		| D_FACTOR_SRC_RMV numlist
		{
		    if(gl.factor_src_correct
		       && in_numlist(gl.ccsDefTable->num, $2))
		    {
		        gl.factor_src_correct = FALSE;
		    }
		}
		| D_FACTOR_TGT_RMV numlist
		{
		    if(gl.factor_tgt_correct
		       && in_numlist(gl.ccsDefTable->num, $2))
		    {
		        gl.factor_tgt_correct = FALSE;
		    }
		}
		| D_FACTOR_VALUE_ADD numlist
		{
		    if(!gl.factor_val_correct
		       && in_numlist(gl.factor_val, $2))
		    {
		        gl.factor_val_correct = TRUE;
		    }
		}
		| D_FACTOR_VALUE_RMV numlist
		{
		    if(gl.factor_val_correct
		       && in_numlist(gl.factor_val, $2))
		    {
		        gl.factor_val_correct = FALSE;
		    }
		}
		| D_END_OF_TABLE
		| D_CONV_TYPES NUM
		{
		    if (gl.first_specline_passed)
		    if ($2 <= 0)
		    {
			semerror("CONV_TYPES can't be changed after first specification line");
		    }
		    if ($2 <= 0)
		    {
			semerror("CONV_TYPES value must be > 0");
		    }
		    gl.ctypes = $2;
		}
		| D_REVERSIBLE_TYPE NUM
		| D_CONV_SYST D_IDENTIFIER
		{
		    if (strcmp($2, gl.curr_conv_syst) != 0)
		    {
			semerror("Wrong CONV_SYST line found");
		    }
		    free($2);
		}
		;

numlist:	NUM
		{
		    g_numlist.length++;
		    if (g_numlist.length >= C3LIM_NUMLIST)
		    {
			semerror("numlist overflow");
		    }
		    g_numlist.list[g_numlist.length - 1] = (int) $1;
		    $$ = &g_numlist;
		}
		| numlist NUM
		{
		    g_numlist.length++;
		    if (g_numlist.length >= C3LIM_NUMLIST)
		    {
			semerror("numlist overflow");
		    }
		    g_numlist.list[g_numlist.length - 1] = (int) $2;
		    $$ = &g_numlist;
		}
		;

statenamelist:  D_IDENTIFIER
		| statenamelist D_IDENTIFIER
		;

charseq:	char
		| charseq char
		{
		    if (euseq_append_OK($1, $2))
		    {
			free($2->storage);
			free($2);
			$$ = $1;
		    }
		    else
		    {
			VERBOSE(VERB_MINI, "euseq_append error%c",
				"charseq-parse", ' ')
			_c3_register_outcome("charseq-parse", 
					     C3E_INTERNAL);
			exit(2);
		    }
		}
		;

char:		UCSCHARSPEC
		{
    		    $$ = euseq_from_dupstr(" ", 2, -1);
		    euseq_setepos($$, -1);
		    euseq_puteu($$, $1);
		}
		| FALLBACK
		{
    		    $$ = euseq_from_dupstr("_", 2, -1);
		}
		| LITT
		| SIGNAL
		;

/* *** */

app_approxline:	app_approxed_char APPROX_APPROX_OP app_all_ctypes
		{
		    _c3_approx_assign(gl.approxTable->approxes, $1, $3);
		    gl.approxTable->chars++;
		}
		| app_approxed_char APPROX_APPROX_OP APPROX_INVARIANT
		;

app_approxed_char:	UCSCHARSPEC
		{
		    if ( ! gl.first_specline_passed) first_specline();
		    if (gl.file_type != C3LIB_FILE_TYPE_APPROXTAB)
		    {
			semerror("Unexpectingly found approx file line");
		    }
		    $$ = $1;
		}
		;

app_all_ctypes:	one_ctype_alts
		{
		    gl.parse_ctype = 1;
		    $$ = new_char_approximation($1, gl.parse_ctype, gl.ctypes);
		}
		| app_all_ctypes typesep one_ctype_alts
		{
		    $$ = added_char_approximation($1, $3, gl.parse_ctype);
		    if ($$ == NULL)
		    {
			VERBOSE(VERB_MINI, "added_char_approximation error%c",
				"app_all_ctypes-parse", ' ')
			exit(2);
		    }
		}
		;

typesep:	'/'
		{
		    if (gl.parse_ctype >= gl.ctypes)
		    {
			semerror("Too many conversion types");
		    }
		    gl.parse_ctype++;
		}
		;

one_ctype_alts:	charseq
		{
		    $$ = new_approxseq_set($1, C3LIM_APP_INI_ALLOC);
		}
		| one_ctype_alts altsep charseq
		{
		    $$ = added_approxseq_set($1, $3);
		    if ($$ == NULL)
		    {
			VERBOSE(VERB_MINI, "added_approxseq_set error%c",
				"one_ctype_alts-parse", ' ')
			exit(2);
		    }
		}
		;

altsep:		';'
		;

/* *** */

def_approxline:	def_approxed_char DEF_APPROX_OP def_all_ctypes
		{
		    _c3_approx_assign(gl.ccsDefTable->approxes, $1, $3);
		}
		;

def_approxed_char:	UCSCHARSPEC
		{
		    if ( ! gl.first_specline_passed) first_specline();
		    if (gl.is_deffile)
		    {
			if (_c3_approx_access(gl.ccsDefTable->approxes, $1)
			    != NULL)
			{
			    semerror("Double spec for character");
		    	}
		    }
		    else
		    {
			semerror("Unexpectingly found def file line");
		    }
		}
		;

def_all_ctypes:	def_one_ctype_alts
		{
		    gl.parse_ctype = 1;
		    $$ = new_char_approximation($1, gl.parse_ctype, gl.ctypes);
		}
		| def_all_ctypes typesep def_one_ctype_alts
		{
		    $$ = added_char_approximation($1, $3, gl.parse_ctype);
		}
		;


def_one_ctype_alts:	charseq
		{
		    $$ = new_approxseq_set($1, C3LIM_APP_INI_ALLOC);
		}
		| def_one_ctype_alts altsep charseq
		{
		    $$ = added_approxseq_set($1, $3);
		}
		;


/* *** */

chareqline:	def_equed_char EQUAL_OP repr_spec_alts
		{
		    _c3_repr_assign(gl.ccsDefTable->repres, $1, $3);
		    /* enc_spec_is($1, $3);  $3 is the first alt */
		}
		;

def_equed_char: UCSCHARSPEC
		{
		    if ( ! gl.first_specline_passed) first_specline();
		    if (! gl.is_deffile
		        && gl.file_type != C3LIB_FILE_TYPE_FACTORTAB)
		    {
			semerror("Unexpectingly found equality line");
		    }
		}
		;

repr_spec_alts:	repr_spec
		{
		    $$ = new_repr_data_set($1, C3LIM_REPR_INI_ALLOC);
		}
		| repr_spec_alts altsep repr_spec
		{
		    $$ = added_repr_data_set($1, $3);
		}
		;

repr_spec:	charprio state EU state
		{
		    SAFE_ZALLOCATE($<reprdata>$,
		    (struct repr_data *),
		    sizeof(struct repr_data),
		    exit(2), "repr_spec");
		    $<reprdata>$->char_priority = $1;
		    $<reprdata>$->reqStartState = $2;
		    $<reprdata>$->encUnit = $3;
		    $<reprdata>$->resultEndState = $4;
		}
		;

charprio:	CHAR_PRIO
		|
		{
		    $$=0;	/* Default zero */
		}
		;

state:		D_IDENTIFIER
		{
		    int i1;

		    if ((i1 = existing_state($1)) >= 0)
		    {
			$$ = i1;
		    }
		    else
		    {
			semerror("non-existing state");
		    }
		}
		|
		{
		    $$ = 0;	/* Default zero */
		}
		;

%%

/*
*/

#include "lex.yy.c"



/*
 * Error in the lexical analysis
 */
void
llerror(s, t)
char *s, *t;
{
    fprintf(ERROUTF, "Lexcal error in line %d (%s%s)\n", yylineno, s, t);
    exit(1);
}

/*
 * Error in the syntactical analysis
 */
void
yyerror(s)
char *s;
{
    fprintf(ERROUTF, "Syntactical error in line %d: %s\n", yylineno, s); 
    exit(1);
}

/*
 * Error in the semantic analysis
 */
void
semerror(s)
char *s;
{
    fprintf(ERROUTF, "Semantic error in line %d: %s\n", yylineno, s); 
    exit(1);
}

bool
in_numlist(
    int val,
    struct c3_numlist *nlist
)
{
    int *ip, i1;
    
    for (ip = nlist->list, i1 = 0; i1 <= nlist->length; ip++, i1++)
    {
	if (*ip == val)
	{
	    return(TRUE);
	}
    }
    return(FALSE);
}
    
#undef CURRFUNC
#define CURRFUNC "existing_state"

int
existing_state(
    char *statename
)
{
    /* TODO */
    return(TRUE);
}

int
yywrap()
{
    return 1;
}

#undef CURRFUNC
#define CURRFUNC "add_ccs_names"

void
add_ccs_names(
    struct c3_names_table *nTable,
    struct ccs_name_set *nSet
)
{
}

#undef CURRFUNC
#define CURRFUNC "new_ccs_name_set"

struct ccs_name_set *
new_ccs_name_set(
    euseq *e,		  
    int n_seqs_alloc
)
{
    struct ccs_name_set *res;
    int i1;

    SAFE_ZALLOCATE(res, (struct ccs_name_set *),
		   1 * sizeof(struct ccs_name_set),
		   return(NULL), CURRFUNC);
    res->alloc = n_seqs_alloc;
    SAFE_ZALLOCATE(res->name, (euseq **),
		   res->alloc * sizeof(euseq *),
		   return(NULL), CURRFUNC);
    for (i1 = 0; i1 < res->alloc; i1++)
    {
	res->name[i1] = NULL;
    }
    res->name[0] = e;
    res->nextIndex = 1;
    return(res);
}

#undef CURRFUNC
#define CURRFUNC "added_ccs_name_set"

struct ccs_name_set *
added_ccs_name_set(
    struct ccs_name_set *a,
    euseq *e
)
{
    if (a->nextIndex >= a->alloc)
    {
	int new_alloc = a->alloc + 2, i1;

	VERBOSE(VERB_DEB2, "reallocating to %d", CURRFUNC, new_alloc)
	SAFE_REALLOCATE(a->name, (euseq **),
			new_alloc, return(NULL), CURRFUNC)
	for (i1 = a->alloc; i1 < new_alloc; i1++)
	{
	    a->name[i1] = NULL;
	}
	a->alloc = new_alloc;
    }
    a->name[a->nextIndex] = e;
    a->nextIndex++;
    return(a);
}

#undef CURRFUNC
#define CURRFUNC "new_approxseq_set"

struct approxseq_set *
new_approxseq_set(
    euseq *e,		  
    int n_seqs_alloc
)
{
    struct approxseq_set *res;
    int i1;

    SAFE_ZALLOCATE(res, (struct approxseq_set *),
		   1 * sizeof(struct approxseq_set),
		   return(NULL), CURRFUNC);
    res->alloc = n_seqs_alloc;
    SAFE_ZALLOCATE(res->seq, (euseq **),
		   res->alloc * sizeof(euseq *),
		   return(NULL), CURRFUNC);
    for (i1 = 0; i1 < res->alloc; i1++)
    {
	res->seq[i1] = NULL;
    }
    res->seq[0] = e;
    res->nextIndex = 1;
    return(res);
}

#undef CURRFUNC
#define CURRFUNC "added_approxseq_set"

struct approxseq_set *
added_approxseq_set(
    struct approxseq_set *a,
    euseq *e
)
{
    if (a->nextIndex >= a->alloc)
    {
	int new_alloc = a->alloc + 2, i1;

	VERBOSE(VERB_DEB2, "reallocating to %d", CURRFUNC, new_alloc)
	SAFE_REALLOCATE(a->seq, (euseq **),
			new_alloc, return(NULL), CURRFUNC)
	for (i1 = a->alloc; i1 < new_alloc; i1++)
	{
	    a->seq[i1] = NULL;
	}
	a->alloc = new_alloc;
    }
    a->seq[a->nextIndex] = e;
    a->nextIndex++;
    return(a);
}

#undef CURRFUNC
#define CURRFUNC "new_char_approximation"

char_approximation *
new_char_approximation(
    struct approxseq_set *a,
    int ctype,
    int ctypes
)
{
    char_approximation *res;
    int i1;

    SAFE_ZALLOCATE(res, (char_approximation *),
		   ctypes * sizeof(char_approximation),
		   return(NULL), CURRFUNC);
    for (i1 = 0; i1 < ctypes; i1++)
    {
	res[i1] = NULL;
    }
    res[ctype-1] = a;
    return(res);
}

char_approximation *
added_char_approximation(
    char_approximation *c,
    struct approxseq_set *a,
    int ctype
)
{
    if (c[ctype - 1] != NULL)
    {
	VERBOSE(VERB_MINI, "Internal error, non-NULL for ctype %d",
		CURRFUNC, ctype)
	exit(2);
    }
    c[ctype - 1] = a;
}

struct repr_data_set *
new_repr_data_set(
    struct repr_data *r,
    int seqs_to_alloc
)
{
    struct repr_data_set *res;

    SAFE_ZALLOCATE(res, (struct repr_data_set *),
		   1 * sizeof(struct repr_data_set),
		   exit(2), CURRFUNC);
    res->alloc = seqs_to_alloc;
    SAFE_ZALLOCATE(res->repr, (struct repr_data **),
		   res->alloc * sizeof(struct repr_data *),
		   exit(2), CURRFUNC);
    res->repr[0] = r;
    res->nextIndex = 1;
    return(res);
}


struct repr_data_set *
added_repr_data_set(
    struct repr_data_set *rds,
    struct repr_data *rd
)
{
    if (rds->nextIndex >= rds->alloc)
    {
	semerror("repr_data_set overflow");
    }
    rds->repr[rds->nextIndex] = rd;
    rds->nextIndex++;
    return(rds);
}


bool
_c3_is_defApproxed(
    enc_unit ucs
)
{
    return(TRUE
	   /* gl.ccsDefTable->ctypetab[0] != NULL
	   && gl.ccsDefTable->ctypetab[0].approxtabline[ucs / 256] != NULL
	   && gl.ccsDefTable->ctypetab[0].approxtabline[ucs / 256][ucs % 256]
	   != NULL */
	   );
}

#undef CURRFUNC
#define CURRFUNC "reset_globals"

void
reset_globals(
    int file_type
)
{
    gl.first_specline_passed = FALSE;
    yylineno = 0;
    gl.ctypes = 0;
    gl.states = 1;

    switch(file_type)
    {
    case C3LIB_FILE_TYPE_SRC_DEFTAB:
	break;

    case C3LIB_FILE_TYPE_TGT_DEFTAB:
	break;

    case C3LIB_FILE_TYPE_APPROXTAB:
	break;

    case C3LIB_FILE_TYPE_FACTORTAB:
        break;

    default:
	break;
    }
}

#undef CURRFUNC
#define CURRFUNC "first_specline"

void
first_specline(
    void
)
{
    VERBOSE(VERB_DEB1, "Entering %c", CURRFUNC, ' ')

    gl.first_specline_passed = TRUE;

    switch(gl.file_type)
    {
    case C3LIB_FILE_TYPE_SRC_DEFTAB:
    case C3LIB_FILE_TYPE_TGT_DEFTAB:

	VERBOSE(VERB_DEB1, "Initiating deftab %c", CURRFUNC, ' ')
	if (gl.ctypes == 0)
	{
	    gl.ctypes = 3;	/* Assuming 3 for backwards compatibility */
	}
	SAFE_ZALLOCATE(gl.ccsDefTable, (struct c3_def_table *),
		       sizeof(struct c3_def_table),
		       exit(2), CURRFUNC);
	gl.ccsDefTable->ctypes = gl.ctypes; /* TODO Wrong? */
	gl.ccsDefTable->states = gl.states;
	gl.ccsDefTable->num = gl.curr_ccs_num;
	gl.ccsDefTable->bitWidth = gl.curr_ccs_bitWidth;
	SAFE_ZALLOCATE(gl.ccsDefTable->approxes,
		       (struct char_plane_set *),
		       1 * sizeof(struct char_plane_set),
		       return, CURRFUNC);
	gl.ccsDefTable->approxes->plane = NULL;
	SAFE_ZALLOCATE(gl.ccsDefTable->repres,
		       (struct repr_plane_set *),
		       1 * sizeof(struct repr_plane_set),
		       return, CURRFUNC);
	gl.ccsDefTable->repres->plane = NULL;
	SAFE_ZALLOCATE(gl.ccsDefTable->transition,
		       (enc_unit *),
		       gl.ccsDefTable->states * sizeof(enc_unit),
		       exit(2), CURRFUNC);
	break;

    case C3LIB_FILE_TYPE_APPROXTAB:
	VERBOSE(VERB_DEB1, "Initiating approx %c",CURRFUNC, ' ')
	if (gl.ctypes == 0)
	{
	    gl.ctypes = 3;	/* Assuming 3 for backwards compatibility */
	}
	SAFE_ZALLOCATE(gl.approxTable, (struct c3_app_table *),
		       sizeof(struct c3_app_table),
		       exit(2), CURRFUNC);
	gl.approxTable->ctypes = gl.ctypes;
	gl.approxTable->chars = 0;
	SAFE_ZALLOCATE(gl.approxTable->approxes, (struct char_plane_set *),
		       sizeof(struct char_plane_set),
		       exit(2), CURRFUNC);
	break;

    case C3LIB_FILE_TYPE_FACTORTAB:
        break;

    default:
	break;
    }
}

#undef CURRFUNC
#define CURRFUNC "mnemsortcmp"

int
mnemsortcmp(
    const void *a,
    const void *b
)
{
    return(euseq_cmp(((c3_mnemdata *) a)->eustr, ((c3_mnemdata *) b)->eustr));
}


#undef CURRFUNC
#define CURRFUNC "read_tableOK"

bool
read_tableOK(
    char *conv_syst,	/* Conversion system */
    struct ccs_data *ccsData,
    int ctype,
    int file_type,
    int factor_no,
    int factor_val,
    bool is_mnem_reconversion,
    c3_mnemtabdata *mtd
)
{
    gl.file_type = file_type;
    gl.is_deffile =
	(gl.file_type == C3LIB_FILE_TYPE_SRC_DEFTAB
	 || gl.file_type == C3LIB_FILE_TYPE_TGT_DEFTAB);
    gl.ccsData = ccsData;
    gl.is_mnem_reconversion = is_mnem_reconversion;
    gl.mnemTabData = mtd;
    gl.curr_conv_syst = conv_syst;
    
    reset_globals(file_type);

    switch(file_type)
    {
    case C3LIB_FILE_TYPE_SRC_DEFTAB:
    case C3LIB_FILE_TYPE_TGT_DEFTAB:
	fh_in = fh_c3_open_hdl(FH_TYPE_DEF, FH_OPEN_READ, conv_syst,
			       ccsData->num);
	if (fh_in < 0)
	{
	    VERBOSE(VERB_MINI, "Couldn't open ccs table%c", CURRFUNC, ' ')
	    _c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	    return(FALSE);
	}
	break;

    case C3LIB_FILE_TYPE_APPROXTAB:
	fh_in = fh_c3_open_hdl(FH_TYPE_APPROX, FH_OPEN_READ, conv_syst, 0);
	if (fh_in < 0)
	{
	    VERBOSE(VERB_MINI, "Couldn't open approx table%c", CURRFUNC, ' ')
	    _c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	    return(FALSE);
	}
	break;

    case C3LIB_FILE_TYPE_FACTORTAB:
#ifdef TEMP_BORT
	fh_in = fh_c3_factor_file_hdl(conv_syst, factor_no, FH_OPEN_READ);
#endif
	if (fh_in < 0)
	{
	    VERBOSE(VERB_MINI, "Couldn't open factor table%c", CURRFUNC, ' ')
	    _c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	    return(FALSE);
	}
	gl.factor_no = factor_no;
	gl.factor_val = factor_val;
        break;

    default:
	VERBOSE(VERB_MINI, "Illegal file_type %d", CURRFUNC, file_type)
	_c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	break;
    }

    yyparse();

    fh_close_OK(fh_in);
	
    switch(file_type)
    {
    case C3LIB_FILE_TYPE_APPROXTAB:
	if (gl.is_mnem_reconversion)
	{

/*	    fill_in_mnemtab(ccsData, mtd);
	    qsort((char *) mtd->tab,
		  (mtd->firstUnused - mtd->tab),
		  sizeof(c3_mnemdata),
		  mnemsortcmp);
*/
	}

	break;

    }


    return (TRUE);
}

extern
struct char_plane *
approx_plane_ref(
    struct char_plane_set *apps,
    int group_plane_num,
    bool create_new
);

#undef CURRFUNC
#define CURRFUNC "_c3_approx_access"

char_approximation *
_c3_approx_access(
    struct char_plane_set *c_p_s,
    enc_unit ucs
)
{
    struct char_plane *c_pl;

    c_pl = approx_plane_ref(c_p_s, ucs >> 16, TRUE);

    if (c_pl == NULL || c_pl->char_appr[ucs / 256] == NULL)
    {
	return(NULL);
    }
    else
    {
	return(c_pl->char_appr[ucs / 256][ucs % 256]);
    }
}

#undef CURRFUNC
#define CURRFUNC "_c3_approx_assign"

void
_c3_approx_assign(
    struct char_plane_set *c_p_s,
    enc_unit ucs,
    char_approximation *c_a
)
{
    struct char_plane *c_pl;

    c_pl = approx_plane_ref(c_p_s, ucs >> 16, TRUE);

    if (c_pl->char_appr[ucs / 256] == NULL)
    {
	int i1;
	char_approximation **t_c_a;

	SAFE_ALLOCATE(t_c_a, (char_approximation **),
		      256 * sizeof(char_approximation *),
		      return, CURRFUNC);
	for (i1 = 0; i1 < 256; i1++)
	{
	    t_c_a[i1] = NULL;
	}
	c_pl->char_appr[ucs / 256] = t_c_a;
    }
    if (c_pl->char_appr[ucs / 256][ucs % 256] != NULL)
    {
	VERBOSE(VERB_MINI, "Double app_assing, ucs %x", CURRFUNC, ucs)
    }
    c_pl->char_appr[ucs / 256][ucs % 256] = c_a;
}

struct repr_plane *
repr_plane_ref(
    struct repr_plane_set *reprs,
    int group_plane_num,
    bool create_new
);

#undef CURRFUNC
#define CURRFUNC "_c3_repr_assign"

void
_c3_repr_assign(
    struct repr_plane_set *r_p_s,
    enc_unit ucs,
    struct repr_data_set *rdt
)
{
    struct repr_plane *r_pl;

    r_pl = repr_plane_ref(r_p_s, ucs >> 16, TRUE);

    if (r_pl->char_repr[ucs / 256] == NULL)
    {
	int i1;
	char_representation **t_c_r;

	SAFE_ALLOCATE(t_c_r, (char_representation **),
		      256 * sizeof(char_representation *),
		      return, CURRFUNC);
	for (i1 = 0; i1 < 256; i1++)
	{
	    t_c_r[i1] = NULL;
	}
	r_pl->char_repr[ucs / 256] = t_c_r;
    }
    r_pl->char_repr[ucs / 256][ucs % 256] = rdt;
}

#undef CURRFUNC
#define CURRFUNC "c3_parsed_approx_table"

struct c3_app_table *
c3_parsed_approx_table(
    char *conv_syst
)
{
    gl.file_type = C3LIB_FILE_TYPE_APPROXTAB;
    gl.curr_conv_syst = conv_syst;
    
    reset_globals(gl.file_type);

    fh_in = fh_c3_open_hdl(FH_TYPE_APPROX, FH_OPEN_READ, conv_syst, 0);
    if (fh_in < 0)
    {
	VERBOSE(VERB_MINI, "Couldn't open approx table%c", CURRFUNC, ' ')
	_c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	return(FALSE);
    }

    yyparse();

    fh_close_OK(fh_in);
	
    if (gl.is_mnem_reconversion)
    {
/*	fill_in_mnemtab(ccsData, mtd);
	qsort((char *) mtd->tab,
	      (mtd->firstUnused - mtd->tab),
	      sizeof(c3_mnemdata),
	      mnemsortcmp);
*/
    }

    return (gl.approxTable);
}

#undef CURRFUNC
#define CURRFUNC "c3_parsed_def_table"

struct c3_def_table *
c3_parsed_def_table(
    char *conv_syst,
    int ccs_num
)
{
    gl.file_type = C3LIB_FILE_TYPE_SRC_DEFTAB;
    gl.is_deffile = TRUE;
    gl.curr_conv_syst = conv_syst;
    gl.curr_ccs_num = ccs_num;
    
    reset_globals(gl.file_type);

    fh_in = fh_c3_open_hdl(FH_TYPE_DEF, FH_OPEN_READ, conv_syst, ccs_num);
    if (fh_in < 0)
    {
	VERBOSE(VERB_MINI, "Couldn't open def table for %d", CURRFUNC, ccs_num)
	_c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	return(FALSE);
    }

    yyparse();

    fh_close_OK(fh_in);
	
    return (gl.ccsDefTable);

}
