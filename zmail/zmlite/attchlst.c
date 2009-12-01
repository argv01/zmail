/*
 * $RCSfile: attchlst.c,v $
 * $Revision: 2.17 $
 * $Date: 2005/05/31 07:36:42 $
 * $Author: syd $
 */

#include <spoor.h>
#include <attchlst.h>
#include <composef.h>
#include <msgf.h>
#include <spoor/listv.h>
#include <spoor/list.h>
#include <spoor/wclass.h>
#include <dynstr.h>
#include <intset.h>
#include <zmail.h>
#include <zmlite.h>
#include <glob.h>
#include "fsfix.h"

#include "catalog.h"

#ifndef lint
static const char attachlist_rcsid[] =
    "$Id: attchlst.c,v 2.17 2005/05/31 07:36:42 syd Exp $";
#endif /* lint */

struct spWclass *attachlist_class = (struct spWclass *) 0;

static Attach *getattachments(struct attachlist *self);

static int
detach_images_and_html_if_necessary(self,i)
struct attachlist *self;
int i;
{
    int x = 0;
    char buf[256], name[256];
    Attach *attachments, *a;
    Attach *hold_a;
    Boolean we_have_html , we_have_images_or_html;
    char *the_type;
    char *path;
    FILE *xreffile;
 
    a = getattachments(self);
    attachments = a;
    strcpy(name,"");
/*
  If we have no attachments then return.
*/
    if (!a) {
        return(0);
    }
    hold_a = a;
/*
  The first time through we look to see if we are viewing html and if there
  are images or html included as attachments.
*/
    we_have_html = False;
    we_have_images_or_html = False;
    do  {
        the_type = attach_data_type(a) ? attach_data_type(a) :
                a->content_type ? a->content_type : "unknown";
/*
  If the type is html and this is what we are going to display then we
  are displaying html.
*/
        if ((i == x) && (ci_strcmp(the_type,"text/x-html") == 0))
          {
            we_have_html = True;
            if (a->content_name)
                (void) strcpy(name, basename(a->content_name));
            else if (a->a_name)
                (void) strcpy(name, basename(a->a_name));
            else
                (void) sprintf(name, "Attach.%d" , x);
          }
/*
  Else if the type is image or html then we found an attachment that may be
  linked the the html we found may have found.
*/
        else
          {
            if (ci_strncmp(the_type,"image/",strlen("image/")) == 0)
              we_have_images_or_html = True;
            if (ci_strncmp(the_type,"text/x-html",strlen("text/x-html")) == 0)
              we_have_images_or_html = True;
          }
        x++;
    } while ((a = (Attach *)a->a_link.l_next) != attachments);
/*
  If we are not viewing html or if we are and the other attachments are not
  images or html then return.
*/
  if (!we_have_html)
    return(x);
  if (!we_have_images_or_html)
    return(x);
/*
  There are images or html along with the html so detach them and write
  out a table of the names to be used later to replace the locations of
  the in the html.
*/
    path = get_detach_dir();
    sprintf(buf,"%s/%s.xrf",path,name);
    xreffile = fopen(buf,"w");
    if (xreffile == NULL) {
	error(UserErrWarning, "Cannot open html xref file");
	return;
    }
    a = hold_a;
    x = 0;
    do  {
        if (x != i)
          {
            the_type = attach_data_type(a) ? attach_data_type(a) :
                a->content_type ? a->content_type : "unknown";
            if ((ci_strncmp(the_type,"image/",strlen("image/")) == 0) ||
                (ci_strncmp(the_type,"text/x-html",strlen("text/x-html")) == 0))
              {
                if (a->content_name)
                    (void) strcpy(name, basename(a->content_name));
                else if (a->a_name)
                    (void) strcpy(name, basename(a->a_name));
                else
                    (void) sprintf(name, "Attach.%d" , x);
                if (xreffile != NULL)
                  fprintf(xreffile,"%s %s %s/%s\n",the_type,name,path,name);
	        ZCommand(zmVaStr("detach -part %d -T -O", x), zcmd_ignore);
              }
          }
        x++;
    } while ((a = (Attach *)a->a_link.l_next) != attachments);
    if (xreffile != NULL)
      fclose(xreffile);
  return(x);
}

static void
do_display(self)
    struct attachlist *self;
{
    struct intset_iterator isi;
    int *sel, u;

