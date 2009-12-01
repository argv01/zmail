/* 
 * $RCSfile: zmlite.c,v $
 * $Revision: 2.108 $
 * $Date: 1998/12/07 23:56:33 $
 * $Author: schaefer $
 */

#include <ctype.h>
#include "dialog.h" /* to circumvent "unreasonable include nesting" */
#include <zmlite.h>

#include <dynstr.h>
#include <sklist.h>
#include <hashtab.h>

#include <spoor.h>
#include <spoor/window.h>
#include <spoor/event.h>
#include <spoor/popupv.h>
#include <spoor/event.h>

#include <addrbook.h>
#include <attchlst.h>
#include <attchtyp.h>
#include <aliasf.h>
#include <charsetf.h>
#include <choose1.h>
#include <compopt.h>
#include <dsearch.h>
#include <dynhdrs.h>
#include <envf.h>
#include <hdrsf.h>
#include <helpd.h>
#include <helpindx.h>
#include <mainf.h>
#include <msgf.h>
#include <multikey.h>
#include <notifier.h>
#include <pagerd.h>
#include <printd.h>
#include <psearch.h>
#include <smalias.h>
#include <sortd.h>
#include <taskmtr.h>
#include <tmpld.h>
#include <tsearch.h>
#include <varf.h>
#include <zmlapp.h>

#include <zmail.h>
#include <child.h>
#include <cmdtab.h>
#include <fetch.h>
#include <file.h>
#include <pager.h>
#include <popenv.h>

#include "catalog.h"

#ifndef lint
static const char zmlite_rcsid[] =
    "$Id: zmlite.c,v 2.108 1998/12/07 23:56:33 schaefer Exp $";
#endif /* lint */

#define LXOR(a,b) ((!(a))!=(!(b)))

GuiItem ask_item;		/* Grr! */

static int zmlErrorLevel = 0;
static int screencmd_pending = 0;
static int LastHistory = -1;
static int WaitingForChildInteractively = 0;
static int ZScriptPending = 0;

static struct dlist CommandHistory;

static struct sklist UniqueMap;

static int stopped = -1;	/* like in m_intr.c */
char *tool_help;

struct zmlapp *ZmlIm = 0;

struct options **ZmlUpdatedList;
u_long RefreshReason;

int taskmeter_up = 0;

/* The BindKey queue.
 * It's a hashtable of dlists.
 * Each hashtable entry refers to a single class or named widget.
 * The dlist entries beneath are deferred bindkey or unbindkey commands
 *  for that class or named widget.
 */

struct bkqlistelt {
    int unb;
    char *seq, *fn, *arg, *label, *doc;
};

struct bklist {
    char *name;
    int classp;
    struct dlist elts;
};

#if defined( IMAP )
/* XXX */

#define LATT_NOINFERIORS 1
#define LATT_NOSELECT 2
#endif

static int bkqinitialized = 0;
static struct hashtab BKqueue;

GuiItem tool;

struct zmlaliasframe *AliasDialog = 0;
struct addrbook *AddrbrowseDialog = 0;
struct charsets *CharsetDialog = 0;
struct smallaliases *CompAliasesDialog = 0;
struct compopt *CompOptionsDialog = 0;
struct dsearch *DatesearchDialog = 0;
struct zmlenvframe *EnvelopeDialog = 0;
struct zmlhdrsframe *HeadersDialog = 0;
struct helpIndex *HelpIndexDialog = 0;
struct zmlmainframe *MainDialog = 0;
struct zmlmsgframe *MessageDialog = 0;
struct multikey *MultikeyDialog = 0;
struct printdialog *PrinterDialog = 0;
struct psearch *SearchDialog = 0;
struct sortdialog *SortDialog = 0;
struct templatedialog *TemplateDialog = 0;
struct zmlvarframe *VariablesDialog = 0;

struct zmlcomposeframe *ComposeDialog = 0;

#define VSN ("version")

#define IDEALCOLUMN (16)
#define SPACES ("                ") /* exactly IDEALCOLUMN spaces */

static void
showkey_emit(d, ks, startch, endch, fn, obj, data, doc, pager)
    struct dynstr *d;
    struct spKeysequence *ks;
    int startch, endch;
    char *fn, *obj, *data, *doc;
    struct zmPager *pager;
{
    int i;

    if (spKeysequence_Length(ks) > 1) {
	dynstr_Set(d, spKeyname(spKeysequence_Nth(ks, 0), 1));
	for (i = 1; i < (spKeysequence_Length(ks) - 1); ++i) {
	    dynstr_AppendChar(d, ' ');
	    dynstr_Append(d, spKeyname(spKeysequence_Nth(ks, i), 1));
	}
	dynstr_AppendChar(d, ' ');
	dynstr_Append(d, spKeyname(startch, 1));
    } else {
	dynstr_Set(d, spKeyname(startch, 1));
    }
    if (endch > startch) {
	dynstr_Append(d, " .. ");
	if (spKeysequence_Length(ks) > 1) {
	    dynstr_Append(d, spKeyname(spKeysequence_Nth(ks, 0), 1));
	    for (i = 1; i < (spKeysequence_Length(ks) - 1); ++i) {
		dynstr_AppendChar(d, ' ');
		dynstr_Append(d, spKeyname(spKeysequence_Nth(ks, i), 1));
	    }
	    dynstr_AppendChar(d, ' ');
	    dynstr_Append(d, spKeyname(endch, 1));
	} else {
	    dynstr_Append(d, spKeyname(endch, 1));
	}
    }
    if (dynstr_Length(d) < (IDEALCOLUMN - 2)) {
	dynstr_AppendN(d, SPACES, IDEALCOLUMN - dynstr_Length(d));
    } else {
	dynstr_Append(d, "    ");
    }
    if (doc) {
	dynstr_Append(d, doc);
	dynstr_AppendChar(d, '\n');
	dynstr_Append(d, SPACES);
	dynstr_Append(d, "  [");
    }
    dynstr_Append(d, fn);
    if (obj) {
	dynstr_AppendChar(d, ' ');
	dynstr_Append(d, obj);
    }
    if (data) {
	dynstr_Append(d, " (");
	dynstr_Append(d, data);
	dynstr_Append(d, ")");
    }
    if (doc) {
	dynstr_AppendChar(d, ']');
    }
    dynstr_AppendChar(d, '\n');
    ZmPagerWrite(pager, dynstr_Str(d));
}

/* return value of 1 means this call printed something, so next call
 *   will need newline.
 * return value of 2 means this call printed nothing, but caller passed
 *   in neednewline.
 * return value of 0 means nothing was printed and caller didn't
 *   need newline.
 */
static int
showkeyrange(pager, prefix, queue, start, end, neednewline)
    ZmPager pager;
    struct spKeysequence *prefix;
    struct dlist *queue;
    int start, end, neednewline;
{
    static struct dynstr d;
    static int initialized = 0;
    int anything = 0, in_neednewline = neednewline;
    int level = spKeysequence_Length(prefix);
    int ch;
    struct spKeymapEntry *kme;
    struct spView *handler = 0;
    struct {
	char *fn, *obj, *data, *doc;
	int ch, docfromkey;
    } save;

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    }

    save.fn = 0;
    for (ch = start; ch <= end; ++ch) {
	spKeysequence_Truncate(prefix, level);
	spKeysequence_Add(prefix, ch);
	if (kme = ((struct spKeymapEntry *)
		   spSend_p(ZmlIm, m_spIm_lookupKeysequence, prefix,
			    &handler))) {
	    if (kme->type == spKeymap_keymap) {
		struct spKeysequence *newks;
		int i;

		newks = ((struct spKeysequence *)
			 emalloc(sizeof (struct spKeysequence),
				 "show-keys"));
		spKeysequence_Init(newks);
		for (i = 0; i < spKeysequence_Length(prefix); ++i)
		    spKeysequence_Add(newks,
				      spKeysequence_Nth(prefix, i));
		dlist_Append(queue, &newks);
		if (save.fn) {
		    if (neednewline
			|| (anything && ((ch - 1) > save.ch)))
			ZmPagerWrite(pager, "\n");
		    neednewline = ((ch - 1) > save.ch);
		    showkey_emit(&d, prefix, save.ch, ch - 1,
				 save.fn, save.obj, save.data,
				 save.doc, pager);
		    anything = 1;
		}
		save.fn = 0;
		ZmPagerWrite(pager, "\n");
		showkey_emit(&d, prefix, ch, ch, catgets(catalog, CAT_LITE, 715, "Prefix key"),
			     0, 0, 0, pager);
		anything = 1;
		neednewline = 1;
	    } else if (kme->type == spKeymap_function) {
		char *doc = kme->content.function.doc;
		int docfromkey = 0, samefn;

		samefn = (save.fn && !strcmp(kme->content.function.fn,
					     save.fn));

		if (doc) {
		    docfromkey = 1;
		} else if (samefn && !save.docfromkey) {
		    doc = save.doc;
		} else {
		    struct spWidgetInfo *wc = spView_getWclass(handler);
		    char *fn = kme->content.function.fn;

		    do {
			doc = spWidget_InteractionDoc(wc, fn);
			wc = spWidgetInfo_super(wc);
		    } while (wc && !doc);
		}

		if (save.fn) {
		    if (!samefn
			|| LXOR(doc, save.doc)
			|| (doc
			    && save.doc
			    && strcmp(doc, save.doc))
			|| LXOR(kme->content.function.obj, save.obj)
			|| LXOR(kme->content.function.data, save.data)
			|| (save.obj && strcmp(kme->content.function.obj,
					       save.obj))
			|| (save.data && strcmp(kme->content.function.data,
						save.data))) {
			if (neednewline
			    || (anything && ((ch - 1) > save.ch)))
			    ZmPagerWrite(pager, "\n");
			neednewline = ((ch - 1) > save.ch);
			showkey_emit(&d, prefix, save.ch, ch - 1,
				     save.fn, save.obj, save.data,
				     save.doc, pager);
			anything = 1;
			save.fn = 0;
		    } /* else do nothing */
		}
		if (!save.fn) {
		    save.ch = ch;
		    save.fn = kme->content.function.fn;
		    save.obj = kme->content.function.obj;
		    save.data = kme->content.function.data;
		    save.doc = doc;
		    save.docfromkey = docfromkey;
		}
	    } /* else do nothing */
	} else if (save.fn) {
	    if (neednewline
		|| (anything && ((ch - 1) > save.ch)))
		ZmPagerWrite(pager, "\n");
	    neednewline = ((ch - 1) > save.ch);
	    showkey_emit(&d, prefix, save.ch, ch - 1,
			 save.fn, save.obj, save.data, save.doc, pager);
	    anything = 1;
	    save.fn = 0;
	}
    }
    if (save.fn) {
	if (neednewline
	    || (anything && ((ch - 1) > save.ch)))
	    ZmPagerWrite(pager, "\n");
	neednewline = ((ch - 1) > save.ch);
	showkey_emit(&d, prefix, save.ch, ch - 1,
		     save.fn, save.obj, save.data, save.doc, pager);
	anything = 1;
	save.fn = 0;
    }
    spKeysequence_Truncate(prefix, level);
    return (anything ? 1 : (in_neednewline ? 2 : 0));
}

static void
showAllKeys_aux(pager)
    ZmPager pager;
{
    struct dlist queue;
    struct spKeysequence *prefix = 0;
    int neednewline = 0, willneednewline;
    int i;

    dlist_Init(&queue, (sizeof (struct spKeysequence *)), 4);
    prefix = (struct spKeysequence *) emalloc(sizeof (struct spKeysequence),
					      "show-keys");
    spKeysequence_Init(prefix);
    TRY {
	dlist_Append(&queue, &prefix);
	do {
	    prefix = *((struct spKeysequence **)
		       dlist_Nth(&queue, dlist_Head(&queue)));
	    dlist_Remove(&queue, dlist_Head(&queue));

	    /* We need to cover 0 through [spKeyMax() - 1] */
	    neednewline = showkeyrange(pager, prefix, &queue, 'a', 'z',
				       neednewline);
	    /* Now it's {0 - [a - 1]} {[z + 1] - [spKeyMax() - 1]} */
	    neednewline = showkeyrange(pager, prefix, &queue, 'A', 'Z',
				       neednewline);
	    /* {0 - [A - 1]}
	       {[Z + 1] - [a - 1]}
	       {[z + 1] - [spKeyMax() - 1])} */
	    neednewline = showkeyrange(pager, prefix, &queue, '0', '9',
				       neednewline);

	    /* {0 - ['0' - 1]}
	       {['9' + 1] - [A - 1]}
	       {[Z + 1] - [a - 1]}
	       {[z + 1] - [spKeyMax() - 1])} */

	    if (neednewline)
		neednewline = 2;

	    for (i = ' '; i < '0'; ++i) {
		neednewline = showkeyrange(pager, prefix, &queue,
					   i, i, neednewline == 2);
	    }
	    /* {0 - [' ' - 1]}
	       {['9' + 1] - [A - 1]}
	       {[Z + 1] - [a - 1]}
	       {[z + 1] - [spKeyMax() - 1])} */
	    for (i = '9' + 1; i < 'A'; ++i) {
		neednewline = showkeyrange(pager, prefix, &queue,
					   i, i, neednewline == 2);
	    }
	    /* {0 - [' ' - 1]}
	       {[Z + 1] - [a - 1]}
	       {[z + 1] - [spKeyMax() - 1])} */
	    for (i = 'Z' + 1; i < 'a'; ++i) {
		neednewline = showkeyrange(pager, prefix, &queue,
					   i, i, neednewline == 2);
	    }
	    /* {0 - [' ' - 1]}
	       {[z + 1] - [spKeyMax() - 1])} */
	    for (i = 'z' + 1; i < 127; ++i) {
		neednewline = showkeyrange(pager, prefix, &queue,
					   i, i, neednewline == 2);
	    }
	    /* {0 - [' ' - 1]}
	       {127 - [spKeyMax() - 1])} */
	    neednewline = showkeyrange(pager, prefix, &queue, 27, 27,
				       neednewline);
	    /* {0 - 26}
	       {28 - [' ' - 1]}
	       {127 - [spKeyMax() - 1])} */
	    neednewline = showkeyrange(pager, prefix, &queue, 1, 26,
				       neednewline);
	    /* {0}
	       {28 - [' ' - 1]}
	       {127 - [spKeyMax() - 1])} */
	    neednewline = showkeyrange(pager, prefix, &queue, 0, 0,
				       neednewline);
	    /* {28 - [' ' - 1]}
	       {127 - [spKeyMax() - 1])} */
	    neednewline = showkeyrange(pager, prefix, &queue, 127, 127,
				       neednewline == 2);
	    /* {28 - [' ' - 1]}
	       {128 - [spKeyMax() - 1])} */
	    for (i = 28; i < ' '; ++i) {
		neednewline = showkeyrange(pager, prefix, &queue,
					   i, i, neednewline == 2);
	    }
	    /* {128 - [spKeyMax() - 1])} */

	    if (willneednewline = neednewline)
		neednewline = 2;

	    for (i = 128; i < spKeyMax(); ++i) {
		neednewline = showkeyrange(pager, prefix, &queue,
					   i, i, neednewline == 2);
	    }
	    /* Got 'em all */

	    neednewline = (neednewline || willneednewline);

	    spKeysequence_Destroy(prefix);
	    free(prefix);
	} while (!dlist_EmptyP(&queue));
    } FINALLY {
	struct spKeysequence **kspp;

	dlist_FOREACH(&queue, struct spKeysequence *, kspp, i) {
	    spKeysequence_Destroy(*kspp);
	    free(*kspp);
	}
	dlist_Destroy(&queue);
    } ENDTRY;
}

static void
showcontext(pager)
    ZmPager pager;
{
    struct spView *v = spIm_focusView(ZmlIm);
    struct dynstr d,d2;
    char *p;
    int level = 0, i, spaces;

    if (!v)
	return;
    dynstr_Init(&d);
    dynstr_Init(&d2);
    TRY {
	while (v) {
	    dynstr_Set(&d, "");
	    dynstr_Set(&d2, "");
	    if (p = spoor_InstanceName(v)) {
		char *slash = index(p, ' ');

		if (slash) {
		    dynstr_AppendN(&d2, p, slash - p);
		} else {
		    dynstr_Append(&d2, p);
		}
	    } else {
	    }
            if (level > 0) {
                if (p)
                    dynstr_Set(&d, zmVaStr(
                      catgets(catalog, CAT_LITE, 716, "IN %s (class %s)"), 
                      dynstr_Str(&d2), spWidgetInfo_name(spView_getWclass(v))));
                else
                    dynstr_Set(&d, zmVaStr(
                      catgets(catalog, CAT_LITE, 718, "IN unnamed instance of class %s"), 
                 spWidgetInfo_name(spView_getWclass(v))));
            } else {
                dynstr_Set(&d, zmVaStr(
                catgets(catalog, CAT_LITE, 717, "%s (class %s)"), 
                dynstr_Str(&d2), spWidgetInfo_name(spView_getWclass(v))));
            }

	    ZmPagerWrite(pager, zmVaStr("%*s%s\n",(level+2),"",dynstr_Str(&d)));
	    while ((v = spView_parent(v))
		   && !spoor_IsClassMember(spoor_Class(v),
					   spWclass_class))
		;
	    ++level;
	}
    } FINALLY {
	dynstr_Destroy(&d);
	dynstr_Destroy(&d2);
    } ENDTRY;
}

static void
showAllKeys()
{
    struct zmPager *pager = ZmPagerStart(PgHelp);

    LITE_BUSY {
	ZmPagerWrite(pager, catgets(catalog, CAT_LITE, 719, "CURRENT KEYBINDINGS IN CONTEXT:\n"));
	showcontext(pager);
	ZmPagerWrite(pager, "\n");
	spTextview_wrapmode(pagerDialog_textview(pager->pager)) =
	    spTextview_nowrap;
	showAllKeys_aux(pager);
    } LITE_ENDBUSY;
    ZmPagerStop(pager);
}

static void
keybindings(self, requestor, data, keys)
    struct zmlapp *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    showAllKeys();
}

static struct spoor *
find_context_instance(name, context)
    char *name;
    struct spoor *context;
{
    struct spoor *result = spoor_FindInstance(name);

