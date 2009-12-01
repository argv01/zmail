
static char file_id[] =
    "$Id: euseq.c,v 1.10 1995/08/18 18:34:27 spencer Exp $";

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
#include "c3_macros.h"
#include "euseq.h"
#include "c3_string.h"


/* Global variables
 *
 */

static int g_extra_allocate;

void
euseq_set_extra_allocate(value)
    int value;
{
    if (value >= 0)
    {
	g_extra_allocate = value;
    }
}

int
_euseq_nullstr_bytelength(str, bytes_per_eu)
    const char *str;
    short bytes_per_eu;
{
    switch(bytes_per_eu)
    {

    case 1:
	return(strlen(str));
	break;

    case 2:
    {
	int i;

	for (i = 0; *(str+i) != 0 || *(str+i+1) != 0; i += 2)
	{
	    ;
	}
	return(i);
	break;
    }

    default:
	fprintf(stderr, "Illegal bytes per eu %d\n", bytes_per_eu);
	return(-1);
	break;
    }

}


euseq *
euseq_from_str(str, bytes_per_eu, number_of_bytes)
    const char *str;
    int bytes_per_eu;		/* was short --bg */
    int number_of_bytes;
{
    euseq *res;

    res = (euseq *) malloc(sizeof(euseq));
    if (res == NULL)
    {
	return(NULL);
    }

    res->storage = (char *)str;
    res->bytes_per_eu = bytes_per_eu;
    res->pos = res->storage;
    res->pos_extra = res->storage - res->bytes_per_eu;
    if (number_of_bytes <= 0)
    {
	number_of_bytes = _euseq_nullstr_bytelength(str, res->bytes_per_eu);
    }
    res->bytes_allocated = number_of_bytes + res->bytes_per_eu;
    res->endp = res->storage + number_of_bytes;
    return(res);
}


euseq *
euseq_from_dupstr(str, bytes_per_eu, alloc_eu_count)
    char *str;
    int bytes_per_eu;		/* was short --bg */
    int alloc_eu_count;
{
    euseq *res;
    int str_bytes, alloc_eu, no_eus;

    res = (euseq *) malloc(sizeof(euseq));
    if (res == NULL)
    {
	return(NULL);
    }

    res->bytes_per_eu = bytes_per_eu;
    if (str == NULL)
    {
	if (alloc_eu_count <= 0)
	{
	    /* TODO Error */
	    return(NULL);	
	}
	alloc_eu = alloc_eu_count;
	no_eus = 0;
    }
    else
    {
	/* Assuming normal str, i.e. 1 byte per encoding unit */
	str_bytes = _euseq_nullstr_bytelength(str, 1);
	alloc_eu = MAX(str_bytes, alloc_eu_count);
	no_eus = str_bytes;
    }
    res->bytes_allocated = (alloc_eu + g_extra_allocate) * res->bytes_per_eu;
    res->storage = malloc(res->bytes_allocated);
    if (res->storage == NULL)
    {
	return(NULL);
    }
    if (str != NULL)
    {
	switch(res->bytes_per_eu)
	{
	case 1:
	    COPY_MEMORY(res->storage, str, str_bytes);
	    break;

	case 2:
	    {
		char *cp; int i;

		for (i = 0, cp = res->storage; *(str+i) != '\0'; i++, cp += 2)
		{
		    /* TODO OK? */
		    *cp = *(str+i) / 256;
		    *(cp + 1) = *(str+i) % 256;
		}
		break;
	    }

	default:
	    fprintf(stderr, "Illegal bytes per eu %d\n", res->bytes_per_eu);
	    return(NULL);
	    break;
	}
    }
    res->pos = res->storage;
    res->pos_extra = res->storage - res->bytes_per_eu;
    res->endp = res->storage + no_eus * res->bytes_per_eu;
    return(res);
}

