/*
 * $RCSfile: keymap.h,v $
 * $Revision: 2.9 $
 * $Date: 1995/09/20 06:39:30 $
 * $Author: liblit $
 *
 * $Log: keymap.h,v $
 * Revision 2.9  1995/09/20 06:39:30  liblit
 * Prototype several zero-argument functions.  Unlike C++, ANSI C has two
 * extremely different meanings for "()" and "(void)" in function
 * declarations.
 *
 * Also prototype some parameter-taking functions.
 *
 * Mild constification.
 *
 * Revision 2.8  1994/04/30 20:09:51  bobg
 * Get rid of ancient DOS junk.  Add ability to document interactions and
 * keybindings.  Add doc strings for all interactions.
 *
 * Revision 2.7  1994/02/22  20:45:01  bobg
 * Make SPOOR keysequences a lot more like a real datatype; get rid of
 * several static buffers.  Change "c-" to "ctrl+" in keynames (but still
 * recognize "c-" on input).  Shorten CVS logs.  Start putting in
 * scrolling- and scrollpct changes mandated by Dan, but compile them out
 * for now.  Create spMsend function mandated by Dan.
 */

#ifndef SPOOR_KEYMAP_H
#define SPOOR_KEYMAP_H

#include <glist.h>
#include <sklist.h>

struct spKeysequence {
    struct glist elts;
};

#define spKeysequence_Length(ks) (glist_Length(&((ks)->elts)))
#define spKeysequence_Init(ks) (glist_Init(&((ks)->elts),(sizeof (int)),4))
#define spKeysequence_Nth(ks,i) (*((int *) glist_Nth(&((ks)->elts),(i))))
#define spKeysequence_Destroy(ks) (glist_Destroy(&((ks)->elts)))
#define spKeysequence_Last(ks) (*((int *) glist_Last(&((ks)->elts))))

extern void spKeysequence_Truncate P((struct spKeysequence *, int));
extern void spKeysequence_Add P((struct spKeysequence *, int));
extern struct spKeysequence *spKeysequence_Parse P((struct spKeysequence *,
						    char *, int));
extern void spKeysequence_Concat P((struct spKeysequence *,
				    struct spKeysequence *));

extern int spKeysequence_Chop P((struct spKeysequence *));

struct spKeymapEntry {
    int c;
    enum {
	spKeymap_function,
	spKeymap_keymap,
	spKeymap_removed,
	spKeymap_translation
    } type;
    union {
	struct {
	    char *fn, *obj, *data, *label, *doc;
	} function;
	struct spKeymap *keymap;
	struct spKeysequence translation;
    } content;
};

struct spKeymap {
    struct sklist entries;
};

extern struct spKeymapEntry *spKeymap_First P((struct spKeymap *));
extern struct spKeymapEntry *spKeymap_Next P((struct spKeymap *,
					      struct spKeymapEntry *));
extern void spKeymap_Destroy P((struct spKeymap *));
extern void spKeymap_Init P((struct spKeymap *));
extern void spKeymap_AddFunction P((struct spKeymap *,
				    struct spKeysequence *,
				    char *, char *, char *, char *, char *));
extern void spKeymap_AddTranslation P((struct spKeymap *,
				       struct spKeysequence *,
				       struct spKeysequence *));
extern void spKeymap_Remove P((struct spKeymap *,
			       struct spKeysequence *));
extern void spKeymap_ReallyRemove P((struct spKeymap *,
				     struct spKeysequence *));
extern struct spKeymapEntry *spKeymap_lookup P((struct spKeymap *,
						struct spKeysequence *));
extern char *spKeyname P((int, int));
extern void spKeynameList_Init P((void));
extern int spKeyMax P((void));

#endif /* SPOOR_KEYMAP_H */