    if (!result) {
	if (!context)
	    context = (struct spoor *) spIm_focusView(ZmlIm);
	while (context
	       && !spoor_IsClassMember(context,
				       (struct spClass *) dialog_class)) {
	    context = (struct spoor *) spView_parent(context);
	}
	if (context) {
	    static struct dynstr d;
	    static int initialized = 0;
	    char addr[32];

	    if (!initialized) {
		dynstr_Init(&d);
		initialized = 1;
	    }
	    dynstr_Set(&d, name);
	    dynstr_AppendChar(&d, ' ');
	    sprintf(addr, "0x%lx", (unsigned long) context);
	    dynstr_Append(&d, addr);
	    result = spoor_FindInstance(dynstr_Str(&d));
	}
    }
    if (!result) {
	/*
	 * The "context" parameter represents a preferred context, not
	 * a manditory restriction.  If the block above failed to find
	 * the instance in the preferred context, ignore the context
	 * entirely and take any match we can find, in any context.
	 */

	const char **candidate;
	const int namelen = strlen(name);

	sklist_Find(&UniqueMap, &name, 1);
	candidate = ((const char **) sklist_LastMiss(&UniqueMap));
	if (!candidate)
	    candidate = ((const char **) sklist_First(&UniqueMap));

	if (candidate) {
	    if (strncmp(*candidate, name, namelen)
		|| ((*candidate)[namelen] != ' '))
		candidate = (const char **) sklist_Next(&UniqueMap, candidate);
	    while (candidate && (!strncmp(*candidate, name, namelen))
		   && ((*candidate)[namelen] == ' ')) {
		if (result = spoor_FindInstance(*candidate))
		    break;
		candidate = (const char **) sklist_Next(&UniqueMap, candidate);
	    }
	}
    }

    return (result);
}

static void
do_sequence(self, requestor, data, keys)
    struct spView *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct dynstr d;
    char *sep = NULL;
    char *start = (char *) data, *end, objname[64], actionname[64];
    struct spoor *obj;
    int first = 1;

    dynstr_Init(&d);
    TRY {
	if (!isascii(*start) || !isalnum(*start)) {
	    sep = start;
	    ++start;
	}
	while (start) {
	    while (*start
		   && ((isascii(*start) && isspace(*start))
		       || (sep && (*start == *sep))))
		++start;
	    if (!*start)
		break;
	    for (end = start; *end && !(isascii(*end) && isspace(*end)); ++end)
		;
	    if (!*end)
		break;
	    bcopy(start, objname, end - start);
	    objname[end - start] = '\0';
	    for (start = end;
		 (*start
		  && ((isascii(*start) && isspace(*start))
		      || (sep && (*start == *sep))));
		 ++start)
		;
	    if (!*start)
		break;
	    for (end = start;
		 (*end
		  && !(isascii(*end) && isspace(*end))
		  && (!sep || (*end != *sep)));
		 ++end)
		;
	    bcopy(start, actionname, end - start);
	    actionname[end - start] = '\0';
	    dynstr_Set(&d, "");
	    if (!sep || (*end != *sep)) {
		for (start = end;
		     (*start
		      && !(isascii(*start) && !isspace(*start))
		      && (!sep || (*start != *sep)));
		     ++start)
		    ;
		if (start && *start && (!sep || (*start != *sep))) {
		    for (end = start;
			 (*end
			  && (!sep || (*end != *sep)));
			 ++end)
			;
		    dynstr_AppendN(&d, start, end - start);
		}
	    }
	    if (!strcmp(objname, "*")) {
		obj = (struct spoor *) self;
	    } else if (!(obj = find_context_instance(objname, self))) {
		continue;
	    }
	    if (first)
		first = 0;
	    else
		++spInteractionNumber;
	    spSend(obj, m_spView_invokeInteraction, actionname, requestor,
		   (dynstr_EmptyP(&d) ? (char *) 0 : dynstr_Str(&d)), keys);
	    if (sep && (*end == *sep))
		start = end + 1;
	    else
		start = NULL;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
widget_info(self, requestor, data, keys)
    struct spView *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    struct dynstr d;
    char *p, *slash;

    dynstr_Init(&d);
    TRY {
	dynstr_Set(&d, catgets(catalog, CAT_LITE, 720, "Field: "));
	if (p = spoor_InstanceName(self)) {
	    if (slash = index(p, ' ')) {
		dynstr_AppendN(&d, p, slash - p);
	    } else {
		dynstr_Append(&d, p);
	    }
	    dynstr_AppendChar(&d, ' ');
	} else {
	    dynstr_Append(&d, catgets(catalog, CAT_LITE, 721, "[no name] "));
	}
	dynstr_AppendChar(&d, '(');
	dynstr_Append(&d, spWidgetInfo_name(spView_getWclass(self)));
	dynstr_AppendChar(&d, ')');
	if (!spoor_IsClassMember(self, (struct spClass *) dialog_class)) {
	    struct spView *parent = spView_parent(self);

	    while (parent
		   && !spoor_IsClassMember(parent,
					   (struct spClass *) dialog_class)) {
		parent = spView_parent(parent);
	    }
	    if (parent) {
		dynstr_Append(&d, catgets(catalog, CAT_LITE, 722, "    Parent: "));
		if (p = spoor_InstanceName(parent)) {
		    if (slash = index(p, ' ')) {
			dynstr_AppendN(&d, p, slash - p);
		    } else {
			dynstr_Append(&d, p);
		    }
		    dynstr_AppendChar(&d, ' ');
		} else {
		    dynstr_Append(&d, catgets(catalog, CAT_LITE, 721, "[no name] "));
		}
		dynstr_AppendChar(&d, '(');
		dynstr_Append(&d, spWidgetInfo_name(spView_getWclass(parent)));
		dynstr_AppendChar(&d, ')');
	    }
	}
	spSend(ZmlIm, m_spIm_showmsg, dynstr_Str(&d), 25, 5, 0);
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

#ifdef HAVE_RANDOM
# include <math.h>
# define Random() (random())
#else /* HAVE_RANDOM */
# include <stdlib.h>
# define Random() (rand())
#endif /* HAVE_RANDOM */

static int
gui_check_mail_wrapper(self, im)
    struct spEvent *self;
    struct spIm *im;
{
    long interval = passive_timeout + (Random() % 11) - 5;

    if (!ZScriptPending)
	gui_check_mail();
    spSend(self, m_spEvent_setup, (interval > 0) ? interval : 1,
	   (long) 0, 1, gui_check_mail_wrapper, (struct spoor *) 0);
    spSend(im, m_spIm_enqueueEvent, self);
    return (0);
}

static int
dodobindkey(seq, unb, obj, fn, arg, label, doc)
    char *seq;
    int unb;
    struct spoor *obj;
    char *fn, *arg, *label, *doc;
{
    int result = 0;

    if (!spoor_IsClassMember(obj, (struct spClass *) spView_class)) {
	return (1);
    }
    TRY {
	if (unb) {
	    spView_unbindInstanceKey((struct spView *) obj,
				     spKeysequence_Parse(0, seq, 1));
	} else {
	    spView_bindInstanceKey((struct spView *) obj,
				   spKeysequence_Parse(0, seq, 1),
				   fn, NULL, arg, label, doc);
	}
	spSend(ZmlIm, m_spIm_refocus);
    } EXCEPT(ANY) {
	result = 1;
    } ENDTRY;
    return (result);
}

static int tryingbkqueue = 0;
static void trybkqueue NP((const char *));

/* Return value is 0 (ok), 1 (error), 2 (could not honor request yet)
 */
/* This function must call trybkqueue before it calls dodobindkey
 * so that enqueued bindkey commands for a given widget or class
 * happen before the current one.  This does not apply, of course,
 * if this function is being called from trybkqueue!
 */
static int
dobindkey(seq, unb, classp, name, fn, arg, label, doc)
    char *seq;
    int unb, classp;
    char *name, *fn, *arg, *label, *doc;
{
    int result = 0;
    struct spoor *obj = (struct spoor *) 0;

    if (ZmlIm && classp) {
	struct spWidgetInfo *winfo = spWidget_Lookup(name);

	if (!winfo)
	    return (2);
	TRY {
	    trybkqueue(name);
	    if (unb) {
		spWidget_unbindKey(winfo, spKeysequence_Parse(0, seq, 1));
	    } else {
		spWidget_bindKey(winfo, spKeysequence_Parse(0, seq, 1),
				 fn, 0, arg, label, doc);
	    }
	    spSend(ZmlIm, m_spIm_refocus);
	    result = 0;
	} EXCEPT(strerror(EINVAL)) {
	    result = 2;
	} ENDTRY;
	return (result);
    } else if (ZmlIm && (obj = spoor_FindInstance(name))) {
	trybkqueue(name);
	return (dodobindkey(seq, unb, obj, fn, arg, label, doc));
    } else {
	char *p, **pp;
	int namelen = strlen(name);

	sklist_Find(&UniqueMap, &name, 1);
	pp = ((char **) sklist_LastMiss(&UniqueMap));
	if (!pp)
	    pp = ((char **) sklist_First(&UniqueMap));
	result = 2;
	if (pp) {
	    p = *pp;
	    if (strncmp(p, name, namelen) || (p[namelen] != ' '))
		if (pp = ((char **) sklist_Next(&UniqueMap, pp)))
		    p = *pp;
	    while (pp && (!strncmp(p, name, namelen))
		   && (p[namelen] == ' ')) {
		if (obj = spoor_FindInstance(p)) {
		    trybkqueue(p);
		    if (dodobindkey(seq, unb, obj, fn, arg, label, doc) == 1)
			return (1);
		}
		result = 0;
		if (pp = ((char **) sklist_Next(&UniqueMap, pp)))
		    p = *pp;
	    }
	}
	return (result);
    }
}

static void
trybkqueue(name)
    const char *name;
{
    struct bklist *bkl;
    struct bkqlistelt *bkqle;
    struct hashtab_iterator hti;
    int i, j, db;

    if (tryingbkqueue)		/* don't allow unwanted recursion */
	return;
    if (name) {
	struct bklist probe;
	/* XXX casting away const */
	probe.name = (char *) name;
	if (!(bkl = (struct bklist *) hashtab_Find(&BKqueue, &probe)))
	    return;
    } else {
	hashtab_InitIterator(&hti);
	if (!(bkl = hashtab_Iterate(&BKqueue, &hti)))
	    return;
    }
    tryingbkqueue = 1;
    TRY {
	do {
	    dlist_FOREACH2(&(bkl->elts), struct bkqlistelt, bkqle, i, j) {
		TRY {
		    db = dobindkey(bkqle->seq, bkqle->unb, bkl->classp,
				   bkl->name, bkqle->fn, bkqle->arg,
				   bkqle->label, bkqle->doc);
		} EXCEPT(strerror(EINVAL)) {
		    error(UserErrWarning,
			  zmVaStr("%s (%s)\n", strerror(EINVAL),
				  except_GetExceptionValue()));
		    db = 0;
		} ENDTRY;
		/* We no longer remove named-widget bindkeys from the queue,
		 * since new instances with the same name can be created.
		 */
		if (bkl->classp && !db) {
		    if (bkqle->seq)
			free(bkqle->seq);
		    if (bkqle->fn)
			free(bkqle->fn);
		    if (bkqle->arg)
			free(bkqle->arg);
		    if (bkqle->label)
			free(bkqle->label);
		    if (bkqle->doc)
			free(bkqle->doc);
		    dlist_Remove(&(bkl->elts), i);
		}
	    }
	    if (!name)
		bkl = (struct bklist *) hashtab_Iterate(&BKqueue, &hti);
	    else
		bkl = 0;
	} while (bkl);
    } FINALLY {
	tryingbkqueue = 0;
    } ENDTRY;
}

void
ZmlSetInstanceName(obj, name, context)
    struct spoor *obj;
    const char *name;
    struct spView *context;
{
    if (!context) {
	spSend(obj, m_spoor_setInstanceName, name);
    } else {
	struct dynstr xname;
	char addr[20], *p;

	dynstr_Init(&xname);
	dynstr_Set(&xname, name);
	dynstr_AppendChar(&xname, ' ');
	sprintf(addr, "0x%lx", (unsigned long) context);
	dynstr_Append(&xname, addr);
	spSend(obj, m_spoor_setInstanceName, dynstr_Str(&xname));
	p = dynstr_GiveUpStr(&xname);
	sklist_Insert(&UniqueMap, &p);
    }
    if (bkqinitialized)
	trybkqueue(name);
}

static int
uniquemapcmp(a, b)
    char **a, **b;
{
    return (strcmp(*a, *b));
}

static int
enableScrollpct()
{
    return (boolean_val(VarScrollpct));
}

static void
initialize_zmlim()
{
    static int initialized = 0;
    int i;
    char buf[16];

    if (initialized)
	return;
    initialized = 1;

    for (i = 0; spCursesIm_defaultKeys[i].name; ++i) {
	(void) spKeysequence_Parse(0, spCursesIm_defaultKeys[i].name, 1);
    }
    for (i = 1; i <= spCursesIm_MAXAUTOFKEY; ++i) {
	sprintf(buf, "\\<f%d>", i);
	(void) spKeysequence_Parse(0, buf, 1); /* make sure the fkeys get
						* created in order */
    }

    zmlapp_InitializeClass();
    spCursesIm_enableLabel = boolean_val(VarFkeyLabels);
    ZmlIm = zmlapp_NEW();
    ZmlSetInstanceName(ZmlIm, "zmlite", 0);
}

static void
no_reverse_cb(unused1, unused2)
    char *unused1;
    ZmCallback unused2;
{
    extern int spCursesWin_NoStandout; /* if I include curswin.h,
					* curses.h goes nuts */

    spCursesWin_NoStandout = boolean_val(VarNoReverse);
}

static void
hist_backward(c, requestor, data, keys)
    struct spCmdline *c;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (LastHistory < 0) {
	LastHistory = dlist_Tail(&CommandHistory);
    } else if (LastHistory == dlist_Head(&CommandHistory)) {
	return;
    } else {
	LastHistory = dlist_Prev(&CommandHistory, LastHistory);
    }
    if (LastHistory >= 0) {
	char *str = *((char **) dlist_Nth(&CommandHistory, LastHistory));

	spSend(spView_observed(c), m_spText_clear);
	spSend(spView_observed(c), m_spText_insert,
	       0, -1, str, spText_mBefore);
    }
}

static void
hist_forward(c, requestor, data, keys)
    struct spCmdline *c;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    if (LastHistory < 0) {
	return;
    } else if (LastHistory == dlist_Tail(&CommandHistory)) {
	return;
    } else {
	LastHistory = dlist_Next(&CommandHistory, LastHistory);
    }
    if (LastHistory >= 0) {
	char *str = *((char **) dlist_Nth(&CommandHistory, LastHistory));

	spSend(spView_observed(c), m_spText_clear);
	spSend(spView_observed(c), m_spText_insert,
	       0, -1, str, spText_mBefore);
    }
}

static void
save_command_history(str)
    char *str;
{
    char *save = savestr(str);

    dlist_Append(&CommandHistory, &save);
    while (dlist_Length(&CommandHistory) > hist_size) {
	xfree(*((char **) dlist_Nth(&CommandHistory,
				    dlist_Head(&CommandHistory))));
	dlist_Remove(&CommandHistory, dlist_Head(&CommandHistory));
    }
    LastHistory = -1;
}

static int
deferred_zscript(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    ZCommand(spEvent_data(ev), zcmd_commandline);
    xfree(spEvent_data(ev));
    return (1);
}

static void
command_accept(self, requestor, data, keys)
    struct spCmdline *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    char *keys;
{
    struct dynstr d;

    spSend(self, m_spView_invokeInteraction, "inputfield-accept",
	   requestor, data, keys);
    dynstr_Init(&d);
    TRY {
	spSend(spView_observed(self), m_spText_appendToDynstr,
	       &d, 0, -1);
	if (dynstr_EmptyP(&d)) {
	    char *tmp = get_var_value(VarNewline);

	    if (tmp && *tmp)
		dynstr_Set(&d, tmp);
	} else {
	    save_command_history(dynstr_Str(&d));
	    wprint("%s\n", dynstr_Str(&d));
	}
	if (!dynstr_EmptyP(&d)) {
	    spSend(spView_observed(self), m_spText_clear);
	    spSend(ZmlIm, m_spIm_enqueueEvent,
		   spEvent_Create((long) 0, (long) 0, 1,
				  deferred_zscript,
				  savestr(dynstr_Str(&d))));
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

struct spWidgetInfo *spwc_FileList = 0;
struct spWidgetInfo *spwc_FolderStatusList = 0;
struct spWidgetInfo *spwc_MessageSummaries = 0;
struct spWidgetInfo *spwc_PullrightMenu = 0;
struct spWidgetInfo *spwc_Radiogroup = 0;
struct spWidgetInfo *spwc_Togglegroup = 0;
struct spWidgetInfo *spwc_Commandfield = 0;

static int
lvsn(argc, argv)
    int argc;
    char **argv;
{
    static struct dynstr d;
    static int initialized = 0;

    if (!initialized) {
	extern int v_vname[], v_str[];
	extern char *rvl();

	dynstr_Init(&d);
	dynstr_Set(&d, get_var_value(VSN));
	if (boolean_val(rvl(v_vname))) {
	    dynstr_AppendChar(&d, ' ');
	    dynstr_Append(&d, rvl(v_str));
	}
	initialized = 1;
    }
    if ((istool == 2)
	&& !chk_option(VarMainPanes, "output")
	&& !chk_option(VarQuiet, "copyright"))
	error(Message, "%s\n%s", dynstr_Str(&d), zmCopyright());
    else
	print("%s\n%s\n", dynstr_Str(&d), zmCopyright());
    return (0 - in_pipe());
}

static int zm_textedit P((int, char **));
static int zm_interact P((int, char **));

static void
add_lite_commands()
{
    static zmCommand cmds[] = {
	{ "textedit", zm_textedit, CMD_DEFINITION },
	{ VSN, lvsn, CMD_DEFINITION },
	{ "interact", zm_interact, CMD_INPUT_NO_MSGS|CMD_OUTPUT_NO_MSGS|CMD_LITE_VUI_ONLY },
	{ 0, 0, 0 }
    };
    int i;

    for (i = 0; cmds[i].command; ++i) {
	zscript_add(cmds + i);
    }
}

void
vui_initialize()
{
    struct spEvent *ev;
    extern int spCursesWin_NoStandout; /* if I include curswin.h,
					* curses.h goes nuts */

    Srandom(getpid() + time(0));

    spCursesWin_NoStandout = boolean_val(VarNoReverse);
    ZmCallbackAdd(VarNoReverse, ZCBTYPE_VAR, no_reverse_cb, 0);
    sklist_Init(&UniqueMap, (sizeof (char *)), uniquemapcmp,
		1, 4);

    spoor_Initialize();

    addrbook_InitializeClass();
    attachlist_InitializeClass();
    attachtype_InitializeClass();
    charsets_InitializeClass();
    chooseone_InitializeClass();
    compopt_InitializeClass();
    dsearch_InitializeClass();
    dialog_InitializeClass();
    dynhdrs_InitializeClass();
    helpDialog_InitializeClass();
    helpIndex_InitializeClass();
    multikey_InitializeClass();
    notifier_InitializeClass();
    pagerDialog_InitializeClass();
    printdialog_InitializeClass();
    psearch_InitializeClass();
    smallaliases_InitializeClass();
    sortdialog_InitializeClass();
    spEvent_InitializeClass();
    taskmeter_InitializeClass();
    templatedialog_InitializeClass();
    tsearch_InitializeClass();
    zmlaliasframe_InitializeClass();
    zmlenvframe_InitializeClass();
    zmlhdrsframe_InitializeClass();
    zmlmainframe_InitializeClass();
    zmlmsgframe_InitializeClass();
    zmlvarframe_InitializeClass();

    initialize_zmlim();

    add_lite_commands();

    spListv_InitializeClass();
    spwc_FileList = spWidget_Create("FileList", spwc_List);
    spwc_FolderStatusList = spWidget_Create("FolderStatusList", spwc_List);
    spwc_MessageSummaries = spWidget_Create("MessageSummaries", spwc_List);

    spMenu_InitializeClass();
    spwc_PullrightMenu = spWidget_Create("PullrightMenu", spwc_Menu);

    spButtonv_InitializeClass();
    spwc_Radiogroup = spWidget_Create("Radiogroup", spwc_Buttonpanel);
    spwc_Togglegroup = spWidget_Create("Togglegroup", spwc_Buttonpanel);

    spCmdline_InitializeClass();
    spwc_Commandfield = spWidget_Create("Commandfield", spwc_Inputfield);

    spWidget_AddInteraction(spwc_Widget, "do-sequence", do_sequence,
			    catgets(catalog, CAT_LITE, 724, "Invoke interaction(s) in named field(s)"));
    spWidget_AddInteraction(spwc_Widget, "widget-info", widget_info,
			    catgets(catalog, CAT_LITE, 788, "Give info on current field"));

    spWidget_bindKey(spwc_Widget, spKeysequence_Parse(0, "^Xw", 1),
		     "widget-info", 0, 0, 0, 0);

    dlist_Init(&CommandHistory, (sizeof (char *)), 16);

    spWidget_AddInteraction(spwc_Commandfield, "command-history-backward",
			    hist_backward,
			    catgets(catalog, CAT_LITE, 789, "Move backward through command history"));
    spWidget_AddInteraction(spwc_Commandfield, "command-history-forward",
			    hist_forward,
			    catgets(catalog, CAT_LITE, 790, "Move forward through command history"));
    spWidget_AddInteraction(spwc_Commandfield, "command-accept",
			    command_accept,
			    catgets(catalog, CAT_LITE, 791, "Perform given Z-Script command"));
    spWidget_bindKey(spwc_Commandfield, spKeysequence_Parse(0, "^N", 0),
		     "command-history-forward", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Commandfield, spKeysequence_Parse(0, "^P", 0),
		     "command-history-backward", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Commandfield, spKeysequence_Parse(0, "\n", 0),
		     "command-accept", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Commandfield, spKeysequence_Parse(0, "\r", 0),
		     "command-accept", 0, 0, 0, 0);

    spWidget_AddInteraction(spwc_Zmliteapp, "show-keys", keybindings,
			    catgets(catalog, CAT_LITE, 792, "Show active keybindings"));
    spView_bindInstanceKey((struct spView *) ZmlIm,
			   spKeysequence_Parse(0, "\\ek", 1), "show-keys",
			   0, 0, 0, 0);

    spIm_LOCKSCREEN {
	spSend(Dialog(&MainDialog), m_dialog_enter);
    } spIm_ENDLOCKSCREEN;

    ev = spEvent_NEW();
    spSend(ev, m_spEvent_setup, (long) passive_timeout, (long) 0, 1,
	   gui_check_mail_wrapper, (struct spoor *) 0);
    spSend(ZmlIm, m_spIm_enqueueEvent, ev);

    if (bkqinitialized)
	trybkqueue(0);

    spTextview_enableScrollpct = enableScrollpct;
}

void
gui_clear_hdrs(f)
    msg_folder *f;
{
    spSend(MainDialog, m_zmlmainframe_clearHdrs);
}

void
gui_check_mail()
{
    if ((istool != 2) || (zmlErrorLevel > 0))
	return;
    if (isoff(glob_flags, IGN_SIGS))
	shell_refresh();
}

static int
doVoidEvent(self, im)
    struct spEvent *self;
    struct spIm *im;
{
    char *p = (char *) spEvent_data(self); /* does this shut the
					    * compiler up? */

    (*((void (*) ()) p))();
    return (1);
}

static int
version_fn(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    spSend(ZmlIm, m_spIm_processEvents);
    ZCommand("version", zcmd_ignore);
    return (1);
}

void
gui_main_loop()
{
    int looping = 1;

    spSend(ZmlIm, m_spIm_enqueueEvent,
	   spEvent_Create((long) 0, (long) 0, 1,
			  version_fn, 0));
    while (looping) {
	TRY {
	    spSend(ZmlIm, m_spIm_refocus);
	    spSend(ZmlIm, m_spIm_interact);
	} EXCEPT("unimplemented") {
	    char buf[128], *p;

	    if (p = (char *) except_GetExceptionValue())
		sprintf(buf, catgets(catalog, CAT_LITE, 793, "Unimplemented (%s)"), p);
	    else
		sprintf(buf, catgets(catalog, CAT_LITE, 725, "Unimplemented"));
	    spSend(ZmlIm, m_spIm_showmsg, buf, 25, 5, 0);
	} EXCEPT("zmail exit") {
	    looping = 0;
	} EXCEPT(ANY) {
	    char buf[128], *p;

	    if (p = (char *) except_GetExceptionValue())
		sprintf(buf, catgets(catalog, CAT_LITE, 726, "Uncaught exception: %s (%s)"),
			except_GetRaisedException(), p);
	    else
		sprintf(buf, catgets(catalog, CAT_LITE, 794, "Uncaught exception: %s"),
			except_GetRaisedException());
	    spSend(ZmlIm, m_spIm_showmsg, buf, 25, 5, 0);
	} ENDTRY;
    }
}

void
set_alarm(timout, func)
    time_t timout;
    void (*func)();
{
    if (func) {
	struct spEvent *ev = spEvent_NEW();

	spSend(ev, m_spEvent_setup, (long) timout, (long) 0, 1,
	       doVoidEvent, func);
	spSend(ZmlIm, m_spIm_enqueueEvent, ev);
    }
}

void
gui_redraw_hdrs(f, list)
    msg_folder *f;
    msg_group *list;
{
    spSend(MainDialog, m_zmlmainframe_redrawHdrs, 0, 0);
}

static int death_raiser = -1;

static int
real_child_death_raiser(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    spSend(ZmlIm, m_spIm_interactReturn, 1);
    return (1);
}

static void
child_death_raiser(pid, status, data)
    int pid;
    WAITSTATUS *status;
    void *data;
{
    if (pid == death_raiser)
	spSend(ZmlIm, m_spIm_enqueueEvent,
	       spEvent_Create((long) 0, (long) 0, 1,
			      real_child_death_raiser, 0));
}

static void
incr_taskmeter_count()
{
    if (++taskmeter_up > 0)
	spCursesIm_enableBusy = 0;
}

static void
decr_taskmeter_count()
{
    if ((--taskmeter_up) == 0)
	spCursesIm_enableBusy = 1;
}

static int
popup_taskmeter(self, im)
    struct spEvent *self;
    struct spIm *im;
{
    incr_taskmeter_count();
    spSend(ZmlIm, m_spIm_popupView, spEvent_data(self), 0, -1, -1);
    return (0);
}

void
gui_wait_for_child(pid, message, msecs)
    int pid;
    const char *message;
    long msecs;
{
    struct taskmeter *tm = taskmeter_NEW();
    struct spEvent *ev = spEvent_NEW();
    struct spPopupView *popup = spPopupView_Create((struct spView *) tm,
						   spPopupView_plain);
    int retval;

    spSend(tm, m_taskmeter_setInterruptable,
	   taskmeter_INTERRUPTABLE | taskmeter_EXCEPTION);
    spSend(tm, m_taskmeter_setMainMsg,
	   message ? message : catgets(catalog, CAT_LITE, 795, "Waiting for child process"));
    if (msecs < 0)
	msecs = 0;
    spSend(ev, m_spEvent_setup, msecs / 1000, msecs % 1000, 1,
	   popup_taskmeter, popup);
    spSend(ZmlIm, m_spIm_enqueueEvent, ev);
    TRY {
	death_raiser = pid;
#ifdef ZM_CHILD_MANAGER
	zmChildSetCallback(pid, ZM_CHILD_EXITED, child_death_raiser, 0);
#endif /* CHILD_MANAGER */
	++WaitingForChildInteractively;
	retval = spSend_i(ZmlIm, m_spIm_interact);
	death_raiser = -1;
    } FINALLY {
	--WaitingForChildInteractively;
	if (ev->inqueue) {
	    spSend(ev, m_spEvent_cancel, 1);
	} else {
	    spoor_DestroyInstance(ev);
	}
	if (spView_window(popup)) {
	    decr_taskmeter_count();
	    spSend(ZmlIm, m_spIm_dismissPopup, popup);
	}
	spoor_DestroyInstance(popup);
	spoor_DestroyInstance(tm);
    } ENDTRY;
}

void
gui_bell()
{
    spSend(ZmlIm, m_spIm_bell);
}

int
gui_set_hdrs(parent, compose)
    struct dialog *parent;
    Compose *compose;
{
    struct dynstr d;
    int result = -1;

    dynstr_Init(&d);

    TRY {
	if (!dyn_choose_one(&d,
			    (ison(compose->flags, FORWARD) ?
			     catgets(catalog, CAT_LITE, 796, "Forward To: ") : catgets(catalog, CAT_LITE, 797, "Send To: ")), NULL,
			    DUBL_NULL, 0, (u_long) 0)) {
	    ZSTRDUP(compose->addresses[TO_ADDR], dynstr_Str(&d));
	    result = 0;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (result);
}

struct watch_it {		/* Duplicated (mostly) from gui/gui_api.c */
    int fd, pid;
    u_long flags;
    char *title;
    ZmPager pager;
};

static void
watch_fd(im, fd, w, dp)
    struct spIm *im;
    int fd;
    struct watch_it *w;
    struct dpipe *dp;
{
    char buf[256];
    int nbytes;

    TRY {
	if (nbytes = dpipe_Read(dp, buf, (sizeof (buf)) - 1)) {
	    int printed = 0;

	    buf[nbytes] = 0;
	    if ((w->flags & DIALOG_NEEDED)
		|| ((w->flags & DIALOG_IF_HIDDEN)
		    && (!spoor_IsClassMember(spIm_view(ZmlIm),
					     ((struct spClass *)
					      zmlmainframe_class))
			|| !chk_option(VarMainPanes, "output")))) {
		if (!(w->pager)) {
		    w->pager = ZmPagerStart(PgText);
		    ZmPagerWrite(w->pager, "");
		    if (w->title)
			ZmPagerSetTitle(w->pager, w->title);
		}
		ZmPagerWrite(w->pager, buf);
		printed = 1;
	    }

	    if (!printed
		|| (w->flags & WPRINT_ALWAYS)) {
		wprint("%s", buf);
	    }
	} else {
	    ekill(w->pid, 0, "watch_fd");
	}
    } EXCEPT(ANY) {
	close(fd);
	spSend(im, m_spIm_unwatchInputFD, fd);
    } ENDTRY;
}

static void
free_watch_it(w)
    struct watch_it *w;
{
    if (w->pager) {
	ZmPagerStop(w->pager);
	w->pager = 0;
    }
    if (w->title) free(w->title);
    free(w);
}

void
gui_watch_filed(fd, pid, flags, title)
    int fd, pid;
    u_long flags;
    char *title;
{
    struct watch_it *w = zmNew(struct watch_it);

#ifdef FNDELAY			/* copied from gui/gui_api.c */
    (void) fcntl(fd, F_SETFL, FNDELAY);
#else
# ifdef O_NDELAY
    (void) fcntl(fd, F_SETFL, O_NDELAY);
# endif /* O_NDELAY */
#endif /* FNDELAY */

    w->fd = fd;
    w->pid = pid;
    w->flags = flags;
    w->title = title ? savestr(title) : (char *) NULL;
    w->pager = 0;

    spSend(ZmlIm, m_spIm_watchInputFD, fd, watch_fd, w, free_watch_it);
}

void
gui_new_hdrs(fldr, old_cnt)
    msg_folder *fldr;
    int old_cnt;
{
    int n = boolean_val("nethack");

    spSend(Dialog(&MainDialog), m_zmlmainframe_newHdrs, fldr, old_cnt);
    if (!spView_window(MainDialog))
	spSend(ZmlIm, m_spIm_showmsg,
	       (n ? "The postman hits! -more- You have new mail" :
		catgets(catalog, CAT_LITE, 798, "You have new mail")),
	       (n ? 16 : 15), 2, 5);
}

int
gui_iconify()
{
    if (istool == 2) {
	spSend(ZmlIm, m_spCharIm_shellMode);
	kill(getpid(), SIGTSTP);
	spSend(ZmlIm, m_spCharIm_progMode);
	return 1;
    } else
	return 0;
}

int
gui_execute(pathname, argv, message, msecs, ask_kill)
    const char *pathname;		/* name of executable file */
    char **argv;
    const char *message;
    long msecs; /* -1 means never */
    int ask_kill;
{
    int status, numwaits;
    
    if (!istool)
	return (execute(argv));

    un_set(&set_options, "child");

    if (screencmd_pending) {
	spIm_LOCKSCREEN {
	    status = execute(argv);
	} spIm_ENDLOCKSCREEN;
    } else {
	static char *set_argv[] = { "child", "=", NULL, NULL };
	FILE *in = (FILE *) 0, *out = (FILE *) 0, *err = (FILE *) 0;
	int pid, errp = 0;
	char buf[16];

	if ((pid = popenvp(&in, &out, &err, pathname, argv)) >= 0) {
	    if (in)
		fclose(in);
	    if (out) {
		gui_watch_filed(dup(fileno(out)), pid,
				DIALOG_IF_HIDDEN, catgets(catalog, CAT_LITE, 799, "Command Output"));
		fclose(out);
	    }
	    if (err) {
		gui_watch_filed(dup(fileno(err)), pid,
				DIALOG_IF_HIDDEN, catgets(catalog, CAT_LITE, 800, "Command Error"));
		fclose(err);
	    }

	    gui_wait_for_child(pid, message, msecs);
	} else {
	    errp = 1;
	}

	if (errp) {
	    if (errno == ENOENT)
		print(catgets(catalog, CAT_LITE, 801, "%s: command not found.\n"), *argv);
	    else
		error(SysErrWarning, catgets(catalog, CAT_LITE, 802, "Couldn't execute %s"), *argv);
	    return -1;
	}

	for (numwaits = 0; ; numwaits++) {
	    /* Block execution of child callback while reorganizing... */
	    (void) zmChildBlock(pid);

	    switch(zmChildWaitPid(pid, (WAITSTATUS *)&status, WNOHANG)) {
	      case -1:
		error(SysErrWarning, "zmChildWaitPid");
		return -1;
	      case 0:
		/*
		 * Child is still running, so we are here
		 * because the stop button was hit or we
		 * were interrupted in some other way.
		 */

		if (ask_kill
		    && (ask(WarnCancel,
			    zmVaStr("%s%s\n%s",
				    (numwaits ?
				     catgets(catalog, CAT_LITE, 803, "Child ignored TERM.  Trying KILL.\n\n") :
				     ""),
				    catgets(catalog, CAT_LITE, 804, "OK to continue program in background"),
				    catgets(catalog, CAT_LITE, 805, "Cancel to terminate program immediately")))
			!= AskYes)) {
		    (void) kill(pid, numwaits? SIGKILL : SIGTERM);
		    sleep(1);
		    continue;
		}

		/*
		 * We will never pclose this child, so we need to make
		 * sure the child will get reaped when it dies.
		 * Do this by setting the "exited" callback associated
		 * with the child to zmChildWaitPid (i.e. make the reaping
		 * occur automatically).
		 */
		(void) zmChildSetCallback(pid, ZM_CHILD_EXITED,
				(void (*)()) zmChildWaitPid, (void *) NULL);
		(void) zmChildUnblock(pid);

		sprintf(buf, "%d", pid);
		set_argv[2] = buf;
		add_option(&set_options, set_argv);

		return -2;	/* magic number means stop button was hit */
	    default:
		/*
		 * The child is dead.  Return its exit status.
		 */

		return (status >> 8);
	    }
	}
    }

    turnoff(glob_flags, WAS_INTR);
    return (status);
}

int
gui_execute_using_sh(argv, message, msecs, ask_kill)
    char **argv;
    const char *message;
    long msecs;			/* -1 means never */
    int ask_kill;
{
    char buf[BUFSIZ];
    static char *new_argv[] = {"sh", "-c", NULL, NULL};
    if (message)
	strcpy(buf, "exec ");
    else
	buf[0] = 0;
    argv_to_string(buf+strlen(buf), argv);
    new_argv[2] = buf;
    return (gui_execute("/bin/sh", new_argv, message, msecs, ask_kill));
}

int
zm_unmultikey(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    char *cmdname = argv ? *(argv++) : (char *) NULL;
    int e, ismap = 0;
    struct spKeysequence *ksp;

    while (argv && *argv && (**argv == '-')) {
	switch (argv[0][1]) {
	  case 'm':
	    ismap = 1;
	    ++argv;
	    break;
	  default:
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 806, "Usage: %s [-m] sequence"), cmdname);
	    return (-1);
	}
    }
    if (!*argv) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 807, "Usage: %s [-m] sequence"), cmdname);
	return (-1);
    }
    if (istool != 2) {
	spoor_Initialize();	  /* Doesn't matter if multiply called */
	spView_InitializeClass(); /* Ditto */
    }
    e = 0;
    TRY {
	ksp = spKeysequence_Parse(0, argv[0], 0);
    } EXCEPT(strerror(EINVAL)) {
	e = 1;
    } ENDTRY;
    if (e) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 311, "Bad key sequence specification: %s"),
	      argv[0]);
	return (-1);
    }
    initialize_zmlim();
    if (ismap)
	spSend(ZmlIm, m_spIm_removeMapping, ksp);
    else
	spSend(ZmlIm, m_spIm_removeTranslation, ksp);
}

