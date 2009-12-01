/*
 * $RCSfile: keymap.c,v $
 * $Revision: 2.21 $
 * $Date: 1995/02/12 02:16:10 $
 * $Author: bobg $
 */

#include <general.h>
#include <keymap.h>
#include <dynstr.h>
#include <excfns.h>
#include <ctype.h>
#include <hashtab.h>
#include <strcase.h>

#ifndef lint
static const char spKeymap_rcsid[] =
    "$Id: keymap.c,v 2.21 1995/02/12 02:16:10 bobg Exp $";
#endif /* lint */

static char *
Strdup(str)
    char *str;
{
    char *result;

    if (!str)
	return ((char *) 0);
    result = emalloc(1 + strlen(str), "Strdup");
    strcpy(result, str);
    return (result);
}

static int
keymapEntryCmp(a, b)
    struct spKeymapEntry *a, *b;
{
    return (a->c - b->c);
}

static void
keymapEntryFinalize(kme)
    struct spKeymapEntry *kme;
{
    switch (kme->type) {
      case spKeymap_function:
	if (kme->content.function.doc)
	    free(kme->content.function.doc);
	if (kme->content.function.fn)
	    free(kme->content.function.fn);
	if (kme->content.function.obj)
	    free(kme->content.function.obj);
	if (kme->content.function.data)
	    free(kme->content.function.data);
	if (kme->content.function.label)
	    free(kme->content.function.label);
	break;
      case spKeymap_keymap:
	spKeymap_Destroy(kme->content.keymap);
	free(kme->content.keymap);
	break;
      case spKeymap_translation:
	spKeysequence_Destroy(&(kme->content.translation));
	break;
    }
}

void
spKeymap_Destroy(km)
    struct spKeymap *km;
{
    sklist_CleanDestroy(&(km->entries), keymapEntryFinalize);
}

void
spKeymap_Init(km)
    struct spKeymap *km;
{
    sklist_Init(&(km->entries), (sizeof (struct spKeymapEntry)),
		keymapEntryCmp, 1, 4);
}

static struct spKeymap *
findKeymap(inkm, keyseq, create)
    struct spKeymap *inkm;
    struct spKeysequence *keyseq;
    int create;
{
    struct spKeymap *km = inkm;
    int l = 0;
    struct spKeymapEntry *kme, probe;

    while (l < (spKeysequence_Length(keyseq) - 1)) {
	probe.c = spKeysequence_Nth(keyseq, l++);
	if (kme = (struct spKeymapEntry *) sklist_Find(&(km->entries),
						       &probe, 1)) {
	    if (kme->type != spKeymap_keymap) {
		if (create) {
		    sklist_CleanRemove(&(km->entries), 0,
				       keymapEntryFinalize);
		    probe.type = spKeymap_keymap;
		    probe.content.keymap = ((struct spKeymap *)
					    emalloc(sizeof (struct spKeymap),
						    "findKeymap"));
		    spKeymap_Init(probe.content.keymap);
		    sklist_Insert(&(km->entries), &probe);
		    km = probe.content.keymap;
		} else {
		    return ((struct spKeymap *) 0);
		}
	    } else {
		km = kme->content.keymap;
	    }
	} else {
	    if (create) {
		probe.type = spKeymap_keymap;
		probe.content.keymap = ((struct spKeymap *)
					emalloc(sizeof (struct spKeymap),
						"findKeymap"));
		spKeymap_Init(probe.content.keymap);
		sklist_Insert(&(km->entries), &probe);
		km = probe.content.keymap;
	    } else {
		return ((struct spKeymap *) 0);
	    }
	}
    }
    return (km);
}

void
spKeymap_AddFunction(inkm, keyseq, fn, obj, data, label, doc)
    struct spKeymap *inkm;
    struct spKeysequence *keyseq;
    char *fn, *obj, *data, *label, *doc;
{
    struct spKeymap *km = findKeymap(inkm, keyseq, 1);
    struct spKeymapEntry kme;

    kme.c = spKeysequence_Last(keyseq);
    kme.type = spKeymap_function;
    kme.content.function.fn = Strdup(fn);
    kme.content.function.obj = Strdup(obj);
    kme.content.function.data = Strdup(data);
    kme.content.function.label = Strdup(label);
    kme.content.function.doc = Strdup(doc);
    sklist_CleanRemove(&(km->entries), &kme, keymapEntryFinalize);
    sklist_Insert(&(km->entries), &kme);
}

