// -*- c++ -*-
#pragma once

#ifdef HAVE_SSDEFAULTSCHEME_H
#include <soundscheme/ssDefaultScheme.h>
#else // !HAVE_SSDEFAULTSCHEME_H

#define SS_BEEP		    "Beep"
#define SS_BEGIN_WAIT	    "BeginWait"		// may not yet exist
#define SS_COMPLETION	    "Completion"
#define SS_CONTINUE_WAIT    "ContinueWait"	// may not yet exist
#define SS_DROP_COPY	    "DropCopy"
#define SS_DROP_IGNORE	    "DropIgnore"
#define SS_DROP_MOVE	    "DropMove"
#define SS_DROP_REFERENCE   "DropReference"
#define SS_END_WAIT	    "EndWait"		// may not yet exist
#define SS_ERROR	    "Error"
#define SS_FATAL_ERROR	    "FatalError"
#define SS_INFO		    "Info"
#define SS_LAUNCH	    "Launch"
#define SS_LOST_KEYSTROKE   "LostKeystroke"	// may not yet exist
#define SS_QUESTION	    "Question"
#define SS_WARNING	    "Warning"

#endif // !HAVE_SSDEFAULTSCHEME_H
