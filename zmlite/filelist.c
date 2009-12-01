
/*
 * $RCSfile: filelist.c,v $
 * $Revision: 2.45 $
 * $Date: 1998/12/07 23:56:32 $
 * $Author: schaefer $
 */

#include <spoor.h>
#include <spoor/splitv.h>
#include <filelist.h>

#include <zmlutil.h>

#include <spoor/button.h>
#include <spoor/buttonv.h>
#include <spoor/text.h>
#include <spoor/textview.h>
#include <spoor/cmdline.h>
#include <spoor/wrapview.h>
#include <spoor/im.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <spoor/wclass.h>

#include <zmail.h>
#include <zmlite.h>

#include "catalog.h"

#ifndef lint
static const char filelist_rcsid[] =
    "$Id: filelist.c,v 2.45 1998/12/07 23:56:32 schaefer Exp $";
#endif /* lint */

struct spWclass *filelist_class = (struct spWclass *) 0;

int m_filelist_setDirectory;
int m_filelist_setPrompt;
int m_filelist_setChoice;
int m_filelist_fullpath;
int m_filelist_setFile;
int m_filelist_setDefault;
#if defined( IMAP )
int m_filelist_setIMAP;
#endif

#undef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define Split spSplitview_Create
#define Wrap spWrapview_Create

static const char default_filelist_fmt[] =
    "%(.?.. [Parent Directory]%|%17f %11t  %m/%d/%y   %T %11s%)";

#if defined( IMAP )
static int gUseIMAP = 0;

int
UseIMAP()
{
	return( gUseIMAP );
}

int
SetUseIMAP( value )
int value;
{
	gUseIMAP = value;
}
#endif

static void
thisIsIt(self, buf)
    struct filelist *self;
    char *buf;
{
    if (self->fn)
	(*(self->fn))(self, buf);
}

static char *
joinpaths(buf, a, b)
    char *buf, *a, *b;
{
    strcpy(buf, a);
#if defined( IMAP )
    if ( !strcmp( a, "" ) || !strcmp( a, GetDelimiterString() ) ) {
	strcpy( buf, b );
	return( buf );
    }
    else 
	sprintf(buf, "%s%c%s", a, GetDelimiter(), b);
#else
    sprintf(buf, "%s/%s", a, b);
#endif
}

static int
legal(path)
    char *path;
{
    char buf[1 + MAXPATHLEN];

    strcpy(buf, path);
    fullpath(buf, 0);
    return (!strncmp(zmailroot, buf, strlen(zmailroot) - 1));
}

static void
listCallback(self, num, clicktype)
    struct spListv *self;
    int num;
    enum spListv_clicktype clicktype;
{
    struct stat statbuf;
    struct filelist *fl = (struct filelist *) spView_callbackData(self);
    char buf[1 + MAXPATHLEN];

#if defined( IMAP )
    if ( using_imap && UseIMAP() ) {
    	joinpaths(buf, fl->imapdir, *((char **) glist_Nth(&(fl->names), num)));
	if ( !strcmp( *((char **) glist_Nth(&(fl->names), num)), ".." ) ) {
		fl->pFolder = GetParent( fl->pFolder );
		strcpy( buf, GetCurrentDirectory( fl->pFolder ) );
	}
        else
		fl->pFolder = FolderByName( buf );
    }
    else
#endif
    joinpaths(buf, fl->dir, *((char **) glist_Nth(&(fl->names), num)));
#if defined( IMAP )
    if ( !( using_imap && UseIMAP() ) )
#endif
    if (!legal(buf)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 657, "May not select a file outside %s"),
	      zmailroot);
	return;
    }
    spSend(fl, m_filelist_setChoice, buf);
    if (clicktype == spListv_doubleclick) {
#if defined( IMAP )
        if ( using_imap && UseIMAP() ) {
		if (GetPathType( buf ) || !strcmp( buf, "" )) {
		    spSend(fl, m_filelist_setDirectory, buf);
		} else {
		    thisIsIt(fl, buf);
                }
	}
	else {
#endif
	estat(buf, &statbuf, "filelist/listCallback");
	if ((statbuf.st_mode & S_IFMT) == S_IFDIR) {
	    spSend(fl, m_filelist_setDirectory, buf);
	} else {
	    thisIsIt(fl, buf);
	}
#if defined( IMAP )
        }
#endif
    }
}

