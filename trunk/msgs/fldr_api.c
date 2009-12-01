/* fldr_api.c     Copyright 1990, 1991 Z-Code Software Corp. */

/*
 * This file is responsible for the internal implementation of the
 * msg_folder object.
 */

#ifndef lint
static char	fldr_api_rcsid[] = "$Id: fldr_api.c,v 2.9 1995/07/08 01:21:51 spencer Exp $";
#endif

#include "zmail.h" /* include "zfolder.h" when moved over to new code */
#include "fldr_api.h"
#include "zmalloc.h"
#include "catalog.h"

static msg_folder init_data;

#ifdef HAVE_STDARG_H
#include <stdarg.h>	/* Hopefully self-protecting */
#else
#ifndef va_dcl
#include <varargs.h>
#endif /* va_dcl */
#endif /* HAVE_STDARG_H */

void
FolderClose(fldr)
ZmFolder fldr;
{
}

void
FolderCloseCallback(fldr)
ZmFolder fldr;
{
}

/* FolderCreate(char *name, ...); */
ZmFolder
#ifdef HAVE_STDARG_H
FolderCreate(char *name, ...)
#else /* !HAVE_STDARG_H */
FolderCreate(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    FolderArg arg;
    va_list args;
    msg_folder *new;
    int status = 0, cnt = 0;
#ifndef HAVE_STDARG_H
    char *name;

    va_start(args);

    name = va_arg(args, char *);
#else /* HAVE_STDARG_H */
    va_start(args, name);
#endif /* !HAVE_STDARG_H */

    if (!(new = zmNew(msg_folder)))
	status = -1;

    while (status == 0 && (arg = va_arg(args, u_long /* !GF FolderArg */)) != FolderEndArgs) {
	cnt++;
	switch ((int) arg) {
	    otherwise :
		error(ZmErrWarning,
		    catgets( catalog, CAT_MSGS, 371, "You cannot set this attribute: %d (item #%d)" ), arg, cnt);
		status = cnt;
	}
    }

    va_end(args);
    if (status != 0)
	return (msg_folder *)0;

    return new;
}

