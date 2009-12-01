
static char file_id[] =
    "$Id: api_util.c,v 1.15 2005/05/31 07:36:40 syd Exp $";


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
#ifdef UNIX 
#if !defined(DARWIN)
#include <malloc.h>
#endif
#endif

#include "c3_string.h"
#include "c3_macros.h"
#include "c3.h"
#include "euseq.h"
#include "lib.h"
#include "parse_util.h"
#include "fh.h"
#include "bin_io.h"

extern int opt_verbosity;

#undef CURRFUNC
#define CURRFUNC "enc_plane_ref"

struct encoding_plane *
enc_plane_ref(encs, group_plane_num, create_new)
    struct encoding_plane_set *encs;
    int group_plane_num;
    int create_new;		/* was c3bool --bg */
{
    struct encoding_plane *res;
    int i1;

    for (i1 = 0; i1 < encs->nextIndex; i1++)
    {
	if (encs->plane[i1]->group_plane_num == group_plane_num)
	{
	    return(encs->plane[i1]);
	}
    }
    /* Not found -- create new? */
    if (create_new)
    {
	if (encs->alloc == 0)
	{
	    C3VERBOSE(VERB_DEB1, "Allocating plane pointer space%c",
		    CURRFUNC, ' ')
	    encs->alloc = 2;	/* TODO constant */
	    SAFE_ZALLOCATE(encs->plane, (struct encoding_plane **),
			   encs->alloc * sizeof(struct encoding_plane *),
			   return(NULL), CURRFUNC);
	    for (i1 = 0; i1 < encs->alloc; i1++)
	    {
		encs->plane[i1] = NULL;
	    }
	}
	if (encs->nextIndex >= encs->alloc)
	{
	    C3VERBOSE(VERB_MINI,
	    "Internal error, reallocation of plane space not implemented%c",
	    CURRFUNC, ' ')
	    encs->alloc = 0;
	    free(encs->plane);
	    return(NULL);
	}
	SAFE_ZALLOCATE(res, (struct encoding_plane *),
		      1 * sizeof(struct encoding_plane),
		      return(NULL), CURRFUNC);
	for (i1 = 0; i1 < 256; i1++)
	{
	    res->char_encod[i1] = NULL;
	}
	res->group_plane_num = group_plane_num;
	encs->plane[encs->nextIndex] = res;
	encs->nextIndex++;
	return(res);
    }
    else
    {
	return(NULL);
    }
}

#undef CURRFUNC
#define CURRFUNC "_c3_encoding_access"

char_encoding *
_c3_encoding_access(ccsData, ucs)
    struct ccs_data *ccsData;
    enc_unit ucs;
{
    struct encoding_plane *e_p;

    e_p = enc_plane_ref(ccsData->encodings, ucs >> 16, FALSE);
    if (e_p == NULL || e_p->char_encod[ucs / 256] == NULL)
    {
	return(NULL);
    }
    else
    {
	return(e_p->char_encod[ucs / 256][ucs % 256]);
    }
}

static struct ccs_data *ccs_data_start = NULL;

void
free_all_ccs_data()
{
    struct ccs_data *ccs_data_ptr = ccs_data_start;
    struct ccs_data *temp_ccs_data_ptr;

    while (ccs_data_ptr != NULL) {
	temp_ccs_data_ptr = ccs_data_ptr;
  	ccs_data_ptr = ccs_data_ptr->next;
#ifdef NOT_DONE_YET
	bio_free_bio_ccs_data(temp_ccs_data_ptr);
#endif
    }
}

#undef CURRFUNC
#define CURRFUNC "to_cache"

static void
to_cache(ccsdata)
    struct ccs_data *ccsdata;
{
    static struct ccs_data *ccs_data_last_allocated;

    if (ccsdata) {
	if (ccs_data_last_allocated == NULL) {
	    ccs_data_start = ccsdata;
            ccsdata->next = NULL;
        } else
	    ccs_data_last_allocated->next = ccsdata;
	ccs_data_last_allocated = ccsdata;
	ccsdata->next = NULL;
    }
}

#undef CURRFUNC
#define CURRFUNC "from_cache"

static struct ccs_data *
from_cache(ccs_num)
    int ccs_num;
{
    struct ccs_data *ccs_data_ptr = ccs_data_start;

    while (ccs_data_ptr != NULL && ccs_data_ptr->num != ccs_num) {
  	ccs_data_ptr = ccs_data_ptr->next;
    }
    return(ccs_data_ptr);
}