static void
choiceActivate(self, str)
    struct spCmdline *self;
    char *str;
{
    char buf[1 + MAXPATHLEN];
    struct filelist *fl = (struct filelist *) self->obj;
    struct stat statbuf;
    int e = 0, s = 0;

#if defined( IMAP )
    if ( using_imap && UseIMAP() ) {
	strcpy( buf, str );
    	str = (char *) spSend_p(fl, m_filelist_fullpath, buf);
    	strcpy(buf, str);
	fl->pFolder = FolderByName( buf );
	if ( !strcmp( str, ".." ) ) {
		strcpy( buf, GetCurrentDirectory( fl->pFolder ) );
		fl->pFolder = GetParent( GetParent( fl->pFolder ) );
	}
        spSend(fl, m_filelist_setChoice, buf);
        if (GetPathType( buf ) == 1) {
		spSend(fl, m_filelist_setDirectory, buf);
        } else {
		thisIsIt(fl, str);
	}
    }
    else {
#endif
    strcpy(buf, str);
    str = (char *) spSend_p(fl, m_filelist_fullpath, buf);
    strcpy(buf, str);
    fullpath(buf, 0);
    if (!legal(buf)) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 657, "May not select a file outside %s"),
	      zmailroot);
	return;
    }
    str = buf;
    TRY {
	estat(str, &statbuf, "filelist/choiceActivate");
	s = 1;
    } EXCEPT(strerror(ENOENT)) {
	if (fl->flags & PB_MUST_EXIST) {
	    error(SysErrWarning, "%s", str);
	    e = 1;
	}
    } EXCEPT(ANY) {
	error(SysErrWarning, "%s", str);
	e = 1;
    } ENDTRY;
    if (e) {
	return;
    }
    if (s && ((statbuf.st_mode & S_IFMT) == S_IFDIR)) {
	spSend(fl, m_filelist_setDirectory, str);
    } else {
	thisIsIt(fl, str);
    }
#if defined( IMAP )
    }
#endif
}

static void
filelist_initialize(self)
    struct filelist *self;
{
    self->fn = 0;

    glist_Init(&(self->names), (sizeof (char *)), 8);

    spSend(self->list = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());
    spSend(self->list, m_spView_setWclass, spwc_FileList);
    spListv_callback(self->list) = listCallback;
    spView_callbackData(self->list) = (struct spoor *) self;
    spListv_okclicks(self->list) = ((1 << spListv_click)
				    | (1 << spListv_doubleclick));

    spSend(self->choice = spCmdline_NEW(), m_spView_setObserved, spText_NEW());
    spCmdline_fn(self->choice) = choiceActivate;
    spCmdline_obj(self->choice) = (struct spoor *) self;

    spSend(self, m_spSplitview_setup, self->list,
	   Wrap(self->choice, NULL, NULL, catgets(catalog, CAT_LITE, 82, "File: "), NULL,
		0, 0, 0),
	   1, 1, 0,
	   spSplitview_topBottom, spSplitview_boxed,
	   spSplitview_SEPARATE);

    spSend(ZmlIm, m_spObservable_addObserver, self);

#if defined( IMAP )
    self->useIMAP = 1;
    if ( using_imap ) {
	    zimap_list_dir( NULL );
    }
    self->pFolder = 0;
#endif

#if defined( IMAP )
    strcpy( self->imapdir, "" );
    if ( using_imap && UseIMAP() ) {
        spSend(self, m_filelist_setDirectory, self->imapdir);
    }
    else 
    {
#endif
    GetCwd(self->dir, MAXPATHLEN);
    spSend(self, m_filelist_setDirectory, self->dir);
#if defined( IMAP )
    }
#endif

}

static void
filelist_finalize(self)
    struct filelist *self;
{
    int i;

