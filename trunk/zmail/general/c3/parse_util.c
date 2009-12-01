
static char file_id[] =
    "$Id: parse_util.c,v 1.5 2005/05/31 07:36:40 syd Exp $";


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
#include <string.h>
#ifdef UNIX
#if !defined(DARWIN)
#include <malloc.h>
#endif
#endif

#include "c3_macros.h"
#include "c3.h"
#include "euseq.h"
#include "parse_util.h"
#include "bin_io.h"

extern int opt_verbosity;

static struct
{
    c3bool compiling_initiated;
    int def_tab_num;
    char *conv_syst;
    struct c3_app_table *appTable;
    int nextDefIndex;
    struct cdtabs *ccsDataTabs;
    struct bio_meta_data *bio_meta;
} gl = 
{
    FALSE,
    0,
    NULL,
    NULL,
    0,
    NULL,
    NULL
};

#undef CURRFUNC
#define CURRFUNC "parse_admin_OK"

c3bool
parse_admin_OK(rdataset, cd, ucs)
    struct repr_data_set *rdataset;
    struct ccs_data *cd;
    int ucs;
{
    int i1, i2, storepos, char_prio, new_alloc;
    struct repr_data **rdata;
    struct parse_data *pd;

    for (rdata = rdataset->repr, i1 = 0;
	 i1 < rdataset->nextIndex;
	 rdata++, i1++)
    {
	storepos = (*rdata)->reqStartState *
	    cd->eunitStore + (*rdata)->encUnit;
	if (cd->parse[storepos] == NULL)
	{
	    SAFE_ZALLOCATE(cd->parse[storepos],
			   (struct parse_data_set *),
			   sizeof (struct parse_data_set),
			   return(FALSE), CURRFUNC);
	    cd->parse[storepos]->pdata = NULL;
	}
	char_prio = (*rdata)->char_priority;
	if (cd->parse[storepos]->alloc == 0) /* Unallocated? */
	{
	    new_alloc = char_prio + 1;
	    SAFE_ALLOCATE(cd->parse[storepos]->pdata,
			   (struct parse_data **),
			   new_alloc * sizeof (struct parse_data *),
			   return(FALSE), CURRFUNC);
	    for (i2 = 0; i2 < new_alloc; i2++)
	    {
		cd->parse[storepos]->pdata[i2] = NULL;
	    }
	    cd->parse[storepos]->alloc = new_alloc;
	}
	else if (cd->parse[storepos]->alloc <= char_prio) /* Too few alloc? */
	{
	    new_alloc = char_prio + 1;
	    SAFE_REALLOCATE(cd->parse[storepos]->pdata,
			   (struct parse_data **),
			   new_alloc * sizeof (struct parse_data *),
			   return(FALSE), CURRFUNC);
	    for (i2 = cd->parse[storepos]->alloc; i2 < new_alloc; i2++)
	    {
		cd->parse[storepos]->pdata[i2] = NULL;
	    }
	}
	if (cd->parse[storepos]->pdata[char_prio] != NULL)
	{
	    C3VERBOSE(VERB_MINI,
"Two chars with same priority for same representation, UCS char %x (hex)",
		    CURRFUNC, ucs)
	    return(FALSE);
	}
	SAFE_ZALLOCATE(pd,
		       (struct parse_data *),
		       sizeof (struct parse_data),
		       return(FALSE), CURRFUNC);
	cd->parse[storepos]->pdata[char_prio] = pd;
	pd->ucs = ucs;
	pd->nextState = (*rdata)->resultEndState;
    }
    return(TRUE);
}

extern
char_encoding *
_c3_encoding_access P((struct ccs_data *, enc_unit ));

#undef CURRFUNC
#define CURRFUNC "encode_admin_OK"

