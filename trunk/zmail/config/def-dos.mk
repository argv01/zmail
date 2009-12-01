# def-dos.mk	Copyright 1992 Z-Code Software Corp.

#
# Command names
#

RENAME =      mv

#
# Extensions
#
# These macros define DOS-specific file extensions.
#

O=obj				# Object modules
E=.exe				# Final executable binary
S=asm				# Assembly source
ZRC=$(DOSZRC)			# Z-Mail resource configuration files

#
# Source files and corresponding objects
#

DOS_HDRS =	\
	dos/include/dirent.h	\
	dos/include/msdos.h	\
	dos/include/pwd.h	\
	dos/include/stubs.h	\
	$(NULL)

DOS_SRC =	\
	dos/dirent.c		\
	dos/fsfix.c		\
	dos/getchar.c		\
	dos/popen.c		\
	dos/pwd.c		\
	dos/stubs.c		\
	$(NULL)

DOS_OBJ =	\
	dos/dirent.$O		\
	dos/fsfix.$O		\
	dos/getchar.$O		\
	dos/popen.$O		\
	dos/pwd.$O		\
	dos/stubs.$O		\
	$(NULL)

#
# Utilities
#

DOS_UTIL_SRC =	\
	dos/util/make.ini	\
	dos/util/makefile	\
	dos/util/sendmail.c	\
	$(NULL)

DOS_UTILITIES = \
	dos/util/sendmail.exe	\
	$(NULL)