enc_unit
euseq_geteu(eusp)
    euseq *eusp;
{
    enc_unit res;

    if (eusp->pos >= eusp->endp)
    {
	return(NOT_AN_ENCODING_UNIT);
    }
    switch(eusp->bytes_per_eu)
    {

    case 1:
	res = BYTEMASK(*(eusp->pos));
	break;

    case 2:
	res = 256 * (BYTEMASK(*(eusp->pos))) + BYTEMASK(*(eusp->pos + 1));
	break;

    default:
	fprintf(stderr, "Illegal bytes per eu %d\n", eusp->bytes_per_eu);
	return(NOT_AN_ENCODING_UNIT);
	break;
    }
    eusp->pos += eusp->bytes_per_eu;
    return(res);
}

enc_unit
euseq_geteu_at(eusp, eu_pos)
    euseq *eusp;
    int eu_pos;
{
    int res;
    char *pos = eusp->storage + eu_pos * eusp->bytes_per_eu;

    if (pos >= eusp->endp)
    {
	return(NOT_AN_ENCODING_UNIT);
    }
    switch(eusp->bytes_per_eu)
    {

    case 1:
	res = BYTEMASK(*(pos));
	break;

    case 2:
	res = 256 * BYTEMASK(*(pos)) + BYTEMASK(*(pos + 1));
	break;

    default:
	fprintf(stderr, "Illegal bytes per eu %d\n", eusp->bytes_per_eu);
	return(NOT_AN_ENCODING_UNIT);
	break;
    }
    return(res);
}

void
euseq_setpos(eus, eu_pos)
    euseq *eus;
    int eu_pos;
{
    /* TODO Error if outside? */
    eus->pos = eus->storage + eu_pos * eus->bytes_per_eu;
}

void
euseq_setepos(eus, eu_pos)
    euseq *eus;
    int eu_pos;
{
    eus->pos_extra = eus->storage + eu_pos * eus->bytes_per_eu;
}


void
euseq_puteu(eus, eu)
    euseq *eus;
    enc_unit eu;
{
    if (eus->pos + eus->bytes_per_eu > eus->storage + eus->bytes_allocated)
    {
	/* TODO Error */
	fprintf(stderr, "euseq_puteu: Overflow\n");
	return;
    }
    switch(eus->bytes_per_eu)
    {

    case 1:
	/* TODO What if EU > 256? */
	*(eus->pos) = eu % 256;
	break;

    case 2:
	/* TODO OK? */
	*(eus->pos) = eu / 256;
	*(eus->pos + 1) = eu % 256;
	break;

    default:
	fprintf(stderr, "Illegal bytes per eu %d\n", eus->bytes_per_eu);
	return;
	break;
    }
    eus->pos += eus->bytes_per_eu;
    if (eus->pos > eus->endp)
    {
	eus->endp = eus->pos;
    }
}


int
euseq_cmp(eus1, eus2)
    euseq *eus1;
    euseq *eus2;
{
    char *pos1, *pos2;
    enc_unit eunit1, eunit2;
    int res = 0;

    pos1 = eus1->pos;
    pos2 = eus2->pos;
    euseq_setpos(eus1, 0);
    euseq_setpos(eus2, 0);

    for(eunit1 = euseq_geteu(eus1), eunit2 = euseq_geteu(eus2);
	eunit1 != NOT_AN_ENCODING_UNIT && eunit2 != NOT_AN_ENCODING_UNIT
	&& res == 0;
	eunit1 = euseq_geteu(eus1), eunit2 = euseq_geteu(eus2))
    {
	if (eunit1 < eunit2) res = -2;
	if (eunit1 > eunit2) res = 1;
    }
    if (res == 0)
    {
	if (eunit1 != NOT_AN_ENCODING_UNIT && eunit2 == NOT_AN_ENCODING_UNIT)
	{
	    res = 1;
	}
	else if (eunit2 != NOT_AN_ENCODING_UNIT
		 && eunit1 == NOT_AN_ENCODING_UNIT)
	{
	    res = -1;
	}
    }
    eus1->pos = pos1;
    eus2->pos = pos2;
    return res;
}

