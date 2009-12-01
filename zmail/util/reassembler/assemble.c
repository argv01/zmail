#include <dpipe.h>
#include <stdio.h>
#include <unistd.h>

#include "assemble.h"
#include "decode.h"
#include "edge.h"
#include "feeder.h"
#include "files.h"
#include "options.h"
#include "sweepers.h"


void
assemble(const char *directory)
{
    edge_init();
    {
	struct dpipe source;
	struct FeederState state = { 0, 0, 0, 0 };
	state.directory = directory;
	dpipe_Init(&source, 0, 0, (dpipe_Callback_t) feeder, &state, 0);
    
	skim(&source, &state);
    
	dpipe_Destroy(&source);
    }

    {
	struct dpipe source, destination;
	struct FeederState state = { 0, 0, 0, 1 };
	FILE *delivery = popen_delivery();

	state.directory = directory;
	dpipe_Init(&destination, (dpipe_Callback_t) dumper, delivery, 0, 0, 1);
	dpipe_Init(&source, 0, 0, (dpipe_Callback_t) feeder, &state, 0);

	merge(&source, &destination, &state);

	dpipe_Destroy(&source);
	dpipe_Flush(&destination);
	dpipe_Close(&destination);
	pclose(delivery);
	dpipe_Destroy(&destination);
    }
}
