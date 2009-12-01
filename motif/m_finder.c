/* m_finder.c	Copyright 1992 Z-Code Software Corp. */

/*
 * FileFinder -- a simplified file search compound widget.
 */

#ifdef SPTX21
#define _XOS_H_
#define NEED_U_LONG
#endif /* SPTX21 */

#include "catalog.h"
#include "cursors.h"
#include "config/features.h"
#include "fetch.h"
#include "finder.h"
#include "fsfix.h"
#include "glob.h"
#include "m_menus.h"
#include "zctime.h"
#include "zm_motif.h"
#include "zmail.h"
#include "zmframe.h"
#include "zmopt.h"

#include <Xm/LabelG.h>
#include <Xm/Text.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>
#include <Xm/BulletinB.h>
#include <Xm/ToggleB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>

#include <Xm/PanedW.h>
#ifdef SANE_WINDOW
#include "xm/sanew.h"
#define PANED_WINDOW_CLASS zmSaneWindowWidgetClass
#else /* !SANE_WINDOW */
#define PANED_WINDOW_CLASS xmPanedWindowWidgetClass
#endif /* !SANE_WINDOW */

#ifdef NOT_ZMAIL
/* Bart: Mon Jul 19 23:02:53 PDT 1993
 * The has_root_prefix() function is defined in shell/commands.c for use
 * in the cd() function.  It handles Apollo "//" stupidity.  If we're not
 * building zmail, just ignore all this foolishness.
 */
#define has_root_prefix(p)	(1)
#ifndef TIMER_API
#define zm_current_time(x)	time((time_t *)0)
#endif /* !TIMER_API */
#endif /* NOT_ZMAIL */

static void research_dir();
#ifndef TIMER_API
static void ffRefreshCB();
#endif /* !TIMER_API */

extern ZcIcon folders_icon;

/*
 * Hack to make the FileFinder more flexible; without this,
 * we'd be forced to CreateActionArea() to get Widget ids.
 */
void
zmClickByName(parent, name)
Widget parent;
char *name;
{
    Widget w;

    if (w = XtNameToWidget(parent, "*action_area"))
	parent = w;

    w = XtNameToWidget(parent, name);

    if (w)
	zmButtonClick(w);
}

int finder_full_paths = 1;

static void
ffCopyDown(list_w, ffs, cbs)
Widget list_w;
FileFinderStruct *ffs;
XmListCallbackStruct *cbs;
{
    char *item;

    if (finder_full_paths) {
	/* DON'T TRIM THIS FILENAME!  trim_filename() removes the current
	 * directory from the path, and we need a full path in the text_w.
	 */
	SetTextString(ffs->text_w, ffs->entries[cbs->item_position - 1]);
	    return;
    }
    item = ffs->entries[cbs->item_position - 1];
#if defined( IMAP )
    if ( ffs->useIMAP ) {
	if ( *item == '{' ) {
		while ( *item != '}' && *item != '\0' ) item++;
		if ( *item == '}' ) item++;
		if ( *item == '\0' ) item = ffs->entries[cbs->item_position - 1];
	}
    }
    else 
    	item = (strlen(item) < strlen(ffs->dir)) ? ".." : basename(item);
#else
    item = (strlen(item) < strlen(ffs->dir)) ? ".." : basename(item);
#endif
    SetTextString(ffs->text_w, item);
}

void
ffSelectionCB(w, ffs, cbs)
Widget w;
FileFinderStruct *ffs;
XmListCallbackStruct *cbs;
{
    char *file, *dot_dot = NULL, *tmp, buf[MAXPATHLEN];
    int n = 1, do_free = 1;
#if defined( IMAP )
    int	isDotDot = 0;
    void *pFolder;
#endif

#if defined( IMAP ) && 0
    if ( ffs->useIMAP && w == ffs->text_w )
	return;
#endif

    if (!(tmp = XmTextGetString(ffs->text_w)))
	return;
#if defined( IMAP )
    if ( !strcmp( tmp, ".." ) ) 
	isDotDot++;
    if ( !ffs->useIMAP ) 
#endif
    if (cbs && cbs->reason == XmCR_DEFAULT_ACTION && cbs->item_position == 1)     { 
	XtFree(tmp);
	tmp = "..";
	do_free = 0;
    }

#if defined( IMAP )
    if ( ffs->useIMAP )
	    pFolder = ffs->pFolder;
#endif
    file = FileFinderGetPath(ffs, tmp, &n);
#if defined( IMAP )
    if ( ffs->useIMAP ) {
    	if ( n == 0 ) ffs->pFolder = pFolder;
	if ( n == -1 ) {
		file = tmp;
		n = 2;
	}
    }
#endif

    if (n < 0) {
	error(UserErrWarning, "%s: %s", file, tmp);
	assign_cursor(FrameGetData(w), None);
    	if (do_free) XtFree(tmp);
	return;
    }

    if (cbs && cbs->reason == XmCR_DEFAULT_ACTION) {
	/* double click in main list -- if it's the first item, save value */
	/* Bart: Mon Aug 10 18:03:50 PDT 1992				XXX
	 * If the client has supplied a search_func and that function does
	 * not either call FileFinderDefaultSearch() or otherwise guarantee
	 * that the first item in the list is the parent directory, this is
	 * wrong ...							XXX
	 */
	if (cbs->item_position == 1)
	    dot_dot = savestr(ffs->dir);
    } else if (w != ffs->text_w) {
	/* search button was pushed */
	n = 1;
    }

#if defined( IMAP )
    if ( ffs->useIMAP && isDotDot ) {
	char	*p, c;

	p = file + strlen( file );
	c = GetDelimiter();
	while ( p != file && *p != c ) p--;
	if ( p != file )
		file = p + 1;	/* skip the delimiter */
    }
#endif
    (void) strcpy(buf, file);	/* Free up FileFinderGetPath()'s buffer */
    if (do_free) XtFree(tmp);
    if (n == 1) {
	/* directory */
#ifdef USE_FAM
	ffs->fam.tracking = False;
	if (fam) {
	    FAMCancelMonitor(fam, &ffs->fam.request);
	    FAMMonitorDirectory(fam, buf, &ffs->fam.request, &ffs->fam.closure);
	}
#endif /* USE_FAM */
	(*ffs->search_func)(FileFindDir, ffs, buf, dot_dot);
    } else {
	/* normal file */
	if (ffs->ok_func)
	    (*ffs->ok_func)(FileFindFile, ffs, buf);
	else
	    FileFinderSelectItem(ffs, buf, True);
    }
    xfree(dot_dot);
    /* Reassign the cursor explicitly in case timeout_cursors()
     * was called from error() during the search.
     */

    assign_cursor(FrameGetData(w), None);
}