c3bool
encode_admin_OK(rdataset, approx, cd, ucs)
    struct repr_data_set *rdataset;
    char_approximation *approx;
    struct ccs_data *cd;
    int ucs;
{
    struct encoding_plane *e_pl;
    char_encoding *the_char_encoding, *t_c_e;
    struct approxseq_set *t_a_s;
    int ctype, i1;
    enc_unit eu;
    euseq *eus;

    e_pl = enc_plane_ref(cd->encodings, ucs >> 16, TRUE);
    if (e_pl->char_encod[ucs / 256] == NULL)
    {
        char_encoding **t_c_ep;

        SAFE_ALLOCATE(t_c_ep, (char_encoding **),
                      256 * sizeof(char_encoding *),
                      return(FALSE), CURRFUNC);
        for (i1 = 0; i1 < 256; i1++)
        {
            t_c_ep[i1] = NULL;
        }
	e_pl->char_encod[ucs / 256] = t_c_ep;
    }
    if (e_pl->char_encod[ucs / 256][ucs % 256] == NULL)
    {
        SAFE_ALLOCATE(the_char_encoding, (char_encoding *),
                      1 * sizeof(char_encoding),
                      return(FALSE), CURRFUNC);
        SAFE_ALLOCATE(the_char_encoding->reprstr, (euseq **),
                      cd->ctypes * sizeof(euseq *),
                      return(FALSE), CURRFUNC);
	for (i1 = 0; i1 < cd->ctypes; i1++)
	{
	    the_char_encoding->reprstr[i1] = NULL;
	}
	e_pl->char_encod[ucs / 256][ucs % 256] = the_char_encoding;
    }
    else
    {
	C3VERBOSE(VERB_MINI,
		"Two specifications for a character, UCS char %x (hex)",
		CURRFUNC, ucs)
		return(FALSE);
    }
    if (rdataset != NULL)	/* Exact encoding representation */
    {
	struct repr_data **rdata;

	the_char_encoding->conv_quality = C3INT_CONV_EXACT;
	rdata = rdataset->repr;	/* First repr allways used (highest */
				/* encoding priority) */
	the_char_encoding->reprstr[0] = euseq_from_dupstr(NULL, cd->byteWidth,
							  cd->byteWidth);
	if ((*rdata)->reqStartState != C3INT_STATE_INI)
	{
	    if ( ! euseq_append_OK(the_char_encoding->reprstr[0],
				   cd->other_to_ini_state
				   [(*rdata)->reqStartState]))
	    {
		C3VERBOSE(VERB_MINI,
		"FALSE from euseq_append_OK(), UCS char %x (hex)",
		CURRFUNC, ucs)
		return(FALSE);
	    }
	}
	if ( ! euseq_append_eu_OK(the_char_encoding->reprstr[0],
				  (*rdata)->encUnit))
	{
	    C3VERBOSE(VERB_MINI,
		    "FALSE from euseq_append_eu_OK(), UCS char %x (hex)",
		    CURRFUNC, ucs)
	    return(FALSE);
	}
	if ((*rdata)->resultEndState != C3INT_STATE_INI)
	{
	    C3VERBOSE(VERB_MINI,
		    "resultEndState not INI, UCS char %x (hex)",
		    CURRFUNC, ucs)
	    return(FALSE);
	}
    }
    else if (approx != NULL)	/* Approx encoding representation. Assumes
    				 * all exact repr. are set - must be
				 * done in a second pass
				 */
    {
	for (ctype = 0; ctype < cd->ctypes; ctype++)
	{
	    the_char_encoding->conv_quality = C3INT_CONV_APPROX;
				/* Searching from last sequence */
	    t_a_s = approx[ctype];
	    for (i1 = (t_a_s->nextIndex - 1); i1 >= 0; i1--)
	    {
				/* Do all chars in sequence have exact repr? */
		eus = t_a_s->seq[i1];
		euseq_setpos(eus, 0);
		the_char_encoding->reprstr[ctype] =
		    euseq_from_dupstr(NULL, cd->byteWidth,
				      3 * cd->byteWidth);
		for (eu = euseq_geteu(eus); eu != NOT_AN_ENCODING_UNIT;
		     eu = euseq_geteu(eus))
		{
		    t_c_e =_c3_encoding_access(cd, eu);
				/* TODO Exact just stored in ctype=0 */
		    if (t_c_e != NULL
			&& t_c_e->conv_quality == C3INT_CONV_EXACT)
		    {
			euseq_append_OK(the_char_encoding->reprstr[ctype],
					t_c_e->reprstr[0]);
/* TODO Huge kludge, to bring the
 * signal pos inidicating pos_extra
 * to this other euseq (for ctype 3).
 */
			if (eus->pos_extra == eus->pos - eus->bytes_per_eu)
			{	/* Have just read signal pos */
			    euseq *x = the_char_encoding->reprstr[ctype];
			    x->pos_extra = x->endp - x->bytes_per_eu;
			}
		    }
		    else
		    {
			/* TODO Free euseq */
			the_char_encoding->reprstr[ctype] = NULL;
		    }
		}
	    }
			    /* No! */
	    if (the_char_encoding->reprstr[ctype] == NULL)
	    {
		C3VERBOSE(VERB_MINI,
			"Appropriate sequence not found, ucs %x (hex)",
			CURRFUNC, ucs)
		return(FALSE);
	    }
	}
    }
    return(TRUE);
}