    if (intset_EmptyP(spListv_selections(self->attachments))) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 51, "Select an attachment to display"));
	return;
    }
    intset_InitIterator(&isi);
    while (sel = intset_Iterate(spListv_selections(self->attachments), &isi)) {
	if (spoor_IsClassMember(spIm_view(ZmlIm),
				(struct spClass *) zmlmsgframe_class)) {
	    u = zmlmsgframe_uniquifier(spIm_view(ZmlIm));
	} else {
	    u = -1;
	}
        detach_images_and_html_if_necessary(self,*sel+1);
	wprint(catgets(catalog, CAT_LITE, 905, "Displaying attachment #%d...\n"), *sel+1);
	ZCommand(zmVaStr("detach -display -part %d", *sel + 1), zcmd_ignore);
	if (u >= 0) {
	    if (!spoor_IsClassMember(spIm_view(ZmlIm),
				     (struct spClass *) zmlmsgframe_class)
		|| (u != zmlmsgframe_uniquifier(spIm_view(ZmlIm)))) {
		spSend(self, m_dialog_deactivate, dialog_Close);
		return;		/* abort after displaying a new
				 * message (viz. rfc822 attachment) */
	    }
	}
    }
}

static void
do_preview(self)
    struct attachlist *self;
{
    struct intset_iterator isi;
    int *sel;

    if (intset_EmptyP(spListv_selections(self->attachments))) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 51, "Select an attachment to display"));
	return;
    }
    intset_InitIterator(&isi);
    while (sel = intset_Iterate(spListv_selections(self->attachments), &isi)) {
	ZCommand(zmVaStr("compcmd preview %d", *sel + 1), zcmd_ignore);
    }
}

static void
aa_preview(b, self)
    struct spButton *b;
    struct attachlist *self;
{
    do_preview(self);
}

static void
aa_display(b, self)
    struct spButton *b;
    struct attachlist *self;
{
    do_display(self);
}

static void
aa_done(b, self)
    struct spButton *b;
    struct attachlist *self;
{
    spSend(self, m_dialog_deactivate, dialog_Close);
}

static Attach *
getattachments(self)
    struct attachlist *self;
{
    if (self->composep)
	return (zmlcomposeframe_comp(spIm_view(ZmlIm))->attachments);
    return (((Msg *) spSend_p(spIm_view(ZmlIm), m_zmlmsgframe_msg))->m_attach);

}

static int
recompute(self)
    struct attachlist *self;
{
    Attach *a;
    char **vec = 0;
    int n;

    spSend(spView_observed(self->attachments), m_spText_clear);
    a = getattachments(self);
    n = list_attachments(a, (Attach *)NULL, &vec, 1, self->composep);
    if (n > 0) {
	int i;
	char *p;

	for (i = 0; i < n; ++i) {
	    if (p = index(vec[i], '\n'))
		*p = '\0';
	    spSend(spView_observed(self->attachments), m_spList_append,
		   vec[i]);
	    if (p)
		*p = '\n';	/* totally unnecessary */
	}
    }
    free_vec(vec);
    return (n);
}

static void
aa_unattach(b, self)
    struct spButton *b;
    struct attachlist *self;
{
    struct intset_iterator isi;
    int *sel;
    int numdeleted = 0;

    if (intset_EmptyP(spListv_selections(self->attachments))) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 53, "Select an attachment to unattach"));
	return;
    }
    intset_InitIterator(&isi);
    while (sel = intset_Iterate(spListv_selections(self->attachments), &isi)) {
	ZCommand(zmVaStr("compcmd attach:off %d", *sel + 1 - numdeleted),
		 zcmd_ignore);
	++numdeleted;
    }
    if (!recompute(self))
	spSend(self, m_dialog_deactivate, dialog_Close);
}

static char *
defaultname(part)
    Attach *part;
{
    static struct dynstr d;
    static int initialized = 0;
    char *name, *dd;

    if (!initialized) {
	dynstr_Init(&d);
	initialized = 1;
    } else {
	dynstr_Set(&d, "");
    }
    name = (part->a_name ? part->a_name : part->content_name);
    if (!name)
	name = "noname";
    if (is_fullpath(name)) {
	dynstr_Set(&d, name);
    } else if (dd = get_detach_dir()) {
	dynstr_Set(&d, dd);
	dynstr_AppendChar(&d, '/');
	dynstr_Append(&d, name);
    } else {
	dynstr_Set(&d, name);
    }
    return (dynstr_Str(&d));
}

static void
aa_save(b, self)
    struct spButton *b;
    struct attachlist *self;
{
    Attach *a = getattachments(self), *part;
    struct intset_iterator isi;
    int *sel;
    struct dynstr d;

    if (intset_EmptyP(spListv_selections(self->attachments))) {
	error(UserErrWarning, catgets(catalog, CAT_LITE, 54, "Select an attachment to save"));
	return;
    }
    intset_InitIterator(&isi);
    dynstr_Init(&d);
    TRY {
	while (sel = intset_Iterate(spListv_selections(self->attachments),
				    &isi)) {
	    dynstr_Set(&d, "");
	    part = lookup_part(a, *sel + 1, 0);
	    if (dyn_choose_one(&d,
			       part->content_name && isoff(part->a_flags, AT_NAMELESS) ?
			         zmVaStr(catgets(catalog, CAT_LITE, 55, "Select file for attachment \"%s\""),
					 part->content_name)
			       : zmVaStr(catgets(catalog, CAT_LITE, 867, "Select file for attachment #%d"),
					 *sel + 1),
			       defaultname(part), 0, 0,
			       PB_FILE_BOX | PB_NOT_A_DIR))
		break;
	    ZCommand(zmVaStr("detach -part %d '%s'", *sel + 1,
			     quotezs(dynstr_Str(&d), '\'')),
		     zcmd_commandline);
	}
    } FINALLY {
	dynstr_Destroy(&d);
    } ENDTRY;
}