    KillSplitviewsAndWrapviews(spSplitview_child(self, 0));
    KillSplitviewsAndWrapviews(spSplitview_child(self, 1));
    spSend(self->choice, m_spView_destroyObserved);
    spoor_DestroyInstance(self->choice);
    spSend(self->list, m_spView_destroyObserved);
    spoor_DestroyInstance(self->list);
    for (i = 0; i < glist_Length(&(self->names)); ++i) {
	free(*((char **) glist_Nth(&(self->names), i)));
    }
    glist_Destroy(&(self->names));
    spSend(ZmlIm, m_spObservable_removeObserver, self);
}

static void
reinit(gl)
    struct glist *gl;
{
    int i;

    for (i = 0; i < glist_Length(gl); ++i)
	free(*((char **) glist_Nth(gl, i)));
    glist_Destroy(gl);
    glist_Init(gl, (sizeof (char *)), 8);
}

static int
comparison(a, b)
    char **a, **b;
{
    return (strcmp(*a, *b));
}

static char *
skip_group(fmt, all)
char *fmt;
int all;
{
    while (*fmt) {
        if (*fmt++ != '%') continue;
        while (isdigit(*fmt)) fmt++;
        if (*fmt == '(') {
            fmt = skip_group(fmt+1, True);
        } else if (*fmt == '|') {
            fmt++; if (!all) return fmt;
        } else if (*fmt == ')')
            return (all) ? fmt+1 : fmt-1;
    }
    return fmt;
}

#define SIZEOFBUF(buf,p) (sizeof((buf)) - ((int )(p) - (int)(buf)) )
char *
FileFinderSummarize(filename, stat_b)
    char *filename;
    struct stat *stat_b;
{
    char now[9];
    struct tm *T = localtime(&stat_b->st_mtime);
    int isdir = (stat_b->st_mode & S_IFMT) == S_IFDIR;
    int isparent;
    char *type, *ptr;
    static char buf[200];
    int width, ll, cond, groupstart = 0, groupwidth = 0;
    int nesting = 0, groupnest = -1;
    long size;
    char *fmt = value_of(VarFilelistFmt);
#if defined( IMAP )
    char slashbuf[ 10 ];
#endif

    if (!fmt) fmt = default_filelist_fmt;
#if defined( IMAP )
    sprintf( slashbuf, "*%c..", GetDelimiter() );
    isparent = (isdir && zglob(filename, slashbuf));
#else
    isparent = (isdir && zglob(filename, "*/.."));
#endif
    if (isparent)
        (void) fullpath(filename, False); /* goes up to .. */

    size = isdir ? 0 : stat_b->st_size;
    type = isdir? catgets( catalog, CAT_MOTIF, 139, "[directory]" ) :
        ((stat_b->st_mode & S_IFMT) != S_IFREG)? catgets( catalog, CAT_MOTIF, 140, "[unknown]" ) : size == 0? catgets( catalog, CAT_MOTIF, 141, "[empty]" ) : catgets(catalog, CAT_MOTIF, 142, "[file]" );
    ptr = buf;
    while (*fmt) {
        if (*fmt != '%') { *ptr++ = *fmt++; continue; }
        fmt++;
        width = 0;
        while (isdigit(*fmt)) width = width*10+*fmt++ -'0';
        *ptr = 0;
        switch (*fmt++) {
        case 'f': strcpy(ptr, basename(filename));
        when 't': strcpy(ptr, type);
        when ')':
            if (--nesting == groupnest) {
                width = groupwidth; ptr = buf+groupstart;
                groupnest = -1;
            }
        when '(':
            if (fmt[1] != '?' && width) {
                groupstart = ptr-buf;

                groupwidth = width;
                width = 0;
                groupnest = nesting++;
                break;
            }
            cond = 0;
            switch (*fmt) {
            case '.': cond = isparent;
            when 'f': cond = (stat_b->st_mode & S_IFMT) == S_IFREG;
            when 'd': cond = isdir;
            when 'z': cond = size == 0;
            }
            fmt++; fmt++;
            nesting++;
            if (cond) break;
            fmt = skip_group(fmt, 0);
        when '|':
            fmt = skip_group(fmt, True);
            nesting--;
        when 's':
            if ((stat_b->st_mode & S_IFMT) != S_IFREG) break;
            if (width)
                sprintf(ptr, "%*ld", width, size);
            else
                sprintf(ptr, "%ld", size);
        when 'm': strftime(ptr, SIZEOFBUF(buf,ptr), "%m", T);
        when 'd': strftime(ptr, SIZEOFBUF(buf,ptr), "%d", T);
        when 'y': strftime(ptr, SIZEOFBUF(buf,ptr), "%y", T);
        when 'T': if (ison(glob_flags, MIL_TIME))
                     strftime(now, sizeof(now), "%H:%M", T);
                  else
                     strftime(now, sizeof(now), "%I:%M%p", T);
                  sprintf(ptr, "%*s", width, now);
        when '%': strcpy(ptr, "%");
        }
        ll = strlen(ptr);
        if (width && ll > width)
            ptr[ll = width] = 0;
        ptr += ll;
        while (ll < width) { *ptr++ = ' '; ll++; }
    }
    *ptr = 0;
    return buf;
}