#undef CURRFUNC
#define CURRFUNC "approx_plane_ref"

struct char_plane *
approx_plane_ref(apps, group_plane_num, create_new)
    struct char_plane_set *apps;
    int group_plane_num;
    c3bool create_new;
{
    struct char_plane *res;
    int i1;

    for (i1 = 0; i1 < apps->nextIndex; i1++)
    {
	if (apps->plane[i1]->group_plane_num == group_plane_num)
	{
	    return(apps->plane[i1]);
	}
    }
    /* Not found -- create new? */
    if (create_new)
    {
	if (apps->alloc == 0)
	{
	    C3VERBOSE(VERB_DEB1, "Allocating plane pointer space%c",
		    CURRFUNC, ' ')
	    apps->alloc = 2;	/* TODO constant */
	    SAFE_ZALLOCATE(apps->plane, (struct char_plane **),
			   apps->alloc * sizeof(struct char_plane *),
			   return(NULL), CURRFUNC);
	    for (i1 = 0; i1 < apps->alloc; i1++)
	    {
		apps->plane[i1] = NULL;
	    }
	}
	if (apps->nextIndex >= apps->alloc)
	{
	    C3VERBOSE(VERB_MINI,
	    "Internal error, reallocation of plane space not implemented%c",
	    CURRFUNC, ' ')
	    return(NULL);
	}
	SAFE_ZALLOCATE(res, (struct char_plane *),
		      1 * sizeof(struct char_plane),
		      return(NULL), CURRFUNC);
	for (i1 = 0; i1 < 256; i1++)
	{
	    res->char_appr[i1] = NULL;
	}
	res->group_plane_num = group_plane_num;
	apps->plane[apps->nextIndex] = res;
	apps->nextIndex++;
	return(res);
    }
    else
    {
	return(NULL);
    }
}

#undef CURRFUNC
#define CURRFUNC "repr_plane_ref"

struct repr_plane *
repr_plane_ref(reprs, group_plane_num, create_new)
    struct repr_plane_set *reprs;
    int group_plane_num;
    c3bool create_new;
{
    struct repr_plane *res;
    int i1;

    for (i1 = 0; i1 < reprs->nextIndex; i1++)
    {
	if (reprs->plane[i1]->group_plane_num == group_plane_num)
	{
	    return(reprs->plane[i1]);
	}
    }
    /* Not found -- create new? */
    if (create_new)
    {
	if (reprs->alloc == 0)
	{
	    C3VERBOSE(VERB_DEB1, "Allocating plane pointer space%c",
		    CURRFUNC, ' ')
	    reprs->alloc = 2;	/* TODO constant */
	    SAFE_ZALLOCATE(reprs->plane, (struct repr_plane **),
			   reprs->alloc * sizeof(struct repr_plane *),
			   return(NULL), CURRFUNC);
	    for (i1 = 0; i1 < reprs->alloc; i1++)
	    {
		reprs->plane[i1] = NULL;
	    }
	}
	if (reprs->nextIndex >= reprs->alloc)
	{
	    C3VERBOSE(VERB_MINI,
	    "Internal error, reallocation of plane space not implemented%c",
	    CURRFUNC, ' ')
	    return(NULL);
	}
	SAFE_ZALLOCATE(res, (struct repr_plane *),
		      1 * sizeof(struct repr_plane),
		      return(NULL), CURRFUNC);
	for (i1 = 0; i1 < 256; i1++)
	{
	    res->char_repr[i1] = NULL;
	}
	res->group_plane_num = group_plane_num;
	reprs->plane[reprs->nextIndex] = res;
	reprs->nextIndex++;
	return(res);
    }
    else
    {
	return(NULL);
    }
}

#undef CURRFUNC
#define CURRFUNC "computed_ccs_data"