static void
save_multikeys_aux(fp, level, prefix, keymap)
    FILE *fp;
    int level;
    struct spKeysequence *prefix;
    struct spKeymap *keymap;
{
    struct spKeymapEntry *kme;
    int i;

    for (kme = spKeymap_First(keymap); kme; kme = spKeymap_Next(keymap, kme)) {
	switch (kme->type) {
	  case spKeymap_translation:
	    fputs("multikey  ", fp);
	    for (i = 0; i < level; ++i) {
		fputs(spKeyname(spKeysequence_Nth(prefix, i), 0), fp);
	    }
	    fputs(spKeyname(kme->c, 0), fp);
	    fputs("\t", fp);
	    for (i = 0;
		 i < spKeysequence_Length(&(kme->content.translation));
		 ++i) {
		fputs(spKeyname(spKeysequence_Nth(&(kme->content.translation),
						  i), 0), fp);
	    }
	    fputc('\n', fp);
	    break;
	  case spKeymap_keymap:
	    spKeysequence_Truncate(prefix, level);
	    spKeysequence_Add(prefix, kme->c);
	    save_multikeys_aux(fp, level + 1, prefix, kme->content.keymap);
	    break;
	}
    }
}

static int
save_multikeys(argv)
    char **argv;		/* argv[0] starts with "-s" */
{
    char *filename, buf[MAXPATHLEN + 1];
    char *term, *e;
    FILE *fp = 0;
    int ftype, retval;
    struct dynstr d;
    struct spKeysequence ks;

    filename = argv[0] + 2;
    if (!*filename)
	filename = argv[1];
    if (!filename || !*filename) {
	if (!(term = getenv("LITETERM"))) {
	    if (!(term = getenv("TERM"))) {
		error(UserErrWarning,
		      catgets(catalog, CAT_LITE, 809, "Cannot choose a default; neither LITETERM nor TERM is set"));
		return (-1);
	    }
	}
	sprintf(buf, "%s/.multikey/%s", get_var_value(VarHome), term);
	filename = buf;
    }
    if (filename != buf) {
	strcpy(buf, filename);
	filename = buf;
    }
    getstat(buf, buf, 0);	/* expand filename in place */
    ftype = 0;
    e = getpath(filename, &ftype); /* test existence/type */
    switch (ftype) {
      case 0:			/* existing file */
	if (ask(WarnOk, zmVaStr(catgets(catalog, CAT_LITE, 810, "Overwrite %s?"), filename)) != AskYes) {
	    return (-1);
	}
	break;
      case 1:			/* existing directory */
	error(UserErrWarning, catgets(catalog, CAT_LITE, 811, "%s is a directory"), filename);
	return (-1);
	/* Ignore the other cases */
    }

    dynstr_Init(&d);
    TRY {
	char *q = rindex(filename, '/');

	if (q && (q != filename)) {
	    dynstr_AppendN(&d, filename, q - filename);
	    e = getdir(dynstr_Str(&d), 1);
	} else {
	    e = filename;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;

    if (!e) {
	error(SysErrWarning, catgets(catalog, CAT_LITE, 812, "Could not create %s"), filename);
	return (-1);
    }
    retval = 0;
    spKeysequence_Init(&ks);
    TRY {
	fp = efopen(filename, "w", "multikey -s");
	fputs(catgets(catalog, CAT_LITE, 813, "# Terminal-specific \"multikey\" definitions\n"), fp);
	fputs(catgets(catalog, CAT_LITE, 727, "# This file generated automatically by Z-Mail Lite\n\n"), fp);
	save_multikeys_aux(fp, 0, &ks, spIm_translations(ZmlIm));
    } EXCEPT(ANY) {
	error(SysErrWarning, catgets(catalog, CAT_LITE, 728, "Could not save to %s"), filename);
	retval = -1;
    } FINALLY {
	spKeysequence_Destroy(&ks);
	if (fp)
	    fclose(fp);
    } ENDTRY;
    return (retval);
}

int
zm_multikey(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    char *cmdname = argv ? *(argv++) : (char *) NULL;
    int e, ismap = 0, ftype, quiet = 0;
    struct spKeysequence ks, *ksp;
    char buf[1 + MAXPATHLEN], *newargv[3], *term;
    int doload = 0;

    while (argv && *argv && (**argv == '-')) {
	switch (argv[0][1]) {
	  case 'q':
	    quiet = 1;
	    ++argv;
	    break;
	  case 's':		/* save */
	    return (save_multikeys(argv));
	  case 'l':		/* load */
	    doload = 1;
	    ++argv;
	    break;
	  case 'm':		/* map */
	    ismap = 1;
	    ++argv;
	    break;
	  default:
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 729, "Usage: %s -l [-q]\n-or- %s -s [file]\n-or- %s [-m] seq1 seq2"),
		  cmdname, cmdname, cmdname);
	    return (-1);
	}
    }
    if (doload) {
	int done = 0;

	if (term = getenv("LITETERM")) {
	    sprintf(buf, "%s/multikey/%s", zmlibdir, term);
	    ftype = 0;
	    getpath(buf, &ftype);
	    if (ftype == 0) {
		newargv[0] = "source";
		newargv[1] = buf;
		newargv[2] = 0;
		if (source(2, newargv, list))
		    return (-1);
		done = 1;
		if (!quiet)
		    wprint(catgets(catalog, CAT_LITE, 730, "Loaded key definitions from %s\n"), buf);
	    } else if (!quiet) {
		wprint(catgets(catalog, CAT_LITE, 731, "Not loading key definitions from %s\n"), buf);
	    }
	}
	if (!done && (term = getenv("TERM"))) {
	    sprintf(buf, "%s/multikey/%s", zmlibdir, term);
	    ftype = 0;
	    getpath(buf, &ftype);
	    if (ftype == 0) {
		newargv[0] = "source";
		newargv[1] = buf;
		newargv[2] = 0;
		if (source(2, newargv, list))
		    return (-1);
		if (!quiet)
		    wprint(catgets(catalog, CAT_LITE, 730, "Loaded key definitions from %s\n"), buf);
	    } else if (!quiet) {
		wprint(catgets(catalog, CAT_LITE, 731, "Not loading key definitions from %s\n"), buf);
	    }
	}
	done = 0;
	if (term = getenv("LITETERM")) {
	    sprintf(buf, "%s/.multikey/%s", get_var_value(VarHome), term);
	    ftype = 0;
	    getpath(buf, &ftype);
	    if (ftype == 0) {
		newargv[0] = "source";
		newargv[1] = buf;
		newargv[2] = 0;
		if (source(2, newargv, list))
		    return (-1);
		done = 1;
		if (!quiet)
		    wprint(catgets(catalog, CAT_LITE, 730, "Loaded key definitions from %s\n"), buf);
	    } else if (!quiet) {
		wprint(catgets(catalog, CAT_LITE, 731, "Not loading key definitions from %s\n"), buf);
	    }
	}
	if (!done && (term = getenv("TERM"))) {
	    sprintf(buf, "%s/.multikey/%s", get_var_value(VarHome), term);
	    ftype = 0;
	    getpath(buf, &ftype);
	    if (ftype == 0) {
		newargv[0] = "source";
		newargv[1] = buf;
		newargv[2] = 0;
		if (source(2, newargv, list))
		    return (-1);
		done = 1;
		if (!quiet)
		    wprint(catgets(catalog, CAT_LITE, 730, "Loaded key definitions from %s\n"), buf);
	    } else if (!quiet) {
		wprint(catgets(catalog, CAT_LITE, 731, "Not loading key definitions from %s\n"), buf);
	    }
	}
	return (0);
    }

    if (!argv[0] || !argv[1] || !*argv[0] || !*argv[1]) {
	error(UserErrWarning,
	      catgets(catalog, CAT_LITE, 738, "Usage: %s -l\n-or- %s -s [file]\n-or- %s [-m] seq1 seq2"),
	      cmdname, cmdname, cmdname);
	return (-1);
    }

    if (istool != 2) {
	spoor_Initialize();	  /* Doesn't matter if multiply called */
	spView_InitializeClass(); /* Ditto */
    }

    e = 0;
    spKeysequence_Init(&ks);
    TRY {
	TRY {
	    spKeysequence_Parse(&ks, argv[0], 0);
	    e = 0;		/* because of volatility */
	} EXCEPT(strerror(EINVAL)) {
	    e = 1;
	} ENDTRY;
	if (e) {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 311, "Bad key sequence specification: %s"),
		  argv[0]);
	} else {
	    initialize_zmlim();
	    e = 0;
	    TRY {
		ksp = spKeysequence_Parse(0, argv[1], 1);
		e = 0;		/* because of volatility */
	    } EXCEPT(strerror(EINVAL)) {
		e = 1;
	    } ENDTRY;
	    if (e) {
		error(UserErrWarning, catgets(catalog, CAT_LITE, 311, "Bad key sequence specification: %s"),
		      argv[1]);
	    } else {
		if (ismap)
		    spSend(ZmlIm, m_spIm_addMapping, &ks, ksp);
		else
		    spSend(ZmlIm, m_spIm_addTranslation, &ks, ksp);
	    }
	}
    } FINALLY {
	spKeysequence_Destroy(&ks);
    } ENDTRY;
    return (e ? -1 : 0);
}

