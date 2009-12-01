extern "C" {
#include "../prune.h"
#include "attach.h"
#include "catalog.h"
#include "error.h"
#include "fileicon.h"
#include "mime.h"
#include "oz.h"
#include "zmalloc.h"
#include "zmflag.h"
#undef werase
}

#include <DList.hh>
#include "MapTable.hh"


const char * const IrixTypeParam = "x-irix-type";

class IconDatabase {
private:
  const fiFileIconDB pointer;

public:
  IconDatabase(const char *path = OZ_DATABASE)
    : pointer(fiLoadDB(path))
  {
  }

  ~IconDatabase()
  {
    if (pointer) fiFreeDB(pointer);
  }

  operator fiFileIconDB()
  {
    return pointer;
  }
};

class IconType {
private:
  const fiIconType pointer;

public:
  IconType(fiFileIconDB database, const char *typeName)
    : pointer(database && typeName ?
	      fiIconTypeFromTypeName(database, typeName) : 0)
  {
  }

  IconType(const fiIconType original)
    : pointer(original)
  {
  }

  ~IconType()
  {
    if (pointer) fiFreeIconType(pointer);
  }

  operator fiIconType()
  {
    return pointer;
  }
};

static IconDatabase database;

static MapTable irixToMime(31);
static MapTable mimeToIrix(31);


void
parse_attach_irix2mime(int argc, char **argv, int)
{
  switch (argc)
    {
    case 1:
      error(UserErrWarning, catgets(catalog, CAT_AUTOTYPE, 1, "%s: missing both desktop type and MIME type"), argv[0]);
      break;
      
    case 2:
      error(UserErrWarning, catgets(catalog, CAT_AUTOTYPE, 2, "%s: no MIME type given for %s"), argv[0], argv[1]);
      break;
      
    default:
      if (argc == 3 || argv[3][0] == '#')
	{
	  if (IconType(database, argv[1]))
	    irixToMime.add(argv[1], argv[2]);
	}
      else
	error(UserErrWarning, catgets(catalog, CAT_AUTOTYPE, 3, "%s: too many arguments for mapping from %s to %s"), argv[0], argv[1], argv[2]);
    }
  
  free_elems(argv);
}


void
parse_attach_mime2irix(int argc, char **argv, int)
{
  switch (argc)
    {
    case 1:
      error(UserErrWarning, catgets(catalog, CAT_AUTOTYPE, 4, "%s: missing both MIME type and desktop type"), argv[0]);
      break;
      
    case 2:
      error(UserErrWarning, catgets(catalog, CAT_AUTOTYPE, 5, "%s: no desktop type given for %s"), argv[0], argv[1]);
      break;
      
    default:
      if (argc == 3 || argv[3][0] == '#')
	{
	  if (IconType(database, argv[2]))
	    mimeToIrix.add(argv[1], argv[2]);
	}
      else
	error(UserErrWarning, catgets(catalog, CAT_AUTOTYPE, 6, "%s: too many arguments for mapping from %s to %s"),
	      argv[0], argv[1], argv[2]);
    }

  free_elems(argv);
}


const char *
autotype_via_oz(const char *filename)
{
  if (database)
    {
      IconType icon = fiIconTypeFromFileName(database, filename);
      const char * const irix = fiTypeName(icon);
      DList<const char *> fringe(4);
      fringe.append(irix);
      const char *mime = NULL;
      Boolean nontrivial = False;
      unsigned cutoff = 16802; // heuristic cutoff to avoid loops

      // your basic breadth-first search with cutoff
      while (!mime && !fringe.emptyP() && --cutoff)
	if (!(mime = irixToMime.find(fringe.nth(fringe.head()))))
	  {
	    nontrivial = True;
	    IconType frontier(database, fringe.nth(fringe.head()));
	    fringe.remove(0);
	    if (frontier)
	      for (fiStringList supers = fiSuperTypes(frontier); supers; supers = fiNextString(supers))
		fringe.append(fiString(supers));
	  }
      
      if (nontrivial)
	// cache the result for next time, even on failure
	irixToMime.add(irix, mime, True);
      
      return mime;
    }
  else
    return NULL;
}



static const char *
sender_suggestion(const struct Attach &part)
{
  const char * const suggestion = FindParam(IrixTypeParam, &part.content_params);
  if (IconType(database, suggestion))
    return suggestion;
  else
    return 0;
}
  


const char *
guess_desk_type(const struct Attach &part)
{
  const char *suggestion;

  if (ison(part.a_flags, AT_DELETE) || pruned(&part))
    return "DanglingLink";
  else if (suggestion = sender_suggestion(part))
    return suggestion;      
  else
    {
      get_attach_keys(0, 0, 0);	// make sure config has been read in

      const char *mime = attach_data_type(&part);
      const char *match = mimeToIrix.find(mime);
      if (!match)
	{
	  const char * subtype = strchr(mime, '/');
	  if (subtype)
	    {
	      const char unknown[] = "*";
	      char * const generic = new char[subtype - mime + sizeof(unknown) + 1];
	      sprintf(generic, "%.*s/%s", subtype - mime, mime, unknown);
	      match = mimeToIrix.find(generic);
	      delete[] generic;
	    }
	}

      return match ? match : "Unknown";
    }
}


void
free_attach_oz()
{
  mimeToIrix.erase();
  irixToMime.erase();
}


void
save_attach_oz(FILE * const file)
{
  putc('\n', file);
  {
    HashTab<Mapping>::Iterator iterator(mimeToIrix);

    for ( ; iterator; iterator.next())
      fprintf(file, "MIME2IRIX %s %s\n", iterator->source, iterator->destination);
  }
  putc('\n', file);
  {
    const char * const prefix[] = { "", "# " };
    HashTab<Mapping>::Iterator iterator(irixToMime);
    
    for ( ; iterator; iterator.next())
      if (iterator->destination)
	fprintf(file, "%sIRIX2MIME %s %s\n", prefix[iterator->cached], iterator->source, iterator->destination);
  }
}
