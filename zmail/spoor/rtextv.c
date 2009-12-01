/*
 * $RCSfile: rtextv.c,v $
 * $Revision: 2.13 $
 * $Date: 1995/03/30 23:31:25 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <textview.h>
#include <rtextv.h>
#include <rtext.h>
#include <charwin.h>

#ifndef lint
static const char spRtextv_rcsid[] =
    "$Id: rtextv.c,v 2.13 1995/03/30 23:31:25 bobg Exp $";
#endif lint

struct spClass *spRtextv_class = (struct spClass *) 0;

#define LXOR(a,b) ((!(a))!=(!(b)))

#ifdef MIN
# undef MIN
#endif /* MIN */
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#ifdef MAX
# undef MAX
#endif /* MAX */
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

static int
spRtextv_nextSoftBol(self, arg)
    struct spRtextv *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    int winwidth = spArg(arg, int);
    int guess, start;
    struct spRegion *region;

    if (spTextview_wrapmode(self) != spRtextv_regionwrap)
	return (spSuper_i(spRtextv_class, self, m_spTextview_nextSoftBol,
			pos, winwidth));

    spTextview_wrapmode(self) = spTextview_wordwrap;
    guess = spSend_i(self, m_spTextview_nextSoftBol, pos, winwidth);
    spTextview_wrapmode(self) = spRtextv_regionwrap;
    region = (struct spRegion *) spSend_p(spView_observed(self),
					      m_spRtext_region, guess);

    if (spRegion_attributes(region) & (1 << spRtextv_nowrap)
	&& ((start = spSend_i(spView_observed(self), m_spText_markPos,
			    spRegion_start(region))) > pos))
	return (start);
    return (guess);
}

static unsigned long
newattrs(self, pos, oldattrs)
    struct spRtextv *self;
    int pos;
    unsigned long oldattrs;
{
    struct spRegion *region;
    unsigned long result = 0;

    if (region = (struct spRegion *) spSend_p(spView_observed(self),
					    m_spRtext_region, pos))
	if (spRegion_attributes(region) & (1 << spRtextv_highlighted))
	    result |= (1 << spCharWin_standout);
    return (result);
}

typedef unsigned long (*ulfn_t)();

static int
spRtextv_nextChange(self, arg)
    struct spRtextv *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    unsigned long (**fn)() = spArg(arg, ulfn_t *);
    int nextstart, nextend;
    struct spRegion *region;

    if (!(region = (struct spRegion *) spSend_p(spView_observed(self),
					      m_spRtext_region, pos)))
	return (-1);
    nextend = spSend_i(spView_observed(self), m_spText_markPos,
		     spRegion_end(region)) + 1;
    nextstart = spSend_i(spView_observed(self), m_spRtext_nextStart, pos);
    *fn = newattrs;
    return ((nextstart < 0) ? nextend :
	    ((nextend < 0) ? nextstart :
	     (nextstart < nextend) ? nextstart : nextend));
}

/* Class initializer */

void
spRtextv_InitializeClass()
{
    if (!spTextview_class)
	spTextview_InitializeClass();
    if (spRtextv_class)
	return;
    spRtextv_class =
	spoor_CreateClass("spRtextv", "view for region-texts",
			  spTextview_class,
			  (sizeof (struct spRtextv)), NULL, NULL);

    spoor_AddOverride(spRtextv_class, m_spTextview_nextSoftBol, NULL,
		      spRtextv_nextSoftBol);
    spoor_AddOverride(spRtextv_class, m_spTextview_nextChange, NULL,
		      spRtextv_nextChange);

    spRtext_InitializeClass();
    spCharWin_InitializeClass();
}