static unsigned int
bklhash(x)
    struct bklist *x;
{
    return (hashtab_StringHash(x->name));
}

static int
bklcmp(a, b)
    struct bklist *a, *b;
{
    return (strcmp(a->name, b->name));
}

/* The strings passed to this function have already been savestr'd */
static void
enqueue_bindkey(seq, unb, classp, name, fn, data, label, doc)
    char *seq;
    int unb, classp;
    char *name, *fn, *data, *label, *doc;
{
    struct bklist *bkl, probe;
    struct bkqlistelt bkqle;
    struct bkqlistelt *pbkqle;
    int j , sameness_count;
    int identical;
    static int speed_load = 0;

    if (!bkqinitialized) {
	hashtab_Init(&BKqueue, bklhash, bklcmp,
		     (sizeof (struct bklist)), 23);
	bkqinitialized = 1;
    }
    probe.name = name;
    if (!(bkl = (struct bklist *) hashtab_Find(&BKqueue, &probe))) {
	probe.classp = classp;
	dlist_Init(&(probe.elts), (sizeof (struct bkqlistelt)), 8);
	hashtab_Add(&BKqueue, &probe);
	bkl = (struct bklist *) hashtab_Find(&BKqueue, &probe);
    } else
	free(name);
/* Give way to those who want to get speed by giving up duplicate checking */
    if (speed_load == 0)
      {
        if (getenv("SPEED_LOAD_BINDKEYS") != NULL) 
          speed_load = 1;
        else
          speed_load = -1;
      }
/* Check for speed loading */
    if (speed_load == 1) {
/* If speed loading append without checking for duplicates (the old way) */
        bkqle.unb = unb;
        bkqle.seq = seq;
        bkqle.fn = fn;
        bkqle.arg = data;
        bkqle.label = label;
        bkqle.doc = doc;
        dlist_Append(&(bkl->elts), &bkqle);
/* Otherwise check for a duplicate before appending (the new way) */
    } else {
/* The following logic tries to keep identical entries out of the queue */
    identical = 0; /* Assume they are not identical */
/* Scan the queue looking for identical entries */
    dlist_FOREACH(&(bkl->elts), struct bkqlistelt, pbkqle, j) {
         sameness_count = 0; /* Assume they are not all the same */
/* If unb is the same increment the sameness count */
         if (unb == pbkqle->unb) 
             sameness_count++;
/* If seq is the same increment the sameness count */
         if ((seq != NULL) && (pbkqle->seq != NULL))
           {
             if (strcmp(seq,pbkqle->seq) == 0)  
                 sameness_count++;
           }
         else
           {
             if (seq == pbkqle->seq)
                 sameness_count++;
           }
/* If fn is the same increment the sameness count */
         if ((fn != NULL) && (pbkqle->fn != NULL))
           {
             if (strcmp(fn,pbkqle->fn) == 0)
                 sameness_count++;
           }
         else
           {
             if (fn == pbkqle->fn)
                 sameness_count++;
           }
/* If data is the same increment the sameness count */
         if ((data != NULL) && (pbkqle->arg != NULL))
           {
             if (strcmp(data,pbkqle->arg) == 0)
                 sameness_count++;
           }
         else
           {
             if (data == pbkqle->arg)
                 sameness_count++;
           }
/* If label is the same increment the sameness count */
         if ((label != NULL) && (pbkqle->label != NULL))
           {
             if (strcmp(label,pbkqle->label) == 0)
                 sameness_count++;
           }
         else
           {
             if (label == pbkqle->label)
                 sameness_count++;
           }
/* If doc is the same increment the sameness count */
         if ((doc != NULL) && (pbkqle->doc != NULL))
           {
             if (strcmp(doc,pbkqle->doc) == 0)
                 sameness_count++;
           }
         else
           {
             if (doc == pbkqle->doc)
                 sameness_count++;
           }
/* If all 6 items are the same then it must be identical to one in the queue */
        if (sameness_count == 6)
          identical = 1; /* This one is identical */
    }
/* If it is not identical to something in the queue put it in the queue */
    if (identical == 0)
      {
        bkqle.unb = unb;
        bkqle.seq = seq;
        bkqle.fn = fn;
        bkqle.arg = data;
        bkqle.label = label;
        bkqle.doc = doc;
        dlist_Append(&(bkl->elts), &bkqle);
      }
    }
}

int
zm_bindkey(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
EXC_BEGIN 
{
    char *cmdname = *argv++;
    char *name = NULL;
    char *label = NULL;
    char *sequence = 0, *command = 0;
    char *data = NULL;
    char *doc = 0;
    int unbind = 0, e = 0, classp = 0;
    int db;

    if (*argv && !strcmp(*argv, "-?"))
	EXC_RETURNVAL(int, help(0, "bindkey", cmd_help));
    TRY {
	if (!*argv)
	    RAISE("usage", NULL);
	unbind = !ci_strcmp(cmdname, "unbindkey");
	while (*argv && (**argv == '-')) {
	    switch (argv[0][1]) {
	      case 'c':
		classp = 1;
		break;
	      case 'l':
		if (!(label = *++argv))
		    RAISE("usage", NULL);
		break;
	      case 'd':
		if (!(doc = *++argv))
		    RAISE("usage", 0);
		break;
	      default:
		RAISE("usage", NULL);
	    }
	    ++argv;
	}
	if (!(name = *argv++))
	    RAISE("usage", NULL);
	if (!(sequence = *argv++))
	    RAISE("usage", NULL);
	if (!unbind && !(command = *argv++))
	    RAISE("usage", NULL);
	if (!unbind && *argv) {
	    data = (char *) emalloc(1 + strlen(*argv), "zm_bindkey");
	    strcpy(data, *argv);
	}
	if (!ZmlIm
	    || !classp
	    || ((db = dobindkey(sequence, unbind, classp, name,
				command, data, label, doc)) == 2)) {
	    enqueue_bindkey(savestr(sequence), unbind, classp,
			    savestr(name), unbind ? 0 : savestr(command),
			    data, (label ? savestr(label) : 0),
			    doc ? savestr(doc) : 0);
	    if (ZmlIm && !classp)
		trybkqueue(name);
	    EXC_RETURNVAL(int, 0);
	}
	EXC_RETURNVAL(int, (db ? -1 : 0));
    } EXCEPT("usage") {
	e = 1;
	if (ci_strcmp(cmdname, "unbindkey")) {
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 741, "Usage: %s [-l label] [-d doc] [-c] name sequence command [data]"),
		  cmdname);
	} else {
	    error(UserErrWarning,
		  catgets(catalog, CAT_LITE, 742, "Usage: %s [-l label] [-c] name sequence"), cmdname);
	}
    } ENDTRY;
    if (e) {
	if (data)
	    free(data);
	EXC_RETURNVAL(int, -1);
    }
    spSend(ZmlIm, m_spIm_refocus);
    EXC_RETURNVAL(int, 0);
} EXC_END

static int
dismissTaskmeter(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    decr_taskmeter_count();
    spSend(im, m_spIm_dismissPopup, spEvent_data(ev));
    return (0);
}

int
gui_handle_intrpt(flags, nest, str, percentage)
    u_long flags;
    int nest;
    char *str;
    long percentage;
{
    static struct glist stack;
    static struct spPopupView *popup;
    static int initialized = 0;
    static struct spEvent *ev = (struct spEvent *) 0;
    struct taskmeter *tm;
    int retval = 0;

#if 0				/* this debugging code is used often */
    {
	static int lastnest = 0;
	int anyflag = 0;

	fputs("gui_handle_intrpt([", stderr);
	if (ison(flags, INTR_ON)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_ON", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_OFF)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_OFF", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_CHECK)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_CHECK", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_NOOP)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_NOOP", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_MSG)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_MSG", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_RANGE)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_RANGE", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_NONE)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_NONE", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_REDRAW)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_REDRAW", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_WAIT)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_WAIT", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_CONT)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_CONT", stderr);
	    anyflag = 1;
	}
	if (ison(flags, INTR_LONG)) {
	    if (anyflag)
		fputc(',', stderr);
	    fputs("INTR_LONG", stderr);
	    anyflag = 1;
	}
	fprintf(stderr, "], nest=%d, \"%s\", pct=%ld)\n",
		nest, str ? str : "(empty)", percentage);

	if (ison(flags, INTR_ON)) {
	    if (nest != (lastnest + 1)) {
		fprintf(stderr, "** Hey, INTR_ON (nest=%d, lastnest=%d)\n",
			nest, lastnest);
	    }
	} else if (ison(flags, INTR_OFF)) {
	    if (nest != (lastnest - 1)) {
		fprintf(stderr, "** Hey, INTR_OFF (nest=%d, lastnest=%d)\n",
			nest, lastnest);
	    }
	} else if (nest != lastnest) {
	    fprintf(stderr,
		    "** Hey, !INTR_ON, !INTR_OFF (nest=%d, lastnest=%d)\n",
		    nest, lastnest);
	}
	lastnest = nest;

    }

#endif /* 0 */

    if (!initialized) {
	initialized = 1;
	glist_Init(&stack, (sizeof (struct taskmeter *)), 4);
	popup = spPopupView_NEW();
    }
    if (!ZmlIm || (istool != 2))
	return (0);
    if (ison(flags, INTR_ON)) {
	if ((intr_level == -1) || (intr_level > percentage))
	    return (0);
	stopped = 0;
	if (ev && spEvent_inqueue(ev)) {
	    spSend(ev, m_spEvent_cancel, 1);
	    ev = (struct spEvent *) 0;
	}
	tm = taskmeter_NEW();
	spSend(tm, m_taskmeter_setMainMsg, str);
	spSend(tm, m_taskmeter_setScale, 0);
	spSend(tm, m_taskmeter_setInterruptable,
	       isoff(flags, INTR_NONE) ? taskmeter_INTERRUPTABLE : 0);
	taskmeter_flags(tm) = flags;
	turnoff(taskmeter_flags(tm), INTR_OFF | INTR_ON);
#if 0				/* Why would we do this? */
	if (!glist_Length(&stack)) {
	    spSend(popup, m_spPopupView_setView, tm);
	}
#else
	spSend(popup, m_spPopupView_setView, tm);
#endif
	glist_Add(&stack, &tm);
	if (!spView_window(popup)) {
	    incr_taskmeter_count();
	    spSend(ZmlIm, m_spIm_popupView, popup, 0, -1, -1);
	}
	retval = 0;
	spSend(ZmlIm, m_spIm_forceUpdate, 0);
    } else if (ison(flags, INTR_OFF)) {
	if (glist_Length(&stack) > nest) {
	    struct taskmeter *oldtm = *((struct taskmeter **)
					glist_Last(&stack));

	    glist_Pop(&stack);
	    if (glist_Length(&stack)) {
		spSend(popup, m_spPopupView_setView,
		       *((struct taskmeter **) glist_Last(&stack)));
	    } else {
		if (ev) {
		    ASSERT(!spEvent_inqueue(ev), "assertion failed",
			   "event in queue");
		    spoor_DestroyInstance(ev);
		}
		spSend(popup, m_spPopupView_setView, (struct spView *) 0);
		ev = spEvent_NEW();
		spSend(ev, m_spEvent_setup, (long) 0, (long) 0, 1,
		       dismissTaskmeter, popup);
		spSend(ZmlIm, m_spIm_enqueueEvent, ev);
	    }
	    spoor_DestroyInstance(oldtm);
	}
	if (nest > 0) {
	    retval = (stopped == 1);
	} else {
	    stopped = -1;
	    retval = 0;
	}
	spSend(ZmlIm, m_spIm_forceUpdate, 0);
    }
    if (isoff(flags, INTR_ON)
	&& (stopped != -1)
	&& glist_Length(&stack)
	&& (tm = *((struct taskmeter **) glist_Last(&stack)))) {
	if (ison(flags, INTR_RANGE)) {
	    if (isoff(flags, INTR_OFF))
		spSend(tm, m_taskmeter_setScale, percentage);
	} else if (ison(flags, INTR_REDRAW) && ison(taskmeter_flags(tm),
						    INTR_RANGE)) {
	    spSend(tm, m_taskmeter_setScale, taskmeter_percentage(tm));
	}
	if (ison(flags, INTR_MSG) && str) {
	    spSend(tm, m_taskmeter_setSubMsg, str);
	}
	taskmeter_flags(tm) = flags;
	if (ison(flags, INTR_WAIT)) {
	    while (spSend_i(ZmlIm, m_spIm_getChar) != ' ')
		;
	    stopped = 1;
	    turnon(glob_flags, WAS_INTR);
	} else {
	    if (ison(glob_flags, WAS_INTR)) {
		stopped = 1;
	    } else if (spSend_i(ZmlIm, m_spIm_checkChar, (long) 0, (long) 0)
		       && (spSend_i(ZmlIm, m_spIm_getChar) == ' ')) {
		stopped = 1;
		turnon(glob_flags, WAS_INTR);
	    }
	}
	retval = ison(glob_flags, WAS_INTR);
	spSend(ZmlIm, m_spIm_forceUpdate, 0);
    }
    return (retval);
}

