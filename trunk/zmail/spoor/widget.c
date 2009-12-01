/* 
 * $RCSfile: widget.c,v $
 * $Revision: 2.3 $
 * $Date: 1995/09/20 06:39:36 $
 * $Author: liblit $
 *
 * $Log: widget.c,v $
 * Revision 2.3  1995/09/20 06:39:36  liblit
 * Prototype several zero-argument functions.  Unlike C++, ANSI C has two
 * extremely different meanings for "()" and "(void)" in function
 * declarations.
 *
 * Also prototype some parameter-taking functions.
 *
 * Mild constification.
 *
 * Revision 2.2  1994/04/30 20:10:02  bobg
 * Get rid of ancient DOS junk.  Add ability to document interactions and
 * keybindings.  Add doc strings for all interactions.
 *
 * Revision 2.1  1994/02/24  19:24:01  bobg
 * Oops, don't forget to add these new files, which make all the new
 * stuff possible.
 *
 */

#include "widget.h"
#include "keymap.h"

static char spWidget_rcsid[] =
    "$Id: widget.c,v 2.3 1995/09/20 06:39:36 liblit Exp $";

struct interaction {
    char *name, *doc;
    void (*fn)();
};

static struct hashtab wclassregistry;
static int wclassregistry_initialized = 0;

static struct hashtab_iterator hti;

void
spWidget_InitIterator()
{
    hashtab_InitIterator(&hti);
}

struct spWidgetInfo *
spWidget_Iterate()
{
    struct spWidgetInfo **result = ((struct spWidgetInfo **)
				    hashtab_Iterate(&wclassregistry, &hti));

    return (result ? *result : 0);
}

struct spWidgetInfo *
spWidget_Lookup(name)
    const char *name;
{
    struct spWidgetInfo probe, *probeptr = &probe, **found;

    /* XXX casting away const */
    probe.name = (char *) name;
    found = (struct spWidgetInfo **) hashtab_Find(&wclassregistry,
						  &probeptr);
    return (found ? *found : 0);
}

void
(*spWidget_GetInteraction(winfo, name))()
    struct spWidgetInfo *winfo;
    char *name;
{
    struct interaction probe, *found;

    probe.name = name;
    if (found = (struct interaction *) hashtab_Find(&(winfo->interactions),
						    &probe)) {
	return (found->fn);
    }
    return (0);
}

char *
spWidget_InteractionDoc(winfo, name)
    struct spWidgetInfo *winfo;
    char *name;
{
    struct interaction probe, *found;

    probe.name = name;
    if (found = (struct interaction *) hashtab_Find(&(winfo->interactions),
						    &probe)) {
	return (found->doc);
    }
    return (0);
}

static unsigned int
interactionhash(i)
    struct interaction *i;
{
    return (hashtab_StringHash(i->name));
}

static int
interactioncmp(i, j)
    struct interaction *i, *j;
{
    return (strcmp(i->name, j->name));
}

static unsigned int
wclasshash(w)
    struct spWidgetInfo **w;
{
    return (hashtab_StringHash((*w)->name));
}

static int
wclasscmp(a, b)
    struct spWidgetInfo **a, **b;
{
    return (strcmp((*a)->name, (*b)->name));
}

struct spWidgetInfo *
spWidget_Create(name, super)
    char *name;
    struct spWidgetInfo *super;
{
    struct spWidgetInfo *result = ((struct spWidgetInfo *)
				   emalloc(sizeof (struct spWidgetInfo),
					   "spWidget_Create"));

    result->name = (char *) emalloc(1 + strlen(name),
				    "spWidget_Create");
    strcpy(result->name, name);
    result->super = super;
    result->keymap = 0;
    hashtab_Init(&(result->interactions), interactionhash,
		 interactioncmp, (sizeof (struct interaction)), 17);

    if (!wclassregistry_initialized) {
	hashtab_Init(&wclassregistry, wclasshash, wclasscmp,
		     (sizeof (struct spWidgetInfo *)), 61);
	wclassregistry_initialized = 1;
    }
    hashtab_Add(&wclassregistry, &result);

    return (result);
}

void
spWidget_AddInteraction(winfo, name, fn, doc)
    struct spWidgetInfo *winfo;
    char *name;
    void (*fn)();
    char *doc;
{
    struct interaction i;

    i.name = (char *) emalloc(1 + strlen(name), "spWidget_AddInteraction");
    strcpy(i.name, name);
    if (doc) {
	i.doc = (char *) emalloc(1 + strlen(doc), "spWidget_AddInteraction");
	strcpy(i.doc, doc);
    } else {
	i.doc = 0;
    }
    i.fn = fn;
    hashtab_Add(&(winfo->interactions), &i);
}

void
spWidget_bindKey(winfo, keyseq, fn, obj, data, label, doc)
    struct spWidgetInfo *winfo;
    struct spKeysequence *keyseq;
    char *fn, *obj, *data, *label, *doc;
{
    if (!(winfo->keymap)) {
	winfo->keymap = (struct spKeymap *) emalloc(sizeof (struct spKeymap),
						    "spWidget_bindKey");
	spKeymap_Init(winfo->keymap);
    }
    spKeymap_AddFunction(winfo->keymap, keyseq, fn, obj, data, label, doc);
}

void
spWidget_unbindKey(winfo, keyseq)
    struct spWidgetInfo *winfo;
    struct spKeysequence *keyseq;
{
    if (!(winfo->keymap)) {
	winfo->keymap = (struct spKeymap *) emalloc(sizeof (struct spKeymap),
						    "spWidget_unbindKey");
	spKeymap_Init(winfo->keymap);
    }
    spKeymap_Remove(winfo->keymap, keyseq);
}
