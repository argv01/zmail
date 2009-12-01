
static char file_id[]="$Id: lib.c,v 1.20 1995/08/16 17:18:27 tom Exp $";

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
#include <stdlib.h>

#include "c3_string.h"
#include "c3_macros.h"
#include "c3.h"
#include "euseq.h"
#include "lib.h"
#include "parse_util.h"
#ifndef NOT_ZMAIL
#include "catalog.h"
#endif

#include "fh.h"

#define FREE_EUSEQ(euseq) \
    if ((euseq) != NULL) {free((euseq)->storage); \
			  free((euseq)); \
			  (euseq) = NULL;}


#define C3_NYI -1


/*
 *   getbit - Get the bit value of a certain bit in a bit array
 *
 *   bit_arr		pointer to storage area
 *   bitno		bit number in the array, bit 0 being the
 *			most significant bit of the first byte
 */
int
getbit(bit_arr, bitno)
       unsigned char *bit_arr;
       int bitno;
{
    return((((int) bit_arr[bitno / 8]) >> (7 - (bitno % 8)) & 1));
}


int opt_verbosity = VERB_NONE;

/*  *** Outcome handling *** *** *** *** *** *** *** *** *** */

static char *warnmess_en[] =
{
    "",
    "Conversion of last character not completed.",	/* C3W_CONVNOTFINAL */
    "Not all conversion parameters initialized.",	/* C3W_CPARNOTINIT */
    "A conversion system did already exist.",		/* C3W_CSYSTNOTFIN */
    "No conversion system was initialized.", 		/* C3W_CSYSTNOTINIT */
    "Non-existing or empty source string.",		/* C3W_EMPTYSRCSTR */
    "Last byte(s) cannot be interpreted.",		/* C3W_INVALEOS */
    "Not all master parameters were initialized.",	/* C3W_MPARNOTINIT */
    "Source character set and target character set the same.",	/* C3W_SAMECCS */
};

static char *errmess_en[] =
{
    "",
    "No information about indicated character set available.", 	/* C3E_CCSNOINFO */
    "Not all conversion parameters initialized.",	/* C3E_CPARNOTINIT */
    "Ill-formed parameter value.",			/* C3E_CPARVALBAD */
    "No conversion system was initialized.",		/* C3E_CSYSTNOTINIT */
    "Not enough memory for this conversion system.",	/* C3E_CSYSTSTARVE */
    "Internal error.",					/* C3E_INTERNAL */
    "Unsupported activation time value.",		/* C3E_INVALACT */
    "Indicated coded character set not supported.",	/* C3E_INVALCCS */
    "Conversion manner number out of range.",		/* C3E_INVALCMANO */
    "Non-existing parameter.",				/* C3E_INVALCPAR */
    "Parameter value not supported.",			/* C3E_INVALCPARVAL */
    "Non-existing copy stream handle.",		/* C3E_INVALCSHANDL */
    "Unknown conversion system.",		/* C3E_INVALCSYST */
    "Unsupported outcome handling level.",	/* C3E_INVALLEV */
    "Non-existing master stream handle.",	/* C3E_INVALMSHANDL */
    "The indicated outcome is not defined.",	/* C3E_INVALOUTC */
    "Non-existing stream handle",		/* C3E_INVALSHANDL */
    "Unknown conversion system.",		/* C3E_INVALSYST */
    "No target string given.",			/* C3E_INVALTGTSTR */
    "Indicated topic not available.",		/* C3E_INVALTOPIC */
    
    "Parameter has no default value.",		/* C3E_NODEFAULT */
    "Unable to allocate memory.",		/* C3E_NOMEM */
    "No previous conversion operation made.",	/* C3E_NOPREVCONV */
    
    "Parameter not of string type.",		/* C3E_PARNOSTR */
    "Parameter not of integer type.",		/* C3E_PARNOTINT */
    "Possible string overlapping.",		/* C3E_POSSOVERLAP */
    
    "Too many subparameters.", 			 /* C3E_SUBPAREXC */
    
    "Target string overflow.",			/* C3E_TGTSTROFLOW */
};

static struct
{
    c3bool csyst_is_initialized;
    char *current_csyst_name;
    /*
     * Exceptions etc.
     */
    int gen_exc_handl_lev;
    int gen_exc_handl_time;
    c3bool have_pending_gen_exc_handl;
    int latest_outcome_code;
    char *exception_data;
} c3_globals = 
{
    FALSE, NULL,
    C3PV_EHL_INFORMATIVE, C3PV_EHA_DELAYED, FALSE, 0, ""
};

/*
 * The private data structures
 */

static c3_stream *stab[C3IL_MAX_STREAM_HANDLES];

struct ccs_no_id_mime *ccs_noidmime;
int ccs_noidmime_length;

/*
 *    Internal functions
 */

c3bool
_c3_exists(stream_handle)
    int stream_handle;
{
    return(stream_handle >= 0
	   && stream_handle < C3IL_MAX_STREAM_HANDLES
	   && stab[stream_handle] != NULL);
}

void
_c3_register_csyst(name)
	const char *name;
{
    if (name == NULL)
    {
	c3_globals.current_csyst_name = NULL;
    	c3_globals.csyst_is_initialized = FALSE;
    }
    else
    {
	c3_globals.current_csyst_name = strdup(name);
	c3_globals.csyst_is_initialized = TRUE;
    }
}

c3bool
_c3_exists_system_name(conv_syst)
	const char *conv_syst;
{
    return(fh_c3_exist_convsyst(conv_syst));
}

char *
_c3_curr_conv_syst_name()
{
    return(c3_globals.current_csyst_name);

}

char *
_ccs_mimename_from_ccs_no(ccs_no)
    const int ccs_no;
{
    struct ccs_no_id_mime *tablep;

    for (tablep = ccs_noidmime;
	 tablep->id != NULL; tablep++)
    {
	if (tablep->num == ccs_no)
	{
	    return(tablep->mime_name != NULL && *tablep->mime_name != '\0'?
		 tablep->mime_name :
		 tablep->id);
	}
    }
}

