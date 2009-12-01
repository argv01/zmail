/* callback.h	Copyright 1993, Z-Code Software Corp. */

#ifndef INCLUDE_CALLBACK_H
#define INCLUDE_CALLBACK_H

#include <general.h>
#include "linklist.h"


typedef struct zmCallbackList {
    const char *name;
    int type;
    struct zmCallback *list;
} zmCallbackList;

typedef struct zmCallback {
    struct link c_link;
    struct zmCallback *chain;
    zmCallbackList *parent;
    void (*routine)();
    char *data;
} zmCallback;

typedef struct zmCallbackData {
    int event;
    VPTR xdata;
} zmCallbackData;

#define ZCBTYPE_VAR     1
#define ZCB_VAR_SET	1
#define ZCB_VAR_UNSET   2
#define ZCBTYPE_CHROOT  2 /* called whenever chroot is done */
#define ZCBTYPE_FILTER	3 /* called whenever filter list is modified */
#define ZCBTYPE_FUNC	4
#define ZCB_FUNC_ADD	1
#define ZCB_FUNC_DEL	2
#define ZCB_FUNC_SEL	3
#define ZCB_CANCEL      -1
#define ZCBTYPE_MENU	5
#define ZCBTYPE_ATTACH  6 /* called whenever an attach -rehash is done */
#define ZCBTYPE_ADDRESS 7 /* called whenever subject or addressees change */
#define ZCBTYPE_PRUNE	8 /* called whenever attachments are (un)deleted */
#define ZCB_PRUNE_DELETE   1
#define ZCB_PRUNE_UNDELETE 2

#define ZCBChain(X) ((X)->chain)

typedef struct zmCallback *ZmCallback;
typedef struct zmCallbackList *ZmCallbackList;
typedef struct zmCallbackData *ZmCallbackData;

ZmCallback ZmCallbackAdd P((const char *, int, void (*)(), VPTR));
void ZmCallbackCallAll P((const char *, int, int, VPTR));
void ZmCallbackRemove P((ZmCallback));


#endif /* INCLUDE_CALLBACK_H */
