/* zm_ask.h	Copyright 1993 Z-Code Software Corp. */

/* This stuff gets its own header file because it needs to be declared
 * at different points of the compilation depending on whether Motif is
 * defined and/or whether prototyping has to be used.
 */

#ifndef _ZM_ASK_H_
#define _ZM_ASK_H_

#include "error.h"
#include "config/features.h"

/*
 * The first argument to gui_ask() specifies the type of ask dialog,
 * as well as the default answer, as follows:
 *
 * WarnYes: warning dialog with "Yes" and "No" buttons, default Yes
 * WarnNo:  warning dialog with "Yes" and "No" buttons, default No
 * WarnOk:  warning dialog with "Ok" and "Cancel" buttons, default Ok
 * WarnCancel: warning dialog with "Ok" and "Cancel" buttons, default Cancel
 * AskOk:   question dialog with "Yes" and "No" buttons, default "Yes"
 * AskUnknown: question dialog with Yes/No/Cancel, no default
 * AskYes:  question dialog with Yes/No/Cancel, default Yes
 * AskNo:   question dialog with Yes/No/Cancel, default No
 * AskCancel:  question dialog with Yes/No/Cancel, default Cancel
 * SendSplit:  question dialog with Send Split/Send Whole/Cancel, default Split
 * SendWhole:   question dialog with Send Split/Send Whole/Cancel, default Whole
 * SendCancel:  question dialog with Send Split/Send Whole/Cancel, default Cancel
 */
typedef enum AskAnswer {
    AskUnknown,
    AskYes,
    AskNo,
    AskOk,
    AskCancel,
    WarnYes,
    WarnNo,
    WarnOk,
    WarnCancel
#ifdef PARTIAL_SEND
    , SendSplit,
    SendWhole,
    SendCancel
#endif /* PARTIAL_SEND */
} AskAnswer;

extern AskAnswer ask VP((AskAnswer, const char *, ...));

#ifdef GUI

extern AskAnswer gui_ask P((AskAnswer, const char *));
extern void gui_error P((PromptReason, const char *));

#endif /* GUI */

#endif /* _ZM_ASK_H_ */