#define DEF_FILELIST_FMT catgets( catalog, CAT_MOTIF, 136, "%(.?..                [Parent Directory]%|%17f %11t  %m/%d/%y   %T %11s%)" )

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

    if (!fmt) fmt = DEF_FILELIST_FMT;
    isparent = (isdir && zglob(filename, "*/.."));
    if (isparent)
	(void) fullpath(filename, False); /* goes up to .. */

    if (ison(glob_flags, MIL_TIME))
	(void) sprintf(now, catgets( catalog, CAT_MOTIF, 137, "%2d:%02d" ), T->tm_hour, T->tm_min);
    else
	(void) sprintf(now, catgets( catalog, CAT_MOTIF, 138, "%2.d:%02d%cm" ), (T->tm_hour) ?
	      ((T->tm_hour <= 12) ? T->tm_hour : T->tm_hour - 12) :
	      12, T->tm_min,
	      ((T->tm_hour < 12) ? 'a' : 'p'));
    size = isdir ? 0 : stat_b->st_size;
    type = isdir? catgets( catalog, CAT_MOTIF, 139, "[directory]" ) :
	((stat_b->st_mode & S_IFMT) != S_IFREG)? catgets( catalog, CAT_MOTIF, 140, "[unknown]" ) :
	    size == 0? catgets( catalog, CAT_MOTIF, 141, "[empty]" ) : catgets( catalog, CAT_MOTIF, 142, "[file]" );
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
	when 'm': sprintf(ptr, catgets( catalog, CAT_MOTIF, 143, "%2.d" ), T->tm_mon+1);
	when 'd': sprintf(ptr, catgets( catalog, CAT_MOTIF, 144, "%02d" ), T->tm_mday);
	when 'y': sprintf(ptr, catgets( catalog, CAT_MOTIF, 145, "%02d" ), T->tm_year%100);
	when 'T': strcpy(ptr, now);
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

char *
FileFinderGetPath(ffs, file, isdir)
FileFinderStruct *ffs;
char *file;
int *isdir;
{
    char buf[MAXPATHLEN];
    int dummy;
#if defined( IMAP )
    char *pPath;
    void *pFolder;
    char *p;
#endif

    if (!isdir)
	isdir = &dummy;

#if defined( IMAP )

    if ( ffs->useIMAP ) {
	if ( !ffs->pFolder )
		ffs->pFolder = GetTreePtr();
	pPath = GetFullPath( file, ffs->pFolder );
	pFolder = FolderByName( pPath );
	if ( pFolder )
		ffs->pFolder = pFolder; 
	if ( !strcmp( file, ".." ) )
		*isdir = 1;
	else if ( strlen( pPath ) )
		*isdir = IsADir( GetPathType( pPath ) );
	else 
		*isdir = 0;
	if ( *isdir != -1 ) {
		if ( !strcmp( "..", file ) ) {
			p = XtMalloc( strlen( pPath ) + 1 );
			strcpy( p, pPath );
		}
		else {
			p = XtMalloc( strlen( file ) + 1 );
			strcpy( p, file );
		}	
	}
	return( ( *isdir == -1 ? SNGL_NULL : p ) );	
    }
    else {
#endif
    /* Expand ~ and variables, prepend dir if needed */
    if ((dummy = dvarstat(ffs->dir, file, buf, NULL)) < 0) {
	return isdir == &dummy? SNGL_NULL :
	    (*isdir = dummy) == -2 ? catgets( catalog, CAT_MOTIF, 146, "variable expansion failed" ) :
		sys_errlist[errno];
    } else
	dummy = 1;			/* In case isdir == &dummy */
    if (!fullpath(buf, FALSE)) {	/* Change dir//file to dir/file */
	*isdir = -1;
	return isdir == &dummy? SNGL_NULL :
	    catgets( catalog, CAT_MOTIF, 147, "cannot expand full path" );
    }
    if (!has_root_prefix(buf)) {
	*isdir = -1;
	return isdir == &dummy? SNGL_NULL :
	    catgets(catalog, CAT_MOTIF, 872, "access not allowed");
    }
    file = getpath(buf, isdir);	/* Now actually stat the full path */
    if (dummy < 0)
	return SNGL_NULL;
#if defined( IMAP )
    }
#endif
    return file;
}

char *
FileFinderGetFullPath(file_w)
Widget file_w;
{
    return FileFinderGetFullPathEx(file_w, (int *) 0);
}

char *
FileFinderGetFullPathEx(file_w, n)
Widget file_w;
int *n;
{
    FileFinderStruct *ffs;
    char *str, *text;
    
    XtVaGetValues(XtParent(XtParent(file_w)), XmNuserData, &ffs, NULL);
    text = XmTextGetString(file_w);
    if (!text) return NULL;
    if (!*text) { XtFree(text); return NULL; }
    str = FileFinderGetPath(ffs, text, n);
    XtFree(text);
    return savestr(str);
}

void
FileFinderSetFullPath(file_w, name)
Widget file_w;
char *name;
{
    FileFinderStruct *ffs;
    char *p;
    char buf[MAXPATHLEN];

    XtVaGetValues(XtParent(XtParent(file_w)), XmNuserData, &ffs, NULL);
    strcpy(buf, name);
    name = savestr(name);
    p = last_dsep(buf);
    if (p == buf) p++;
    *p = 0;
    if (strcmp(buf, ffs->dir))
	FileFinderDefaultSearch(FileFindDir, ffs, buf, NULL);
    SetTextString(file_w, (finder_full_paths) ? name : basename(name));
    xfree(name);
}

