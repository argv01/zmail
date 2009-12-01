/* au.c		Copyright 1992 Z-Code Software Corp.  All Rights Reserved. */
/*
 * To compile this standalone as a soundfile player, say:
 * 
cc -DI_WISH_THOSE_GUYS_MADE_AU_MODULAR -DAUDIO -DMAIN -I.. -I../include -I../config au.c -o au
 * on the sgi, add -laudio
 * on the dec, add -laudio -li
 */


#ifndef I_WISH_THOSE_GUYS_MADE_AU_MODULAR
#include "zmail.h"
#undef msg	/* zmail.h #defines it, Alib.h uses it as a variable name */
#ifdef HAVE_MEMCPY
# ifdef bcmp
#  undef bcmp
# endif /* bcmp */
# define bcmp(s, d, n) memcmp(d, s, n)
#endif /* HAVE_MEMCPY */
#endif /* I_WISH_THOSE_GUYS_MADE_AU_MODULAR */

#ifdef AUDIO

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "au.h"
#include "catalog.h"
#include "strcase.h"

#ifdef sgi
/* link with -laudio */
#include <audio.h>
#endif /* sgi */


#ifdef ultrix
/* link with -laudio -li */

#undef NULL	/* so compiler will shut up when it gets redefined */
#undef FALSE	/* so compiler will shut up when it gets redefined */
/*
 * XXX
 * Hack to make it work-- zmail.h defines malloc,calloc,realloc as char *,
 * Alib.h includes stdlib.h which defines them as void *.  There must be
 * a "right" way to do this... ask bart when everything calms down around here.
 */
#ifdef malloc
this hack is unacceptable if we are #defining malloc to be something else...
#endif
#define malloc malloc_foobar
#define calloc calloc_foobar
#define realloc realloc_foobar

#include <sys/file.h>
#include "XMedia/audio/Alib.h"

#undef malloc
#undef calloc
#undef realloc
#endif /* ultrix */



/*
 * Returns NULL on error, in which case errno will be set appropriately.
 */
extern AuData
AuReadFp(fp)
FILE *fp;
{
    struct stat statbuf;
    long len;
    AuData data;

    /*
     * Magic stuff to discard the .au header
     */
    {
	long magic, hdr_length;
	unsigned char hdr_length_charbuf[sizeof(long)];
	int i;

	if (fread(&magic, sizeof(magic), 1, fp) == 1 &&
	    bcmp((char *)&magic, ".snd", 4) == 0) {
	    if (fread(hdr_length_charbuf, sizeof(hdr_length_charbuf), 1, fp)
									!= 1){
		return 0;
	    }

	    /*
	     * The file assumes big-endian, but we don't...
	     */
	    hdr_length = 0;
	    for (i = 0; i < sizeof(long); ++i)
		hdr_length |= hdr_length_charbuf[sizeof(long)-1-i] << (8*i);
		
	    if (fseek(fp, hdr_length, 0) < 0) {
		return 0;
	    }
	} else {
	    /*
	     * Assume raw mulaw (no header)
	     */
	    if (fseek(fp, 0L, 0) < 0) {
		return 0;
	    }
	}
    }

    if (fstat(fileno(fp), &statbuf) < 0) {
	return 0;
    }

    len = statbuf.st_size - ftell(fp);

    data = (AuData) malloc(sizeof(data->filename) + sizeof(data->len) + len);
    if (!data) {
	return 0;
    }

    data->filename = NULL;
    data->len = len;
    if (fread(data->data, 1, len, fp) != len) {
	return 0;
    }

    return data;
}

/*
 * Returns NULL on error, in which case errno will be set appropriately.
 */
extern AuData
AuReadFile(filename)
char *filename;
{
    FILE *fp;
    AuData data;

    if (!(fp = fopen(filename, "r")))
	return 0;
    
    data = AuReadFp(fp);
    if (data) {
	data->filename = (char *)malloc(strlen(filename) + 1);
	if (data->filename)
	    (void) strcpy(data->filename, filename);
    }

    fclose(fp);

    return data;
}

extern void
AuDestroy(data)
AuData data;
{
    if (data->filename)
	(void) free(data->filename);
    (void) free((char *)data);
}

#ifdef sgi

#include <audio.h>

/*
 * Ain't got a clue how sfconvert does it, I just ran
 *	sfconvert ramp signed_to_mulaw_lookup -inputraw end -outputraw mulaw
 * and  sfconvert ramp mulaw_to_signed_lookup -inputraw mulaw end -outputraw integer 8 2scomp
 * where "ramp" is a file containing the unsigned bytes 0..255.
 */
