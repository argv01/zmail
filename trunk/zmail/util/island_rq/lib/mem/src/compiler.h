#ifndef _COMPILER_H
#define _COMPILER_H

#ifdef unix
#ifdef SABER
#define Header(name, args)	name args
#else /* !SABER */
#define Header(name, args)	name ()
#endif /* SABER */
#else /* !unix */
#define Header(name, args)	name args
#endif /* unix */

#define INTERNAL

#ifndef far
#define far
#endif /* far */

#ifndef huge
#define huge
#endif /* !huge */

#endif /* _COMPILER_H */