void
FileFinderDefaultSearch(type, ffs, dir, file)
FileFinderType type;
FileFinderStruct *ffs;
const char *dir;
const char *file;	/* "file" is the entry to select within dir */
{
    const char	 *path;
    char	**names, buf[MAXPATHLEN], *olddir = NULL, *p;
    XmString	 *items;
    int		  i, j, x, sel_pos = 1;
    Widget	  file_text = ffs->text_w, file_list = ffs->list_w;
    Boolean	  search_dots;
    Boolean	  only_folders;
    struct stat	  stat_b;
    FolderType    f_type;
#if defined( IMAP )
    char *pPath;
    void *pFolder;
#endif

    ask_item = file_text;

#if defined( IMAP )
    if ( !ffs->useIMAP ) {
#endif
    if (type == FileFindFile || file && !dir) {
	/* Bart: Mon Aug 31 17:54:40 PDT 1992
	 * There was a really confusing bit of flakery here just to make
	 * sure that file was something sensible by the time we got to
	 * the second if() below, and then to try to restore it's previous
	 * value afterwards.  Clearer to just use path as the temporary
	 * throughout, and initialize it from file.
	 */
	path = file;
	if (dir || !path || !is_fullpath(path)) {
	    if (dgetstat(dir, path, buf, NULL) == 0)
		path = buf;
	    else
		path = NULL;
	}
	if (path && (p = last_dsep(path)) && p > path) {
	    if (path != buf)
		(void) strcpy(buf, path);
	    buf[p-path] = 0;
	    dir = buf;
	}
    }
    if (dir) {
	olddir = ffs->dir;
	ffs->dir = savestr(expandpath(dir, buf, False));
    } else if (!*ffs->dir) {
	ask_item = file_text;
	error(UserErrWarning, catgets( catalog, CAT_MOTIF, 148, "Please specify a file or directory." ));
	return;
    }

    x = 0;
    path = getpath(ffs->dir, &x);
    if (x == -1) {
	if (type != FileFindReset)
	    error(UserErrWarning, catgets( catalog, CAT_MOTIF, 149, "Cannot read \"%s\": %s." ), ffs->dir, path);
	p = last_dsep(ffs->dir);
	if (p && p > ffs->dir) {
	    *p = '\0';
	    FileFinderDefaultSearch(FileFindReset, ffs, NULL, NULL);
	}
	xfree(olddir);
	return;
    } else if (x != 1) {
	error(ZmErrWarning, catgets( catalog, CAT_MOTIF, 150, "\"%s\" is not a directory." ), path);
	xfree(ffs->dir);
	ffs->dir = olddir;
	return;
    }

    timeout_cursors(True);

    /* Bart: Mon Aug 10 18:41:11 PDT 1992
     * Also assign the cursor explicitly in case timeout_cursors()
     * was already a nested call.
     */
    assign_cursor(FrameGetData(ffs->dir_w), please_wait_cursor);

    xfree(olddir);
    if ((i = strlen(p = trim_filename(ffs->dir))) > 50)
	p += (i - 50);
#if defined( IMAP )
    } else {
	    timeout_cursors(True);

    /* Bart: Mon Aug 10 18:41:11 PDT 1992
     * Also assign the cursor explicitly in case timeout_cursors()
     * was already a nested call.
     */
	    assign_cursor(FrameGetData(ffs->dir_w), please_wait_cursor);
    }
#endif 

#if defined( IMAP )
    if ( ffs->useIMAP ) {
	zimap_list_dir( NULL );
	if ( !dir || !strlen( dir ) ) {
		ffs->pFolder = GetTreePtr();
		ffResetMenu(ffs);
	}
	else if ( FolderIsFile( ffs->pFolder ) ) {
		ffs->pFolder = GetParent( ffs->pFolder );
		ffResetMenu(ffs);
	}
#if 0
	pPath = GetFullPath( dir, ffs->pFolder );
#else
	pPath = GetCurrentDirectory( ffs->pFolder );
#endif
	x = GetFolderCount( pPath );
	names = malloc( sizeof(char *) * (x + 1) );
	pFolder = FolderByName( pPath );
    }
#endif

    search_dots = XmToggleButtonGetState(ffs->dots_w);
    only_folders = XmToggleButtonGetState(ffs->folders_w);

#if defined( IMAP )
    if ( !ffs->useIMAP ) {
#endif
    /* Get a full path name with no ".." as simply as possible. */
    if (!expandpath(path, buf, False))
	goto done;
    if (search_dots)
	(void) strcat(buf, "/{.*,*}");
    else
	(void) strcat(buf, "/{..,*}");
    if ((x = filexp(buf, &names)) < 1) {
	error(SysErrWarning, catgets( catalog, CAT_SHELL, 120, "Cannot read \"%s\"" ), ffs->dir);
	goto done;
    }
#if defined( IMAP )
    }
#endif

    items = (XmString *)XtMalloc((unsigned)((x+1) * sizeof(XmString)));

#if defined( IMAP )
    if ( !ffs->useIMAP ) {
#endif
    if (file) {
	if (dgetstat(ffs->dir, file, buf, NULL) == 0)
	    file = buf;
    }

    /* start at 1 -- ".." is the 0th entry special-cased below */
    for (i = 0; i < x; i++) {
	Boolean found_dot = False; /* one "." per dir -- save redundant work */
	Boolean found_dot_dot = !!strcmp(ffs->dir, zmailroot);
	Boolean skip;
	j = 0;
	skip = False;
	if (!found_dot_dot && (found_dot_dot = zglob(names[i], "*/..")))
	    skip = True;
	else if (search_dots && !found_dot &&
		   (found_dot = zglob(names[i], "*/.")))
	    skip = True;
	else if (!only_folders && stat(names[i], &stat_b) < 0)
	    skip = True;
	else if (only_folders &&
		 ((f_type = stat_folder(names[i], &stat_b)) == FolderInvalid
		      || f_type == FolderUnknown))
	    skip = True;
	if (skip) {
	    xfree(names[i]);
	    for (j = i; i < x; ++i)
		names[i] = names[i+1];
	    x--, i = j-1;
	} else {
	    items[i] = XmStr(FileFinderSummarize(names[i], &stat_b));
	    if (file && !pathcmp(names[i], file))
		sel_pos = i + 1;
	}
    }
#if defined( IMAP )
    }
    else {
	char *foo;

	for ( i = 0; i < x; i++ ) {
		foo = GetFolderName( i, pPath );
		names[i] = malloc( strlen( foo ) + 1 );
		strcpy( names[i], foo );
		items[i] = XmStr( GetFolderAttributeString( i, pPath ) );
	}
    }
#endif

    names[x] = NULL;
    items[x] = (XmString)0;
    free_vec(ffs->entries);
    ffs->entries = names;

    /* even if names is empty... */
    XtVaSetValues(file_list,
	XmNitems,             x? items : 0,
	XmNitemCount,         x,
	NULL);
    /* if we do this as part the the setvalues, the focus isn't set right */
    if (sel_pos)
	XmListSelectItem(file_list, items[sel_pos-1], False);
    XmStringFreeTable(items);

    if (x) {
	LIST_VIEW_POS(file_list, sel_pos? sel_pos : 1);
	if (type != FileFindReset)
	    if (finder_full_paths)
		SetTextString(file_text, file && *file? file : ffs->dir);
	    else
		SetTextString(file_text, file && *file? basename(file) : "");
    }
    ffResetMenu(ffs);
done:
    assign_cursor(FrameGetData(ffs->dir_w), None);

#ifndef USE_FAM
    /* Record that we did the search */
#ifdef TIMER_API
    timer_resume(ffs->timer);
#else /* !TIMER_API */
    ffs->timer = zm_current_time(1);
#endif /* TIMER_API */
#endif /* !USE_FAM */

    timeout_cursors(False);
}

void
FileFinderSelectItem(ffs, item, activate)
FileFinderStruct *ffs;
const char *item;
int activate;
{
    char pattern[MAXPATHLEN];
    struct stat stat_b;
    int i;

    if (finder_full_paths) {
	(void) pathcpy(&pattern[1], item);
	pattern[0] = '*';
    } else {
	(void) pathcpy(pattern, item);
    }
    
    for (i = 0; ffs->entries && ffs->entries[i]; i++)
	if (zglob(ffs->entries[i], pattern)) {
	    if (stat(ffs->entries[i], &stat_b) == 0) {
		XmString xm_item =
		    XmStr(FileFinderSummarize(ffs->entries[i], &stat_b));
		XmListReplaceItemsPos(ffs->list_w, &xm_item, 1, i+1);
		XmStringFree(xm_item);
	    }
	    /* This is a browseSelection, not a defaultAction */
	    XmListSelectPos(ffs->list_w, i+1, activate);
	    break;
	}
    if (activate && ffs->entries && !ffs->entries[i])
	FileFinderDefaultSearch(FileFindFile, ffs, NULL, item);
}

#ifdef TIMER_API
#if defined( IMAP )
void
#else
static void
#endif
ffUpdate(ffs)
    FileFinderStruct *ffs;
{
    char directory[MAXPATHLEN], *file;
#if defined( IMAP )
    char	*p;
#endif

#if defined( IMAP )
    if ( ffs->useIMAP ) {
	p = GetCurrentDirectory( ffs->pFolder );
	if ( p ) {
		char	*q, c;

		q = p + strlen( p );
		c = GetDelimiter();
		while ( q != p && *q != c ) q--;
		if ( p != q )
			p = q + 1;	/* skip the delimiter */
		strcpy( directory, p );
	}
    }
    else {
#endif 
    strcpy(directory, has_root_prefix(ffs->dir) ? ffs->dir : zmailroot);
#if defined( IMAP )
    }
#endif
    file = XmTextGetString(ffs->text_w);
    (*ffs->search_func)(FileFindDir, ffs, directory, file);
    SetTextString(ffs->text_w, file);
    XtFree(file);
}

#ifdef USE_FAM
static void
ffSuspend(shell, ffs)
    Widget shell;
    FileFinderStruct *ffs;
{
    if (fam) FAMSuspendMonitor(fam, &ffs->fam.request);
}

static void
ffResume(shell, ffs)
    Widget shell;
    FileFinderStruct *ffs;
{
    if (fam) FAMResumeMonitor(fam, &ffs->fam.request);
    ffUpdate(ffs);
}