/*
static unsigned char signed_to_mulaw_lookup[256] = {
0377,0347,0333,0323,0315,0311,0305,0302,0277,0275,0273,0271,0267,0265,0263,0261,
0257,0256,0255,0254,0253,0252,0251,0250,0247,0246,0245,0244,0243,0242,0241,0240,
0237,0237,0236,0236,0235,0235,0234,0234,0233,0233,0232,0232,0231,0231,0230,0230,
0227,0227,0226,0226,0225,0225,0224,0224,0223,0223,0222,0222,0221,0221,0220,0220,
0217,0217,0217,0217,0216,0216,0216,0216,0215,0215,0215,0215,0214,0214,0214,0214,
0213,0213,0213,0213,0212,0212,0212,0212,0211,0211,0211,0211,0210,0210,0210,0210,
0207,0207,0207,0207,0206,0206,0206,0206,0205,0205,0205,0205,0204,0204,0204,0204,
0203,0203,0203,0203,0202,0202,0202,0202,0201,0201,0201,0201,0200,0200,0200,0200,
0000,0000,0000,0000,0001,0001,0001,0001,0001,0002,0002,0002,0002,0003,0003,0003,
0003,0004,0004,0004,0004,0005,0005,0005,0005,0006,0006,0006,0006,0007,0007,0007,
0007,0010,0010,0010,0010,0011,0011,0011,0011,0012,0012,0012,0012,0013,0013,0013,
0013,0014,0014,0014,0014,0015,0015,0015,0015,0016,0016,0016,0016,0017,0017,0017,
0017,0020,0020,0021,0021,0022,0022,0023,0023,0024,0024,0025,0025,0026,0026,0027,
0027,0030,0030,0031,0031,0032,0032,0033,0033,0034,0034,0035,0035,0036,0036,0037,
0037,0040,0041,0042,0043,0044,0045,0046,0047,0050,0051,0052,0053,0054,0055,0056,
0057,0061,0063,0065,0067,0071,0073,0075,0077,0102,0106,0112,0116,0124,0134,0150,
};
static unsigned char mulaw_to_signed_lookup[256] = {
0202,0206,0212,0216,0222,0226,0232,0236,0242,0246,0252,0256,0262,0266,0272,0276,
0301,0303,0305,0307,0311,0313,0315,0317,0321,0323,0325,0327,0331,0333,0335,0337,
0340,0341,0342,0343,0344,0345,0346,0347,0350,0351,0352,0353,0354,0355,0356,0357,
0360,0361,0361,0362,0362,0363,0363,0364,0364,0365,0365,0366,0366,0367,0367,0370,
0370,0370,0371,0371,0371,0371,0372,0372,0372,0372,0373,0373,0373,0373,0374,0374,
0374,0374,0374,0374,0375,0375,0375,0375,0375,0375,0375,0375,0376,0376,0376,0376,
0376,0376,0376,0376,0376,0376,0376,0376,0377,0377,0377,0377,0377,0377,0377,0377,
0377,0377,0377,0377,0377,0377,0377,0377,0377,0377,0377,0377,0377,0377,0377,0000,
0176,0171,0165,0161,0155,0151,0145,0141,0135,0131,0125,0121,0115,0111,0105,0101,
0076,0074,0072,0070,0066,0064,0062,0060,0056,0054,0052,0050,0046,0044,0042,0040,
0037,0036,0035,0034,0033,0032,0031,0030,0027,0026,0025,0024,0023,0022,0021,0020,
0017,0016,0016,0015,0015,0014,0014,0013,0013,0012,0012,0011,0011,0010,0010,0007,
0007,0007,0006,0006,0006,0006,0005,0005,0005,0005,0004,0004,0004,0004,0003,0003,
0003,0003,0003,0003,0002,0002,0002,0002,0002,0002,0002,0002,0001,0001,0001,0001,
0001,0001,0001,0001,0001,0001,0001,0001,0000,0000,0000,0000,0000,0000,0000,0000,
0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,0000,
};
*/
static unsigned short mulaw_to_signed_short_lookup[256] = {
0101000, 0103004, 0105010, 0107014, 0111021, 0113025, 0115031, 0117035,
0121041, 0123046, 0125052, 0127056, 0131062, 0133067, 0135073, 0137077,
0140502, 0141504, 0142507, 0143511, 0144513, 0145515, 0146517, 0147521,
0150523, 0151525, 0152530, 0153532, 0154534, 0155536, 0156540, 0157542,
0160344, 0160745, 0161346, 0161747, 0162350, 0162751, 0163352, 0163753,
0164354, 0164755, 0165356, 0165757, 0166360, 0166761, 0167363, 0167764,
0170264, 0170465, 0170665, 0171066, 0171266, 0171467, 0171670, 0172070,
0172271, 0172471, 0172672, 0173072, 0173273, 0173473, 0173674, 0174074,
0174235, 0174335, 0174435, 0174536, 0174636, 0174736, 0175036, 0175137,
0175237, 0175337, 0175437, 0175540, 0175640, 0175740, 0176040, 0176141,
0176221, 0176261, 0176321, 0176361, 0176421, 0176462, 0176522, 0176562,
0176622, 0176662, 0176722, 0176762, 0177022, 0177063, 0177123, 0177163,
0177213, 0177233, 0177253, 0177273, 0177313, 0177333, 0177353, 0177373,
0177413, 0177434, 0177454, 0177474, 0177514, 0177534, 0177554, 0177574,
0177610, 0177620, 0177630, 0177640, 0177650, 0177660, 0177670, 0177700,
0177710, 0177720, 0177730, 0177740, 0177750, 0177760, 0177770, 0000000,
0077000, 0074774, 0072770, 0070764, 0066757, 0064753, 0062747, 0060743,
0056737, 0054732, 0052726, 0050722, 0046716, 0044711, 0042705, 0040701,
0037276, 0036274, 0035271, 0034267, 0033265, 0032263, 0031261, 0030257,
0027255, 0026253, 0025250, 0024246, 0023244, 0022242, 0021240, 0020236,
0017434, 0017033, 0016432, 0016031, 0015430, 0015027, 0014426, 0014025,
0013424, 0013023, 0012422, 0012021, 0011420, 0011017, 0010415, 0010014,
0007514, 0007313, 0007113, 0006712, 0006512, 0006311, 0006110, 0005710,
0005507, 0005307, 0005106, 0004706, 0004505, 0004305, 0004104, 0003704,
0003543, 0003443, 0003343, 0003242, 0003142, 0003042, 0002742, 0002641,
0002541, 0002441, 0002341, 0002240, 0002140, 0002040, 0001740, 0001637,
0001557, 0001517, 0001457, 0001417, 0001357, 0001316, 0001256, 0001216,
0001156, 0001116, 0001056, 0001016, 0000756, 0000715, 0000655, 0000615,
0000565, 0000545, 0000525, 0000505, 0000465, 0000445, 0000425, 0000405,
0000365, 0000344, 0000324, 0000304, 0000264, 0000244, 0000224, 0000204,
0000170, 0000160, 0000150, 0000140, 0000130, 0000120, 0000110, 0000100,
0000070, 0000060, 0000050, 0000040, 0000030, 0000020, 0000010, 0000000,
};


