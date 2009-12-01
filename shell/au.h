/* au.h		Copyright 1992 Z-Code Software Copr.  All Rights Reserved. */

#ifndef _AU_H_
#define _AU_H_

typedef struct {
    char *filename;
    long len;
    unsigned char data[1];
} *AuData;

extern AuData AuReadFile(/* char * */);
extern AuData AuReadFp(/* FILE * */);
extern void AuDestroy(/* AuData */);
extern int AuPlay(/* AuData */);
extern void AuWaitForSoundToFinish();

extern int AuOpenTheDevice();
extern void AuCloseTheDevice();

#ifndef I_WISH_THOSE_GUYS_MADE_AU_MODULAR
#ifndef MAC_OS
#include "shell/linklist.h"
#else
#include "linklist.h"
#endif /* !MAC_OS */

typedef enum {
    AuAction,
    AuCommand,
    AuEvent
} SoundType;

typedef struct _sound_actions {
    struct link link;
    SoundType type;
    AuData sound_data;
} SoundAction;

extern SoundAction *sound_actions;

#define retrieve_sound (SoundAction *)retrieve_link

extern void sound_event();
#endif /* !I_WISH_THOSE_GUYS_MADE_AU_MODULAR */

#endif /* _AU_H_ */