struct ccs_data *
computed_ccs_data(appt, deft)
    struct c3_app_table *appt;
    struct c3_def_table *deft;
{
    struct ccs_data *cd;
    struct parse_data_set **p;
    char_approximation ***alinep, **acolp;
    struct char_plane *aplanep;
    struct repr_plane *rplanep;
    struct repr_data_set ***rlinep, **rcolp;
    int uplane, ulin, ucol;

    SAFE_ZALLOCATE(cd, (struct ccs_data *),
		  sizeof (struct ccs_data), return(NULL), CURRFUNC);

    cd->num = deft->num;
    cd->bitWidth = deft->bitWidth;
    cd->byteWidth = (deft->bitWidth + 7) / 8;
    cd->ctypes = appt->ctypes;
/*    cd->characters = appt->chars; No! */
/*    cd->max_char_prio = 1; TODO */
    cd->states = deft->states;
    switch (cd->byteWidth)
    {
    case 1:
	if (cd->bitWidth == 7)
	{
	    cd->eunitStore = 128;
	}
	else
	{
	    cd->eunitStore = 256;
	}
	break;
    case 2:
	cd->eunitStore = 256 * 256;
	break;
    default:
	_c3_register_outcome(CURRFUNC, C3E_INTERNAL);
	return(NULL);
    }

    SAFE_ZALLOCATE(cd->parse, (struct parse_data_set **),
		   cd->states * cd->eunitStore * 
		   sizeof (struct parse_data_set *),
		   return(NULL), CURRFUNC);
    for (p = cd->parse, ulin = 0; ulin < cd->states*cd->eunitStore;
	 p++, ulin++)
    {
	*p = NULL;
    }
    SAFE_ZALLOCATE(cd->encodings,
		   (struct encoding_plane_set *),
		   1 * sizeof(struct encoding_plane_set),
		   return(NULL), CURRFUNC);
    cd->encodings->plane = NULL;
    /*
     * First pass through all characters - just encoding representation
     */
    for (uplane = 0, rplanep = deft->repres->plane[0];
	 uplane < deft->repres->nextIndex;
	 uplane++, rplanep++)
    {
	for (ulin = 0, rlinep = rplanep->char_repr;
	     ulin < 256;
	     rlinep++, ulin++)
	{

#define APP(ref)  (*alinep == NULL ? NULL : ref)
#define REPR(ref) (*rlinep == NULL ? NULL : ref)

	    if (*rlinep != NULL)
	    {
		C3VERBOSE(VERB_DEB2, "Line %x (hex)", CURRFUNC, ulin)
		for (rcolp = *rlinep, ucol = 0;
		     ucol < 256; ucol++)
		{
		    if (REPR(*rcolp) != NULL)
		    {
			C3VERBOSE(VERB_DEB2, "Col %x, repr", CURRFUNC, ucol)
			if (! encode_admin_OK(*rcolp, NULL,
					      cd, ulin*256+ucol))
			{
			    return(NULL);
			}
		    }
		    if (*rlinep != NULL) rcolp++;
		}
	    }
	}
    }
    /*
     * Second pass through all characters
     */
    cd->characters = 0;
    for (uplane = 0, rplanep = deft->repres->plane[0];
	 uplane < deft->repres->nextIndex;
	 uplane++, rplanep++)
    {
	aplanep = approx_plane_ref(appt->approxes,
				   rplanep->group_plane_num,
				   FALSE);
	for (ulin = 0, rlinep = rplanep->char_repr,
	     alinep = aplanep->char_appr;
	     ulin < 256;
	     rlinep++, alinep++, ulin++)
	{
	    if (*alinep == NULL && *rlinep != NULL)
	    {
		C3VERBOSE(VERB_MINI,
    "Warning: Representations exists but not approximations, line %x (hex)",
		CURRFUNC, ulin)
	    }
	    if (*alinep != NULL || *rlinep != NULL)
	    {
		C3VERBOSE(VERB_DEB2, "Line %x (hex)", CURRFUNC, ulin)
		for (acolp = *alinep, rcolp = *rlinep, ucol = 0;
		     ucol < 256; ucol++)
		{
		    if (*alinep != NULL && *acolp == NULL
			&& REPR(*rcolp) != NULL
			&& (ulin > 0 || ucol > 0x7f))
		    {
			C3VERBOSE(VERB_MINI,
    "Warning: Representations exists but not approximations, UCS-char %x (hex)",
			CURRFUNC, ulin*256+ucol)
		    }
		    C3VERBOSE(VERB_DEB2, "Col %x:", CURRFUNC, ucol)
		    if (REPR(*rcolp) != NULL)
		    {
			cd->characters++;
			C3VERBOSE(VERB_DEB2, "repr%c", CURRFUNC, ' ')
			if (! parse_admin_OK(*rcolp, cd, ulin*256+ucol))
			{
			    return(NULL);
			}
		    }
		    else if (APP(*acolp) != NULL)
		    {
			cd->characters++;
			C3VERBOSE(VERB_DEB2, "appr%c", CURRFUNC, ' ')
			if (! encode_admin_OK(NULL, *acolp,
					      cd, ulin*256+ucol))
			{
			    return(NULL);
			}
		    }
		    if (*alinep != NULL) acolp++;
		    if (*rlinep != NULL) rcolp++;
		}
	    }
	}
    }
    return(cd);
}

