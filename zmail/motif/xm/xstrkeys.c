#include "osconfig.h"
#include <Xm/Xm.h>

#if XmVersion < 1002

#ifdef M_UNIX
#define STREAMCONN
#endif /* M_UNIX */

#include "config.h"
#include "Xlibint.h"
#include <X11/Xresource.h>

#if XlibSpecificationRelease >= 5
#include <X11/keysymdef.h>
#endif /* X11R5 or later */

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

static struct ks_info {
    char	*ks_name;
    KeySym	ks_val;
} keySymInfo[] = {
#include	"ks_names.h"
};

char *KEYSYMSTR = "\
!\n\
!		OSF Keysyms\n\
!\n\
osfBackSpace			: 1004FF08\n\
osfInsert			: 1004FF63\n\
osfDelete			: 1004FFFF\n\
osfCopy				: 1004FF02\n\
osfCut				: 1004FF03\n\
osfPaste			: 1004FF04\n\
\n\
osfAddMode			: 1004FF31\n\
osfPrimaryPaste			: 1004FF32\n\
osfQuickPaste			: 1004FF33\n\
\n\
osfPageUp			: 1004FF41\n\
osfPageDown			: 1004FF42\n\
\n\
osfEndLine			: 1004FF57\n\
osfBeginLine			: 1004FF58\n\
\n\
osfActivate			: 1004FF44\n\
\n\
osfMenuBar			: 1004FF45\n\
\n\
osfClear			: 1004FF0B\n\
osfCancel			: 1004FF69\n\
osfHelp				: 1004FF6A\n\
osfMenu				: 1004FF67\n\
osfSelect			: 1004FF60\n\
osfUndo				: 1004FF65\n\
\n\
osfLeft				: 1004FF51\n\
osfUp				: 1004FF52\n\
osfRight			: 1004FF53\n\
osfDown				: 1004FF54\n\
\n\
!		DEC specific keysyms\n\
!\n\
DRemove				: 1000FF00\n\
\n\
!		HP specific keysyms\n\
!\n\
Reset				: 1000FF6C\n\
System				: 1000FF6D\n\
User				: 1000FF6E\n\
ClearLine			: 1000FF6F\n\
InsertLine			: 1000FF70\n\
DeleteLine			: 1000FF71\n\
InsertChar			: 1000FF72\n\
DeleteChar			: 1000FF73\n\
BackTab				: 1000FF74\n\
KP_BackTab			: 1000FF75\n\
hpModelock1			: 1000FF48\n\
hpModelock2			: 1000FF49\n\
\n\
XK_mute_acute			: 0x100000a8\n\
XK_mute_grave			: 0x100000a9\n\
XK_mute_asciicircum		: 0x100000aa\n\
XK_mute_diaeresis		: 0x100000ab\n\
XK_mute_asciitilde		: 0x100000ac\n\
XK_lira				: 0x100000af\n\
XK_guilder			: 0x100000be\n\
XK_Ydiaeresis			: 0x100000ee\n\
XK_IO				: 0x100000ee\n\
XK_longminus			: 0x100000f6\n\
XK_block			: 0x100000fc\n\
\n\
!		Apollo specific keysyms\n\
!\n\
apLineDel            : 1000FF00\n\
apCharDel            : 1000FF01\n\
apCopy               : 1000FF02\n\
apCut                : 1000FF03\n\
apPaste              : 1000FF04\n\
apMove               : 1000FF05\n\
apGrow               : 1000FF06\n\
apCmd                : 1000FF07\n\
apShell              : 1000FF08\n\
apLeftBar            : 1000FF09\n\
apRightBar           : 1000FF0A\n\
apLeftBox            : 1000FF0B\n\
apRightBox           : 1000FF0C\n\
apUpBox              : 1000FF0D\n\
apDownBox            : 1000FF0E\n\
apPop                : 1000FF0F\n\
apRead               : 1000FF10\n\
apEdit               : 1000FF11\n\
apSave               : 1000FF12\n\
apExit               : 1000FF13\n\
apRepeat             : 1000FF14\n\
apKP_parenleft       : 1000FFA8\n\
apKP_parenright      : 1000FFA9\n\
\n\
! Apollo specific keysyms as they were in the share-mode server.\n\
! These are needed for compatibility with old Apollo clients.\n\
!\n\
LineDel            : 1000FF00\n\
CharDel            : 1000FF01\n\
Copy               : 1000FF02\n\
Cut                : 1000FF03\n\
Paste              : 1000FF04\n\
Move               : 1000FF05\n\
Grow               : 1000FF06\n\
Cmd                : 1000FF07\n\
Shell              : 1000FF08\n\
LeftBar            : 1000FF09\n\
RightBar           : 1000FF0A\n\
LeftBox            : 1000FF0B\n\
RightBox           : 1000FF0C\n\
UpBox              : 1000FF0D\n\
DownBox            : 1000FF0E\n\
Pop                : 1000FF0F\n\
Read               : 1000FF10\n\
Edit               : 1000FF11\n\
Save               : 1000FF12\n\
Exit               : 1000FF13\n\
Repeat             : 1000FF14\n\
KP_parenleft       : 1000FFA8\n\
KP_parenright      : 1000FFA9\n";