static void
ffHandleEvent(event, ffs)
    FAMEvent *event;
    FileFinderStruct *ffs;
{
    if (ffs->fam.tracking)
	switch (event->code) {
	    case FAMChanged:
	    case FAMDeleted:
	    case FAMCreated:
	    case FAMMoved:
		/* avoid flicker when events come rapidly */
		remove_deferred(ffUpdate, ffs);
	        add_deferred(ffUpdate, ffs);
	    }
    else if (event->code == FAMEndExist)
	ffs->fam.tracking = True;
}

#else /* !USE_FAM */

static void
ffSuspend(shell, ffs)
    Widget shell;
    FileFinderStruct *ffs;
{
    timer_suspend(ffs->timer);
}

static void
ffResume(shell, ffs)
    Widget shell;
    FileFinderStruct *ffs;
{
    ffUpdate(ffs);
    XmProcessTraversal(ffs->list_w, XmTRAVERSE_CURRENT);
}

static void
ffExpire(ffs, timer)
    FileFinderStruct *ffs;
    TimerId timer;
{
    struct stat status;
    
    if (stat(ffs->dir, &status) < 0 || time(NULL) - status.st_mtime < passive_timeout)
	ffUpdate(ffs);
    else
	timer_resume(timer);
}

static void
ffFrequency(ffs)
    FileFinderStruct *ffs;
{
    timer_reset(ffs->timer, passive_timeout * 1000);
    timer_resume(ffs->timer);
}
#endif /* !USE_FAM */

#else /* !TIMER_API */

static void
ffUpdateTimer(ffs, action)
FileFinderStruct *ffs;
DeferredAction *action;
{
    long now = zm_current_time(0);
    char dir[MAXPATHLEN], *file;
    struct stat sbuf;

    /* Checking the global variable time_out here -- ok?  XXX */
    if (now - ffs->timer > passive_timeout) {
	(void) strcpy(dir, ffs->dir);
	if (stat(dir, &sbuf) < 0 || sbuf.st_mtime > ffs->timer) {
	    file = XmTextGetString(ffs->text_w);
	    if (!has_root_prefix(dir))
		strcpy(dir, zmailroot);
	    if (ffs->search_func) {
		(*ffs->search_func)(FileFindDir, ffs, dir, file);
		ffs->timer = now;
	    } else
		FileFinderDefaultSearch(FileFindDir, ffs, dir, file);
	    /* Bart: Tue Jun 30 13:54:08 PDT 1992
	     * Make sure the text value doesn't change unexpectedly.
	     */
	    SetTextString(ffs->text_w, file);
	    XtFree(file);
	}
    }
    if (action) action->da_finished = FALSE;
}

static void
ffRefresh(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    int needs_updating = False;

    if (strcmp(ffs->root, zmailroot))
	needs_updating = True;
    if (zm_current_time(0) - ffs->timer > passive_timeout)
	needs_updating = True;
    if (needs_updating) {
	ZSTRDUP(ffs->root, zmailroot);
	ffs->timer = 0;
	ffUpdateTimer(ffs, NULL);
    }
    if (w)
	XmProcessTraversal(ffs->list_w, XmTRAVERSE_CURRENT);
}

static void
ffUpdate(w, ffs)
    Widget w;
    FileFinderStruct *ffs;
{
    ffs->timer = 0;
    ffUpdateTimer(ffs, NULL);
}

static void
ffRefreshCB(ffs)
{
    ffRefresh((Widget *) 0, ffs);
}
#endif /* !TIMER_API */

static void
ffChroot(ffs)
    FileFinderStruct *ffs;
{
    if (strcmp(ffs->root, zmailroot)) {
	ZSTRDUP(ffs->root, zmailroot);
	ffUpdate(ffs);
    }
}

static void
ffFreeFinder(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
#ifdef USE_FAM
    if (fam) FAMCancelMonitor(fam, &ffs->fam.request);
#else /* !USE_FAM */
#ifdef TIMER_API
    timer_destroy(ffs->timer);
#else /* !TIMER_API */
	 remove_deferred(ffUpdateTimer, ffs);
#endif /* TIMER_API */
#endif /* USE_FAM */

    xfree(ffs->dir);
    free_vec(ffs->entries);
    xfree(ffs->root);
    xfree(ffs);
}

static void
ffMenuCB(w, call_data)
Widget w;
XtPointer call_data;
{
    int n = (int)call_data;
    FileFinderStruct *ffs;
    char *p, buf[MAXPATHLEN], old[MAXPATHLEN];

    XtVaGetValues(w, XmNuserData, &ffs, NULL);
#if defined( IMAP )
    if ( !ffs->useIMAP ) { 
#endif
    pathcpy(buf, ffs->dir);
    *old = 0;
    if (strlen(zmailroot) > 1 && has_root_prefix(ffs->dir)) {
#ifdef apollo
	if (pathncmp(zmailroot, "//", 2) == 0)
	    p = zmailroot + 1;
	else
#endif /* apollo */
	    p = zmailroot;
	while (p = find_dsep(p))
	    p++, n++;
    }
#ifdef apollo
    if (pathncmp(buf, "//", 2) == 0) {
	p = buf + 1;
	while ((p = find_dsep(p)) && n--)
	    p++;
	if (p == buf + 1)
	    p++;
    } else
#endif /* apollo */
    {
	p = buf;
	while ((p = find_dsep(p)) && n--)
	    p++;
	if (p == buf)
	    p++;
    }
    if (p && *p) {
	strcpy(old, (is_dsep(*p)) ? p+1 : p);
	*p = 0;
	if ((p = find_dsep(old)) != NULL)
	    *p = 0;
    }
    /* Bart: Mon Jan 25 12:51:27 PST 1993
     * Don't re-search if the path is the same as where we are.
     * This works around a Motif bug (?) wherein this callback
     * gets called when the menu is *posted* (by a click on the
     * option menu button) as well as when a selection is made.
     */
    if (pathcmp(buf, ffs->dir) != 0) {
	XmProcessTraversal(ffs->list_w, XmTRAVERSE_CURRENT);
	FileFinderDefaultSearch(FileFindDir, ffs, buf, old);
    }
#if defined( IMAP )
    }
    else {
	int	i;
	char	*q, *p, dStr[ 2 ];

	p = GetCurrentDirectory( ffs->pFolder );
	buf[ 0 ] = '\0';
	dStr[0] = GetDelimiter();
	dStr[1] = '\0';
	for ( i = 0; i < n; i++ ) {
        	q = GetNextComponent( p, dStr[0] );
               	if ( q ) {
               		p += strlen( q ) + 1;
			if ( !buf[ 0 ] ) 
				strcpy( buf, q );
			else {
				strcat( buf, dStr );
				strcat( buf, q );
			}
			free( q );
		}
	} 
	ffs->pFolder = FolderByName( buf );
	XmProcessTraversal(ffs->list_w, XmTRAVERSE_CURRENT);
#if 0
	FileFinderDefaultSearch(FileFindDir, ffs, buf, NULL);
#else
	ffUpdate( ffs );
#endif
    }
#endif
}