char *
_ccs_id_from_ccs_no(ccs_no)
    const int ccs_no;
{
    struct ccs_no_id_mime *tablep;

    for (tablep = ccs_noidmime;
	 tablep->id != NULL; tablep++)
    {
	if (tablep->num == ccs_no)
	{
	    return(tablep->id);
	}
    }
    return(NULL);
}


int
c3_ccs_number(ccs_naming_system, ccs_name)
    const char *ccs_naming_system;
        /* The character set naming system used for ccs_name */
    const char *ccs_name;
        /* The character set name for which the C3 ccs
         * number is sought. */
{
    struct ccs_no_id_mime *tablep;

    for (tablep = ccs_noidmime;
	 tablep->id != NULL; tablep++)
    {
	if (strcasecmp(tablep->id, ccs_name)  == 0)
	{
	    return(tablep->num);
	}
    }
    return(-1);
}

int
_ccs_no_from_filename(filename)
    char *filename;
{
    struct ccs_no_id_mime *tablep;
    for (tablep = ccs_noidmime;
	 tablep->id != NULL; tablep++)
    {
	int idlen;

	idlen = strlen(tablep->id);
	if (strncasecmp(filename, "def-", 4) == 0
	    && strncasecmp(filename+4, tablep->id, idlen) == 0
	    && strcasecmp(filename+4+idlen, ".txta") == 0)
	{
	    break;
	}
    }
    if (tablep->id == NULL)
    {
	return(-1);
    }
    else	/* Name found */
    {
	return(tablep->num);
    }
}


c3bool
_c3_ccs_is_available(ccs_no)
    int ccs_no;
{
    return((get_bio_meta_ccs_index(ccs_no) == -1) ? FALSE : TRUE);
}


int
_c3_set_defaults_OK(stream)
	c3_stream *stream;
{
    stream->src_ccs = NULL;
    stream->tgt_ccs = NULL;
    stream->ctype = C3IV_CTYPE_UNDEF;
    stream->src_lb.amount = 0;
    stream->tgt_lb = 0;
    stream->direction = 0;
    stream->signal_in_tgt = euseq_from_str("&", 1, -1);
    stream->signal_in_src = euseq_from_str("&", 1, -1);
    stream->signal_subst_in_tgt = euseq_from_str("&&", 1, -1);
    stream->select = C3IV_SELECT_ALL;
    stream->approx.type = C3SV_APPROX_TYPE_NO;
/*    stream->approx.type = C3SV_APPROX_TYPE_TWO; */
    stream->approx.pre = euseq_from_str("[1m", 1, -1);
    stream->approx.post = euseq_from_str("[m", 1, -1);
    stream->data_err.tgt_effect = C3IV_DATA_ERR_SECODE;
    stream->data_err.before = "(";
    stream->data_err.betw = "?";
    stream->data_err.after = ")";
    stream->factors = NULL;
    stream->current_state = 0;
    stream->impl_conv_mode = 1;
    stream->data_complete = FALSE;
    return(TRUE);
}

#undef CURRFUNC
#define CURRFUNC "_c3_do_set_iparam_ERR"

int
_c3_do_set_iparam_ERR(stream_handle, par_spec, int_value)
	int stream_handle;
	int par_spec;
	int int_value;
{
    int res = C3E_INTERNAL;

    if (stab[stream_handle]->wtable != NULL)
    {
	/* TODO
	 * _c3_debug_inform(CURRFUNC, 3, "Worktable change not done");
	 *
	 * Paverkar denna andring arbetstabellens innehall?
	 * I sa fall:
	 *     Skapa nytt arbetstabellstabells-"entry"
	 *     Kopiera alla parametrar fr}n gamla "entry"-t utom PARAM
	 *     (som far vardet IVALUE)
	 */
    }
    switch (par_spec)
    {
    case C3I_SRC_CCS:
    	if (_c3_ccs_is_available(int_value))
    	{
    	    if (stab[stream_handle]->src_num != int_value)
    	    {
		stab[stream_handle]->src_num = int_value;
		stab[stream_handle]->data_complete = FALSE;
	    }
	    res = C3E_SUCCESS;
	}
	break;

    case C3I_TGT_CCS:
    	if (_c3_ccs_is_available(int_value))
    	{
    	    if (stab[stream_handle]->tgt_num != int_value)
    	    {
		stab[stream_handle]->tgt_num = int_value;
		stab[stream_handle]->data_complete = FALSE;
	    }
	    res = C3E_SUCCESS;
	}
	break;

    case C3I_CTYPE:
	if (int_value > 0 && int_value <= C3_MAXCONVTYPES
	    && (stab[stream_handle]->direction == C3IV_DIRECT_CONV
		|| int_value == C3IV_CTYPE_UNIQUE))
	{
	    if (stab[stream_handle]->ctype != int_value)
	    {
		stab[stream_handle]->ctype = int_value;
		stab[stream_handle]->data_complete = FALSE;
	    }
	    res = C3E_SUCCESS;
	}
	else
	{
	    res = C3E_INVALCPARVAL;
	}
	break;

    case C3I_DIRECT:
/*
 * if statement when reconversion is re-implemented:

	if (int_value == C3IV_DIRECT_CONV
	 || (int_value == C3IV_DIRECT_RECONV
	    && (stab[stream_handle]->ctype == C3IV_CTYPE_UNIQUE
	       || stab[stream_handle]->ctype == C3IV_CTYPE_UNDEF)))

 *
 */
	if (int_value == C3IV_DIRECT_CONV)
	{
	    if (stab[stream_handle]->direction != int_value)
	    {
		stab[stream_handle]->direction = int_value;
		stab[stream_handle]->data_complete = FALSE;
	    }
	    res = C3E_SUCCESS;
	}
	else
	{
	    res = C3E_INVALCPARVAL;
	}
	break;

    case C3I_SELECT:
	if (int_value == C3IV_SELECT_ALL
	 || int_value == C3IV_SELECT_PART_WITHOUT_TRACE
	 || int_value == C3IV_SELECT_PART_WITH_TRACE)
	{
	    stab[stream_handle]->select = int_value;
	    /* TODO stab[stream_handle]->data_complete ? */
	    res = C3E_SUCCESS;
	}
	else
	{
	    res = C3E_INVALCPARVAL;
	}
	break;

    default:
	res = C3E_INVALCPAR;
	break;
    }
    return(res);
}