static void
xfree_handle(handle)
    VPTR handle;
{
    xfree(*((void **)handle));
}

#if defined( IMAP )
void
#else
static void
#endif
rescan(self)
    struct filelist *self;
{
    DIR *dir = (DIR *) 0;
    struct dirent *direntry;
    struct tm *tm;
    char buf[1 + MAXPATHLEN];
    char date[20], time[20];
    char *tmp, *labelbuf;
    int i, hr;
    struct stat statbuf;
    struct glist newnames;
#if defined( IMAP )
    char *pPath, *foo;
    void *pFolder;
    int x;
#endif

    glist_Init(&newnames, sizeof(char *), 8);
    TRY {
      TRY {
	LITE_BUSY {
#if defined( IMAP )
	    if ( using_imap && UseIMAP() ) {
		zimap_list_dir( NULL );
		if ( !self->pFolder )
			self->pFolder = GetTreePtr();
		pPath = GetCurrentDirectory( self->pFolder );
		strcpy( self->imapdir, pPath );

		x = GetFolderCount( self->imapdir );
		pFolder = self->pFolder;
		for ( i = 0; i < x; i++ ) {
			foo = GetFolderName( i, self->imapdir );
		    	tmp = emalloc(1 + strlen(foo), "filelist_SetDirectory");
			strcpy( tmp, foo );
			glist_Add(&newnames, &tmp );
		}
	    }
	    else {
#endif
		    dir = eopendir(self->dir, "filelist_SetDirectory");
		    while (direntry = readdir(dir)) {
			if (strcmp(".", direntry->d_name)
			    && strcmp("..", direntry->d_name)) {
			    tmp = emalloc(1 + strlen(direntry->d_name),
					  "filelist_SetDirectory");
			    strcpy(tmp, direntry->d_name);
			    glist_Add(&newnames, &tmp);
			}
		    }
#if defined( IMAP )
	    }
#endif
	    spSend(spView_observed(self->list), m_spText_clear);
	    glist_Sort(&newnames, comparison);
#if defined( IMAP )
	    if ( !(using_imap && UseIMAP() ) ) {
#endif
	    tmp = emalloc(3, "filelist_SetDirectory");
	    strcpy(tmp, "..");
	    glist_Insert(&newnames, &tmp, 0);
#if defined( IMAP )
	    }
#endif
	    for (i = 0; i < glist_Length(&newnames); ++i) {
#if defined( IMAP )
		if ( !( using_imap && UseIMAP() ) ) {
#endif
		sprintf(buf, "%s/%s", self->dir,
			*((char **) glist_Nth(&newnames, i)));
#ifdef HAVE_LSTAT
		if (stat(buf, &statbuf))
		  elstat(buf, &statbuf, "filelist_setDirectory");
#else /* !HAVE_LSTAT */
		estat(buf, &statbuf, "filelist_setDirectory");
#endif /* !HAVE_LSTAT */
                labelbuf = FileFinderSummarize(*((char **)
						 glist_Nth(&newnames, i)),
					       &statbuf);
		spSend(spView_observed(self->list), m_spList_append, labelbuf);
#if defined( IMAP )
		}
		else {
			sprintf( buf, "%-20s   %s", 
				*((char **) glist_Nth(&newnames, i)),
				GetAttributeStringByName( 
					*((char **) glist_Nth(&newnames, i)),
					pFolder ) );
			spSend(spView_observed(self->list), m_spList_append, buf);
		}
#endif
	    }
	    glist_CleanDestroy(&(self->names), xfree_handle);
	    bcopy(&newnames, &(self->names), sizeof(struct glist));
	} LITE_ENDBUSY;
      } EXCEPT(ANY) {
	glist_CleanDestroy(&newnames, xfree_handle);
	PROPAGATE();
      } ENDTRY;
    } FINALLY {
      if (dir)
	closedir(dir);
    } ENDTRY;
}