static Bool initialized = False;
static XrmDatabase keysymdb;

#if XlibSpecificationRelease >= 5

/* Copyright 1985, 1987, 1990 Massachusetts Institute of Technology */

/*
Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation, and that the name of M.I.T. not be used in advertising or
publicity pertaining to distribution of the software without specific,
written prior permission.  M.I.T. makes no representations about the
suitability of this software for any purpose.  It is provided "as is"
without express or implied warranty.
*/

#ifdef X_NOT_STDC_ENV
extern char *getenv();
#endif

extern XrmQuark _XrmInternalStringToQuark();

#if __STDC__
#define Const const
#else
#define Const /**/
#endif

typedef unsigned long Signature;

#define NEEDKTABLE
#include "ks_tables.h"

#ifndef KEYSYMDB
#define KEYSYMDB "/usr/lib/X11/XKeysymDB"
#endif

static XrmQuark Qkeysym[2];

XrmDatabase
_XInitKeysymDB()
{
    if (!initialized)
    {
	char *dbname;

	XrmInitialize();
	/* use and name of this env var is not part of the standard */
	/* implementation-dependent feature */
	dbname = getenv("XKEYSYMDB");
	if (!dbname)
	    dbname = KEYSYMDB;
	if (!(keysymdb = XrmGetFileDatabase(dbname)))
	    keysymdb = XrmGetStringDatabase(KEYSYMSTR);
	if (keysymdb)
	    Qkeysym[0] = XrmStringToQuark("Keysym");
	initialized = True;
    }
    return keysymdb;
}

#if NeedFunctionPrototypes
KeySym XStringToKeysym(s)
    _Xconst char *s;
#else
KeySym XStringToKeysym(s)
    char *s;
#endif
{
    register int i, n;
    int h;
    register Signature sig = 0;
    register Const char *p = s;
    register int c;
    register int idx;
    Const unsigned char *entry;
    unsigned char sig1, sig2;
    KeySym val;

    while (c = *p++)
	sig = (sig << 1) + c;
    i = sig % KTABLESIZE;
    h = i + 1;
    sig1 = (sig >> 8) & 0xff;
    sig2 = sig & 0xff;
    n = KMAXHASH;
    while (idx = hashString[i])
    {
	entry = &_XkeyTable[idx];
	if ((entry[0] == sig1) && (entry[1] == sig2) &&
	    !strcmp(s, (char *)entry + 4))
	{
	    val = (entry[2] << 8) | entry[3];
	    if (!val)
		val = XK_VoidSymbol;
	    return val;
	}
	if (!--n)
	    break;
	i += h;
	if (i >= KTABLESIZE)
	    i -= KTABLESIZE;
    }

    if (!initialized)
	(void)_XInitKeysymDB();
    if (keysymdb)
    {
	XrmValue result;
	XrmRepresentation from_type;
	char c;
	KeySym val;
	XrmQuark names[2];

	names[0] = _XrmInternalStringToQuark(s, p - s - 1, sig, False);
	names[1] = NULLQUARK;
	(void)XrmQGetResource(keysymdb, names, Qkeysym, &from_type, &result);
	if (result.addr && (result.size > 1))
	{
	    val = 0;
	    for (i = 0; i < result.size - 1; i++)
	    {
		c = ((char *)result.addr)[i];
		if ('0' <= c && c <= '9') val = (val<<4)+c-'0';
		else if ('a' <= c && c <= 'z') val = (val<<4)+c-'a'+10;
		else if ('A' <= c && c <= 'Z') val = (val<<4)+c-'A'+10;
		else return NoSymbol;
	    }
	    return val;
	}
    }
    return (NoSymbol);
}

