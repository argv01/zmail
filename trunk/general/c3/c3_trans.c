#include <stdlib.h>
#include <string.h>
#include "c3.h"
#include "c3_trans.h"
#include "catalog.h"
#include "dynstr.h"
#include "glist.h"
#include "lib.h"
#include "mime.h"
#include "zmopt.h"
#include "zprint.h"

struct cs_nicename {
    mimeCharSet 	cs;
    catalog_ref		nice_name;
    char		*mime_name;
};

/*
 * this table only purpose is to get a nice name for the character
 * set.  Should match table in C3, order doesn't matter.
 */

static struct cs_nicename csnicename[] =
{
    {UsAscii,		catref(CAT_MSGS, 969, "ASCII"),	"us-ascii"},
    {Iso_646_Gb,	catref(CAT_MSGS, 970, "UK 7-bit"),	"iso-646-gb"},
    {Iso_646_No,	catref(CAT_MSGS, 971, "Norwegian 7-bit"),
						"iso-646-no"},
    {Iso_646_Yu,	catref(CAT_MSGS, 972, "Croatian/Slovene 7-bit"),
						"iso-646-yu"},
    {Iso_8859_1,	catref(CAT_MSGS, 973, "Latin-1"),	"iso-8859-1"},
    {Iso_8859_2,	catref(CAT_MSGS, 974, "Latin-2"),	"iso-8859-2"},
    {Iso_8859_3,	catref(CAT_MSGS, 975, "Latin-3"), 	"iso-8859-3"},
    {Iso_8859_4,	catref(CAT_MSGS, 976, "Latin-4"),	"iso-8859-4"},
    {Iso_8859_5,	catref(CAT_MSGS, 977, "Latin-Cyrillic"),
						"iso-8859-5"},
    {Iso_8859_6,	catref(CAT_MSGS, 978, "Latin-Arabic"), "iso-8859-6"},
    {Iso_8859_7,	catref(CAT_MSGS, 979, "Latin-Greek"),  "iso-8859-7"},
    {Iso_8859_8,	catref(CAT_MSGS, 980, "Latin-8"), 	"iso-8859-8"},
    {Iso_8859_9,	catref(CAT_MSGS, 981, "Latin-9"),	"iso-8859-9"},
    {X_Macintosh,	catref(CAT_MSGS, 982, "Macintosh Latin 1"),
						"x-macintosh"},
    {X_Iso_10646_2_1,	catref(CAT_MSGS, 983, "UCS in 2-octet form at level 1"),
						"iso-10646-2-1"},
    {X_Iso_646_Se1,	catref(CAT_MSGS, 984, "Swedish General 7-bit"), 
						"x-iso-646-se1"},
    {X_Iso_646_Se2,	catref(CAT_MSGS, 985, "Swedish 7-bit for names"),
						"x-iso-646-se2"},
    {X_IBM_437,		catref(CAT_MSGS, 986, "Original IBM PC"),
						"x-ibm-437"},
    {X_IBM_850,		catref(CAT_MSGS, 987, "International IBM PC"),
						"x-ibm-850"},
    {NoMimeCharSet,	catref(CAT_MSGS, 0, ""), ""},
};

/*
 * this is only to get the nice name from the static table here.
 * should only be called to get up c3 table
 */

static char *
nicename_from_cs(cs)
    mimeCharSet cs;
{
    struct cs_nicename *tablep;

    for (tablep = csnicename;
         tablep->cs != NoMimeCharSet; tablep++)
    {
        if (tablep->cs == cs)
        {
            if (catalog) {
                return(savestr(catgetref(tablep->nice_name)));
            }
        }
    }
    return(NULL);
}

int
c3_is_known_to_c3(cs)
    mimeCharSet cs;
{
    return(_c3_ccs_is_available((int)cs));
}

static struct glist CharsetNumList;

typedef struct C3Entry {
    mimeCharSet 	cs;
    const char 		*mime_name;
    const char		*nice_name;
} C3Entry;

/*
 * takes whatever name is given and adds it to the table.
 * caller should make sure that an x- is prepended if needed
 */