#if defined( IMAP )
static void
filelist_setIMAP(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
#if 0
    char *indirname, *savename = NULL;

    indirname = spArg(arg, char *);
    if (indirname && (indirname != self->dir)) {
      ZSTRDUP(savename, self->dir);
      strcpy(self->dir, indirname);
      fullpath(self->dir, 0);
    }
    if (!legal(self->dir)) {
      xfree(savename);
      spSend(self, m_filelist_setDirectory, zmailroot);
      return;
    }

    TRY {
      TRY {
	rescan(self);
      } EXCEPT(ANY) {
	strcpy(self->dir, savename);
	PROPAGATE();
      } ENDTRY;
    } FINALLY {
      xfree(savename);
    } ENDTRY;
    spSend(self, m_filelist_setChoice, self->dir);
#endif
}
#endif

static void
filelist_setDirectory(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    char *indirname, *savename = NULL;

#if defined( IMAP )
    if ( using_imap && UseIMAP() ) {
    indirname = spArg(arg, char *);
    if (indirname && (indirname != self->imapdir)) {
      ZSTRDUP(savename, self->imapdir);
      strcpy(self->imapdir, indirname);
    }

    TRY {
      TRY {
	rescan(self);
      } EXCEPT(ANY) {
	strcpy(self->imapdir, savename);
	PROPAGATE();
      } ENDTRY;
    } FINALLY {
      xfree(savename);
    } ENDTRY;
    spSend(self, m_filelist_setChoice, self->imapdir);
    }
    else {
#endif
    indirname = spArg(arg, char *);
    if (indirname && (indirname != self->dir)) {
      ZSTRDUP(savename, self->dir);
      strcpy(self->dir, indirname);
      fullpath(self->dir, 0);
    }
    if (!legal(self->dir)) {
      xfree(savename);
      spSend(self, m_filelist_setDirectory, zmailroot);
      return;
    }

    TRY {
      TRY {
	rescan(self);
      } EXCEPT(ANY) {
	strcpy(self->dir, savename);
	PROPAGATE();
      } ENDTRY;
    } FINALLY {
      xfree(savename);
    } ENDTRY;
    spSend(self, m_filelist_setChoice, self->dir);
#if defined( IMAP )
    }
#endif
}

static void
filelist_setPrompt(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    char *prompt;
    struct spView *v = spView_parent(self->choice);

    prompt = spArg(arg, char *);
    if (v && spoor_IsClassMember(v, spWrapview_class))
	spSend(v, m_spWrapview_setLabel, prompt, spWrapview_left);
}

static void
filelist_setChoice(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    char *p;

    p = spArg(arg, char *);
#if defined( IMAP )
    if ( !(using_imap && UseIMAP()) )
#endif
    if (!legal(p)) {
	spSend(self, m_filelist_setDirectory, zmailroot);
	return;
    }
    spSend(spView_observed(self->choice), m_spText_clear);
    spSend(spView_observed(self->choice), m_spText_insert,
	   0, -1, p, spText_mBefore);
}