int
#ifdef HAVE_STDARG_H
FolderSet(msg_folder *data, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
FolderSet(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    FolderArg arg;
    va_list args;
    char *str, buf[MAXPATHLEN];
    msg_group *list;
    int status = 0, cnt = 0;
#ifndef HAVE_STDARG_H
    msg_folder *data;

    va_start(args);
    data = va_arg(args, msg_folder *);
#else /* HAVE_STDARG_H */
    va_start(args, data);
#endif /* !HAVE_STDARG_H */
    if (!data) {
	error(ZmErrFatal, catgets( catalog, CAT_MSGS, 372, "passed null msg_folder to FolderSet" ));
	status = -1;
    }

    while (status == 0 && (arg = va_arg(args, u_long /* !GF FolderArg */)) != FolderEndArgs) {
	cnt++;
	switch ((int) arg) {
	    case FolderTrack :
		data->mf_track = *va_arg(args, Ftrack *);
		break;

	    case FolderName :
		data->mf_name = va_arg(args, char *);
		break;

	    case FolderCount :
		data->mf_count = va_arg(args, int);
		break;

	    case FolderCurMsg :
		data->mf_current = va_arg(args, int);
		break;

	    case FolderTypeBits:	/* FolderType mf_type */
		data->mf_type = va_arg(args, u_long /* !GF FolderType */); /* !GF */
		break;

	    case FolderBackup:	/* struct mfolder *mf_backup */
		data->mf_backup = va_arg(args, struct mfolder *);
		break;

	    case FolderParent:	/* struct mfolder *mf_parent */
		data->mf_parent = va_arg(args, struct mfolder *);
		break;

	    case FolderNumber:	/* int mf_number */
		data->mf_number = va_arg(args, int);
		break;

	    case FolderTempName:	/* char *mf_tempname */
		data->mf_tempname = va_arg(args, char *);
		break;

	    case FolderFILEstruct:	/* FILE *mf_file */
		data->mf_file = va_arg(args, FILE *);
		break;

	    case FolderFlags:	/* long mf_flags */
		data->mf_flags = va_arg(args, long);
		break;

	    case FolderMsgs:		/* struct Msg *mf_msgs */
		data->mf_msgs = va_arg(args, struct Msg *);
		break;

	    /* XXX -- could this be a problem for varargs? */
	    case FolderGroup:	/* struct mgroup mf_group */
		data->mf_group = *va_arg(args, struct mgroup *);
		break;

	    case FolderLastCount:	/* int mf_last_count */
		data->mf_last_count = va_arg(args, int);
		break;

#ifdef MOTIF
	    case FolderGuiItem:	/* GuiItem mf_hdr_list */
		data->mf_hdr_list = va_arg(args, GuiItem);
		break;

#endif /* MOTIF */

	    /* XXX -- could this be a problem for varargs? */
	    case FolderInformation:		/* struct mailinfo mf_info; */
		data->mf_info = *va_arg(args, struct mailinfo *);
		break;

	    /* XXX -- could this be a problem for varargs? */
	    case FolderCallbacks:	/* DeferredAction *mf_callbacks */
		data->mf_callbacks = va_arg(args, DeferredAction *);
		break;

	    default :
		error(ZmErrWarning,
		    catgets( catalog, CAT_MSGS, 373, "You cannot set this attribute: %d (item #%d)" ), arg, cnt);
		status = cnt;
	}
    }

    va_end(args);
    return status;
}

int
#ifdef HAVE_STDARG_H
FolderGet(msg_folder *data, ...)
#else /* !HAVE_STDARG_H */
/*VARARGS1*/
FolderGet(va_alist)
va_dcl
#endif /* HAVE_STDARG_H */
{
    FolderArg arg;
    va_list args;
    int status = 0, cnt = 0;
#ifndef HAVE_STDARG_H
    msg_folder *data;

    va_start(args);
    data = va_arg(args, msg_folder *);
#else /* HAVE_STDARG_H */
    va_start(args, data);
#endif /* !HAVE_STDARG_H */
    if (!data) {
	error(ZmErrFatal, catgets( catalog, CAT_MSGS, 374, "passed null msg_folder to FolderSet" ));
	status = -1;
    }

    while (status == 0 && (arg = va_arg(args, u_long /* FolderArg !GF */)) != FolderEndArgs) {
	cnt++;
	switch ((int) arg) {
	    case FolderTrack : {
		Ftrack **track = va_arg(args, Ftrack **);
		*track = &data->mf_track;
		break;
	    }

	    case FolderName : {
		char **name = va_arg(args, char **);
		*name = data->mf_name; /* macro fromk folder.h */
		break;
	    }

	    case FolderTypeBits : {	/* FolderType mf_type */
		FolderType *bits = va_arg(args, FolderType *);
		*bits = data->mf_type;
		break;
	    }

	    case FolderBackup : {	/* struct mfolder *mf_backup */
		struct mfolder **fldr = va_arg(args, struct mfolder **);
		*fldr = data->mf_backup;
		break;
	    }

	    case FolderParent : {	/* struct mfolder *mf_parent */
		struct mfolder **fldr = va_arg(args, struct mfolder **);
		*fldr = data->mf_parent;
		break;
	    }

	    case FolderNumber : {	/* int mf_number */
		int *number = va_arg(args, int *);
		*number = data->mf_number;
		break;
	    }

	    case FolderTempName : {	/* char *mf_tempname */
		char **tmpname = va_arg(args, char **);
		*tmpname = data->mf_tempname;
		break;
	    }

	    case FolderFILEstruct : {	/* FILE *mf_file */
		FILE **file = va_arg(args, FILE **);
		*file = data->mf_file;
		break;
	    }

	    case FolderFlags : {	/* long mf_flags */
		long *flags = va_arg(args, long *);
		*flags = data->mf_flags;
		break;
	    }

	    case FolderMsgs : {		/* struct Msg *mf_msgs */
		struct Msg **msgs = va_arg(args, struct Msg **);
		*msgs = data->mf_msgs;
		break;
	    }

	    /*
	    mgroup *group;
	    FolderGet(fldr, FolderGroup, &group, FolderEndArgs);
	    */
	    case FolderGroup : {	/* struct mgroup mf_group */
		struct mgroup **mgroup = va_arg(args, struct mgroup **);
		*mgroup = &data->mf_group;
		break;
	    }

	    case FolderLastCount : {	/* int mf_last_count */
		int *count = va_arg(args, int *);
		*count = data->mf_last_count;
		break;
	    }

#ifdef MOTIF
	    case FolderGuiItem : {	/* GuiItem mf_hdr_list */
		GuiItem *item = va_arg(args, GuiItem *);
		*item = data->mf_hdr_list;
		break;
	    }

#endif /* MOTIF */

	    case FolderInformation : {		/* struct mailinfo mf_info; */
		struct mailinfo **info = va_arg(args, struct mailinfo **);
		*info = &data->mf_info;
		break;
	    }

	    case FolderCallbacks : {	/* DeferredAction *mf_callbacks */
		DeferredAction **callbacks = *va_arg(args, DeferredAction **);
		*callbacks = data->mf_callbacks;
		break;
	    }

	    case FolderCount : {	/* mf_group.mg_count */
		int *count = va_arg(args, int *);
		*count = data->mf_group.mg_count;
		break;
	    }

	    case FolderCurMsg : {
		int *cur = va_arg(args, int *);
		*cur = data->mf_current; /* macro */
		break;
	    }

	    default :
		error(ZmErrWarning,
		    catgets( catalog, CAT_MSGS, 375, "You cannot set this attribute: %d (item #%d)" ), arg, cnt);
		status = cnt;
		break;
	}
    }
    va_end(args);

    return status;
}

/* Convenience Routines: technically, in a true OOP environment, there
 * should be a routine that "gets" each of the items in a msg_folder object.
 */

FolderType
FolderGetType(fldr)
msg_folder *fldr;
{
    return fldr->mf_type;
}

int
FolderGetCurMsg(fldr)
msg_folder *fldr;
{
    return fldr->mf_current; /* macro */
}

int
FolderGetCount(fldr)
msg_folder *fldr;
{
    return fldr->mf_count; /* macro */
}

int
FolderGetNumber(fldr)
msg_folder *fldr;
{
    return fldr->mf_number;
}

long
FolderGetFlags(fldr)
msg_folder *fldr;
{
    return fldr->mf_flags;
}

struct Msg *
FolderGetMsgs(fldr)
msg_folder *fldr;
{
    return fldr->mf_msgs;
}

struct mgroup *
FolderGetGroup(fldr)
msg_folder *fldr;
{
    return &fldr->mf_group;
}