void
spKeymap_AddTranslation(inkm, fromks, toks)
    struct spKeymap *inkm;
    struct spKeysequence *fromks, *toks;
{
    struct spKeymap *km = findKeymap(inkm, fromks, 1);
    struct spKeymapEntry kme;

    kme.c = spKeysequence_Last(fromks);
    kme.type = spKeymap_translation;
    spKeysequence_Init(&(kme.content.translation));
    spKeysequence_Concat(&(kme.content.translation), toks);
    sklist_CleanRemove(&(km->entries), &kme, keymapEntryFinalize);
    sklist_Insert(&(km->entries), &kme);
}

void
spKeymap_Remove(inkm, keyseq)
    struct spKeymap *inkm;
    struct spKeysequence *keyseq;
{
    struct spKeymap *km = findKeymap(inkm, keyseq, 1);
    struct spKeymapEntry kme;

    kme.c = spKeysequence_Last(keyseq);
    kme.type = spKeymap_removed;
    sklist_CleanRemove(&(km->entries), &kme, keymapEntryFinalize);
    sklist_Insert(&(km->entries), &kme);
}

static int
reallyremove(level, km, ks)
    int level;
    struct spKeymap *km;
    struct spKeysequence *ks;
{
    struct spKeymapEntry probe;
    struct spKeymapEntry *kme;

    probe.c = spKeysequence_Nth(ks, level);
    kme = (struct spKeymapEntry *) sklist_Find(&(km->entries), &probe, 1);

    if (level == (spKeysequence_Length(ks) - 1)) {
	if (kme) {
	    sklist_CleanRemove(&(km->entries), 0, keymapEntryFinalize);
	} else {
	    return (0);
	}
    } else {
	if (kme && (kme->type == spKeymap_keymap)) {
	    if (reallyremove(level + 1, kme->content.keymap, ks)) {
		sklist_CleanRemove(&(km->entries), &probe,
				   keymapEntryFinalize);
	    } else {
		return (0);
	    }
	} else {
	    return (0);
	}
    }
    return (sklist_EmptyP(&(km->entries)));
}

void
spKeymap_ReallyRemove(inkm, keyseq)
    struct spKeymap *inkm;
    struct spKeysequence *keyseq;
{
    reallyremove(0, inkm, keyseq);
}

struct spKeymapEntry *
spKeymap_First(km)
    struct spKeymap *km;
{
    return (sklist_First(&(km->entries)));
}

struct spKeymapEntry *
spKeymap_Next(km, kme)
    struct spKeymap *km;
    struct spKeymapEntry *kme;
{
    return (sklist_Next(&(km->entries), kme));
}

struct spKeymapEntry *
spKeymap_lookup(inkm, keyseq)
    struct spKeymap *inkm;
    struct spKeysequence *keyseq;
{
    struct spKeymap *km = findKeymap(inkm, keyseq, 0);

    if (km) {
	struct spKeymapEntry probe;

	probe.c = spKeysequence_Last(keyseq);
	return ((struct spKeymapEntry *)
		sklist_Find(&(km->entries), &probe, 0));
    }
    return ((struct spKeymapEntry *) 0);
}

static struct glist keynamelist;
static struct hashtab keynametable;

struct kntabEntry {
    char *name;
    int key;
};

static unsigned int
keynamehash(k)
    struct kntabEntry *k;
{
    return (hashtab_StringHash(k->name));
}

static int
keynamecmp(k1, k2)
    struct kntabEntry *k1, *k2;
{
    return (ci_strcmp(k1->name, k2->name));
}

int
spKeynameList_Add(name)
    char *name;
{
    int result = 256 + glist_Length(&keynamelist);
    struct kntabEntry new;

    new.name = Strdup(name);
    new.key = result;
    glist_Add(&keynamelist, &(new.name));
    hashtab_Add(&keynametable, &new);
    return (result);
}