#undef CURRFUNC
#define CURRFUNC "_c3_do_set_sparam_ERR"

int
_c3_do_set_sparam_ERR(stream_handle, par_spec, str_value)
	int stream_handle;
	int par_spec;
	const char *str_value;
{
    int res = C3E_INTERNAL;
    int new_value;

    if (stab[stream_handle]->wtable != NULL)
    {
	/* TODO
	 * _c3_debug_inform(CURRFUNC, 3, "Worktable change not done");
	 *
	 * Paverkar denna andring arbetstabellens innehall?
	 * I sa fall:
	 *     Skapa nytt arbetstabellstabells-"entry"
	 *     Kopiera alla parametrar fr}n gamla "entry"-t utom PARAM
	 *     (som far vardet IVALUE)
	 */
    }
    switch (par_spec)
    {
    case C3S_SRC_LB:
	res = C3E_SUCCESS;
	if (strcasecmp(str_value, "cr") == 0)
	{
	    new_value = C3LIB_LB_CR;
	}
	else if (strcasecmp(str_value, "lf") == 0)
	{
	    new_value = C3LIB_LB_LF;
	}
	else if (strcasecmp(str_value, "crlf") == 0)
	{
	    new_value = C3LIB_LB_CRLF;
	}
	else if (strcasecmp(str_value, "lfcr") == 0)
	{
	    new_value = C3LIB_LB_LFCR;
	}
	else
	{
	    res = C3E_INVALCPARVAL;
	}
	if (res == C3E_SUCCESS
	    && stab[stream_handle]->src_lb.types[0] != new_value)
	{
	    stab[stream_handle]->src_lb.amount = 1;
	    stab[stream_handle]->src_lb.types[0] = new_value;
	    stab[stream_handle]->data_complete = FALSE;
	}

	break;

    case C3S_TGT_LB:
	res = C3E_SUCCESS;
	if (strcasecmp(str_value, "cr") == 0)
	{
	    new_value = C3LIB_LB_CR;
	}
	else if (strcasecmp(str_value, "lf") == 0)
	{
	    new_value = C3LIB_LB_LF;
	}
	else if (strcasecmp(str_value, "crlf") == 0)
	{
	    new_value = C3LIB_LB_CRLF;
	}
	else if (strcasecmp(str_value, "lfcr") == 0)
	{
	    new_value = C3LIB_LB_LFCR;
	}
	else
	{
	    res = C3E_INVALCPARVAL;
	}
	if (res == C3E_SUCCESS
	    && 	stab[stream_handle]->tgt_lb != new_value)
	{
	    stab[stream_handle]->tgt_lb = new_value;
	    stab[stream_handle]->data_complete = FALSE;
	}
	break;

    case C3S_SIGN:
	res = C3E_INVALCPARVAL;
	break;

    case C3S_SIGN_SUBST:
	res = C3E_INVALCPARVAL;
	break;

    case C3S_APPROX:
	res = C3E_INVALCPARVAL;
	break;

    case C3S_DATA_ERR:
	res = C3E_INVALCPARVAL;
	break;

    case C3F_TGT_ENVIRON:
	res = C3E_INVALCPARVAL;
	break;

    case C3F_CYR_LANG:
	res = C3E_INVALCPARVAL;
	break;

    case C3F_SRC_PC_CTRL_CHAR:
	res = C3E_INVALCPARVAL;
	break;

    case C3F_TGT_PC_CTRL_CHAR:
	res = C3E_INVALCPARVAL;
	break;

    case C3F_PRESERVE_ASCII:
	res = C3E_INVALCPARVAL;
	break;

    case C3F_RCGN_646_SWE:
	res = C3E_INVALCPARVAL;
	break;

    default:
	/* Assume locale factor? */
	res = C3E_INVALCPAR;
	break;
    }
    return(res);
}

#undef CURRFUNC
#define CURRFUNC "_c3_free_stream"

void
_c3_free_stream(str)
	c3_stream *str;
{
    if (str != NULL)
    {
        /* TODO Free a lot of things */
	free(str);
    }
}


#undef CURRFUNC
#define CURRFUNC "_c3_do_gen_exception_handl"

void
_c3_do_gen_exception_handl(outcome_code, handling_level)
    int outcome_code;
    int handling_level;
{
    int i1;

    if (outcome_code == C3E_SUCCESS)
    {
#if 0  /* don't reset this value */
	c3_globals.exception_data = "";
#endif
    }
    else if (outcome_code > 0) /* Warnings */
    {
	i1 = abs(outcome_code);
	if (i1 > abs(C3W_ZZZ))
	{
	    /* Prevent eternal loop... */
	    if (abs(C3E_INTERNAL) > abs(C3E_ZZZ)) {return;} 
	    C3VERBOSE(VERB_MINI, "outcome code (%d) > C3W_ZZZ",
		    CURRFUNC, outcome_code)
	    _c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	    return;
	}
	if (handling_level >= C3PV_EHL_INFORMATIVE)
	{
	    fprintf(stderr, "C3 Warning: %s (%d)\n",
		    warnmess_en[i1], outcome_code);
	}
	if (handling_level >= C3PV_EHL_WARY)
	{
	    exit(1);
	}
    }
    else /* Errors */
    {
	i1 = abs(outcome_code);
	if (i1 > abs(C3E_ZZZ))
	{
	    /* Prevent eternal loop... */
	    if (abs(C3E_INTERNAL) > abs(C3E_ZZZ)) {return;}
	    C3VERBOSE(VERB_MINI, "outcome code (%d) > C3E_ZZZ",
		    CURRFUNC, outcome_code)
	    _c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	    return;
	}
	if (handling_level >= C3PV_EHL_INFORMATIVE)
	{
	    fprintf(stderr, "C3 Error: %s (%d)\n",
		    errmess_en[i1], outcome_code);
	}
	if (handling_level >= C3PV_EHL_SECURE)
	{
	    exit(1);
	}
    }
}

