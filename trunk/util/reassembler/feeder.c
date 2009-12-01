#include <dpipe.h>
#include <dputil.h>
#include <dynstr.h>
#include <excfns.h>
#include <limits.h>
#include <mime-api.h>
#include <stdio.h>
#include <sys/param.h>
#include <unistd.h>

#include "fatal.h"
#include "feeder.h"
#include "options.h"


/*
 * This bit is quite Unix'y, but I don't feel like setting up a whole
 * dpipe just to use mime_Readline or mime_Headers for the skipping.
 */

void
skip_headers(FILE *source)
{
  unsigned short newlines = 0;

  do
    {
      switch (getc(source))
	{
	case '\n':
	  newlines++;
	  break;
	case EOF:
	  return; /* ! */
	default:
	  newlines = 0;
	}
    }
  while (newlines < 2);
}


static void
next_fragment(struct dpipe *source, struct FeederState *state)
{
    char filename[PATH_MAX];
    if (state->file) {
	fclose(state->file);

	if (destructive && state->destructive) {
	    sprintf(filename, "%s/%u", state->directory, state->sequence);
	    unlink(filename);
	}
    }
    
    sprintf(filename, "%s/%u", state->directory, ++state->sequence);
  
    if (state->file = state->sequence == 1 ? efopen(filename, "r", WHERE(next_fragment)) : fopen(filename, "r"))
	if (state->sequence > 1) skip_headers(state->file);
}


void
feeder(struct dpipe *source, struct FeederState *state)
{
    if (!state->file || feof(state->file))
	next_fragment(source, state);

    if (state->file) {
	{
	    char * const transfer = emalloc(BUFSIZ, WHERE(feeder));
	    const int count = efread(transfer, 1, BUFSIZ, state->file, WHERE(feeder));
	    
	    if (count > 0)
		dpipe_Put(source, transfer, count);
	    else
		dpipe_Close(source);
	}
    } else
	dpipe_Close(source);
}
