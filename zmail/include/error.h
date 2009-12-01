/* error.h	Copyright 1995 Z-Code Software, a Divison of NCD */

#ifndef _ERROR_H_
# define _ERROR_H_

#include <general.h>

/*
 * error.h
 * For programs that want to use error but don't want to
 * include zmail.h.
 * It would be helpful if zmail just included this...
 */

/* Copyright 1992 Z-Code Software Corp. */

typedef enum {
    SysErrFatal,
    SysErrWarning,
    ZmErrFatal,
    ZmErrWarning,
    UserErrFatal,
    UserErrWarning,
    QueryChoice,
    QueryWarning,
    HelpMessage,
    Message,
    UrgentMessage,
    ForcedMessage
} PromptReason;

extern void error VP((PromptReason, const char *, ...));

#endif /* _ERROR_H_ */