/*
 * NOTE: as of the time of this writing (Mon Jul 20 22:58:34 PDT 1992)
 * there is a bug in ALopenport such that if ALopenport is called when
 * there are already 4 ports open,
 * a new file descriptor gets opened but not closed.
 * Therefore we must never try; i.e. keep AU_MAXPORTS <= 4.
 */
#ifndef AU_MAXPORTS
#define AU_MAXPORTS 4		/* max number of simultaneous sounds */
#endif /* AU_MAXPORTS */

static int nports = 0;
static ALport ports[AU_MAXPORTS];
static int times_opened = 0;

static int
open_another_port()
{
    ALconfig config;

    if (nports == AU_MAXPORTS)
	return 0;	/* stuff will just have to get written to the end
			   of the last port */

    /*
     * Open the audio stuff for writing
     */
    config = ALnewconfig();
    ALsetwidth(config, AL_SAMPLE_16);
    ALsetchannels(config, AL_MONO);

    ports[nports] = ALopenport("Z-Mail sounds", "w", config);

    ALfreeconfig(config);

    if (ports[nports])
	nports++;

    /*
     * Only return failure if there is really no port to write to.
     * If the ALopenport failed, but there is a port opened,
     * we simply play the sound to any opened port (so our new sounds
     * might have to wait for any queued sounds to finish).
     * Probably should make it so that it gets played to the
     * one with the least amount of sound queued; maybe later... XXX
     */
    return nports > 0 ? 0 : -1;
}

/*
 * return 0 on success, -1 on error
 */
extern int
AuOpenTheDevice()
{
    ALconfig config;


    if (times_opened == 0) {
#ifndef MAIN
	if (!getenv("AU_DONT_SYSTEM"))
	    return 0;
	else
#endif /* !MAIN */
	if (open_another_port() < 0)
	    return -1;
    }
    times_opened++;
    return 0;
}

extern void
AuCloseTheDevice()
{
    if (times_opened <= 0)
	return;
    if (--times_opened == 0) {
#ifndef MAIN
	if (getenv("AU_DONT_SYSTEM"))
#endif /* MAIN */
	while (nports > 0) {
	    --nports;
	    ALcloseport(ports[nports]);
	}
    }
}


/*
 * Return 0 on success, -1 on error.
 * If AuOpenTheDevice was called beforehand, this function
 * sends the sound to the device and returns (effectively
 * playing the sound in the background).
 * Otherwise this function opens the device, plays the sound, waits
 * for it to finish, and then closes the device.
 */
extern int
AuPlay(data)
AuData data;
{
    long pvbuf[2], pvbuflen;
    int device_was_open, i;
    short *shortbuf = (short *)malloc(data->len * sizeof(short));

    if (!shortbuf)
	return -1;
    for (i = 0; i < data->len; ++i)
	shortbuf[i] = mulaw_to_signed_short_lookup[data->data[i]];


    /*
     * Close all ports whose sound has finished, except for one
     */
    for (i = 0; i < nports;) {
	if (nports == 1 || ALgetfilled(ports[i]) > 0)
	    i++;
	else {
	    ALcloseport(ports[i]);
	    ports[i] = ports[nports-1];
	    --nports;
	}
    }

    device_was_open = (nports > 0);
    if (!device_was_open)
	if (AuOpenTheDevice() < 0)
	    return -1;

    if (ALgetfilled(ports[nports-1]) > 0)
	open_another_port();


    /*
     * Make sure the output rate is right.
     */
    pvbuflen = 0;
    pvbuf[pvbuflen++] = AL_OUTPUT_RATE;
    pvbuf[pvbuflen++] = 8000;
    ALsetparams(AL_DEFAULT_DEVICE, pvbuf, pvbuflen);

    ALwritesamps(ports[nports-1], shortbuf, (long)data->len);

    if (!device_was_open) {
	AuWaitForSoundToFinish();
	AuCloseTheDevice();
    }

    free(shortbuf);
    return 0;
}