void
gui_redraw_hdr_items(unused, list, sel)
    GuiItem unused;
    msg_group *list;
    int sel;
{
    spSend(Dialog(&MainDialog), m_zmlmainframe_redrawHdrs, list, sel);
}

void
EnterScreencmd(clr)
    int clr;
{
    if (screencmd_pending++ == 0) {
	if (clr) {
	    spSend(spView_window(ZmlIm), m_spWindow_clear);
	    spSend(spView_window(ZmlIm), m_spWindow_sync);
	}
	spSend(ZmlIm, m_spCharIm_shellMode);
    }
}

void
ExitScreencmd(prompt)
    int prompt;
{
    if (--screencmd_pending == 0) {
	if (prompt) {
	    int c;

	    fputs(catgets(catalog, CAT_LITE, 743, "Press ENTER to resume Z-Mail Lite: "), stdout);
	    fflush(stdout);
	    do {
		c = fgetc(stdin);
	    } while ((c != EOF) && (c != '\n'));
	}
	spSend(ZmlIm, m_spCharIm_progMode);
	spSend(ZmlIm, m_spIm_showmsg, "", 25, 0, 0);
    }
    if (screencmd_pending < 0)
	screencmd_pending = 0;
}

int
screencmd(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    int prompt = 0, clr = 0, result;
    int shargc = 1;
    static char *shargv[] = { "sh", NULL };

    ++argv;
    --argc;
    while ((argc > 0) && (*argv[0] == '-')) {
	if (index(argv[0] + 1, 'p'))
	    prompt = 1;
	if (index(argv[0] + 1, 'c'))
	    clr = 1;
	++argv;
	--argc;
    }
    if (argc < 1) {
	argc = shargc;
	argv = shargv;
    }
    if (!istool)
	return (exec_argv(argc, argv, list));

    TRY {
	EnterScreencmd(clr);
	result = exec_argv(argc, argv, list);
    } FINALLY {
	turnoff(glob_flags, WAS_INTR);
	ExitScreencmd(prompt);
    } ENDTRY;
    return (result);
}

AskAnswer
gui_confirm_addresses(compose)
    Compose *compose;
{
    AskAnswer retval = ask(((istool > 1 || ison(compose->send_flags, SEND_NOW))
			    ? AskOk : AskYes),
			   catgets(catalog, CAT_LITE, 744, "Send Message?"));
    /* XXX this logic should not be used, of course, if we ever fully
       implement $verify += addresses in Lite the way we have it in Motif */
    return (AskCancel == retval) ? AskNo: retval;
}

static void
helpPagerInit(p)
    struct zmPager *p;
{
    p->pager = (struct spView *) helpDialog_NEW();
}

static int
helpPagerWrite(p, str)
    struct zmPager *p;
    char *str;
{
    spSend(p->pager, m_pagerDialog_append, str);
    return (0);
}

static int
help_interact(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    struct helpDialog *p = (struct helpDialog *) spEvent_data(ev);

    TRY {
	spSend(p, m_dialog_interactModally);
    } FINALLY {
	spoor_DestroyInstance(p);
    } ENDTRY;
    return (1);
}

static void
helpPagerEnd(p)
    struct zmPager *p;
{
    if (p->title)
	spSend(p->pager, m_pagerDialog_setTitle, p->title);
    if (WaitingForChildInteractively)
	spSend(ZmlIm, m_spIm_interactReturn, 0);
    spSend(ZmlIm, m_spIm_enqueueEvent,
	   spEvent_Create((long) 0, (long) 0, 1,
			  help_interact, p->pager));
}

static void
textPagerInit(p)
    struct zmPager *p;
{
    p->pager = (struct spView *) pagerDialog_NEW();
    if (p->file)
	spSend(p->pager, m_pagerDialog_setFile, p->file);
    if (ison(p->flags, PG_EDITABLE))
	spSend(spView_observed(pagerDialog_textview(p->pager)),
	       m_spText_setReadOnly, 0);
}

static int
textPagerWrite(p, str)
    struct zmPager *p;
    char *str;
{
    char *tstr;
    size_t len;

    len = strlen(str);
    tstr = (char *) quick_c3_translate(str, &len, p->charset, displayCharSet);
    if (pg_check_interrupt(p, len) || pg_check_max_text_length(p)) {
	char intrmsg[200];
	sprintf(intrmsg,
		catgets(catalog, CAT_LITE, 908, "\n[ ** display aborted after %d bytes ** ]\n"), 
		p->tot_len);
	spSend(p->pager, m_pagerDialog_append, intrmsg);
	return EOF;
    }
    spSend(p->pager, m_pagerDialog_append, tstr);
    p->tot_len += len;
    return (0);
}

static int
text_interact(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    struct pagerDialog *p = (struct pagerDialog *) spEvent_data(ev);

    TRY {
	spSend(p, m_dialog_interactModally);
    } FINALLY {
	spoor_DestroyInstance(p);
    } ENDTRY;
    return (1);
}

static void
textPagerEnd(p)
    struct zmPager *p;
{
    if (p->title)
	spSend(p->pager, m_pagerDialog_setTitle, p->title);
    if (WaitingForChildInteractively)
	spSend(ZmlIm, m_spIm_interactReturn, 0);
    spSend(ZmlIm, m_spIm_enqueueEvent,
	   spEvent_Create((long) 0, (long) 0, 1,
			  text_interact, p->pager));
}

static void
hindexPagerInit(p)
    struct zmPager *p;
{
    static struct helpIndex *hindex = 0;

    if (!hindex)
	hindex = helpIndex_NEW();

    p->pager = (struct spView *) hindex;
    spSend(hindex, m_helpIndex_clear);
}

static int
hindexPagerWrite(p, str)
    struct zmPager *p;
    char *str;
{
    spSend(p->pager, m_helpIndex_append, str);
    return (0);
}

static int
hindex_interact(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    struct helpIndex *self = (struct helpIndex *) spEvent_data(ev);

    if (!spView_window(self))
	spSend(self, m_dialog_interactModally);
    return (1);
}

static void
hindexPagerEnd(p)
    struct zmPager *p;
{
    spSend(ZmlIm, m_spIm_enqueueEvent,
	   spEvent_Create(0, 0, 1, hindex_interact, p->pager));
}

static void
msgPagerInit(p)
    struct zmPager *p;
{
    if (ison(p->flags, PG_PINUP)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 745, "Pinups not supported in Z-Mail Lite"));
    } else {
	spSend(p->pager = (struct spView *) Dialog(&MessageDialog),
	       m_zmlmsgframe_setmsg, current_msg, 0);
    }
}

static int
msgPagerWrite(p, str)
    struct zmPager *p;
    char *str;
{
    size_t len = strlen(str);
    if (pg_check_max_text_length(p)) {
	char intrmsg[200];
	sprintf(intrmsg,
		catgets(catalog, CAT_LITE, 909, "\n[ ** display aborted after %d bytes ** ]\n"), 
		p->tot_len);
	spSend(p->pager, m_zmlmsgframe_append, intrmsg);
	return EOF;
    }
    spSend(p->pager, m_zmlmsgframe_append, str);
    p->tot_len += len;
    return (0);
}

static void
msgPagerEnd(p)
    struct zmPager *p;
{
    if (!spView_window(p->pager))
	spSend(p->pager, m_dialog_enter);
}

static void
wprintPagerInit(p)
    struct zmPager *p;
{
    /* Hacer nada */
}

static int
wprintPagerWrite(p, str)
    struct zmPager *p;
    char *str;
{
    wprint("%s", str);
    return (0);
}

static void
wprintPagerEnd(p)
    struct zmPager *p;
{
    /* Hacer nada */
}

static void
printpager_init(pager)
    ZmPager pager;
{
    ZmPagerSetFlag(pager, PG_NO_GUI);
}

struct zmPagerDevice gui_pager_devices[] = {
    { PgHelp, helpPagerInit, helpPagerWrite, helpPagerEnd, 0 },
    { PgHelpIndex, hindexPagerInit, hindexPagerWrite, hindexPagerEnd, 0 },
    { PgText, textPagerInit, textPagerWrite, textPagerEnd, 0 },
    { PgMessage, msgPagerInit, msgPagerWrite, msgPagerEnd, 0 },
    { PgNormal, wprintPagerInit, wprintPagerWrite, wprintPagerEnd, 0 },
    { PgInternal, wprintPagerInit, wprintPagerWrite, wprintPagerEnd, 0 },
    { PgPrint, printpager_init, 0, 0 },
    { PgOutput, wprintPagerInit, wprintPagerWrite, wprintPagerEnd, 0 },
};

void
gui_open_folder(fldr)
    msg_folder *fldr;
{
    /* Do nothing */
}

int
gui_get_state(item)
    int item;
{
    switch (item) {
      case GSTATE_PINUP:
	return (spIm_view(ZmlIm)
		&& spoor_IsClassMember(spIm_view(ZmlIm),
				       (struct spClass *) zmlmsgframe_class)
		&& (spIm_view(ZmlIm) != ((struct spView *)
					 Dialog(&MessageDialog))));

      case GSTATE_ATTACHMENTS:
	return (spIm_view(ZmlIm)
		&& spoor_IsClassMember(spIm_view(ZmlIm),
				       (struct spClass *) zmlmsgframe_class)
		&& (spSend_i(spIm_view(ZmlIm),
			   m_zmlmsgframe_countAttachments) > 0));

      case GSTATE_IS_NEXT:
	return (spIm_view(ZmlIm)
		&& (current_folder == 
		    (msg_folder *) spSend_p(spIm_view(ZmlIm),
					  m_dialog_folder))
		&& (next_msg(current_msg, 1) != current_msg));

      case GSTATE_IS_PREV:
	return (spIm_view(ZmlIm)
		&& (current_folder ==
		    (msg_folder *) spSend_p(spIm_view(ZmlIm),
					  m_dialog_folder))
		&& (next_msg(current_msg, -1) != current_msg));

      case GSTATE_ACTIVE_COMP:
	return (ComposeDialog
		|| spoor_IsClassMember(spIm_view(ZmlIm),
				       (struct spClass *) zmlcomposeframe_class));
      case GSTATE_PHONETAG:
        return(-1);
      case GSTATE_TAGIT:
        return(-1);
      default:
        return(-1);
/*	RAISE("unimplemented", "gui_get_state"); */
	/* Not reached */
    }
    /* Not reached */
}

void
gui_install_all_btns(slot, name, dialog)
    int slot;
    char *name;
    struct dialog *dialog;
{
    struct blistCopyElt bce;
    struct blistCopyElt *bcep;
    char *old = 0, *unsupported = 0;
    ZmButtonList blist, oldlist;
    int i, j;

    switch (slot) {
      case MAIN_WINDOW_BUTTONS:
      case MSG_WINDOW_BUTTONS:
      case COMP_WINDOW_BUTTONS:
      case MAIN_WINDOW_MENU:
      case MSG_WINDOW_MENU:
      case COMP_WINDOW_MENU:
	break;
      case TOOLBOX_ITEMS:
	unsupported = catgets(catalog, CAT_LITE, 746, "toolbox");
	break;
      case HEADER_POPUP_MENU:
	unsupported = catgets(catalog, CAT_LITE, 747, "summaries area popup");
	break;
      case OUTPUT_POPUP_MENU:
	unsupported = catgets(catalog, CAT_LITE, 748, "output area popup");
	break;
      case MESSAGE_TEXT_POPUP_MENU:
	unsupported = catgets(catalog, CAT_LITE, 749, "message text popup");
	break;
      case COMMAND_POPUP_MENU:
	unsupported = catgets(catalog, CAT_LITE, 750, "command area popup");
	break;
      default:
	unsupported = catgets(catalog, CAT_LITE, 751, "requested button/menu option");
	break;
    }

    if (unsupported) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 752, "%s not supported in Z-Mail Lite"),
	      unsupported);
	return;
    }

    if (name) {
	old = button_panels[slot];
	button_panels[slot] = savestr(name);
    }
    name = button_panels[slot];

    blist = GetButtonList(name);

    if (dialog) {
	bce.context = slot;
	bce.container = 0;
	bce.dialog = dialog;
	dlist_Append(&(blist->copylist), &bce);
	spSend(dialog, m_dialog_installZbuttonList, blist);
    } else if (old && (oldlist = GetButtonList(old))) {
	dlist_FOREACH2(&(oldlist->copylist), struct blistCopyElt, bcep, i, j) {
	    if (bcep->context == slot) {
		bce.context = slot;
		bce.container = 0;
		bce.dialog = bcep->dialog;
		spSend(bcep->dialog, m_dialog_uninstallZbuttonList, slot,
		       oldlist);
		dlist_Append(&(blist->copylist), &bce);
	    }
	}
	dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	    if (!(bcep->container))
		spSend(bcep->dialog, m_dialog_installZbuttonList, blist);
	}
    }
    xfree(old);
}

void
gui_install_button(button, blist)
    ZmButton button;
    ZmButtonList blist;
{
    int i;
    struct blistCopyElt *bcep;

    if (istool < 2)
	return;

    dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	spSend(bcep->dialog, m_dialog_installZbutton, button, blist, 0);
    }
}

void
gui_remove_button(button)
    ZmButton button;
{
    struct blistCopyElt *bcep;
    ZmButtonList blist = GetButtonList(ButtonParent(button));
    int i;

    dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	spSend(bcep->dialog, m_dialog_uninstallZbutton, button, blist, 0);
    }
}

char *
gui_msg_context()
{
    if (istool < 2)
	return ("");
    return ((char *) spSend_p((spIm_view(ZmlIm)), m_dialog_mgroupstr));
}

/* Largely copied from motif/m_comp2.c */
void
gui_request_priority(compose, p)
    Compose *compose;
    const char *p;
{
    if (ison(compose->flags, EDIT_HDRS)) {
	spSend(compose->compframe, m_zmlcomposeframe_writeEdFile, 0, 0);
	resume_compose(compose);
	if (reload_edfile() != 0) {
	    suspend_compose(compose);
	    return;
	}
    }
    request_priority(compose, p);
    if (ison(compose->flags, EDIT_HDRS)) {
	(void) prepare_edfile();
	suspend_compose(compose);
	spSend(compose->compframe, m_zmlcomposeframe_readEdFile, 0);
    }
}

/* Largely copied from gui_request_priority,
   hence largely copied from motif/m_comp2.c */
void
gui_request_receipt(compose, on, p)
    Compose *compose;
    int on;
    char *p;
{
    if (ison(compose->flags, EDIT_HDRS)) {
	spSend(compose->compframe, m_zmlcomposeframe_writeEdFile, 0, 0);
	resume_compose(compose);
	if (reload_edfile() != 0) {
	    suspend_compose(compose);
	    return;
	}
    }
    request_receipt(compose, on, p);
    if (ison(compose->flags, EDIT_HDRS)) {
	(void) prepare_edfile();
	suspend_compose(compose);
	spSend(compose->compframe, m_zmlcomposeframe_readEdFile, 0);
    }
}

int
gui_choose_one_aux(d, query, dflt, choices, n_choices, flags, flags2, out)
    struct dynstr *d;
    char *query, *dflt, **choices;
    int n_choices;
    unsigned long flags, flags2;
    GENERIC_POINTER_TYPE *out;
{
    int retval;
    struct chooseone *co = chooseone_Create(query, dflt, flags,
					    choices, n_choices,
					    flags2);

    TRY {
	switch (spSend_i(co, m_dialog_interactModally)) {
	  case chooseone_Accept:
	    spSend(co, m_chooseone_result, d);
	    if (out) {
		if (flags2 & chooseone_OpenFolder) {
		    *((int *) out) = spToggle_state(co->rw_toggle);
		} else if (flags2 & chooseone_RenameFolder) {
		    spSend(spView_observed(co->newname),
			   m_spText_appendToDynstr,
			   (struct dynstr *) out, 0, -1);
		    dynstr_Set((struct dynstr *) out,
			       ((char *)
				spSend_p(co->files,
					 m_filelist_fullpath,
					 dynstr_Str((struct dynstr *) out))));
		}
	    }
	    retval = 0;
	    break;
	  case dialog_Close:
	  case dialog_Cancel:
	    retval = -1;
	    break;
	  case chooseone_Retry:
	    spSend(co, m_chooseone_result, d);
	    retval = 1;
	    break;
	  case chooseone_Omit:
	    retval = 0;
	    break;
	  case chooseone_FileOptionSearch:
	    {
		struct chooseone *co2;

		co2 = chooseone_Create(query, dflt,
				       ((flags &
					 ~(PB_FILE_OPTION | PB_TRY_AGAIN)) |
					PB_FILE_BOX), choices, n_choices, 0);
		TRY {
		    switch (spSend_i(co2, m_dialog_interactModally)) {
		      case chooseone_Accept:
			spSend(co2, m_chooseone_result, d);
			retval = 0;
			break;
		      case dialog_Close:
		      case dialog_Cancel:
			retval = -1;
			break;
		    }
		} FINALLY {
		    spoor_DestroyInstance(co2);
		} ENDTRY;
	    }
	    break;
	}
    } FINALLY {
	spoor_DestroyInstance(co);
    } ENDTRY;
    return (retval);
}