#undef CURRFUNC
#define CURRFUNC "_c3_tgt_transf1_OK"

c3bool
_c3_tgt_transf1_OK(src_eus, tgt_eus)
    euseq *src_eus;
    euseq *tgt_eus;
{
    int no_src_bytes = euseq_no_bytes(src_eus);

    if (no_src_bytes > euseq_bytes_remaining(tgt_eus))
    {
	return(FALSE); /* Target string overflow */
    }
    else
    {
	euseq_setpos(src_eus, 0);

 	switch(no_src_bytes)
	{
	case 1:
	    euseq_puteu(tgt_eus, euseq_geteu(src_eus));
	    break;
	case 2:
	    euseq_puteu(tgt_eus, euseq_geteu(src_eus));
	    euseq_puteu(tgt_eus, euseq_geteu(src_eus));
	    break;
	default:
	{
	    int src_eu;

	    for (src_eu = euseq_geteu(src_eus);
		 src_eu != NOT_AN_ENCODING_UNIT;
		 src_eu = euseq_geteu(src_eus))
	    {
		euseq_puteu(tgt_eus, src_eu);
	    }
	    break;
	}
	}
    }
    return(TRUE);
}

#define JUST_READ_SIGNALPOS(euseq) \
    (euseq)->pos - (euseq)->bytes_per_eu == (euseq)->pos_extra

#undef CURRFUNC
#define CURRFUNC "_c3_tgt_transf2_OK"

c3bool
_c3_tgt_transf2_OK(src_eus, tgt_eus, signal_eu)
    euseq *src_eus;
    euseq *tgt_eus;
    int signal_eu;
{
    int no_src_bytes = euseq_no_bytes(src_eus);

    if (no_src_bytes > euseq_bytes_remaining(tgt_eus))
    {
	return(FALSE); /* Target string overflow */
    }
    else
    {
	enc_unit anEncUnit;

	euseq_setpos(src_eus, 0);
	for (anEncUnit = euseq_geteu(src_eus);
	     anEncUnit != NOT_AN_ENCODING_UNIT;
	     anEncUnit = euseq_geteu(src_eus))
	{
	    if (JUST_READ_SIGNALPOS(src_eus))
	    {
		euseq_puteu(tgt_eus, signal_eu);
	    }
	    else
	    {
		euseq_puteu(tgt_eus, anEncUnit);
	    }
	}
    }
    return(TRUE);
}

#undef CURRFUNC
#define CURRFUNC "_c3_tgt_transf_OK"

c3bool
_c3_tgt_transf_OK(src_eu, tabvec, cstate, one_state_length, tgt_eus,
	signal_eu)
    int src_eu;
    struct wtableinfo *tabvec;
    state *cstate;
    int one_state_length;
    euseq *tgt_eus;
    int signal_eu;
{
    struct wtableinfo *tabp = &(tabvec[(*cstate * one_state_length) + src_eu]);
    int no_src_bytes = euseq_no_bytes(tabp->seq);

    if (no_src_bytes == 0)
    {
      *cstate = tabp->resultState;
      return(TRUE);
    }
    else if (no_src_bytes > euseq_bytes_remaining(tgt_eus))
    {
	return(FALSE);  /* Target string overflow? */
    }
    else
    {
	int anEncUnit;

	euseq_setpos(tabp->seq, 0);
	for (anEncUnit = euseq_geteu(tabp->seq);
	     anEncUnit != NOT_AN_ENCODING_UNIT;
	     anEncUnit = euseq_geteu(tabp->seq))

	{
	    if (JUST_READ_SIGNALPOS(tabp->seq))
	    {
		euseq_puteu(tgt_eus, signal_eu);
	    }
	    else
	    {
		C3VERBOSE(VERB_DEB3, "Target encUnit %04x", CURRFUNC, anEncUnit)
		euseq_puteu(tgt_eus, anEncUnit);
	    }
	}
	*cstate = tabp->resultState;
    }
    return(TRUE);
}

#define C3X_CSTATE_BASE			 0
#define C3X_CSTATE_SIGNAL_FOUND		-1
#define C3X_CSTATE_SIGNAL_READING	-2

#undef CURRFUNC
#define CURRFUNC "_c3_register_outcome"

void
_c3_register_outcome(function_name, outcome_code)
    const char *function_name;
    const int outcome_code;
{	
    if (outcome_code == C3E_INTERNAL)
	strlen(0);
    if (c3_globals.gen_exc_handl_time == C3PV_EHA_DELAYED
	&& c3_globals.have_pending_gen_exc_handl)
    {
	_c3_do_gen_exception_handl(c3_globals.latest_outcome_code,
				   c3_globals.gen_exc_handl_lev);
	c3_globals.have_pending_gen_exc_handl = FALSE;
    }
    
    c3_globals.latest_outcome_code = outcome_code;
    
    if (c3_globals.gen_exc_handl_time == C3PV_EHA_IMMEDIATE)
    {
	_c3_do_gen_exception_handl(outcome_code, c3_globals.gen_exc_handl_lev);
	c3_globals.have_pending_gen_exc_handl = FALSE;
    }
    else
    {
	c3_globals.have_pending_gen_exc_handl = TRUE;
    }
}


#undef CURRFUNC
#define CURRFUNC "c3_initialize"

