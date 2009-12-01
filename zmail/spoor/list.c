/*
 * $RCSfile: list.c,v $
 * $Revision: 2.15 $
 * $Date: 1995/02/12 02:16:12 $
 * $Author: bobg $
 */

#include "list.h"

#ifndef lint
static const char spList_rcsid[] =
    "$Id: list.c,v 2.15 1995/02/12 02:16:12 bobg Exp $";
#endif /* lint */

struct spClass *spList_class = 0;

int m_spList_append;
int m_spList_prepend;
int m_spList_getItem;
int m_spList_remove;
int m_spList_getNthItem;
int m_spList_replace;
int m_spList_length;
int m_spList_insert;

static void
spList_clear(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    while (!glist_EmptyP(&(self->separators))) {
	spSend(self, m_spText_removeMark,
	       *((int *) glist_Last(&(self->separators))));
	glist_Pop(&(self->separators));
    }
    spSuper(spList_class, self, m_spText_clear);
}

static void
spList_append(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    char *str = spArg(arg, char *);
    int updatingMarks = spText_updateMarks(self);
    int l, m;

    TRY {
	spText_updateMarks(self) = 0;
	if ((l = spSend_i(self, m_spText_length)) > 0) {
	    spSend(self, m_spText_insert, -1, 1, "\n", spText_mAfter);
	    m = spSend_i(self, m_spText_addMark, l, spText_mAfter);
	    glist_Add(&(self->separators), &m);
	}
	spSend(self, m_spText_insert, -1, -1, str, spText_mAfter);
	spSend(self, m_spObservable_notifyObservers, spList_appendItem, str);
    } FINALLY {
	spText_updateMarks(self) = updatingMarks;
    } ENDTRY;
}

static void
spList_prepend(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    char *str = spArg(arg, char *);
    int l = spSend_i(self, m_spText_length), slen = strlen(str);

    spSend(self, m_spText_insert, 0, slen, str, spText_mAfter);
    if (l > 0) {
	int m;

	spSend(self, m_spText_insert, slen, 1, "\n", spText_mNeutral);
	m = spSend_i(self, m_spText_addMark, slen, spText_mAfter);
	glist_Insert(&(self->separators), &m, 0);
    }
    spSend(self, m_spObservable_notifyObservers, spList_prependItem, str);
}

static int
spList_getItem(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    int pos = spArg(arg, int);
    struct dynstr *d = spArg(arg, struct dynstr *);
    int l, m, u;
    int mark, markpos;

    if ((u = glist_Length(&(self->separators))) == 0) {
	if (spSend_i(self, m_spText_length) == 0)
	    return (-1);
	if (d)
	    spSend(self, m_spText_appendToDynstr, d, 0, -1);
	return (0);
    }
    --u;
    l = 0;
    while (l <= u) {
	m = (l + u) / 2;
	mark = *((int *) glist_Nth(&(self->separators), m));
	markpos = spText_markPos((struct spText *) self, mark);
	if (markpos == pos) {
	    break;
	} else if (pos < markpos) {
	    u = m - 1;
	} else {		/* pos > markpos */
	    l = m + 1;
	}
    }
    if (pos <= markpos) {	/* mark is at the end of the item */
	if (d) {
	    if (m == 0) {
		spSend(self, m_spText_appendToDynstr, d, 0, markpos);
	    } else {
		int prevmark = *((int *) glist_Nth(&(self->separators),
						   m - 1));
		int prevmarkpos = spText_markPos((struct spText *) self,
						 prevmark);

		spSend(self, m_spText_appendToDynstr, prevmarkpos + 1,
		       markpos - (prevmarkpos + 1));
	    }
	}
	return (m);
    } else {			/* mark is at the beginning of the item */
	if (d) {
	    if (m == (glist_Length(&(self->separators)) - 1)) {
		spSend(self, m_spText_appendToDynstr, d, markpos + 1, -1);
	    } else {
		int nextmark = *((int *) glist_Nth(&(self->separators),
						   m + 1));
		int nextmarkpos = spText_markPos((struct spText *) self,
						 nextmark);

		spSend(self, m_spText_appendToDynstr, markpos + 1,
		       nextmarkpos - (markpos + 1));
	    }
	}
	return (m + 1);
    }
}

static void
spList_remove(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    int indx = spArg(arg, int);
    int l, m, mp;

    if ((l = glist_Length(&(self->separators))) == 0) {
	spSend(self, m_spText_clear);
    } else {
	if (indx == l) {	/* removing the last item */
	    m = *((int *) glist_Last(&(self->separators)));
	    mp = spText_markPos((struct spText *) self, m);
	    glist_Pop(&(self->separators));
	    spSend(self, m_spText_removeMark, m);
	    spSend(self, m_spText_delete, mp, -1);
	} else if (indx == 0) {
	    m = *((int *) glist_Nth(&(self->separators), 0));
	    mp = spText_markPos((struct spText *) self, m);
	    glist_Remove(&(self->separators), 0);
	    spSend(self, m_spText_removeMark, m);
	    spSend(self, m_spText_delete, 0, mp + 1);
	} else {
	    int prevm, prevmpos;

	    m = *((int *) glist_Nth(&(self->separators), indx));
	    mp = spText_markPos((struct spText *) self, m);
	    prevm = *((int *) glist_Nth(&(self->separators), indx - 1));
	    prevmpos = spText_markPos((struct spText *) self, prevm);
	    glist_Remove(&(self->separators), indx);
	    spSend(self, m_spText_removeMark, m);
	    spSend(self, m_spText_delete, prevmpos + 1,
		   (mp + 1) - (prevmpos + 1));
	}
    }
    spSend(self, m_spObservable_notifyObservers, spList_removeItem, &indx);
}