extern void
AuWaitForSoundToFinish()
{
    /*
     * Wait for sounds to finish on all the ports.
     * Close all but one.
     */
    while (1) {
	if (nports == 0)
	    break;
	if (nports == 1 && ALgetfilled(ports[0]) <= 0)
	    break;
	if (ALgetfilled(ports[nports-1])) {
	    sginap(6);		/* sleep for 1/10 of a second */
	    continue;
	}
	--nports;
	ALcloseport(ports[nports]);
    }
}

#endif /* sgi */




#ifdef ultrix

/***************************************************************************
 *
 * Copyright (c) Digital Equipment Corporation, 1991, 1992 All Rights Reserved.
 * Unpublished rights reserved under the copyright laws of the United States.
 * The software contained on this media is proprietary to and embodies the
 * confidential technology of Digital Equipment Corporation.  Possession, use,
 * duplication or dissemination of the software and media is authorized only
 * pursuant to a valid written license from Digital Equipment Corporation.
 * RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by the U.S.
 * Government is subject to restrictions as set forth in 
 * Subparagraph (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as 
 * applicable.
 *
 ***************************************************************************/
/*
 *   Digital Equipment Corporation,  Western Software Lab
 *   
 *   Written by Ki Wong
 *   Date: 11/12/91
 *   play a client sound file  program.
 *
 */


#define ULAW_SOUND_SAMPLE_RATE 8000
#define MAX_SOUND_SAMPLES 4000
#define SOUNDOP   1

#define MOD(a, b) ((a > 0) ? (a)%(b) : (b)+(a))
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif /* !MIN */

static long glSoundSampleRate = ULAW_SOUND_SAMPLE_RATE;
static long glClientPos;
static long glCurrServPos;
static long glPrevServPos;

static long glDataPlayed;
static long glDataSent;
static long glLoopBufSize;
static long glLoopGuard = 8000;	/* stay this far away from the beginning of good data
				 * in the server.  This is required so that 
				 * you are sure the server does not break up
				 * good sound that it is about to play
				 */

static int index_into_data;	/* XXX make a better way to do this */
static AudioServer 	  *server;
static int times_opened = 0;

static Bool gbVerbose = True;

/*
 * Creates a player, a speaker and connects the player to the speaker
 * by a wire. Returns the player id.
 */
static VDeviceId
CreatePlayer(pAserver, ldCloud)
     AudioServer *pAserver;
     LoudId       ldCloud;
{
  VDeviceId          vdPlayer;
  VDeviceId          vdSpeaker;
  WireId             wire;
  aVDeviceAttributes vdPlayerAttrs;
  aVDeviceAttributes vdSpeakerAttrs;
  aWireAttributes    wireAttrs;
  PortInfoStruct     sink1, source1;
  VDeviceValueMask   vdMask;
  WireValueMask      wireMask;
  /* create the vdPlayer as a child of 'ldCloud' */

  vdMask = VDeviceClassValue | VDeviceSourcesValue |VDeviceEventMaskValue;
  vdPlayerAttrs.eventmask = DeviceNotifyMask | CommandNotifyMask;
  vdPlayerAttrs.dclass = AA_CLASS_PLAYER;
  vdPlayerAttrs.sources = 1;
  vdPlayerAttrs.sourceslist = &source1;
  source1.encoding = AA_8_BIT_ULAW;
  source1.samplerate = glSoundSampleRate;
  source1.samplesize = 8;
  vdPlayer = 
    ACreateVDevice(pAserver, ldCloud, vdMask, &vdPlayerAttrs, ACurrentTime);
  /* create  a speaker(output) device as child of 'ldCloud' */

  vdMask = VDeviceClassValue | VDeviceSinksValue;
  vdSpeakerAttrs.dclass = AA_CLASS_OUTPUT;
  vdSpeakerAttrs.sinks = 1;
  vdSpeakerAttrs.sinkslist = &sink1;
  sink1.encoding = AA_8_BIT_ULAW;
  sink1.samplerate = glSoundSampleRate;
  sink1.samplesize = 8;
  vdSpeaker = 
    ACreateVDevice(pAserver, ldCloud, vdMask,  &vdSpeakerAttrs, ACurrentTime);

  /* create a wire between the player sink and speaker source */
  wireMask = (WireSinkVdIdValue |  WireSinkIndexValue | 
          WireSourceVdIdValue | WireSourceIndexValue);
  wireAttrs.sinkvdid = vdSpeaker;
  wireAttrs.sinkindex = 0;
  wireAttrs.sourcevdid = vdPlayer;
  wireAttrs.sourceindex = 0;
  wire = ACreateWire(pAserver, wireMask, &wireAttrs, ACurrentTime);
  return(vdPlayer);
}
/*
 * Fill up the loop buffer to the size specified in lLimit.
 */
