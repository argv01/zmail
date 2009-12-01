#ifndef INCLUDE_FINDER_H
#define INCLUDE_FINDER_H

#include <general.h>
#include <ztimer.h>
#include <Xm/List.h>

#include "osconfig.h"
#include "config/features.h"
#include "gui_def.h"
#include "zcunix.h"
#include "zmframe.h"
#ifdef USE_FAM
#include "zm_fam.h"
#endif /* USE_FAM */


typedef struct {
    void (*search_func)(), (*ok_func)();
    GuiItem dir_w, list_w, text_w, dots_w;
#if defined( IMAP )
    GuiItem imap_w;
#endif
    caddr_t client_data;
    char *dir;		/* directory in this file-finder */
    char **entries;	/* file entries displayed */
    char *root; 	/* value of zmailroot */
    GuiItem folders_w;
#if defined( IMAP )
    char useIMAP;
    char *imapdir;
    void *pFolder;
#endif

#ifdef USE_FAM
    struct {
	Boolean tracking;
	FAMRequest request;
	FAMClosure closure;
    } fam;
#else /* !USE_FAM */
#ifdef TIMER_API
    TimerId timer;
#else /* !TIMER_API */
    time_t timer;
#endif /* TIMER_API */
#endif /* USE_FAM */
} FileFinderStruct;

typedef enum {
    FileFindDir,
    FileFindFile,
    FileFindReset
} FileFinderType;


extern char *FileFinderGetFullPath P((Widget));
extern char *FileFinderGetFullPathEx P((Widget, int *));
extern char *FileFinderGetPath P((FileFinderStruct *, char *, int *));
extern char *FileFinderPromptBox P((Widget, const char *, const char *, u_long));
extern char *FileFinderSummarize P((char *, struct stat *));
extern void FileFinderDefaultSearch P((FileFinderType, FileFinderStruct *, const char *, const char *));
extern void FileFinderSelectItem P((FileFinderStruct *, const char *, int));
extern char *FileFinderPromptBoxLoop P((ZmFrame, u_long));

extern ZmFrame ReuseFileFinderDialog P((ZmFrame, const char *));
extern ZmFrame CreateFileFinderDialogFrame P((const char *, const char *, Widget *));

extern void ffSelectionCB P((Widget, FileFinderStruct *, XmListCallbackStruct *));


#ifdef USE_FAM
#define FileFinderExpire(finder)  (0L)
#else /* !USE_FAM */
#ifdef TIMER_API
#define FileFinderExpire(finder)  (timer_trigger((finder).timer))
#else /* !TIMER_API */
#define FileFinderExpire(finder)  ((finder).timer = 0)
#endif /* !TIMER_API */
#endif /* !USE_FAM */

#endif /* !INCLUDE_FINDER_H */
