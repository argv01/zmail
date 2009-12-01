/* bytes.h	Copyright 1994 Z-Code Software, a Division of NCD */

#ifndef _BYTES_H_
# define _BYTES_H_

# ifdef MAC_OS
#  include "Memory.h"
# else /* !MAC_OS */
#  ifdef MSDOS
typedef char		Byte;
#  else /* !MSDOS */
typedef unsigned char	Byte;
#  endif /* !MSDOS */
#  define Boolean	Byte
# endif /* MAC_OS */

#endif /* !_BYTES_H_ */