int
c3_cs_list_length()
{
    static first_time = TRUE;
    int i,j;
    mimeCharSet cs;
    int length;
    C3Entry c3entry;

    if (first_time) {
        first_time = FALSE;
        glist_Init(&CharsetNumList, (sizeof (C3Entry)), 16);
	    fh_c3_set_backup_table_path("ZMLIB");
	    fh_c3_build_table(C3_SYSNAME, &ccs_noidmime, &ccs_noidmime_length);
	    fetched_ccsData(C3_SYSNAME, 0);	/* set up dummy to read bio_data */

        for (i = 0, j = 0; i < ccs_noidmime_length; i++) {
	        c3entry.cs = (mimeCharSet) ccs_noidmime[i].num;
    	    c3entry.mime_name = ccs_noidmime[i].mime_name;
    	    c3entry.nice_name = nicename_from_cs(c3entry.cs);
    	    if (c3entry.nice_name == NULL)	/* make sure we have it */
		        c3entry.nice_name = c3entry.mime_name;
	        glist_Set(&CharsetNumList, j, &c3entry);
	        j++;
	    }
	    /*
	     * now add all the names that Z-mail knows about, but
	     * C3 does not
	    */
	    for (i = 0; csnicename[i].cs != NoMimeCharSet; i++) {
	        if (!IsKnownMimeCharSet(csnicename[i].cs))
	    	    c3_add_to_table(csnicename[i].mime_name, 
			    catgetref(csnicename[i].nice_name));
	    }
    }
    return (glist_Length(&CharsetNumList));
}

mimeCharSet
c3_add_to_table(csname, csnicename)
    const char *csname;
    const char *csnicename;
{
    static mimeCharSet cs = UnknownMimeCharSet+1;

    C3Entry c3entry;
    const char *pstr = csname;
    int length = c3_cs_list_length();

    c3entry.cs = cs; 
    cs++;			/* increment for next call */
    if (pstr == NULL)
	pstr = "";
    c3entry.mime_name = savestr(pstr);
    c3entry.nice_name = savestr(csnicename);
    glist_Set(&CharsetNumList, length, &c3entry);
    return(c3entry.cs);
}

int 
IsKnownMimeCharSet(cs)
mimeCharSet cs;
{
    int length = c3_cs_list_length();
    int i;

    for (i = 0; i < length; i++)
        if (((C3Entry *) glist_Nth(&CharsetNumList,(i)))->cs == cs)
            return(TRUE);
    return(FALSE);
}

/*
 * Search internal table for known name only
 */

int
IsKnownMimeCharSetName(csname, cs)
    const char *csname;
    mimeCharSet *cs;
{
    int length = c3_cs_list_length();
    int i;

    for (i = 0; i < length; i++)
        if (ci_strcmp(((C3Entry *) glist_Nth(&CharsetNumList,(i)))->mime_name,
                csname) == 0) {
	    *cs = ((C3Entry *) glist_Nth(&CharsetNumList,(i)))->cs;
            return(TRUE); 
	}
    return(FALSE);
}

mimeCharSet
c3_cs_list(pos)
    int pos;
{
    int length = c3_cs_list_length();
    return (pos < length ?
                ((C3Entry *) glist_Nth(&CharsetNumList,(pos)))->cs :
                NoMimeCharSet);
}

const char *
c3_nicename_from_cs(cs)
    mimeCharSet cs;
{
    int i, length = c3_cs_list_length();

    for (i =0; i < length; i++)
    {
	if (cs == ((C3Entry *) glist_Nth(&CharsetNumList,(i)))->cs)
        {
            return(((C3Entry *) glist_Nth(&CharsetNumList,(i)))->nice_name);
        }
    }
    return("");		/* should never happen */
}

const char *
c3_mimename_from_cs(cs)
    mimeCharSet cs;
{
    int i, length = c3_cs_list_length();

    for (i =0; i < length; i++)
    {
	if (cs == ((C3Entry *) glist_Nth(&CharsetNumList,(i)))->cs)
        {
            return(((C3Entry *) glist_Nth(&CharsetNumList,(i)))->mime_name);
        }
    }
    return("");		/* should never happen */
}

static int c3_setup = FALSE;
static int sh;
static int c3_failure = FALSE;

