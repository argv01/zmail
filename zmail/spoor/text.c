/*
 * $RCSfile: text.c,v $
 * $Revision: 2.30 $
 * $Date: 1995/09/12 20:43:13 $
 * $Author: liblit $
 */

#include <config.h>

#include <stdio.h>
#include <zctype.h>
#include <zcunix.h>

#include <spoor.h>
#include <obsrvbl.h>
#include <text.h>

#include <dynstr.h>

#include <regexpr.h>
#include "bfuncs.h"

#ifndef lint
static const char spText_rcsid[] =
    "$Id: text.c,v 2.30 1995/09/12 20:43:13 liblit Exp $";
#endif /* lint */

#undef MAX
#undef MIN
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))

struct spClass           *spText_class = (struct spClass *) 0;

struct mark {
    int pos;
    enum spText_markType type;
};

int m_spText_clear;
int m_spText_insert;
int m_spText_replace;
int m_spText_delete;
int m_spText_substring;
int m_spText_length;
int m_spText_addMark;
int m_spText_removeMark;
int m_spText_setMark;
int m_spText_setReadOnly;
int m_spText_writeFile;
int m_spText_readFile;
int m_spText_writePartial;
int m_spText_appendToDynstr;
int m_spText_rxpSearch;

static void
spText_initialize(self)
    struct spText          *self;
{
    self->updateMarks = 1;
    self->allocated = self->gapStart = self->gapEnd = self->newlines = 0;
    self->readOnly = 0;
    self->buf = NULL;
    dlist_Init(&(self->marks), (sizeof (struct mark)), 16);
}

static void
spText_finalize(self)
    struct spText          *self;
{
    if (self->buf)
	free(self->buf);
    dlist_Destroy(&(self->marks));
}

static void
spText_clear(self, arg)
    struct spText          *self;
    spArgList_t             arg;
{
    int i;
    struct mark *mark;

    self->gapStart = 0;
    self->gapEnd = self->allocated;
    if (self->updateMarks)
	dlist_FOREACH(&(self->marks), struct mark, mark, i) {
	    mark->pos = 0;
	}
    self->newlines = 0;
    spSend(self, m_spObservable_notifyObservers,
	   self->newlines ? spText_linesChanged : spObservable_contentChanged,
	   0);
}

static void
gap_accommodate(self, required)
    struct spText *self;
    int required;
{
    const int gapSize = self->gapEnd - self->gapStart;
    
    if (gapSize < required) {
	/* The constants in the padding computation are fairly arbitrary */
	const int minimum = self->allocated + required;
	const int padding = MAX(512, MIN(16*1024, self->allocated / 5));
	const int request = minimum + padding;
	const int tailSize = self->allocated - self->gapEnd;
	const int newGapEnd = request - tailSize;

	if (self->buf)
	    self->buf = (char *) erealloc(self->buf, request, "gap_accommodate");
	else
	    self->buf = (char *) emalloc(request, "gap_accommodate");

	self->allocated = request;
	safe_bcopy(self->buf + self->gapEnd,
		   self->buf + newGapEnd, tailSize);
	self->gapEnd = newGapEnd;
    }
}
	