static void
FillLoopBuffer(data, pAserver, sdSound, sdSoundType, 
		    psSoundName, lBytesRead, lLimit)
	AuData data;
        AudioServer *pAserver;
        SoundId sdSound;
        aSoundType sdSoundType;
        char *psSoundName;
        long lBytesRead;
        long lLimit;
{
    Bool bDone = False;
    long lGot;
    long lTotalGot = 0;
    char *psBuf = (char *)malloc(glLoopBufSize);

    /* fill the loop buffer */
    while (!bDone && glClientPos+lBytesRead <= lLimit )
    {

	/* read the first length of the file into the loop buffer */
	lTotalGot = 0;

	while (lTotalGot != lBytesRead)
	{
	    /*
	    lGot = read(iFd, &psBuf[lTotalGot], lBytesRead-lTotalGot);
	    */
	    lGot = MIN(data->len - index_into_data, lBytesRead-lTotalGot);
	    bcopy(data->data + index_into_data,
		  &psBuf[lTotalGot],
		  lGot);
	    index_into_data += lGot;

	    if (lGot == 0)
		break;
	    lTotalGot += lGot;
	};



	if (lTotalGot > 0)
	{
	    APutSoundData(pAserver, sdSound, SOUNDOP, glClientPos, 
	        sdSoundType, lTotalGot, 
	        (SoundData) psBuf, ACurrentTime);
	    glDataSent = (glClientPos += lTotalGot);
	}

	if (lGot == 0)
	    bDone = True;
	else if (lGot==-1)
	{
	    fprintf(stderr, catgets( catalog, CAT_SHELL, 12, "Error on read of %s\n" ), psSoundName);
	    exit(-1);
	}
    }

    /* just in case glClientPos is == glLoopBufSize */
    if (glClientPos == glLoopBufSize)
	glClientPos = 0;

    free (psBuf);
}

/*
 * Handles CommandStart and SoundPosition event. 
 */
static void
HandleEvent(pAserver, pEvent)
	AudioServer *pAserver;
	aEvent *pEvent;
{
  aEvent event;

  ANextEvent(pAserver, &event);
  if(event.u.u.type==N_CommandStart) 
    {
      glPrevServPos = glCurrServPos = 0;
    } 
  else if(event.u.u.type==N_SoundPosition) 
    {
      glPrevServPos = glCurrServPos;
      glCurrServPos = (int)event.u.soundPositionN.position;
      glDataPlayed += MOD(glCurrServPos-glPrevServPos, glLoopBufSize);
      if (gbVerbose)
	{
	  fprintf(stdout,".");
	  fflush(stdout);
	}
    }

  if (pEvent)
    bcopy(&event, pEvent, sizeof(aEvent));
}

/*
 * At each SoundPosition event coming back, the client will read 4000 bytes
 * and send them to the server. 
 */
static void 
SendAtPosEvent(data, pAserver, sdSoundType, sdSound, psSoundName)
    AuData data;
    register AudioServer *pAserver;
    aSoundType sdSoundType;
    SoundId   sdSound;
    char *psSoundName;
{
  aEvent event;
 
  Bool bDone = False;
  Bool bNeedToRead = True;
  long lGot;
  long lTemp;

  SoundData psBuf = (SoundData )malloc(MAX_SOUND_SAMPLES);
    
  while(!bDone) 
    {
      HandleEvent(pAserver, &event);
      if(event.u.u.type==N_SoundPosition)
	{




	  if (bNeedToRead)
	  {
	    /* read as much as possible into the buffer */
	    /* lGot = read(iFd, psBuf, MAX_SOUND_SAMPLES); */
	    lGot = MIN(data->len - index_into_data, MAX_SOUND_SAMPLES);
	    bcopy(data->data + index_into_data,
		  psBuf,
		  lGot);
	    index_into_data += lGot;

	    /* if end of file, break out the loop */
	    if (lGot == 0)
	      break;
	    else if (lGot==-1) 
	      {
		fprintf(stderr, catgets( catalog, CAT_SHELL, 12, "Error on read of %s\n" ), psSoundName);
		exit(-1);
	      }

	    bNeedToRead = False;
          }

	     /* Continue if we are too far ahead of the server.  Do not put this sound yet.
	      * Wait for the next position event and try again.
	      * Note: This avoids making the server have to split a bucket.
	      */
	  lTemp = MOD(glClientPos+lGot+glLoopGuard,glLoopBufSize);
	  if (lTemp < glClientPos) { /* wrapped */
	      if  ((glCurrServPos >= glClientPos) || (glCurrServPos <= lTemp)) continue;
	  } else { /* not wrapped */
	      if  ((glCurrServPos >= glClientPos) && (glCurrServPos <= lTemp)) continue;
	  }
	  /* flush the big buffer */
	  APutSoundData(pAserver, sdSound, SOUNDOP, 
			glClientPos, sdSoundType, lGot, 
			(SoundData) psBuf, ACurrentTime);
	  AFlush(pAserver);
	  glDataSent += lGot;
	  glClientPos = MOD(glClientPos+lGot,glLoopBufSize);
	  bNeedToRead = True;
	}
    }

  free(psBuf);
}

