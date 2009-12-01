/* charsets.c	Copyright 1993 Z-Code Software Corporation */

#ifndef lint
static char	charsets_rcsid[] = "$Id: charsets.c,v 2.6 1995/09/07 06:02:57 liblit Exp $";
#endif

#include "osconfig.h"
#include <Xm/Xm.h>
#include <except.h>
#include "catalog.h"
#include "charsets.h"
#include "zcstr.h"
#include "strcase.h"

#if defined(XmVersion) && XmVersion >= 1002 /* Motif 1.2 or later */

static XmFontListEntry
entry_for_charset( fontlist, charset )
     XmFontList fontlist;
     const char *charset;
{
  XmFontContext context;
  XmFontListEntry entry;
  Boolean searching = True;
  
  ASSERT( XmFontListInitFontContext( &context, fontlist ), "Xm",
	 catgets(catalog, CAT_MOTIF, 866, "unable to initialize font context") );
  
  while (searching && (entry = XmFontListNextEntry( context )) != NULL)
    {
      register char *tag =XmFontListEntryGetTag( entry );
      if (tag && !ci_strcmp( tag, charset ))
	searching = False;
    }

  XmFontListFreeFontContext( context );
  return entry;
}


static XmFontList
sublist_for_charset( list, charset )
     XmFontList list;
     const char *charset;
{
  XmFontListEntry entry = entry_for_charset( list, charset );
  
  return entry ? XmFontListAppendEntry( NULL, entry ) : XmFontListCopy( list );
}

#else /* Motif 1.2 or later */

/* We can actually do the same sort of search for pre-1.2, when we
 * have time to implement it.  The Motif API is somewhat different,
 * though.  There is no XmFontListEntry type, and therefore no
 * XmFontListAppendEntry() or XmFontListNextEntry() functions.  You
 * need to use XmFontStruct, XmFontListGetNextFont(), and
 * XmFontListAdd() instead, in a slightly different way.
 * 
 * Also, font sets don't exist until 1.2; all font lists are lists of
 * simple fonts.  That means that the ":<label>" notation in
 * app-defaults needs to change to "=<label>", and no semicolons are
 * allowed.  It also (probably) means that multifont encodings like
 * Japanese cannot be displayed properly.  =(
 */

#define sublist_for_charset( list, charset )  (XmFontListCopy( list ))

#endif /* Motif 1.2 or later */

void
restrict_to_charset( widget, charset )
     Widget widget;
     const char *charset;
{
  if (charset)
    {
      XmFontList broad = NULL;
      XtVaGetValues( widget, XmNfontList, &broad, NULL );
      XtVaSetValues( widget, XmNfontList,
		    sublist_for_charset( broad, charset ), NULL );
    }
}
