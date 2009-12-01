/* fonts_rc.c 	Copyright 1993 Z-Code Software Corp. */

/*
 * Simple garbage collection for fonts
 * -----------------------------------
 *
 * To quote from XmFontListEntryCreate(3X):
 *
 *    The toolkit does not copy the X Font structure specified by the
 *    font argument.  Therefore, an application programmer must not
 *    free XFontStruct or XFontSet until all font lists and/or font
 *    entries that reference it have been freed.
 *
 * So that's what we try to do here.  Under Motif 1.1, we were able to
 * store a reference count in each XFontStruct's ext_data field.  Neat
 * idea, but does not work with opaque XFontSets and XmFontLists under
 * X11R5, Motif 1.2, and beyond.
 *
 * Instead, we exploit the fact that XFontStructs are always
 * manipulated by reference (well, pointer) and XFontSets and
 * XmFontLists already are pointers.  We use these pointers to index
 * into a hash table.  Reference counts are stored in that hash table,
 * and updated whenever we shuffle things around, such as from the
 * fonts dialog.
 */


#include "osconfig.h"
#include "gui_def.h"
#include "fonts_gc.h"
#include <general.h>
#include <hashtab.h>
#include <Xm/Xm.h>


/* Hash table behavior -- fairly arbitrary, and should be tuned. */
enum { Buckets = 17 };


struct reference
{
  unsigned int count;
  XmFontList list;
};

static struct hashtab *references = 0;


/*
 * Mash a reference into a hashable unsigned integer
 *
 * Start with the XFontSet or XFontStruct pointer.  Based on ALIGNMENT
 * macro, strip off the low couple of bits that are probably always
 * the same.  Then, by casting, strip off enough high bits to fit in
 * an unsigned int.  These high bits, if any, are generally
 * uninteresting anyway since all of the pointers are to the same
 * broad region: the heap.
 */

static unsigned int
reference_hash( reference )
     const struct reference *reference;
{
  return ((unsigned int) reference->list) / ALIGNMENT;
}
  

/* Test two references for inequality */

static int
reference_compare( alpha, beta )
     const struct reference *alpha, *beta;
{
  return alpha->list != beta->list;
}


/*
 * Add one reference to an XFontSet or XFontStruct
 *
 * First look for a prior reference.  If there was one, increment its
 * count.  If there was no prior reference, generate and add a new
 * entry initialized to one counted reference.
 */

void
reference_add( list )
     XmFontList list;
{
  struct reference *prior = 0;
  struct reference probe;
  probe.list = list;

  if (prior = (struct reference *) hashtab_Find( references, &probe ))
    prior->count++;
  else
    {
      probe.count  = 1;
      hashtab_Add( references, &probe );
    }
}



/*
 * Remove one reference to an XFontSet or XFontStruct
 *
 * Locate the prior reference, and reduce its reference count.  If
 * there are no remaining references, free the unneeded object.  The
 * stored flavor is used to select the appropriate deallocator.
 */

void
reference_remove( list )
     XmFontList list;
{
  struct reference *chaff;
  struct reference probe;
  probe.list = list;

  if ((chaff = (struct reference *) hashtab_Find( references, &probe )) && --chaff->count == 0)
    {
      hashtab_Remove( references, 0 );
      XmFontListFree( chaff->list );
    }
}



/*
 * Initialize the reference counter package
 *
 * Must be called at least once before reference_{add,remove}.
 * Safe to call multiple times.
 */

void
reference_init()
{
  if (!references)
    hashtab_Init( references = (struct hashtab *) malloc( sizeof( struct hashtab ) ),
		 (unsigned int (*) P((CVPTR))) reference_hash,
		 (int (*) P ((CVPTR, CVPTR))) reference_compare,
		 sizeof( struct reference ), Buckets );
}