/*
 * No more input. Plays the remaining samples in the loop buffer.
 */
static void
PlayRemaining(pAserver)
	AudioServer *pAserver;
{
  /* play the remaining data */
  while (glDataPlayed < glDataSent) 
    {
      HandleEvent(pAserver, NULL);
    }

}
/*
 * Plays the client sound file. The client sound file has no audio header
 * in it.
 */
static void
PlayClientSound(pAserver, ldCloud, vdid, sdSound, data, sdSoundType)
	AudioServer *pAserver;
	LoudId ldCloud;
	VDeviceId vdid;
	SoundId sdSound;
	AuData data;
	aSoundType sdSoundType;
{
    struct stat stStat;
    long lBytesRead = 1000;
    char *psSoundName = "?";

#if 0 /* XXX */
    if ((iFd = open(psSoundName, O_RDONLY, 0)) == -1) 
    {
      fprintf(stderr,catgets( catalog, CAT_SHELL, 14, "Cannot open file %s\n" ), psSoundName);
      exit(-1);
    }

    fstat(iFd, &stStat);

    /* round off the optimal IO size */
    if (stStat.st_blksize > 1000)
      lBytesRead = stStat.st_blksize/1000 * 1000;
    else
      lBytesRead = stStat.st_blksize;
#endif /* 0 */
    index_into_data = 0;

    /* reading a client sound file, fill up the loop buffer first 
     * because there are ample of sound samples.
     */
    FillLoopBuffer(data, pAserver, sdSound, sdSoundType, 
		   psSoundName, lBytesRead, glLoopBufSize-MAX_SOUND_SAMPLES);
    /* play the sound now */
    APlay(pAserver, ldCloud, vdid, sdSound, 0, End, ACurrentTime);
    AFlush(pAserver);

    /* send data when receive a position event */
    SendAtPosEvent(data, pAserver, sdSoundType, sdSound, psSoundName, False);

    PlayRemaining(pAserver);

#if 0 /* XXX */
    close(iFd);
#endif /* 0 */
}



/*
 * XXX Not sure what is used here...
 */
static LoudId             ldRootLoud;
static LoudId             ldCloud;
static LoudValueMask      ldValueMask;          
static VDeviceId          vdPlayer;
static SoundId            sdSound;
static aLoudAttributes    ldAttrs;
static char              *psSoundName = NULL;
static SoundValueMask     sdValueMask;
static aSoundHandleAttributes sdAttrs;
static ATime              lRetTime;

static AudioServer *
_really_open_device()
{
    AudioServer *server = AOpenAudio(NULL);
    if (!server)
	return server;

    /*
     * XXX The following is just yanked from playsoundfile.c-- I don't
     * understand it yet...
     */
    ldRootLoud = AGetRootLoudId(server);
    /* create CLOUD (or ldCloud) */
    ldValueMask = LoudEventMaskValue;

    ldAttrs.eventmask = QueueNotifyMask;
    ldCloud = ACreateLoud(server, ldRootLoud, ldValueMask, 
			&ldAttrs, ACurrentTime);

    /* create the vdPlayer as a child of 'ldCloud' */
    vdPlayer = CreatePlayer(server, ldCloud);
    /* map and activate our ldCloud */
    AMapLoud(server, ldCloud, ACurrentTime);

    /* set up loop buffer size to be 4 sec of sample rate */
    glLoopBufSize = glSoundSampleRate << 2;

    /* first, allocate the loop buffer */
    sdValueMask = 
    SoundModeValue | SoundLengthValue | SoundTypeValue;


    sdAttrs.soundmode = soundHandleLoopBuffer | soundHandleRead |
						   soundHandleWrite;
    sdAttrs.length = glLoopBufSize;        
    sdAttrs.soundtype.sampleRate = glSoundSampleRate;
    sdAttrs.soundtype.sampleSize = 8;
    sdAttrs.soundtype.encoding = AA_8_BIT_ULAW;
    /* create the sound handle */
    sdSound = 
	ACreateSoundHandle(server, sdValueMask, &sdAttrs, ACurrentTime);
    return server;
}

/*
 * return 0 on success, -1 on error
 */
extern int
AuOpenTheDevice()
{
    if (times_opened == 0) {
	return (server = _really_open_device()) ? 0 : -1;
    }
    times_opened++;
    return 0;
}
extern void
AuCloseTheDevice()
{
    if (times_opened <= 0)
	return;
    if (--times_opened == 0) {
	ACloseAudio(server);
    }
}

