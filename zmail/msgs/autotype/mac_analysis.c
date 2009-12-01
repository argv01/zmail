#include "analysis.h"
#include "attach.h"
#include "catalog.h"
	/* "when" will the horror end? */
#ifdef when
# undef when
# undef otherwise
#endif /* when */
#include "mac-stuff.h"

extern AttachTypeAlias	*type_mactypes;

const char *autotype_via_mactype(const char *file) 
{
    OSErr err;
    CInfoPBRec CPB;
    HFileParam hfp;
    char *d, kind[10], buf[MAXPATHLEN];
    short vRef = 0;
    long dirID = 0;
    

    if (!file) return nil;
    strcpy(buf, file);	/* thou shalt not poke thy caller's buffer */
    err = GetPathRef(buf, &vRef, &dirID); /* GetPathRef does c2p */
    c2pstr(buf);
    CPB.hFileInfo.ioNamePtr = buf;
    CPB.hFileInfo.ioDirID = 0;
    CPB.hFileInfo.ioVRefNum = 0;
    CPB.hFileInfo.ioFDirIndex = 0;
    switch (PBGetCatInfo (&CPB, 0)) {
	case noErr: break;
	case fnfErr:
	    CPB.hFileInfo.ioDirID = 0;
	    CPB.hFileInfo.ioVRefNum = vRef; 
	    err = PBGetCatInfo (&CPB, 0);
	    break;
	default:
	    return nil;
    }
    hfp.ioCompletion = nil;
    hfp.ioNamePtr = buf;
    hfp.ioVRefNum = CPB.hFileInfo.ioVRefNum;
    hfp.ioFDirIndex = 0;
    hfp.ioDirID = CPB.hFileInfo.ioDirID;
    if (err = PBHGetFInfo((HParmBlkPtr) &hfp, FALSE)) {
	print(catgets(catalog, CAT_MSGS, 1013, "autotyper:  couldn't read attachment file information"));
	return nil;
    }

    /* creator default is Z-Mail sig */
    d = (char *) kind;
    *((long *)(d)) =  CPB.hFileInfo.ioFlFndrInfo.fdType;
    kind[4] = '/';
    d += 5;
    *((long *)(d)) = CPB.hFileInfo.ioFlFndrInfo.fdCreator;
    kind[9] = 0;
    if (!type_mactypes)
        (void) get_attach_keys(0, (Attach *)0, NULL);
    d = get_type_from_mactype((char *)kind);    
    if (!strcmp(d, kind))
    	d = nil;
    if (!d && hfp.ioFlRLgLen)
	d = "application/mac-binhex40";

    return d;

} /* autotype_via_mactype */