static void
spText_insert(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    int                     pos, len, end, n, i;
    int                     newnewlines = 0;
    char                   *text;
    struct mark            *mark;
    enum spText_markType    instype;
    struct spText_changeinfo cinfo;

    pos = spArg(arg, int);
    len = spArg(arg, int);
    text = spArg(arg, char *);
    instype = spArg(arg, enum spText_markType);

    if (pos < 0)		/* append */
	pos = self->allocated - (self->gapEnd - self->gapStart);

    if (len < 0)
	len = strlen(text);

    for (i = 0; i < len; ++i) {
	if (text[i] == '\n')
	    ++newnewlines;
    }

    cinfo.pos = pos;
    cinfo.len = len;
    cinfo.newlines = newnewlines;

    gap_accommodate(self, len);

    /* Now the gap is big enough. */
    if (pos > self->gapStart) {
	n = pos - self->gapStart;
	safe_bcopy(self->buf + self->gapEnd, self->buf + self->gapStart, n);
	self->gapEnd += n;
	self->gapStart += n;
    } else if (pos < self->gapStart) {
	n = self->gapStart - pos;
	safe_bcopy(self->buf + pos, self->buf + self->gapEnd - n, n);
	self->gapStart -= n;
	self->gapEnd -= n;
    }
    /* Now the gap is in the right place. */
    bcopy(text, self->buf + self->gapStart, len);
    self->gapStart += len;
    if (self->updateMarks)
	dlist_FOREACH(&(self->marks), struct mark, mark, i) {
	    if ((mark->pos > pos)
		|| ((mark->pos == pos)
		    && ((instype == spText_mBefore)
			|| ((instype == spText_mNeutral)
			    && (mark->type == spText_mBefore))))) {
		mark->pos += len;
	    }
	}
    self->newlines += newnewlines;
    if (newnewlines)
	spSend(self, m_spObservable_notifyObservers, spText_linesChanged,
	       &cinfo);
    else
	spSend(self, m_spObservable_notifyObservers,
	       spObservable_contentChanged, &cinfo);
}

static void
spText_replace(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    int pos, origlen, newlen;
    char *str;
    enum spText_markType instype;
    int newlines = 0;		/* net change in # of newlines */
    int i;

    pos = spArg(arg, int);
    origlen = spArg(arg, int);
    newlen = spArg(arg, int);
    str = spArg(arg, char *);
    instype = spArg(arg, enum spText_markType);

    if (pos < 0)		/* append */
	pos = self->allocated - (self->gapEnd - self->gapStart);

    if (origlen < 0)		/* replace through EOT */
	origlen = (self->allocated - (self->gapEnd - self->gapStart)) - pos;

    if (newlen < 0)
	newlen = strlen(str);

    for (i = pos; i < (pos + origlen); ++i)
	if (spText_getc(self, i) == '\n')
	    --newlines;

    for (i = 0; i < newlen; ++i)
	if (str[i] == '\n')
	    ++newlines;

    if (origlen == newlen) {
	if (pos > self->gapStart) { /* gap precedes replacement */
	    bcopy(str, self->buf + pos + (self->gapEnd - self->gapStart),
		  newlen);
	} else if ((pos + origlen) <= self->gapStart) {	/* gap follows */
	    bcopy(str, self->buf + pos, newlen);
	} else {		/* gap intervenes */
	    int part1 = self->gapStart - pos;

	    bcopy(str, self->buf + pos, part1);
	    bcopy(str + part1, self->buf + self->gapEnd, newlen - part1);
	}
    } else {			/* size will change... */
	if (newlen < origlen) {	/* ...will shrink */
	    int n;

	    /* Place the gap at the end of the new text */
	    if ((pos + newlen) < self->gapStart) {
		n = self->gapStart - (pos + newlen);
		safe_bcopy(self->buf + pos + newlen,
			   self->buf + self->gapEnd - n, n);
		self->gapStart -= n;
		self->gapEnd -= n;
	    } else if ((pos + newlen) > self->gapStart) {
		n = (pos + newlen) - self->gapStart;
		safe_bcopy(self->buf + self->gapEnd,
			   self->buf + self->gapStart, n);
		self->gapEnd += n;
		self->gapStart += n;
	    }
	    /* Now the gap is in the right place */

	    bcopy(str, self->buf + pos, newlen);
	    self->gapEnd += (origlen - newlen);
	} else {		/* ...will grow */
	    int need = newlen - origlen;
	    int n;

	    gap_accommodate(self, need);

	    /* Place the gap at the start of the new text */
	    if (pos < self->gapStart) {	/* gap follows */
		n = self->gapStart - pos;
		safe_bcopy(self->buf + pos, self->buf + self->gapEnd - n, n);
		self->gapStart -= n;
		self->gapEnd -= n;
	    } else if (pos > self->gapStart) { /* gap precedes */
		n = pos - self->gapStart;
		safe_bcopy(self->buf + self->gapEnd,
			   self->buf + self->gapStart, n);
		self->gapStart += n;
		self->gapEnd += n;
	    }
	    /* Now the gap is in the right place */
	    bcopy(str, self->buf + pos, newlen);
	    self->gapStart += newlen;
	    self->gapEnd += origlen;
	}
	/* Now update marks */
	if (self->updateMarks) {
	    struct mark *mark;
	    int i;

	    dlist_FOREACH(&(self->marks), struct mark, mark, i) {
		if ((mark->pos > pos)
		    || ((mark->pos == pos)
			&& ((instype == spText_mBefore)
			    || ((instype == spText_mNeutral)
				&& (mark->type == spText_mBefore))))) {
		    if ((mark->pos < (pos + origlen))
			&& (mark->pos < (pos + newlen))) {
			/* Within old string and its replacement */
			/* Do nothing */
		    } else if (mark->pos >= (pos + origlen)) {
			/* Beyond old string and its replacement */
			mark->pos += (newlen - origlen);
		    } else {
			/* In the overlap */
			mark->pos = pos + newlen;
		    }
		}
	    }
	}
    }

    self->newlines += newlines;
    if (newlines)
	spSend(self, m_spObservable_notifyObservers,
	       spText_linesChanged, &newlines);
    else
	spSend(self, m_spObservable_notifyObservers,
	       spObservable_contentChanged, 0);
}