static struct bio_meta_data *bio_meta = NULL;

int
get_bio_meta_ccs_index(ccs_num) 
    int ccs_num;
{
    int i1, meta_i = -1;

    if (bio_meta != NULL)
	{
	for (i1 = 0; i1 < bio_meta->n_ccs; i1++)
	    {
	    if (bio_meta->ccs_spec[i1]->ccs_no == ccs_num)
		{
		meta_i = i1;
		break;
		}
	    }
	}
    return(meta_i);
}

#undef CURRFUNC
#define CURRFUNC "fetched_ccsData"

struct ccs_data *
fetched_ccsData(conv_syst, ccs_num)
    const char *conv_syst;
    int ccs_num;
{
    struct ccs_data *res;
    int i1, this_length, meta_i;
    static int firsttime = TRUE;
    static fhandle fh;

    if ((res = from_cache(ccs_num)) != NULL)
    {
	return(res);
    }

    fh = fh_c3_open_hdl(FH_TYPE_BIN, FH_OPEN_BINREAD, conv_syst, 0);

    if (firsttime) {
	firsttime = FALSE;
    	this_length = bio_bio_meta_data(fh, &bio_meta, BIO_ACTION_READ);
    	if (this_length == BIO_IO_ERR)
	{
	    C3VERBOSE(VERB_MINI, "Couldn't fetch binfile metadata%c", CURRFUNC, ' ')
            return NULL;
	}
    }

    meta_i = get_bio_meta_ccs_index(ccs_num);
     
    if (meta_i < 0)
    {
	C3VERBOSE(VERB_MINI, "CCS %d not found in binary file", CURRFUNC,
		ccs_num)
        return NULL;
    }
    C3VERBOSE(VERB_DEB1, "Positioning to %d", CURRFUNC,
	    bio_meta->ccs_spec[meta_i]->filpos)
    if ( ! fh_position_OK(fh, bio_meta->ccs_spec[meta_i]->filpos))
    {
	C3VERBOSE(VERB_MINI, "Couldn't position binfile%c", CURRFUNC, ' ')
        return NULL;
    }
    this_length = bio_ccs_data(fh, &res, BIO_ACTION_READ);
    if (this_length == BIO_IO_ERR)
    {
	C3VERBOSE(VERB_MINI, "Couldn't read binfile ccs data%c", CURRFUNC, ' ')
        return NULL;
    }

    to_cache(res);

    if (res->num != ccs_num)
    {
	C3VERBOSE(VERB_MINI, "Binfile contents/position error, looked for %d",
		CURRFUNC, ccs_num)
	C3VERBOSE(VERB_MINI, "found num == %d", CURRFUNC, res->num)
        return NULL;
    }

    fh_close_OK(fh);	
    return(res);
}

#undef CURRFUNC
#define CURRFUNC "_c3_update_convinfo_OK"