static char *
filelist_fullpath(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    char *name;
    static char buf[1 + MAXPATHLEN];
#if defined( IMAP )
    char tmpbuf[ 1 + MAXPATHLEN ];
#endif

    name = spArg(arg, char *);
    if (name) {
#if defined( IMAP )
	sprintf( tmpbuf, "%c~+%&", GetDelimiter() );
	if (index(tmpbuf, name[0]))
#else
	if (index("/~+%&", name[0]))
#endif
	    return (name);
#if defined( IMAP )
	if ( using_imap && UseIMAP() ) {
		strcpy( buf, name );
	}
	else
#endif
#if defined( IMAP )
	sprintf(buf, "%s%c%s", self->dir, GetDelimiter(), name);
#else
	sprintf(buf, "%s/%s", self->dir, name);
#endif
	return (buf);
    }
    return (NULL);
}

static void
filelist_setFile(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    char *name, *p;
    int i;

    name = spArg(arg, char *);
    name = (char *) spSend_p(self, m_filelist_fullpath, name);
    if (!legal(name)) {
	spSend(self, m_filelist_setDirectory, zmailroot);
	return;
    }
#if defined( IMAP )
    if (p = rindex(name, GetDelimiter())) {
#else
    if (p = rindex(name, '/')) {
#endif
	char dir[1 + MAXPATHLEN];

	strncpy(dir, name, (p - name));
	dir[p - name] = '\0';
	spSend(self, m_filelist_setDirectory, dir);
	++p;
    } else {
	p = name;
    }
    spSend(spView_observed(self->choice), m_spText_clear);
    spSend(spView_observed(self->choice), m_spText_insert,
	   0, -1, name, spText_mBefore);
    for (i = 0; i < glist_Length(&(self->names)); ++i) {
	if (!strcmp(p, *((char **) glist_Nth(&(self->names), i)))) {
	    char buf[10];

	    sprintf(buf, "%d", i + 1);
	    spSend(self->list, m_spView_invokeInteraction,
		   "list-click-line", 0, buf, 0);
	    break;
	}
    }
}

static void
filelist_setDefault(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    char *p, pbuf[1 + MAXPATHLEN], *b;
    int isdir = 1;

    p = spArg(arg, char *);
    strcpy(pbuf, p);

    b = getpath(pbuf, &isdir);
    if (legal(b)) {
	switch (isdir) {
	  case 1:
	    spSend(self, m_filelist_setDirectory, b);
	    break;
	  case 0:
	    spSend(self, m_filelist_setFile, b);
	    break;
	    /* Don't sweat the error case */
	}
    } else {
	spSend(self, m_filelist_setDirectory, zmailroot);
    }
}

static void
filelist_receiveNotification(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    struct spObservable *obs = spArg(arg, struct spObservable *);
    int event = spArg(arg, int);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    spSuper(filelist_class, self, m_spObservable_receiveNotification,
	    obs, event, data);
    if ((obs == (struct spObservable *) ZmlIm)
	&& (event == dialog_refresh)) {
#if defined( IMAP )
	if ( !( using_imap && UseIMAP() ) )
#endif
	rescan(self);
    }
}

static void
filelist_desiredSize(self, arg)
    struct filelist *self;
    spArgList_t arg;
{
    int *minh, *minw, *maxh, *maxw, *besth, *bestw;
    int screenw = 80, screenh = 24;

    minh = spArg(arg, int *);
    minw = spArg(arg, int *);
    maxh = spArg(arg, int *);
    maxw = spArg(arg, int *);
    besth = spArg(arg, int *);
    bestw = spArg(arg, int *);

    spSuper(filelist_class, self, m_spView_desiredSize,
	    minh, minw, maxh, maxw, besth, bestw);
    if (spView_window(ZmlIm))
	spSend(spView_window(ZmlIm), m_spWindow_size, &screenh, &screenw);
    *besth = MAX(screenh - 12, *minh);
}

static void
filebox_previous_line(self, requestor, data, keys)
    struct filelist *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self->list, m_spView_invokeInteraction, "list-up",
	   requestor, data, keys);
    spSend(self->list, m_spView_invokeInteraction, "list-click",
	   requestor, data, keys);
}

