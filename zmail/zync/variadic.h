#ifndef __VARIADIC_H__
#define __VARIADIC_H__

/*
 * Conveniently include the proper header for variadic arguments.
 *
 * Note that this header does not even try to be a uniform front-end
 * to both stdarg and varargs.
 */

#include "config.h"


#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif /* HAVE_STDARG_H */


#endif /* !__VARIADIC_H__ */