/*
 * Returns a c3 return code:
 *     rc < 0  - Error condition
 *            if C3E_CCSNOINFO (no charset info) then target_string contains
 *                nothing and *sstr_untrans = 0
 *            other error codes - target_string contains data only if
 *                *sstr_untrans > 0
 *     rc = 0  - success (target_string contains translated data)
 *     rc > 0  - Warning condition (target_string contains translated data)
 */
int				/* c3 return code, < 0 is error */
c3_translate(tstr, sstr, tlen, tactlen, slen, tgt_charset, src_charset, 
	sstr_untrans)
    char *tstr;			/* target string (output) NULL terminated */
    const char *sstr;		/* source string */
    int tlen;			/* target string length */
    int *tactlen;		/* actual target string length (output) */
    int slen;			/* source string length */
    mimeCharSet tgt_charset;	/* target string mime charset */
    mimeCharSet src_charset;	/* source string mime charset */
    int *sstr_untrans;		/* offset of first untranslated byte */
{
    char *oc_message;
    static int rc;
    static char *c3verbose = NULL;
#ifdef C3_INITBUG
    static mimeCharSet last_src_charset = 0;
    static mimeCharSet last_tgt_charset = 0;
#endif

    *sstr_untrans = 0;
#ifdef C3_INITBUG
    if (c3_setup && !c3_failure) {
    	if (src_charset != last_src_charset ||
		tgt_charset != last_tgt_charset) {
	    c3_finalize();
	    c3_setup = FALSE;
	}
    }
#endif

    if (!c3_setup) {
	c3_setup = TRUE;
	c3_initialize(C3_SYSNAME);			/* c3 initialization */
	if ((rc = c3_outcome_code(&oc_message)) < 0)
	    c3_failure = TRUE;
	else {
	    sh = c3_create_stream();
	    c3_set_iparam(sh, C3I_CTYPE, C3IV_CTYPE_LEGIBLE); /* set type of conversion */
	    c3_set_sparam(sh, C3S_TGT_LB, "crlf");	/* target Line Break */
	c3verbose = value_of("__verbose_c3_mapping");
	}
    }
    if (c3_failure)
	return (rc);

    /*make sure our charsets are supported */
    if (!c3_is_known_to_c3(src_charset) || !c3_is_known_to_c3(tgt_charset)) {
	rc = C3E_CCSNOINFO;	/* TODO not sure of the correct error here */
	/*
	 * TODO would be nice to tell the caller somehow *which* charset
	 * it was that was not supported!
	 */
	*tactlen = 0;
    } else {
#ifdef C3_INITBUG
	last_src_charset = src_charset;
	last_tgt_charset = tgt_charset;
#endif
	c3_set_iparam(sh, C3I_SRC_CCS, src_charset);	/* set source char set */
	c3_set_iparam(sh, C3I_TGT_CCS, tgt_charset);	/* set target char set */
	/*
	 * for target length, save 1 for terminator NULL and
	 * 1 for 0 length offset
	*/
	c3_bconv(sh, tstr, sstr, tlen-2, tactlen, slen);/* source char stream,
							 * target string,
							 * source string,
							 * target max bytes,
							 * target actual bytes,
							 * source length  */
	rc = c3_outcome_code(&oc_message);
	c3_finalize_stream(sh);				/* finalize for next source */ 
#if 0
	/* temporary for QA  */
	if (c3verbose)
	print("Message mapped from %s to %s\n", GetMimeCharSetName(src_charset),
		GetMimeCharSetName(tgt_charset));
#endif
    }
    tstr[*tactlen] = 0;		/* just to make sure */

    /*
     * TODO
     * Handle special error case where translation died in the middle
     * of a multibyte character sequence.
     */
    if (rc == C3E_TGTSTROFLOW) {
	*sstr_untrans = (int)c3_exception_data() - (int)sstr;
	/* double check to make sure nothing funny happened */
	if (*sstr_untrans <= 0 || *sstr_untrans > slen)
	    *sstr_untrans = slen;		/* something is wrong */
    } else if (rc != C3E_CCSNOINFO) {
	*sstr_untrans = slen;
    }
    return (rc);
}