void
euseq_output(eus)
    euseq *eus;
{
    char *byp;

    if (eus == NULL)
    {
	fprintf(stderr, "NULL-euseq\n");
	return;
    }
    fprintf(stderr, "bpos=%d bend=%d", eus->pos - eus->storage,
	    eus->endp - eus->storage);
    if (eus->storage == eus->endp)
    {
	fprintf(stderr, " <empty>");
    }
    for(byp=eus->storage; byp < eus->endp; byp++)
    {
	if (byp == eus->pos_extra)
	{
	    fprintf(stderr, " *[\"%c\" '%02x]", BYTEMASK(*byp), BYTEMASK(*byp));
	}
	else
	{
	    fprintf(stderr, " [\"%c\" '%02x]", BYTEMASK(*byp), BYTEMASK(*byp));
	}
    }
    fprintf(stderr, ".\n");
}


int
euseq_append_OK(eus_tgt, eus_src)
    euseq *eus_tgt;
    euseq *eus_src;
{
    int old_no_bytes, new_no_bytes, bytes_to_copy;

    if (FALSE)
    {
	printf("append_txt: <");
	euseq_output(eus_tgt);
	printf(">(%d) <", eus_tgt->endp - eus_tgt->storage);
	euseq_output(eus_src);
	printf(">(%d) <", eus_src->endp - eus_src->storage);
    }

    if (eus_tgt == NULL || eus_src == NULL
	|| eus_src->bytes_per_eu !=  eus_tgt->bytes_per_eu)
    {
	/* TODO Error */
	return(FALSE);
    }
    bytes_to_copy = eus_src->endp - eus_src->storage;
    old_no_bytes = eus_tgt->endp - eus_tgt->storage;
    new_no_bytes = bytes_to_copy + old_no_bytes;
    if (new_no_bytes <= eus_tgt->bytes_allocated)
    {
	COPY_MEMORY(eus_tgt->endp, eus_src->storage, bytes_to_copy);
	eus_tgt->endp += bytes_to_copy;
    }
    else
    {
	char *newstorage;
	int bytes_to_allocate;
	int old_int_pos, old_int_pos_extra;

	bytes_to_allocate = new_no_bytes + g_extra_allocate;
	newstorage = malloc(bytes_to_allocate);
	if (newstorage == NULL)
	{
	    /* TODO Error */
	    return(FALSE);
	}
	COPY_MEMORY(newstorage, eus_tgt->storage, old_no_bytes);
	COPY_MEMORY(newstorage + old_no_bytes,
		    eus_src->storage, bytes_to_copy);
	old_int_pos = eus_tgt->pos - eus_tgt->storage;
	old_int_pos_extra = eus_tgt->pos_extra - eus_tgt->storage;
	free(eus_tgt->storage);
	eus_tgt->storage = newstorage;
	eus_tgt->bytes_allocated = bytes_to_allocate;
	eus_tgt->endp = eus_tgt->storage + old_no_bytes + bytes_to_copy;
	eus_tgt->pos = eus_tgt->storage + old_int_pos;
	eus_tgt->pos_extra = eus_tgt->storage + old_int_pos_extra;
    }
    /* TODO Find a better solution */
    /* Special for the string-appended-to-SIGNAL case:
     * If src extrapos is default and tgt extrapos is non-default
     * then use tgt extrapos in target, and vice versa.
     */
    /* TODO: If both is non-default, then what? Error? */

    if (eus_tgt->pos_extra == eus_tgt->storage - eus_tgt->bytes_per_eu
	&& eus_src->pos_extra != (eus_src->storage - eus_src->bytes_per_eu))
    {
	eus_tgt->pos_extra += (eus_src->pos_extra - eus_src->storage)
	    + old_no_bytes;
    }
    return(TRUE);
}

int
euseq_append_eu_OK(eus_tgt, eu)		/* Append an eu */
    euseq *eus_tgt;
    enc_unit eu;
{
    euseq *t;

    /* Quick and dirty */
    t = euseq_from_dupstr(NULL, eus_tgt->bytes_per_eu,
			  eus_tgt->bytes_per_eu);
    euseq_puteu(t, eu);
    return(euseq_append_OK(eus_tgt, t));
    /* TODO free t storage */
}

int
euseq_bytes_remaining(eus)
    euseq *eus;
{
    return(eus->bytes_allocated - (eus->pos - eus->storage));
}

int
euseq_no_bytes(eus)
    euseq *eus;
{
    return((eus == NULL) ? 0
	   : eus->endp - eus->storage);
}