#else /* X11R4 or earlier */

/* $XConsortium: XStrKeysym.c,v 11.4 89/12/11 19:10:34 rws Exp $ */
/* Copyright 1985, 1987, Massachusetts Institute of Technology */

#ifndef KEYSYMDB
#define KEYSYMDB "/usr/lib/X11/XKeysymDB"
#endif

_XInitKeysymDB()
{
    if (!initialized)
    {
        char *dbname, *getenv();

        XrmInitialize();
        /* use and name of this env var is not part of the standard */
        /* implementation-dependent feature */
        dbname = getenv("XKEYSYMDB");
        if (!dbname)
            dbname = KEYSYMDB;
	if (!(keysymdb = XrmGetFileDatabase(dbname)))
	    keysymdb = XrmGetStringDatabase(KEYSYMSTR);
	if (keysymdb)
	    initialized = True;
    }
}

KeySym XStringToKeysym(s)
     const char *s;
{
    int i;

    /*
     *	Yes,  yes,  yes.  I know this is a linear search,  and we should
     *	do better,  but I'm in a hurry right now.
     */

    for (i = 0; i < ((sizeof keySymInfo)/(sizeof keySymInfo[0])); i++) {
	if (strcmp(s, keySymInfo[i].ks_name) == 0)
	    return (keySymInfo[i].ks_val);
    }
    if (!initialized)
	_XInitKeysymDB();
    if (keysymdb)
    {
	XrmString type_str;
	XrmValue result;
	char c;
	KeySym val;

	XrmGetResource(keysymdb, s, "Keysym", &type_str, &result);
	if (result.addr && (result.size > 1))
	{
	    val = 0;
	    for (i = 0; i < result.size - 1; i++)
	    {
		c = ((char *)result.addr)[i];
		if ('0' <= c && c <= '9') val = val*16+c-'0';
		else if ('a' <= c && c <= 'z') val = val*16+c-'a'+10;
		else if ('A' <= c && c <= 'Z') val = val*16+c-'A'+10;
		else return NoSymbol;
	    }
	    return val;
	}
    }
    return (NoSymbol);
}

extern char *_XrmGetResourceName();

char *XKeysymToString(ks)
    KeySym ks;
{
    int i;

    /*
     *	Yes,  yes,  yes.  I know this is a linear search,  and we should
     *	do better,  but I'm in a hurry right now.
     */

    for (i = 0; i < ((sizeof keySymInfo)/(sizeof keySymInfo[0])); i++) {
	if (ks == keySymInfo[i].ks_val)
	    return (keySymInfo[i].ks_name);
    }
    if (!initialized)
	_XInitKeysymDB();
    if (keysymdb)
    {
	char buf[8];
	XrmValue resval;

	sprintf(buf, "%lX", ks);
	resval.addr = (caddr_t)buf;
	resval.size = strlen(buf) + 1;
	return _XrmGetResourceName(keysymdb, "String", &resval);
    }
    return ((char *) NULL);
}

#endif /* X11R5 or later */

#endif /* XmVersion < 1002 */
