/*
 * $RCSfile: text.h,v $
 * $Revision: 2.9 $
 * $Date: 1995/03/30 23:31:34 $
 * $Author: bobg $
 */

#ifndef SPOOR_TEXT_H
#define SPOOR_TEXT_H

#include <spoor.h>
#include <dlist.h>
#include "obsrvbl.h"

struct spText {
    SUPERCLASS(spObservable);
    struct dlist marks;
    int allocated, gapStart, gapEnd, newlines;
    int readOnly, updateMarks;
    char *buf;
};

#define spText_readOnly(t) (((struct spText *) (t))->readOnly)
#define spText_newlines(t) (((struct spText *) (t))->newlines)
#define spText_updateMarks(t) (((struct spText *) (t))->updateMarks)

extern struct spClass    *spText_class;

#define spText_NEW() \
    ((struct spText *) spoor_NewInstance(spText_class))

extern int m_spText_clear;
extern int m_spText_insert;
extern int m_spText_replace;
extern int m_spText_delete;
extern int m_spText_substring;
extern int m_spText_length;
extern int m_spText_addMark;
extern int m_spText_removeMark;
extern int m_spText_setMark;
extern int m_spText_setReadOnly;
extern int m_spText_writeFile;
extern int m_spText_readFile;
extern int m_spText_writePartial;
extern int m_spText_appendToDynstr;

#ifdef REGEXPR
extern int m_spText_rxpSearch;
#endif /* REGEXPR */

extern void             spText_InitializeClass();

enum spText_markType {
    spText_mNeutral = 0,
    spText_mBefore,
    spText_mAfter,
    spText_MARKTYPES
};

enum spText_observation {
    spText_linesChanged = spObservable_OBSERVATIONS,
    spText_readOnlynessChanged,
    spText_OBSERVATIONS
};

struct spText_changeinfo {
    int pos, len, newlines;
};

extern int spText_getc P((struct spText *, int));
extern int spText_markPos P((struct spText *, int));

#endif /* SPOOR_TEXT_H */