ffResetMenu(ffs)
FileFinderStruct *ffs;
{
    Widget sub_menu, parent = XtParent(ffs->dir_w);
    char **choices;	/* Need to be freed when menu is destroyed */
    char *dir;
#if defined( IMAP )
    char *pPath;
    char dStr[2];
#endif
    int n, l = 0;

    XtVaGetValues(ffs->dir_w, XmNsubMenuId, &sub_menu, 0);

#ifdef apollo
    if (pathncmp(ffs->dir, "//", 2) == 0) {
	choices = unitv("//");
	l++;
    } else
#endif /* apollo */

#if defined( IMAP )
    {
    if ( ffs->useIMAP )
        choices = unitv(GetDelimiterString());
    else 
	choices = unitv(SSLASH);
    }
#else
	choices = unitv(SSLASH);
#endif

#if defined( IMAP )
    if ( ffs->useIMAP ) {
	dStr[0] = GetDelimiter();
	dStr[1] = '\0';
	pPath = GetCurrentDirectory( ffs->pFolder );
    	n = vcat(&choices, strvec(pPath, dStr, TRUE));
    }
    else {
#endif
    if (has_root_prefix(ffs->dir))
	l = strlen(zmailroot);
    dir = ffs->dir+l;
    if (is_dsep(*dir)) dir++;
    n = vcat(&choices, strvec(dir, DSEP, TRUE));
#if defined( IMAP )
    }
#endif
    if (n < 1)
	return;
    if (l > 1)
	ZSTRDUP(choices[0], basename(zmailroot));
#ifdef NOT_NOW
    {
	Widget sub_menu;
	MenuItem *items = BuildMenuItems(choices, ffMenuCB);
	sub_menu = BuildPulldownMenu(parent, NULL, items, ffs);
	XtFree(items);
	XtVaSetValues(ffs->dir_w, XmNsubMenuId, sub_menu, NULL);
	SetNthOptionMenuChoice(ffs->dir_w, n-1);
    }
    /* Otherwise everything before free_vec() */
#endif /* NOT_NOW */
    {
	DestroyOptionMenu(ffs->dir_w);
	ffs->dir_w = BuildSimpleMenu(parent, "directory_label",
				     choices, XmMENU_OPTION, ffs, ffMenuCB);
	SetNthOptionMenuChoice(ffs->dir_w, n-1);
	XtManageChild(ffs->dir_w);
    }
#ifdef NOT_NOW
    /*
     * This swaps in a new pulldown without destroying and recreating
     * the option menu itself.  That should be more efficient, and it
     * should also keep torn-off menus torn off.  Unfortunately, this
     * actually tends to crash when used with torn-off menus.  I don't
     * yet understand why.
     *							-- brl
     */
    {
	Widget newMenu = BuildSimplePulldownMenu(parent, choices, ffMenuCB, ffs);
	Widget oldMenu;

	XtVaGetValues(ffs->dir_w, XmNsubMenuId, &oldMenu, 0);
	XtVaSetValues(ffs->dir_w, XmNsubMenuId,  newMenu, 0);
	SetNthOptionMenuChoice(ffs->dir_w, n-1);

 	add_deferred(ZmXtDestroyWidget, oldMenu);
    }
#endif /* NOT_NOW */

    free_vec(choices);
}

extern void remove_callback_cb();

/*
 * Create a FileFinder area.
 * The function returns an unmanaged widget that contains subwidgets.
 * The subwidgets are a label, a list widget, and a labeled text widget.
 * The list widget is named "file_list", the text widget "file_text", and the
 * label of the text widget is "file_label", and the manager is "file_finder".
 *
 *  parent       The FileFinder is a child of this widget.
 *  path	 the default path of the directory.
 *  search_func  Function to invoke if user clicked on a directory.
 *  ok_func      Function to invoke if user clicked on a file.
 *
 * NOTE: it would be nice if we could do a multiple selection box for
 * certain actions (like removing a folder).
 */