static void
spText_delete(self, arg)
    struct spText          *self;
    spArgList_t             arg;
{
    int                     pos, len, end, i, newlines = 0;
    struct mark            *mark;
    struct spText_changeinfo cinfo;

    pos = spArg(arg, int);
    len = spArg(arg, int);

    if (len < 0)
	len = (self->allocated - (self->gapEnd - self->gapStart)) - pos;

    end = pos + len;
    if (end <= self->gapStart) {
	/* CASE 1: area to be deleted is all in segment 1 */
	for (i = 0; i < len; ++i) {
	    if (self->buf[pos + i] == '\n')
		--newlines;
	}
	safe_bcopy(self->buf + end,
		   self->buf + pos,
		   self->gapStart - end);
	self->gapStart -= len;
    } else if ((pos <= self->gapStart)
	       && (end > self->gapStart)) {
	/* CASE 2: area to be deleted spans segments 1 and 2 */
	int                     seg1del = self->gapStart - pos;

	for (i = 0; i < seg1del; ++i) {
	    if (self->buf[pos + i] == '\n')
		--newlines;
	}
	for (i = seg1del; i < len; ++i) {
	    if (self->buf[self->gapEnd + i - seg1del] == '\n')
		--newlines;
	}
	self->gapStart = pos;
	self->gapEnd += (len - seg1del);
    } else if (pos > self->gapStart) {
	/* CASE 3: area to be deleted is all in segment 2 */
	safe_bcopy(self->buf + self->gapEnd, /* copy beginning of seg 2 */
		   self->buf + self->gapStart, /* to the end of seg 1 */
		   pos - self->gapStart);
	self->gapEnd += (pos - self->gapStart);
	self->gapStart = pos;
	/* Now area to be deleted is at the beginning of segment 2 */
	for (i = 0; i < len; ++i) {
	    if (self->buf[self->gapEnd + i])
		--newlines;
	}
	self->gapEnd += len;
    }
    self->newlines += newlines;
    if (self->updateMarks)
	dlist_FOREACH(&(self->marks), struct mark, mark, i) {
	    if (mark->pos >= end) {
		mark->pos -= len;
	    } else if (mark->pos > pos) {
		mark->pos = pos;
	    }
	}

    cinfo.pos = pos;
    cinfo.len = len;
    cinfo.newlines = newlines;

    if (newlines)
	spSend(self, m_spObservable_notifyObservers,
	       spText_linesChanged, &cinfo);
    else
	spSend(self, m_spObservable_notifyObservers,
	       spObservable_contentChanged, &cinfo);
}

static void
spText_substring(self, args)
    struct spText          *self;
    spArgList_t             args;
{
    int                     pos, len, n;
    char                   *buf, *ptr;

    pos = spArg(args, int);
    len = spArg(args, int);
    buf = spArg(args, char *);

    ptr = buf;
    if (pos < self->gapStart) {
	if (pos + len <= self->gapStart) {
	    safe_bcopy(self->buf + pos, ptr, len);
	} else {
	    n = self->gapStart - pos;
	    safe_bcopy(self->buf + pos, ptr, n);
	    ptr += n;
	    len -= n;
	    pos = self->gapStart;
	}
    }
    if (pos >= self->gapStart) {
	safe_bcopy(self->buf + pos + self->gapEnd - self->gapStart, ptr, len);
    }
}