c3bool
_c3_update_convinfo_OK(conv_syst, stream)
    char *conv_syst;	/* Conversion system */
    c3_stream *stream;
{
    struct parse_data_set **p_dataTp;
    struct parse_data **p_datap;
    struct wtableinfo *w_data;
    char_encoding *t_c_e;
    int srci, statei, char_prio;
    short q;
    size_t alloclen;
    static size_t last_alloclen = 0;

    /* Fetch table data */

    if (stream->src_ccs == NULL
	|| stream->src_ccs->num != stream->src_num)
    {
	stream->src_ccs = fetched_ccsData(conv_syst, stream->src_num);
	if (stream->src_ccs == NULL)
	{
	    C3VERBOSE(VERB_MINI, "Couldn't fetch src%c", CURRFUNC, ' ')
	    return(FALSE);
	}
    }
    if (stream->tgt_ccs == NULL
	|| stream->tgt_ccs->num != stream->tgt_num)
    {
	stream->tgt_ccs = fetched_ccsData(conv_syst, stream->tgt_num);
	if (stream->tgt_ccs == NULL)
	{
	    C3VERBOSE(VERB_MINI, "Couldn't fetch tgt%c", CURRFUNC, ' ')
	    return(FALSE);
	}
    }

    /* TODO Apply line ending handling etc. */

/* TODO    _c3_update_for_linebreak(stream); */

    /*
     * Combine the ccs_data structs into one work_data struct
     */
    stream->max_outstr_length = -1;
    stream->states = stream->src_ccs->states;
    alloclen =
	    (stream->states * stream->src_ccs->eunitStore)
		* sizeof (struct wtableinfo);
    if (stream->wtable == NULL) 
    {
	SAFE_ZALLOCATE(stream->wtable, (struct wtableinfo *),
		       alloclen, return(FALSE), CURRFUNC);
	last_alloclen = alloclen;
    }
    else if (last_alloclen < alloclen)
    {
	free(stream->wtable);
	SAFE_ZALLOCATE(stream->wtable, (struct wtableinfo *),
		       alloclen, return(FALSE), CURRFUNC);
	last_alloclen = alloclen;
    }

    /* TODO Check allocation? */
    p_dataTp = stream->src_ccs->parse;
    w_data = stream->wtable;
    for (statei = 0; statei < stream->states; statei++)
    {
	for (srci=0; srci < stream->src_ccs->eunitStore; srci++)
	{
	    if (/* TODO src is not UCS && */ *p_dataTp == NULL)
	    {
		C3VERBOSE(VERB_MINI, "Missing parse spec of src eu %x",
			CURRFUNC, srci)
		C3VERBOSE(VERB_MINI, "in state %d", CURRFUNC, statei)
	    }
	    else
	    {
				/* Studying a certain src eu: */

				/* The first character it can represent */
		q = C3INT_CONV_NOT;
		for (p_datap = (*p_dataTp)->pdata, char_prio = 0;
		     char_prio < (*p_dataTp)->alloc;
		     p_datap++, char_prio++)
		{
		    if (p_datap == NULL) /* Not used char_prio */
		    {
			continue;
		    }
		    t_c_e =_c3_encoding_access(stream->tgt_ccs,
					       (*p_datap)->ucs);
		    if (t_c_e == NULL)
		    {
			C3VERBOSE(VERB_MINI,
			"Internal error, encoding data missing for src eu %x",
				CURRFUNC, srci)
			C3VERBOSE(VERB_MINI, "in state %d", CURRFUNC, statei)
			C3VERBOSE(VERB_MINI, "tgt ucs %d", CURRFUNC,
				(*p_datap)->ucs)
			return(FALSE);
		    }
		    q = w_data->conv_quality = t_c_e->conv_quality;

				/* ...which exists in target */
		    if (q == C3INT_CONV_EXACT)
		    {

				/* ... -- use! */
			w_data->resultState =
			    /* TODO src is UCS? 0: */ (*p_datap)->nextState;
			w_data->seq = t_c_e->reprstr[0]; /* TODO */
			break;
		    }
		}
		if (q != C3INT_CONV_EXACT) /* Not found -- use approx */
		{
		    p_datap = (*p_dataTp)->pdata;
		    t_c_e =_c3_encoding_access(stream->tgt_ccs,
					       (*p_datap)->ucs);
		    if (t_c_e == NULL)
		    {
			C3VERBOSE(VERB_MINI,
			"Internal error, encoding data missing for src eu %x",
				CURRFUNC, srci)
			C3VERBOSE(VERB_MINI, "in state %d", CURRFUNC, statei)
			C3VERBOSE(VERB_MINI, "tgt ucs %d", CURRFUNC,
				(*p_datap)->ucs)
			return(FALSE);
		    }
		    if (t_c_e->conv_quality ==
			C3INT_CONV_APPROX)
		    {
			w_data->resultState =
			    /* TODO src is UCS? 0: */ (*p_datap)->nextState;
			w_data->seq = t_c_e->reprstr[stream->ctype - 1];
		    }
		    else
		    {
			C3VERBOSE(VERB_MINI,
				"Internal error, unknown quality %d",
				CURRFUNC, t_c_e->conv_quality)
			C3VERBOSE(VERB_MINI, "src eu %x",	CURRFUNC, srci)
		        C3VERBOSE(VERB_MINI, "in state %d", CURRFUNC, statei)
			C3VERBOSE(VERB_MINI, "ctype %x",	CURRFUNC, 
				stream->ctype)
			return(FALSE);
		    }
		}

	    }

	    p_dataTp++;
	    w_data++;
	}
    }
    stream->data_complete = TRUE;
    return(TRUE);
}
