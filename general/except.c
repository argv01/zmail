/*
 * $RCSfile: except.c,v $
 * $Revision: 2.25 $
 * $Date: 1995/07/14 04:11:54 $
 * $Author: schaefer $
 *
 * Exception-handling for C
 * Derived from original work by Mike McInerny and Mike Horowitz
 * at the Information Technology Center, Carnegie Mellon University
 */

#include <except.h>
#include <zcsig.h>

#ifndef HAVE_SIGLIST
# include <sigarray.h>
#endif /* HAVE_SIGLIST */

static const char except_rcsid[] =
    "$Id: except.c,v 2.25 1995/07/14 04:11:54 schaefer Exp $";

RETSIGTYPE (**_except_SigHandlerList)(); /* make sure this matches
					  * the declaration in except.h */

struct except_HandlerContext *except_CurrentContext = 0;
const char *except_ExceptionID = 0;
GENERIC_POINTER_TYPE *except_ExceptionValue = 0;
void (*except_UncaughtHandler)() = 0;

const char *
except_GetRaisedException()
{
    return (except_ExceptionID);
}				/* except_GetRaisedException */

void
except_SetExceptionValue(v)
    GENERIC_POINTER_TYPE *		    v;
{
    except_ExceptionValue = v;
}				/* except_SetExceptionValue */

GENERIC_POINTER_TYPE *
except_GetExceptionValue()
{
    return (except_ExceptionValue);
}				/* except_GetExceptionValue */

void
except_PushContext(context)
    struct except_HandlerContext *context;
{
    context->nested = except_CurrentContext;
    except_CurrentContext = context;
}				/* except_PushContext */

int
except_CheckException(xid, context, flags)
    const char *xid;
    struct except_HandlerContext *context;
    int                    *flags;
{
    if ((strcmp(xid, except_ExceptionID) == 0) ||
	(strcmp(xid, except_ANY) == 0)) {
	except_CurrentContext = context->nested;
	*flags |= except_Handled;

	return (1);
    }
    return (0);
}				/* except_CheckException */

void
except_ResetContext(context)
    struct except_HandlerContext *context;
{
    except_CurrentContext = context;
}				/* except_ResetContext */

struct except_HandlerContext *
except_GetCurrentContext()
{
    return (except_CurrentContext);
}				/* except_GetCurrentContext */

static void
except_DefaultHandler()
{
    static int              i = 0;

    /* something that is guaranteed to cause an uncaught signal */
    i = 3 / i;
}				/* except_DefaultHandler */

int
except_RAISE(xid, value)
    const char *xid;
    GENERIC_POINTER_TYPE *value;
{
    if (xid) {
	except_ExceptionID = xid;
	except_ExceptionValue = value;
    }
    if (except_CurrentContext) {
	LONGJMP(except_CurrentContext->env, except_Raised);
    } else if (except_UncaughtHandler) {
	(*except_UncaughtHandler) ();
    } else {
	except_DefaultHandler();
    }

    return (0);
}				/* except_RAISE */

static RETSIGTYPE
SigHandler(sig)
#ifndef WIN16
    int sig;
#else
    short sig;
#endif /* !WIN16 */
{
#ifndef WIN16
    signal(sig, SigHandler);	/* reinstall this handler */
#else
    signal(sig, (RETSIGTYPE (*) P((short))) SigHandler);
#endif /* !WIN16 */
    RAISE(strsignal(sig), 0);
}

void
_except_HandleSignals(VA_ALIST(int sig))
    VA_DCL
{
    VA_LIST ap;
#ifndef WIN16
    VA_ZLIST(int sig);
#else /* WIN16 */
    VA_ZLIST(short sig);
#endif /* !WIN16 */
    int i;

    for (i = 1; i < MAXSIGNUM; ++i) {
	_except_SigHandlerList[i] = (RETSIGTYPE (*) ()) 0;
    }
    VA_START(ap, int, sig);
    do {
	_except_SigHandlerList[sig] = signal(sig, SigHandler);
    } while ((sig = VA_ARG(ap, int)) > 0);
    VA_END(ap);
}
    
void
_except_RestoreSigHandlers(handlers)
    except_sighandlerlist_t handlers;
{
#ifndef WIN16
    int i;
#else /* WIN16 */
    short i;
#endif /* WIN16 */

    for (i = 1; i < MAXSIGNUM; ++i) {
	if (handlers[i]) {
	    signal(i, handlers[i]);
	    handlers[i] = 0;
	}
    }
}

void (*except_GetUncaughtExceptionHandler())()
{
    return (except_UncaughtHandler);
}				/* except_GetUncaughtExceptionHandler */

#ifdef MAC_OS
# include "zminit.seg"
#endif /* MAC_OS */
void
except_SetUncaughtExceptionHandler(h)
    void (*h)();
{
    except_UncaughtHandler = h;
}				/* except_SetUncaughtExceptionHandler */
