#ifndef INCLUDE_ZM_FAM_H
#define INCLUDE_ZM_FAM_H


#include "config/features.h"
#ifdef USE_FAM


#include <fam.h>

typedef void (*FAMCallbackProc)(FAMEvent *, void *);
    
typedef struct {
    FAMCallbackProc callback;
    void *data;
} FAMClosure;

extern void FAMDispatch(FAMConnection **);
extern void FAMError(FAMConnection **);


extern FAMConnection *fam;
extern FAMRequest i18n_fam_request;


#endif /* USE_FAM */


#endif /* !INCLUDE_ZM_FAM_H */
