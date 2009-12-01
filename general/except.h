/*
 * $RCSfile: except.h,v $
 * $Revision: 2.29 $
 * $Date: 1995/10/26 20:53:19 $
 * $Author: liblit $
 *
 * Exception-handling for C
 * Derived from original work by Mike McInerny and Mike Horowitz
 * at the Information Technology Center, Carnegie Mellon University
 */

#ifndef EXCEPT_H
# define EXCEPT_H

# include <general.h>
# include <maxsig.h>

# include <zcjmp.h>

# ifdef HAVE_STRSIGNAL
#  include <zmstring.h>
# else /* !HAVE_STRSIGNAL */
#  ifndef HAVE_SYS_SIGLIST
extern char *sys_siglist[];
#  endif /* HAVE_SYS_SIGLIST */
#  define strsignal(n) (sys_siglist[(n)])
# endif /* !HAVE_STRSIGNAL */

# define except_ANY "except_ANY"

/* Simpler interface to ANY */
# define ANY except_ANY

/* Simpler interfaces to RAISE, ASSERT, and PROPAGATE */
# define RAISE(x, v) except_RAISE(x, v)
# ifndef USE_NATIVE_ASSERT
# define ASSERT(condition, xid, value) except_ASSERT(condition, xid, value)
# endif
# define PROPAGATE() except_PROPAGATE()

/* Simpler interfaces to except_{begin,end,for,while,do} */
# define EXC_BEGIN except_begin
# define EXC_END except_end
# define EXC_FOR except_for
# define EXC_WHILE except_while
# define EXC_DO except_do

/* Simpler interfaces to except_{break,continue,return,returnvalue} */
# define EXC_BREAK except_break
# define EXC_CONTINUE except_continue
# define EXC_RETURN except_return
# define EXC_RETURNVAL(t,e) except_returnvalue(t,(e))

# define DECLARE_EXCEPTION(n) extern const char n[]
# define DEFINE_EXCEPTION(n,s) const char n[] = s

/*
   PRIVATE STRUCTURE
	Used to deal with a stack of current handler contexts.
	The head of the stack is held by except_CurrentContext.
 */
struct except_HandlerContext {
    struct except_HandlerContext *nested;
    JMP_BUF env;
};

/*
   PRIVATE FLAGS
	except_Raised indicates an exception was raised.
	except_Handled indicates the exception was handled
		in the current handler context.

   INVARIANT:	Flags may have except_Handled set only if
		except_Raised is also set.
 */

# define except_Raised	(1<<0)
# define except_Handled	(1<<1)


#ifdef WIN16
typedef RETSIGTYPE (*except_sighandlerlist_t[MAXSIGNUM+1]) NP((short));
extern RETSIGTYPE (**_except_SigHandlerList) NP((short));
#else /* WIN16 */
typedef RETSIGTYPE (*except_sighandlerlist_t[MAXSIGNUM+1]) NP((int));
extern RETSIGTYPE (**_except_SigHandlerList) NP((int));
#endif /* WIN16 */

extern void _except_RestoreSigHandlers P((except_sighandlerlist_t));
extern void _except_HandleSignals(VA_PROTO(int sig));

# define TRYSIG(list) \
    { except_sighandlerlist_t _except_sighandlers; \
      int _except_flags; \
      struct except_HandlerContext _except_context; \
      _except_SigHandlerList = _except_sighandlers; \
      _except_HandleSignals list; \
      except_PushContext(&_except_context); \
      if ((_except_flags = SETJMP(_except_context.env)) == 0) {

# define EXCEPTSIG(sig) \
      } else if (except_CheckException(strsignal(sig), &_except_context, &_except_flags)) {

# define FINALLYSIG \
      } \
      _except_RestoreSigHandlers(_except_sighandlers); \
      except_ResetContext(_except_context.nested); \
      {

# define ENDTRYSIG \
      } \
      _except_RestoreSigHandlers(_except_sighandlers); \
      except_ResetContext(_except_context.nested); \
      if (_except_flags == except_Raised) { \
          except_RAISE((VPTR)0, (VPTR)0); \
      } \
    }

# ifndef USE_NATIVE_ASSERT         
# define TRY \
	{ int _except_flags; \
	  struct except_HandlerContext _except_context; \
	  except_PushContext(&_except_context); \
	  if ((_except_flags = SETJMP(_except_context.env)) == 0) {

# define EXCEPT(xid) \
          } else if (except_CheckException(xid, &_except_context, &_except_flags)) {

# define FINALLY \
          } \
	  except_ResetContext(_except_context.nested); \
	  {

# define ENDTRY \
	  } \
	  except_ResetContext(_except_context.nested); \
	  if (_except_flags == except_Raised) { \
	      except_RAISE((VPTR)0, (VPTR)0); \
          } \
        }