Widget
CreateFileFinder(parent, path, search_func, ok_func, client_data)
Widget parent;
char *path;
void (*search_func)(), (*ok_func)();
caddr_t client_data;
{
    Widget pane, form, dots_w, list_w, text_w, dir_w, folders_w;
#if defined( IMAP )
    Widget imap_w = (Widget) NULL;
#endif
    FileFinderStruct *ffs = XtNew(FileFinderStruct);
    char *p, *dir, buf[MAXPATHLEN], **pp, *file = NULL;
    ZmCallback zcb;
#ifdef SANE_WINDOW
    Boolean use_sane = XmIsSaneWindow(parent);
#endif /* SANE_WINDOW */

    if (!ffs)
	return 0;

#ifdef SANE_WINDOW
    if (use_sane)
	parent = XtVaCreateWidget(NULL, xmFormWidgetClass, parent,
				  XmNresizePolicy, XmRESIZE_NONE,
				  ZmNextResizable, True, 0);
#endif /* SANE_WINDOW */
    
    pane = XtVaCreateWidget("file_finder",
#ifdef SANE_WINDOW
	use_sane? zmSaneWindowWidgetClass : xmPanedWindowWidgetClass, parent,
#else /* !SANE_WINDOW */
	xmPanedWindowWidgetClass, parent,
#endif /* !SANE_WINDOW */
	XmNuserData,	ffs,
	XmNseparatorOn,	False,
	XmNsashWidth,	1,
	XmNsashHeight,	1,
#ifdef SANE_WINDOW
	use_sane? XmNtopAttachment : 0, XmATTACH_FORM,
	XmNbottomAttachment, XmATTACH_FORM,
	XmNleftAttachment, XmATTACH_FORM,
	XmNrightAttachment, XmATTACH_FORM,
#endif /* SANE_WINDOW */
	NULL);
    XtAddCallback(pane, XmNdestroyCallback, (XtCallbackProc) ffFreeFinder, ffs);
#ifdef TIMER_API
    XtAddCallback(GetTopShell(pane), XmNpopdownCallback, (XtCallbackProc) ffSuspend, ffs);
    XtAddCallback(GetTopShell(pane), XmNpopupCallback,	 (XtCallbackProc) ffResume,  ffs);
#else /* !TIMER_API */
    XtAddCallback(GetTopShell(pane), XmNpopupCallback, (XtCallbackProc) ffRefresh, ffs);
#endif /* TIMER_API */

    DialogHelpRegister(pane, "file_finder");

    form = XtVaCreateManagedWidget("controls", xmFormWidgetClass, pane,
				   XmNresizePolicy, XmRESIZE_GROW, 0);

#if defined( IMAP )
    ffs->useIMAP = 0;
    ffs->imap_w = imap_w = XtVaCreateManagedWidget("imap_toggle",
	xmToggleButtonWidgetClass, form,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNsensitive,		(using_imap && !InWriteToFile() && imap_initialized ? True : False ),
	NULL);
    XtAddCallback(imap_w, XmNvalueChangedCallback, research_dir, ffs);
#endif
    folders_w = XtVaCreateManagedWidget("folders_toggle",
	xmToggleButtonWidgetClass, form,
	XmNtopAttachment,  	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNsensitive,		True,
	NULL);
#if defined( IMAP )
	XtVaSetValues(imap_w,
		XmNrightAttachment, 	XmATTACH_WIDGET,
		XmNrightWidget,		folders_w,
		NULL);
#endif
    dots_w = XtVaCreateManagedWidget("dots_toggle",
	xmToggleButtonWidgetClass, form,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNsensitive,		True,
	NULL);
    XtVaSetValues(folders_w,
	XmNrightAttachment, 	XmATTACH_WIDGET,
	XmNrightWidget,		dots_w,
	NULL);
    XtAddCallback(dots_w, XmNvalueChangedCallback, research_dir, ffs);
    XtAddCallback(folders_w, XmNvalueChangedCallback, research_dir, ffs);

    dir_w = XtVaCreateManagedWidget(NULL, xmRowColumnWidgetClass, form,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_WIDGET,
	XmNrightWidget,		folders_w,
	XmNmarginHeight,	0,
	XmNmarginWidth,		0,
	NULL);
    dir_w = BuildSimpleMenu(dir_w, "directory_label",
	pp = unitv(value_of("folder")), XmMENU_OPTION,
		ffs, ffMenuCB);
    free_vec(pp);
    XtManageChild(dir_w);

    XtManageChild(form);

    SetPaneMaxAndMin(dir_w);	/* Should use larger of dir_w or dots_w */

    list_w = XmCreateScrolledList(pane, "file_list", NULL, 0);
    XtVaSetValues(list_w, XmNselectionPolicy, XmBROWSE_SELECT, NULL);
#ifdef SANE_WINDOW
    XtVaSetValues(XtParent(list_w), ZmNextResizable, True, NULL);
#endif /* SANE_WINDOW */
    ListInstallNavigator(list_w);
    XtManageChild(list_w);

    text_w = CreateLabeledTextForm("file", pane, NULL);
    XtVaSetValues(XtParent(text_w), XmNskipAdjust, True, NULL);
    XtVaSetValues(text_w,
	XmNresizeWidth,      False,
	XmNverifyBell,       False,
	XmNuserData,	     list_w,
	NULL);
    XtAddCallback(text_w, XmNmodifyVerifyCallback, (XtCallbackProc) filec_cb, NULL);
    XtAddCallback(text_w, XmNmotionVerifyCallback, (XtCallbackProc) filec_motion, NULL);
    XtAddCallback(text_w, XmNactivateCallback, (XtCallbackProc) ffSelectionCB, ffs);

#ifdef SANE_WINDOW
    if (use_sane)
	XtManageChild(parent);
#endif /* SANE_WINDOW */

    XtVaSetValues(list_w, XmNuserData, text_w, NULL);

    if (path)
	path = strcpy(buf, path); /* Save in case it came from getpath() */

    if (!path || !(p = last_dsep(path)))
	dir = ".", file = path;
    else {
	int n = 0;
	dir = getpath(path, &n);
	if (n == -1) {
	    /* file doesn't exist; see if the directory exists */
	    *p = 0;
	    n = 0;
	    dir = getpath(path, &n);
	    /* directory doesn't even exist */
	    if (n == -1)
		*p = SLASH, dir = ".", file = path;
	    else /* directory exists, set the file part (which doesn't) */
		file = strcpy(buf, p+1); /* this will be the prompt */
	} else if (n == 0 && (p = last_dsep(dir))) {
	    /* file exists; set the directory and file parts */
	    *p++ = 0; 	/* dir is alreay set, just terminate it */
	    file = strcpy(buf, p);
	} else
	    file = NULL;
    }
    XmToggleButtonSetState(dots_w, file && zglob(file, ".*"), False);

    bzero((char *) ffs, sizeof *ffs);
    ffs->dots_w = dots_w;
    ffs->folders_w = folders_w;
    ffs->dir_w = dir_w;
    ffs->list_w = list_w;
    ffs->text_w = text_w;
    ffs->search_func = search_func ? search_func : FileFinderDefaultSearch;
    ffs->ok_func = ok_func;
    ffs->client_data = client_data;
    ffs->dir = savestr(dir);
    ffs->root = savestr(zmailroot);
    ffs->entries = DUBL_NULL;
#if defined( IMAP )
    ffs->imap_w = imap_w;
    ffs->useIMAP = 0;
#endif

    XtAddCallback(list_w, XmNbrowseSelectionCallback, (XtCallbackProc) ffCopyDown, ffs);
    XtAddCallback(list_w, XmNdefaultActionCallback, (XtCallbackProc) ffSelectionCB, ffs);
#ifdef TIMER_API
    zcb = ZmCallbackAdd("", ZCBTYPE_CHROOT, ffChroot, ffs);
#else /* !TIMER_API */
    zcb = ZmCallbackAdd("", ZCBTYPE_CHROOT, ffUpdate, ffs);
#endif /* TIMER_API */
    XtAddCallback(list_w, XmNdestroyCallback, remove_callback_cb, zcb);
    zcb = ZmCallbackAdd(VarFilelistFmt, ZCBTYPE_VAR, ffUpdate, ffs);
    XtAddCallback(list_w, XmNdestroyCallback, remove_callback_cb, zcb);
#ifndef USE_FAM
#ifdef TIMER_API
    zcb = ZmCallbackAdd(VarTimeout, ZCBTYPE_VAR, ffFrequency, ffs);
    XtAddCallback(list_w, XmNdestroyCallback, remove_callback_cb, zcb);
#else /* !TIMER_API */
    zcb = ZmCallbackAdd("", ZCBTYPE_CHROOT, ffRefreshCB, ffs);
    XtAddCallback(list_w, XmNdestroyCallback, remove_callback_cb, zcb);
#endif /* !TIMER_API */
#endif /* !USE_FAM */

#ifdef USE_FAM
    ffs->fam.tracking = False;
    ffs->fam.closure.callback = (FAMCallbackProc) ffHandleEvent;
    ffs->fam.closure.data = ffs;
    if (fam) FAMMonitorDirectory(fam, dir, &ffs->fam.request, &ffs->fam.closure );
#else
#ifdef TIMER_API
    /* just need to create ... search_func will resume */
    ffs->timer = timer_construct((void (*)NP((VPTR, TimerId)))ffExpire, ffs, passive_timeout * 1000);
#else /* !TIMER_API */
    add_deferred(ffUpdateTimer, ffs);
#endif /* TIMER_API */
#endif /* USE_FAM */

    (*ffs->search_func)(FileFindDir, ffs, dir, file);
#ifndef TIMER_API
    ffs->timer = zm_current_time(1);
#endif /* !TIMER_API */

    return pane;
}

static void
ok_callback(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    ffs->client_data = (caddr_t)1;
}

static void
exists_callback(w, ffs, flags)
Widget w;
FileFinderStruct *ffs;
{
    int n = isoff(flags, PB_MUST_EXIST);
    char *file = XmTextGetString(ffs->text_w);
    char *tmp = FileFinderGetPath(ffs, file, &n);

    if (n < 0 || n == 1 && ison(flags, PB_NOT_A_DIR)) {
        if (n < 0)
	  error(UserErrWarning, "%s: %s", file, tmp);
        else
	  error(UserErrWarning, catgets( catalog, CAT_SHELL, 142, "\"%s\" is a directory." ), file);

	ffs->client_data = (caddr_t)0;	/* Make sure */
	/* Bart: Tue Aug 11 15:06:33 PDT 1992
	 * Hack to clear please_wait_cursor cursor after error()
	 */
	assign_cursor(frame_list, do_not_enter_cursor);
	assign_cursor(FrameGetData(ffs->dir_w), None);
    } else
	ok_callback(w, ffs);
    XtFree(file);
}

static void
cancel_callback(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    ffs->client_data = (caddr_t)2;
}