int
gui_choose_one(d, query, dflt, choices, n_choices, flags)
    struct dynstr *d;
    char *query;
    const char *dflt, **choices;
    int n_choices;
    unsigned long flags;
{
    return (gui_choose_one_aux(d, query, dflt, choices,
			       n_choices, flags, 0, 0));
}

int
gui_help(str, context)
    const char *str;
    unsigned long context;		/* ignored for now */
{
    struct zmPager *pager = ZmPagerStart(PgHelpIndex);

    ZmPagerWrite(pager, "");
    spSend(pager->pager, m_helpIndex_setTopic, str);
    ZmPagerStop(pager);
}

#define DIALOG(x, y) \
    if (dp == (struct dialog **) &x) { \
        if (!x) \
            x = y(); \
        return ((struct dialog *) x); \
    }

struct dialog *
Dialog(gp)
    GENERIC_POINTER_TYPE *gp;
{
    struct dialog **dp = (struct dialog **) gp;

    DIALOG(AddrbrowseDialog, addrbook_NEW);
    DIALOG(AliasDialog, zmlaliasframe_NEW);
    DIALOG(CharsetDialog, charset_NEW);
    DIALOG(CompAliasesDialog, smallaliases_NEW);
    DIALOG(CompOptionsDialog, compopt_NEW);
    DIALOG(DatesearchDialog, dsearch_NEW);
    DIALOG(EnvelopeDialog, zmlenvframe_NEW);
    DIALOG(HeadersDialog, zmlhdrsframe_NEW);
    DIALOG(HelpIndexDialog, helpIndex_NEW);
    DIALOG(MainDialog, zmlmainframe_NEW);
    DIALOG(MessageDialog, zmlmsgframe_NEW);
    DIALOG(MultikeyDialog, multikey_NEW);
    DIALOG(PrinterDialog, printdialog_NEW);
    DIALOG(SearchDialog, psearch_NEW);
    DIALOG(SortDialog, sortdialog_NEW);
    DIALOG(TemplateDialog, templatedialog_NEW);
    DIALOG(VariablesDialog, zmlvarframe_NEW);

    return (0);
}

#if defined( IMAP ) 

static int irg = 0;

void
SetInRemoveGUI()
{
        irg = 1;
}

void
ClearInRemoveGUI()
{
        irg = 0;
	check_new_mail();
}

int
InRemoveGUI()
{
        return( irg );
}

static int iwf = 0;

void
SetInWriteToFile()
{
        iwf = 1;
}

void
ClearInWriteToFile()
{
        iwf = 0;
}

int
InWriteToFile()
{
        return( iwf );
}

int
GetUseIMAP()
{
	return( UseIMAP() );
}

#endif /* IMAP */

void
ZCommand(cmd, mode)
    char *cmd;
    enum zcmd_mode mode;
{
    char *buf, *percent = NULL, *liststr;
    struct dialog *dialog;
    msg_folder *fldr;
    msg_group mgroup;
    int cm;

#if defined( IMAP )
    if ( !strcmp( cmd, "zmenu_save_to_file" ) )
        SetInWriteToFile();
#endif
    if (!(dialog = (struct dialog *) spIm_view(ZmlIm)))
	dialog = Dialog(&MainDialog);

    fldr = (msg_folder *) spSend_p(dialog, m_dialog_folder);
    current_folder = fldr;
    liststr = (char *) spSend_p(dialog, m_dialog_mgroupstr);
    clear_msg_group(&current_folder->mf_group);
    if (liststr && *liststr && (cm = chk_msg(liststr)))
	current_msg = cm - 1;
    if (mode != zcmd_commandline) {
	buf = (char *) emalloc(1 + (3 * strlen(liststr)) + strlen(cmd),
			       "ZCommand");
	percent = cmd;
	while (percent
	       && (percent = index(percent, '%'))
	       && (percent[1] == '%'))
	    ;
	if (percent && (percent[1] != 's'))
	    error(ZmErrFatal, catgets(catalog, CAT_LITE, 753, "Bad format string!!"));
	sprintf(buf, cmd, liststr, liststr, liststr);
    } else {
	buf = (char *) emalloc(1 + strlen(cmd),
			       "ZCommand");
	strcpy(buf, cmd);
    }
    init_msg_group(&mgroup, 1, 0);
    ++ZScriptPending;
    TRY {
	if (percent && (!liststr || !*liststr)) {
	    RAISE("no messages", NULL);
	}
#if defined( IMAP )
	SetInRemoveGUI();
#endif
	if (cmd_line(buf, &mgroup) == 0) {
	    int mlcount = count_msg_list(&mgroup);

	    gui_redraw_hdr_items(0, &current_folder->mf_group, 0);
	    if ((mode != zcmd_ignore)
		&& (mlcount != 0)
		&& ((mode == zcmd_use)
		    || (mode == zcmd_commandline)
		    || ((mode == zcmd_useIfNonempty) && (mlcount > 0)))) {
		if (fldr == current_folder) {
		    spSend(dialog, m_dialog_setmgroup, &mgroup);
		    if (dialog != (struct dialog *) MainDialog)
			spSend(Dialog(&MainDialog), m_dialog_setmgroup,
			       &mgroup);
		}
	    }
	}
#if defined( IMAP )
	ClearInRemoveGUI();
	ClearInWriteToFile();
#endif
    } EXCEPT("no messages") {
	error(HelpMessage, catgets(catalog, CAT_LITE, 576, "Select one or more messages"));
    } FINALLY {
	--ZScriptPending;
	free(buf);
	destroy_msg_group(&mgroup);
	gui_refresh(current_folder, REDRAW_SUMMARIES);
    } ENDTRY;
}

void
gui_cleanup()
{
    if (istool == 2) {
	istool = 1;
	if (spView_window(ZmlIm)) {
	    spSend(ZmlIm, m_spCharIm_shellMode);
	}
	spoor_DestroyInstance(ZmlIm);
	ZmlIm = 0;
    }
}

static int neednewline = 0;

static void
save_bindkeys_aux(fp, name, classp, keymap, level, ks)
    FILE *fp;
    char *name;
    int classp;
    struct spKeymap *keymap;
    int level;
    struct spKeysequence *ks;
{
    struct dynstr d;
    struct spKeymapEntry *kme;
    int i;
    char *p = index(name, ' ');

    dynstr_Init(&d);
    if (p) {
	dynstr_AppendN(&d, name, p - name);
	name = dynstr_Str(&d);
    }
    TRY {
	for (kme = spKeymap_First(keymap); kme; kme = spKeymap_Next(keymap,
								    kme))
	    switch (kme->type) {
	      case spKeymap_function:
		if (neednewline) {
		    fputc('\n', fp);
		    neednewline = 0;
		}
		fputs("bindkey ", fp);
		if (kme->content.function.label) {
		    fputs("-l '", fp);
		    fputs(quotezs(kme->content.function.label, '\''), fp);
		    fputs("' ", fp);
		}
		if (kme->content.function.doc) {
		    fputs("-d \"", fp);
		    fputs(quotezs(kme->content.function.doc, '"'), fp);
		    fputs("\" ", fp);
		}
		if (classp)
		    fputs("-c ", fp);
		fputs(name, fp);
		fputc(' ', fp);
		fputc('\'', fp);
		for (i = 0; i < level; ++i)
		    fputs(quotezs(spKeyname(spKeysequence_Nth(ks, i), 0),
				  '\''), fp);
		fputs(quotezs(spKeyname(kme->c, 0), '\''), fp);
		fputs("' ", fp);
		fputs(quotezs(kme->content.function.fn, 0), fp);
		if (kme->content.function.data) {
		    fputs(" '", fp);
		    fputs(quotezs(kme->content.function.data, '\''), fp);
		    fputc('\'', fp);
		}
		fputc('\n', fp);
		break;

	      case spKeymap_removed:
		if (neednewline) {
		    fputc('\n', fp);
		    neednewline = 0;
		}
		fputs("unbindkey ", fp);
		if (classp)
		    fputs("-c ", fp);
		fputs(name, fp);
		fputs(" '", fp);
		for (i = 0; i < level; ++i)
		    fputs(quotezs(spKeyname(spKeysequence_Nth(ks, i), 0),
				  '\''), fp);
		fputs(quotezs(spKeyname(kme->c, 0), '\''), fp);
		fputs("'\n", fp);
		break;

	      case spKeymap_keymap:
		spKeysequence_Truncate(ks, level);
		spKeysequence_Add(ks, kme->c);
		save_bindkeys_aux(fp, name, classp, kme->content.keymap,
				  level + 1, ks);
		break;
	    }
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
save_bindkeys(name, classp, keymap, fp)
    char *name;
    int classp;
    struct spKeymap *keymap;
    FILE *fp;
{
    struct spKeysequence ks;

    spKeysequence_Init(&ks);
    TRY {
	save_bindkeys_aux(fp, name, classp, keymap, 0, &ks);
    } FINALLY {
	spKeysequence_Destroy(&ks);
    } ENDTRY;
}

void
gui_save_state()
{
    int i;
    struct hashtab_iterator hti;
    struct spoor_RegistryElt *re;
    FILE *fp = 0;
    struct dynstr d;
    struct spWidgetInfo *winfo;

    if (istool != 2)
	return;
    dynstr_Init(&d);
    TRY {
	if (!dyn_choose_one(&d, catgets(catalog, CAT_LITE, 755, "Save keybindings to:"), 
			    "~/.zmbindkey", 0, 0,
			    PB_FILE_BOX | PB_NOT_A_DIR)
	    && !dynstr_EmptyP(&d)) {

	    TRY {
		fp = efopen(dynstr_Str(&d), "w", catgets(catalog, CAT_LITE, 756, "saving keybindings"));
	    } EXCEPT(ANY) {
		error(SysErrWarning, catgets(catalog, CAT_LITE, 120, "Cannot open %s"), dynstr_Str(&d));
		PROPAGATE();
	    } ENDTRY;

	    fputs(catgets(catalog, CAT_LITE, 758, "# Z-Mail Lite keybindings\n"), fp);
	    fputs(catgets(catalog, CAT_LITE, 727, "# This file generated automatically by Z-Mail Lite\n\n"),
		  fp);
	    neednewline = 0;

	    /* First enumerate all the widgetclasses we know about
	     * and dump their keymaps (in "bindkey" format).
	     */
	    spWidget_InitIterator();
	    while (winfo = spWidget_Iterate()) {
		if (spWidgetInfo_keymap(winfo))
		    save_bindkeys(spWidgetInfo_name(winfo), 1,
				  spWidgetInfo_keymap(winfo), fp);
		neednewline = 1;
	    }

	    /* We no longer use the instance registry to find named widgets,
	     * since all named-widget bindkeys persist in the bindkey
	     * queue, which is traversed as the last step (below).
	     */
#if 0
	    /* Next, iterate through the instance registry looking for
	     * views which have keymaps, and dump those too.
	     */
	    hashtab_InitIterator(&hti);
	    while (re = ((struct spoor_RegistryElt *)
			 hashtab_Iterate(spoor_InstanceRegistry, &hti))) {
		if (spoor_IsClassMember(re->obj, spView_class)
		    && spView_keybindings(re->obj)) {
		    save_bindkeys(re->name, 0,
				  spView_keybindings(re->obj), fp);
		    neednewline = 1;
		}
	    }
#endif

	    /* Finally, save the bindkeys that never happened because
	     * they're still in the bkqueue.
	     */
	    /* Actually, they may or may not have happened, but we're
	     * no longer relying on the instance registry to find widgets
	     * for which named-widget bindkeys have already happened.
	     * This solves several problems, among them:  Only the first
	     * named-widget with a given name was getting the bindkeys
	     * for that name; a named widget may have gotten destroyed,
	     * taking its keybindings with it (and defeating saveopts -g).
	     *  [bobg, Mon May 16 20:07:52 1994]
	     */
	    if (bkqinitialized && !hashtab_EmptyP(&BKqueue)) {
		struct hashtab_iterator hti;
		struct bklist *bkl;
		struct bkqlistelt *bkqle;
		struct dynstr d2;
		char *p;
		int j;

		hashtab_InitIterator(&hti);
		dynstr_Init(&d2);
		TRY {
		    while (bkl = ((struct bklist *)
				  hashtab_Iterate(&BKqueue, &hti))) {
			dlist_FOREACH(&(bkl->elts), struct bkqlistelt,
				      bkqle, j) {
			    if (neednewline) {
				fputc('\n', fp);
				neednewline = 0;
			    }
			    dynstr_Set(&d2, "");
			    if (p = index(bkl->name, ' '))
				dynstr_AppendN(&d2, bkl->name,
					       p - bkl->name);
			    else
				dynstr_Set(&d2, bkl->name);
			    if (bkqle->fn) {
				fputs("bindkey ", fp);
				if (bkqle->label) {
				    fputs("-l '", fp);
				    fputs(quotezs(bkqle->label, '\''), fp);
				    fputs("' ", fp);
				}
				if (bkqle->doc) {
				    fputs("-d \"", fp);
				    fputs(quotezs(bkqle->doc, '"'), fp);
				    fputs("\" ", fp);
				}
			    } else {
				fputs("unbindkey ", fp);
			    }
			    if (bkl->classp)
				fputs("-c ", fp);
			    fputs(dynstr_Str(&d2), fp);
			    fputs(" '", fp);
			    fputs(quotezs(bkqle->seq, '\''), fp);
			    fputc('\'', fp);
			    if (bkqle->fn) {
				fputs(" ", fp);
				fputs(quotezs(bkqle->fn, 0), fp);
				if (bkqle->arg) {
				    fputs(" '", fp);
				    fputs(quotezs(bkqle->arg, '\''), fp);
				    fputc('\'', fp);
				}
			    }
			    fputc('\n', fp);
			}
		    }
		} FINALLY {
		    dynstr_Destroy(&d2);
		} ENDTRY;
	    }
	}
    } FINALLY {
	dynstr_Destroy(&d);
	if (fp)
	    fclose(fp);
    } ENDTRY;
}

void
gui_update_list(list)
    struct options **list;
{
    if (istool == 2) {
	ZmlUpdatedList = list;
	spSend(ZmlIm, m_spObservable_notifyObservers, dialog_updateList, 0);
    }
}

void
gui_title(title)
    char *title;
{
    /* Stub */
}

void
gui_refresh(fldr, reason)
    msg_folder *fldr;
    u_long reason;
{
    if (istool != 2)
	return;

    RefreshReason = reason;
    turnon(fldr->mf_flags, GUI_REFRESH);
    if (current_folder)
	gui_redraw_hdr_items(0, &(current_folder->mf_group), 0);
    spSend(ZmlIm, m_spObservable_notifyObservers, dialog_refresh, 0);
    turnoff(fldr->mf_flags, GUI_REFRESH);
}

void
timeout_cursors(on)
    int on;
{
    /* Stub */
}

int
gui_redraw(argc, argv, grp)
    int argc;
    char **argv;
    msg_group *grp;
{
    msg_group tmp;
    int all = (argv && *argv && *++argv && strncmp(*argv, "-a", 2) == 0);

    if (all)
	argv++;
    if (istool > 1) {
	argc = get_msg_list(argv, grp);
	if (argc > 0 || ison(glob_flags, IS_PIPE)) {
	    init_msg_group(&tmp, msg_cnt, 1);
	    msg_group_combine(&tmp, MG_SET, grp);
	    msg_group_combine(&tmp, MG_ADD, &current_folder->mf_group);
	    spSend(Dialog(&MainDialog), m_dialog_setfolder, current_folder);
	    spSend(MainDialog, m_dialog_setmgroup, &tmp);
	    destroy_msg_group(&tmp);
	} else {
	    spSend(Dialog(&MainDialog), m_dialog_setfolder, current_folder);
	}
	if (all) {
	    /* "Touch" all the messages so gui_refresh does a
	     * complete repaint of the header display.
	     */
	    clear_msg_group(&current_folder->mf_group);	/* Clean slate */
	    resize_msg_group(&current_folder->mf_group, msg_cnt);
	    msg_group_combine(&current_folder->mf_group, MG_OPP, /* Set all */
			      &current_folder->mf_group);
	}
	gui_refresh(current_folder, REDRAW_SUMMARIES);
	spSend(ZmlIm, m_spView_invokeInteraction, "redraw", 0, 0, 0);
    } else {
	return -1;
    }
    return 0;
}

int
gui_task_meter(argc, argv)
    int argc;
    char **argv;
{
    char *argv0, message[256];
    int val, old_intr_level = intr_level;
    u_long flags = 0L;

    argv0 = *argv;
    message[0] = 0;
    while (argv && *++argv) {
	if (**argv == '-') {
	    switch (argv[0][1]) {
	      case 'o' :
		if (argv[0][2] == 'f') {
		    turnon(flags, INTR_OFF);
		} else if (argv[0][2] == 'n') {
		    turnon(flags, INTR_ON);
		} else {
		    error(UserErrWarning, catgets(catalog, CAT_LITE, 760, "%s: -on or -off"), argv0);
		    return -1;
		}
		break;
	      case 'n' :
		turnon(flags, INTR_NONE);
		break;
	      case 'c' :
		turnon(flags, INTR_CHECK);
		break;
	      case 'w' :
		turnon(flags, INTR_WAIT);
		break;
	      case '?' :
		return help(0, argv0, cmd_help);
	    }
	} else {
	    (void) argv_to_string(message, argv);
	    break;
	}
    }
    if (!flags
	|| ((ison(flags, INTR_ON) + ison(flags, INTR_CHECK) +
	     ison(flags, INTR_OFF)) > 1)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 761, "You must specify -on, -off or -check."));
	return -1;
    }
    /* allow "task_meter -wait" without requiring -on */
    if (isoff(flags, INTR_ON+INTR_OFF+INTR_CHECK) && ison(flags, INTR_WAIT))
	turnon(flags, INTR_ON);
    if (ison(flags, INTR_WAIT)) /* don't allow "-off -wait" */
	turnoff(flags, INTR_OFF);
    if (message[0])
	turnon(flags, INTR_MSG);

    intr_level = 1;
    val = handle_intrpt(flags, message, 2);
    if (ison(flags, INTR_ON)) {
	turnoff(flags, INTR_ON+INTR_OFF);
	turnon(flags, INTR_CHECK);
	val = handle_intrpt(flags, message, 2);
    }
    intr_level = old_intr_level;
    return val;
}

typedef struct MsgEdit {
    u_long offset;
    msg_folder *fldr;
    char *file;
} MsgEdit;

#ifdef ZM_CHILD_MANAGER
static int
restore_edited_msg(ev, im)
    struct spEvent *ev;
    struct spIm *im;
{
    MsgEdit *medit = (MsgEdit *) spEvent_data(ev);
    int i;
    int mcount = medit->fldr->mf_group.mg_max;
    msg_folder *cur_fldr;
    msg_group tmp;
    