void
c3_initialize(conv_syst)
	const char *conv_syst;
{
    int i1, length;
	
    if (c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3W_CSYSTNOTFIN);
	c3_finalize();
    }
    
    if (conv_syst == NULL || *conv_syst == '\0')
    {
	conv_syst = strdup(CSYST_DEF);  /* From -D in Makefile */
    }
    if (! _c3_exists_system_name(conv_syst))
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALSYST);
	return;
    }
    
    _c3_register_csyst(conv_syst);
    for (i1 = 0; i1 < C3IL_MAX_STREAM_HANDLES; i1++)
    {
	stab[i1] = NULL;
    }   
    _c3_register_outcome(CURRFUNC, C3E_SUCCESS);
    C3VERBOSE(VERB_NORM, "Conversion system %s", CURRFUNC, conv_syst)

    fh_c3_build_table(conv_syst, &ccs_noidmime, &ccs_noidmime_length);

}


#undef CURRFUNC
#define CURRFUNC "c3_set_exception_handling"

void
c3_set_exception_handling(level, activation_time)
    const int level;
        /* The desired level of general exception handling.
         */
    const int activation_time;
        /* The desired activation time for the general
         * exception handling.
         */
{
    if (level < 0 || level > C3PV_EHL_ZZZ)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALLEV);	
	return;
    }
    c3_globals.gen_exc_handl_lev = level;
    if (activation_time < 0 || activation_time > C3PV_EHA_ZZZ)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALACT);	
	return;
    }
    c3_globals.gen_exc_handl_time = activation_time;
    return;
}

#undef CURRFUNC
#define CURRFUNC "c3_outcome_code"

int
c3_outcome_code(message)
    char **message;
        /* The message used in the general outcome handling
         * for the outcome of the previous genuine API call.
         */
{
    int loutcome = c3_globals.latest_outcome_code;
    
    if (loutcome == 0) /* C3E_SUCCESS */
    {
    	*message = NULL;
    }
    else if (loutcome > 0)
    {
	*message = warnmess_en[loutcome];
    }
    else if (loutcome < 0)
    {
	*message = errmess_en[abs(loutcome)];
    }
    c3_globals.have_pending_gen_exc_handl = FALSE;
    return(loutcome);	
}

#undef CURRFUNC
#define CURRFUNC "c3_exception_data"

char *
c3_exception_data()
{
    return(c3_globals.exception_data);
}

/* CONVERSION INITIALIZATION FUNCTIONS */

#undef CURRFUNC
#define CURRFUNC "c3_create_stream"

int
c3_create_stream()
{
    int i1;
    
    if (!c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3E_CSYSTNOTINIT);
	return(-1);
    }
    /*
     * Find a free handle
     */
    for (i1 = 0; i1 < C3IL_MAX_STREAM_HANDLES && stab[i1] != NULL; i1++)
	;
    if (i1 == C3IL_MAX_STREAM_HANDLES)
    {
	/* TODO Fix dynamic reallocation */
	C3VERBOSE(VERB_NORM, "STREAM_HANDLES overflow%c", CURRFUNC, ' ')
	return(-1);
    }

    /*
     * Allocate handle memory
     */
    SAFE_ZALLOCATE(stab[i1], (c3_stream *),
		  sizeof (c3_stream), return(-1), CURRFUNC);
    if (!_c3_set_defaults_OK(stab[i1]))
    {
	/* TODO _c3_free_stream(stab[i1]); */

	/* Error reported from defaults routine. TODO: Should have CURRFUNC? */
	return(-1);
    }
    _c3_register_outcome(CURRFUNC, C3E_SUCCESS);
    return(i1);
}



#define C3I 0
#define C3S 1
#define C3F 2
#define C3NONE 3

int param_properties[] =
{
    C3NONE,
    C3I,   /* C3I_SRC_CCS */
    C3I,   /* C3I_TGT_CCS */
    C3I,   /* C3I_CTYPE */

    C3S,   /* C3S_SRC_LB */
    C3S,   /* C3S_TGT_LB */
    C3I,   /* C3I_DIRECT */
    C3S,   /* C3S_SIGN */
    C3S,   /* C3S_SIGN_SUBST */
    C3I,   /* C3I_SELECT */
    C3S,   /* C3S_APPROX */
    C3I,   /* C3S_DATA_ERR */

    C3NONE,
    C3NONE,
    C3NONE,
    C3NONE,
    C3NONE,
    C3NONE,
    C3NONE,
    C3NONE,
    C3NONE,


    C3F,   /* C3F_TGT_ENVIRON */
    C3F,   /* C3F_CYR_LANG */
    C3F,   /* C3F_SRC_PC_CTRL_CHAR */
    C3F,   /* C3F_TGT_PC_CTRL_CHAR */
    C3F,   /* C3F_PRESERVE_ASCII */
    C3F,   /* C3F_RCGN_646_SWE */
     -1    /* END */
};


#undef CURRFUNC
#define CURRFUNC "c3_set_iparam"

void
c3_set_iparam(stream_handle, par_spec, int_value)
	const int stream_handle;
	const int par_spec;
	const int int_value;
{
    int i1;

    C3VERBOSE(VERB_DEB2, "Parameter %d", CURRFUNC, par_spec)

    if (!c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3E_CSYSTNOTINIT);
	return;
    }
    if ( ! _c3_exists(stream_handle))
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALSHANDL);
	return;
    }
    /* 
     *  Is amongst known parameters?
     */
    if (par_spec < 0 || par_spec > C3X_PARAM_ZZZ)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALCPAR);
	return;
    }
    /* 
     *  Is of integer type?
     */
    if (param_properties[par_spec] != C3I)
    {
	_c3_register_outcome(CURRFUNC, C3E_PARNOTINT);
	return;
    }
    i1 = _c3_do_set_iparam_ERR(stream_handle, par_spec, int_value);

    _c3_register_outcome(CURRFUNC, i1);
    C3VERBOSE(VERB_DEB2, "Value %d", CURRFUNC, int_value)
}

#undef CURRFUNC
#define CURRFUNC "c3_set_sparam"

