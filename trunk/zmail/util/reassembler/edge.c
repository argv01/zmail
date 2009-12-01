#include <dpipe.h>
#include <glist.h>
#include "feeder.h"
#include "edge.h"
#include "options.h"


static struct glist locations;


void
edge_init()
{
    if (externSize)
	glist_Init(&locations, sizeof(struct Location), 4);
}


void
edge_add(const struct dpipe *source, const struct FeederState *state)
{
    if (externSize) {
	struct Location newLocation;
	newLocation.sequence = state->sequence;
	newLocation.offset   = ftell(state->file) - dpipe_Ready(source);

	glist_Add(&locations, &newLocation);
    }
}


int
edge_large(unsigned partNumber)
{
    if (externSize) {
	const struct Location * const start = (struct Location *) glist_Nth(&locations, partNumber);
	const struct Location * const end   = (struct Location *) glist_Nth(&locations, partNumber + 1);

	return start->sequence != end->sequence || end->offset - start->offset > 1024;

    } else
	return 0;
}