static int
spList_getNthItem(self, arg)	/* returns posn of 1st character */
    struct spList *self;
    spArgList_t arg;
{
    int indx = spArg(arg, int);
    struct dynstr *d = spArg(arg, struct dynstr *);
    int m, mp, pm, pmp;

    if (glist_EmptyP(&(self->separators))) {
	if ((indx == 0) && (spSend_i(self, m_spText_length) > 0)) {
	    if (d) {
		spSend(self, m_spText_appendToDynstr, d, 0, -1);
	    }
	    return (0);
	}
	return (-1);
    }
    if (indx == 0) {
	if (d) {
	    m = *((int *) glist_Nth(&(self->separators), 0));
	    mp = spText_markPos((struct spText *) self, m);
	    spSend(self, m_spText_appendToDynstr, d, 0, mp);
	}
	return (0);
    }
    if (indx == glist_Length(&(self->separators))) {
	m = *((int *) glist_Last(&(self->separators)));
	mp = spText_markPos((struct spText *) self, m);
	if (d) {
	    spSend(self, m_spText_appendToDynstr, d, mp + 1, -1);
	}
	return (mp + 1);
    }
    pm = *((int *) glist_Nth(&(self->separators), indx - 1));
    pmp = spText_markPos((struct spText *) self, pm);
    if (d) {
	m = *((int *) glist_Nth(&(self->separators), indx));
	mp = spText_markPos((struct spText *) self, m);
	spSend(self, m_spText_appendToDynstr, d, pmp + 1, mp - (pmp + 1));
    }
    return (pmp + 1);
}

static void
spList_replace(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    int indx = spArg(arg, int);
    char *str = spArg(arg, char *);
    int m, mp, pm, pmp;

    if (glist_EmptyP(&(self->separators))) {
	if ((indx == 0) && (spSend_i(self, m_spText_length) > 0)) {
	    spSend(self, m_spText_replace, 0, -1, -1, str, spText_mNeutral);
	}
	return;
    }
    if (indx == 0) {
	m = *((int *) glist_Nth(&(self->separators), 0));
	mp = spText_markPos((struct spText *) self, m);
	spSend(self, m_spText_replace, 0, mp, -1, str, spText_mNeutral);
	return;
    }
    if (indx == glist_Length(&(self->separators))) {
	m = *((int *) glist_Last(&(self->separators)));
	mp = spText_markPos((struct spText *) self, m);
	spSend(self, m_spText_replace, mp + 1, -1, -1, str, spText_mNeutral);
	return;
    }
    pm = *((int *) glist_Nth(&(self->separators), indx - 1));
    pmp = spText_markPos((struct spText *) self, pm);
    m = *((int *) glist_Nth(&(self->separators), indx));
    mp = spText_markPos((struct spText *) self, m);
    spSend(self, m_spText_replace, pmp + 1, mp - (pmp + 1), -1, str,
	   spText_mNeutral);
}

static void
spList_initialize(self)
    struct spList *self;
{
    glist_Init(&(self->separators), (sizeof (int)), 16);
}

static void
spList_finalize(self)
    struct spList *self;
{
    glist_Destroy(&(self->separators));
}

static int
spList_length(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    return (glist_EmptyP(&(self->separators)) ?
	    ((spSend_i(self, m_spText_length) == 0) ? 0 : 1) :
	    1 + glist_Length(&(self->separators)));
}

static void
spList_insert(self, arg)
    struct spList *self;
    spArgList_t arg;
{
    int indx = spArg(arg, int);
    char *str = spArg(arg, char *);

    if (indx == 0) {
	spSend(self, m_spList_prepend, str);
    } else if ((indx < 0) || (indx >= spSend_i(self, m_spList_length))) {
	spSend(self, m_spList_append, str);
    } else {
	/* Surrounded on both sides */
	int pos = spSend_i(self, m_spList_getNthItem, indx, 0);
	int m;

	spSend(self, m_spText_insert, pos, 1, "\n", spText_mBefore);
	m = spSend_i(self, m_spText_addMark, pos, spText_mAfter);
	glist_Insert(&(self->separators), &m, indx);
	spSend(self, m_spText_insert, pos, -1, str, spText_mBefore);
	spSend(self, m_spObservable_notifyObservers, spList_insertItem,
	       &indx);
    }
}

void
spList_InitializeClass()
{
    if (!spText_class)
	spText_InitializeClass();
    if (spList_class)
	return;
    spList_class = spoor_CreateClass("spList", "list", spText_class,
				     (sizeof (struct spList)),
				     spList_initialize, spList_finalize);

    spoor_AddOverride(spList_class, m_spText_clear, 0, spList_clear);

    m_spList_insert = spoor_AddMethod(spList_class,
				      "insert",
				      "insert item",
				      spList_insert);
    m_spList_append = spoor_AddMethod(spList_class, "append",
				      "add an item to the end",
				      spList_append);
    m_spList_prepend = spoor_AddMethod(spList_class, "prepend",
				       "add an item to the beginning",
				       spList_prepend);
    m_spList_getItem = spoor_AddMethod(spList_class, "getItem",
				       "get list item string from pos",
				       spList_getItem);
    m_spList_remove = spoor_AddMethod(spList_class, "remove",
				      "remove list item by index",
				      spList_remove);
    m_spList_getNthItem = spoor_AddMethod(spList_class, "getNthItem",
					  "get item by index",
					  spList_getNthItem);
    m_spList_replace = spoor_AddMethod(spList_class, "replace",
				       "replace a list item",
				       spList_replace);
    m_spList_length = spoor_AddMethod(spList_class, "length",
				      "answer number of items",
				      spList_length);

    spEvent_InitializeClass();
    spIm_InitializeClass();
    spList_InitializeClass();
}