    for (i = 0; i < mcount; i++)
	if (medit->fldr->mf_msgs[i]->m_offset == medit->offset)
	    break;
    if (i == mcount)		/* lost the message... */
	return;
    turnoff(medit->fldr->mf_msgs[i]->m_flags, EDITING);
    init_msg_group(&tmp, msg_cnt, 1);
    (void) add_msg_to_group(&tmp, i);
    if (check_replies(medit->fldr, &tmp, TRUE) >= 0) {
	cur_fldr = current_folder;
	current_folder = medit->fldr;
	if (load_folder(medit->file, FALSE, i, NULL_GRP) > 0)
	    (void) unlink(medit->file);
	current_folder = cur_fldr;
    }
    destroy_msg_group(&tmp);
    xfree(medit->file);
    xfree(medit);
    gui_refresh(current_folder, REDRAW_SUMMARIES);
}

static void
msg_child_exit_callback(pid, status, medit)
    int pid;
    void *status;
    MsgEdit *medit;
{
    struct spEvent *ev = spEvent_Create((long) 0, (long) 0,
					1,
					restore_edited_msg, medit);

    zmChildWaitPid(pid, (WAITSTATUS *)0, 0);
    spSend(ZmlIm, m_spIm_enqueueEvent, ev);
}
#endif /* ZM_CHILD_MANAGER */

int
gui_spawn_process(argv)
    char **argv;
{
    int pid;
    
#ifndef ZM_CHILD_MANAGER
    /* XXX Can't do this cleanly in SYSV */
    (void) signal(SIGCHLD, sigchldcatcher);
#endif /* ZM_CHILD_MANAGER  */
    if ((pid = zmChildVFork()) != 0) return pid;
    
    (void) signal(SIGINT, SIG_DFL);
    (void) signal(SIGQUIT, SIG_DFL);
#ifdef SIGTSTP
    /* don't want the user to ^Z this guy */
#ifdef _SC_JOB_CONTROL
    if (sysconf(_SC_JOB_CONTROL) >= 0)
#endif /* _SC_JOB_CONTROL */
	(void) signal(SIGTSTP, SIG_IGN);
#endif /* SIGTSTP */
    (void) signal(SIGPIPE, SIG_DFL);
    (void) closefileds_above(2);
#ifdef SYSV_SETPGRP
    if (setpgrp() == -1)
#else /* !SYSV_SETPGRP */
	if (setpgrp(0, getpid()) == -1)
#endif /* SYSV_SETPGRP */
	    error(SysErrWarning, "setpgrp");
#ifdef apollo
    setgid(getgid());
#endif /* apollo */
    execvp(*argv, argv);
    if (errno == ENOENT)
	_exit(-2);
    _exit(-1); return 0;
}

void
gui_edit_external(m_no, file, edit)
    int m_no;
    char *file, *edit;
{
    int argc;
    char **argv;
    struct dynstr cmd;
    int pid;

    dynstr_Init(&cmd);
    TRY {
	GetWineditor(&cmd);
	dynstr_Append(&cmd, file);
	argc = 0;
	argv =  mk_argv(dynstr_Str(&cmd), &argc, FALSE);
    } FINALLY {
	dynstr_Destroy(&cmd);
    } ENDTRY;

    if (!argv) {
	(void) unlink(file);
	return;
    }
    TRY {
	pid = gui_spawn_process(argv);
	if (pid == -1) {
	    error(SysErrWarning, *argv);
	} else {
	    MsgEdit *medit = (MsgEdit *) calloc(1, sizeof *medit);

	    medit->offset = msg[m_no]->m_offset;
	    medit->fldr = current_folder;
	    medit->file = savestr(file);
#ifdef ZM_CHILD_MANAGER
	    zmChildSetCallback(pid,
			       ZM_CHILD_EXITED, msg_child_exit_callback,
			       (GENERIC_POINTER_TYPE *) medit);
#endif /* ZM_CHILD_MANAGER  */
	    turnon(msg[m_no]->m_flags, EDITING);
	}
    } FINALLY {
	free_vec(argv);
    } ENDTRY;
}

void
trip_alarm(func)
    void (*func)();
{
    /* Stub */
}

AskAnswer
gui_ask(dflt, question)
    AskAnswer dflt;
    const char *question;
{
    struct notifier *n;
    int result;

    switch (dflt) {
      case WarnYes:
      case WarnNo:
	n = notifier_Create(question,
			    catgets(catalog, CAT_LITE, 762, "Warning"),
			    (1 << notifier_Yes) | (1 << notifier_No),
			    (dflt == WarnYes) ? notifier_Yes : notifier_No);
	break;
      case WarnOk:
      case WarnCancel:
	n = notifier_Create(question,
			    catgets(catalog, CAT_LITE, 762, "Warning"),
			    (1 << notifier_Ok) | (1 << notifier_Cancel),
			    (dflt == WarnOk) ? notifier_Ok : notifier_Cancel);
	break;
      case AskOk:
	n = notifier_Create(question,
			    catgets(catalog, CAT_LITE, 764, "Choice"),
			    (1 << notifier_Yes) | (1 << notifier_No),
			    notifier_Yes);
	break;
#ifdef PARTIAL_SEND
      case SendSplit:
      case SendWhole:
	n = notifier_Create(question,
			    catgets(catalog, CAT_LITE, 764, "Choice"),
			    ((1 << notifier_SendSplit) | (1 << notifier_SendWhole) |
			     (1 << notifier_Cancel)),
			    (((dflt == SendSplit) ? notifier_SendSplit :
			      (((dflt == SendWhole) ? notifier_SendWhole :
				notifier_Cancel)))));
	break;
#endif /* PARTIAL_SEND */
      default:
	n = notifier_Create(question,
			    catgets(catalog, CAT_LITE, 764, "Choice"),
			    ((1 << notifier_Yes) | (1 << notifier_No) |
			     (1 << notifier_Cancel)),
			    (((dflt == AskYes) ? notifier_Yes :
			      (((dflt == AskNo) ? notifier_No :
				notifier_Cancel)))));
	break;
    }
    result = spSend_i(n, m_dialog_interactModally);
    spoor_DestroyInstance(n);
    switch (result) {
      case notifier_Done:
      case notifier_Ok:
      case notifier_Yes:
#ifdef PARTIAL_SEND
      case notifier_SendSplit:
#endif /* PARTIAL_SEND */
	return (AskYes);
      case notifier_No:
#ifdef PARTIAL_SEND
      case notifier_SendWhole:
#endif /* PARTIAL_SEND */
	return (AskNo);
      default:
	return (AskCancel);
    }
}

void
gui_error(reason, message)
    PromptReason reason;
    const char *message;
{
    struct notifier *n = 0;

    ++zmlErrorLevel;
    TRY {
	if (reason == ForcedMessage)
	    wprint("%s\n", message);
	if ((reason != ForcedMessage)
	    || (CurrentDialog != (struct dialog *) MainDialog)
	    || !chk_option(VarMainPanes, "output")) {
	    switch (reason) {
	      case SysErrFatal:
	      case ZmErrFatal:
	      case UserErrFatal:
		n = notifier_Create(message,
				    catgets(catalog, CAT_LITE, 766, "Error"),
				    (1 << notifier_Bye), notifier_Bye);
		break;
	      default:
		n = notifier_Create(message,
				    ((reason == HelpMessage) ?
				     catgets(catalog, CAT_LITE, 17, "Help") :
				     catgets(catalog, CAT_LITE, 768, "Message")),
				    (1 << notifier_Ok), notifier_Ok);
		break;
	    }
	    spSend(n, m_dialog_interactModally);
	}
    } FINALLY {
	if (n) {
	    spoor_DestroyInstance(n);
	}
	--zmlErrorLevel;
    } ENDTRY;
}

#if defined( IMAP )
static int isaf = 0;

int
InSpecialAddFolder()
{
	isaf = 1;
}

int
OutSpecialAddFolder()
{
	isaf = 0;
}

int
SpecialAddFolder()
{
	return( isaf );
}
#endif

static int
special_addfolder()
{
    struct dynstr d;
    int retval = -1;
    int rw;

    dynstr_Init(&d);
    TRY {
	if (!gui_choose_one_aux(&d, catgets(catalog, CAT_LITE, 769, "Select folder to add"),
				getdir("+", 1), 0, 0,
				PB_FILE_BOX | PB_NOT_A_DIR | PB_MUST_EXIST,
				chooseone_OpenFolder, &rw)) {
#if defined( IMAP )
	    if ( using_imap && UseIMAP() ) {
		char	buf[ 1024 ];
		InSpecialAddFolder();
		if ( !strcmp( dynstr_Str(&d), "" ) )
			error(UserErrWarning,
			      catgets(catalog, CAT_LITE, 771, "Please specify a file"));
  		else {
			if ( rw )

/* XXX assumption is that slot 0 holds the valid prefix */

			sprintf( buf, "builtin open %s%s", open_folders[0]->imap_prefix,
				dynstr_Str(&d) );
			else
			sprintf( buf, "builtin open -r %s%s", open_folders[0]->imap_prefix,
				dynstr_Str(&d) );
	    		ZCommand(buf, zcmd_commandline);
		}
		OutSpecialAddFolder();
	    }
            else {
#endif
	    if (rw)
		dynstr_Replace(&d, 0, 0, "open ");
	    else
		dynstr_Replace(&d, 0, 0, "open -r ");
	    ZCommand(dynstr_Str(&d), zcmd_commandline);
#if defined( IMAP )
	    }
#endif
	    retval = 0;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (retval);
}

static int
special_newfolder()
{
    struct dynstr d;
    int retval = -1;

    dynstr_Init(&d);
    TRY {
	if (!gui_choose_one_aux(&d,
				catgets(catalog, CAT_LITE, 770, "Select folder or directory to create"),
				getdir("+", 1), 0, 0,
				PB_FILE_BOX | PB_NOT_A_DIR,
				chooseone_CreateFolder, 0)) {
	    int isdir = ZmGP_DontIgnoreNoEnt;

#if defined( IMAP )
	    if ( using_imap && UseIMAP() ) {
		if ( !strcmp( dynstr_Str(&d), "" ) )
			error(UserErrWarning,
			      catgets(catalog, CAT_LITE, 771, "Please specify a file"));
		else if ( FolderByName( dynstr_Str(&d) ) )
			error(UserErrWarning,
			      catgets(catalog, CAT_LITE, 771, "\"%s\" already exists"), dynstr_Str(&d));
  		else {
			if ( zimap_newfolder( dynstr_Str(&d), GetMakeDir() ) )
				AddFolder( dynstr_Str(&d), GetDelimiter(), 
					( GetMakeDir() ? LATT_NOSELECT : LATT_NOINFERIORS ) );
		}
	    }
	    else {
#endif
	    getpath(dynstr_Str(&d), &isdir);
	    if (isdir == ZmGP_Dir) {
		error(UserErrWarning,
		      catgets(catalog, CAT_LITE, 771, "\"%s\" already exists and is a directory"),
		      dynstr_Str(&d));
	    } else if (((isdir == ZmGP_Error) && (errno == ENOENT))
		       || ((isdir == ZmGP_File)
			   && (ask(WarnNo,
				   zmVaStr(catgets(catalog, CAT_LITE, 772, "%s already exists.  Make it empty?"),
					   dynstr_Str(&d))) == AskYes))) {
		FILE *fp = mask_fopen(dynstr_Str(&d), "w");

		if (!fp) {
		    error(SysErrWarning, dynstr_Str(&d));
		} else {
		    fclose(fp);
		    retval = 0;
		}
	    }
#if defined( IMAP )
	    }
#endif
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (retval);
}

static int
special_save()
{
    struct dynstr d;
    int retval = -1;

    dynstr_Init(&d);
    TRY {
	if (!dyn_choose_one(&d, catgets(catalog, CAT_LITE, 773, "Select folder in which to save messages"),
			    getdir("+", 1), 0, 0,
			    PB_FILE_BOX | PB_NOT_A_DIR)) {
#if defined( IMAP )
	    if ( using_imap && UseIMAP() ) {
		char	buf[ 1024 ];
		if ( !strcmp( dynstr_Str(&d), "" ) )
			error(UserErrWarning,
			      catgets(catalog, CAT_LITE, 771, "Please specify a file"));
  		else {
			sprintf( buf, "save %s%s", current_folder->imap_prefix,
				dynstr_Str(&d) );
	    		ZCommand(buf, zcmd_commandline);
		}
	    }
            else {
#endif

	    dynstr_Replace(&d, 0, 0, "save ");
	    ZCommand(dynstr_Str(&d), zcmd_commandline);
#if defined( IMAP )
	    }
#endif
	    retval = 0;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (retval);
}

static int
special_dynhdrs()
{
    struct zmlcomposeframe *self = (struct zmlcomposeframe *) spIm_view(ZmlIm);
    int retval = -1;

    if (!spoor_IsClassMember(self, (struct spClass *) zmlcomposeframe_class))
	return (-1);
    ZCommand("\\set compose_state -= edit_headers", zcmd_ignore);
    if (!zmlcomposeframe_dh(self))
	zmlcomposeframe_dh(self) = dynhdrs_NEW();
    spSend(zmlcomposeframe_dh(self), m_dialog_interactModally);
    return (0);
}

static int
special_rename()
{
    struct dynstr d, d2;
    int retval = -1;

    dynstr_Init(&d);
    dynstr_Init(&d2);
    TRY {
	if (!gui_choose_one_aux(&d,
				catgets(catalog, CAT_LITE, 774, "Select folder to rename and enter its new name"),
				getdir("+", 1), 0, 0,
				PB_FILE_BOX | PB_NOT_A_DIR | PB_MUST_EXIST,
				chooseone_RenameFolder, &d2)) {
	    dynstr_Set(&d, quotezs(dynstr_Str(&d), 0));
	    dynstr_Set(&d2, quotezs(dynstr_Str(&d2), 0));
#if defined( IMAP )
	    if ( using_imap && UseIMAP() ) {
		if ( zimap_rename( dynstr_Str(&d), dynstr_Str(&d2) ) )
        		ChangeName( dynstr_Str(&d), dynstr_Str(&d2) );
		else 
			error(UserErrWarning,
			    catgets( catalog, CAT_LITE, 743, "Selected file already exists or error renaming item" ));
	    }
	    else {
#endif
		    ZCommand(zmVaStr("\\rename %s %s",
			     dynstr_Str(&d), dynstr_Str(&d2)),
			     zcmd_ignore);
	    }
	    retval = 0;
	}
    } FINALLY {
	dynstr_Destroy(&d);
	dynstr_Destroy(&d2);
    } ENDTRY;
    return (retval);
}

static int
special_attach()
{
    struct attachlist *self;

    TRY {
	self = attachlist_NEW();
	spSend_i(self, m_dialog_interactModally);
    } EXCEPT(attachlist_err_BadContext) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 775, "Cannot open that dialog in this context"));
	self = 0;
    } EXCEPT(attachlist_err_NoAttachments) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 776, "No attachments"));
	self = 0;
    } FINALLY {
	if (self)
	    spoor_DestroyInstance(self);
    } ENDTRY;
    return (0);
}

static int
special_attachfile()
{
    struct dynstr d;
    int retval = 0;

    dynstr_Init(&d);
    TRY {
	if (dyn_choose_one(&d, catgets(catalog, CAT_LITE, 777, "Select a file to attach"),
			   0, 0, 0,
#if defined( IMAP )
			   PB_NOIMAP | PB_FILE_BOX | PB_NOT_A_DIR | PB_MUST_EXIST)) {
#else
			   PB_FILE_BOX | PB_NOT_A_DIR | PB_MUST_EXIST)) {
#endif
	    retval = -1;
	} else {
	    struct attachtype *self = attachtype_Create(0);
	    char *type, *encoding;
	    struct dynstr d2;

	    dynstr_Init(&d2);
	    TRY {
		spSend(self, m_attachtype_select, autotype(dynstr_Str(&d)));
		if (spSend_i(self,
			     m_dialog_interactModally) != dialog_Cancel) {
		    spSend(spView_observed(attachtype_comment(self)),
			   m_spText_appendToDynstr, &d2, 0, -1);
		    ZCommand(zmVaStr("compcmd attach '%s' '%s' '%s' '%s'",
				     dynstr_Str(&d),
				     attachtype_type(self),
				     attachtype_encoding(self),
				     quotezs(dynstr_Str(&d2), '\'')),
			     zcmd_commandline);
		    if (!atoi(get_var_value(VarStatus)))
			spSend(ZmlIm, m_spIm_showmsg,
			       zmVaStr(catgets(catalog, CAT_LITE, 778, "Attached %s"), dynstr_Str(&d)),
			       15, 2, 5);
		} else {
		    retval = -1;
		}
	    } FINALLY {
		spoor_DestroyInstance(self);
		dynstr_Destroy(&d2);
	    } ENDTRY;
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
    return (retval);
}

static int
special_searchreplace()
{
    struct tsearch *self = tsearch_NEW();

    TRY {
	if (CurrentDialog) {
	    if (spoor_IsClassMember(CurrentDialog,
				    (struct spClass *) zmlmsgframe_class)) {
		spSend(self, m_tsearch_setText,
		       spView_observed(zmlmsgframe_body(CurrentDialog)));
	    } else if (spoor_IsClassMember(CurrentDialog,
					   (struct spClass *) zmlcomposeframe_class)) {
		spSend(self, m_tsearch_setText,
		       spView_observed(zmlcomposeframe_body(CurrentDialog)));
	    }
	}
	spSend(self, m_dialog_interactModally);
    } FINALLY {
	spoor_DestroyInstance(self);
    } ENDTRY;
    return (0);
}

static int
special_attachnew()
{
    struct attachtype *self;
    int retval = 0;

    if (!spoor_IsClassMember(spIm_view(ZmlIm), (struct spClass *) zmlcomposeframe_class)) {
	error(UserErrWarning,
	      catgets(catalog, CAT_LITE, 775, "Cannot open that dialog in this context"));
	return (-1);
    }
    self = attachtype_Create(1);
    TRY {
	if (spSend_i(self, m_dialog_interactModally) == dialog_Cancel) {
	    retval = -1;
	} else {
	    char name[1 + MAXPATHLEN];
	    FILE *errfp;
	    char *errnm = NULL;
	    AttachProg *ap;
	    int err;

	    new_attach_filename(zmlcomposeframe_comp(spIm_view(ZmlIm)),
				name);
	    ap = coder_prog(1, 0, attachtype_type(self), name, "x", 0);
	    if (errfp = open_tempfile("err", &errnm))
		fclose(errfp);
	    popen_coder(ap, name, errnm, "x");
	    err = handle_coder_err(ap->exitStatus, ap->program, errnm);
	    unlink(errnm);
	    xfree(errnm);
	    if (!err) {
		struct dynstr d;

		dynstr_Init(&d);
		TRY {
		    spSend(spView_observed(attachtype_comment(self)),
			   m_spText_appendToDynstr, &d, 0, -1);
		    ZCommand(zmVaStr("compcmd attach '%s' '%s' '%s' '%s'",
				     name, attachtype_type(self),
				     attachtype_encoding(self),
				     quotezs(dynstr_Str(&d), '\'')),
			     zcmd_commandline);
		    if (!atoi(get_var_value(VarStatus)))
			spSend(ZmlIm, m_spIm_showmsg,
			       catgets(catalog, CAT_LITE, 780, "Attached new file"), 15, 2, 5);
		} FINALLY {
		    dynstr_Destroy(&d);
		} ENDTRY;
	    }
	}
    } FINALLY {
	spoor_DestroyInstance(self);
    } ENDTRY;
    return (retval);
}

struct {
    char *name;
    struct dialog **dialog;
    enum {
	d_modal, d_nonmodal, d_special
    } type;
    int (*special)();
} dialogNames[] = {
    { "addfolder",
	  0, d_special, special_addfolder },
    { "aliases",
	  (struct dialog **) &AliasDialog, d_nonmodal, 0 },
    { "attach",
	  0, d_special, special_attach },
    { "attachfile",
	  0, d_special, special_attachfile },
    { "attachnew",
	  0, d_special, special_attachnew },
    { "browser",
	  (struct dialog **) &AddrbrowseDialog, d_modal, 0 },
    { "charset",
	  (struct dialog **) &CharsetDialog, d_modal, 0 },
    { "compaliases",
	  (struct dialog **) &CompAliasesDialog, d_modal, 0 },
    { "compoptions",
	  (struct dialog **) &CompOptionsDialog, d_modal, 0 },
    { "dates",
	  (struct dialog **) &DatesearchDialog, d_modal, 0 },
    { "dynamichdrs",
	  0, d_special, special_dynhdrs },
    { "envelope",
	  (struct dialog **) &EnvelopeDialog, d_nonmodal, 0 },
    { "headers",
	  (struct dialog **) &HeadersDialog, d_nonmodal, 0 },
    { "helpindex",
	  (struct dialog **) &HelpIndexDialog, d_modal, 0 },
    { "main",
	  (struct dialog **) &MainDialog, d_nonmodal, 0 },
    { "message",
	  (struct dialog **) &MessageDialog, d_nonmodal, 0 },
    { "multikey",
	  (struct dialog **) &MultikeyDialog, d_modal, 0 },
    { "newfolder",
	  0, d_special, special_newfolder },
    { "printer",
	  (struct dialog **) &PrinterDialog, d_modal, 0 },
    { "renamefolder",
	  0, d_special, special_rename },
    { "save",
	  0, d_special, special_save },
    { "search",
	  (struct dialog **) &SearchDialog, d_modal, 0 },
    { "searchreplace",
          0, d_special, special_searchreplace },
    { "sort",
	  (struct dialog **) &SortDialog, d_modal, 0 },
    { "templates",
	  (struct dialog **) &TemplateDialog, d_modal, 0 },
    { "variables",
	  (struct dialog **) &VariablesDialog, d_nonmodal, 0 },
    { 0, 0, 0, 0 }
};

int
gui_dialog(name)
    char *name;
{
    int i;

    if (!name || !*name)
	return (-1);
    for (i = 0; dialogNames[i].name; ++i) {
	if (!ci_strcmp(name, dialogNames[i].name))
	    break;
    }
    if (dialogNames[i].name) {
	switch (dialogNames[i].type) {
	  case d_modal:
	    spSend(Dialog(dialogNames[i].dialog), m_dialog_interactModally);
	    break;
	  case d_nonmodal:
	    spSend(Dialog(dialogNames[i].dialog), m_dialog_enter);
	    break;
	  case d_special:
	    return ((*(dialogNames[i].special))());
	}
	return (0);
    } else if (!strcmp(name, "help")) {
	ZCommand("help", zcmd_ignore);
	return (0);
    } else {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 781, "dialog: unknown dialog name \"%s\""),
	      name);
	return (-1);
    }
}