extern int
AuPlay(data)
AuData data;
{
    /*
     * XXX This is probably what we should
     * do anyway, on all systems.
     * The "> /dev/null" part is because for some reason
     * these routines output dots to stdout.
     */


#ifndef MAIN	/* This was really fun when I forgot to #define MAIN... */
    extern int errno;
    char buf[1000];
    int return_val;

    /*
    if (fork())
	return 0;
    freopen("/dev/null", "w", stdout);
    */

    if (data->filename && !getenv("AU_DONT_SYSTEM")) {
	sprintf(buf, "au %s > /dev/null &", data->filename);
	return_val = system(buf);
	errno = 0;
	return return_val ? -1 : 0;
    }

#endif /* !MAIN */

    PlayClientSound(server, ldCloud, vdPlayer, sdSound, data,
		    sdAttrs.soundtype);
#ifndef MAIN
    _exit(0);
#endif /* !MAIN */

    return 0;
}

extern void
AuWaitForSoundToFinish()
{
    /* don't know how! */
}

#endif /* ultrix */



#if !defined(sgi) && !defined(ultrix)

#ifdef hpux
#define DEV_AUDIO "/dev/audioBU"	/* the one that takes 8-bit mulaw */
#else /* !hpux */
#define DEV_AUDIO "/dev/audio"	/* this works on the sparc and maybe elsewhere*/
#endif /* !hpux */

static FILE *audio_fp;
static int times_opened = 0;

extern int
AuOpenTheDevice()
{
    if (times_opened == 0) {
	audio_fp = fopen(DEV_AUDIO, "w");
	if (!audio_fp)
	    return -1;
    }
    times_opened++;
    return 0;
}
extern void
AuCloseTheDevice()
{
    if (times_opened <= 0)
	return;
    if (--times_opened == 0)
	fclose(audio_fp);
}

static int
Fwrite(data, siz, n, fp)
char *data;
FILE *fp;
{
    int nwrote = 0;
    int nwrote_this_time;
    while (n > 0) {
	nwrote_this_time = fwrite(data+nwrote, siz, n-nwrote, fp);
	if (nwrote_this_time <= 0)
	    break;
	nwrote += nwrote_this_time;
    }
    return nwrote;
}

extern int
AuPlay(data)
AuData data;
{
    int n, device_was_open;

    device_was_open = (audio_fp != 0);
    if (!device_was_open)
	if (AuOpenTheDevice() < 0)
	    return -1;

    n = Fwrite(data->data, 1, data->len, audio_fp);
    fflush(audio_fp);

    if (!device_was_open) {
	AuWaitForSoundToFinish();
	AuCloseTheDevice();
    }

    return n == data->len ? 0 : -1;
}

extern void
AuWaitForSoundToFinish()
{
    /* don't know how! */
}

#endif /* !sgi && !ultrix */

#ifdef MAIN

main(argc, argv)
char **argv;
{
    AuData data;

#ifdef ultrix
    /*
     * XXX until I figure out how to make it stop giving those dots...
     */
    (void) freopen("/dev/null", "w", stdout);
#endif /* ultrix */

    if (argc == 1 && isatty(fileno(stdin)))
	goto usage;

    if (getenv("AU_STDIN_OK") && argc == 1) {
	data = AuReadFp(stdin);
	if (!data)
	    perror("stdin"), exit(1);
    } else if (argc == 2) {
	data = AuReadFile(argv[1]);
	if (!data)
	    perror(argv[1]), exit(1);
    } else
	goto usage;

    if (AuOpenTheDevice() < 0)
	perror("AuOpenTheDevice"), exit(1);

    if (AuPlay(data) < 0)
	perror("AuPlay"), exit(1);;

    AuWaitForSoundToFinish();

    AuCloseTheDevice();

    AuDestroy(data);

    return 0;

usage:
    fprintf(stderr, "Usage: %s filename\n", argv[0]);
    exit(1);
}

#endif /* MAIN */

/*
 * The following stuff should be in another file, zm_au.c or something...
 */
#ifndef I_WISH_THOSE_GUYS_MADE_AU_MODULAR

/* zm_sound() -- support for the "sound" command.  Load and play sounds.
   options:
	usage: sound [filename]
		     [-event type filename]
		     [-command type filename]
		     [-action type filename]
		     [off [action] ]
   if filename is given, just play it. (wait for it)
   if -action or -event is given, just load the sound and link it up
      to a list.
   if -event is given, add an "action hook", which is Xt's method of
      calling us whenever a widget's action routine is called as a
      result of an event.  (eg. button clicking causes an "Activate" event)
   -command is used to associate sounds with arbitrary zmail commands.
   -action is used to associate sounds with arbitrary zmail actions such as
   help or error.  e.g., "help" might play recorded help -- "error" might
   choose to look up the "error" sound and play it.  These are actions that
   may not be associated directly with z-script commands.

   Currently, you can't have more than one sound "name" (tag) associated
   with any of event/action/command list.  This seems to be a problem with
   retrieve_link() in that it can't get non-unique links.
 */
