/* $Id: bin_io.c,v 1.8 2005/05/31 07:36:40 syd Exp $ */

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

#include <stddef.h>
#include <stdio.h>		/* TODO */
#ifdef UNIX
#if !defined(DARWIN)
#include <malloc.h>
#endif
#endif
#include "c3_macros.h"
#include "parse_util.h"
#include "c3.h"
#include "euseq.h"
#include "bin_io.h"

/*
 * These I/O macros assume
 *     int this_length, avaliable to use
 *     int action, with desired value
 *     int tot_length, which value is increased
 *     with the returned value from the I/O function
 */

#define IOint(OBJ) \
    this_length = bio_int(fh, OBJ, action); \
    if (this_length == BIO_IO_ERR) \
    { \
        fprintf(stderr, "IO ERR\n"); \
        return BIO_IO_ERR; \
    } \
    tot_length += this_length

#define IOshort(OBJ) \
    this_length = bio_short(fh, OBJ, action); \
    if (this_length == BIO_IO_ERR) \
    { \
        fprintf(stderr, "IO ERR\n"); \
        return BIO_IO_ERR; \
    } \
    tot_length += this_length

#define IOeuseq(OBJ) \
    this_length = bio_euseq(fh, OBJ, action); \
    if (this_length == BIO_IO_ERR) \
    { \
        fprintf(stderr, "IO ERR\n"); \
        return BIO_IO_ERR; \
    } \
    tot_length += this_length

#define IOparse_data_set(OBJ) \
    this_length = bio_parse_data_set(fh, OBJ, action); \
    if (this_length == BIO_IO_ERR) \
    { \
        fprintf(stderr, "IO ERR\n"); \
        return BIO_IO_ERR; \
    } \
    tot_length += this_length

#define IO1bytes(OBJ, ARG1) \
    this_length = bio_bytes(fh, OBJ, action, ARG1); \
    if (this_length == BIO_IO_ERR) \
    { \
        fprintf(stderr, "IO1 ERR\n"); \
        return BIO_IO_ERR; \
    } \
    tot_length += this_length

#define IO1char_encoding(OBJ, ARG1) \
    this_length = bio_char_encoding(fh, OBJ, action, ARG1); \
    if (this_length == BIO_IO_ERR) \
    { \
        fprintf(stderr, "IO1 ERR\n"); \
        return BIO_IO_ERR; \
    } \
    tot_length += this_length