void
c3_set_sparam(stream_handle, par_spec, str_value)
	const int stream_handle;
	const int par_spec;
	const char *str_value;
{
    int i1;

    C3VERBOSE(VERB_DEB2, "Parameter %d", CURRFUNC, par_spec)
    if (!c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3E_CSYSTNOTINIT);
	return;
    }
    if ( ! _c3_exists(stream_handle))
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALSHANDL);
	return;
    }
    /* 
     *  Is amongst known parameters?
     */
    if (par_spec < 0 || par_spec > C3X_PARAM_ZZZ)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALCPAR);
	return;
    }
    /* 
     *  Is of string type?
     */
    if (param_properties[par_spec] != C3S)
    {
	_c3_register_outcome(CURRFUNC, C3E_PARNOSTR);
	return;
    }
    i1 = _c3_do_set_sparam_ERR(stream_handle, par_spec, str_value);
    _c3_register_outcome(CURRFUNC, i1);
    C3VERBOSE(VERB_DEB2, "Value <%s>", CURRFUNC, str_value)
}

/* CONVERSION FUNCTIONS */

#undef CURRFUNC
#define CURRFUNC "c3_bconv"

char *
c3_bconv(stream_handle, target_str, source_str, target_str_max_length,
	target_str_result_length, source_str_length)
	const int stream_handle;
	char *target_str;
	const char *source_str;
	const unsigned int target_str_max_length;
	int *target_str_result_length;
	const unsigned int source_str_length;
{
    struct wtableinfo *theWorktable;
    int currState, one_state_length;
    enc_unit signal_eUnit_src, signal_eUnit_tgt;
    int convertNow;
    int inside_approx;
    int is_mreconv;
    euseq *src_eusp, *tgt_eusp;
    enc_unit src_eu;
    int outcome = C3E_SUCCESS;
    euseq *signalStrBuffer=NULL, *outseq;
    char_encoding *c_enc;
    c3_stream *stream;

    /*
     * Parameter error checks
     */

    if (!c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3E_CSYSTNOTINIT);
	return NULL;
    }
    if (stab[stream_handle] == NULL)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALSHANDL);
	return NULL;
    }
    stream = stab[stream_handle];
    if (stream->src_num == 0
	|| stream->tgt_num == 0
	|| stream->ctype == C3IV_CTYPE_UNDEF)
    {
	_c3_register_outcome(CURRFUNC, C3E_CPARNOTINIT);
	return NULL;
    }
    if (source_str == NULL) /* TODO || *source_str == '\0' */
    {
	_c3_register_outcome(CURRFUNC, C3W_EMPTYSRCSTR);
    }
    if (target_str == NULL)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALTGTSTR);
	return NULL;
    }
    /* TODO: Overlapping ==> C3E_POSSOVERLAP */
    /*  TODO Registrera huruvuda SRC {r null/empty - i s} fall varning
     *  senare om minnet inte r{ckte till aktuell arrbetstabell. (Detta
     *  fel ger v{l annars error?)
     */
    if ( ! stream->data_complete)
    {
	if ( ! _c3_update_convinfo_OK(c3_globals.current_csyst_name,
				      stream))
	{
		/* don't return here without an error code!!!  tom */
		/* Not sure of the correct error here TODO */
	    _c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	    return NULL;
	}
    }
    /* TODO Extend this */
