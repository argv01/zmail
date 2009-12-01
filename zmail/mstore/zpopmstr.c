/* 
 * $RCSfile: zpopmstr.c,v $
 * $Revision: 1.5 $
 * $Date: 1995/02/14 01:25:51 $
 * $Author: bobg $
 */

#include <spoor.h>
#include <zpopmstr.h>
#include <zpopfldr.h>

/* This Mac cruft should go away when we do this for real */
#ifdef MAC_OS
#include "mac-stuff.h"
extern MacGlobalsPtr gMacGlobalsPtr;
#endif /* MAC_OS */
#ifdef _WINDOWS
extern char *gUserPass;
#endif /* _WINDOWS */

#ifndef lint
static const char zpopmstore_rcsid[] =
    "$Id: zpopmstr.c,v 1.5 1995/02/14 01:25:51 bobg Exp $";
#endif /* lint */

/* The class descriptor */
struct spClass *zpopmstore_class = (struct spClass *) 0;

static void
zpopmstore_initialize(self)
    struct zpopmstore *self;
{
    /* Code to initialize a struct zpopmstore */
    self->server = 0;
    self->user = 0;
    self->pass = 0;
    self->folder = 0;
}

static void
zpopmstore_finalize(self)
    struct zpopmstore *self;
{
    /* Code to finalize a struct zpopmstore */
}

struct zpopmstore *
zpopmstore_Open(name, data);
    char *name;
    GENERIC_POINTER_TYPE *data;
{
    /* We have two choices here -- collect user name and password
     * without verifying, and then call pop_open() in mstore_Setup;
     * or do everything right here, and make mstore_Setup a no-op.
     * I prefer the former, because it means we can create a zpop
     * store without connecting to it immediately.
     */
    struct zpopmstore *self;

    /* For now we presume pre-Mstore zmail POP initialization must
     * already have occurred, else we have a failure state.
     */
    if (!pop_initialized)
	return 0; /* RAISE("?", 0); */

    self = zpopmstore_NEW();

    /* Z-Mail global cruft */
    if (!(self->user = zlogin))
	self->user = value_of(VarUser);
#ifdef MAC_OS
    self->pass = gMacGlobalsPtr->pass;
#else /* !MAC_OS */
# ifdef _WINDOWS
    self->pass = gUserPass;
# else /* !_WINDOWS */
    self->pass = 0;
# endif /* !_WINDOWS */
#endif /* !MAC_OS */

    return self;
}

static void
zpopmstore_Setup(self)
    struct zpopmstore *self;
{
    self->server = pop_open(NULL, self->user, self->pass, POP_NO_GETPASS);
}

static void
zpopmstore_Update(self)
    struct zpopmstore *self;
{
    if (self->folder)
	mfldr_Update(self->folder);
}

static void
zpopmstore_Close(self, arg)
    struct zpopmstore *self;
    spArgList_t arg;
{
    if (spArg(arg, int)) {
	zpopmstore_Update(self);
	pop_quit(self->server);
    } else
	pop_close(self->server);
    self->server = 0;
}

static struct zpopfolder *
zpopmstore_OpenFolder(self, arg)
    struct zpopmstore *self;
    spArgList_t arg;
{
    char *name = spArg(arg, char *);
    GENERIC_POINTER_TYPE *data = spArg(arg, GENERIC_POINTER_TYPE *);

    if (!self->server)
	mstore_Setup(self, data);

    if (self->folder)
	return 0; /* RAISE("?", 0); */

    self->folder = zpopfolder_NEW();

    /* What now?  Since nearly every significant folder operation is
     * going to be delegated back to the mstore, do we even need to
     * do anything else here?
     */

    return self->folder;
}

static void
zpopmstore_mfldr_DeleteMsg(self, arg)
    struct zpopmstore *self;
    spArgList_t arg;
{
    struct mfldr *mf = spArg(arg, struct mfldr *);
    int n = spArg(arg, int);

    /*
    if (mf != self->folder)
	RAISE("?", 0);
    */

    pop_delete(self->server, n);
}

void
zpopmstore_InitializeClass()
{
    if (!mstore_class)
	mstore_InitializeClass();
    if (zpopmstore_class)
	return;
    zpopmstore_class = spoor_CreateClass("zpopmstore",
	     "ZPOP subclass of mstore",
	     mstore_class,
	     (sizeof (struct zpopmstore)),
	     zpopmstore_initialize,
	     zpopmstore_finalize);

    /* Override inherited methods */
    spoor_AddOverride(zpopmstore_class,
		      m_mstore_Setup, NULL,
		      zpopmstore_Setup);
    spoor_AddOverride(zpopmstore_class,
		      m_mstore_Update, NULL,
		      zpopmstore_Update);
    spoor_AddOverride(zpopmstore_class,
		      m_mstore_Close, NULL,
		      zpopmstore_Close);
    spoor_AddOverride(zpopmstore_class,
		      m_mstore_OpenFolder, NULL,
		      zpopmstore_OpenFolder);

    spoor_AddOverride(zpopmstore_class,
		      m_mstore_mfldr_Import, NULL,
		      zpopmstore_mfldr_Import);
    spoor_AddOverride(zpopmstore_class,
		      m_mstore_mfldr_DeleteMsg, NULL,
		      zpopmstore_mfldr_DeleteMsg);

    spoor_AddOverride(zpopfolder_class,
		      m_mstore_mfldr_mmsg_Size, NULL,
		      zpopmstore_mfldr_mmsg_Size);
    spoor_AddOverride(zpopfolder_class,
		      m_mstore_mfldr_mmsg_KeyHash, NULL,
		      zpopmstore_mfldr_mmsg_KeyHash);
    spoor_AddOverride(zpopfolder_class,
		      m_mstore_mfldr_mmsg_HeaderHash, NULL,
		      zpopmstore_mfldr_mmsg_HeaderHash);

    /* Add new methods */

    /* Initialize classes on which the zpopmstore class depends */

    /* Initialize class-specific data */
}