int
bio_int(fh, object, action)
    fhandle fh;
    int *object;
    int action;
{
    int val;

    switch(action)
    {
    case BIO_ACTION_READ:
	val = fh_byte_read(fh);
	val = (val << 8) | fh_byte_read(fh);
	val = (val << 8) | fh_byte_read(fh);
	val = (val << 8) | fh_byte_read(fh);
	if (fh_latest_error(fh))
	{
	    return BIO_IO_ERR;
	}
	else
	{
	    *object = val;
	    return 4;
	}
	break;

    case BIO_ACTION_WRITE:
	val = *object;
	fh_byte_write(fh, (val >> 24) & 0xff);
	fh_byte_write(fh, (val >> 16) & 0xff);
	fh_byte_write(fh, (val >> 8) & 0xff);
	fh_byte_write(fh, (val) & 0xff);
	if (fh_latest_error(fh))
	{
	    return BIO_IO_ERR;
	}
	else
	{
	    return 4;
	}
	break;

    case BIO_ACTION_LENGTHCOMPUTE:
	return 4;
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
}

int
bio_short(fh, object, action)
    fhandle fh;
    short *object;
    int action;
{
    short val;

    switch(action)
    {
    case BIO_ACTION_READ:
	val = (short)fh_byte_read(fh);
	val = (val << 8) | (short)fh_byte_read(fh);
	val = (val << 8) | (short)fh_byte_read(fh);
	val = (val << 8) | (short)fh_byte_read(fh);
	if (fh_latest_error(fh))
	{
	    return BIO_IO_ERR;
	}
	else
	{
	    *object = val;
	    return 4;
	}
	break;

    case BIO_ACTION_WRITE:
	val = *object;
	fh_byte_write(fh, (val >> 24) & 0xff);
	fh_byte_write(fh, (val >> 16) & 0xff);
	fh_byte_write(fh, (val >> 8) & 0xff);
	fh_byte_write(fh, (val) & 0xff);
	if (fh_latest_error(fh))
	{
	    return BIO_IO_ERR;
	}
	else
	{
	    return 4;
	}
	break;

    case BIO_ACTION_LENGTHCOMPUTE:
	return 4;
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
}

#undef CURRFUNC
#define CURRFUNC "bio_bytes"

int
bio_bytes(fh, object, action, length)
    fhandle fh;
    char **object;
    int action;
    short *length;
{
    int this_length, tot_length = 0;

    switch(action)
    {
    case BIO_ACTION_READ:
	IOshort( length);
	if (*length == 0)
	{
	    *object = NULL;
	    return(tot_length);
	}
	SAFE_ZALLOCATE(*object, (char *),
		       *length * sizeof (char),
		       return(BIO_IO_ERR), CURRFUNC);
	fh_read_bytes(fh, *length, (*object), &this_length);
	if (this_length != *length)
	{
	    return BIO_IO_ERR;
	}
	else
	{
	    return (tot_length + this_length);
	}
	break;

    case BIO_ACTION_WRITE:
	IOshort( length);
	fh_write_bytes(fh, *length, (*object), &this_length);
	if (this_length != *length)
	{
	    return BIO_IO_ERR;
	}
	else
	{
	    return (tot_length + this_length);
	}
	break;

    case BIO_ACTION_LENGTHCOMPUTE:
	return (bio_int(fh, NULL, BIO_ACTION_LENGTHCOMPUTE)
		+ *length);
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
    return BIO_IO_ERR;
}

#undef CURRFUNC
#define CURRFUNC "bio_euseq"

int
bio_euseq(fh, object,action)
    fhandle fh;
    euseq **object;
    int action;
{
    int this_length, tot_length = 0, pos_extra_offset;
    short seq_length, val;

    switch(action)
    {
    case BIO_ACTION_READ:
	IOshort( &val);
	if (val == 0)
	{
	    *object = NULL;
	    return (tot_length);
	}
	SAFE_ZALLOCATE(*object, (euseq *),
		       1 * sizeof (euseq),
		       return(BIO_IO_ERR), CURRFUNC);
	((*object)->bytes_per_eu) = val;

	IOint( &pos_extra_offset);

	this_length = bio_bytes(fh, &((*object)->storage),
				action, &seq_length);
	if (this_length == BIO_IO_ERR)
	{
	    return BIO_IO_ERR;
	}
	tot_length += this_length;
	(*object)->bytes_allocated = (int)seq_length;
	(*object)->endp = (*object)->storage + seq_length;	
	(*object)->pos = (*object)->storage;
	(*object)->pos_extra = (*object)->storage + pos_extra_offset;
	return(tot_length);
	break;

    case BIO_ACTION_WRITE:
    case BIO_ACTION_LENGTHCOMPUTE:

	if (object == NULL || (*object) == NULL)
	{
	    val = 0;		/* Just a zero int */
	    return(bio_short(fh, &val, action));
	}
	seq_length = (*object)->endp - (*object)->storage;
	if ((*object)->bytes_allocated == 0
	    || seq_length == 0)
	{
	    val = 0;		/* Just a zero int */
	    return(bio_short(fh, &val, action));
	}
	val = (*object)->bytes_per_eu;
	IOshort( &val);
	val = (*object)->pos_extra - (*object)->storage;
	IOshort( &val);
	this_length = bio_bytes(fh, &((*object)->storage),
				action, &seq_length);
	if (this_length == BIO_IO_ERR)
	{
	    return BIO_IO_ERR;
	}
	tot_length += this_length;
	return(tot_length);
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
    return BIO_IO_ERR;
}

#undef CURRFUNC
#define CURRFUNC "bio_parse_data_set"

int
bio_parse_data_set(fh, object,action)
    fhandle fh;
    struct parse_data_set **object;
    int action;
/* Format:
       int count;
       {
           int ucs;
	   int nextState;
       } COUNT times
 */
{
    int this_length, tot_length = 0;
    int i1, ucs, last_non_null;
    short nextState;
    short val;

    switch(action)
    {
    case BIO_ACTION_READ:

	SAFE_ZALLOCATE(*object, (struct parse_data_set *),
		       1 * sizeof (struct parse_data_set),
		       return(BIO_IO_ERR), CURRFUNC);
	IOshort( &val);
	SAFE_ZALLOCATE((*object)->pdata, (struct parse_data **),
		       val * sizeof (struct parse_data *),
		       return(BIO_IO_ERR), CURRFUNC);
	(*object)->alloc = val;
	for (i1 = 0; i1 < val; i1++)
	{
	    SAFE_ZALLOCATE((*object)->pdata[i1], (struct parse_data *),
		       val * sizeof (struct parse_data),
		       return(BIO_IO_ERR), CURRFUNC);
	    IOint(&ucs);
	    (*object)->pdata[i1]->ucs = ucs;
	    IOshort(&nextState);
	    (*object)->pdata[i1]->nextState = nextState;
	}
	return(tot_length);
	break;

    case BIO_ACTION_WRITE:
    case BIO_ACTION_LENGTHCOMPUTE:

	last_non_null = -1;
	for (i1 = (*object)->alloc -1; i1 >= 0; i1--)
	{
	    if ((*object)->pdata[i1] != NULL)
	    {
		last_non_null = i1;
		break;
	    }
	}
	if (last_non_null == -1)
	    fprintf(stderr, "last_non_null == -1!?\n");	/* TODO */
	val = last_non_null + 1;
	IOshort( &val);
	for (i1 = 0; i1 < val; i1++)
	{
	    if ((*object)->pdata[i1] == NULL)
	    {
		fprintf(stderr, "Non-continous char_prio?!\n");	/* TODO */
		val = UCS2_NOT_A_CHAR;
		IOshort( &val);
		IOshort( &val);
	    }
	    else
	    {
		val = (short )(*object)->pdata[i1]->ucs;
		IOshort( &val);
		val = (*object)->pdata[i1]->nextState;
		IOshort( &val);
	    }
	}
	return(tot_length);
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
    return BIO_IO_ERR;
}

#undef CURRFUNC
#define CURRFUNC "bio_char_encoding"

int
bio_char_encoding(fh, object, action, ctypes)
    fhandle fh;
    char_encoding **object;
    int action;
    int ctypes;
/* Format:
       short conv_quality;
       euseq reprstr[0];
         :
       euseq reprstr[CTYPES - 1];
 */
{
    int this_length, tot_length = 0;
    int i1;
    short val;

    switch(action)
    {
    case BIO_ACTION_READ:

	SAFE_ZALLOCATE(*object, (char_encoding *),
		       1 * sizeof (char_encoding),
		       return(BIO_IO_ERR), CURRFUNC);
	IOshort(&val);
	(*object)->conv_quality = val;
	SAFE_ZALLOCATE((*object)->reprstr, (euseq **),
		       ctypes * sizeof (euseq *),
		       return(BIO_IO_ERR), CURRFUNC);
	for (i1 = 0; i1 < ctypes; i1++)
	{
	    IOeuseq(&((*object)->reprstr[i1]));
	}
	return(tot_length);
	break;

    case BIO_ACTION_WRITE:
    case BIO_ACTION_LENGTHCOMPUTE:

	val = (*object)->conv_quality;
	IOshort( &val);
	for (i1 = 0; i1 < ctypes; i1++)
	{
	    if ((*object)->reprstr[i1] == NULL)
	    {
/*		fprintf(stderr, "NULL for reprstr[%d]?!\n", i1);
 */		IOeuseq( NULL);/* TODO */
	    }
	    else
	    {
		IOeuseq( &((*object)->reprstr[i1]));
	    }
	}
	return(tot_length);
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
    return BIO_IO_ERR;
}

#undef CURRFUNC
#define CURRFUNC "char_encoding_insert"

void
char_encoding_insert(c_e, e_p_s, ucs)
    char_encoding *c_e;
    struct encoding_plane_set *e_p_s;
    int ucs;
{
    struct encoding_plane *e_pl;
    int i1;

    e_pl = enc_plane_ref(e_p_s, ucs >> 16, TRUE);
    if (e_pl->char_encod[ucs / 256] == NULL)
    {
        char_encoding **t_c_ep;

        SAFE_ALLOCATE(t_c_ep, (char_encoding **),
                      256 * sizeof(char_encoding *),
                      return, CURRFUNC);
        for (i1 = 0; i1 < 256; i1++)
        {
            t_c_ep[i1] = NULL;
        }
	e_pl->char_encod[ucs / 256] = t_c_ep;
    }
    if (e_pl->char_encod[ucs / 256][ucs % 256] == NULL)
    {
	e_pl->char_encod[ucs / 256][ucs % 256] = c_e;
    }
    else
    {
	fprintf(stderr, "NON-null when inserting ucs %x\n", ucs);
    }

}


#undef CURRFUNC
#define CURRFUNC "bio_ccs_data"

int
bio_ccs_data(fh, object, action)
    fhandle fh;
    struct ccs_data **object;
    int action;
/* Format:
       int num;
       int bitWidth;
       int byteWidth;
       int ctypes;
       int eunitStore;
       int states;
       int characters;
       euseq other_to_ini_state[1]
           :
       euseq other_to_ini_state[states - 1]	   
       parse_data_set parse[0]
           :
       parse_data_set parse[(states * eunitStore) - 1]
       int planes;		Number of different planes
       { int ucs;
         char_encoding enc;
       } CHARACTERS times
 */

{
    int this_length, tot_length = 0;
    int i1, lin, col, ucs, char_num;
    short val;
    struct encoding_plane *pl;
    char_encoding *c_e;

    switch(action)
    {
    case BIO_ACTION_READ:

	SAFE_ZALLOCATE(*object, (struct ccs_data *),
		       1 * sizeof (struct ccs_data),
		       return(BIO_IO_ERR), CURRFUNC);
	IOint( &((*object)->num));
	IOint( &((*object)->bitWidth));
	IOint( &((*object)->byteWidth));
	IOint( &((*object)->ctypes));
	IOint( &((*object)->eunitStore));
	IOint( &((*object)->states));
	IOint( &((*object)->characters));
	SAFE_ZALLOCATE((*object)->other_to_ini_state, (euseq **),
		       (*object)->states * sizeof (euseq *),
		       return(BIO_IO_ERR), CURRFUNC);
				/* Start index 1 -- 0 not used! */
	for (i1 = 1; i1 < (*object)->states; i1++)
	{
	    IOeuseq(&((*object)->other_to_ini_state[i1]));
	}
	val = (*object)->states * (*object)->eunitStore;
	SAFE_ZALLOCATE((*object)->parse, (struct parse_data_set **),
		       val * sizeof (struct parse_data_set *),
		       return(BIO_IO_ERR), CURRFUNC);
	for (i1 = 0; i1 < val ; i1++)
	{
	    IOparse_data_set(&((*object)->parse[i1]));
	}
	SAFE_ZALLOCATE((*object)->encodings, (struct encoding_plane_set *),
		       1 * sizeof (struct encoding_plane_set),
		       return(BIO_IO_ERR), CURRFUNC);
	IOshort( &val);		/* planes */
	SAFE_ZALLOCATE((*object)->encodings->plane,
		       (struct encoding_plane **),
		       val * sizeof (struct encoding_plane *),
		       return(BIO_IO_ERR), CURRFUNC);
	(*object)->encodings->alloc = val;
	(*object)->encodings->nextIndex = 0;
	val = (short)(*object)->characters;
	for (i1 = 0; i1 < val; i1++)
	{
	    IOint (&ucs);
	    IO1char_encoding( &c_e, (*object)->ctypes);
	    char_encoding_insert(c_e, (*object)->encodings, ucs);
	}
	return(tot_length);
	break;

    case BIO_ACTION_WRITE:
    case BIO_ACTION_LENGTHCOMPUTE:

	IOint( &((*object)->num));
	IOint( &((*object)->bitWidth));
	IOint( &((*object)->byteWidth));
	IOint( &((*object)->ctypes));
	IOint( &((*object)->eunitStore));
	IOint( &((*object)->states));
	IOint( &((*object)->characters));
				/* Start index 1 -- 0 not used! */
	for (i1 = 1; i1 < (*object)->states; i1++)
	{
	    IOeuseq( &((*object)->other_to_ini_state[i1]));
	}
	val = (*object)->states * (*object)->eunitStore;
	for (i1 = 0; i1 < val ; i1++)
	{
	    IOparse_data_set( &((*object)->parse[i1]));
	}
	char_num = 0;
	val = (*object)->encodings->nextIndex;
	IOshort( &val);
	for (i1 = 0; i1 < val ; i1++)
	{
	    pl = (*object)->encodings->plane[i1];
	    for (lin = 0; lin < 256 ; lin++)
	    {
		if (pl->char_encod[lin] != NULL)
		{
		    for (col = 0; col < 256 ; col++)
		    {
			if (pl->char_encod[lin][col] != NULL)
			{
			    char_num++;
			    ucs = lin * 256 + col;
			    IOint( &ucs);
			    IO1char_encoding( 
			       &(pl->char_encod[lin][col]),
			       (*object)->ctypes);
			}
		    }
		}
	    }
	}
	if (char_num != (*object)->characters)
	{
	    fprintf(stderr, "char != characters, %d, %d\n",
		    char_num, (*object)->characters);
	}
	return(tot_length);
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
    return BIO_IO_ERR;
}

/*
 * free all this allocated meta_data --tcj
 */

void
bio_free_bio_meta_data(object)
    struct bio_meta_data *object;
{
    int i1;

    if (object == NULL)
	return;
    if (object->ccs_spec) {
	free (object->conv_system);
	free (object->iso_time);
	free (object->comment);
	for (i1 = 0; i1 < object->n_ccs; i1++) {
	    if (object->ccs_spec[i1])
		free(object->ccs_spec[i1]);
	}
	free (object->ccs_spec);
    }
    free(object);
}

#undef CURRFUNC
#define CURRFUNC "bio_bio_meta_data"
/*
 * memory is only allocated on when action is READ  --tcj
 */

int
bio_bio_meta_data(fh, object, action)
    fhandle fh;
    struct bio_meta_data **object;
    int action;
/* Format:
       string "aozo";		Magic string
       int format_version;
       int meta_data_storage;
       bytes conv_system;
       bytes iso_time;
       bytes comment;
       int n_ccs;
       int ccs_spec[0]->ccs_no;
       int ccs_spec[0]->filpos;
         :
       int ccs_spec[n_ccs - 1]->ccs_no;
       int ccs_spec[n_ccs - 1]->filpos;
 */
{
    int this_length, tot_length = 0;
    int i1;
    short val, dummy;
    char magic[5];

    switch(action)
    {
    case BIO_ACTION_READ:

	fh_read_bytes(fh, 4, magic, &this_length);
	if (this_length != 4)
	{
	    return BIO_IO_ERR;
	}
	tot_length += this_length;
	if (magic[0] != 'a' || magic[1] != 'o'
	    || magic[2] != 'z' || magic[3] != 'o')
	{
	    fprintf(stderr, "Illegal magic string\n");
	    return BIO_IO_ERR;
	}
	SAFE_ZALLOCATE(*object, (struct bio_meta_data *),
		       1 * sizeof (struct bio_meta_data),
		       return(BIO_IO_ERR), CURRFUNC);
	IOint( &((*object)->format_version));
	if ((*object)->format_version != BIO_FORMAT_VERSION)
	{
	    fprintf(stderr, "Illegal format version\n");
	    return BIO_IO_ERR;
	}
	IOint( &((*object)->meta_data_storage));
	IO1bytes( &((*object)->conv_system), &dummy);
	IO1bytes( &((*object)->iso_time), &dummy);
	IO1bytes( &((*object)->comment), &dummy);
	IOint( &((*object)->n_ccs));
	SAFE_ZALLOCATE((*object)->ccs_spec, (struct bio_ccs_pos_spec **),
		       (*object)->n_ccs * sizeof (struct bio_ccs_pos_spec *),
		       return(BIO_IO_ERR), CURRFUNC);
	for (i1 = 0; i1 < (*object)->n_ccs; i1++)
	{
	    SAFE_ZALLOCATE((*object)->ccs_spec[i1],
			   (struct bio_ccs_pos_spec *),
			   1 * sizeof (struct bio_ccs_pos_spec),
			   return(BIO_IO_ERR), CURRFUNC);
	    IOint( &((*object)->ccs_spec[i1]->ccs_no));
	    IOint( &((*object)->ccs_spec[i1]->filpos));
	}
	return(tot_length);
	break;
	
    case BIO_ACTION_WRITE:
    case BIO_ACTION_LENGTHCOMPUTE:

	if (action == BIO_ACTION_WRITE)
	{
	    fh_write_bytes(fh, 4, "aozo", &this_length);
	    if (this_length != 4)
	    {
		return BIO_IO_ERR;
	    }
	}
	else
	{
	    this_length = 4;
	}
	tot_length += this_length;
	IOint( &((*object)->format_version));
	(*object)->meta_data_storage = 
	      4		/* Magic string */
	    + 4		/* format_version */
  	    + 4		/* meta_data_storage */
	    + strlen((*object)->conv_system) + 5 /* length, conv_system+NULL */
	    + strlen((*object)->iso_time) + 5 /* length, iso_time+NULL */
	    + strlen((*object)->comment) + 5 /* length, comment+NULL */
	    + 4		/* n_ccs */
	    + 4 * 2 * (*object)->n_ccs;	/* ccs_spec */
	IOint( &((*object)->meta_data_storage));
	val = strlen((*object)->conv_system) + 1;
	IO1bytes( &((*object)->conv_system), &val);
	val = strlen((*object)->iso_time) + 1;
	IO1bytes( &((*object)->iso_time), &val);
	val = strlen((*object)->comment) + 1;
	IO1bytes( &((*object)->comment), &val);
	IOint( &((*object)->n_ccs));
	for (i1 = 0; i1 < (*object)->n_ccs; i1++)
	{
	    IOint( &((*object)->ccs_spec[i1]->ccs_no));
	    IOint( &((*object)->ccs_spec[i1]->filpos));
	}
	if (tot_length != (*object)->meta_data_storage)
	{
	    fprintf(stderr, "tot_length != meta_data_storage, %d, %d\n",
		    tot_length, (*object)->meta_data_storage);
	}
	return(tot_length);
	break;

    default:
	return BIO_IO_ERR;
	break;
    }
    return BIO_IO_ERR;
}