static void
attachments_cb(self, which, clicktype)
    struct spListv *self;
    int which;
    enum spListv_clicktype clicktype;
{
    struct attachlist *al = (struct attachlist *) spView_callbackData(self);

    if (clicktype != spListv_doubleclick)
	return;
    if (al->composep)
	do_preview(al);
    else
	do_display(al);
}

char attachlist_err_BadContext[] = "attachlist_err_BadContext";
char attachlist_err_NoAttachments[] = "attachlist_err_NoAttachments";

static void
attachlist_initialize(self)
    struct attachlist *self;
{
    if (self->composep =
	spoor_IsClassMember(spIm_view(ZmlIm),
			    (struct spClass *) zmlcomposeframe_class)) {
	spSend(self, m_spView_setWclass, spwc_ComposeAttachlist);
    } else if (spoor_IsClassMember(spIm_view(ZmlIm),
				   (struct spClass *) zmlmsgframe_class)) {
	spSend(self, m_spView_setWclass, spwc_MessageAttachlist);
    } else {
	RAISE(attachlist_err_BadContext, 0);
    }

    spSend(self->attachments = spListv_NEW(), m_spView_setObserved,
	   spList_NEW());

    if (!recompute(self)) {
	spSend(self->attachments, m_spView_destroyObserved);
	spoor_DestroyInstance(self->attachments);
	RAISE(attachlist_err_NoAttachments, 0);
    }

    ZmlSetInstanceName(self, "attach", self);
    ZmlSetInstanceName(self->attachments, "attach-list", self);

    spListv_callback(self->attachments) = attachments_cb;
    spView_callbackData(self->attachments) = (struct spoor *) self;

    spSend(self, m_spWrapview_setLabel, catgets(catalog, CAT_LITE, 56, "Attachments"), spWrapview_top);
    spWrapview_boxed(self) = 1;
    spWrapview_highlightp(self) = 1;

    dialog_MUNGE(self) {
	if (self->composep)
	    spSend(self, m_dialog_setActionArea,
		   ActionArea(self,
			      catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			      catgets(catalog, CAT_LITE, 58, "Display"), aa_preview,
			      catgets(catalog, CAT_LITE, 59, "Unattach"), aa_unattach,
			      0));
	else
	    spSend(self, m_dialog_setActionArea,
		   ActionArea(self,
			      catgets(catalog, CAT_LITE, 24, "Done"), aa_done,
			      catgets(catalog, CAT_LITE, 58, "Display"), aa_display,
			      catgets(catalog, CAT_LITE, 62, "Save"), aa_save,
			      0));
	spSend(self, m_dialog_setView, self->attachments);
    } dialog_ENDMUNGE;
    spSend(self, m_dialog_addFocusView, self->attachments);
    ZmlSetInstanceName(dialog_actionArea(self), "attach-aa", self);

    spSend(self->attachments, m_spView_invokeInteraction,
	   "list-click", 0, 0, 0);
}

static void
attachlist_finalize(self)
    struct attachlist *self;
{
    spSend(self, m_dialog_setView, 0);
    spSend(self->attachments, m_spView_destroyObserved);
    spoor_DestroyInstance(self->attachments);
}

struct spWidgetInfo *spwc_Attachlist = 0;
struct spWidgetInfo *spwc_MessageAttachlist = 0;
struct spWidgetInfo *spwc_ComposeAttachlist = 0;

void
attachlist_InitializeClass()
{
    if (!dialog_class)
	dialog_InitializeClass();
    if (attachlist_class)
	return;

    attachlist_class =
	spWclass_Create("attachlist", 0,
			(struct spClass *) dialog_class,
			(sizeof (struct attachlist)),
			attachlist_initialize,
			attachlist_finalize,
			spwc_Attachlist = spWidget_Create("Attachlist",
							  spwc_Popup));

    spwc_ComposeAttachlist = spWidget_Create("ComposeAttachlist",
					     spwc_Attachlist);
    spwc_MessageAttachlist = spWidget_Create("MessageAttachlist",
					     spwc_Attachlist);

    zmlcomposeframe_InitializeClass();
    spListv_InitializeClass();
    spList_InitializeClass();
    zmlmsgframe_InitializeClass();
}