# endif /* USE_NATIVE_ASSERT */

# define except_ExceptionRaised() (_except_flags & except_Raised)
# define except_ExceptionHandled() (_except_flags & except_Handled)

# define except_ASSERT(condition, xid, value) \
	((condition) || except_RAISE(xid, value))
# define except_PROPAGATE() \
	{ if (except_ExceptionRaised()) except_RAISE((VPTR)0, (VPTR)0); }

extern const char *except_GetRaisedException P((void));
extern void  except_SetExceptionValue P((GENERIC_POINTER_TYPE *));
extern GENERIC_POINTER_TYPE *except_GetExceptionValue P((void));
extern void  except_SetUncaughtExceptionHandler
              P((void (*) ()));
extern void (*except_GetUncaughtExceptionHandler())P((void));
extern int except_RAISE P((const char *, GENERIC_POINTER_TYPE *));

/*
   LOCAL NON-EXPORTED ROUTINES USED BY THE MACROS ABOVE -- USE MACROS INSTEAD!
 */
extern void  except_PushContext P((struct except_HandlerContext *));
extern int   except_CheckException P((const char *,
				      struct except_HandlerContext *,
				      int *));
extern void  except_ResetContext P((struct except_HandlerContext *));
extern struct except_HandlerContext *except_GetCurrentContext P((void));

/***************************************************************

	|-| Name:	except_begin, except_end, except_for,
			except_while, except_do

	|-| Abstract:	Bracketing constructs for routines containing TRY
			statements with embedded returns or loop statements
			containing TRY statements with embedded continues or
			breaks.

			TRY statements may be nested.  Embedded return or
			continue or break statements must be replaced by
			except_return, except_continue, or except_break
			(see above).  This does NOT apply to break statements
			for switch statements.

	|-| Parameters:	None

	|-| Results:	None

	|-| Side-effects:
			Sets up appropriate state

	|-| Exceptions:	None

	|-| Log:
	    19 Jun 90	mlh	Created.

***************************************************************/

/*
   Use this to replace the open bracket { at the start of a routine
   containing a TRY statement with an embedded except_return statement.
 */
# define except_begin \
	{ struct except_HandlerContext *_except_routinecontext = \
		except_GetCurrentContext();

/*
   Use this to replace the close bracket } at the end of a routine to
   match the corresponding except_begin.

   Similarly, use this to match any of except_for, except_while, or except_do
   when used under the circumstances described below.  Note that in this case
   the except_end does not replace a close bracket if an open bracket is used.
 */
# define except_end }

/*
   Use these to replace for, while, or do when such statements contain a
   TRY statement with an embedded except_break or except_continue.  Such use
   must be terminated with a matching except_end.  If the loop requires
   { } bracketing, the except_end does NOT replace the closing } bracket!
 */
# define except_for \
	{ struct except_HandlerContext *_except_loopcontext = \
		except_GetCurrentContext(); for
# define except_while \
	{ struct except_HandlerContext *_except_loopcontext = \
		except_GetCurrentContext(); while
# define except_do \
	{ struct except_HandlerContext *_except_loopcontext = \
		except_GetCurrentContext(); do

/***************************************************************

	|-| Name:	except_break, except_continue, except_return,
			except_returnvalue

	|-| Abstract:	These are non-local exits from a TRY clause.
			They should NOT be used for loops totally
			embedded within TRY clauses.

			except_break and except_continue for loops that
			contain a TRY statement.

			except_return(value) for returning from a routine
			from within a TRY statement.

	|-| Parameters:
		t	type for return statement value.

		e	value for return statement;
			if value expression may raise an exception
			during its evaluation, better to store
			the result in a local and return the local,
			otherwise the exception will not be handled
			by the handler context established by the
			containing TRY statement.

	|-| Results:	None

	|-| Side-effects:
			As one might expect.

	|-| Exceptions:	See note for parameter e above.

	|-| Log:
	    27 Mar 90	mlh	Created.

***************************************************************/

# define except_break \
	{ except_ResetContext(_except_loopcontext); break; }
# define except_continue \
	{ except_ResetContext(_except_loopcontext); continue; }
# define except_returnvalue(t, e) \
	{ t _rval = e; \
	  except_ResetContext(_except_routinecontext); \
	  return _rval; }
# define except_return \
	{ except_ResetContext(_except_routinecontext); return; }

#endif				/* EXCEPT_H */