static int
spText_length(self, arg)
    struct spText          *self;
    spArgList_t             arg;
{
    return (self->allocated - (self->gapEnd - self->gapStart));
}

static int
spText_addMark(self, arg)
    struct spText          *self;
    spArgList_t             arg;
{
    struct mark             m;

    m.pos = spArg(arg, int);
    m.type = spArg(arg, enum spText_markType);

    if (m.pos < 0)
	m.pos = self->allocated - (self->gapEnd - self->gapStart);
    dlist_Prepend(&(self->marks), &m);
    return (dlist_Head(&(self->marks)));
}

static void
spText_removeMark(self, arg)
    struct spText          *self;
    spArgList_t             arg;
{
    int                     markNum;

    markNum = spArg(arg, int);

    dlist_Remove(&(self->marks), markNum);
}

static void
spText_setMark(self, arg)
    struct spText          *self;
    spArgList_t             arg;
{
    int markNum, pos, end;

    markNum = spArg(arg, int);
    pos = spArg(arg, int);

    end = (self->allocated - (self->gapEnd - self->gapStart));

    if ((pos < 0)
	|| (pos > end))
	pos = end;
    ((struct mark *) dlist_Nth(&(self->marks), markNum))->pos = pos;
}

static void
spText_setReadOnly(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    int old = self->readOnly;

    self->readOnly = spArg(arg, int);

    if (!(self->readOnly) != !old)
	spSend(self, m_spObservable_notifyObservers,
	       spText_readOnlynessChanged, 0);
}

static void
spText_writePartial(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    FILE *fp;
    int start, len, end, Max;

    fp = spArg(arg, FILE *);
    start = spArg(arg, int);
    len = spArg(arg, int);
    end = self->allocated - (self->gapEnd - self->gapStart);
    Max = MIN(end, start + len);
    if (self->buf) {
	if (Max < self->gapStart) {		/* entire writable region
						 * is in seg1 */
	    efwrite(self->buf + start, 1, len,
		    fp, "spText_writePartial");
	} else if (start >= self->gapStart) {	/* entire region in seg2 */
	    efwrite((self->buf + start +
		     (self->gapEnd - self->gapStart)),
		    1, len, fp, "spText_writePartial");
	} else {				/* spans both segs */
	    efwrite(self->buf + start, 1,
		    self->gapStart - start,
		    fp, "spText_writePartial");
	    efwrite(self->buf + self->gapEnd, 1,
		    len - (self->gapStart - start),
		    fp, "spText_writePartial");
	}
    }
}

static void
spText_writeFile(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    char *filename;
    FILE *fp;

    filename = spArg(arg, char *);
    fp = efopen(filename, "w", "spText_writeFile");
    TRY {
	spSend(self, m_spText_writePartial, fp, 0,
	       self->allocated - (self->gapEnd - self->gapStart));
    } FINALLY {
	fclose(fp);
    } ENDTRY;
}

static void
spText_readFile(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    char *filename;
    FILE *fp;
    struct stat statbuf;
    int i;
    
    filename = spArg(arg, char *);
    estat(filename, &statbuf, "spText_readFile");
    fp = efopen(filename, "r", "spText_readFile");
    TRY {
	spSend(self, m_spText_clear);
	gap_accommodate(self, statbuf.st_size);
	efread(self->buf, 1, statbuf.st_size, fp, "spText_readFile");
	self->newlines = 0;
	for (i = 0; i < statbuf.st_size; ++i) {
	    if (self->buf[i] == '\n')
		++(self->newlines);
	}
	self->gapStart = statbuf.st_size;
    } FINALLY {
	fclose(fp);
    } ENDTRY;
    /* Leave all old marks at beginning of text (ecch, bogus) */
}

