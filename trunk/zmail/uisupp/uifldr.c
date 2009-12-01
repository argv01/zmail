#include <zmail.h>
#include <general.h>
#include <uifldr.h>
#include <fsfix.h>

#ifndef lint
static char	uifldr_rcsid[] =
    "$Id: uifldr.c,v 1.14 1994/06/02 19:53:03 pf Exp $";
#endif

#define MAX_SHORT_NAME_LENGTH 40

static void shorten_name P ((uifolder_t *));
static void mapfldr P ((uifolderlist_t *, int, int));
static void disambiguate P ((uifolder_t *));
static int sortfunc P ((CVPTR, CVPTR));
static zmBool makenames P ((uifolderlist_t *));
static zmBool makename P ((uifolder_t *, zmBool));
static void restore_name P ((uifolder_t *));

void
uifolderlist_Add(flist, fldr)
uifolderlist_t *flist;
msg_folder *fldr;
{
    uifolder_t uif;
    int n;

    /* make sure not already added */
    if (uifolderlist_Get(flist, fldr))
	return;
    if (isoff(fldr->mf_flags, CONTEXT_IN_USE))
	return;
    bzero((VPTR) &uif, sizeof uif);
    uif.fno = fldr->mf_number;
    uif.fldr = fldr;
    if (fldr == &spool_folder)
	turnon(uif.flags, uifolder_Spool);
    makename(&uif, False);
    n = glist_Add(&flist->flist, &uif);
    mapfldr(flist, uif.fno, n);
    flist->changed = True;
}

#ifdef NOT_NOW
/* need to think about reopen folders dialog... */

void
uifolderlist_Close(oflist, cflist, fldr)
msg_folder *fldr;
uifolderlist_t *oflist, *cflist;
{
    uifolder_t *uif = uifolderlist_Get(oflist, fldr);

    if (!uif) return;
    uif->fno = 0;
    uif->filename = savestr(uif->fldr->mf_name);
    uif->fldr = NULL;
    glist_Add(&cflist->flist, uif);
    uifolderlist_Remove(oflist, uif);
}
#endif /* NOT_NOW */

static zmBool
makename(uif, force)
uifolder_t *uif;
zmBool force;
{
    char buf[200];
    msg_folder *fldr = uifolder_GetFolder(uif);
    
    if (uif->long_name) return True;
    if ((force && !fldr->mf_name) || ison(fldr->mf_flags, TEMP_FOLDER)) {
	sprintf(buf, catgets(catalog, CAT_UISUPP, 1, "Folder #%d"), fldr->mf_number);
	uif->long_name = savestr(buf);
	uif->short_name = savestr(buf);
	uif->basename = savestr(buf);
	return True;
    }
    if (uifolder_IsSpool(uif)) {
	get_spool_name(buf);
	uif->long_name = savestr(buf);
	uif->short_name = savestr(buf);
	uif->basename = savestr(buf);
	return True;
    }
    if (!fldr->mf_name) return False;
    uif->long_name = savestr(basename(fldr->mf_name));
    uif->short_name = savestr(uif->long_name);
    uif->basename = savestr(uif->long_name);
    shorten_name(uif);
    return True;
}

static zmBool
makenames(fl)
uifolderlist_t *fl;
{
    int i;
    uifolder_t *uif;
    
    uifolderlist_FOREACH(fl, uif, i)
	if (!makename(uif, True))
	    return False;
    return True;
}

void
uifolderlist_Remove(flist, fldr)
uifolderlist_t *flist;
msg_folder *fldr;
{
    uifolder_t *uif = uifolderlist_Get(flist, fldr);

    if (!uif) return;
    xfree(uif->long_name);
    xfree(uif->short_name);
    xfree(uif->basename);
    glist_Remove(&flist->flist,
	gintlist_Nth(&flist->map, fldr->mf_number));
    mapfldr(flist, fldr->mf_number, -1);
    flist->changed = True;
}

uifolder_t *
uifolderlist_Get(flist, fldr)
uifolderlist_t *flist;
msg_folder *fldr;
{
    int no = fldr->mf_number;
    int mno;
    
    if (gintlist_Length(&flist->map) <= no)
	return NULL;
    mno = gintlist_Nth(&flist->map, no);
    if (mno < 0) return NULL;
    return glist_Nth(&flist->flist, mno);
}

