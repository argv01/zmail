/* zpopsync.c   Copyright 1995 Z-Code Software, a Divison of NCD */

#include <stdio.h>

#ifdef ZPOP_SYNC

#include <zmail.h>
#include <zctype.h>
#include <getpath.h>
#include <v7folder.h>
#include <v7msg.h>
#include <ghosts.h>
#include <vars.h>
#include <callback.h>
#include <spoor.h>
#include "zpopsync.h"
#include "zpopfldr.h"
#include "zpopmsg.h"

static char zpop_initialized;

time_t afterlife;

/* This is not yet handling ghosts for folders other than the spool */

void
bury_ghosts(fldr)
    msg_folder *fldr;
{
    struct v7folder *v7;
    int isdir = ZmGP_IgnoreNoEnt;
    char *tomb = getpath(value_of(VarTombfile), &isdir);

    if (! zpop_initialized)
	zpop_initialize();

    if (afterlife == 0)
	return;

    switch (isdir) {
	case ZmGP_File:
	    ghost_OpenTomb(tomb);
	    v7 = core_to_mfldr(fldr);
	    TRY {
		mfldr_BuryGhosts(v7); /* Seals the tomb */
	    } FINALLY {
		mfldr_Destroy(v7);
	    } ENDTRY;
	    break;
	case ZmGP_Dir:
	    error(UserErrWarning, catgets(catalog, CAT_MSGS, 963, "%s: is a directory"),
		VarTombfile);
	    break;
	case ZmGP_Error:
	    error(SysErrWarning, value_of(VarTombfile));
	    break;
    }
}

void
exorcise_ghosts()
{
    if (zpop_initialized) {
	int isdir = ZmGP_DontIgnoreNoEnt;
	char *tomb = getpath(value_of(VarTombfile), &isdir);

	switch (isdir) {
	    case ZmGP_File:
		ghost_OpenTomb(tomb);
		TRY {
		    ghost_Exorcise(afterlife);
		} FINALLY {
		    ghost_SealTomb();
		} ENDTRY;
		break;
	    case ZmGP_Dir:
	    error(UserErrWarning, catgets(catalog, CAT_MSGS, 963, "%s: is a directory"),
		    VarTombfile);
		break;
	    case ZmGP_Error:
		if (errno != ENOENT)
		    error(SysErrWarning, value_of(VarTombfile));
		break;
	}
    }
}

static void
afterlife_cb(data, cdata)
char *data;
ZmCallbackData cdata;
{
    long val;

    if (cdata->event == ZCB_VAR_UNSET) {
	afterlife = 0;
    } else {
	val = atol((char *)cdata->xdata);
	if (val > 0)
	    afterlife = (time_t)60 * (time_t)60 * (time_t)24 * (time_t)val;
	else
	    afterlife = 0;
    }
}

void
zpop_initialize()
{
    if (!zpop_initialized) {
        int  isdir = ZmGP_IgnoreNoEnt;
        char *tomb = getpath(value_of(VarTombfile), &isdir);
	char *after, afterval[64];

	spoor_Initialize();
	v7folder_InitializeClass();
	v7message_InitializeClass();
	zpopfolder_InitializeClass();
	zpopmessage_InitializeClass();

	ZmCallbackAdd(VarAfterlife, ZCBTYPE_VAR, afterlife_cb, NULL);
	after = value_of(VarAfterlife);
	if (after) {
	    afterval[0] = 0;
	    after = strncat(afterval, after, 64);
	}
	set_var(VarAfterlife, "=", after);

        ghost_OpenTomb(tomb);
        ghost_SealTomb();

	zpop_initialized = 1;
    }
}

#endif /* ZPOP_SYNC */