static struct dfltname {
    char * name;
    int val;
} dfltnames[] = {
    { "nul", 0 },
    { "tab", 9 },
    { "newline", 10 },
    { "return", 13 },
    { "esc", 27 },
    { "space", 32 },
    { "del", 127 }
};

void
spKeynameList_Init()
{
    char buf[10];
    int i;
    struct kntabEntry k;

    glist_Init(&keynamelist, (sizeof (char *)), 8);
    hashtab_Init(&keynametable, keynamehash, keynamecmp,
		 sizeof (struct kntabEntry), 47);

    /* Note: these keys do *not* get added to keynamelist! */
    for (i = 0; i < (sizeof (dfltnames) / sizeof (struct dfltname)); ++i) {
	k.name = dfltnames[i].name;
	k.key = dfltnames[i].val;
	hashtab_Add(&keynametable, &k);
    }

    k.name = "ctrl+@";
    k.key = 0;
    hashtab_Add(&keynametable, &k);

    for (i = 1; i <= 26; ++i) {
	sprintf(buf, "ctrl+%c", i + 'a' - 1);
	k.name = Strdup(buf);
	k.key = i;
	hashtab_Add(&keynametable, &k);
    }
    for (i = 27; i < 32; ++i) {
	sprintf(buf, "ctrl+%c", i + '[' - 27);
	k.name = Strdup(buf);
	k.key = i;
	hashtab_Add(&keynametable, &k);
    }
}

char *
spKeyname(ch, pretty)
    int ch, pretty;
{
    static struct dynstr d;
    static int initialized = 0;
    static char buf[16];
    int i;

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }

    for (i = 0; i < (sizeof (dfltnames) / sizeof (struct dfltname)); ++i) {
	if (ch == dfltnames[i].val) {
	    if (pretty) {
		return (dfltnames[i].name);
	    } else {
		dynstr_Set(&d, "\\<");
		dynstr_Append(&d, dfltnames[i].name);
		dynstr_AppendChar(&d, '>');
		return (dynstr_Str(&d));
	    }
	}
    }
    if (ch <= ' ') {
	if (ch > 26) {
	    if (pretty)
		sprintf(buf, "ctrl+%c", '[' + ch - 27);
	    else
		sprintf(buf, "\\<ctrl+%c>", '[' + ch - 27);
	} else {
	    if (pretty)
		sprintf(buf, "ctrl+%c", 'a' + ch - 1);
	    else
		sprintf(buf, "\\<ctrl+%c>", 'a' + ch - 1);
	}
	return (buf);
    } else if (ch < 128) {
	switch (ch) {
	  case '\\':
	    return (pretty ? "\\" : "\\\\");
	  case '^':
	    return (pretty ? "^" : "\\^");
	  default:
	    buf[0] = ch;
	    buf[1] = '\0';
	    return (buf);
	}
    } else if (ch < 256) {
	sprintf(buf, "\\%03o", ch);
	return (buf);
    } else if ((ch - 256) < glist_Length(&keynamelist)) {
	if (pretty) {
	    return (*((char **) glist_Nth(&keynamelist, ch - 256)));
	} else {
	    sprintf(buf, "\\<%s>", *((char **) glist_Nth(&keynamelist,
							 ch - 256)));
	    return (buf);
	}
    } else {
	sprintf(buf, "spKeyname(%d)", ch);
	RAISE(strerror(EINVAL), buf);
    }
}

void
spKeysequence_Concat(destks, sourceks)
    struct spKeysequence *destks, *sourceks;
{
    int i;

    for (i = 0; i < spKeysequence_Length(sourceks); ++i)
	spKeysequence_Add(destks, spKeysequence_Nth(sourceks, i));
}

void
spKeysequence_Truncate(ks, newlen)
    struct spKeysequence *ks;
    int newlen;
{
    while (spKeysequence_Length(ks) > newlen)
	glist_Pop(&(ks->elts));
}

