/* dyn_c3.c	Copyright 1995 Network Computing Devices, Inc. */

#include "mime.h"
#include "dyn_c3.h"
#include "dputil.h"
#include "c3.h"

DEFINE_EXCEPTION(c3_err_noinfo, "c3_err_noinfo");
DEFINE_EXCEPTION(c3_err_warning, "c3_err_warning");

/* 
 * Translate character set of sstr and append to dstr.
 * 
 * In case of error, raise one of the following exceptions:
 * 
 *  c3_err_noinfo
 *     No info for target charset.  Nothing is appended to DEST.  (The
 *     caller can treat this error in whatever it pleases.)
 * 
 *  c3_err_warning
 *     Whatever the meaning of (rc > 0) is.
 */
void
dyn_c3_translate(dstr, sstr, slen, in_charset, out_charset)
    struct dynstr *dstr;
    const char *sstr;
    const int slen;
    mimeCharSet in_charset;
    mimeCharSet out_charset;
{
    char trbin[256];
    int trbinLen = 0;
    int sstrLen = slen;
    int untrans, rc;

    do {
	rc = c3_translate(trbin, sstr, sizeof (trbin), &trbinLen, sstrLen,
			  out_charset, in_charset, &untrans);
	if (rc >= 0) {
	    dynstr_Append(dstr, trbin);		/* take the translated data */
	    if (rc > 0)
		RAISE(c3_err_warning, (VPTR)0);	/* XXX Is this really right? */
	} else if (rc == C3E_CCSNOINFO) {
	    RAISE(c3_err_noinfo, (VPTR)0);	/* no table for the charset */
	} else if (rc == C3E_TGTSTROFLOW) {
	    dynstr_Append(dstr, trbin);
	    sstr += untrans;
	    sstrLen -= untrans;
	} else	/* Some other error?		XXX Is this really a no-op? */
	    dynstr_Append(dstr, sstr);		/* else keep the original */
    } while (rc == C3E_TGTSTROFLOW);
}

/*
 * Dpipe filter (e.g. for dpipelines) to translate character sets.
 *
 * Filter data "fdata" is expected to point to an array of two pointers
 * to char, which are the source and destination character sets.
 */
void
dpFilter_c3_translate(rdp, wdp, fdata)
    struct dpipe *rdp, *wdp;
    GENERIC_POINTER_TYPE *fdata;
{
    char *inbuf = 0;
    mimeCharSet *charsets = (mimeCharSet *)fdata;
    struct dynstr d;
    int i, j;

    if ((i = dpipe_Get(rdp, &inbuf)) > 0) {
	dynstr_Init(&d);
	TRY {
	    dyn_c3_translate(&d, inbuf, i, charsets[0], charsets[1]);
	    j = dynstr_Length(&d);
	    dpipe_Put(wdp, dynstr_GiveUpStr(&d), j);
	} EXCEPT(c3_err_noinfo) {
	    dpipe_Put(wdp, inbuf, i);	/* Pass through unchanged */
	    inbuf = 0;			/* Don't free */
#ifdef NOT_NOW /* Alternate behavior */
	    dpipe_Unget(rdp, inbuf, i);
	    inbuf = 0;			/* Don't free */
	    PROPAGATE;
#endif /* NOT_NOW */
	} EXCEPT(c3_err_warning) {
	    j = dynstr_Length(&d);
	    dpipe_Put(wdp, dynstr_GiveUpStr(&d), j);
	} FINALLY {
	    if (inbuf) free(inbuf);
	} ENDTRY;
    }
    if (dpipe_Eof(rdp)) {
	dpipe_Close(wdp);
    }
}

/*
 * Convenience function.  Translates a string, returns it and
 * places its length in *slen.
 */
const char *
quick_c3_translate(sstr, slen, in_charset, out_charset)
    const char *sstr;
    size_t *slen;
    mimeCharSet in_charset, out_charset;
{
    static struct dynstr d;
    static char initialized = 0;

    if (!sstr || *slen == 0 ||
	    !C3_TRANSLATION_REQUIRED(in_charset ,out_charset))
	return sstr;

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }

    dynstr_Set(&d, "");
    TRY {
	dyn_c3_translate(&d, sstr, *slen, in_charset, out_charset);
    } EXCEPT(c3_err_noinfo) {
	dynstr_AppendN(&d, sstr, *slen);
    } EXCEPT(c3_err_warning) {
	;	/* Nothing */
    } ENDTRY;
    *slen = dynstr_Length(&d);
    return dynstr_Str(&d);
}

int
fp_c3_translate(in_fp, out_fp, in_charset, out_charset)
FILE *in_fp, *out_fp;
mimeCharSet in_charset, out_charset;
{
    struct dpipeline dpl;
    mimeCharSet charsets[2];
    int retval = 0;

    charsets[0] = in_charset;
    charsets[1] = out_charset;

    /* Dpipelines take their right-end before their left-end, because
     * the right-end is a dpipe reader and the left-end is a writer.
     */
    dpipeline_Init(&dpl, dputil_DpipeToFILE, out_fp,
			 dputil_FILEtoDpipe, in_fp);
    dpipeline_Prepend(&dpl, dpFilter_c3_translate, charsets, 0);
    TRY {
	dpipe_Pump(dpipeline_rdEnd(&dpl));
	retval = fflush(out_fp);
    } EXCEPT(ANY) {
	retval = -1;
    } FINALLY {
	dpipeline_Destroy(&dpl);
    } ENDTRY;

    return retval;
}