int
zm_dialog(argc, argv, list)
    int argc;
    char **argv;
    msg_group *list;
{
    int close = 0 , rc , cat = -1;

    if (argv[2] != NULL)
      if (!strncmp(argv[2], "-C", 2))
        if (argv[3] != NULL)
          cat = atoi(argv[3]);
    for (argc--, argv++; *argv; ++argv, --argc) {
	if (!strcmp(*argv, "-close")) {
	    close = 1;
	} else if (!strncmp(*argv, "-i", 2)
		   || !strncmp(*argv, "-I", 2)) {
	    wprint(catgets(catalog, CAT_LITE, 782, "dialog: ignoring option \"%s\"\n"), *argv);
	} else if (**argv == '-') {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 783, "dialog: unrecognized option \"%s\""),
		  *argv);
	    return (-1);
	} else {
	    break;
	}
    }
    if (close) {
	spSend(CurrentDialog, m_dialog_deactivate, dialog_Close);
    } else {
	rc = gui_dialog(*argv);
        if (cat > 0)
          if (strncmp(*argv,"Variables",9) == 0)
            select_category_num(cat);
	return (rc);
    }
}

/*
 * update a button.  if is_cb is non-NULL, we must be being called
 * as a callback routine, to update button sensitivities and values.
 * otherwise, the labels, etc. may have changed.
 */
void
gui_update_button(button, is_cb, oldb)
    ZmButton button, oldb;
    ZmCallbackData is_cb;
{
    struct blistCopyElt *bcep;
    ZmButtonList blist = GetButtonList(ButtonParent(button));
    int i;

    dlist_FOREACH(&(blist->copylist), struct blistCopyElt, bcep, i) {
	spSend(bcep->dialog, m_dialog_updateZbutton,
	       button, blist, is_cb, oldb, 0);
    }
    if (!is_cb)
	ZmCallbackCallAll(BListName(blist), ZCBTYPE_MENU, 0, NULL);
}

void
gui_print_status(str)
    const char *str;
{
    if (istool == 2)
	spSend(ZmlIm, m_spIm_showmsg, str, 15, 1, 0);
}

#define TEXTEDIT_BUILTIN   (1 << 0)
#define TEXTEDIT_FORMAT    ((1 << 1) | TEXTEDIT_SELECTION)
#define TEXTEDIT_VEC       (1 << 2)
#define TEXTEDIT_SELECTION (1 << 3)
#define TEXTEDIT_ARG       (1 << 4)

enum {
    TexteditSetItem = 1,
    TexteditPipe,
    TexteditIndent,
    TexteditUnindent,
    TexteditFill,
    TexteditDeleteAll,
    TexteditGetSelectionPosition,
    TexteditGetCursorPosition,
    TexteditGetText,
    TexteditSaveToFile,
    TexteditSetSelectionPosition,
    TexteditSetCursorPosition,
    TexteditGetSelection
};

struct {
    char *name;
    unsigned long flags;
    int op;
} textedit_cmds[] = {
    { "set-item", TEXTEDIT_ARG, TexteditSetItem },
    { "text-indent", TEXTEDIT_FORMAT | TEXTEDIT_ARG | TEXTEDIT_VEC,
	  TexteditIndent },
    { "text-unindent", TEXTEDIT_FORMAT | TEXTEDIT_VEC,
	  TexteditUnindent },
    { "text-fill", TEXTEDIT_FORMAT | TEXTEDIT_VEC, TexteditFill },
    { "text-pipe", TEXTEDIT_FORMAT | TEXTEDIT_ARG, TexteditPipe },
    { "text-delete-all", 0, TexteditDeleteAll },
    { "text-get-cursor-position", TEXTEDIT_ARG, TexteditGetCursorPosition },
    { "text-set-cursor-position", TEXTEDIT_ARG, TexteditSetCursorPosition },
    { "text-get-selection-position", TEXTEDIT_SELECTION | TEXTEDIT_ARG,
	  TexteditGetSelectionPosition },
    { "text-get-selection", TEXTEDIT_SELECTION | TEXTEDIT_ARG,
	  TexteditGetSelection },
    { "text-get-text", TEXTEDIT_ARG, TexteditGetText },
    { "text-save-to-file", TEXTEDIT_ARG, TexteditSaveToFile },
    { "text-insert", TEXTEDIT_BUILTIN | TEXTEDIT_ARG, 0 },
    { "text-next-line", TEXTEDIT_BUILTIN, 0 },
    { "text-previous-line", TEXTEDIT_BUILTIN, 0 },
    { "text-scroll-up", TEXTEDIT_BUILTIN, 0 },
    { "text-scroll-down", TEXTEDIT_BUILTIN, 0 },
    { "text-forward-word", TEXTEDIT_BUILTIN, 0 },
    { "text-start-selecting", TEXTEDIT_BUILTIN, 0 },
    { "text-stop-selecting", TEXTEDIT_BUILTIN, 0 },
    { "text-resume-selecting", TEXTEDIT_BUILTIN, 0 },
    { "text-paste", TEXTEDIT_BUILTIN, 0 },
    { "text-set-selection-position", TEXTEDIT_ARG,
	  TexteditSetSelectionPosition },
    { "text-end", TEXTEDIT_BUILTIN, 0 },
    { "text-backward-char", TEXTEDIT_BUILTIN, 0 },
    { "text-forward-char", TEXTEDIT_BUILTIN, 0 },
    { "text-delete-backward-char", TEXTEDIT_BUILTIN, 0 },
    { "text-delete-forward-char", TEXTEDIT_BUILTIN, 0 },
    { "text-beginning-of-line", TEXTEDIT_BUILTIN, 0 },
    { "text-end-of-line", TEXTEDIT_BUILTIN, 0 },
    { "text-next-page", TEXTEDIT_BUILTIN, 0 },
    { "text-previous-page", TEXTEDIT_BUILTIN, 0 },
    { "text-delete-to-end-of-line", TEXTEDIT_BUILTIN, 0 },
    { "text-delete-to-beginning-of-line", TEXTEDIT_BUILTIN, 0 },
    { "text-beginning", TEXTEDIT_BUILTIN, 0 },
    { "text-backward-word", TEXTEDIT_BUILTIN, 0 },
    { "text-delete-backward-word", TEXTEDIT_BUILTIN, 0 },
    { "text-delete-forward-word", TEXTEDIT_BUILTIN, 0 },
    { "text-open-line", TEXTEDIT_BUILTIN, 0 },
    { "text-deselect", TEXTEDIT_BUILTIN, 0 },
    { "text-cut-selection", TEXTEDIT_BUILTIN, 0 },
    { "text-copy-selection", TEXTEDIT_BUILTIN, 0 },
    { "text-clear-selection", TEXTEDIT_BUILTIN, 0 },
    { "text-select-all", TEXTEDIT_BUILTIN, 0 },
    { "inputfield-accept", TEXTEDIT_BUILTIN, 0 },
    { 0, 0, 0 }
};

static int
zm_textedit(argc, argv)
    int argc;
    char **argv;
{
    static struct dynstr widgetname;
    static int initialized = 0;
    struct spoor *obj;
    char *cmd, *arg = 0;
    int cno;

    if (!initialized) {
	dynstr_Init(&widgetname);
	initialized = 1;
    }
    if (!argc || !*argv || !*++argv)
	return (-1);
    cmd = *argv++;
    for (cno = 0; textedit_cmds[cno].name; ++cno) {
	if ((!strcmp(textedit_cmds[cno].name, cmd))
	    || (!strncmp(textedit_cmds[cno].name, "text-", 5) &&
		!strcmp(textedit_cmds[cno].name+5, cmd)))
	    break;
    }
    if (!textedit_cmds[cno].name) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 784, "Bad textedit command: %s"), cmd);
	return (-1);
    }
    if (ison(textedit_cmds[cno].flags, TEXTEDIT_ARG))
	if (!(arg = *argv++)) {
	    error(UserErrWarning, catgets(catalog, CAT_LITE, 785, "Argument expected for %s"), cmd);
	    return (-1);
	}
    if (textedit_cmds[cno].op == TexteditSetItem) {
	dynstr_Set(&widgetname, arg);
	return (0);
    }
    if ((!(obj = find_context_instance(dynstr_Str(&widgetname), 0)))
	|| !spoor_IsClassMember(obj, (struct spClass *) spTextview_class)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 786, "No text area named %s"),
	      dynstr_Str(&widgetname));
	return (-1);
    }
    if (textedit_cmds[cno].flags & TEXTEDIT_BUILTIN) {
	TRY {
	    spSend(obj, m_spView_invokeInteraction,
		   textedit_cmds[cno].name, 0, arg, 0);
	} EXCEPT(spView_FailedInteraction) {
	    /*
	     * Even if the interaction failed, the textedit command
	     * still succeeds.  This may seem counterintuitive but
	     * that's the way the other platforms work too.
	     */
	} ENDTRY;
	return (0);
    } else if (textedit_cmds[cno].flags & TEXTEDIT_SELECTION) {
	int start, end;

	if (!spTextview_selection(obj, &start, &end)
	    || (end < start)) {
	    end = -1;
	    start = spText_markPos((struct spText *) spView_observed(obj),
				   spTextview_textPosMark(obj));
	}
	if ((textedit_cmds[cno].flags & TEXTEDIT_FORMAT) == TEXTEDIT_FORMAT) {
	    struct dynstr d;
	    char *out = 0, **vec, *col;

	    if (end < 0) {
		error(UserErrWarning, catgets(catalog, CAT_LITE, 787, "No text selected"));
		return (-1);
	    }
	    dynstr_Init(&d);
	    TRY {
		spSend(spView_observed(obj), m_spText_appendToDynstr,
		       &d, start, end - start + 1);
		if (textedit_cmds[cno].flags & TEXTEDIT_VEC)
		    vec = strvec(dynstr_Str(&d), "\n", 0);
		switch (textedit_cmds[cno].op) {
		  case TexteditPipe:
		    out = format_pipe_str(dynstr_Str(&d), arg);
		    break;
		  case TexteditIndent:
		    if (vec)
			out = format_indent_lines(vec, arg);
		    break;
		  case TexteditUnindent:
		    if (vec)
			out = format_unindent_lines(vec,
						    format_prefix_length(vec,
									 1));
		    break;
		  case TexteditFill:
		    if (!(col = get_var_value(VarWrapcolumn))
			|| !*col)
			col = "72";
		    if (vec)
			out = format_fill_lines(vec, atoi(col) - 1);
		    break;
		}
		if (out) {
		    spSend(spView_observed(obj), m_spText_delete,
			   start, end - start + 1);
		    spSend(spView_observed(obj), m_spText_insert,
			   start, -1, out, spText_mNeutral);
		}
	    } FINALLY {
		dynstr_Destroy(&d);
		if (out)
		    free(out);
	    } ENDTRY;
	    return (0);
	}
	switch (textedit_cmds[cno].op) {
	  case TexteditGetSelectionPosition:
	    if (argv && *argv) {
		if (end < 0) {
		    ZCommand(zmVaStr("set %s = %d", *argv, start),
			     zcmd_commandline);
		} else {
		    ZCommand(zmVaStr("set %s = %d", *argv, end),
			     zcmd_commandline);
		}
	    }
	    ZCommand(zmVaStr("set %s = %d", arg, start), zcmd_commandline);
	    break;
	  case TexteditGetSelection: {
	      struct dynstr d;

	      if (end < 0) {
		  ZCommand(zmVaStr("set %s = ''", arg), zcmd_commandline);
	      } else {
		  dynstr_Init(&d);
		  TRY {
		      spSend(spView_observed(obj), m_spText_appendToDynstr,
			     &d, start, end - start + 1);
		      ZCommand(zmVaStr("set %s = '%s'", arg,
				       quotezs(dynstr_Str(&d), '\'')),
			       zcmd_commandline);
		  } FINALLY {
		      dynstr_Destroy(&d);
		  } ENDTRY;
	      }
	    }
	    break;
	}
    } else {
	switch (textedit_cmds[cno].op) {
	  case TexteditGetCursorPosition:
	    ZCommand(zmVaStr("set %s = %d", arg,
			     spText_markPos((struct spText *)
					    spView_observed(obj),
					    spTextview_textPosMark(obj))),
		     zcmd_commandline);
	    break;
	  case TexteditSetCursorPosition:
	    spSend(spView_observed(obj), m_spText_setMark,
		   spTextview_textPosMark(obj), atoi(arg));
	    break;
	  case TexteditDeleteAll:
	    spSend(spView_observed(obj), m_spText_delete, 0, -1);
	    break;
	  case TexteditGetText: {
	      struct dynstr d;

	      dynstr_Init(&d);
	      TRY {
		  spSend(spView_observed(obj), m_spText_appendToDynstr,
			 &d, 0, -1);
		  ZCommand(zmVaStr("set %s = '%s'", arg,
				   quotezs(dynstr_Str(&d), '\'')),
			   zcmd_commandline);
	      } FINALLY {
		  dynstr_Destroy(&d);
	      } ENDTRY;
	    }
	    break;
	  case TexteditSaveToFile:
	    spSend(obj, m_spTextview_writeFile, arg);
	    break;
	  case TexteditSetSelectionPosition:
	    spSend(spView_observed(obj), m_spText_setMark,
		   spTextview_textPosMark(obj), atoi(arg));
	    if (argv && *argv) {
		spSend(obj, m_spView_invokeInteraction,
		       "text-start-selecting", 0, 0, 0);
		spSend(spView_observed(obj), m_spText_setMark,
		       spTextview_textPosMark(obj), atoi(*argv));
		spSend(obj, m_spView_invokeInteraction,
		       "text-stop-selecting", 0, 0, 0);
	    }
	    break;
	}
	return (0);
    }
}

DEFINE_EXCEPTION(usage, "usage");
DEFINE_EXCEPTION(bad_instance, "bad-instance");

static int
zm_interact(argc, argv)
    int argc;
    char **argv;
{
    const char * const command = **argv ? *argv++ : "interact";
    char *interaction = 0;
    int status = 0;

    TRY {
	struct spoor *recipient;
	struct spWidgetInfo *winfo;
	const char *name;
	char *arguments;
	
	ASSERT(name = *argv++, usage, "zm_interact");
	ASSERT(interaction = *argv++, usage, "zm_interact");
	arguments = joinv(0, argv++, " ");
	
	/* XXX casting away const */
	ASSERT(recipient = find_context_instance(name, 0),
	       bad_instance, (VPTR) name);

	spSend(recipient, m_spView_invokeInteraction,
	       interaction, 0, arguments, 0);
	
    } EXCEPT(usage) {
	error(UserErrWarning,
	      catgets(catalog, CAT_LITE, 910, "Usage: %s name interaction [arguments ...]"),
	      command);
	status = -1;

    } EXCEPT(bad_instance) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 911, "No widget named \"%s\""),
	      (char *) except_GetExceptionValue());
	status = -2;

    } EXCEPT(spView_FailedInteraction) {
	status = 1;
	
    } EXCEPT(spView_NoInteraction) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 912, "No interaction named \"%s\""),
	      (char *) except_GetExceptionValue());
	status = 2;
	
    } FINALLY {
	if (interaction) free(interaction);

    } ENDTRY;

    return status;
}
