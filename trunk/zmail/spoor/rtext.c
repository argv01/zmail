/*
 * $RCSfile: rtext.c,v $
 * $Revision: 2.13 $
 * $Date: 1995/02/12 02:16:22 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <text.h>
#include <rtext.h>
#include <intset.h>
#include <glist.h>

#ifndef lint
static const char spRtext_rcsid[] =
    "$Id: rtext.c,v 2.13 1995/02/12 02:16:22 bobg Exp $";
#endif /* lint */

struct spClass *spRtext_class = (struct spClass *) 0;

/* Method selectors */
int m_spRtext_addRegion;
int m_spRtext_removeRegion;
int m_spRtext_region;
int m_spRtext_nextStart;
int m_spRtext_appendRegionToDynstr;

static struct spRtext *current_rtext;

static int
regionCmp(a, b)
    struct spRegion *a, *b;
{
    int s1, s2, sdiff;
    int exact = 1;

    if (a == b)
	return (0);
    if (a->start >= 0) {
	s1 = spText_markPos(current_rtext, a->start);
    } else {
	s1 = a->pos;
	exact = 0;
    }
    if (b->start >= 0) {
	s2 = spText_markPos(current_rtext, b->start);
    } else {
	s2 = b->pos;
	exact = 0;
    }
    sdiff = s1 - s2;
    return (exact ? (sdiff ? sdiff : (a != b)) : sdiff);
}

static void
regionFinal(r)
    struct spRegion *r;
{
    spSend(current_rtext, m_spText_removeMark, r->start);
    spSend(current_rtext, m_spText_removeMark, r->end);
}

static void
spRtext_initialize(self)
    struct spRtext *self;
{
    sklist_Init(&(self->regions), (sizeof (struct spRegion)),
		regionCmp, 1, 4);
}

static void
spRtext_finalize(self)
    struct spRtext *self;
{
    current_rtext = self;
    sklist_CleanDestroy(&(self->regions), regionFinal);
}

static struct spRegion *
spRtext_addRegion(self, arg)
    struct spRtext *self;
    spArgList_t arg;
{
    int start = spArg(arg, int);
    int end = spArg(arg, int);
    struct spRegion region;

    current_rtext = self;
    region.start = spSend_i(self, m_spText_addMark, start, spText_mBefore);
    region.end = spSend_i(self, m_spText_addMark, end, spText_mAfter);
    region.attributes = 0;
    return ((struct spRegion *) sklist_Insert(&(self)->regions, &region));
}

static void
spRtext_removeRegion(self, arg)
    struct spRtext *self;
    spArgList_t arg;
{
    struct spRegion *region = spArg(arg, struct spRegion *);

    current_rtext = self;
    sklist_CleanRemove(&(self->regions), region, regionFinal);
}

static struct spRegion *
spRtext_region(self, arg)
    struct spRtext *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    struct spRegion probe, *r;

    current_rtext = self;
    probe.start = -1;
    probe.pos = pos;
    if (!(r = (struct spRegion *) sklist_Find(&(self->regions), &probe, 1))) {
	if ((!(r = (struct spRegion *) sklist_LastMiss(&(self->regions))))
	    || (spSend_i(self, m_spText_markPos, r->end) < (pos - 1))) {
	    return (0);
	}
    }
    return (r);
}

static int
spRtext_nextStart(self, arg)
    struct spRtext *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    struct spRegion probe, *r;

    current_rtext = self;
    probe.start = -1;
    probe.pos = pos;

    if (!(r = (struct spRegion *) sklist_Find(&(self->regions), &probe, 1))) {
	if (!(r = (struct spRegion *) sklist_LastMiss(&(self->regions))))
	    return (-1);
	if (!(r = (struct spRegion *) sklist_Next(&(self->regions), r)))
	    return (-1);
    }
    return (spSend_i(self, m_spText_markPos, r->start));
}

static void
spRtext_appendRegionToDynstr(self, arg)
    struct spRtext *self;
    spArgList_t arg;
{
    struct dynstr *d = spArg(arg, struct dynstr *);
    struct spRegion *region = spArg(arg, struct spRegion *);
    int s, e;

    s = spSend_i(self, m_spText_markPos, spRegion_start(region));
    e = spSend_i(self, m_spText_markPos, spRegion_end(region));
    spSend(self, m_spText_appendToDynstr, d, s, 1 + e - s);
}

/* Class initializer */

void
spRtext_InitializeClass()
{
    if (!spText_class)
	spText_InitializeClass();
    if (spRtext_class)
	return;
    spRtext_class =
	spoor_CreateClass("spRtext", "text with attribute regions",
			  spText_class,
			  (sizeof (struct spRtext)),
			  spRtext_initialize,
			  spRtext_finalize);

    m_spRtext_addRegion =
	spoor_AddMethod(spRtext_class, "addRegion",
			"add a region wrapper",
			spRtext_addRegion);
    m_spRtext_removeRegion =
	spoor_AddMethod(spRtext_class, "removeRegion",
			"delete a region wrapper",
			spRtext_removeRegion);
    m_spRtext_region =
	spoor_AddMethod(spRtext_class, "region",
			"get enclosing region",
			spRtext_region);
    m_spRtext_nextStart =
	spoor_AddMethod(spRtext_class, "nextStart",
			"find start of next region to the right",
			spRtext_nextStart);
    m_spRtext_appendRegionToDynstr =
	spoor_AddMethod(spRtext_class, "appendRegionToDynstr",
			"extract a region, append it to a dynstr",
			spRtext_appendRegionToDynstr);
}