/* TODO
    if (stream->src_ccs->byteWidth != 1
    	&& stream->tgt_ccs->byteWidth != 1)
    {
	C3VERBOSE(VERB_MINI, "Only 7 or 8 bit character sets allowed%c",
		CURRFUNC, ' ')
	exit(2);
    }
*/
    theWorktable = stream->wtable;
    currState = stream->current_state;
    one_state_length = stream->src_ccs->eunitStore;

    /* TODO This should depend on if a signal-sequence-using type is used. */
    if (stream->ctype == C3IV_CTYPE_UNIQUE)
    {
        /* TODO Signal euseq with length != 1 is currently not allowed -
        	check somewhere */
        /* TODO According to API-spec F the below two signal euseqs contains
        	"The octet sequence by which the signal sequence is
        	represented in the target character set". Hence the
		first of them should be
        	backwards converted to the source character set in order to
        	find that character there.
        */
        signal_eUnit_tgt = euseq_geteu_at(stream->signal_in_tgt, 0);
        signal_eUnit_src = signal_eUnit_tgt;
	/* TODO Convert to src CCS (se above) */
    }
    else
    {
	/* TODO Temporary solution */
        signal_eUnit_tgt = (enc_unit) '&';
    }
    convertNow = (stream->select == C3IV_SELECT_ALL ?
		  1 : stream->impl_conv_mode);
    inside_approx = FALSE;
    is_mreconv = (stream->direction == C3IV_DIRECT_RECONV
		  && stream->ctype == C3IV_CTYPE_UNIQUE);
    tgt_eusp = euseq_from_str((const char *)target_str,
	       stream->tgt_ccs->byteWidth,
    	       target_str_max_length);
    src_eusp = euseq_from_str(source_str,
    	       stream->src_ccs->byteWidth,
	       source_str_length);
    for (src_eu = euseq_geteu(src_eusp);
	 src_eu != NOT_AN_ENCODING_UNIT;
	 src_eu = euseq_geteu(src_eusp))
    {
	/* TODO SOH interpretation, set convertNow */
	
	C3VERBOSE(VERB_DEB2, "Source encUnit %04x", CURRFUNC, src_eu)

	if (stream->src_ccs->bitWidth == 7
	    && src_eu > 127)
	{
	    C3VERBOSE(VERB_MINI, "Illegal source encoding unit (%x) found",
		    CURRFUNC, src_eu)
	    outcome = C3E_INVALSRCENC;
	    break;
	}

	if (stream->approx.type == C3SV_APPROX_TYPE_ONE
	    && theWorktable[src_eu].conv_quality == C3INT_CONV_APPROX)
	{
	    if ( ! _c3_tgt_transf1_OK(stream->approx.pre,
				      tgt_eusp))
	    {
	        outcome = C3E_TGTSTROFLOW;
	        break;
	    }
	}
	else if (stream->approx.type == C3SV_APPROX_TYPE_TWO
	         && !inside_approx
		 && theWorktable[src_eu].conv_quality ==C3INT_CONV_APPROX)
	{
	    if ( ! _c3_tgt_transf1_OK(stream->approx.pre,
				      tgt_eusp))
	    {
	        outcome = C3E_TGTSTROFLOW;
	        break;
	    }
	    inside_approx = TRUE;
	}
	else if (stream->approx.type == C3SV_APPROX_TYPE_TWO
		 && inside_approx
		 && theWorktable[src_eu].conv_quality == C3INT_CONV_EXACT)
	{
	    if ( ! _c3_tgt_transf1_OK(stream->approx.post,
				      tgt_eusp))
	    {
	        outcome = C3E_TGTSTROFLOW;
	        break;
	    }
	    inside_approx = FALSE;
	}
	if (! convertNow)
	{
	    euseq *tmp_eu = euseq_from_str(src_eusp->pos,
					   src_eusp->bytes_per_eu,
					   src_eusp->bytes_per_eu);

	    if ( ! _c3_tgt_transf1_OK(tmp_eu, tgt_eusp))
	    {
	        outcome = C3E_TGTSTROFLOW;
	        break;
	    }
	    free(tmp_eu);
	}
	else
	{
	    switch (currState)
	    {
       	    case C3X_CSTATE_BASE:
	    default:
		/* TODO Handle non-existant states? */
		/* TODO This must be converted to src ccs */
		if (is_mreconv && currState == C3X_CSTATE_BASE
		    && src_eu == signal_eUnit_src)
		{
		    currState = C3X_CSTATE_SIGNAL_FOUND;
		    signalStrBuffer = euseq_from_dupstr(NULL,
		    		      stream->src_ccs->byteWidth, 16);
			/* To make this buffer comparable to the stored
			 * charData eu_seqs, the first encoding unit must
			 * be SPACE (and be the position of the signal
			 * character).
			 */
		    euseq_puteu(signalStrBuffer, (enc_unit) ' ');
		    euseq_setepos(signalStrBuffer, 0);
/*
		    stream->mnemTabData->current =
			stream->mnemTabData->tab;
*/
		}
		else if (currState == C3X_CSTATE_BASE
			 && stream->ctype == C3IV_CTYPE_UNIQUE
			 && src_eu == signal_eUnit_src)
		{
		    if ( ! _c3_tgt_transf1_OK(stream->signal_subst_in_tgt,
					      tgt_eusp))
		    {
			outcome = C3E_TGTSTROFLOW;
			goto exit;
		    }
		}
		else
		{
		    if ( ! _c3_tgt_transf_OK(src_eu, theWorktable, &currState,
					     one_state_length, tgt_eusp,
					     signal_eUnit_tgt))
		    {
			outcome = C3E_TGTSTROFLOW;
			goto exit;
		    }
		}
		break;
			
	    case C3X_CSTATE_SIGNAL_READING:
	    case C3X_CSTATE_SIGNAL_FOUND:

		if (currState == C3X_CSTATE_SIGNAL_FOUND
		    && src_eu == signal_eUnit_src)
		    /* Assuming double signal_eUnit_src means the char itself*/
		{
		    currState = C3X_CSTATE_BASE;
		    if ( ! _c3_tgt_transf1_OK(stream->signal_in_tgt, tgt_eusp))
		    {
			outcome = C3E_TGTSTROFLOW;
			goto exit;
		    }
		}
		else 
		{
		    euseq_puteu(signalStrBuffer, src_eu);
/*		    switch (_c3_exists_mnem(signalStrBuffer, stream))
 */
		    switch (0)
		    {

		    case 0: /* No match */
			/* TODO Should output conv(str) */
			if ( ! _c3_tgt_transf1_OK(signalStrBuffer, tgt_eusp))
			{
			    outcome = C3E_TGTSTROFLOW;
			    goto exit;
			}
			currState = C3X_CSTATE_BASE;
			break;

		    case 1: /* Part match */
			currState = C3X_CSTATE_SIGNAL_READING;
			break;

		    case 2: /* Exact match */
/*			c_enc = *_c3_chardata_ref(stab[stream_handle]->tgt_ccs,
				    stream->mnemTabData->current->ucs);
 */			c_enc = NULL;
			if (c_enc == NULL)
			{
			    /* TODO Handle how? */
			    euseq *tmp_eus = euseq_from_str("???", 1, 3);

			    if ( ! _c3_tgt_transf1_OK(tmp_eus, tgt_eusp))
			    {
				outcome = C3E_TGTSTROFLOW;
				goto exit;
			    }
			    FREE_EUSEQ(tmp_eus)
			}
			else
			{
			    if (c_enc
				->conv_quality == C3INT_CONV_EXACT)
			    {
				/* TODO */
				outseq =
				    euseq_from_dupstr(" ",
				    stab[stream_handle]->tgt_ccs->byteWidth, -1);
				euseq_append_OK(outseq,
						c_enc->reprstr[stream->ctype]);
			    }
			    else
			    {
				outseq = NULL;
/*				    _c3_find_approx_eunitseq(
				        c_enc[stream->ctype],
					stab[stream_handle]->tgt_ccs,
					signal_eUnit_tgt);
*/				if (outseq == NULL)
				{
				    C3VERBOSE(VERB_MINI,
					"Couldn't find approx seq for ??%c",
					 CURRFUNC,
					 ' ')
/*					 stream->mnemTabData->current->ucs)
 */
				    goto exit;
				}
			    }
			    if ( ! _c3_tgt_transf2_OK(outseq,
						      tgt_eusp,
						      signal_eUnit_tgt))
			    {
				outcome = C3E_TGTSTROFLOW;
				goto exit;
			    }
			}
			FREE_EUSEQ(outseq)
			currState = C3X_CSTATE_BASE;
			FREE_EUSEQ(signalStrBuffer)
			break;
		    }
		}
		break;

	    }
	}    
    }
 exit:	
    if (outcome != C3E_TGTSTROFLOW
	&& currState == C3X_CSTATE_SIGNAL_READING)
    {
	/* TODO Should output conv(str) */
	if ( ! _c3_tgt_transf1_OK(signalStrBuffer, tgt_eusp))
	{
	    outcome = C3E_TGTSTROFLOW;
	}
    }

    /* TODO c3_globals.exception_data should be reset somewhere */
    if (outcome == C3E_TGTSTROFLOW)
    {
	c3_globals.exception_data = src_eusp->pos - src_eusp->bytes_per_eu;
    }
    else if (stream->approx.type == C3SV_APPROX_TYPE_TWO
	     && inside_approx)
    {
	if ( ! _c3_tgt_transf1_OK(stream->approx.post,
			      tgt_eusp))
	{
	    outcome = C3E_TGTSTROFLOW;
	    c3_globals.exception_data = src_eusp->pos - src_eusp->bytes_per_eu;
	}
    }
    *target_str_result_length = tgt_eusp->pos - tgt_eusp->storage;
    stream->current_state = currState;
    stream->impl_conv_mode = convertNow;

    /* TODO Warn if currState != 0 */
    FREE_EUSEQ(signalStrBuffer)
    free(tgt_eusp);
    free(src_eusp);
    _c3_register_outcome(CURRFUNC, outcome);
    return(target_str);
}