c3bool
c3_start_compiling_OK(conv_syst)
    char *conv_syst;
{
    gl.compiling_initiated = TRUE;
    gl.def_tab_num = -1;
    gl.conv_syst = strdup(conv_syst);
    gl.appTable = c3_parsed_approx_table(gl.conv_syst);
    if (gl.appTable == NULL)
    {
	return(BIO_IO_ERR);
    }
    SAFE_ZALLOCATE(gl.bio_meta, (struct bio_meta_data *),
		   1 * sizeof (struct bio_meta_data),
		   return(BIO_IO_ERR), CURRFUNC);
    gl.bio_meta->ccs_spec_alloc = 32;
    SAFE_ZALLOCATE(gl.bio_meta->ccs_spec, (struct bio_ccs_pos_spec **),
		   gl.bio_meta->ccs_spec_alloc * 
		   sizeof (struct bio_ccs_pos_spec *),
		   return(BIO_IO_ERR), CURRFUNC);
    SAFE_ZALLOCATE(gl.bio_meta->ccs_spec[0], (struct bio_ccs_pos_spec *),
		   1 * sizeof (struct bio_ccs_pos_spec),
		   return(BIO_IO_ERR), CURRFUNC);
    gl.bio_meta->ccs_spec[0]->filpos = 0;

    SAFE_ZALLOCATE(gl.bio_meta->ccs_d, (struct ccs_data **),
		   gl.bio_meta->ccs_spec_alloc * 
		   sizeof (struct ccs_data *),
		   return(BIO_IO_ERR), CURRFUNC);
    return(TRUE);
}

#undef CURRFUNC
#define CURRFUNC "c3_compile_OK"

c3bool
c3_compile_OK(what, file_type)
    int what;
    int file_type;
{
    struct c3_def_table *dt;
    struct ccs_data *cd;
    int this_length;

    if ( ! gl.compiling_initiated) {
	C3VERBOSE(VERB_MINI, "Compiling is not initiated%c", CURRFUNC, ' ')
	return(FALSE);
    }
    switch(file_type)
    {
    case C3LIB_FILE_TYPE_SRC_DEFTAB:
    case C3LIB_FILE_TYPE_TGT_DEFTAB:
	dt = c3_parsed_def_table(gl.conv_syst, what);
	if (dt == NULL)
	{
	    C3VERBOSE(VERB_MINI, "Parsing failed%c", CURRFUNC, ' ')
	    return(FALSE);
	}
	C3VERBOSE(VERB_DEB1, "Computing ccs_data%c", CURRFUNC, ' ')
	cd = computed_ccs_data(gl.appTable, dt);
	if (cd == NULL)
	{
	    C3VERBOSE(VERB_MINI, "Computing ccs_data failed%c", CURRFUNC, ' ')
	    return(FALSE);
	}  		
	/*
	 *  Mata ut till tmp-bin-fil...
	 */
	gl.def_tab_num++;
	if (gl.def_tab_num + 1 >= gl.bio_meta->ccs_spec_alloc)
	{
	    C3VERBOSE(VERB_MINI, "ccs_spec overflow%c", CURRFUNC, ' ')
	    return(FALSE);
	}

	SAFE_ZALLOCATE(gl.bio_meta->ccs_spec[gl.def_tab_num + 1],
		       (struct bio_ccs_pos_spec *),
		       1 * sizeof (struct bio_ccs_pos_spec),
		       return(BIO_IO_ERR), CURRFUNC);
	gl.bio_meta->ccs_spec[gl.def_tab_num]->ccs_no = what;
	this_length = bio_ccs_data(0, &cd, BIO_ACTION_LENGTHCOMPUTE);
	if (this_length == BIO_IO_ERR)
	{
	    C3VERBOSE(VERB_MINI, "Binfile ccs_data compute error%c",
		    CURRFUNC, ' ')
	    return(FALSE);
	}
	gl.bio_meta->ccs_spec[gl.def_tab_num + 1]->filpos = 
	    gl.bio_meta->ccs_spec[gl.def_tab_num]->filpos + this_length;
	C3VERBOSE(VERB_DEB1, "filpos %d", CURRFUNC,
		gl.bio_meta->ccs_spec[gl.def_tab_num]->filpos)
	gl.bio_meta->ccs_d[gl.def_tab_num] = cd;
	break;

    case C3LIB_FILE_TYPE_FACTORTAB:
	C3VERBOSE(VERB_MINI,
	"Factor table compilation not yet implemented%c", CURRFUNC, ' ')
	return(FALSE);
        break;

    default:
	C3VERBOSE(VERB_MINI, "Illegal file_type value%c", CURRFUNC, ' ')
	return(FALSE);
	break;
    }
    return(TRUE);
}