static void
research_dir(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    int *list, cnt;
    char *file = NULL;

#if defined( IMAP )
    if ( ffs->imap_w && using_imap && imap_initialized )
    	ffs->useIMAP = XmToggleButtonGetState(ffs->imap_w);
    else
	XmToggleButtonSetState(ffs->imap_w, False, False );	
    if ( using_imap && imap_initialized ) 
	XtVaSetValues( ffs->imap_w, XmNsensitive, True, NULL );	
    else
	XtVaSetValues( ffs->imap_w, XmNsensitive, False, NULL );	
    if ( ffs->useIMAP ) {
	XtVaSetValues( ffs->folders_w, XmNsensitive, False, NULL );	
	XtVaSetValues( ffs->dots_w, XmNsensitive, False, NULL );	
    }
    else {
	XtVaSetValues( ffs->folders_w, XmNsensitive, True, NULL );	
	XtVaSetValues( ffs->dots_w, XmNsensitive, True, NULL );	
    }
#endif
    if (XmListGetSelectedPos(ffs->list_w, &list, &cnt)) {
	cnt = list[0] - 1;
	/* if this is not the ".." entry, then search for it again */
	if (strlen(ffs->entries[cnt]) >= strlen(ffs->dir)) {
	    file = savestr(ffs->entries[cnt]);
	    FileFinderDefaultSearch(FileFindFile, ffs, NULL, file);
	    xfree(file);
	}
	XtFree((char *)list);
	if (file) return;
    }
    FileFinderDefaultSearch(FileFindReset, ffs, NULL, NULL);
}

#ifdef NOT_NOW
static void
search_callback(w, ffs)
Widget w;
FileFinderStruct *ffs;
{
    char *file = XmTextGetString(ffs->text_w);
    FileFinderDefaultSearch(FileFindDir, ffs, file, NULL);
    XtFree(file);
    /* Bart: Tue Oct  6 11:23:00 PDT 1992
     * Reassign the cursor explicitly in case timeout_cursors()
     * was called from error() during the search.
     */
    assign_cursor(FrameGetData(w), None);
}
#endif /* NOT_NOW */

#include "bitmaps/folder.xbm"
ZcIcon finder_icon = {
    "finder_icon", 0, folder_width, folder_height, folder_bits
};

ActionAreaItem ff_actions[] = {
    { "Ok",	ok_callback,	NULL },
    { NULL,	(void_proc)0,	NULL },
    { "Search",	ffSelectionCB,  NULL },
    { NULL,	(void_proc)0,	NULL },
    { "Cancel", cancel_callback,NULL },
};

ZmFrame
CreateFileFinderDialogFrame(name, title, pane_p)
const char *name, *title;
Widget *pane_p;
{
    Widget pane, bboard;
    ZmFrame frame;
    
    frame = FrameCreate(name,
	FrameFileFinder,    hidden_shell,
	FrameFlags,         FRAME_CANNOT_SHRINK,
	/* FrameChildClass, xmBulletinBoardWidgetClass, */
	FrameChildClass,    xmFormWidgetClass,
	FrameChild,         &bboard,
	title ? FrameTitle : FrameEndArgs, title,
	FrameEndArgs);
   XtVaSetValues(bboard,
	XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL,
	XmNresizePolicy, XmRESIZE_NONE,
	NULL);
    pane = XtVaCreateWidget(NULL, PANED_WINDOW_CLASS, bboard,
	XmNtopAttachment,	XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	XmNleftAttachment,	XmATTACH_FORM,
	XmNrightAttachment,	XmATTACH_FORM,
	XmNseparatorOn,	False,
	XmNsashWidth,	1,
	XmNsashHeight,	1,
	NULL);
    XtManageChild(pane);
    *pane_p = pane;
    return frame;
}

void
CreateFileFinderDialogTitle(pane, label_w)
Widget pane;
Widget *label_w;	/* Returns the prompt label widget */
{
    Widget form, xm_frame, w, ff;
    Pixmap pix;

    if (!label_w)
	label_w = &ff;	/* Temp space */

    form = XtCreateManagedWidget(NULL, xmFormWidgetClass, pane, NULL, 0);
    w = XtVaCreateWidget(NULL, xmLabelGadgetClass, form,
	XmNrightAttachment,		XmATTACH_FORM,
	XmNtopAttachment,		XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	NULL);
    load_icons(w, &finder_icon, 1, &pix);
    xm_frame = XtVaCreateManagedWidget(NULL, xmFrameWidgetClass, form,
	XmNrightAttachment,		XmATTACH_WIDGET,
	XmNrightWidget,		 	w,
	XmNleftAttachment,		XmATTACH_FORM,
	XmNtopAttachment,		XmATTACH_FORM,
	XmNbottomAttachment,	XmATTACH_FORM,
	NULL);
    *label_w = XtVaCreateManagedWidget("directions", xmLabelGadgetClass, xm_frame,
	XmNalignment, XmALIGNMENT_BEGINNING,
	NULL);
    XtVaSetValues(w,
	XmNlabelType,		XmPIXMAP,
	XmNlabelPixmap,		pix,
	XmNuserData,         	&finder_icon,
	NULL);
    XtManageChild(w);
    XtManageChild(form);
    {
	/* SetPaneMaxAndMin doesn't work with forms...hack */
	Widget kids[2];
	kids[0] = w;
	kids[1] = xm_frame; /* use *label_w if xm_frame doesn't work. */
	FixFormWidgetHeight(form, kids, XtNumber(kids));
    }
}

Widget
CreateFileFinderDialogFinder(pane, path, okproc)
    Widget pane;
    const char *path;
    void (*okproc)();
{
    char full[MAXPATHLEN];
    Widget ff = CreateFileFinder(pane, expandpath(path, full, FALSE),
				 (void_proc)0, okproc, NULL);
    
    XtManageChild(ff);
    return ff;
}

#if defined( IMAP )    /* XXX ugly, but had no other way */
static ZmFrame frame = (ZmFrame) 0;
#endif

ZmFrame
CreateFileFinderDialog(parent, title, path, label_w)
Widget parent;
const char *title, *path;
Widget *label_w;	/* Returns the prompt label widget */
{
#if !defined( IMAP )
    ZmFrame frame;
#endif
    FileFinderStruct *ffs;
    Widget pane, w, ff;

    frame = CreateFileFinderDialogFrame("file_finder_dialog", title, &pane);
    CreateFileFinderDialogTitle(pane, label_w);
    ff = CreateFileFinderDialogFinder(pane, path, ok_callback);
    XtVaGetValues(ff, XmNuserData, &ffs, NULL);
    SetDeleteWindowCallback(GetTopShell(pane), cancel_callback, ffs);
    ff_actions[0].data =
    ff_actions[2].data =
    ff_actions[XtNumber(ff_actions)-1].data = (caddr_t)ffs;
    w = CreateActionArea(pane, ff_actions, XtNumber(ff_actions), title);
    SetPaneMaxAndMin(w);
    FrameSet(frame, FrameClientData, ffs, FrameEndArgs);
    XtManageChild(FrameGetChild(frame));

    return frame;
}

ZmFrame
CreateFolderPromptDialog(name, title, parent, choices, options, label_w, path)
const char *name, *title;
Widget parent;
char **choices;
u_long *options;
Widget *label_w;
     char *path;
{
#if !defined( IMAP )
    ZmFrame frame;
#endif
    FileFinderStruct *ffs;
    Widget pane, w, ff, box;
    int ct;
    char **ptr;

    for (ct = 0, ptr = choices; *ptr; ct++, ptr++);
    frame = CreateFileFinderDialogFrame(name, title, &pane);
    CreateFileFinderDialogTitle(pane, label_w);
    ff = CreateFileFinderDialogFinder(pane, path, ok_callback);
    box = CreateToggleBox(pane, False, True, False, (void_proc) 0,
	options, NULL, choices, ct);
    XtVaSetValues(box, XmNskipAdjust, True, NULL);
    XtManageChild(box);
    XtVaGetValues(ff, XmNuserData, &ffs, NULL);
    SetDeleteWindowCallback(GetTopShell(pane), cancel_callback, ffs);
    ff_actions[0].data =
    ff_actions[2].data =
    ff_actions[XtNumber(ff_actions)-1].data = (caddr_t)ffs;
    w = CreateActionArea(pane, ff_actions, XtNumber(ff_actions), "");
    SetPaneMaxAndMin(w);
    FrameSet(frame, FrameClientData, ffs, FrameEndArgs);
    XtManageChild(FrameGetChild(frame));
    return frame;
}