/* INFORMATION FUNCTIONS */

static struct c3struct_values_param poss_param_val;

#undef CURRFUNC
#define CURRFUNC "c3_values_param"

struct c3struct_values_param *
c3_values_param()
{
    char **chpp;
    int length;

    if (!c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3E_CSYSTNOTINIT);
	return(NULL);
    }
    if (fh_c3_def_list_OK(c3_globals.current_csyst_name, &chpp, &length))
    {
	char **p;
	int *ip;

	SAFE_ALLOCATE(poss_param_val.src_ccs, (int *),
		      (length + 1) * sizeof(int), return(NULL), CURRFUNC);

	for (ip = poss_param_val.src_ccs, p = chpp;
	     *p != NULL; ip++, p++) 
	{
	    *ip = _ccs_no_from_filename(*p);
	}
	*ip = -1;
	poss_param_val.tgt_ccs =poss_param_val.src_ccs;
    }
    SAFE_ALLOCATE(poss_param_val.ctype, (int *),
		  (3 + 1) * sizeof(int), return(NULL), CURRFUNC);
    *poss_param_val.ctype = 1;
    *(poss_param_val.ctype + 1) = 2;
    *(poss_param_val.ctype + 2) = 3;
    *(poss_param_val.ctype + 3) = -1;

    SAFE_ALLOCATE(poss_param_val.direct, (int *),
		  (2 + 1) * sizeof(int), return(NULL), CURRFUNC);
    *poss_param_val.direct = 0;
    *(poss_param_val.direct + 1) = 1;
    *(poss_param_val.direct + 2) = -1;

    SAFE_ALLOCATE(poss_param_val.select, (int *),
		  (3 + 1) * sizeof(int), return(NULL), CURRFUNC);
    *poss_param_val.select = 0;
    *(poss_param_val.select + 1) = 2;
    *(poss_param_val.select + 2) = 3;
    *(poss_param_val.select + 3) = -1;

    poss_param_val.factors = NULL;
    return(&poss_param_val);
}

static struct c3struct_charset_properties charset_prop;

#if 0 
#undef CURRFUNC
#define CURRFUNC "c3_charset_properties"

struct c3struct_charset_properties *
c3_charset_properties(charset)
    const int charset;
{
    struct ccs_no_id_mime *tablep;

    charset_prop.encoding_unit_length = -1;

    for (tablep = ccs_noidmime;
	 tablep->id != NULL; tablep++)
    {
	if (tablep->num == charset)
	{
	    charset_prop.ccs_id = tablep->id;
	    charset_prop.mime_name =
		(tablep->mime_name != NULL && *tablep->mime_name != '\0'?
		 tablep->mime_name :
		 (tablep->mime_name != NULL ? tablep->id : NULL));
	    break;
	}
    }
    if (tablep->id == NULL)
    {
	_c3_register_outcome(CURRFUNC, C3E_CCSNOINFO);	
	return(NULL);
    }
    else	/* Name found */
    {
	_c3_register_outcome(CURRFUNC, C3E_SUCCESS);
	return(&charset_prop);
    }
}
#endif

#undef CURRFUNC
#define CURRFUNC "c3_finalize_stream"

void
c3_finalize_stream(stream_handle)
    const int stream_handle;
      /* Handle to the stream to be finalized.
       */
{
    if (! c3_globals.csyst_is_initialized
        && stab[stream_handle] == NULL)
    {
	_c3_register_outcome(CURRFUNC, C3E_INVALSHANDL);
	return;
    }
    if (stab[stream_handle]->current_state != 0)
    {
	_c3_register_outcome(CURRFUNC, C3W_INVALEOS);
	/* TODO Data error handling */
    }
    stab[stream_handle]->current_state = 0;
    /* TODO stream->impl_conv_mode = ?; */
    /* TODO More to reset? */
}


#undef CURRFUNC
#define CURRFUNC "c3_finalize"

void
c3_finalize()
{
    int i1;

    if (!c3_globals.csyst_is_initialized)
    {
	_c3_register_outcome(CURRFUNC, C3E_CSYSTNOTINIT);
	return;
    }
    _c3_register_csyst(NULL);
    for (i1 = 0; i1 < C3IL_MAX_STREAM_HANDLES; i1++)
    {
	_c3_free_stream(stab[i1]);
    }   
    _c3_register_outcome(CURRFUNC, C3E_SUCCESS);

}