#undef CURRFUNC
#define CURRFUNC "c3_end_compiling_OK"

c3bool
c3_end_compiling_OK()
{
    int this_length, tot_length = 0;
    int i1;
    fhandle fh;

    if ( ! gl.compiling_initiated) {
	C3VERBOSE(VERB_MINI, "Compiling is not initiated%c", CURRFUNC, ' ')
	return(FALSE);
    }
    gl.compiling_initiated = FALSE;
    /* free(gl.appTable); */
    /*
     *  Las varje def-bin-fil och mata ut till stora-bin-filen
     */
    gl.bio_meta->format_version = BIO_FORMAT_VERSION;
    gl.bio_meta->conv_system = strdup(gl.conv_syst);
    gl.bio_meta->iso_time = strdup("950711T0105");
    gl.bio_meta->comment = strdup("C3 binary file");
    gl.bio_meta->n_ccs = gl.def_tab_num + 1;
    this_length = bio_bio_meta_data(fh, &gl.bio_meta,
				    BIO_ACTION_LENGTHCOMPUTE);
    C3VERBOSE(VERB_DEB1, "meta_data_storage %d", CURRFUNC,
	    gl.bio_meta->meta_data_storage)
    for (i1 = 0; i1 < gl.bio_meta->n_ccs; i1++)
    {
	gl.bio_meta->ccs_spec[i1]->filpos += this_length;
	C3VERBOSE(VERB_DEB1, "filpos %d", CURRFUNC,
		gl.bio_meta->ccs_spec[i1]->filpos)
    }

    fh = fh_c3_open_hdl(FH_TYPE_BIN, FH_OPEN_BINWRITE, gl.conv_syst, 0);
    if (fh < 0)
    {
	C3VERBOSE(VERB_MINI, "Binfile open error%c", CURRFUNC, ' ')
        return(FALSE);
    }
    this_length = bio_bio_meta_data(fh, &gl.bio_meta, BIO_ACTION_WRITE);
    C3VERBOSE(VERB_DEB1, "meta write %d", CURRFUNC, this_length)
    if (this_length == BIO_IO_ERR)
    {
	C3VERBOSE(VERB_MINI, "Couldn't fetch binfile metadata%c", CURRFUNC, ' ')
        return(FALSE);
    }
    tot_length += this_length;
    for (i1 = 0; i1 < gl.bio_meta->n_ccs; i1++)
    {
	this_length = bio_ccs_data(fh, &(gl.bio_meta->ccs_d[i1]),
				   BIO_ACTION_WRITE);
	C3VERBOSE(VERB_DEB1, "ccs_d write %d", CURRFUNC, this_length)
	if (this_length == BIO_IO_ERR)
	{
	    C3VERBOSE(VERB_MINI, "Binfile ccs_data write error%c",
		    CURRFUNC, ' ')
	    return(FALSE);
	}
	tot_length += this_length;
	C3VERBOSE(VERB_DEB1, "tot_length %d", CURRFUNC, tot_length)
    }
    fh_close_OK(fh);
    return(TRUE);
}

/*
struct cdtabs *
_c3_cdtabs(
    void
)
{
    return(gl.ccsDataTabs);
}
*/
