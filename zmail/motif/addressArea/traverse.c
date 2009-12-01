/* traverse.c	Copyright 1994 Z-Code Software, a Divison of NCD */

#include "osconfig.h"
#include "zmstring.h" /* unreasonable include nesting */
#include "addressArea.h"
#include "private.h"
#include "traverse.h"
#include "vars.h"
#include "zm_motif.h"
#include "zmcomp.h"
#include <X11/Intrinsic.h>
#include <Xm/Text.h>
#include <Xm/Xm.h>


void
progress(prompter)
    struct AddressArea *prompter;
{
    if (prompter->progress) {
	switch (prompter->dominant) {

	case uicomp_Unknown:
	    if (!ADDRESSED(prompter->compose, TO_ADDR)) {
		AddressAreaGotoAddress(prompter, uicomp_To);
		break;
	    } else
		flavor_menu_set(prompter, uicomp_To);
	    
	case uicomp_To:
	    if (boolean_val(VarAskcc) && !ADDRESSED(prompter->compose, CC_ADDR)) {
		AddressAreaGotoAddress(prompter, uicomp_Cc);
		break;
	    }

#ifdef VarAskbcc
	case uicomp_Cc:
	    if (boolean_val(VarAskbcc) && !ADDRESSED(prompter->compose, BCC_ADDR)) {
		AddressAreaGotoAddress(prompter, uicomp_Bcc);
		break;
	    }
#endif /* VarAskbcc */

	default:
	    prompter->progress = False;
	    if ((boolean_val(VarAsk) || boolean_val(VarAsksub))
		&& !XmTextGetLastPosition(prompter->subject))
		AddressAreaGotoSubject(prompter);
	    else if (prompter->progressLast)
		SetTextInput(*(prompter->progressLast));
	}
	if (prompter->progressLast && ison(prompter->compose->flags, IS_REPLY)) {
	    prompter->progress = False;
	    SetTextInput(*(prompter->progressLast));
	}
    }
}


static void
traverse_last(target)
    Widget target;
{
    SetTextInput(target);
    XmTextSetCursorPosition(target, XmTextGetLastPosition(target));
}		  


void
AddressAreaGotoSubject(prompter)
    struct AddressArea *prompter;
{
    if (prompter)
	traverse_last(prompter->subject);
}


void
AddressAreaGotoAddress(prompter, flavor)
    struct AddressArea *prompter;
    enum uicomp_flavor flavor;
{
    if (prompter) {
	flavor_menu_set(prompter, flavor);
	traverse_last(prompter->field);
    }
}