static int
spText_rxpSearch(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    regexp_t rxp;
    struct re_registers regs;
    int pos, *after, result, lastpos;

    rxp = spArg(arg, regexp_t);
    pos = spArg(arg, int);
    after = spArg(arg, int *);

    lastpos = (self->allocated - (self->gapEnd - self->gapStart));

    result = re_search_2(rxp, self->buf, self->gapStart,
			 self->buf + self->gapEnd,
			 self->allocated - self->gapEnd,
			 pos, lastpos - pos, &regs, lastpos);
    if ((result >= 0) && after)
	*after = regs.end[0];
    return ((result >= 0) ? result : -1);
}

static void
spText_appendToDynstr(self, arg)
    struct spText *self;
    spArgList_t arg;
{
    struct dynstr *d;
    int pos, len, n;

    d = spArg(arg, struct dynstr *);
    pos = spArg(arg, int);
    len = spArg(arg, int);

    if (len < 0)
	len = (self->allocated - (self->gapEnd - self->gapStart)) - pos;
    if (pos < self->gapStart) {
	if (pos + len <= self->gapStart) {
	    dynstr_AppendN(d, self->buf + pos, len);
	} else {
	    n = self->gapStart - pos;
	    dynstr_AppendN(d, self->buf + pos, n);
	    len -= n;
	    pos = self->gapStart;
	}
    }
    if (pos >= self->gapStart) {
	dynstr_AppendN(d, self->buf + pos + self->gapEnd - self->gapStart,
		       len);
    }
}

void
spText_InitializeClass()
{
    if (!spObservable_class)
	spObservable_InitializeClass();
    if (spText_class)
	return;
    spText_class = spoor_CreateClass("spText", "editable text",
				     spObservable_class,
				     (sizeof (struct spText)),
				     spText_initialize,
				     spText_finalize);

    /* No overrides */
    m_spText_clear = spoor_AddMethod(spText_class, "clear",
				     "empty the buffer", spText_clear);
    m_spText_insert = spoor_AddMethod(spText_class, "insert",
				      "insert characters", spText_insert);
    m_spText_replace = spoor_AddMethod(spText_class, "replace",
				       "replace characters", spText_replace);
    m_spText_delete = spoor_AddMethod(spText_class, "delete",
				      "delete characters", spText_delete);
    m_spText_substring = spoor_AddMethod(spText_class, "substring",
					 "extract substring from buffer",
					 spText_substring);
    m_spText_length = spoor_AddMethod(spText_class, "length",
				      "number of characters in buffer",
				      spText_length);
    m_spText_addMark = spoor_AddMethod(spText_class, "addMark",
				       "put a new mark in the text",
				       spText_addMark);
    m_spText_removeMark = spoor_AddMethod(spText_class, "removeMark",
					  "take a mark out of the text",
					  spText_removeMark);
    m_spText_setMark = spoor_AddMethod(spText_class, "setMark",
				       "move mark to specified position",
				       spText_setMark);
    m_spText_setReadOnly =
	spoor_AddMethod(spText_class, "setReadOnly",
			"turn read-onlyness on or off",
			spText_setReadOnly);
    m_spText_writeFile =
	spoor_AddMethod(spText_class, "writeFile",
			"write text to a file",
			spText_writeFile);
    m_spText_readFile =
	spoor_AddMethod(spText_class, "readFile",
			"read text from a file",
			spText_readFile);
    m_spText_writePartial =
	spoor_AddMethod(spText_class, "writePartial",
			"write a portion of text to a file",
			spText_writePartial);
    m_spText_appendToDynstr =
	spoor_AddMethod(spText_class, "appendToDynstr",
			"append text substring to a dynstr",
			spText_appendToDynstr);
    m_spText_rxpSearch =
	spoor_AddMethod(spText_class, "rxpSearch",
			"return index of first occurrence of regexp",
			spText_rxpSearch);
}

int
spText_getc(self, pos)		/* for speed */
    struct spText *self;
    int pos;
{
    return ((pos < self->gapStart) ?
	    (self->buf[pos]) :
	    (self->buf[pos + self->gapEnd - self->gapStart]));
}

int
spText_markPos(self, mark)
    struct spText *self;
    int mark;
{
    return (((struct mark *) dlist_Nth(&(self->marks), mark))->pos);
}