int
spKeysequence_Chop(ks)
    struct spKeysequence *ks;
{
    int result = spKeysequence_Last(ks);

    glist_Pop(&(ks->elts));
    return (result);
}

void
spKeysequence_Add(ks, c)
    struct spKeysequence *ks;
    int c;
{
    glist_Add(&(ks->elts), &c);
}

struct spKeysequence *
spKeysequence_Parse(inks, names, create)
    struct spKeysequence *inks;
    char *names;
    int create;
{
    char *p = names;
    int ch;
    static struct spKeysequence staticks;
    static int initialized = 0;
    struct spKeysequence *ks = inks;

    if (!ks) {
	if (!initialized) {
	    spKeysequence_Init(&staticks);
	    initialized = 1;
	} else {
	    spKeysequence_Truncate(&staticks, 0);
	}
	ks = &staticks;
    }

    while (p && *p) {
	switch (*p) {
	  case '^':
	    if (((ch = *++p), !Upper(ch))
		|| !isascii(ch)
		|| (ch < '?')
		|| (ch > '_'))
		RAISE(strerror(EINVAL), "spKeysequence_Parse");
	    spKeysequence_Add(ks, (ch == '?') ? 127 : (ch - ('A' - 1)));
	    ++p;
	    break;
	  case '\\':
	    if (!(ch = *(++p))) {
		RAISE(strerror(EINVAL), "spKeysequence_Parse");
	    } else if (ch == '<') {
		char *e = (char *) index(++p, '>');
		struct dynstr d;
		struct kntabEntry *k, probe;

		if (!e)
		    RAISE(strerror(EINVAL), "spKeysequence_Parse");
		dynstr_Init(&d);
		TRY {
		    dynstr_AppendN(&d, p, e - p);
		    if ((dynstr_Length(&d) > 2)
			&& !ci_strncmp("c-", dynstr_Str(&d), 2))
			dynstr_Replace(&d, 0, 2, "ctrl+"); /* backward
							    * compatibility */
		    probe.name = dynstr_Str(&d);
		    if (k = (struct kntabEntry *) hashtab_Find(&keynametable,
							       &probe)) {
			spKeysequence_Add(ks, k->key);
		    } else if (create) {
			spKeysequence_Add(ks,
					  spKeynameList_Add(dynstr_Str(&d)));
		    } else {
			RAISE(strerror(EINVAL), "spKeysequence_Parse");
		    }
		} FINALLY {
		    dynstr_Destroy(&d);
		} ENDTRY;
		p = e + 1;
	    } else {
		int a;

		switch (lower(ch)) {
		  case '0':
		  case '1':
		  case '2':
		  case '3':
		  case '4':
		  case '5':
		  case '6':
		  case '7':
		    a = 8 * 8 * (ch - '0');
		    if (*(p + 1)) {
			a += 8 * (*(++p) - '0');
		    }
		    if (*(p + 1)) {
			a += *(++p) - '0';
		    }
		    spKeysequence_Add(ks, a);
		    break;
		  case 'b':
		  case 'B':
		    spKeysequence_Add(ks, 8); /* ^H, or backspace */
		    break;
		  case 'e':
		  case 'E':
		    spKeysequence_Add(ks, 27); /* ESC */
		    break;
		  case 'f':
		  case 'F':
		    spKeysequence_Add(ks, 12); /* ^L, or formfeed */
		    break;
		  case 'g':
		  case 'G':
		    spKeysequence_Add(ks, 7); /* ^G */
		    break;
		  case 'n':
		  case 'N':
		    spKeysequence_Add(ks, '\n');
		    break;
		  case 'r':
		  case 'R':
		    spKeysequence_Add(ks, '\r');
		    break;
		  case 't':
		  case 'T':
		    spKeysequence_Add(ks, '\t');
		    break;
		  default:
		    spKeysequence_Add(ks, ch);
		    break;
		}
		++p;
	    }
	    break;
	  default:
	    spKeysequence_Add(ks, *(p++));
	    break;
	}
    }
    return (ks);
}

int
spKeyMax()
{
    return (255 + glist_Length(&keynamelist));
}