static void
filebox_next_line(self, requestor, data, keys)
    struct filelist *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self->list, m_spView_invokeInteraction, "list-down",
	   requestor, data, keys);
    spSend(self->list, m_spView_invokeInteraction, "list-click",
	   requestor, data, keys);
}

static void
filebox_next_page(self, requestor, data, keys)
    struct filelist *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self->list, m_spView_invokeInteraction, "list-next-page",
	   requestor, data, keys);
    spSend(self->list, m_spView_invokeInteraction, "list-click",
	   requestor, data, keys);
}

static void
filebox_previous_page(self, requestor, data, keys)
    struct filelist *self;
    struct spoor *requestor;
    GENERIC_POINTER_TYPE *data;
    struct spKeysequence *keys;
{
    spSend(self->list, m_spView_invokeInteraction, "list-previous-page",
	   requestor, data, keys);
    spSend(self->list, m_spView_invokeInteraction, "list-click",
	   requestor, data, keys);
}


struct spWidgetInfo *spwc_Filebox = 0;

void
filelist_InitializeClass()
{
    if (!spSplitview_class)
	spSplitview_InitializeClass();
    if (filelist_class)
	return;

    filelist_class =
	spWclass_Create("filelist", NULL,
			spSplitview_class,
			(sizeof (struct filelist)),
			filelist_initialize,
			filelist_finalize,
			spwc_Filebox = spWidget_Create("Filebox",
						       spwc_Widget));

    m_filelist_setDirectory =
	spoor_AddMethod(filelist_class, "setDirectory",
			NULL, filelist_setDirectory);
#if defined( IMAP )
    m_filelist_setIMAP =
	spoor_AddMethod(filelist_class, "setIMAP",
			NULL, filelist_setIMAP);
#endif
    m_filelist_setPrompt =
	spoor_AddMethod(filelist_class, "setPrompt",
			NULL,
			filelist_setPrompt);
    m_filelist_setChoice =
	spoor_AddMethod(filelist_class, "setChoice",
			NULL,
			filelist_setChoice);
    m_filelist_fullpath =
	spoor_AddMethod(filelist_class, "fullpath",
			NULL,
			filelist_fullpath);
    m_filelist_setFile =
	spoor_AddMethod(filelist_class, "setFile",
			NULL,
			filelist_setFile);
    m_filelist_setDefault =
	spoor_AddMethod(filelist_class, "setDefault",
			NULL,
			filelist_setDefault);
    spoor_AddOverride(filelist_class, m_spObservable_receiveNotification,
		      NULL, filelist_receiveNotification);
    spoor_AddOverride(filelist_class, m_spView_desiredSize, NULL,
		      filelist_desiredSize);

    spWidget_AddInteraction(spwc_Filebox, "filebox-up",
			    filebox_previous_line,
			    catgets(catalog, CAT_LITE, 664, "Select previous file"));
    spWidget_AddInteraction(spwc_Filebox, "filebox-down",
			    filebox_next_line, catgets(catalog, CAT_LITE, 665, "Select next file"));
    spWidget_AddInteraction(spwc_Filebox, "filebox-next-page",
			    filebox_next_page, catgets(catalog, CAT_LITE, 666, "Next page of files"));
    spWidget_AddInteraction(spwc_Filebox, "filebox-previous-page",
			    filebox_previous_page, catgets(catalog, CAT_LITE, 667, "Previous page of files"));

    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "^P", 1),
		     "filebox-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "\\<up>", 1),
		     "filebox-up", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "^N", 1),
		     "filebox-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "\\<down>", 1),
		     "filebox-down", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "\\<pageup>", 1),
		     "filebox-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "\\ev", 1),
		     "filebox-previous-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "\\<pagedown>", 1),
		     "filebox-next-page", 0, 0, 0, 0);
    spWidget_bindKey(spwc_Filebox, spKeysequence_Parse(0, "^V", 1),
		     "filebox-next-page", 0, 0, 0, 0);

    spButton_InitializeClass();
    spButtonv_InitializeClass();
    spText_InitializeClass();
    spTextview_InitializeClass();
    spCmdline_InitializeClass();
    spWrapview_InitializeClass();
    spIm_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
}