zm_sound(argc, argv)
int argc;
char *argv[];
{
    char *opt, *type, *action, *sound = NULL, *cmd = *argv;
    SoundAction *sa;
#ifdef GUI
    static XtActionHookId sound_hook;
#endif /* GUI */

    if (!*++argv) {
usage:
	print(catgets( catalog, CAT_SHELL, 19, "usage: %s [filename] [-event type filename]\n" ), cmd);
	print(catgets( catalog, CAT_SHELL, 20, "\t[-action type filename] [-command type filename]\n" ));
	print(catgets( catalog, CAT_SHELL, 21, "\t[-off [type] ]\n" ));
	return -1;
    }

    if (!strcmp(*argv, "-off")) {
	if (!argv[1]) {
#ifdef GUI
	    if (istool) {
		if (sound_hook) {
		    XtRemoveActionHook(sound_hook);
		    sound_hook = 0;
		}
		AuCloseTheDevice();
	    }
#endif /* GUI */
	    while (sa = sound_actions) {
		remove_link(&sound_actions, sa);
		xfree(sa->link.l_name);
		AuDestroy(sa->sound_data);
		xfree(sa);
	    }
	} else while (*++argv) {
	    if (!(sa = retrieve_sound(sound_actions, *argv, 0)))
		error(UserErrWarning, catgets( catalog, CAT_SHELL, 22, "No such sound: %s" ), *argv);
	    else {
		remove_link(&sound_actions, sa);
		xfree(sa->link.l_name);
		AuDestroy(sa->sound_data);
		xfree(sa);
		if (istool && !sound_actions) {
#ifdef GUI
		    if (sound_hook) {
			XtRemoveActionHook(sound_hook);
			sound_hook = 0;
		    }
#endif /* GUI */
		    AuCloseTheDevice();
		}
	    }
	}
    } else if (argv[0][0] == '-' &&
	    (argv[0][1] == 'e' || argv[0][1] == 'a' || argv[0][1] == 'c')) {
	SoundAction *sa1;
	opt = *argv;
	if (!(action = *++argv) || !(sound = *++argv))
	    goto usage;
	sa1 = retrieve_sound(sound_actions, action, 0);
	if (!(sa = zmNew(SoundAction))) {
	    error(ZmErrWarning, catgets( catalog, CAT_SHELL, 23, "cannot create new sound" ));
	    return -1;
	}
	if (!(sa->sound_data = AuReadFile(sound))) {
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 24, "Cannot read sound file \"%s\"" ), sound);
	    free(sa);
	    return -1;
	}
	sa->link.l_name = savestr(action);
	switch(opt[1]) {
	    case 'e' : sa->type = AuEvent;
	    when 'a' : sa->type = AuAction;
	    when 'c' : sa->type = AuCommand;
	    otherwise : goto usage;
	}
	insert_sorted_link(&sound_actions, sa, 0);
	if (sa1) {
	    remove_link(&sound_actions, sa1);
	    xfree(sa1->link.l_name);
	    AuDestroy(sa1->sound_data);
	    xfree(sa1);
	}
	/* if there are *any* event sounds, add action hooks. */
#ifdef GUI
	if (istool && opt[1] == 'e' && !sound_hook) {
	    /* Yuk: dependent on Xt */
	    sound_hook = XtAppAddActionHook(app, sound_event, sound_actions);
	}
#endif /* GUI */
	if (AuOpenTheDevice() < 0)
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 25, "Cannot open audio device" ));
    } else if (*argv) {
	if (AuOpenTheDevice() < 0)
	    error(SysErrWarning, catgets( catalog, CAT_SHELL, 25, "Cannot open audio device" ));
	else {
	    do  {
		AuData data = AuReadFile(*argv);
		if (data) {
		    AuPlay(data);
		    AuWaitForSoundToFinish();
		    AuDestroy(data);
		} else {
		    error(SysErrWarning, catgets( catalog, CAT_SHELL, 24, "Cannot read sound file \"%s\"" ), *argv);
		}
	    } while (*++argv);
	    AuCloseTheDevice();
	}
    }
}

extern int
retrieve_and_play_sound(type, name)
SoundType type;
char *name;
{
    int return_value;
    SoundAction *sa = retrieve_sound(sound_actions, name, ci_strcmp);

    if (!sa || sa->type != type)
	return -1;
    /*
    if (AuOpenTheDevice() < 0)
	return -1;
    */
    return_value = AuPlay(sa->sound_data);
    /*
    AuCloseTheDevice();
    */
    return return_value;
}

#ifdef GUI
void
sound_event(widget, client_data, action_name, event, args, num_args)
Widget widget;
XtPointer client_data;
String action_name;
XEvent *event;
String *args;
int *num_args;
{
    SoundAction *sa = retrieve_sound((SoundAction *)
	client_data, action_name, ci_strcmp);

    if (sa && sa->type == AuEvent) {
	Debug("playing audio %s for widget \"%s\"\n" ,
	    action_name, XtName(widget));
	AuOpenTheDevice();
	(void) AuPlay(sa->sound_data);
	AuCloseTheDevice();
    }
}
#endif /* GUI */
#endif /* !I_WISH_THOSE_GUYS_MADE_AU_MODULAR */








#endif /* AUDIO */