char *
FileFinderPromptBoxLoop(frame, flags)
ZmFrame frame;
u_long flags;
{
    FileFinderStruct *ffs;
    char *tmp, *path, *p;
    int flag;
#if defined( IMAP )
    void RememberFrame();
    void * GetFoldersP();                                                       
#endif

    timeout_cursors(TRUE);
    assign_cursor(frame_list, do_not_enter_cursor);
    assign_cursor(frame, None);

    ffs = (FileFinderStruct *) FrameGetClientData(frame);
#if defined( IMAP )
    ffs->pFolder = GetFoldersP();
    RememberFrame( frame );
#endif 
    ffs->client_data = NULL;

    FramePopup(frame);
    ask_item = ffs->text_w;

    do {
	flag = 1;
	while (flag) {
	    while (ffs->client_data == NULL)
		XtAppProcessEvent(app, XtIMAll);
	    
	    if (ffs->client_data != (caddr_t)1) {
		flag = 0;
		break;
	    }
	    p = tmp = XmTextGetString(ffs->text_w);
	    if (p) while (isspace(*p)) p++;
	    if (!p || !*p) {
		ffs->client_data = NULL;
		error(UserErrWarning, catgets(catalog, CAT_SHELL, 873, "Please specify a file."));
		assign_cursor(frame, None);
	    } else
		flag = 0;
	    XtFree(tmp);
	}
	if (ison(flags, PB_MUST_EXIST|PB_NOT_A_DIR)) {
	    if (ffs->client_data == (caddr_t)1)
		exists_callback(ffs->text_w, ffs, flags);	/* Hack */
	    if (ffs->client_data)
		turnoff(flags, PB_MUST_EXIST|PB_NOT_A_DIR);
	}
    } while (ison(flags, PB_MUST_EXIST|PB_NOT_A_DIR));
    FramePopdown(frame);

    assign_cursor(frame_list, please_wait_cursor);
    timeout_cursors(FALSE);
    if (ffs->client_data == (caddr_t)1) {
	tmp = XmTextGetString(ffs->text_w);
	path = FileFinderGetPath(ffs, tmp, (int *)0);
	if (path) {
	    XtFree(tmp);
	    path = strcpy(XtMalloc((unsigned)strlen(path) + 1), path);
	} else
	    path = tmp;
	return path;
    }
    return NULL;
}

ZmFrame
ReuseFileFinderDialog(frame, path)
ZmFrame frame;
const char *path;
{
    char *name = NULL;
    Boolean dot_state = False;
    FileFinderStruct *ffs;
   
#if defined( IMAP )
    void RememberFrame();
    void *GetFoldersP();
#endif

    ffs = (FileFinderStruct *)FrameGetClientData(frame);
#if defined( IMAP )
    ffs->pFolder = GetFoldersP();
    RememberFrame( frame );
#endif 
    if (path && *path) {
	name = basename(path);
	dot_state = zglob(name, ".??*");
    }
#if defined( IMAP )
    if ( using_imap && !InWriteToFile() )
	XtVaSetValues( ffs->imap_w, XmNsensitive, True, NULL );	
    else
	XtVaSetValues( ffs->imap_w, XmNsensitive, False, NULL );	
    if ( ffs->useIMAP ) {
	XtVaSetValues( ffs->folders_w, XmNsensitive, False, NULL );	
	XtVaSetValues( ffs->dots_w, XmNsensitive, False, NULL );	
    }
    else {
	XtVaSetValues( ffs->folders_w, XmNsensitive, True, NULL );	
	XtVaSetValues( ffs->dots_w, XmNsensitive, True, NULL );	
    }
#endif
    if (name) {
	if (dot_state && !XmToggleButtonGetState(ffs->dots_w))
	    XmToggleButtonSetState(ffs->dots_w, True, False);
	else
	    dot_state = False;
    }
    /* Bart: Fri Jul 31 13:08:57 PDT 1992
     * Don't change the dialog at all when path == NULL, and
     * don't rescan it unless the dot_state has come on or
     * the file that we're looking for isn't in the ffs->dir.
     */
    if (dot_state)
	FileFinderDefaultSearch(FileFindFile, ffs, ".", path);
    else if (name)
	FileFinderSelectItem(ffs, path, True);
    return frame;
}

/* FileFinderPromptBox -- post a dialog with a file finder and
 * return the file chosen by the user or NULL if cancelled.
 */

#if defined( IMAP )    /* XXX ugly, but had no other way */

FileFinderStruct *
Getffs()
{
	if ( frame )
		return( (FileFinderStruct *) FrameGetClientData(frame) );
	else
		return( (FileFinderStruct *) NULL );
}

void
RememberFrame( fr )
ZmFrame fr;
{
	frame = fr;
}

void	
ClearUseIMAP()
{
	FileFinderStruct *ffs;

	if ( frame ) {
		ffs = Getffs();
		if ( ffs )
			ffs->useIMAP = 0;
	}
}

void	
ClearFoldersP()
{
	FileFinderStruct *ffs;

	if ( frame ) {
		ffs = Getffs();
		if ( ffs )
			ffs->pFolder = (void *) NULL;
	}
}

int
UseIMAP()
{
	return( GetUseIMAP() );
}

int
GetUseIMAP()
{
	FileFinderStruct *ffs;
	int	retval = 0;

	if ( frame ) {
		ffs = Getffs();
		if ( ffs )
			retval = ffs->useIMAP;
	}
	return( retval );
}

void
SetFolderParent( p )
void *p;
{
	FileFinderStruct *ffs;

	if ( frame ) {
		ffs = Getffs();
		if ( ffs ) {
			ffs->pFolder = GetParent( p );
		}
	}	
}

void *
GetFoldersP()
{
	FileFinderStruct *ffs;
	void	*p = (void *) NULL;

	if ( frame ) {
		ffs = Getffs();
		if ( ffs )
			p = ffs->pFolder;
	}
	return( p );
}

#endif /* IMAP */

char *
FileFinderPromptBox(parent, label, path, flags)
Widget parent;
const char *label;
const char *path;
u_long flags;
{
    static ZmFrame frame = (ZmFrame) 0;
    static Widget label_w;
    FileFinderStruct *ffs;

    if (!frame)
	frame = CreateFileFinderDialog(parent, NULL, path, &label_w);
    else
	ReuseFileFinderDialog(frame, path);
    XtVaSetValues(label_w, XmNlabelString, zmXmStr(label), NULL);
    ffs = (FileFinderStruct *) FrameGetClientData(frame);
    if (isoff(flags, PB_FOLDER_TOGGLE)) {
	XtVaSetValues(XtParent(ffs->dir_w),
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, ffs->dots_w,
	    NULL);
	XtUnmanageChild(ffs->folders_w);
    } else {
	XtVaSetValues(XtParent(ffs->dir_w),
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, ffs->folders_w,
	    NULL);
	XtManageChild(ffs->folders_w);
    }

    return FileFinderPromptBoxLoop(frame, flags);
}

#if defined ( IMAP )
void
FinderRedraw(ffs)
FileFinderStruct *ffs;
{
	if ( !ffs )
		ffs = Getffs();
	if ( ffs )
		ffUpdate(ffs);
}
#endif