int
uifolderlist_GetIndex(flist, fldr)
uifolderlist_t *flist;
msg_folder *fldr;
{
    int no = fldr->mf_number;
    
    if (gintlist_Length(&flist->map) <= no)
	return -1;
    return gintlist_Nth(&flist->map, no);
}

uifolder_t *
uifolderlist_GetAt(flist, no)
uifolderlist_t *flist;
int no;
{
    return (uifolder_t *) glist_Nth(&flist->flist, no);
}

void
uifolderlist_Init(flist)
uifolderlist_t *flist;
{
    int i;

    glist_Init(&flist->flist, sizeof(uifolder_t), 10);
    gintlist_Init(&flist->map, 10);
    for (i = 0; i < folder_count; i++) {
	if (!open_folders[i]) continue;
	if (!open_folders[i]->mf_name)
	    continue;
	uifolderlist_Add(flist, open_folders[i]);
    }
    flist->changed = True;
    flist->seqno = 123;
    uifolderlist_Sort(flist);
}

int
uifolderlist_Sort(flist)
uifolderlist_t *flist;
{
    char *lastname;
    struct glist *gl = &flist->flist;
    int i, glen = glist_Length(gl);

    if (!flist->changed) return flist->seqno;
    makenames(flist);
    glist_Sort(gl, sortfunc);
    for (i = 0; i < glen-1; i++) {
	uifolder_t *a = uifolderlist_GetAt(flist, i);
	uifolder_t *b = uifolderlist_GetAt(flist, i+1);
	if (strcmp(a->basename, b->basename)) {
	    if (ison(a->flags, uifolder_Disambiguated))
		restore_name(a);
	    if (i == glen-2 && ison(b->flags, uifolder_Disambiguated))
		restore_name(b);
	    continue;
	}
	b = NULL;
	lastname = a->basename;
	disambiguate(a);
	for (i++; i < glen; i++) {
	    a = uifolderlist_GetAt(flist, i);
	    if (strcmp(a->basename, lastname))
		break;
	    disambiguate(a);
	}
    }
    for (i = 0; i < glen; i++) {
	uifolder_t *a = uifolderlist_GetAt(flist, i);
	mapfldr(flist, a->fno, i);
    }
    flist->changed = False;
    flist->seqno++;
    return flist->seqno;
}

static int
sortfunc(va, vb)
CVPTR va, vb;
{
    const uifolder_t *a = va, *b = vb;

    return ci_strcmp(a->short_name, b->short_name);
}

void
fname_ExtractDir(buf, name)
char *buf;
const char *name;
{
    const char *s = name+strlen(name)-1;
    int len;

    for (; s > name; s--)
	if (is_dsep(*s)) {
	    len = s-name;
	    strncpy(buf, name, len);
	    buf[len] = '\0';
	    return;
	}
    if (is_dsep(*name)) {
	buf[0] = *name;
	buf[1] = '\0';
	return;
    }
    strcpy(buf, name);
}

static void
disambiguate(a)
uifolder_t *a;
{
    char buf[MAXPATHLEN], *s;

    if (ison(a->flags, uifolder_Disambiguated))
	return;
    fname_ExtractDir(buf, uifolder_GetFolderFilename(a));
    s = zmVaStr("%s (%s)", a->basename, buf);
    str_replace(&a->long_name, s);
    str_replace(&a->short_name, s);
    shorten_name(a);
    turnon(a->flags, uifolder_Disambiguated);
}

static void
restore_name(a)
uifolder_t *a;
{
    str_replace(&a->long_name, a->basename);
    str_replace(&a->short_name, a->basename);
    shorten_name(a);
}

static void
shorten_name(uf)
uifolder_t *uf;
{
    if (strlen(uf->short_name) <= MAX_SHORT_NAME_LENGTH) return;
    uf->short_name[MAX_SHORT_NAME_LENGTH] = '\0';
}

static void
mapfldr(flist, no, ind)
uifolderlist_t *flist;
int no, ind;
{
    int i = gintlist_Length(&flist->map);
    for (; i < no; i++)
	gintlist_Set(&flist->map, i, -1);
    gintlist_Set(&flist->map, no, ind);
}

zmBool
uifolder_ChangeTo(uif)
uifolder_t *uif;
{
    char buf[80];

    sprintf(buf, "folder #%d", uifolder_GetFolderNo(uif));
    return uiscript_Exec(buf, 0) == 0;
}

char *
uifolder_GetFolderFilename(uif)
uifolder_t *uif;
{
    if (uif->filename)
	return uif->filename;
    return uifolder_GetFolder(uif)->mf_name;
}
